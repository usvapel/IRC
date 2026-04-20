#pragma once

#include <stddef.h>
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
    Socket(const Socket &other) = delete;
    Socket &operator=(const Socket &other) = delete;
    Socket(Socket &&other) noexcept;
    Socket &operator=(Socket &&other) noexcept;
    /**
     * @brief creates and returns a socket to listen on the specified port
     *
     * @param int32_t port to listen on
     * @return Socket object
     */
    static Socket makeListeningSocket(int32_t port);
    /**
     * @brief Wrap a give file descriptor in a Socket object
     *
     * @param int32_t clientFD fd received from accept()
     * @return Socket object
     */
    static Socket makeClientSocket(int32_t clientFD);
    /**
     * @brief make the give fd nonblocking with fcntl
     *
     * @param int32_t fd
     * @return void
     */
    void makeNonBlocking();
    /**
     * @brief return the contained fd
     *
     * @param
     * @return file descriptor
     */
    int32_t getFD() const;

    /**
     * @brief Wrapper for the recv(_fd, buffer, length, 0)
     *
     * @param[out] buffer Pointer to memory to write into
     * @param[in] length Length of buffer
     * @return return value of
     */
    ssize_t receiveData(char *buffer, size_t length);

    /**
     * @brief  Wrapper for the send(_fd, buffer, length, 0)
     *
     * @param[in] buffer Pointer to message to be sent
     * @param[in] length Length of buffer
     * @return void
     */
    ssize_t sendData(const char *buffer, size_t length);

  private:
    Socket(int32_t fd);
    int32_t _fd;
};
