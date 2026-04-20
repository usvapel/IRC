#include "Parser.hpp"

#include <algorithm>
#include <cstdint>

#include "Channel.hpp"
#include "Server.hpp"
#include "Utils.hpp"

void Parser::channelModeParse(const Command &cmd, Channel &channel,
                              Server &server, int32_t fd) {
  bool               onOff = false;
  size_t             index = 2;
  std::string        onBuffer;
  std::string        offBuffer;
  std::string        argBuffer(" ");
  const std::string &modestring = cmd.params[1];
  Client            &client = server._clients.at(fd);
  const std::string &authorNick = client.getNickname();

  auto append = [&](char c) { (onOff ? onBuffer : offBuffer) += c; };

  for (char c : modestring) {
    switch (c) {
      case '+':
        onOff = true;
        onBuffer += '+';
        continue;

      case '-':
        onOff = false;
        offBuffer += '-';
        continue;

      case 'i':
        append('i');
        channel.setMode(Channel::ChannelMode::INVITE_ONLY, onOff);
        continue;

      case 't':
        append('t');
        channel.setMode(Channel::ChannelMode::TOPIC_SET_BY_CHANOP_ONLY, onOff);
        continue;

      case 'k': {
        if (!onOff) {
          if (channel.isModeOn(Channel::ChannelMode::KEY_PROTECTED))
            offBuffer += 'k';
          channel.setMode(Channel::ChannelMode::KEY_PROTECTED, false);
          channel.setKey("");
          continue;
        }

        if (index >= cmd.params.size())
          continue;

        channel.setMode(Channel::ChannelMode::KEY_PROTECTED, true);
        channel.setKey(cmd.params[index++]);
        onBuffer += 'k';
        continue;
      }

      case 'o': {
        if (index >= cmd.params.size())
          continue;

        OptionalUser user = channel.findUser(cmd.params[index]);
        if (!user) {
          server.sendMessageWithCodeToUser(authorNick, authorNick,
                                           Numeric::ERR_NOSUCHNICK,
                                           ":" + cmd.params[index]);
          continue;
        }

        user->get().toggleOperatorPrivilege();
        argBuffer.append(cmd.params[index++] + " ");
        append('o');
        continue;
      }

      case 'l': {
        if (!onOff) {
          offBuffer += 'l';
          channel.setUserLimit(UINT32_MAX);
          channel.setMode(Channel::ChannelMode::LIMITED_USER_COUNT, false);
          continue;
        }

        if (index >= cmd.params.size())
          continue;

        if (!std::ranges::all_of(cmd.params[index], ::isdigit))
          continue;

        try {
          channel.setUserLimit(
              static_cast<uint32_t>(std::stoul(cmd.params[index])));
          channel.setMode(Channel::ChannelMode::LIMITED_USER_COUNT, true);
          onBuffer += 'l';
          argBuffer.append(cmd.params[index] + " ");
        } catch (...) {}
        index++;
        continue;
      }

      default:
        std::string unknownMode(1, c);
        server.sendMessageWithCodeToUser(authorNick, authorNick,
                                         Numeric::ERR_UNKNOWNMODE, unknownMode);
        break;
    }
    break;
  }

  auto dedupe = [](std::string &s) {
    s.erase(std::unique(s.begin(), s.end()), s.end());
    if (s.size() == 1)
      s.clear();
  };

  dedupe(onBuffer);
  dedupe(offBuffer);

  channel.setNewModes(onBuffer + offBuffer + argBuffer);
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
