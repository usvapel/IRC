#pragma once
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include "Socket.hpp"

#define BACKLOG_SIZE 1024
#define RCVBUF_SIZE 65536
#define SNDBUF_SIZE 65536

#define POLL_TIME 1000

class Client;
class Channel;

class Server {
  private:
    // INFO: Listening
    Socket   _listenSocket;
    int32_t  _port;
    uint32_t _backlogSize;

    // INFO: Polling
    struct epoll_event  _epoll{};
    struct epoll_event *_epollEvents{};
    int32_t             _epollFD = -1;
    int32_t             _nEpollFDs;

    /**
     * @brief map of Client classes, each has its own Socket class
     */
    std::unordered_map<int, Client> _clients;
    std::unordered_map<int, Socket> _sockets;

    // INFO: Functionality
    using Function = void (Server::*)(Client &, const Command &);
    void handlePassword(Client &client, const Command &cmd);
    void handleNickname(Client &client, const Command &cmd);
    void handleUserJoin(Client &client, const Command &cmd);
    void handleCapNegotiation(Client &client, const Command &cmd);
    inline static const std::unordered_map<std::string, Function> _functionMap =
        {{"PASS", &Server::handlePassword},
         {"NICK", &Server::handleNickname},
         {"USER", &Server::handleUserJoin},
         {"CAP", &Server::handleCapNegotiation}};

    // INFO: Formulate responses
    void replyMessage(Client &client, int code, std::string const &msg);
    void sendWelcomeMessages(Client &client);

    bool isNicknameInUse(std::string const &nick);

    // INFO: Channels:
    std::vector<std::unique_ptr<Channel>> _channels;

    // INFO: Security
    const std::string _pwd;
    void              disconnectUser(int32_t fd);

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
     * @brief remove client and socket from the maps
     *
     * @param fd
     */
    void removeClient(int fd);

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

    void processMessage(Client &client, std::optional<Command> const &cmd);

    void run(void);

    // INFO: Channel management:
    /**
     * @brief Creates a new Channel and emplaces it as a unique_ptr to _channels
     * vector. Rerturns a reference to this Channel
     *
     * @param client Client that is the creator of the Channel.
     * @param name Name of the Channel
     * @return Returns a reference to the newly created Channel
     */
    Channel &newChannel(const Client &client, const std::string &name);

    /**
     * @brief Return a reference to the _channels vector of the Server
     */
    std::vector<std::unique_ptr<Channel>> &getChannels(void);

    /**
     * @brief Tries to find a Channel with the name <target>. If not found,
     * throws a std::runtime_error exception
     *
     * @param target Name of the Channel to search for
     */
    Channel &findChannel(const std::string &target) const;
};
