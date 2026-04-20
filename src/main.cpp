#include <sys/epoll.h>

#include <exception>
#include <iostream>
#include <string>

#include "Logger.hpp"
#include "Server.hpp"

#ifndef GIT_HASH
#define GIT_HASH "unknown_build"
#endif

#define MAX_EVENTS 10
struct epoll_event ev, events[MAX_EVENTS];
int                epollfd, nfds;

int main(int ac, char **av) {
  if (ac != 3) {
    std::cerr << "Please use args <port> <password>" << std::endl;
    return (1);
  }
  try {
    Logger::setLogFile("irc_server.log");
    LOG << "Starting server, build: " << GIT_HASH;
    Server server(std::stoi(av[1]), BACKLOG_SIZE, av[2]);
    server.start();
    server.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return (1);
  }
}
