
#include "irc.hpp"

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
}
