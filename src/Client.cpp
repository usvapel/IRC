#include "Client.hpp"

#include <sys/socket.h>

#include <cerrno>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Logger.hpp"
#include "Server.hpp"
#include "Socket.hpp"

Client::Client()
    : _responseBuffer(""),
      _recvBuffer(""),
      _passwordOK(false),
      _shouldClose(false),
      _state(State::CONNECTED) {};

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
}

void Client::appendToResponseBuffer(std::string const &msg) {
  _responseBuffer.append(msg);
}

std::string Client::getResponseBuffer() {
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
    std::string msg = _recvBuffer.substr(0, stoppingPoint + 2);
    _recvBuffer.erase(0, stoppingPoint + 2);
    return msg;
  } else {
    return "";
  }
}

bool Client::shouldClose() {
  return _shouldClose;
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

std::string Client::getNickname() {
  return _nick;
}

void Client::setNickname(std::string const &name) {
  _nick = name;
}

std::string Client::getUsername() {
  return _username;
}

void Client::setUsername(std::string const &name) {
  _username = name;
}

std::string Client::getRealname() {
  return _realname;
}

void Client::setRealname(std::string const &name) {
  _realname = name;
}
