#include "Utils.hpp"

#include <algorithm>
#include <vector>

bool Utils::validateNickname(const std::string &nick) {
  const std::string forbidden = " ,*?!@";
  const std::string forbiddenFirst = "$:";
  const std::string forbiddenChannel = "&#";
  const std::string forbiddenPrefix = "~&@%";
  if (nick.empty() || nick.length() > 9)
    return false;
  if (forbiddenFirst.find(nick[0]) != std::string::npos ||
      forbiddenChannel.find(nick[0]) != std::string::npos ||
      forbiddenPrefix.find(nick[0]) != std::string::npos)
    return false;
  if (nick.find_first_of(forbidden) != std::string::npos)
    return false;
  return true;
}

bool Utils::isHandshakeCmd(const std::string &cmd) {
  static const std::vector<std::string> handshakeCmds = {"CAP", "PASS", "NICK",
                                                         "USER"};
  return (std::find(handshakeCmds.begin(), handshakeCmds.end(), cmd) !=
          handshakeCmds.end());
}
