#pragma once
#include <pwd.h>
#include <unistd.h>

#include <iostream>
#include <unordered_map>
#include <vector>

class Client {
  public:
    Client();
    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;
    Client(Client &&) = default;  // move constructor
    Client &operator=(Client &&) = default;
    ~Client();

    enum class State {
      CONNECTED,
      NICK_RECEIVED,
      USER_RECEIVED,
      REGISTERED,
    };

    /**
     * @brief Checks if the socketBuffer contains a full message terminated by
     * "\r\n"
     * @return bool Is there a full message in buffer
     */
    bool checkBuffer();
    void set_nick() {
      struct passwd *pw = getpwuid(getuid());
      if (pw) {
        _nickname = pw->pw_name;
      }
    }

    /**
     * @brief Extracts a message from the socketBuffer.
     *
     * @return std::string The first full message contained in the socketBuffer.
     */
    std::string extractMessage();

    std::string getNickname();
    void        setNickname(std::string const &nick);

    State getState();
    void  setState(State s);

    std::string getUsername();
    void        setUsername(std::string const &name);

    std::string getRealname();
    void        setRealname(std::string const &name);

    bool isRegistered();
    bool shouldClose();

    void        appendToRecvBuffer(std::string const &input);
    void        appendToResponseBuffer(std::string const &msg);
    std::string getResponseBuffer();

    void setPasswordOK(bool b);
    bool isPasswordOK();

  private:
    std::string _responseBuffer;
    std::string _recvBuffer;
    std::string _nick;
    std::string _username;
    std::string _realname;
    bool        _passwordOK;
    bool        _shouldClose;
    State       _state;
    static Socket *_clientSocket;
    std::string    _response;
    std::string    _nickname;
    std::string    _username;
    std::string    _recv_buf;
};
