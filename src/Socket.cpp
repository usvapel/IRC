#include "Socket.hpp"

#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Logger.hpp"

Socket::Socket(int32_t fd) : _fd(fd) {};

Socket::~Socket() {
  close(_fd);
}

Socket *Socket::makeListeningSocket(int32_t port) {
  int32_t listenerFD = socket(AF_INET, SOCK_STREAM, 0);
  if (listenerFD == -1) {
    LOG << "Error creating listener socket";
    return nullptr;
  }

  int32_t option = 1;
  if (setsockopt(listenerFD, SOL_SOCKET, SO_REUSEADDR, &option,
                 sizeof(option)) == -1) {
    LOG << "Error setting socket options";
    close(listenerFD);
    return nullptr;
  }

  struct sockaddr_in serverAddr = {};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(listenerFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ==
      -1) {
    LOG << "Error binding socket to port";
    close(listenerFD);
    return nullptr;
  }

  if (listen(listenerFD, SOMAXCONN) == -1) {
    LOG << "Error listening to port";
    close(listenerFD);
    return nullptr;
  }
  Socket *listener = new Socket(listenerFD);
  listener->makeNonBlocking(listenerFD);
  return listener;
};

Socket *Socket::makeClientSocket(int32_t clientFD) {
  Socket *socket = new Socket(clientFD);
  socket->makeNonBlocking(clientFD);
  return socket;
};

void Socket::makeNonBlocking(int32_t fd) {
  fcntl(fd, F_SETFL, O_NONBLOCK);
};

int32_t Socket::getFD() const {
  return _fd;
}
