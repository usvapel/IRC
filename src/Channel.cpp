#include "Channel.hpp"

#include <stdexcept>

Channel::Channel(const Server &server, const Client &client,
                 const std::string &name)
    : _server(server), _name(name) {
  User creator(client);
  creator.addOperatorPrivilege();
  _users.try_emplace(client.getNickname(), std::make_unique<User>(creator));
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

unsigned int Channel::getUserCount(void) const {
  return (_users.size());
}

// INFO: Utilities:
Channel::User &Channel::addUser(const Client &client) {
  for (const auto &[key, value] : _users) {
    if (value->getClient() == &client)
      // FIXME:: Inform operator that User already exists?
      return (*value);
  }
  _users.try_emplace(client.getNickname(), std::make_unique<User>(client));
  return (*_users.at(client.getNickname()));
}

Channel::User &Channel::findUser(const std::string &nickname) {
  for (auto &[key, value] : _users) {
    if (value->getNickName() == nickname)
      return (*value);
  }
  throw std::runtime_error("User " + nickname + " not found");
}

void Channel::resetFlags(void) {
  _channelFlags = 0;
}

void Channel::toggleFlag(const ChannelFlag flag) {
  _channelFlags ^= static_cast<uint16_t>(flag);
}

bool Channel::isFlagOn(const ChannelFlag flag) {
  return (_channelFlags & static_cast<uint16_t>(flag));
}

// INFO: Operator commands:
void Channel::kickUser(Channel::User &target) {
  // FIXME: What else needs to be done when kicking?

  std::erase_if(_users, [&](const auto &item) {
    auto const &[nickname, user] = item;
    return (target.getClient() == user->getClient());
  });
}

void Channel::kickUser(const std::string nickname) {
  User &target = findUser(nickname);
  kickUser(target);
}

void Channel::inviteUser(const std::string &nickname) {
  (void)nickname;
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

Channel::User::~User() {}

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
