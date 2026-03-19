#include "Server.hpp"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdint>

#include "Socket.hpp"

Server::Server(const int32_t port, const uint32_t backlogSize,
               const std::string &pwd)
    : _port(port), _backlogSize(backlogSize), _pwd(pwd) {
  int sendBufSize = SNDBUF_SIZE;
  int receiveBufSize = RCVBUF_SIZE;

  if (_port < 0 || _port > 65535)
    throw std::runtime_error("Port value out of bounds. Use value 0 - 65535");

  _address.sin_family = AF_INET;
  _address.sin_port = htons(_port);
  _address.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((_listenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    throw std::runtime_error("Failed to create server listen socket");

  if (setsockopt(_listenSock, SOL_SOCKET, SO_RCVBUF, &receiveBufSize,
                 sizeof(receiveBufSize)) < 0)
    throw std::runtime_error(
        "Failed to set server listen socket receive buffer size");
  if (setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, &sendBufSize,
                 sizeof(sendBufSize)) < 0)
    throw std::runtime_error(
        "Failed to set server listen socket send buffer size");
}

void Server::start(void) {
  _addressLen = sizeof(_address);
  if (bind(_listenSock, (struct sockaddr *)&_address, _addressLen) < 0)
    throw std::runtime_error("Failed to bind server socket with port " +
                             std::to_string(_port));
  if (listen(_listenSock, _backlogSize) < 0)
    throw std::runtime_error("Failed to start listening from server socket");

  if ((_epollfd = epoll_create(_backlogSize)) < 0)
    throw std::runtime_error("Failed to set server");
  _ev.events = EPOLLIN;
  _ev.data.fd = _listenSock;
  if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, _listenSock, &_ev) < 0)
    throw std::runtime_error("Failed to start polling on listening socket");

  _events = new struct epoll_event[_backlogSize];
}

void Server::run(void) {
  char buffer[1024];
  while (true) {
    std::cout << "Polling for new connections. Clients: ";
    std::cout << _clients.size() << std::endl;
    _nfds = epoll_wait(_epollfd, _events, _backlogSize, 100);
    for (int i = 0; i < _nfds; ++i) {
      if (_events[i].data.fd == _listenSock) {
        int32_t connectionSocket =
            accept(_listenSock, (struct sockaddr *)&_address, &_addressLen);

        // FIXME: Do we need to store this for later use?
        struct epoll_event connectionPoll;
        connectionPoll.events = EPOLLIN;
        connectionPoll.data.fd = connectionSocket;

        if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, connectionSocket,
                      &connectionPoll) < 0)
          std::cerr << "Failed to add connectiont to polling list\n";

        if (setNonBlocking(connectionSocket) < 0) {
          epoll_ctl(_epollfd, EPOLL_CTL_DEL, connectionSocket, _events);
          continue;
        }
        _clients.push_back(connectionSocket);
      } else {
        ssize_t received = recv(_clients[0], buffer, 1024, MSG_DONTWAIT);
        if (received < 0)
          std::cerr << "failed to receive\n";
        send(_clients[0], "test", 5, 0);
        std::cout << "received: " << received << std::endl;
        std::cout << buffer;
      }
    }
  }
}

Server::~Server(void) {
  if (_epollfd != -1)
    close(_epollfd);
  if (_listenSock != -1)
    close(_listenSock);
  if (_events)
    delete[] _events;
}

// NOTE: Private:
int32_t Server::setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags == -1) {
    return (-1);
  }
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  return (0);
}
