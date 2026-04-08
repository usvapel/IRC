#include <iomanip>

#include "Client.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "Utils.hpp"

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
