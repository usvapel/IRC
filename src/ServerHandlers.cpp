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
  Client &client = _clients.at(fd);
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
