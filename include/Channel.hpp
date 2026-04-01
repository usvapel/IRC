#pragma once
#include <cstdint>
#include <vector>

// a - toggle the anonymous channel flag;
// i - toggle the invite - only channel                      flag;
// m - toggle the moderated                                  channel;
// n - toggle the no messages to channel from clients on the outside;
// q - toggle the quiet channel                              flag;
// p - toggle the private channel                            flag;
// s - toggle the secret channel                             flag;
// r - toggle the server reop channel                        flag;
// t - toggle the topic settable by channel operator only flag;
enum class ChannelFlag : uint16_t {
  NONE = 0,
  ANONYMOUS = 1,
  INVITE_ONLY = 1 << 1,
  MODERATED = 1 << 2,
  NO_MESSAGES_FROM_OUTSIDE = 1 << 3,
  QUIT_CHANNEL = 1 << 4,
  PRIVATE_CHANNEL = 1 << 5,
  SECRET_CHANNEL = 1 << 6,
  SERVER_REOP_CHANNEL = 1 << 7,
  TOPIC_SET_MY_CHANOP_ONLY = 1 << 8

};

class Channel {
  private:
    uint16_t _channelFlags = 0;

  public:
    Channel();
    Channel(const Channel &other) = delete;
    Channel &operator=(const Channel &other) = delete;
    ~Channel();

    // O - give "channel creator" status;
    void giveChannelCreatorStatus(void);

    // o - give / take channel   operator privilege;
    void toggleChannelOperatorPrivilege(void);

    // v - give / take the voice privilege;
    void toggleVoicePrivilege(void);

    // k - set / remove the channel       key(password);
    void toggleChannelKey(void);

    // l - set / remove the user limit to channel;
    void toggleUserLimit(void);

    // b - set / remove ban mask to keep users                           out;
    void toggleBanMask(void);

    // e - set / remove an exception mask to override a ban              mask;
    void toggleExceptionMaskToOverrideBanMask(void);

    // I - set / remove an invitation mask to automatically override the
    // invite-only flag; };
    void toggleInvitationMaskToOverrideInviteOnlyFlag(void);
};
