#include "Logger.hpp"
#include "Server.hpp"
#include "irc.hpp"

#ifndef GIT_HASH
#define GIT_HASH "unknown_build"
#endif

// Connect to UNIX-domain stream socket
// nc -U /tmp/testsocket
// irssi
// /connect 127.0.0.1 6767

struct clientDetails {
    int32_t          clientfd;
    int32_t          serverfd;
    std::vector<int> clientList;
    clientDetails(void) {
      this->clientfd = -1;
      this->serverfd = -1;
    }
};

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
