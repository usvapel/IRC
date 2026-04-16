#include <assert.h>

#include <cstdlib>
#include <iostream>
#include <span>
#include <sstream>
#include <string>

#include "Channel.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "Utils.hpp"

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

void Server::removeEmptyChannels(void) {
  std::vector<std::string> removedChannels;
  const int count = std::erase_if(_channels, [&](const auto &channel) {
    const auto &[channelName, channelPointer] = channel;
    if (channelPointer->getUserCount() == 0) {
      removedChannels.push_back(channelPointer->getName());
      return (true);
    }
    return (false);
  });
  if (count) {
    LOG << std::to_string(count) + " channels removed:\n";
    for (auto &channelName : removedChannels) {
      LOG << channelName + "\n";
    }
  }
}

// INFO: PRIVMSG
void Server::handleMsg(int32_t fd, const Command &cmd) {
  bool privMsg = cmd.command == "PRIVMSG";
  LOG << "handling " << (privMsg ? "PRIVMSG" : "NOTICE") << " command";
  std::string buffer;
  for (auto &it : std::span(cmd.params).subspan(1))
    buffer += it;
  OptionalClient sender = _clients.at(fd);
  if (!sender)
    return;
  std::string prefix = sender->get().generatePrefix();
  // INFO: channel
  if (cmd.params[0][0] == '#' || cmd.params[0][0] == '&') {
    OptionalChannel channel = findChannel(cmd.params[0]);
    if (!channel) {
      std::string errStr = cmd.params[0] + " " + sender->get().getNickname() +
                           " :No such channel";
      if (privMsg) {
        replyNumeric(fd, Numeric::ERR_NOSUCHCHANNEL, errStr);
      }
      return;
    }
    std::string fullMessage = prefix + (privMsg ? " PRIVMSG " : " NOTICE ") +
                              cmd.params[0] + " :" + buffer;
    channel->get().messageAllUsersOnChannel(sender->get().getNickname(),
                                            fullMessage);
    return;
  }

  // INFO: /msg
  OptionalClient target = findClientByName(cmd.params[0]);
  if (!target) {
    std::string errStr =
        cmd.params[0] + " " + sender->get().getNickname() + " :No such nick";
    if (privMsg) {
      replyNumeric(fd, Numeric::ERR_NOSUCHNICK, errStr);
    }
    return;
  }
  std::string targetNick(target->get().getNickname());
  std::string fullMessage = prefix + (privMsg ? " PRIVMSG " : " NOTICE ") +
                            targetNick + " :" + buffer;
  replyMessage(_nickToFd.at(targetNick), fullMessage);
}

// INFO: TOPIC
void Server::handleTopic(int32_t fd, const Command &cmd) {
  LOG << "handling TOPIC command";
  if (cmd.params.size() < 1) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS, ":Not enough parameters");
    return;
  }

  OptionalChannel channel = findChannel(cmd.params[0]);
  if (!channel) {
    replyNumeric(fd, Numeric::ERR_NOSUCHCHANNEL, ":No such channel");
    return;
  }

  const std::string &nick = _clients.at(fd).getNickname();
  OptionalUser       user = channel->get().findUser(nick);
  if (!user) {
    replyNumeric(fd, Numeric::ERR_NOTONCHANNEL, ":You're not on that channel");
    return;
  }

  const std::string &topic = channel->get().getTopic();
  if (cmd.params.size() < 2) {
    if (topic.empty())
      replyNumeric(fd, Numeric::RPL_NOTOPIC, ":No topic is set");
    else
      replyNumeric(fd, Numeric::RPL_TOPIC, ":" + topic);
    return;
  }

  if (channel->get().isModeOn(Channel::ChannelMode::TOPIC_SET_BY_CHANOP_ONLY)) {
    if (!user->get().isOperator()) {
      replyNumeric(fd, Numeric::ERR_CHANOPRIVSNEEDED,
                   ":You're not channel operator");
      return;
    }
  }

  const std::string &new_topic = cmd.params[1];
  channel->get().setTopic(new_topic);
  OptionalClient client = findClientByName(nick);
  std::string    prefix = client->get().generatePrefix();
  std::string    topicMessage = prefix + " " + cmd.command + " " +
                             channel->get().getName() + " :" + new_topic;
  channel->get().messageAllUsersOnChannel(topicMessage);
  return;
}

