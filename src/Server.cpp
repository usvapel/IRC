#include "Server.hpp"

#include <sys/epoll.h>
#include <sys/types.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Client.hpp"
#include "Logger.hpp"
#include "Parser.hpp"
#include "Utils.hpp"

#define SERVER_NAME "usvaIRC"
#include <cstring>

#include "Client.hpp"
#include "Logger.hpp"

Server::Server(const int32_t port, const uint32_t backlogSize,
               const std::string &pwd)
    : _listenSocket(Socket::makeListeningSocket(port)),
      _port(port),
      _backlogSize(backlogSize),
      _pwd(pwd) {
  int sendBufSize = SNDBUF_SIZE;
  int receiveBufSize = RCVBUF_SIZE;

  if (setsockopt(_listenSocket.getFD(), SOL_SOCKET, SO_RCVBUF, &receiveBufSize,
                 sizeof(receiveBufSize)) < 0)
    throw std::runtime_error(
        "Failed to set server listen socket receive buffer size");
  if (setsockopt(_listenSocket.getFD(), SOL_SOCKET, SO_SNDBUF, &sendBufSize,
                 sizeof(sendBufSize)) < 0)
    throw std::runtime_error(
        "Failed to set server listen socket send buffer size");
}

void Server::start(void) {
  if ((_epollFD = epoll_create(_backlogSize)) < 0)
    throw std::runtime_error("Failed to set server");
  _epoll.events = EPOLLIN;
  _epoll.data.fd = _listenSocket.getFD();
  if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _listenSocket.getFD(), &_epoll) < 0)
    throw std::runtime_error("Failed to start polling on listening socket");
  _epollEvents = new struct epoll_event[_backlogSize];
}

// FIXME: Should we store client IP and port to a struct inside Socket *
// (not NULL arguments in accept?)
// int32_t clientFD = accept(_listener->getFD(),
//                           (struct sockaddr *)&CLIENTADDRESS,
//                           &CLIENTADDRESSLENGTH);
// FIXME: Do we need to store connectionPoll struct for later use?
// FIXME: What to do if adding clientSocket->getFD to polling fails?
// FIXME: What to do if accept() fails?
void Server::run(void) {
  while (true) {
    // LOG << "Polling for new connections. Clients: ";
    // LOG << _clients.size();
    _nEpollFDs = epoll_wait(_epollFD, _epollEvents, _backlogSize, POLL_TIME);
    for (int i = 0; i < _nEpollFDs; ++i) {
      // check for disconnected clients and remove them from the map
      if (_epollEvents[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        removeClient(_epollEvents[i].data.fd);
        continue;
      }
      // add new connections
      if (_epollEvents[i].data.fd == _listenSocket.getFD()) {
        while (true) {  // loop until accept() returns -1 and
                        // errno is EAGAIN or EWOULDBLOCK
          int32_t clientFD = accept(_listenSocket.getFD(), NULL, NULL);
          if (clientFD == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              throw std::runtime_error("fatal error accepting clients");
            }
          } else {
            LOG << "New connection accepted on FD: " << clientFD;
            _sockets.try_emplace(clientFD, Socket::makeClientSocket(clientFD));
            _clients.try_emplace(clientFD, Client());

            struct epoll_event connectionPoll{};
            connectionPoll.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
            connectionPoll.data.fd = clientFD;
            if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, clientFD, &connectionPoll) <
                0) {
              removeClient(clientFD);
              LOG << "Failed to add connection to polling list\n";
              continue;
            }
          }
        }
      } else if (_epollEvents[i].events & EPOLLIN) {  // incoming data
        char    buffer[2048] = {};
        auto    it = _sockets.find(_epollEvents[i].data.fd);
        ssize_t received;
        if (it != _sockets.end()) {
          received = it->second.receiveData(buffer, sizeof(buffer));
        } else {
          epoll_ctl(_epollFD, EPOLL_CTL_DEL, _epollEvents[i].data.fd, NULL);
          continue;
        }
        if (received <= 0) {
          std::cerr << "failed to receive\n";
          continue;
        } else {
          std::cout << "bytes: " << received << std::endl;
          LOG << "client " << _epollEvents[i].data.fd
              << " received: " << buffer;
          auto clientIt = _clients.find(_epollEvents[i].data.fd);
          if (clientIt != _clients.end()) {
            clientIt->second.appendToRecvBuffer(buffer);
            std::string command;
            while (!(command = clientIt->second.extractMessage()).empty()) {
              auto cmd = Parser::parse(command);
              LOG << "Received full command: " << command;
              processMessage(clientIt->first, cmd);
            }
          } else {
            std::cerr << "Got rogue network packet" << std::endl;
          }
        }
      } else if (_epollEvents[i].events & EPOLLOUT) {  // outgoing data
        int32_t     fd = _epollEvents[i].data.fd;
        Client     &client = _clients.at(fd);
        std::string msg = client.getResponseBuffer();
        if (!msg.empty()) {
          Socket &soc = _sockets.at(fd);
          ssize_t bytes = soc.sendData(msg.c_str(), msg.length());
          if (bytes >= 0) {
            client.removeFromResponse(bytes);
            LOG << "Sent " << bytes << " bytes to FD " << fd;
          } else {
            LOG << "Error sending data to FD " << fd;
          }
          if (client.getResponseBuffer().empty()) {
            if (client.shouldClose()) {
              removeClient(fd);
            } else {  // all data was sent, removed EPOLLOUT
              modifyEpoll(fd, EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);
            }
          }
        }
      }
    }
  }
}

