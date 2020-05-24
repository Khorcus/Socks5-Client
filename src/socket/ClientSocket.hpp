#ifndef SOCKS5_CLIENT_CLIENT_SOCKET_HPP
#define SOCKS5_CLIENT_CLIENT_SOCKET_HPP


#include <cstdint>
#include <string>


class ClientSocket {
public:
    ClientSocket();
    ~ClientSocket();
    bool connect(const char *host, uint16_t port);
    bool make_non_blocking();
    int send(const void *data, size_t size);
    int receive(void *data, size_t size);
    void receive_all();
    int get_fd();

private:
    int sfd;
};


#endif //SOCKS5_CLIENT_CLIENT_SOCKET_HPP
