#include "Parser.hpp"

#include <bits/stdc++.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>

#include "Channel.hpp"
#include "Server.hpp"
#include "Utils.hpp"

int32_t Parser::channelModeParse(const Command &cmd, Channel &channel) {
  bool               onOff = false;
  size_t             index = 2;
  const std::string &modestring = cmd.params[1];
  for (auto &c : modestring) {
    switch (c) {
      case '+':
        onOff = true;
        continue;
      case '-':
        onOff = false;
        continue;
      case 'i':
        channel.setMode(Channel::ChannelMode::INVITE_ONLY, onOff);
        continue;
      case 't':
        channel.setMode(Channel::ChannelMode::TOPIC_SET_BY_CHANOP_ONLY, onOff);
        continue;
      case 'k':
        if (index > modestring.size())
          continue;
        if (index >= cmd.params.size())
          continue;
        channel.setMode(Channel::ChannelMode::KEY_PROTECTED, onOff);
        channel.setKey(cmd.params[index]);
        index++;
        continue;
      case 'o':
        if (index > modestring.size())
          continue;
        if (index >= cmd.params.size())
          continue;
        channel.findUser(cmd.params[index])->get().toggleOperatorPrivilege();
        index++;
        continue;
      case 'l':
        if (index > modestring.size() && onOff == true)
          continue;
        if (index >= cmd.params.size())
          continue;
        if (onOff == false) {
          channel.setUserLimit(UINT32_MAX);
          continue;
        }
        if (!std::ranges::all_of(cmd.params[index], ::isdigit))
          continue;
        channel.setMode(Channel::ChannelMode::LIMITED_USER_COUNT, onOff);
        try {
          channel.setUserLimit(
              static_cast<uint32_t>(std::stoul(cmd.params[index])));
        } catch (...) {}
        index++;
        continue;
      default:
        return index;
    }
  }
  return -1;
}

std::optional<Command> Parser::parse(std::string message) {
  Command cmd;

  if (message.empty())
    return std::nullopt;

  auto endpoint = message.find_first_of(' ');
  if (message[0] == ':') {
    if (endpoint == std::string::npos)
      return std::nullopt;
    cmd.prefix = std::string(message.substr(1, endpoint - 1));
    message.erase(0, endpoint + 1);
  }

  endpoint = message.find_first_of(' ');
  cmd.command = std::string(message.substr(0, endpoint));
  if (endpoint != std::string::npos) {
    message.erase(0, endpoint + 1);
  } else {
    return cmd;
  }

  while (!message.empty()) {
    if (message[0] == ' ') {
      message.erase(0, 1);
      continue;
    }
    endpoint = message.find_first_of(' ');
    if (message[0] == ':') {
      cmd.params.push_back(std::string(message.substr(1)));
      break;
    }
    cmd.params.push_back(std::string(message.substr(0, endpoint)));
    if (endpoint != std::string::npos) {
      message.erase(0, endpoint + 1);
    } else {
      break;
    }
  }
  return cmd;
}
