#pragma once
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Command.hpp"
#include "Socket.hpp"

#define BACKLOG_SIZE 1024
#define RCVBUF_SIZE 65536
#define SNDBUF_SIZE 65536
#define POLL_TIME 1000

class Client;

class Server {
  private:
    // Listening
    Socket  *_listenSocket = nullptr;
    int32_t  _port;
    uint32_t _backlogSize;

    // Polling
    struct epoll_event  _epoll{};
    struct epoll_event *epollEvents{};
    int32_t             _epollFD = -1;
    int32_t             _nEpollFDs;

    /**
     * @brief map of Client classes, each has its own Socket class
     */
    std::unordered_map<int, Client> _clientData;

    // functionality
    using Function = void (Server::*)(Client *, const Command &);
    void handlePassword(Client *client, const Command &cmd);
    void handleNickname(Client *client, const Command &cmd);
    void handleUserJoin(Client *client, const Command &cmd);
    inline static const std::unordered_map<std::string, Function> _functionMap =
        {{"PASS", &Server::handlePassword},
         {"NICK", &Server::handleNickname},
         {"USER", &Server::handleUserJoin}};

    // Security
    const std::string _pwd;

  public:
    Server(void) = delete;
    Server(const Server &other) = delete;
    Server &operator=(const Server &other) = delete;

    /**
     * @brief Initializes Server object with given <port>, <backlogSize> and
     * <pwd>. The private member struct sockadrr_in is initialized with the
     * given port for IPv4 connections. The send buffer size and receive buffer
     * sizes are initialized according to Server.hpp macros SNDBUF_SIZE and
     * RCVBUF_SIZE.
     *
     * @param port Port used to listen for new connections.
     * @param backlogSize Queue size for pending connections
     * @param pwd Password used to connect to server
     */
    Server(const int32_t port, const uint32_t backlogSize,
           const std::string &pwd);
    /**
     * @brief Destructs the Server object and closes all FD's.
     */
    ~Server(void);

    /**
     * @brief Returns the port on which Server is listening
     *
     * @return int32_t _port
     */
    int32_t getListenPort(void) const;

    /**
     * @brief Returns the servers listening FD
     *
     * @return int32_t _serverfd
     */
    int32_t getServerFd(void) const;

    /**
     * @brief Returns a reference to the vector of client side FD's
     */
    std::vector<int32_t> &getClients(void) const;

    /**
     * @brief Starts the server and initializes _epollfd. Starts polling on the
     * _listenfd
     */
    void start(void);

    /**
     * @brief Checks if the given password <pwd> matches the server's password
     *
     * @param pwd Password to check against the server's password
     * @return True or false depending on if the passwords match
     */
    bool passwordIsCorrect(const std::string &pwd);

    void run(void);
};
