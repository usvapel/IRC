#include <exception>

#include "Server.hpp"
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

#define MAX_EVENTS 10
struct epoll_event ev, events[MAX_EVENTS];
int                epollfd, nfds;

int main(int ac, char **av) {
  if (ac != 3) {
    std::cerr << "Please use args <port> <password>" << std::endl;
    return (1);
  }
  try {
    Server server(std::stoi(av[1]), BACKLOG_SIZE, av[2]);
    server.start();
    server.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return (1);
  }

  // while (true) {}
}

// int main(int ac, char **av) {
//   auto client = new clientDetails();
//   (void)ac;
//   (void)av;
//
//   client->serverfd = socket(AF_INET, SOCK_STREAM, 0);
//   if (client->serverfd == -1) {
//     std::cerr << "socket error" << '\n';
//     delete client;
//     exit(2);
//   }
//
//   int opt = 1;
//   if (setsockopt(client->serverfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
//                  sizeof opt) < 0) {
//     std::cerr << "setsockopt error" << '\n';
//     delete client;
//     exit(2);
//   }
//
//   struct sockaddr_in serverAddr;
//   serverAddr.sin_family = AF_INET;
//   serverAddr.sin_port = htons(port);
//   serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
//
//   if (unlink(MY_SOCK_PATH) == -1) {
//     std::cerr << "unlink error" << '\n';
//   }
//
//   if (bind(client->serverfd, (struct sockaddr *)&serverAddr,
//            sizeof(serverAddr)) == -1) {
//     std::cerr << "bind error" << '\n';
//     delete client;
//     exit(2);
//   }
//
//   if (listen(client->serverfd, LISTEN_BACKLOG) == -1) {
//     std::cerr << "listen error" << '\n';
//     delete client;
//     exit(2);
//   }
//
//   fd_set readfds;
//   size_t valread;
//   int    maxfd;
//   int    lastSd = 0;
//   int    activity;
//
//   while (true) {
//     std::cout << "waiting for activity\n";
//     FD_ZERO(&readfds);
//     FD_SET(client->serverfd, &readfds);
//     maxfd = client->serverfd;
//     for (auto sd : client->clientList) {
//       FD_SET(sd, &readfds);
//       if (sd > maxfd) {
//         maxfd = sd;
//       }
//     }
//     if (lastSd > maxfd) {
//       maxfd = lastSd;
//     }
//
//     activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
//     if (activity < 0) {
//       std::cerr << "select error\n";
//       continue;
//     }
//
//     if (FD_ISSET(client->serverfd, &readfds)) {
//       client->clientfd =
//           accept(client->serverfd, (struct sockaddr *)NULL, NULL);
//       if (client->clientfd == -1) {
//         std::cerr << "accept error\n";
//         continue;
//       }
//       client->clientList.push_back(client->clientfd);
//       std::cout << "new client connected\n";
//       std::cout << "new connection socket fd is " << client->clientfd
//                 << ", ip is: " << inet_ntoa(serverAddr.sin_addr)
//                 << ", port: " << ntohs(serverAddr.sin_port) << '\n';
//     }
//
//     char    buf[4096];
//     ssize_t n = recv(client->clientfd, buf, sizeof(buf) - 1, 0);
//     buf[n] = 0;
//     std::cout << "(irssi) RAW: " << buf;
//
//     char message[1024];
//     for (size_t i = 0; i < client->clientList.size(); ++i) {
//       lastSd = client->clientList[i];
//       if (FD_ISSET(lastSd, &readfds)) {
//         valread = read(lastSd, message, 1024);
//         message[valread] = 0;
//         if (valread == 0) {
//           std::cout << "client disconnected\n";
//           std::cout << "host disconnected, ip: "
//                     << inet_ntoa(serverAddr.sin_addr)
//                     << ", port: " << ntohs(serverAddr.sin_port) << '\n';
//           close(lastSd);
//           client->clientList.erase(client->clientList.begin() + i);
//         } else {
//           std::cout << "message from client: " << message << '\n';
//         }
//       }
//     }
//   }
//
//   if (close(client->serverfd) == -1) {
//     std::cerr << "close error" << '\n';
//     delete client;
//     exit(2);
//   }
//
//   if (unlink(MY_SOCK_PATH) == -1) {
//     std::cerr << "unlink error" << '\n';
//     delete client;
//     exit(2);
//   }
//
//   return 0;
// }
