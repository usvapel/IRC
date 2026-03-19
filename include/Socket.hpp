#ifndef SOCKET_HPP
#define SOCKET_HPP
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdint>
/**
 * @class Socket
 * @brief A wrapper around C POSIX API sockets
 *
 * There are two ways to create a Socket:
 * - makeListeningSocket(int port) creates and returns a socket to listen on the
 * specified port: it calls socket() which assigns a fd, makes that fd
 * nonblocking, calls setsockopt(SO_REUSEADDR), bind(fd) to the specified port,
 * and calls listen(fd)
 * - makeClientSocket(int clientFD) creates and returns a Socket object from the
 * fd returned by accept(), and makes it non-blocking
 */
class Socket {
  public:
    ~Socket();
    /**
     * @brief creates and returns a socket to listen on the specified port
     *
     * @param int32_t port to listen on
     * @return Socket object
     */
    static Socket *makeListeningSocket(int32_t port);
    /**
     * @brief Wrap a give file descriptor in a Socket object
     *
     * @param int32_t clientFD fd received from accept()
     * @return Socket object
     */
    static Socket *makeClientSocket(int32_t clientFD);
    /**
     * @brief make the give fd nonblocking with fcntl
     *
     * @param int32_t fd
     * @return void
     */
    void makeNonBlocking(int32_t fd);
    /**
     * @brief return the contained fd
     *
     * @param
     * @return file descriptor
     */
    int32_t getFD() const;

  private:
    Socket(int32_t fd);
    int32_t _fd;
};

#endif  // SOCKET_HPP
