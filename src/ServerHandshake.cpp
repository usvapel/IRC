#include "Client.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "Utils.hpp"

bool Server::passwordIsCorrect(const std::string &pwd) {
  return (_pwd == pwd);
}

void Server::sendWelcomeMessages(int32_t fd) {
  Client     &client = _clients.at(fd);
  std::string clientMask = client.getNickname() + "!" + client.getUsername() +
                           "@" + client.getHostname();
  replyNumeric(fd, Numeric::RPL_WELCOME,
               +":Welcome to the Internet Relay Network " + clientMask);
  replyNumeric(fd, Numeric::RPL_YOURHOST,
               std::string(":Your host is ") + SERVER_NAME +
                   " running version " + GIT_HASH);
  replyNumeric(fd, Numeric::RPL_CREATED, ":This Server was created today");
  replyNumeric(fd, Numeric::RPL_MYINFO,
               std::string(SERVER_NAME) + " " + GIT_HASH + " io itkol");
}

bool Server::isNicknameInUse(std::string const &nick) {
  for (auto &[fd, client] : _clients) {
    if (client.getNickname() == nick) {
      return true;
    }
  }
  return false;
}

void Server::handleCapNegotiation(int32_t fd, const Command &cmd) {
  std::string capMsg = "CAP * LS :none";
  if (cmd.params.size() >= 1 && cmd.params[0] != "END")
    replyMessage(fd, capMsg);
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
