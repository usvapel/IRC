#pragma once
#include <cstdint>
#include <optional>
#include <string>

#include "Command.hpp"

class Server;
class Channel;

/**
 * @namespace Parser
 * @brief Stateless  module for parsing raw IRC messages.
 * * @note This parser does not maintain state or buffers. It expects
 * to be fed discrete, complete lines, with the expected '\r\n' stripped
 */
namespace Parser {

void channelModeParse(const Command &cmd, Channel &channel, Server &server,
                      int32_t fd);

/**
 * @brief Given a std::string of an extracted message stripped of the ending
 * '\r\n', parses a command struct to be used by message handlers
 *
 * @param std::string message, a received message.
 * @return Command struct or nullopt
 */
std::optional<Command> parse(std::string message);
}  // namespace Parser