bool Server::passwordIsCorrect(const std::string &pwd) {
  return (_pwd == pwd);
}

void Server::removeClient(int fd) {
  LOG << "Removed client " << fd;
  // handle sending QUIT messages here?
  _clients.erase(fd);
  _sockets.erase(fd);
}

void Server::handlePassword(int32_t fd, const Command &cmd) {
  LOG << "handling PASS command";
  Client &client = _clients.at(fd);
  if (client.isRegistered()) {
    replyNumeric(fd, Numeric::ERR_ALREADYREGISTRED,
                 ":Unauthorized command (already registered)");
    return;
  }

  if (cmd.params.empty()) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS,
                 "PASS :Not enough parameters");
    return;
  }

  if (cmd.params[0] == _pwd) {
    LOG << "Password matches";
    client.setPasswordOK(true);
  } else {
    LOG << "Password doesn't match, got '" << cmd.params[0] << "', expected '"
        << _pwd << "'";
    replyNumeric(fd, Numeric::ERR_PASSWDMISMATCH, ":Incorrect password");
  }
}

void Server::handleJoin(int32_t fd, const Command &cmd) {
  LOG << "handling JOIN command";
  Client &client = _clients.at(fd);
  if (!client.isPasswordOK()) {
    replyNumeric(fd, Numeric::ERR_PASSWDMISMATCH, ":Incorrect password");
    return;
  }
  if (cmd.params.size() < 1) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS, ":Not enough parameters");
    return;
  }

  //  FIXME: vv Throw here only for development/debugging purposes vv
  if (cmd.params.size() > 2) {
    throw std::runtime_error("Too many params for JOIN command");
  }
  // FIXME: ^^ Throw here only for development/debugging purposes ^^

  std::vector<std::string> channelNames;
  std::vector<std::string> channelKeys;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    switch (i) {
      case 0: {
        std::string       channel;
        std::stringstream channelStream(cmd.params[i]);
        while (std::getline(channelStream, channel, ',')) {
          channelNames.push_back(channel);
        }
        break;
      }
      case 1: {
        std::string       key;
        std::stringstream keyStream(cmd.params[i]);
        while (std::getline(keyStream, key, ',')) {
          channelKeys.push_back(key);
        }
        break;
      }
    }
  }
  // FIXME: Should we allow the amount of keys be different than channel amount?
  if (channelKeys.size() > 0 && channelNames.size() != channelKeys.size()) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS, ":Not enough parameters");
    return;
  }
  for (size_t i = 0; i < channelNames.size(); ++i) {
    OptionalChannel channel = findChannel(channelNames[i]);
    Client         &clientToAdd = _clients.at(fd);
    LOG << clientToAdd.getNickname() + " trying to join channel " +
               channelNames[i];
    if (!channel.has_value()) {
      LOG << channelNames[i] + " not found. Creating channel " +
                 channelNames[i];
      newChannel(clientToAdd, channelNames[i]);
      continue;
    } else {
      LOG << channelNames[i] + " found. " + clientToAdd.getNickname() +
                 " joining the joining";
      // FIXME: Need to implement password checks!
      channel->get().addUser(clientToAdd);
    }
  }
}

