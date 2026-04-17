#include "Channel.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include "Client.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "Utils.hpp"

Channel::Channel(Server &server, const Client &client, const std::string &name)
    : _server(server), _name(name) {
  auto creator = std::make_unique<User>(client);
  creator->addOperatorPrivilege();
  _users.try_emplace(client.getNickname(), std::move(creator));
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
  _timeCreated = std::to_string(seconds.count());
}

Channel::~Channel() {}

// INFO: Getters and setters:
const std::string &Channel::getName(void) const {
  return (_name);
}

void Channel::setName(const std::string &name) {
  _name = name;
}

const std::string &Channel::getTopic(void) const {
  return (_topic);
}

void Channel::setTopic(const std::string &topic) {
  _topic = topic;
}

std::string &Channel::getNewModes() {
  return _newModes;
}

void Channel::setNewModes(const std::string &modes) {
  _newModes = modes;
}

const std::string &Channel::getUNIXTimeCreated(void) const {
  return (_timeCreated);
}

std::string Channel::getModes(const std::string &nickname) const {
  OptionalUser user = findUser(nickname);
  std::string  modestring = "+";
  std::string  modeArgs{};
  if (isModeOn(ChannelMode::LIMITED_USER_COUNT)) {
    modestring.append("l");
    if (user) {
      modeArgs.append(" ").append(std::to_string(_userLimit));
    }
  }
  if (isModeOn(ChannelMode::INVITE_ONLY)) {
    modestring.append("i");
  }
  if (isModeOn(ChannelMode::KEY_PROTECTED)) {
    modestring.append("k");
    if (user) {
      if (user->get().isOperator()) {
        modeArgs.append(" ").append(_key);
      } else {
        modeArgs.append(" ").append(_key.length(), '*');
      }
    }
  }
  if (isModeOn(ChannelMode::TOPIC_SET_BY_CHANOP_ONLY)) {
    modestring.append("t");
  }
  return (modestring + modeArgs);
}

void Channel::setUserLimit(const uint32_t limit) {
  // if (_users.size() > limit)
  // FIXME: How to handle this edge case?
  // IRC server won't kick people out, but it won't allow new users to join.
  _userLimit = limit;
}

void Channel::setKey(const std::string &key) {
  _key = key;
}

uint32_t Channel::getUserLimit(void) const {
  return (_userLimit);
}

unsigned int Channel::getUserCount(void) const {
  return (_users.size());
}

// INFO: Utilities:
std::optional<std::reference_wrapper<Channel::User>> Channel::tryAddUser(
    const Client &client, const std::string &key) {
  if (isModeOn(ChannelMode::KEY_PROTECTED)) {
    if (key != _key) {
      _server.sendMessageWithCodeToUser(
          client.getNickname(), client.getNickname(),
          Numeric::ERR_BADCHANNELKEY,
          this->getName() + " :Cannot join channel (+k)");
      LOG << client.getNickname() + " can't join channel " + this->getName() +
                 " because keys don't match\n";
      return (std::nullopt);
    }
  }
  if (_users.size() >= _userLimit) {
    _server.sendMessageWithCodeToUser(
        client.getNickname(), client.getNickname(), Numeric::ERR_CHANNELISFULL,
        this->getName() + " :Cannot join channel (+l)");
    LOG << client.getNickname() + " can't join channel " + this->getName() +
               " because it's full\n";
    return (std::nullopt);
  }
  auto it = _users.find(client.getNickname());
  if (it != _users.end()) {
    _server.sendMessageWithCodeToUser(
        client.getNickname(), client.getNickname(), Numeric::ERR_USERONCHANNEL,
        client.getNickname() + " " + this->getName() +
            " :is already on channel");
    LOG << client.getNickname() + " is already on channel " + this->getName() +
               "\n";
    return (std::nullopt);
  }
  return (addUser(client));
}

std::optional<std::reference_wrapper<Channel::User>> Channel::addUser(
    const Client &client) {
  _users.try_emplace(client.getNickname(), std::make_unique<User>(client));
  return (*_users.at(client.getNickname()));
}

std::optional<std::reference_wrapper<Channel::User>> Channel::findUser(
    const std::string &nickname) const {
  auto it = _users.find(nickname);
  if (it != _users.end()) {
    return (std::ref(*(*it).second));
  }
  return std::nullopt;
}

void Channel::messageAllUsersOnChannel(const std::string &message) {
  for (const auto &e : _users) {
    _server.sendMessageToUser(e.first, e.first, message);
  }
}

void Channel::messageAllUsersOnChannel(const std::string &message,
                                       const int32_t      code) {
  for (const auto &e : _users) {
    _server.sendMessageWithCodeToUser(e.first, e.first, code, message);
  }
}

