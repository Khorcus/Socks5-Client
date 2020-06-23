#ifndef SOCKS5_CLIENT_CLIENT_SOCKET_HPP
#define SOCKS5_CLIENT_CLIENT_SOCKET_HPP


#include <cstdint>
#include <string>
#include "../utils.hpp"


class ClientSocket {
public:
    ClientSocket();

    ClientSocket(const ClientSocket &) = delete;

    ClientSocket(ClientSocket &&src) noexcept;

    ClientSocket &operator=(const ClientSocket &) = delete;

    ClientSocket &operator=(ClientSocket &&src) noexcept;

    ~ClientSocket();

    bool connect(const char *host, uint16_t port);

    bool make_non_blocking();

    int send(const void *data, size_t size);

    int receive(void *data, size_t size);

    void discard_all();

    int get_fd() const;

    status get_s() const;

    void set_s(status s);

    void swap(ClientSocket &other);

private:
    int sfd;
    status s;
};


#endif //SOCKS5_CLIENT_CLIENT_SOCKET_HPP
