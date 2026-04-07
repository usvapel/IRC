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

  std::cout << "Pekka makes a server 'Test channel'\n\n";
  server.newChannel(pekka, "Test channel");
  auto channel = server.findChannel("Test channel");
  if (channel) {
    std::cout << "Channel name: " << channel->get().getName() << std::endl;
    std::cout << "Channel user count: " << channel->get().getUserCount()
              << std::endl;
    std::cout << "\n";
    auto user = channel->get().findUser("pekka");
    if (user) {
      std::cout << user->get().getNickName() << " found on the server\n";
    }
    std::cout << "\n";
    channel->get().addUser(esko);
    channel->get().addUser(esko);
    std::cout << "\n";
    std::cout << "Channel name: " << channel->get().getName() << std::endl;
    std::cout << "Channel user count: " << channel->get().getUserCount()
              << std::endl;
    std::cout << "\n";
    channel->get().tryKickUser("esko");
    channel->get().tryKickUser("esko");
    std::cout << "\n";
    std::cout << "Channel name: " << channel->get().getName() << std::endl;
    std::cout << "Channel user count: " << channel->get().getUserCount()
              << std::endl;
    std::cout << "\n";
    channel->get().tryKickUser("pekka");
    channel->get().tryKickUser("pekka");
    std::cout << "\n";
    std::cout << "Channel name: " << channel->get().getName() << std::endl;
    std::cout << "Channel user count: " << channel->get().getUserCount()
              << std::endl;
    std::cout << "\n";
    auto user2 = channel->get().findUser("antti");
    if (user2) {
      std::cout << user2->get().getNickName() << " found on the server\n";
    }
  }
  std::cout << "\n";
  auto channel2 = server.findChannel("Non existing");
  if (channel2) {
    std::cout << "Non existing found\n";
  }
  std::cout << "Non existing channel not found\n";

  return (0);
}
