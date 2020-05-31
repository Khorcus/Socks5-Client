#include <vector>
#include <iostream>
#include "SocksClient.hpp"

#include "../utils.hpp"
#include "SocksActions.hpp"

#define EV_NUMBER 32

SocksClient::SocksClient() :
        socks_host(),
        socks_port(),
        server_host(),
        server_port(),
        ping_count(),
        k_queue(EV_NUMBER),
        socket_pool() {}

SocksClient::SocksClient(const char *socks_host, uint16_t socks_port, const char *server_host, uint16_t server_port) :
        socks_host(socks_host),
        socks_port(socks_port),
        server_host(server_host),
        server_port(server_port),
        ping_count(),
        k_queue(EV_NUMBER),
        socket_pool() {}

void SocksClient::init(const char *socks_host, uint16_t socks_port, const char *server_host, uint16_t server_port) {
    this->socks_host = socks_host;
    this->socks_port = socks_port;
    this->server_host = server_host;
    this->server_port = server_port;
}

void SocksClient::start_test(uint16_t session_count, const std::string &test_string, uint16_t time) {
    socket_pool = std::vector<ClientSocket>(session_count);
    if (!k_queue.init()) {
        return;
    }
    for (int i = 0; i < session_count; i++) {
        if (!socket_pool[i].connect(socks_host, socks_port)) {
            return;
        }
        if (!socket_pool[i].make_non_blocking()) {
            return;
        }
        if (!socket_pool[i].send("\05\01\00", 3)) {
            return;
        }
        socket_pool[i].set_s(CONNECTION);
        k_queue.add_read_event(socket_pool[i].get_fd(), &socket_pool[i]);
    }

    k_queue.add_timer_event(time);

    SocksActions actions = SocksActions(server_host, server_port, test_string, k_queue);
    k_queue.start_loop(&actions);
    std::cout << actions.get_ping_count() << std::endl;
}

unsigned long long SocksClient::get_ping_count() {
    return ping_count;
}
