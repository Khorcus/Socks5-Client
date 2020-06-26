#ifndef SOCKS5_CLIENT_SOCKS_CLIENT_HPP
#define SOCKS5_CLIENT_SOCKS_CLIENT_HPP


#include <cstdint>
#include <string>
#include "../kqueue/KQueue.hpp"
#include "../socket/ClientSocket.hpp"

class SocksClient {
public:
    SocksClient();

    SocksClient(const char *socks_host, uint16_t socks_port, const char *server_host, uint16_t server_port);

    void init(const char *socks_host, uint16_t socks_port, const char *server_host, uint16_t server_port);

    void start_test(uint16_t session_count, const std::string &test_string, uint16_t time);

    unsigned long long get_ping_count();

    static void on_first_send(ClientSocket &client_socket);

private:
    const char *socks_host;
    uint16_t socks_port;
    const char *server_host;
    uint16_t server_port;
    unsigned long long ping_count;
    KQueue k_queue;
    std::vector<ClientSocket> socket_pool;
};


#endif //SOCKS5_CLIENT_SOCKS_CLIENT_HPP
