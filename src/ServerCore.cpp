#include <sys/epoll.h>
#include <sys/types.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include "Utils.hpp"

volatile sig_atomic_t Server::_sigintReceived = false;

Server::Server(const int32_t port, const uint32_t backlogSize,
               const std::string &pwd)
    : _listenSocket(Socket::makeListeningSocket(port)),
      _port(port),
      _backlogSize(backlogSize),
      _lastPingCheck(std::chrono::system_clock::now()),
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
  initializeSignalHandling();
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
  while (_sigintReceived == false) {
    // LOG << "Polling for new connections. Clients: ";
    // LOG << _clients.size();
    TimeStamp now = std::chrono::system_clock::now();
    removeEmptyChannels();
    _nEpollFDs = epoll_wait(_epollFD, _epollEvents, _backlogSize, POLL_TIME);
    for (int i = 0; i < _nEpollFDs; ++i) {
      // check for disconnected clients and remove them from the map
      if (_epollEvents[i].events & (EPOLLHUP | EPOLLERR)) {
        std::string msg = "Unexpected connection loss";
        startDisconnect(_epollEvents[i].data.fd, msg, false);
        removeClient(_epollEvents[i].data.fd);
        continue;
      }
      // add new connections
      if (_epollEvents[i].data.fd == _listenSocket.getFD()) {
        while (true) {  // loop until accept() returns -1 and
          // errno is EAGAIN or EWOULDBLOCK
          struct sockaddr_in client_addr;
          socklen_t          addr_len = sizeof(client_addr);
          int32_t clientFD = accept(_listenSocket.getFD(),
                                    (struct sockaddr *)&client_addr, &addr_len);
          if (clientFD == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              throw std::runtime_error("fatal error accepting clients");
            }
          } else {
            LOG << "New connection accepted on FD: " << clientFD;
            _sockets.try_emplace(clientFD, Socket::makeClientSocket(clientFD));
            _clients.try_emplace(clientFD, Client(&client_addr));
            struct epoll_event connectionPoll{};
            connectionPoll.events = EPOLLIN | EPOLLHUP | EPOLLERR;
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
        if (received > 0) {
          std::cout << "bytes: " << received << std::endl;
          LOG << "client " << _epollEvents[i].data.fd
              << " received: " << buffer;
          auto clientIt = _clients.find(_epollEvents[i].data.fd);
          if (clientIt != _clients.end()) {
            clientIt->second.appendToRecvBuffer(buffer);
            std::string command;
            while (!(command = clientIt->second.extractMessage()).empty()) {
              if (command.length() > MAX_MESSAGE_LEN) {
                replyNumeric(_epollEvents[i].data.fd, Numeric::ERR_INPUTTOOLONG,
                             ":Input too long");
              } else {
                auto cmd = Parser::parse(command);
                LOG << "Received full command: " << command;
                processMessage(clientIt->first, cmd);
              }
            }
          } else {
            std::cerr << "Got rogue network packet" << std::endl;
          }
        } else if (received == 0) {
          LOG << "Client " << _epollEvents[i].data.fd << " disconnected";
          startDisconnect(_epollEvents[i].data.fd, "Client disconnected", true);
        } else if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
          LOG << "Fatal read error";
          startDisconnect(_epollEvents[i].data.fd, "Read error", false);
          removeClient(_epollEvents[i].data.fd);
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
              modifyEpoll(fd, EPOLLIN | EPOLLHUP | EPOLLERR);
            }
          }
        }
      }
    }
    if (now - _lastPingCheck > std::chrono::seconds(5)) {
      pingInactiveClients(now);
      _lastPingCheck = now;
    }
  }
}

void Server::processMessage(int32_t fd, std::optional<Command> const &cmd) {
  if (!cmd.has_value()) {
    return;
  }
  Client &client = _clients.at(fd);
  if (!client.isRegistered() && !Utils::isHandshakeCmd(cmd->command)) {
    replyNumeric(fd, Numeric::ERR_NOTREGISTERED, ":Client not registered");
    return;
  }
  if (cmd.has_value()) {
    auto it = _functionMap.find(cmd->command);
    if (it != _functionMap.end()) {
      client.setLastMsgRecv(std::chrono::system_clock::now());
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
  LOG << "Removing client " << fd;
  epoll_ctl(_epollFD, EPOLL_CTL_DEL, fd, NULL);
  _clients.erase(fd);
  _sockets.erase(fd);
}

OptionalClient Server::findClientByName(const std::string &name) {
  auto it = _nickToFd.find(name);
  if (it == _nickToFd.end())
    return std::nullopt;
  return std::ref(_clients.at(it->second));
}

void Server::initializeSignalHandling(void) {
  struct sigaction sa;
  sigset_t         signalsToBlock;
  std::memset(&sa, 0, sizeof(sa));
  sigfillset(&signalsToBlock);
  sa.sa_handler = &signalHandler;
  sa.sa_mask = signalsToBlock;
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    throw std::runtime_error("Failed initialize SIGINT handling");
  }
  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  if (sigaction(SIGQUIT, &sa, NULL) == -1) {
    throw std::runtime_error("Failed to ignore SIGQUIT");
  }
}

void Server::signalHandler(const int sig) {
  LOG << "SIGINT (" << std::to_string(sig) << ") received";
  _sigintReceived = true;
}

void Server::startDisconnect(int32_t fd, std::string reason,
                             bool socketExists) {
  Client                  &removed = _clients.at(fd);
  std::vector<std::string> channels = removed.getChannels();
  std::string quitMsg = removed.generatePrefix() + " QUIT :Quit :" + reason;
  for (auto channelName : channels) {
    OptionalChannel chan = findChannel(channelName);
    if (chan.has_value()) {
      chan->get().messageAllUsersOnChannel(removed.getNickname(), quitMsg);
      OptionalUser user = chan->get().findUser(removed.getNickname());
      if (user.has_value())
        chan->get().kickUser(*user);
    }
  }
  _nickToFd.erase(removed.getNickname());

  if (socketExists) {
    std::string errorMsg =
        "ERROR :Closing Link: " + removed.getHostname() + " (" + quitMsg + ")";
    replyMessage(fd, errorMsg);
  } else {
    removed.clearResponseBuffer();
  }
  removed.setShouldClose(true);
  modifyEpoll(fd, EPOLLOUT | EPOLLHUP | EPOLLERR);
}

void Server::pingInactiveClients(TimeStamp now) {
  for (auto &[fd, client] : _clients) {
    if (client.shouldClose()) {
      continue;
    }
    if (now - client.getLastMsgRecv() > std::chrono::seconds(CLIENT_TIMEOUT)) {
      LOG << "Starting timeout kick for client " << fd;
      startDisconnect(fd, "Client timed out", true);
      continue;
    }
    if (now - client.getLastMsgRecv() >
            std::chrono::seconds(CLIENT_PING_START) &&
        now - client.getLastPingSent() >
            std::chrono::seconds(CLIENT_PING_INTERVAL)) {
      LOG << "Possibly inactive client " << fd;
      std::string msg = std::string("PING ") + SERVER_NAME;
      replyMessage(fd, msg);
      client.setPingSent(now);
    }
  }
}
