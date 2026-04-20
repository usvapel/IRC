#include "Socket.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>

#include "Logger.hpp"

Socket::Socket(int32_t fd) : _fd(fd) {};

Socket::~Socket() {
  if (_fd > 0)
    close(_fd);
}

Socket::Socket(Socket &&other) noexcept : _fd(other._fd) {
  other._fd = -1;
}

Socket &Socket::operator=(Socket &&other) noexcept {
  if (this != &other) {
    if (_fd >= 0) {
      close(_fd);
    }
    this->_fd = other._fd;
    other._fd = -1;
  }
  return *this;
}

Socket Socket::makeListeningSocket(int32_t port) {
  int32_t listenerFD = socket(AF_INET, SOCK_STREAM, 0);
  if (listenerFD == -1) {
    LOG << "Error creating listener socket";
    throw std::runtime_error("Error creating listener socket");
  }
  Socket listener(listenerFD);

  int32_t option = 1;
  if (setsockopt(listenerFD, SOL_SOCKET, SO_REUSEADDR, &option,
                 sizeof(option)) == -1) {
    LOG << "Error setting socket options";
    throw std::runtime_error("Error setting socket options");
  }

  struct sockaddr_in serverAddr = {};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(listenerFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ==
      -1) {
    LOG << "Error binding socket to port";
    throw std::runtime_error("Error binding socket to port");
  }

  if (listen(listenerFD, SOMAXCONN) == -1) {
    LOG << "Error listening to port";
    throw std::runtime_error("Error listening to port");
  }

  listener.makeNonBlocking();
  return listener;
};

Socket Socket::makeClientSocket(int32_t clientFD) {
  Socket socket = Socket(clientFD);
  socket.makeNonBlocking();
  return socket;
};

void Socket::makeNonBlocking() {
  if (fcntl(_fd, F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error("fcntl failed");
  }
};

int32_t Socket::getFD() const {
  return _fd;
}

ssize_t Socket::receiveData(char *buffer, size_t length) {
  return recv(_fd, buffer, length, 0);
}

ssize_t Socket::sendData(const char *buffer, size_t length) {
  return send(_fd, buffer, length, 0);
}