// INFO: INVITE
void Server::handleInvite(int32_t fd, const Command &cmd) {
  LOG << "handling INVITE command";
  if (cmd.params.size() < 2) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS, ":Not enough parameters");
    return;
  }

  OptionalChannel channel = findChannel(cmd.params[1]);
  if (!channel) {
    replyNumeric(fd, Numeric::ERR_NOSUCHCHANNEL, ":" + cmd.params[1]);
    return;
  }

  const std::string &senderNick = _clients.at(fd).getNickname();
  OptionalUser       senderUser = channel->get().findUser(senderNick);
  if (!senderUser) {
    replyNumeric(fd, Numeric::ERR_NOTONCHANNEL, "You're not on that channel");
    return;
  }

  if (channel->get().isModeOn(Channel::ChannelMode::INVITE_ONLY)) {
    if (!senderUser->get().isOperator()) {
      replyNumeric(fd, Numeric::ERR_CHANOPRIVSNEEDED,
                   "You're not channel operator");
      return;
    }
  }

  const std::string &targetNick = cmd.params[0];
  const std::string &channelName = channel->get().getName();
  if (channel->get().findUser(targetNick)) {
    replyNumeric(fd, Numeric::ERR_USERONCHANNEL,
                 targetNick + " " + channelName + " :is already on channel");
    return;
  }

  const std::string &messageToSender = targetNick + " :" + channelName;
  replyNumeric(fd, Numeric::RPL_INVITING, messageToSender);

  OptionalClient     senderClient = findClientByName(senderNick);
  const std::string &prefix = senderClient->get().generatePrefix();
  const std::string &messageToTarget =
      prefix + " INVITE " + targetNick + " :" + channelName;
  sendMessageToUser(senderNick, targetNick, messageToTarget);
  sendMessageToUser(senderNick, senderNick, messageToTarget);
  return;
}

// INFO: PART
void Server::handlePart(int32_t fd, const Command &cmd) {
  LOG << "handling PART command";
  if (cmd.params.size() < 1) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS, ":Not enough parameters");
    return;
  }

  //  FIXME: vv Throw here only for development/debugging purposes vv
  if (cmd.params.size() > 2) {
    throw std::runtime_error("Too many params for PART command");
  }
  // FIXME: ^^ Throw here only for development/debugging purposes ^^

  std::vector<OptionalChannel> channels;
  std::string                  reason;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    switch (i) {
      case 0: {
        std::string       channelName;
        std::stringstream channelStream(cmd.params[i]);
        while (std::getline(channelStream, channelName, ',')) {
          OptionalChannel optChannel = findChannel(channelName);
          if (!optChannel.has_value()) {
            replyNumeric(fd, Numeric::ERR_NOSUCHCHANNEL, ":No such channel");
            continue;
          }
          channels.push_back(optChannel);
        }
        break;
      }
      case 1: {
        reason = cmd.params[i];
        break;
      }
    }
  }
  if (channels.size() == 0) {
    return;
  }
  auto it = _clients.find(fd);
  if (it == _clients.end()) {
    return;
  }
  OptionalClient client = it->second;
  if (!client.has_value()) {
    return;
  }
  std::string prefix = client->get().generatePrefix();
  for (auto &channel : channels) {
    OptionalUser optUser = channel->get().findUser(client->get().getNickname());
    if (!optUser.has_value()) {
      replyNumeric(fd, Numeric::ERR_NOTONCHANNEL,
                   ":You're not on that channel");
      continue;
    }
    std::string partMessage =
        prefix + " " + cmd.command + " " + channel->get().getName();
    if (reason.size() > 0) {
      partMessage += " :" + reason;
    } else {
      partMessage += " :Bye bye!";
    }
    channel->get().messageAllUsersOnChannel(partMessage);
    channel->get().kickUser(optUser->get());
    client->get().removeChannel(channel->get().getName());
  }
}

