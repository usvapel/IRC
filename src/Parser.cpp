#include "Parser.hpp"

#include <optional>

std::optional<Command> Parser::parse(std::string_view message) {
  Command cmd;

  if (message.empty())
    return std::nullopt;

  auto endpoint = message.find_first_of(' ');
  if (message[0] == ':') {
    if (endpoint == std::string::npos)
      return std::nullopt;
    cmd.prefix = std::string(message.substr(1, endpoint - 1));
    message.remove_prefix(endpoint + 1);
  }

  endpoint = message.find_first_of(' ');
  cmd.command = std::string(message.substr(0, endpoint));
  if (endpoint != std::string::npos) {
    message.remove_prefix(endpoint + 1);
  } else {
    return cmd;
  }

  while (!message.empty()) {
    if (message[0] == ' ') {
      message.remove_prefix(1);
      continue;
    }
    endpoint = message.find_first_of(' ');
    if (message[0] == ':') {
      cmd.params.push_back(std::string(message.substr(1)));
      break;
    }
    cmd.params.push_back(std::string(message.substr(0, endpoint)));
    if (endpoint != std::string::npos) {
      message.remove_prefix(endpoint + 1);
    } else {
      break;
    }
  }
  return cmd;
}
