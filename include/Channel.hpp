#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Client.hpp"

class Channel {
  public:
    class User;

  private:
    std::string  _name = "";
    std::string  _key = "";
    std::string  _topic = "";
    std::string  _invitationMask = "";
    unsigned int _userLimit = 0;
    uint16_t     _channelFlags = 0;

    std::vector<std::unique_ptr<Channel::User>> _users;

  public:
    Channel(const Client &client, const std::string &name);
    ~Channel();

    Channel() = delete;
    Channel(const Channel &other) = delete;
    Channel &operator=(const Channel &other) = delete;

    const std::string &getName(void) const;
    unsigned int       getUserCount(void) const;

    class User {
      private:
        const Client *_client = nullptr;
        bool          _isOperator = false;

      public:
        User() = delete;
        User(const Client &client);
        User(const User &other);
        User &operator&(const User &other);
        ~User();

        const Client *getClient(void) const;
        void          toggleOperatorPrivilege(void);
        void          addOperatorPrivilege(void);
        void          removeOperatorPrivilege(void);
    };

    enum class ChannelFlag : uint16_t {
      INVITE_ONLY = 1,
      TOPIC_SET_BY_CHANOP_ONLY = 1 << 1,
      KEY_PROTECTED = 1 << 2,
      LIMITED_USER_COUNT = 1 << 3,
    };

    // NOTE: Operator commands:
    User &addUser(const Client &client);

    void kickUser(User &user);

    // TODO:4.3.2 Channel Invitation
    // For channels which have the invite-only flag set (See Section 4.2.2
    // (Invite Only Flag)), users whose address matches an invitation mask set
    // for the channel are allowed to join the channel without any invitation.

    void inviteUser(const std::string &user);

    void changeTopic(const std::string &topic);

    void viewTopic(void);

    // void setMode(Mode mode);

    void toggleFlag(const ChannelFlag flag);

    void resetFlags(void);

    bool isFlagOn(const ChannelFlag flag);

    // FIXME: Mandatory: Use separate function for all (wrapper for
    // toggleChannelFlag()) or just use the toggleChannelFlag()?

    // TODO: i - toggle the invite - only channel                      flag;
    // 4.2.2 Invite Only Flag
    // When the channel flag 'i' is set, new members are only accepted if their
    // mask matches Invite-list (See section 4.3.2) or they have been invited by
    // a channel operator.  This flag also restricts the usage of the INVITE
    // command (See "IRC Client Protocol" [IRC-CLIENT]) to channel operators.

    void toggleInviteOnly(void);

    // TODO: t - toggle the topic settable by channel operator only flag;
    // 4.2.8 Topic
    // The channel flag 't' is used to restrict the usage of the
    // TOPIC command to channel operators.

    void toggleTopicSettableByChanopOnly(void);

    // TODO: k - set / remove the channel       key(password);
    // 4.2.10 Channel Key
    // When a channel key is set (by using the mode 'k'), servers MUST reject
    // their local users request to join the channel unless this key is given.
    // The channel key MUST only be made visible to the channel members in the
    // reply sent by the server to a MODE query.

    void toggleChannelKey(const std::string &key);

    // TODO: o - give / take channel   operator privilege;
    // 4.1.2 Channel Operator Status
    // The mode 'o' is used to toggle the operator status of a channel member.

    void toggleChannelOperatorPrivilege(User &user);

    // TODO: l - set / remove the user limit to channel;
    // 4.2.9 User Limit
    // A user limit may be set on channels by using the channel
    // flag 'l'. When the limit is reached, servers MUST forbid their local
    // users to join the channel. The value of the limit MUST only be made
    // available to the channel members in the reply sent by the server to a
    // MODE query.

    void setUserLimit(const unsigned int limit);
};

// WARN: Not in subject:
// enum class ChannelFlag{
//  ANONYMOUS = 1 << 4,
//  MODERATED = 1 << 5,
//  NO_MESSAGES_FROM_OUTSIDE = 1 << 6,
//  QUIET_CHANNEL = 1 << 7,
//  PRIVATE_CHANNEL = 1 << 8,
//  SECRET_CHANNEL = 1 << 9,
//  SERVER_REOP_CHANNEL = 1 << 10,
// };

// WARN: Nickname masks not in subject
// std::string _banMask = "";
// std::string _banExceptionMask = "";

/* WARN: Not defined in subject
 * a - toggle the anonymous channel flag;
 * m - toggle the moderated channel;
 * n - toggle the no messages to channel from clients on the outside;
 * q - toggle the quiet channel flag;
 * p - toggle the private channel flag;
 * s - toggle the secret channel flag;
 * r - toggle the server reop channel flag;
 * O - give "channel creator" status;
 * v - give / take the voice privilege;
 * b - set / remove ban mask to keep users out;
 * e - set / remove an exception mask to override a ban mask;
 * I - set / remove an invitation mask to automatically override the
 * invite-only flag; };
 */
