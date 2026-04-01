#include "Channel.hpp"

Channel::Channel(const Client &client, const std::string &name) : _name(name) {}

Channel::~Channel() {}

void Channel::addUser(Client &client) {
  _users.emplace_back(client);
}

const std::string &Channel::getName(void) const {
  return (_name);
}

unsigned int Channel::getUserCount(void) const {
  return (_users.size());
}

void Channel::resetChannelFlags(void) {
  _channelFlags = 0;
}

void Channel::toggleChannelFlag(const ChannelFlag flag) {
  _channelFlags ^= static_cast<uint16_t>(flag);
}

bool Channel::isFlagOn(const ChannelFlag flag) {
  return (_channelFlags & static_cast<uint16_t>(flag));
}

// NOTE: USER:

Channel::User::User(const Client &client) {}
Channel::User::~User() {}
