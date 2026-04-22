#include "Utils.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
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

std::string Utils::getTimestamp() {
  std::time_t now_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream ss;
  ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}
