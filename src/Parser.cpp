#include "Parser.hpp"

#include <algorithm>

#include "Channel.hpp"
#include "Server.hpp"
#include "Utils.hpp"

int32_t Parser::channelModeParse(const Command &cmd, Channel &channel) {
  bool               onOff = false;
  size_t             index = 2;
  std::string        onBuffer = "";
  std::string        offBuffer = "";
  const std::string &modestring = cmd.params[1];

  auto append = [&](char c) { (onOff ? onBuffer : offBuffer) += c; };

  for (auto &c : modestring) {
    switch (c) {
      case '+':
        onOff = true;
        onBuffer += '+';
        break;

      case '-':
        onOff = false;
        offBuffer += '-';
        break;

      case 'i':
        append('i');
        channel.setMode(Channel::ChannelMode::INVITE_ONLY, onOff);
        break;

      case 't':
        append('t');
        channel.setMode(Channel::ChannelMode::TOPIC_SET_BY_CHANOP_ONLY, onOff);
        break;

      case 'k': {
        if (!onOff) {
          if (channel.isModeOn(Channel::ChannelMode::KEY_PROTECTED))
            offBuffer += 'k';
          channel.setMode(Channel::ChannelMode::KEY_PROTECTED, false);
          channel.setKey("");
          break;
        }

        if (index >= cmd.params.size())
          break;

        channel.setMode(Channel::ChannelMode::KEY_PROTECTED, true);
        channel.setKey(cmd.params[index++]);
        onBuffer += 'k';
        break;
      }

      case 'o': {
        if (index >= cmd.params.size())
          break;

        OptionalUser user = channel.findUser(cmd.params[index++]);
        // FIXME: handle missing user some way?
        if (!user)
          break;

        user->get().toggleOperatorPrivilege();
        append('o');
        break;
      }

      case 'l': {
        if (!onOff) {
          offBuffer += 'l';
          channel.setUserLimit(UINT32_MAX);
          channel.setMode(Channel::ChannelMode::LIMITED_USER_COUNT, false);
          break;
        }

        if (index >= cmd.params.size())
          break;

        if (!std::ranges::all_of(cmd.params[index], ::isdigit))
          break;

        try {
          channel.setUserLimit(
              static_cast<uint32_t>(std::stoul(cmd.params[index])));
          channel.setMode(Channel::ChannelMode::LIMITED_USER_COUNT, true);
          onBuffer += 'l';
        } catch (...) {}
        index++;
        break;
      }

      default:
        return index;
    }
  }

  auto dedupe = [](std::string &s) {
    s.erase(std::unique(s.begin(), s.end()), s.end());
  };

  dedupe(onBuffer);
  dedupe(offBuffer);

  channel.setNewModes(onBuffer + offBuffer);
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
