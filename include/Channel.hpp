#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "Client.hpp"
#include "Server.hpp"

class Server;

class Channel {
  public:
    class User;
    enum class ChannelFlag : uint16_t;

    /**
     * @brief Constructs a new channel with name <name> and sets <client> as a
     * User with operator rights
     *
     * @param client Client that creates the channel
     * @param name Name of the channel
     */
    Channel(Server &server, const Client &client, const std::string &name);
    ~Channel();

    Channel() = delete;
    Channel(const Channel &other) = delete;
    Channel &operator=(const Channel &other) = delete;

    // INFO: Getters and setters:
    const std::string &getName(void) const;
    void               setName(const std::string &name);

    void               setTopic(const std::string &topic);
    const std::string &getTopic(void) const;

    unsigned int getUserLimit(void) const;
    // TODO: l - set / remove the user limit to channel;
    // 4.2.9 User Limit
    // A user limit may be set on channels by using the channel
    // flag 'l'. When the limit is reached, servers MUST forbid their local
    // users to join the channel. The value of the limit MUST only be made
    // available to the channel members in the reply sent by the server to a
    // MODE query.
    void setUserLimit(const unsigned int limit);

    unsigned int getUserCount(void) const;

    // INFO: Utilities:
    /**
     * @brief Tries to add a ner User <client> to the channel _users vector. If
     * succesful, returns a reference to this User. If the User already exists,
     * throws a std::runtime_error exception.
     *
     * @param client Client based on which to create a new user.
     * @return Returns a reference to the newly created User.
     */
    std::optional<std::reference_wrapper<Channel::User>> addUser(
        const Client &client);

    /**
     * @brief Tries to find User with <nickname> from _users. Returns a
     * std::optional containing a reference of the User.
     *
     * @param nickname Nickname to look for in the _users
     */
    std::optional<std::reference_wrapper<User>> findUser(
        const std::string &nickname);
    // User &findUser(const std::string &nickname);

    // INFO: Operator commands:
    /**
     * @brief Attempts to kick (remove) the given User from the Server.
     *
     * @param user User to be kicked (removed) from the Server
     */
    // FIXME: In definition. What else needs to be done when kicking?
    void kickUser(User &target);

    /**
     * @brief Attempts to kick (remove) the given User from the Server.
     *
     * @param nickname User to be kicked (removed) from the Server
     */
    void tryKickUser(const std::string nickname);

    // TODO:4.3.2 Channel Invitation
    // For channels which have the invite-only flag set (See Section 4.2.2
    // (Invite Only Flag)), users whose address matches an invitation mask set
    // for the channel are allowed to join the channel without any invitation.

    void inviteUser(const std::string &nickname);

    // void setMode(Mode mode);

    /**
     * @brief Toggles a server.
     *
     * @param flag Channel::ChannelFlag to be toggled.
     */
    void toggleFlag(const ChannelFlag flag);

    /**
     * @brief Resets all Channel flags.
     */
    void resetFlags(void);

    /**
     * @brief Checks if a givent flag is on or off.
     *
     * @param flag Channel::ChannelFlag to be checked.
     * @return Returns true if given flag is on. Otherwise return false.
     */
    bool isFlagOn(const ChannelFlag flag);

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

    std::unordered_map<std::string, std::unique_ptr<Channel::User>> &
    getUsers() {
      return _users;
    }

  private:
    Server     &_server;
    std::string _name = "";
    std::string _key = "";
    std::string _topic = "";
    std::string _invitationMask = "";
    uint32_t    _userLimit = UINT32_MAX;
    uint16_t    _channelFlags = 0;

    std::unordered_map<std::string, std::unique_ptr<Channel::User>> _users;

  public:
    class User {
      public:
        User() = delete;
        User(const Client &client);
        User(const User &other);
        User &operator&(const User &other);
        ~User() = default;

        // INFO: Getters and setters:
        const Client      *getClient(void) const;
        const std::string &getNickName(void) const;

        /**
         * @brief Toggles User's operator privilege on/off.
         */
        void toggleOperatorPrivilege(void);

        /**
         * @brief Adds operator privilege to user.
         */
        void addOperatorPrivilege(void);

        /**
         * @brief Removes user's operator privilege.
         */
        void removeOperatorPrivilege(void);

      private:
        const Client *_client = nullptr;
        bool          _isOperator = false;
    };

    enum class ChannelFlag : uint16_t {
      INVITE_ONLY = 1,
      TOPIC_SET_BY_CHANOP_ONLY = 1 << 1,
      KEY_PROTECTED = 1 << 2,
      LIMITED_USER_COUNT = 1 << 3,
    };
};

using OptionalUser = std::optional<std::reference_wrapper<Channel::User>>;

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
