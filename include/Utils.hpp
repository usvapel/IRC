#pragma once
#include <cstdint>
#include <string>

namespace Utils {
bool validateNickname(const std::string &nick);
bool isHandshakeCmd(const std::string &cmd);
}  // namespace Utils

namespace Numeric {
// handshake
constexpr int32_t RPL_WELCOME = 1;
constexpr int32_t RPL_YOURHOST = 2;
constexpr int32_t RPL_CREATED = 3;
constexpr int32_t RPL_MYINFO = 4;
// Replies
constexpr int32_t RPL_UMODEIS = 221;
constexpr int32_t RPL_NOTOPIC = 331;
constexpr int32_t RPL_TOPIC = 332;
constexpr int32_t RPL_TOPICWHOTIME = 333;
constexpr int32_t RPL_INVITING = 341;
constexpr int32_t RPL_NAMERPLY = 353;
constexpr int32_t RPL_ENDOFNAMES = 366;
// errors
constexpr int32_t ERR_NOSUCHNICK = 401;
constexpr int32_t ERR_NOSUCHCHANNEL = 403;
constexpr int32_t ERR_CANNOTSENDTOCHAN = 404;
constexpr int32_t ERR_INPUTTOOLONG = 417;
constexpr int32_t ERR_UNKNOWNCOMMAND = 421;
constexpr int32_t ERR_NONICKNAMEGIVEN = 431;
constexpr int32_t ERR_ERRONEUSNICKNAME = 432;
constexpr int32_t ERR_NICKNAMEINUSE = 433;
constexpr int32_t ERR_USERNOTINCHANNEL = 441;
constexpr int32_t ERR_NOTONCHANNEL = 442;
constexpr int32_t ERR_USERONCHANNEL = 443;
constexpr int32_t ERR_NOTREGISTERED = 451;
constexpr int32_t ERR_NEEDMOREPARAMS = 461;
constexpr int32_t ERR_ALREADYREGISTRED = 462;
constexpr int32_t ERR_PASSWDMISMATCH = 464;
constexpr int32_t ERR_CHANNELISFULL = 471;
constexpr int32_t ERR_BADCHANNELKEY = 475;
constexpr int32_t ERR_BADCHANMASK = 476;
constexpr int32_t ERR_CHANOPRIVSNEEDED = 482;
}  // namespace Numeric