void Channel::messageAllUsersOnChannel(const std::string &sender,
                                       const std::string &message) {
  for (const auto &e : _users) {
    if (e.first == sender) {
      continue;
    }
    _server.sendMessageToUser(e.first, e.first, message);
  }
}
void Channel::messageAllUsersOnChannel(const std::string &sender,
                                       const std::string &message,
                                       const int32_t      code) {
  for (const auto &e : _users) {
    if (e.first == sender) {
      continue;
    }
    _server.sendMessageWithCodeToUser(e.first, e.first, code, message);
  }
}

void Channel::messageNewUserJoining(Client &clientToAdd) {
  const std::string &nick = clientToAdd.getNickname();
  const std::string &channel = this->getName();

  auto it = _users.find(nick);
  if (it == _users.end())
    return;
  User       &user = *it->second;
  std::string joinMessage = clientToAdd.generatePrefix() + " JOIN " + channel;
  messageAllUsersOnChannel(joinMessage);
  // FIXME: Change _topic.length() to a mode check?
  if (_topic.length() > 0) {
    std::string topicMessage = channel + " :" + this->getTopic();
    _server.sendMessageWithCodeToUser(nick, nick, Numeric::RPL_TOPIC,
                                      topicMessage);
  }
  // FIXME: Need to include other prefixes than operator?
  std::string prefix;
  if (user.isOperator()) {
    prefix = "@";
  } else {
    prefix = "";
  }
  // FIXME: Change '=' to be handled dynamically if implementing secret channel
  // or private channel.
  std::string nameReply =
      "= " + channel + " :" + prefix + nick + this->userList(user);
  _server.sendMessageWithCodeToUser(nick, nick, Numeric::RPL_NAMERPLY,
                                    nameReply);
  std::string endOfNamesReply = channel + " :End of /NAMES list";
  _server.sendMessageWithCodeToUser(nick, nick, Numeric::RPL_ENDOFNAMES,
                                    endOfNamesReply);
}

void Channel::resetModes(void) {
  _channelModes = 0;
}

void Channel::setMode(const ChannelMode mode, const bool status) {
  if (status == false) {
    if (isModeOn(mode)) {
      toggleMode(mode);
    }
  } else {
    if (isModeOn(mode) == false) {
      toggleMode(mode);
    }
  }
}

void Channel::toggleMode(const ChannelMode flag) {
  _channelModes ^= static_cast<uint16_t>(flag);
}

bool Channel::isModeOn(const ChannelMode flag) const {
  return (_channelModes & static_cast<uint16_t>(flag));
}

void Channel::tryKickUser(const std::string nickname) {
  std::optional<std::reference_wrapper<User>> target = findUser(nickname);
  if (target) {
    kickUser(target.value().get());
    return;
  }
  // FIXME: Inform operator that user not found?
  std::cout << "Can't kick "
            << nickname + " because they are not found on the server\n";
}

// INFO: Operator commands:
void Channel::kickUser(Channel::User &target) {
  auto it = _users.find(target.getNickName());
  if (it != _users.end())
    _users.erase(it);
}

void Channel::inviteUser(const std::string &nickname) {
  (void)nickname;
}

std::string Channel::userList(void) const {
  std::string list;
  for (auto it = _users.begin(); it != _users.end(); ++it) {
    if (it != _users.begin()) {
      list += " ";
    }
    if (it->second->isOperator()) {
      list += "@" + it->first;
    } else {
      list += it->first;
    }
  }
  return (list);
}

std::string Channel::userList(const User &userToSkip) const {
  std::string list;
  for (auto it = _users.begin(); it != _users.end(); ++it) {
    if (it != _users.begin()) {
      list += " ";
    }
    if (it->first == userToSkip.getNickName())
      continue;
    if (it->second->isOperator()) {
      list += "@" + it->first;
    } else {
      list += it->first;
    }
  }
  return (list);
}

bool Channel::keyIsCorrect(const std::string &key) const {
  return (key == _key);
}

// INFO: Channel::User:
Channel::User::User(const Client &client) : _client(&client) {}

Channel::User::User(const User &other)
    : _client(other._client), _isOperator(other._isOperator) {}

Channel::User &Channel::User::operator&(const User &other) {
  if (this == &other)
    return (*this);
  _client = other._client;
  _isOperator = other._isOperator;
  return (*this);
}

const Client *Channel::User::getClient(void) const {
  return (_client);
}

const std::string &Channel::User::getNickName(void) const {
  return (_client->getNickname());
}

void Channel::User::toggleOperatorPrivilege(void) {
  _isOperator = !_isOperator;
}

void Channel::User::addOperatorPrivilege(void) {
  _isOperator = true;
}

void Channel::User::removeOperatorPrivilege(void) {
  _isOperator = false;
}

bool Channel::User::isOperator(void) const {
  return (_isOperator);
}
