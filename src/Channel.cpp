#include "Channel.hpp"

#include "Client.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "Utils.hpp"

Channel::Channel(Server &server, const Client &client, const std::string &name)
    : _server(server), _name(name) {
  auto creator = std::make_unique<User>(client);
  creator->setOperatorPrivilege(true);
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

std::optional<std::reference_wrapper<Channel::User>> Channel::tryAddUser(
    const Client &client, const std::string &key) {
  const std::string &nickname = client.getNickname();
  if (isModeOn(ChannelMode::INVITE_ONLY)) {
    auto it = std::ranges::find(_inviteList, nickname);
    if (it == _inviteList.end()) {
      _server.sendMessageWithCodeToUser(
          nickname, nickname, Numeric::ERR_INVITEONLYCHAN,
          this->getName() + " :Cannot join channel (+i)");
      LOG << nickname + " can't join channel " + this->getName() +
                 " because it's invite only\n";
      return (std::nullopt);
    } else {
      _inviteList.erase(it);
    }
  }
  if (isModeOn(ChannelMode::KEY_PROTECTED)) {
    if (key != _key) {
      _server.sendMessageWithCodeToUser(
          nickname, nickname, Numeric::ERR_BADCHANNELKEY,
          this->getName() + " :Cannot join channel (+k)");
      LOG << nickname + " can't join channel " + this->getName() +
                 " because keys don't match\n";
      return (std::nullopt);
    }
  }
  if (_users.size() >= _userLimit) {
    _server.sendMessageWithCodeToUser(
        nickname, nickname, Numeric::ERR_CHANNELISFULL,
        this->getName() + " :Cannot join channel (+l)");
    LOG << nickname + " can't join channel " + this->getName() +
               " because it's full\n";
    return (std::nullopt);
  }
  auto it = _users.find(nickname);
  if (it != _users.end()) {
    _server.sendMessageWithCodeToUser(
        nickname, nickname, Numeric::ERR_USERONCHANNEL,
        nickname + " " + this->getName() + " :is already on channel");
    LOG << nickname + " is already on channel " + this->getName() + "\n";
    return (std::nullopt);
  }
  return (addUser(client));
}

bool Channel::tryAddInvite(const User &senderUser, const std::string &invited) {
  std::string sender = senderUser.getNickName();
  if (senderUser.isOperator() == false) {
    _server.sendMessageWithCodeToUser(sender, sender,
                                      Numeric::ERR_CHANOPRIVSNEEDED,
                                      "You're not channel operator");
    return (false);
  }
  OptionalClient invitedClient = _server.findClientByName(invited);
  if (!invitedClient) {
    _server.sendMessageWithCodeToUser(
        sender, sender, Numeric::ERR_NOSUCHNICK,
        invited + " " + sender + " :No such nick");
    return (false);
  }
  auto userIt = _users.find(invited);
  if (userIt != _users.end()) {
    _server.sendMessageWithCodeToUser(
        sender, sender, Numeric::ERR_USERONCHANNEL,
        invited + " " + this->getName() + " :is already on channel");
    return (false);
  }
  auto inviteIt = std::ranges::find(_inviteList, invited);
  if (inviteIt != _inviteList.end()) {
    return (true);
  } else {
    _inviteList.push_back(invited);
    return (true);
  }
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
  if (_topic.length() > 0) {
    std::string topicMessage = channel + " :" + this->getTopic();
    _server.sendMessageWithCodeToUser(nick, nick, Numeric::RPL_TOPIC,
                                      topicMessage);
  }
  std::string prefix;
  if (user.isOperator()) {
    prefix = "@";
  } else {
    prefix = "";
  }
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

void Channel::removeUser(const std::string nickname) {
  auto it = _users.find(nickname);
  if (it != _users.end()) {
    _users.erase(it);
    if (_users.size() == 0) {
      _server.removeEmptyChannel(_name);
    }
  }
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

void Channel::User::setOperatorPrivilege(const bool status) {
  _isOperator = status;
}

bool Channel::User::isOperator(void) const {
  return (_isOperator);
}
