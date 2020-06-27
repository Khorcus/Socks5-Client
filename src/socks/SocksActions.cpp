#include "SocksActions.hpp"
#include "../socket/ClientSocket.hpp"
#include <netinet/in.h>
#include <iostream>
#include <utility>
#include <arpa/inet.h>

SocksActions::SocksActions() :
        ping_count(0) {
}

void SocksActions::on_read_event(int fd, void *udata) {
    auto *s = reinterpret_cast<ClientSocket *>(udata);
    if (s->get_s() == DISCARD) {
        s->discard_all(on_discard_all);
        return;
    }

    if (!s->get_receive_data().empty()) {
        if ((s->receive(s->get_receive_data().data() + s->get_receive_data().size(),
                        s->get_receive_size() - s->get_receive_data().size())) == -1) {
            //TODO: remove_read_event (пока не надо)
            s->get_receive_data().clear();
            s->set_receive_size(0);
            if (s->get_receive_f()) {
                //TODO: Прокинуть ошибку
                s->get_receive_f()(s->get_receive_data(), *s);
            } else {
                return;
            }
        }
        if (s->get_receive_data().size() == s->get_receive_size() && s->get_receive_f()) {
            //TODO: remove_read_event (пока не надо)
            s->get_receive_data().clear();
            s->set_receive_size(0);
            s->get_receive_f()(s->get_receive_data(), *s);
        }
    }
    switch (s->get_s()) {
        case CONNECTION: {
            if (!s->receive_all(2, on_connection_receive)) {
                std::cerr << "Failed to read connection response: " << std::strerror(errno) << std::endl;
            }
            break;
        }
        case COMMAND: {
            if (!s->receive_all(10, on_command_receive)) {
                std::cerr << "Failed to read command response: " << std::strerror(errno) << std::endl;
            }
            break;
        }
        case END: {
            ++ping_count;
            s->set_s(DISCARD);
            s->get_k_queue().add_read_event(s->get_fd(), &s);
            s->discard_all(on_discard_all);
        }
        case DISCARD: {

        }
    }
}

void SocksActions::on_write_event(int fd, void *udata) {
    auto *s = reinterpret_cast<ClientSocket *>(udata);
    if (!s->get_send_data().empty()) {
        int sent_size = 0;
        if ((sent_size = s->send(s->get_send_data().data(), s->get_send_data().size())) == -1) {
            //TODO: remove_write_event
            s->get_send_data().clear();
            if (s->get_send_f()) {
                //TODO: Прокинуть ошибку
                s->get_send_f();
            } else {
                return;
            }
        }
        if (s->get_send_data().size() == sent_size) {
            //TODO: remove_write_event
            s->get_send_data().clear();
            if (s->get_send_f()) {
                s->get_send_f();
            }
        } else {
            s->get_send_data().erase(s->get_send_data().begin(), s->get_send_data().begin() + sent_size);
        }
    }
}

unsigned long long SocksActions::get_ping_count() const {
    return ping_count;
}

void SocksActions::on_connection_receive(std::vector<uint8_t> &connection_answer, ClientSocket &s) {
    uint8_t command_request[10] = {0x05, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    struct in_addr server_addr{};
    uint16_t server_port = s.get_server_port();

    if (connection_answer[0] != 0x05) {
        std::cerr << "Server doesn't support VER=5" << std::endl;
        return;
    }

    if (connection_answer[1] != 0x00) {
        std::cerr << "Server doesn't support no auth METHOD" << std::endl;
        return;
    }

    if (inet_aton(s.get_server_host(), &server_addr) == 0) {
        std::cerr << "Invalid server host" << std::endl;
        return;
    }
    memcpy(command_request + 4, &server_addr.s_addr, 4);
    memcpy(command_request + 8, &server_port, 2);

    if (!s.send_all(command_request, sizeof(command_request), on_connection_send)) {
        std::cerr << "Failed to send command request: " << std::strerror(errno) << std::endl;
    }
}

void SocksActions::on_connection_send(ClientSocket &s) {
    s.set_s(COMMAND);
    s.get_k_queue().add_read_event(s.get_fd(), &s);
}

void SocksActions::on_command_receive(std::vector<uint8_t> &command_answer, ClientSocket &s) {
    if (command_answer[1] != 0x00) {
        std::cerr << "Command response error" << std::endl;
        return;
    }

    if (!s.send_all(s.get_test_string().c_str(), s.get_test_string().length(),
                    on_command_send)) {
        std::cerr << "Failed to send test string: " << std::strerror(errno) << std::endl;
    }
}

void SocksActions::on_command_send(ClientSocket &s) {
    s.set_s(END);
    s.get_k_queue().add_read_event(s.get_fd(), &s);
}

void SocksActions::on_discard_all(std::vector<uint8_t> &command_answer, ClientSocket &s) {
    s.set_s(END);
    s.get_k_queue().add_read_event(s.get_fd(), &s);
    if (!s.send_all(s.get_test_string().c_str(), s.get_test_string().length(), nullptr)) {
        std::cerr << "Failed to send test string: " << std::strerror(errno) << std::endl;
    }
}
