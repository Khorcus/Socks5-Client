#include "SocksActions.hpp"
#include "../socket/ClientSocket.hpp"
#include <netinet/in.h>
#include <iostream>
#include <utility>
#include <arpa/inet.h>

#define BUF_SIZE 1024

SocksActions::SocksActions() :
        ping_count(0) {
}

void SocksActions::on_read_event(int fd, void *udata) {
    auto *client_socket = reinterpret_cast<ClientSocket *>(udata);
    if (!client_socket->receive_data.empty()) {
        if ((client_socket->receive(client_socket->receive_data.data() + client_socket->receive_data.size(),
                                    client_socket->receive_size - client_socket->receive_data.size())) == -1) {
            //TODO: remove_read_event (пока не надо)
            client_socket->receive_data.clear();
            client_socket->receive_size = 0;
            if (client_socket->receive_f) {
                //TODO: Прокинуть ошибку
                client_socket->receive_f(client_socket->receive_data);
            } else {
                return;
            }
        }
        if (client_socket->receive_data.size() == client_socket->receive_size && client_socket->receive_f) {
            //TODO: remove_read_event (пока не надо)
            client_socket->receive_data.clear();
            client_socket->receive_size = 0;
            client_socket->receive_f(client_socket->receive_data);
        }
    }
    switch (client_socket->get_s()) {
        case CONNECTION: {
            if (!client_socket->receive_all(2, on_connection_receive)) {
                std::cerr << "Failed to read connection response: " << std::strerror(errno) << std::endl;
            }
            break;
        }
        case COMMAND: {
            if (!client_socket->receive_all(10, on_command_receive)) {
                std::cerr << "Failed to read command response: " << std::strerror(errno) << std::endl;
            }
            break;
        }
        case END: {
            ++ping_count;
            client_socket->discard_all();
            if (!client_socket->send_all(client_socket->get_test_string().c_str(),
                                         client_socket->get_test_string().length(),
                                         nullptr)) {
                std::cerr << "Failed to send test string: " << std::strerror(errno) << std::endl;
            }
        }
    }
}

void SocksActions::on_write_event(int fd, void *udata) {
    auto *client_socket = reinterpret_cast<ClientSocket *>(udata);
    if (!client_socket->send_data.empty()) {
        int sent_size = 0;
        if ((sent_size = client_socket->send(client_socket->send_data.data(), client_socket->send_data.size())) == -1) {
            //TODO: remove_write_event
            client_socket->send_data.clear();
            if (client_socket->send_f) {
                //TODO: Прокинуть ошибку
                client_socket->send_f();
            } else {
                return;
            }
        }
        if (client_socket->send_data.size() == sent_size) {
            //TODO: remove_write_event
            client_socket->send_data.clear();
            if (client_socket->send_f) {
                client_socket->send_f();
            }
        } else {
            client_socket->send_data.erase(client_socket->send_data.begin(),
                                           client_socket->send_data.begin() + sent_size);
        }
    }
}

unsigned long long SocksActions::get_ping_count() const {
    return ping_count;
}

void SocksActions::on_connection_receive(std::vector<uint8_t> &connection_answer, ClientSocket &client_socket) {
    uint8_t command_request[10] = {0x05, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    struct in_addr server_addr{};
    uint16_t server_port = client_socket.get_server_port();

    if (connection_answer[0] != 0x05) {
        std::cerr << "Server doesn't support VER=5" << std::endl;
        return;
    }

    if (connection_answer[1] != 0x00) {
        std::cerr << "Server doesn't support no auth METHOD" << std::endl;
        return;
    }

    if (inet_aton(client_socket.get_server_host(), &server_addr) == 0) {
        std::cerr << "Invalid server host" << std::endl;
        return;
    }
    memcpy(command_request + 4, &server_addr.s_addr, 4);
    memcpy(command_request + 8, &server_port, 2);

    if (!client_socket.send_all(command_request, sizeof(command_request), on_connection_send)) {
        std::cerr << "Failed to send command request: " << std::strerror(errno) << std::endl;
    }
}

void SocksActions::on_connection_send(ClientSocket &client_socket) {
    client_socket.set_s(COMMAND);
    client_socket.get_k_queue().add_read_event(client_socket.get_fd(), &client_socket);
}

void SocksActions::on_command_receive(std::vector<uint8_t> &command_answer, ClientSocket &client_socket) {
    if (command_answer[1] != 0x00) {
        std::cerr << "Command response error" << std::endl;
        return;
    }

    if (!client_socket.send_all(client_socket.get_test_string().c_str(), client_socket.get_test_string().length(),
                                on_command_send)) {
        std::cerr << "Failed to send test string: " << std::strerror(errno) << std::endl;
    }
}

void SocksActions::on_command_send(ClientSocket &client_socket) {
    client_socket.set_s(END);
    client_socket.get_k_queue().add_read_event(client_socket.get_fd(), &client_socket);
}
