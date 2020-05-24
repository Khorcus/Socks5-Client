#include <vector>
#include <iostream>
#include "SocksClient.hpp"

#include "../utils.hpp"
#include "SocksActions.hpp"

#define EV_NUMBER 32

SocksClient::SocksClient(const char *socks_host, uint16_t socks_port, const char *server_host, uint16_t server_port) :
        socks_host(socks_host),
        socks_port(socks_port),
        server_host(server_host),
        server_port(server_port),
        ping_count(),
        k_queue(EV_NUMBER),
        socket_pool() {}

SocksClient::~SocksClient() {
    k_queue.~KQueue();
    delete[] socket_pool;
}

void SocksClient::start_test(uint16_t session_count, const std::string &test_string, uint16_t time) {
    socket_pool = new ClientSocket *[session_count];
    if (!k_queue.init()) {
        return;
    }
    for (int i = 0; i < session_count; i++) {
        auto *s = new ClientSocket();
        socket_pool[i] = s;
        if (!s->connect(socks_host, socks_port)) {
            return;
        }
        if (!s->make_non_blocking()) {
            return;
        }
        if (!s->send("\05\01\00", 3)) {
            return;
        }
        auto *data = new struct udata;
        data->s = CONNECTION;
        k_queue.add_read_event(s->get_fd(), data);
    }

    k_queue.add_timer_event(time);

    SocksActions actions = SocksActions(server_host, server_port, test_string, k_queue);
    k_queue.start_loop(&actions);
    std::cout << actions.get_ping_count() << std::endl;
}

unsigned long long SocksClient::get_ping_count() {
    return ping_count;
}

