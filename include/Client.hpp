#pragma once

#include <stddef.h>

#include <chrono>
#include <string>
#include <vector>

constexpr size_t MAX_RECV_BUFFER = 8192;
constexpr size_t MAX_SEND_BUFFER = 1048576;
constexpr size_t MAX_MESSAGE_LEN = 510;

using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;

class Client {
  public:
    Client() = default;
    Client(struct sockaddr_in *addr);
    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;
    Client(Client &&) = default;
    Client &operator=(Client &&) = default;
    ~Client();

    enum class State {
      CONNECTED,
      NICK_RECEIVED,
      USER_RECEIVED,
      REGISTERED,
    };

    /**
     * @brief Extracts a message from the socketBuffer.
     *
     * @return std::string The first full message contained in the socketBuffer.
     */
    std::string extractMessage();

    const std::string &getNickname() const;
    void               setNickname(std::string const &nick);

    State getState();
    void  setState(State s);

    const std::string &getUsername() const;
    void               setUsername(std::string const &name);

    const std::string &getRealname() const;
    void               setRealname(std::string const &name);

    const std::string &getHostname() const;
    void               setHostname(std::string const &name);

    /**
     * @brief Returns a vector of channel names Client is on.
     */
    const std::vector<std::string> getChannels(void) const;

    /**
     * @brief Adds a channel name to a vector of channel names Client is on.
     *
     * @param channel Channel name to be added.
     */
    void addChannel(const std::string &channel);

    /**
     * @brief Removes a channel name from a vector of channel names Client is on
     * if it's found.
     *
     * @param channelToRemove Channel name to remove from vector if it's found.
     */
    void removeChannel(const std::string &channelToRemove);

    const std::string generatePrefix() const;

    bool isRegistered();
    bool shouldClose();
    void setShouldClose(bool b);

    void         appendToRecvBuffer(std::string const &input);
    void         appendToResponseBuffer(std::string const &msg);
    void         clearResponseBuffer();
    std::string &getResponseBuffer();
    void         removeFromResponse(size_t bytes);

    void setPasswordOK(bool b);
    bool isPasswordOK();

    TimeStamp getLastPingSent();
    TimeStamp getLastMsgRecv();
    void      setPingSent(TimeStamp t);
    void      setLastMsgRecv(TimeStamp t);
    bool      isWaitingForPong();
    void      setWaitingForPong(bool b);

  private:
    std::string              _responseBuffer;
    std::string              _recvBuffer;
    std::string              _nick;
    std::string              _username;
    std::string              _realname;
    std::string              _hostname;
    bool                     _passwordOK;
    bool                     _shouldClose;
    bool                     _waitingForPong;
    State                    _state;
    TimeStamp                _lastMsgRecv;
    TimeStamp                _lastPingSent;
    std::vector<std::string> _channels;
};
