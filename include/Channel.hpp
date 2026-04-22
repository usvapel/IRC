#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Client.hpp"

class Server;

class Channel {
  public:
    class User;
    enum class ChannelMode : uint16_t;

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

    std::string &getNewModes();
    void         setNewModes(const std::string &modes);
    /**
     * @brief Returns a string containing the active modes and their
     * corresponding mode arguments.
     */
    /**
     * @brief Returns a string containing the active modes and their
     * corresponding mode arguments depending on nickname's User status on the
     * channel. If not a user, get the modes but not arguments.
     *
     * @param nickname Nickname of the mode request sender.
     */
    std::string getModes(const std::string &nickname) const;

    /**
     * @brief Returns a UNIX time stamp of the time when channel was created.
     */
    const std::string &getUNIXTimeCreated(void) const;

    /**
     * @brief Sets the channel key.
     *
     * @param key Key to be used on the channel.
     */
    void setKey(const std::string &key);

    /**
     * @brief Returns the user limit of channel.
     */
    uint32_t getUserLimit(void) const;

    // TODO: l - set / remove the user limit to channel;
    // 4.2.9 User Limit
    // A user limit may be set on channels by using the channel
    // flag 'l'. When the limit is reached, servers MUST forbid their local
    // users to join the channel. The value of the limit MUST only be made
    // available to the channel members in the reply sent by the server to a
    // MODE query.

    /**
     * @brief Sets the limit of simultaneous users on the channel.
     *
     * @param limit Number of maximum simultaneous users on the channel.
     */
    void setUserLimit(const uint32_t limit);

    /**
     * @brief Returns the current user count on the channel.
     */
    unsigned int getUserCount(void) const;

    // INFO: Utilities:

    /**
     * @brief Tries to add a ner User <client> to the channel. If
     * succesful, returns a reference to this User. If the User already exists,
     * throws a std::runtime_error exception.
     *
     * @param client Client to be added.
     * @param key Channel key.
     */
    std::optional<std::reference_wrapper<Channel::User>> tryAddUser(
        const Client &client, const std::string &key = "");

    /**
     * @brief Tries to find User with <nickname> from _users. Returns a
     * std::optional containing a reference of the User.
     *
     * @param nickname Nickname to look for in the _users
     */
    std::optional<std::reference_wrapper<User>> findUser(
        const std::string &nickname) const;

    // INFO: Operator commands:

    /**
     * @brief Attempts to remove the given User from the Server.
     *
     * @param nickname User to be removed from the Server
     */
    void removeUser(const std::string nickname);

    /**
     * @brief Sets a channel <mode> on or off base on the given <status>
     *
     * @param mode ChannelMode to be set
     * @param status Boolean true/false to set the mode to.
     */
    void setMode(const ChannelMode mode, const bool status);

    /**
     * @brief Resets all Channel flags.
     */
    void resetModes(void);

    /**
     * @brief Checks if a givent flag is on or off.
     *
     * @param flag Channel::ChannelFlag to be checked.
     * @return Returns true if given flag is on. Otherwise return false.
     */
    bool isModeOn(const ChannelMode flag) const;

    // TODO: i - toggle the invite - only channel                      flag;
    // 4.2.2 Invite Only Flag
    // When the channel flag 'i' is set, new members are only accepted if their
    // mask matches Invite-list (See section 4.3.2) or they have been invited by
    // a channel operator.  This flag also restricts the usage of the INVITE
    // command (See "IRC Client Protocol" [IRC-CLIENT]) to channel operators.
    // void toggleInviteOnly(void);

    // TODO: t - toggle the topic settable by channel operator only flag;
    // 4.2.8 Topic
    // The channel flag 't' is used to restrict the usage of the
    // TOPIC command to channel operators.
    // void toggleTopicSettableByChanopOnly(void);

    // TODO: k - set / remove the channel       key(password);
    // 4.2.10 Channel Key
    // When a channel key is set (by using the mode 'k'), servers MUST reject
    // their local users request to join the channel unless this key is given.
    // The channel key MUST only be made visible to the channel members in the
    // reply sent by the server to a MODE query.
    // void toggleChannelKey(void);

    // TODO: o - give / take channel   operator privilege;
    // 4.1.2 Channel Operator Status
    // The mode 'o' is used to toggle the operator status of a channel member.
    // void toggleChannelOperatorPrivilege(User &user);

