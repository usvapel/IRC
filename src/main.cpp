#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>

#include "irc.hpp"
#define MAXLINE 1024
#define SERVER_PORT 6667

int main() {
	int listenfd;
	int connfd;
	int n;

	struct sockaddr_in serveraddr;
	uint8_t			   buff[MAXLINE];
	uint8_t			   recvline[MAXLINE];

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		exit(1);
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);

	if ((bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) <
		0)
		exit(1);
	if ((listen(listenfd, 10)) < 0)
		exit(1);

	while (true) {
		struct sockaddr_in addr;
		socklen_t		   addr_len;
		std::cout << "waiting for a connection on port " << SERVER_PORT
				  << std::endl;
		connfd = accept(listenfd, (sockaddr *)NULL, NULL);
		memset(recvline, 0, MAXLINE);
		while ((n = read(connfd, recvline, MAXLINE - 1)) > 0) {
			std::cout << recvline << std::cout;
		}
	}

	return 0;
}
