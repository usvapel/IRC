#include "Server.hpp"

#include <cstring>

#include "Logger.hpp"

Server::Server(const int32_t port, const uint32_t backlogSize,
               const std::string &pwd)
    : _port(port), _backlogSize(backlogSize), _pwd(pwd) {
  int sendBufSize = SNDBUF_SIZE;
  int receiveBufSize = RCVBUF_SIZE;

  _listenSocket = Socket::makeListeningSocket(port);
  if (_listenSocket == nullptr)
    throw std::runtime_error("Port value out of bounds. Use value 0 - 65535");

  if (setsockopt(_listenSocket->getFD(), SOL_SOCKET, SO_RCVBUF, &receiveBufSize,
                 sizeof(receiveBufSize)) < 0)
    throw std::runtime_error(
        "Failed to set server listen socket receive buffer size");
  if (setsockopt(_listenSocket->getFD(), SOL_SOCKET, SO_SNDBUF, &sendBufSize,
                 sizeof(sendBufSize)) < 0)
    throw std::runtime_error(
        "Failed to set server listen socket send buffer size");
}

void Server::start(void) {
  if ((_epollFD = epoll_create(_backlogSize)) < 0)
    throw std::runtime_error("Failed to set server");
  _epoll.events = EPOLLIN;
  _epoll.data.fd = _listenSocket->getFD();
  if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _listenSocket->getFD(), &_epoll) < 0)
    throw std::runtime_error("Failed to start polling on listening socket");
  LOG << "Starting server, build: " << GIT_HASH;
  epollEvents = new struct epoll_event[_backlogSize];
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
  char buffer[2048];
  while (true) {
    std::cout << "Polling for new connections. Clients: ";
    std::cout << _clients.size() << std::endl;
    _nEpollFDs = epoll_wait(_epollFD, epollEvents, _backlogSize, POLL_TIME);
    for (int i = 0; i < _nEpollFDs; ++i) {
      if (epollEvents[i].data.fd == _listenSocket->getFD()) {
        int32_t clientFD = accept(_listenSocket->getFD(), NULL, NULL);
        if (clientFD < 0) {
          std::cerr << "Failed to accept connection to client\n";
          continue;
        }
        Socket *clientSocket = Socket::makeClientSocket(clientFD);

        struct epoll_event connectionPoll{};
        connectionPoll.events = EPOLLIN;
        connectionPoll.data.fd = clientSocket->getFD();
        if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, clientSocket->getFD(),
                      &connectionPoll) < 0) {
          std::cerr << "Failed to add connectiont to polling list\n";
          continue;
        }

        _clients.push_back(clientSocket);
      } else {
        ssize_t received =
            recv(epollEvents[i].data.fd, buffer, sizeof(buffer), MSG_DONTWAIT);
        if (received < 0)
          std::cerr << "failed to receive\n";
        std::cout << "bytes: " << received << std::endl;
        std::cout << "message: " << buffer;
        send(epollEvents[i].data.fd, buffer, strlen(buffer), 0);
      }
    }
  }
}

void Server::handlePassword(Client *client, const Command &cmd) {
  (void)client, (void)cmd;
  LOG << "handling PASS command";
}

void Server::handleNickname(Client *client, const Command &cmd) {
  (void)client, (void)cmd;
  LOG << "handling NICK command";
}

void Server::handleUserJoin(Client *client, const Command &cmd) {
  (void)client, (void)cmd;
  LOG << "handling NICK command";
}
Server::~Server(void) {
  if (_epollFD != -1)
    close(_epollFD);
  if (epollEvents)
    delete[] epollEvents;
}
