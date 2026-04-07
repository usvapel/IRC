#include <memory>

#include "Channel.hpp"
#include "Server.hpp"
#include "irc.hpp"

// INFO: Channel testing:
int main(void) {
  Server server(6667, 10, "test");

  Client pekka;
  Client esko;
  pekka.setNickname("pekka");
  esko.setNickname("esko");

  server.newChannel(pekka, "Test channel");
  auto channel = server.findChannel("Test channel");
  if (channel) {
    std::cout << channel->get().getName() << std::endl;
    std::cout << channel->get().getUserCount() << std::endl;
    channel->get().addUser(esko);
    channel->get().addUser(esko);
    std::cout << channel->get().getName() << std::endl;
    std::cout << channel->get().getUserCount() << std::endl;
    channel->get().tryKickUser("esko");
    channel->get().tryKickUser("esko");
    std::cout << channel->get().getName() << std::endl;
    std::cout << channel->get().getUserCount() << std::endl;
    channel->get().tryKickUser("pekka");
    std::cout << channel->get().getName() << std::endl;
    std::cout << channel->get().getUserCount() << std::endl;
  }

  return (0);
}