void Server::handleKick(int32_t fd, const Command &cmd) {
  LOG << "handling KICK command";
  Client &client = _clients.at(fd);
  if (!client.isPasswordOK()) {
    replyNumeric(fd, Numeric::ERR_PASSWDMISMATCH, ":Incorrect password");
    return;
  }
  if (cmd.params.size() < 2) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS, ":Not enough parameters");
    return;
  }

  //  FIXME: vv Throw here only for development/debugging purposes vv
  if (cmd.params.size() > 3) {
    throw std::runtime_error("Too many params for KICK command");
  }
  // FIXME: ^^ Throw here only for development/debugging purposes ^^

  OptionalChannel          channel;
  std::vector<std::string> users;
  std::string              comment;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    switch (i) {
      case 0: {
        channel = findChannel(cmd.params[i]);
        if (!channel.has_value()) {
          replyNumeric(fd, Numeric::ERR_NOSUCHCHANNEL, ":No such channel");
          return;
        }
        break;
      }
      case 1: {
        std::string       user;
        std::stringstream userStream(cmd.params[i]);
        while (std::getline(userStream, user, ','))
          users.push_back(user);
        break;
      }
      case 2: {
        comment = cmd.params[i];
        break;
      }
    }
  }
  for (size_t i = 0; i < users.size(); ++i) {
    OptionalClient clientToKick = findClientByName(users[i]);
    if (!clientToKick.has_value()) {
      replyNumeric(fd, Numeric::ERR_NOSUCHNICK, ":No such nick");
      continue;
    }
    OptionalUser user = channel->get().findUser(users[i]);
    if (!user.has_value()) {
      replyNumeric(fd, Numeric::ERR_NOSUCHNICK,
                   ":No such nick " + users[i] + " on channel " +
                       channel->get().getName());
      continue;
    }
    int32_t fdToKick = _nickToFd.at(users[i]);
    replyMessage(fdToKick, comment);
    channel->get().kickUser(user->get());
  }
}

void Server::handleNickname(int32_t fd, const Command &cmd) {
  LOG << "handling NICK command";
  Client &client = _clients.at(fd);
  if (!client.isPasswordOK()) {
    replyNumeric(fd, Numeric::ERR_PASSWDMISMATCH, ":Incorrect password");
    return;
  }

  if (cmd.params.empty()) {
    replyNumeric(fd, Numeric::ERR_NONICKNAMEGIVEN, ":No nickname given");
    return;
  }

  if (client.isRegistered() ||
      client.getState() == Client::State::NICK_RECEIVED) {
    replyNumeric(fd, Numeric::ERR_ALREADYREGISTRED,
                 ":Unauthorized command (already registered)");
    return;
  }

  if (Utils::validateNickname(cmd.params[0])) {
    if (isNicknameInUse(cmd.params[0])) {
      replyNumeric(fd, Numeric::ERR_NICKNAMEINUSE, ":Nickname already in use");
      return;
    } else {
      client.setNickname(cmd.params[0]);
      _nickToFd.try_emplace(client.getNickname(), fd);
      client.setState(Client::State::NICK_RECEIVED);
      if (client.isRegistered()) {
        sendWelcomeMessages(fd);
      }
    }
  } else {
    replyNumeric(fd, Numeric::ERR_ERRONEUSNICKNAME, ":Erroneous nickname");
    return;
  }
}

void Server::handleUserJoin(int32_t fd, const Command &cmd) {
  LOG << "handling USER command";
  Client &client = _clients.at(fd);
  if (!client.isPasswordOK()) {
    replyNumeric(fd, Numeric::ERR_PASSWDMISMATCH, ":Incorrect password");
    return;
  }

  if (client.isRegistered() ||
      client.getState() == Client::State::USER_RECEIVED) {
    replyNumeric(fd, Numeric::ERR_ALREADYREGISTRED,
                 ":Unauthorized command (already registered)");
    return;
  }

  if (cmd.params.empty() || cmd.params.size() != 4) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS,
                 "USER :Incorrect parameter count");
    return;
  }

  if (cmd.params[0].find_first_of("@!") != std::string::npos) {
    // Could also just remove illegal chars instead of rejecting Numeric?
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS,
                 "USER :Illegal characters in username");
    return;
  }

  client.setUsername(cmd.params[0].substr(0, 10));
  client.setRealname(cmd.params[3].substr(0, 50));
  client.setState(Client::State::USER_RECEIVED);
  if (client.isRegistered()) {
    sendWelcomeMessages(fd);
  }
}

void Server::handleCapNegotiation(int32_t fd, const Command &cmd) {
  (void)cmd;
  std::string capMsg = "CAP * LS :none\r\n";
  replyMessage(fd, capMsg);
}

void Server::handleQuit(int fd, const Command &cmd) {
  std::string quitMsg = "Client Quit";
  if (!cmd.params.empty()) {
    quitMsg = cmd.params[0];
  }
  Client     &client = _clients.at(fd);
  std::string errorMsg = "ERROR :Closing Link: (Quit: " + quitMsg + ")\r\n";
  replyMessage(fd, errorMsg);
  client.setShouldClose(true);
  LOG << "Client " << fd << " initiated QUIT sequence.";
}

