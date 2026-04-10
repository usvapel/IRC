#include <assert.h>

#include <ranges>

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

// INFO: PRIVMSG
void Server::handlePrivMsg(int32_t fd, const Command &cmd) {
  LOG << "handling PRIVMSG command";
  std::string buffer;
  for (auto &it : std::span(cmd.params).subspan(1))
    buffer += it;
  OptionalClient sender = _clients.at(fd);
  if (!sender)
    return;
  std::string prefix = ":" + sender->get().getNickname() + "!~" +
                       sender->get().getUsername() + "@" +
                       sender->get().getHostname();
  // INFO: channel
  if (cmd.params[0][0] == '#') {
    OptionalChannel channel = findChannel(cmd.params[0]);
    if (!channel) {
      channel = newChannel(*sender, cmd.params[0]);
      _channels.try_emplace(cmd.params[0], &channel->get());
    }
    std::string fullMessage =
        prefix + " PRIVMSG " + cmd.params[0] + " :" + buffer + "\r\n";
    for (auto &member : channel->get().getUsers()) {
      if (!member.second || !member.second->getClient() ||
          member.second->getClient() == &sender->get())
        continue;
      const auto &nick = member.second->getNickName();
      auto        it = _nickToFd.find(nick);
      if (it == _nickToFd.end())
        continue;
      replyMessage(it->second, fullMessage);
    }
    return;
  }

  // INFO: /msg
  OptionalClient target = findClientByName(cmd.params[0]);
  if (!target) {
    std::string errStr =
        sender->get().getNickname() + " " + cmd.params[0] + " :No such nick";
    replyNumeric(fd, Numeric::ERR_NOSUCHNICK, errStr);
    return;
  }
  std::string targetNick(target->get().getNickname());
  std::string fullMessage =
      prefix + " PRIVMSG " + targetNick + " :" + buffer + "\r\n";
  replyMessage(_nickToFd.at(targetNick), fullMessage);
}

// INFO: KICK
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
}

// INFO: JOIN
void Server::handleJoin(int32_t fd, const Command &cmd) {
  LOG << "handling JOIN command";
  Client         &client = _clients.at(fd);
  OptionalChannel channel = findChannel(cmd.params[0]);
  if (!channel) {
    std::cout << "channel not found, creating new channel " << cmd.params[0]
              << '\n';
    channel = newChannel(_clients.find(fd)->second, cmd.params[0]);
    _channels.try_emplace(cmd.params[0], &channel->get());
  }
  channel->get().addUser(client);
  if (!client.isPasswordOK()) {
    replyNumeric(fd, Numeric::ERR_PASSWDMISMATCH, ":Incorrect password");
    return;
  }
  if (cmd.params.size() < 1) {
    replyNumeric(fd, Numeric::ERR_NEEDMOREPARAMS, ":Not enough parameters");
    return;
  }
}

// INFO: QUIT
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