    std::unordered_map<std::string, std::unique_ptr<Channel::User>> &
    getUsers() {
      return _users;
    }

    /**
     * @brief Tries to add <invited> to the invite list of the channel.
     *
     * @param sender Nickname of the invitation sender.
     * @param invited Nickname of the client to be invited.
     * @return Returns true if succesful. Otherwise false.
     */
    bool tryAddInvite(const User &sender, const std::string &invited);

    /**
     * @brief Send a <message> to all Users on channel.
     *
     * @param message Message to be sent to all users.
     */
    void messageAllUsersOnChannel(const std::string &message);

    /**
     * @brief Send a <message> to all Users on channel.
     *
     * @param message Message to be sent to all users.
     * @param sender Sender of <message> to be ignored.
     */
    void messageAllExceptSenderOnChannel(const std::string &message,
                                         const std::string &sender);

    /**
     * @brief Send a <message> with <code> to all users on channel.
     *
     * @param message Message to be sent to all users.
     * @param code Code to be sent with the <message> to all users.
     */
    void messageAllUsersOnChannel(const std::string &message,
                                  const int32_t      code);

    /**
     * @brief Send a <message> to all users except the <sender>.
     *
     * @param sender Nickname of the <sender> of <message>.
     * @param message Message to be sent to all users on channel.
     */
    void messageAllUsersOnChannel(const std::string &sender,
                                  const std::string &message);
    /**
     * @brief Send a <message> with <code> to all users except the <sender>.
     *
     * @param sender Nickname of the <sender> of <message>.
     * @param message Message to be sent to all users on channel.
     * @param code Code to be sent with the <message> to all users.
     */
    void messageAllUsersOnChannel(const std::string &sender,
                                  const std::string &message,
                                  const int32_t      code);

    /**
     * @brief Sends a standardized message to all Users on a channel that
     * clientToAdd is joining as a new user.
     *
     * @param clientToAdd Client being added as a new user.
     */
    void messageNewUserJoining(Client &clientToAdd);

    /**
     * @brief Checks if the given key matches channel's key.
     *
     * @param key Key to check against channel's key.
     * @return Boolean value based on if keys match.
     */
    bool keyIsCorrect(const std::string &key) const;

    /**
     * @brief Changes user's nick on channel to match the new nick on <client>.
     *
     * @param oldNick Old nickname of the client.
     * @param client The client who's nickname is to be changed.
     */
    void changeUserNick(const std::string &oldNick, const Client &client);

  private:
    Server                  &_server;
    std::string              _name = "";
    std::string              _key = "";
    std::string              _topic = "";
    std::string              _invitationMask = "";
    std::string              _timeCreated = "";
    uint32_t                 _userLimit = UINT32_MAX;
    uint16_t                 _channelModes = 0;
    std::string              _newModes = "";
    std::vector<std::string> _inviteList{};

    std::unordered_map<std::string, std::unique_ptr<Channel::User>> _users;

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
     *  @brief Creates and returns a string of users on a channel with a prefix
     * of their highest membership prefix.
     */
    std::string userList(void) const;

    /**
     * @brief
     *  @brief Creates and returns a string of users on a channel (excluding
     * userToSkip) with a prefix of their highest membership prefix.
     *
     * @param userToSkip User not to be included in the list.
     */
    std::string userList(const User &userToSkip) const;

    /**
     * @brief Toggles a mode on the channel.
     *
     * @param mode Channel::ChannelMode to be toggled.
     */
    void toggleMode(const ChannelMode mode);

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
         * @brief Sets user's operator privilege to <status>.
         *
         * @param status Boolean to set user's operator privilege to.
         */
        void setOperatorPrivilege(const bool status);

        /**
         * @brief Returns a boolean telling if the User is operator on the
         * channel.
         *
         * @return True if user is operator. Otherwise false.
         */
        bool isOperator(void) const;

      private:
        const Client *_client = nullptr;
        bool          _isOperator = false;
    };

    enum class ChannelMode : uint16_t {
      INVITE_ONLY = 1,
      TOPIC_SET_BY_CHANOP_ONLY = 1 << 1,
      KEY_PROTECTED = 1 << 2,
      LIMITED_USER_COUNT = 1 << 3,
    };
};

using OptionalUser = std::optional<std::reference_wrapper<Channel::User>>;
