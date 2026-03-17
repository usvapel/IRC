#include <asm-generic/socket.h>
#include <err.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <vector>

/*
 * htonl, htons, ntohl, ntohs
 * - convert values between host and network byte order
 */
#include <arpa/inet.h>

/*
 * getprotobyname, getprotobynumber, getprotent — network protocol
 * database functions
 */
#include <netdb.h>

#define MY_SOCK_PATH "/tmp/testsocket"
#define LISTEN_BACKLOG 50
