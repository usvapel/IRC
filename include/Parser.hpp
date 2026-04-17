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
 * to be fed discrete, complete lines ending in '\r\n'
 */
namespace Parser {
/**
 * @brief Given a std::string_view of an extracted message ending in '\r\n',
 * returns a Command struct
 *
 * @param message, a string_view of a received message, including '\r\n'.
 * Underlying string should only be discarded after receiving the response
 * @return std::optional containing a populated Commands struct, if there was a
 * legal-looking IRC command (<:prefix> COMMAND <params>...), or std::nullopt if
 * no command was able to be found in the string
 */

void channelModeParse(const Command &cmd, Channel &channel, Server &server,
                      int32_t fd);
std::optional<Command> parse(std::string message);
}  // namespace Parser