// INFO: KICK
void Server::handleKick(int32_t fd, const Command &cmd) {
  LOG << "handling KICK command";
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
        while (std::getline(userStream, user, ',')) {
          users.push_back(user);
        }
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
      std::string sender = _clients.at(fd).getNickname();
      std::string errorMessage = users[i] + " " + sender + " :No such nick";
      replyNumeric(fd, Numeric::ERR_NOSUCHNICK, errorMessage);
      continue;
    }
    OptionalUser user = channel->get().findUser(users[i]);
    if (!user.has_value()) {
      std::string errorMessage = users[i] + " " + channel->get().getName() +
                                 " :They aren't on the channel";
      replyNumeric(fd, Numeric::ERR_USERNOTINCHANNEL, errorMessage);
      continue;
    }
    std::string kickMessage =
        cmd.command + " " + channel->get().getName() + " " + users[i];
    if (comment.empty() == false) {
      kickMessage += " :" + comment;
    } else {
      kickMessage += " :Bye bye!";
    }
    channel->get().messageAllUsersOnChannel(kickMessage);
    channel->get().kickUser(user->get());
    clientToKick->get().removeChannel(channel->get().getName());
  }
}

// INFO: JOIN
void Server::handleJoin(int32_t fd, const Command &cmd) {
  LOG << "handling JOIN command";
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
  Client &clientToAdd = _clients.at(fd);
  for (size_t i = 0; i < channelNames.size(); ++i) {
    OptionalChannel channel = findChannel(channelNames[i]);
    LOG << clientToAdd.getNickname() + " trying to join " + channelNames[i];
    if (!channel.has_value()) {
      if (channelNames[i][0] != '#' && channelNames[i][0] != '&') {
        LOG << channelNames[i] + " bad channel mask";
        replyNumeric(fd, Numeric::ERR_BADCHANMASK, ":" + channelNames[i]);
        continue;
      }
      LOG << channelNames[i] + " not found. Creating channel";
      Channel &createdChannel = newChannel(clientToAdd, channelNames[i]);
      createdChannel.messageNewUserJoining(clientToAdd);
      clientToAdd.addChannel(channelNames[i]);
      continue;
    } else {
      LOG << clientToAdd.getNickname() + " joining channel " + channelNames[i];
      std::string  key = channelKeys.size() > 0 ? channelKeys[i] : "";
      OptionalUser user = channel->get().tryAddUser(clientToAdd, key);
      if (user.has_value()) {
        channel->get().messageNewUserJoining(clientToAdd);
        clientToAdd.addChannel(channelNames[i]);
      }
    }
  }
}

// INFO: QUIT
void Server::handleQuit(int fd, const Command &cmd) {
  std::string quitMsg = "Client Quit";
  if (!cmd.params.empty()) {
    quitMsg = cmd.params[0];
  }
  Client     &client = _clients.at(fd);
  std::string errorMsg = "ERROR :Closing Link: (Quit: " + quitMsg + ")";
  replyMessage(fd, errorMsg);
  client.setShouldClose(true);
  LOG << "Client " << fd << " initiated QUIT sequence.";
}

void Server::handlePing(int32_t fd, const Command &cmd) {
  std::string msg = ":" SERVER_NAME " PONG " SERVER_NAME;
  if (!cmd.params.empty()) {
    msg += " " + cmd.params[0];
  }
  replyMessage(fd, msg);
}

void Server::handleMode(int32_t fd, const Command &cmd) {
  Client     &client = _clients.at(fd);
  std::string nickname = client.getNickname();

  //  FIXME: vv Throw here only for development/debugging purposes vv
  if (cmd.params.size() > 2) {
    throw std::runtime_error("Too many params for MODE command");
  }
  // FIXME: ^^ Throw here only for development/debugging purposes ^^

  if (cmd.params.size() < 1) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS, ":Not enough parameters");
    return;
  }
  OptionalChannel channel = findChannel(cmd.params[0]);
  if (!channel) {
    replyNumeric(fd, Numeric::ERR_NOSUCHCHANNEL, ":No such channel");
    return;
  }
  if (cmd.params.size() == 1) {
    // FIXME: Format message correctly.
    replyNumeric(fd, Numeric::RPL_CHANNELMODEIS, "");
    // FIXME: Server's SHOULD return RPL_CREATIONTIME after RPL_CHANNELMODEIS
    replyNumeric(fd, Numeric::RPL_CREATIONTIME, "");
    return;
  } else {
    OptionalUser user = channel->get().findUser(nickname);
    if (!user || user->get().isOperator() == false) {
      replyNumeric(fd, Numeric::ERR_CHANOPRIVSNEEDED,
                   ":You're not channel operator");
      return;
    }
    // FIXME: Parse mode message and apply to channel!
    // Iterate through the parsed modes and use:
    // channel->get().setMode(const ChannelMode mode, const bool status)
  }
}
