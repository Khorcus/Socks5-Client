#include "SocksActions.hpp"
#include "../socket/ClientSocket.hpp"
#include <netinet/in.h>
#include <iostream>
#include <utility>
#include <arpa/inet.h>

#define BUF_SIZE 1024

SocksActions::SocksActions(
        const char *server_host,
        uint16_t server_port,
        std::string test_string,
        KQueue k_queue) :
        server_host(server_host),
        server_port(server_port),
        test_string(std::move(test_string)),
        ping_count(0),
        k_queue(std::move(k_queue)) {}

SocksActions::~SocksActions() = default;


void SocksActions::on_read_event(int fd, void *udata) {
    auto *data = reinterpret_cast<ClientSocket *>(udata);
    switch (data->get_s()) {
        case CONNECTION: {
            uint8_t connection_answer[2];
            uint8_t command_request[10] = {0x05, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            struct in_addr server_addr{};

            if (data->receive(connection_answer, sizeof(connection_answer)) == -1) {
                std::cerr << "Failed to read connection response: " << std::strerror(errno) << std::endl;
                break;
            }

            if (connection_answer[0] != 0x05) {
                std::cerr << "Server doesn't support VER=5" << std::endl;
                break;
            }

            if (connection_answer[1] != 0x00) {
                std::cerr << "Server doesn't support no auth METHOD" << std::endl;
                break;
            }

            if (inet_aton(server_host, &server_addr) == 0) {
                std::cerr << "Invalid server host" << std::endl;
                exit(EXIT_FAILURE);
            }
            memcpy(command_request + 4, &server_addr.s_addr, 4);
            memcpy(command_request + 8, &server_port, 2);

            if ((send(fd, command_request, sizeof(command_request), 0)) == -1) {
                std::cerr << "Failed to send command request: " << std::strerror(errno) << std::endl;
                break;
            }
            data->set_s(COMMAND);
            k_queue.add_read_event(fd, data);
            break;
        }
        case COMMAND: {
            uint8_t command_answer[10];

            if ((recv(fd, command_answer, sizeof(command_answer), 0)) == -1) {
                std::cerr << "Failed to read command response: " << std::strerror(errno) << std::endl;
                break;
            }

            if (command_answer[1] != 0x00) {
                std::cerr << "Command response error" << std::endl;
                break;
            }

            if ((send(fd, test_string.c_str(), test_string.length(), 0)) == -1) {
                std::cerr << "Failed to send test string: " << std::strerror(errno) << std::endl;
                break;
            }
            data->set_s(END);
            k_queue.add_read_event(fd, data);
            break;
        }
        case END: {
            ++ping_count;
            char buf[BUF_SIZE];
            while (recv(fd, buf, sizeof(buf), 0) > 0) {
            }
            if ((send(fd, test_string.c_str(), test_string.length(), 0)) == -1) {
                std::cerr << "Failed to send test string: " << std::strerror(errno) << std::endl;
            }
        }
    }
}

unsigned long long SocksActions::get_ping_count() const {
    return ping_count;
}

