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
    k_queue.add_timer_event(time);
    for (int i = 0; i < session_count; i++) {
        socket_pool[i].set_k_queue(k_queue);
        //TODO: Understand why in the absence of const in the start_test definition, the program does not compile
        socket_pool[i].set_test_data(server_host, server_port, const_cast<std::string &>(test_string));
        if (!socket_pool[i].connect(socks_host, socks_port)) {
            return;
        }
        if (!socket_pool[i].make_non_blocking()) {
            return;
        }
        socket_pool[i].send_all("\05\01\00", 3, on_first_send);
    }

    SocksActions actions = SocksActions();
    k_queue.start_loop(&actions);
    ping_count = actions.get_ping_count();
}

unsigned long long SocksClient::get_ping_count() {
    return ping_count;
}

void SocksClient::on_first_send(ClientSocket &client_socket) {
    client_socket.set_s(CONNECTION);
    client_socket.get_k_queue().add_read_event(client_socket.get_fd(), &client_socket);
}