void Server::sendWelcomeMessages(int32_t fd) {
  Client     &client = _clients.at(fd);
  std::string clientMask =
      client.getNickname() + "!" + client.getUsername() + "@localhost";
  replyNumeric(fd, Numeric::RPL_WELCOME,
               ":Welcome to IRC, " + clientMask + "!");
  replyNumeric(fd, Numeric::RPL_YOURHOST,
               std::string(":Your host is ") + SERVER_NAME +
                   "running version " + GIT_HASH);
  replyNumeric(fd, Numeric::RPL_CREATED, ":This Server was created today");
  replyNumeric(fd, Numeric::RPL_MYINFO,
               std::string(SERVER_NAME) + " " + GIT_HASH + " io itkol");
}

void Server::processMessage(int32_t fd, std::optional<Command> const &cmd) {
  Client &client = _clients.at(fd);
  if (cmd.has_value()) {
    auto it = _functionMap.find(cmd->command);
    if (it != _functionMap.end()) {
      auto handler = it->second;
      (this->*handler)(fd, *cmd);
    } else {
      if (!client.isRegistered()) {
        replyNumeric(fd, Numeric::ERR_NOTREGISTERED, ":Client not registered");
      } else {
        replyNumeric(fd, Numeric::ERR_UNKNOWNCOMMAND,
                     cmd->command + " :command not known");
      }
    }
  } else {
    LOG << "Malformed message received from " << client.getNickname();
  }
}

void Server::sendMessageToUser(const std::string &from, const std::string &to,
                               const std::string &message) {
  auto it = _nickToFd.find(to);
  if (it != _nickToFd.end()) {
    replyMessage(it->second, message);
  } else {
    it = _nickToFd.find(from);
    // FIXME: Format message correctly  "<client> <nickname> :No such
    // nick/channel"
    replyNumeric(it->second, Numeric::ERR_NOSUCHNICK, ":No such nick");
  }
}

void Server::sendMessageWithCodeToUser(const std::string &from,
                                       const std::string &to,
                                       const int32_t      code,
                                       const std::string &message) {
  auto it = _nickToFd.find(to);
  if (it != _nickToFd.end()) {
    replyNumeric(it->second, code, message);
  } else {
    it = _nickToFd.find(from);
    // FIXME: Format message correctly  "<client> <nickname> :No such
    // nick/channel"
    replyNumeric(it->second, Numeric::ERR_NOSUCHNICK, ":No such nick");
  }
}

Server::~Server(void) {
  if (_epollFD != -1)
    close(_epollFD);
  if (_epollEvents)
    delete[] _epollEvents;
}

void Server::replyMessage(int32_t fd, std::string const &msg) {
  Client &client = _clients.at(fd);
  client.appendToResponseBuffer(msg);
  LOG << "sending message '" << msg << "' to client " << fd;
  modifyEpoll(fd, EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR);
}

void Server::replyNumeric(int32_t fd, int32_t code, std::string const &msg) {
  std::ostringstream message;
  Client            &client = _clients.at(fd);
  message << ":" << SERVER_NAME << " ";
  message << std::setw(3) << std::setfill('0') << code << " ";
  std::string target = client.getNickname();
  if (target.empty() || !client.isRegistered())
    target = "*";
  message << target << " ";
  message << msg << "\r\n";
  replyMessage(fd, message.str());
}

bool Server::isNicknameInUse(std::string const &nick) {
  for (auto &[fd, client] : _clients) {
    if (client.getNickname() == nick) {
      return true;
    }
  }
  return false;
}

void Server::disconnectUser(int32_t fd) {
  epoll_ctl(_epollFD, EPOLL_CTL_DEL, fd, NULL);
  _clients.erase(fd);
  _sockets.erase(fd);
}

// INFO: Channel management:
Channel &Server::newChannel(const Client &client, const std::string &name) {
  _channels.try_emplace(name, std::make_unique<Channel>(*this, client, name));
  return (*_channels.at(name));
}

std::unordered_map<std::string, std::unique_ptr<Channel>> &Server::getChannels(
    void) {
  return (_channels);
}

std::optional<std::reference_wrapper<Channel>> Server::findChannel(
    const std::string &channelName) const {
  auto it = _channels.find(channelName);
  if (it != _channels.end()) {
    return (std::ref(*(*it).second));
  }
  return (std::nullopt);
}

void Server::modifyEpoll(int32_t fd, uint32_t events) {
  struct epoll_event epoll{};
  epoll.events = events;
  epoll.data.fd = fd;
  if (epoll_ctl(_epollFD, EPOLL_CTL_MOD, fd, &epoll) < 0) {
    LOG << "Failed to modify polling list for " << fd;
  }
}
