#include "Client.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <string>

#include "Channel.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "irc.hpp"

Client::Client(struct sockaddr_in *addr)
    : _responseBuffer(""),
      _recvBuffer(""),
      _nick(""),
      _passwordOK(false),
      _shouldClose(false),
      _state(State::CONNECTED) {
  char ip[INET_ADDRSTRLEN] = {};
  if (inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip))) {
    _hostname = ip;
  } else {
    _hostname = "unknown";
  }
};

Client::~Client() {};

void Client::removeFromResponse(size_t bytes) {
  if (bytes >= _responseBuffer.length()) {
    _responseBuffer.clear();
  } else {
    _responseBuffer.erase(0, bytes);
  }
}

void Client::appendToRecvBuffer(std::string const &input) {
  _recvBuffer.append(input);
  if (_recvBuffer.length() > MAX_RECV_BUFFER &&
      _recvBuffer.find("\r\n") == std::string::npos) {
    _recvBuffer.clear();
    LOG << "Client '" << _nick
        << "' exceeded receive buffer size, erased buffer";
  }
}

void Client::appendToResponseBuffer(std::string const &msg) {
  _responseBuffer.append(msg);
  if (_responseBuffer.length() > MAX_RECV_BUFFER) {
    _responseBuffer.clear();
    LOG << "Client '" << _nick
        << "' exceeded response buffer size, erased buffer";
    // FIXME: kick client at this point?
  }
}

void Client::clearResponseBuffer() {
  _responseBuffer.clear();
}

std::string &Client::getResponseBuffer() {
  return _responseBuffer;
}

/*
void Client::readSocket() {
  char    buffer[RCVBUF_SIZE] = {};
  ssize_t bytesRead = _socket->receiveData(buffer, RCVBUF_SIZE);
  if (bytesRead > 0) {
    _recvBuffer.append(buffer, bytesRead);
  } else if (bytesRead == 0) {
    LOG << "Socket closing, fd: " << _socket->getFD();
    _shouldClose = true;
  } else {
    if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
      throw std::runtime_error("Error reading socket");
    }
  }
}
*/

std::string Client::extractMessage() {
  auto stoppingPoint = _recvBuffer.find("\r\n");
  if (stoppingPoint != std::string::npos) {
    std::string msg = _recvBuffer.substr(0, stoppingPoint);
    _recvBuffer.erase(0, stoppingPoint + 2);
    return msg;
  } else {
    return "";
  }
}

bool Client::shouldClose() {
  return _shouldClose;
}

void Client::setShouldClose(bool b) {
  _shouldClose = b;
}

bool Client::isRegistered() {
  return _state == State::REGISTERED;
}

void Client::setPasswordOK(bool b) {
  _passwordOK = b;
}

bool Client::isPasswordOK() {
  return _passwordOK;
}

void Client::setState(State s) {
  if (_state == State::REGISTERED)
    return;
  if (s == State::NICK_RECEIVED) {
    (_state == State::USER_RECEIVED) ? _state = State::REGISTERED
                                     : _state = State::NICK_RECEIVED;
  }
  if (s == State::USER_RECEIVED) {
    (_state == State::NICK_RECEIVED) ? _state = State::REGISTERED
                                     : _state = State::USER_RECEIVED;
  }
}

Client::State Client::getState() {
  return _state;
}

const std::string &Client::getNickname() const {
  return _nick;
}

void Client::setNickname(std::string const &name) {
  _nick = name;
}

const std::string &Client::getUsername() const {
  return _username;
}

void Client::setUsername(std::string const &name) {
  _username = name;
}

const std::string &Client::getRealname() const {
  return _realname;
}

void Client::setRealname(std::string const &name) {
  _realname = name;
}

const std::string &Client::getHostname() const {
  return _hostname;
}

void Client::setHostname(std::string const &name) {
  _hostname = name;
}

const std::vector<std::string> Client::getChannels(void) const {
  return (_channels);
}

void Client::addChannel(const std::string &channel) {
  _channels.push_back(channel);
}

void Client::removeChannel(const std::string &channelToRemove) {
  std::erase_if(_channels, [&](const std::string &channel) {
    return (channel == channelToRemove);
  });
}

const std::string Client::generatePrefix() const {
  return (":" + this->getNickname() + "!" + this->getUsername() + "@" +
          this->getHostname());
}
