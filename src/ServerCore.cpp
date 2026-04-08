#include <sys/epoll.h>
#include <sys/types.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "Client.hpp"
#include "Logger.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include "Utils.hpp"

Server::Server(const int32_t port, const uint32_t backlogSize,
               const std::string &pwd)
    : _listenSocket(Socket::makeListeningSocket(port)),
      _port(port),
      _backlogSize(backlogSize),
      _pwd(pwd) {
  int sendBufSize = SNDBUF_SIZE;
  int receiveBufSize = RCVBUF_SIZE;

  if (setsockopt(_listenSocket.getFD(), SOL_SOCKET, SO_RCVBUF, &receiveBufSize,
                 sizeof(receiveBufSize)) < 0)
    throw std::runtime_error(
        "Failed to set server listen socket receive buffer size");
  if (setsockopt(_listenSocket.getFD(), SOL_SOCKET, SO_SNDBUF, &sendBufSize,
                 sizeof(sendBufSize)) < 0)
    throw std::runtime_error(
        "Failed to set server listen socket send buffer size");
}

Server::~Server(void) {
  if (_epollFD != -1)
    close(_epollFD);
  if (_epollEvents)
    delete[] _epollEvents;
}

void Server::start(void) {
  if ((_epollFD = epoll_create(_backlogSize)) < 0)
    throw std::runtime_error("Failed to set server");
  _epoll.events = EPOLLIN;
  _epoll.data.fd = _listenSocket.getFD();
  if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _listenSocket.getFD(), &_epoll) < 0)
    throw std::runtime_error("Failed to start polling on listening socket");
  _epollEvents = new struct epoll_event[_backlogSize];
}

// FIXME: Should we store client IP and port to a struct inside Socket *
// (not NULL arguments in accept?)
// int32_t clientFD = accept(_listener->getFD(),
//                           (struct sockaddr *)&CLIENTADDRESS,
//                           &CLIENTADDRESSLENGTH);
// FIXME: Do we need to store connectionPoll struct for later use?
// FIXME: What to do if adding clientSocket->getFD to polling fails?
// FIXME: What to do if accept() fails?
void Server::run(void) {
  while (true) {
    // LOG << "Polling for new connections. Clients: ";
    // LOG << _clients.size();
    _nEpollFDs = epoll_wait(_epollFD, _epollEvents, _backlogSize, POLL_TIME);
    for (int i = 0; i < _nEpollFDs; ++i) {
      // check for disconnected clients and remove them from the map
      if (_epollEvents[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        removeClient(_epollEvents[i].data.fd);
        continue;
      }
      // add new connections
      if (_epollEvents[i].data.fd == _listenSocket.getFD()) {
        while (true) {  // loop until accept() returns -1 and
                        // errno is EAGAIN or EWOULDBLOCK
          int32_t clientFD = accept(_listenSocket.getFD(), NULL, NULL);
          if (clientFD == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              throw std::runtime_error("fatal error accepting clients");
            }
          } else {
            LOG << "New connection accepted on FD: " << clientFD;
            _sockets.try_emplace(clientFD, Socket::makeClientSocket(clientFD));
            _clients.try_emplace(clientFD, Client());

            struct epoll_event connectionPoll{};
            connectionPoll.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
            connectionPoll.data.fd = clientFD;
            if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, clientFD, &connectionPoll) <
                0) {
              removeClient(clientFD);
              LOG << "Failed to add connection to polling list\n";
              continue;
            }
          }
        }
      } else if (_epollEvents[i].events & EPOLLIN) {  // incoming data
        char    buffer[2048] = {};
        auto    it = _sockets.find(_epollEvents[i].data.fd);
        ssize_t received;
        if (it != _sockets.end()) {
          received = it->second.receiveData(buffer, sizeof(buffer));
        } else {
          epoll_ctl(_epollFD, EPOLL_CTL_DEL, _epollEvents[i].data.fd, NULL);
          continue;
        }
        if (received <= 0) {
          std::cerr << "failed to receive\n";
          continue;
        } else {
          std::cout << "bytes: " << received << std::endl;
          LOG << "client " << _epollEvents[i].data.fd
              << " received: " << buffer;
          auto clientIt = _clients.find(_epollEvents[i].data.fd);
          if (clientIt != _clients.end()) {
            clientIt->second.appendToRecvBuffer(buffer);
            std::string command;
            while (!(command = clientIt->second.extractMessage()).empty()) {
              auto cmd = Parser::parse(command);
              LOG << "Received full command: " << command;
              processMessage(clientIt->first, cmd);
            }
          } else {
            std::cerr << "Got rogue network packet" << std::endl;
          }
        }
      } else if (_epollEvents[i].events & EPOLLOUT) {  // outgoing data
        int32_t     fd = _epollEvents[i].data.fd;
        Client     &client = _clients.at(fd);
        std::string msg = client.getResponseBuffer();
        if (!msg.empty()) {
          Socket &soc = _sockets.at(fd);
          ssize_t bytes = soc.sendData(msg.c_str(), msg.length());
          if (bytes >= 0) {
            client.removeFromResponse(bytes);
            LOG << "Sent " << bytes << " bytes to FD " << fd;
          } else {
            LOG << "Error sending data to FD " << fd;
          }
          if (client.getResponseBuffer().empty()) {
            if (client.shouldClose()) {
              removeClient(fd);
            } else {  // all data was sent, removed EPOLLOUT
              modifyEpoll(fd, EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);
            }
          }
        }
      }
    }
  }
}

void Server::processMessage(int32_t fd, std::optional<Command> const &cmd) {
  Client &client = _clients.at(fd);
  if (!client.isRegistered() && !Utils::isHandshakeCmd(cmd->command)) {
    replyNumeric(fd, Numeric::ERR_NOTREGISTERED, ":Client not registered");
    return;
  }
  if (cmd.has_value()) {
    auto it = _functionMap.find(cmd->command);
    if (it != _functionMap.end()) {
      auto handler = it->second;
      (this->*handler)(fd, *cmd);
    } else {
      replyNumeric(fd, Numeric::ERR_UNKNOWNCOMMAND,
                   cmd->command + " :command not known");
    }
  } else {
    LOG << "Malformed message received from " << client.getNickname();
  }
}

void Server::modifyEpoll(int32_t fd, uint32_t events) {
  struct epoll_event epoll{};
  epoll.events = events;
  epoll.data.fd = fd;
  if (epoll_ctl(_epollFD, EPOLL_CTL_MOD, fd, &epoll) < 0) {
    LOG << "Failed to modify polling list for " << fd;
  }
}

void Server::removeClient(int32_t fd) {
  epoll_ctl(_epollFD, EPOLL_CTL_DEL, fd, NULL);
  _clients.erase(fd);
  _sockets.erase(fd);
}
