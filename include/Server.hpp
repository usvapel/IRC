#pragma once
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "Channel.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include "Parser.hpp"
#include "Socket.hpp"

constexpr int         BACKLOG_SIZE = 1024;
constexpr size_t      RCVBUF_SIZE = 65536;
constexpr size_t      SNDBUF_SIZE = 65536;
constexpr int         POLL_TIME = 1000;
constexpr int         CLIENT_PING_START = 40;
constexpr int         CLIENT_PING_INTERVAL = 30;
constexpr int         CLIENT_TIMEOUT = 100;
constexpr std::string SERVER_NAME = "usvaIRC";

class Client;
class Channel;

namespace Parser {
void channelModeParse(const Command &cmd, Channel &channel, Server &server,
                      int32_t fd);
}

using OptionalClient = std::optional<std::reference_wrapper<Client>>;
using OptionalChannel = std::optional<std::reference_wrapper<Channel>>;
using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;

class Server {
  private:
    // INFO: Listening
    Socket    _listenSocket;
    int32_t   _port{};
    uint32_t  _backlogSize{};
    TimeStamp _lastPingCheck;

    // INFO: Polling
    struct epoll_event  _epoll{};
    struct epoll_event *_epollEvents{};
    int32_t             _epollFD = -1;
    int32_t             _nEpollFDs{};

    // INFO: Signal handling
    static volatile sig_atomic_t _sigintReceived;

    friend void Parser::channelModeParse(const Command &cmd, Channel &channel,
                                         Server &server, int32_t fd);

    /**
     * @brief map of Client classes, each has its own Socket class
     */
    std::unordered_map<int32_t, Client>      _clients;
    std::unordered_map<std::string, int32_t> _nickToFd;
    std::unordered_map<int32_t, Socket>      _sockets;

    void modifyEpoll(int32_t fd, uint32_t events);

    /**
     * @brief Initializes server signal handling.
     */
    void initializeSignalHandling(void);

    using Function = void (Server::*)(int32_t, const Command &);
    void handlePassword(int32_t fd, const Command &cmd);
    void handleJoin(int32_t fd, const Command &cmd);
    void handlePart(int32_t fd, const Command &cmd);
    void handleKick(int32_t fd, const Command &cmd);
    void handleNickname(int32_t fd, const Command &cmd);
    void handleUserJoin(int32_t fd, const Command &cmd);
    void handleCapNegotiation(int32_t fd, const Command &cmd);
    void handleMsg(int32_t fd, const Command &cmd);
    void handleTopic(int32_t fd, const Command &cmd);
    void handleInvite(int32_t fd, const Command &cmd);
    void handleQuit(int32_t fd, const Command &cmd);
    void handlePing(int32_t fd, const Command &cmd);
    void handleMode(int32_t fd, const Command &cmd);
    inline static const std::unordered_map<std::string, Function> _functionMap =
        {
            {"PASS", &Server::handlePassword},
            {"JOIN", &Server::handleJoin},
            {"PART", &Server::handlePart},
            {"KICK", &Server::handleKick},
            {"NICK", &Server::handleNickname},
            {"USER", &Server::handleUserJoin},
            {"CAP", &Server::handleCapNegotiation},
            {"NOTICE", &Server::handleMsg},
            {"PRIVMSG", &Server::handleMsg},
            {"TOPIC", &Server::handleTopic},
            {"INVITE", &Server::handleInvite},
            {"QUIT", &Server::handleQuit},
            {"PING", &Server::handlePing},
            {"PONG", &Server::handlePing},
            {"MODE", &Server::handleMode},
    };

    // formulate responses
    void replyMessage(int32_t fd, std::string const &msg);
    void replyNumeric(int32_t fd, int32_t code, std::string const &msg);
    void sendWelcomeMessages(int32_t fd);

    bool isNicknameInUse(std::string const &nick);

    static void signalHandler(const int sig);

    std::unordered_map<std::string, std::unique_ptr<Channel>> _channels;

    // INFO: Security
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

    OptionalClient findClientByName(const std::string &name);
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

    void processMessage(int32_t fd, std::optional<Command> const &cmd);

    /**
     * @brief Tries to send a <message> to a user under <nickname>
     *
     * @param from Nickname of the user sending the <message>
     * @param to Nickname of the user to send the <message> to
     * @param message Message to send to the <user>
     */
    void sendMessageToUser(const std::string &from, const std::string &to,
                           const std::string &message);

    /**
     * @brief Tries to send a <message> with a <code> to a user under <nickname>
     *
     * @param from Nickname of the user sending the <message>
     * @param to Nickname of the user to send the <message> to
     * @param code Numeric reply value to pass to the <user> with the <message>
     * @param message Message to send to the <user>
     */
    void sendMessageWithCodeToUser(const std::string &from,
                                   const std::string &to, const int32_t code,
                                   const std::string &message);

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
    std::unordered_map<std::string, std::unique_ptr<Channel>> &getChannels(
        void);

    /**
     * @brief Tries to find a Channel with the name <channelName>
     *
     * @param channelName Name to look for in the channels.
     */
    std::optional<std::reference_wrapper<Channel>> findChannel(
        const std::string &channelName) const;

    /**
     * @brief removes a channel from server is it's user count is 0.
     */
    void removeEmptyChannels(void);

    /**
     * @brief Tries to remove the given <channel> if it's empty.
     *
     * @param channel Name of the channel to be tried to remove.
     */
    void removeEmptyChannel(const std::string &channel);

    /**
     * @brief starts the process of removing a client from the server.
     */
    void startDisconnect(int32_t fd, std::string reason, bool socketExists);

    /**
     * @brief Loops through all clients, sends PING to those who have been
     * inactive longer than CLIENT_PING_INTERVAL seconds, and starts the removal
     * of those that have been inactive for longer than CLIENT_TIMEOUT seconds.
     *
     * @param TimeStamp now, a time_point of when the function was called
     */
    void pingInactiveClients(TimeStamp now);
};
