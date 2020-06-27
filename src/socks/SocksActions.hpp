
#ifndef SOCKS5_CLIENT_SOCKS_ACTIONS_HPP
#define SOCKS5_CLIENT_SOCKS_ACTIONS_HPP

#include <cstdint>
#include <string>
#include "../kqueue/Actions.hpp"
#include "../utils.hpp"
#include "../kqueue/KQueue.hpp"
#include "../socket/ClientSocket.hpp"

class SocksActions : public Actions {
public:
    explicit SocksActions();

    void on_read_event(int fd, void *udata) override;

    void on_write_event(int fd, void *udata) override;

    unsigned long long get_ping_count() const;

    static void on_connection_receive(std::vector<uint8_t> &connection_answer, ClientSocket &s);

    static void on_connection_send(ClientSocket &s);

    static void on_command_receive(std::vector<uint8_t> &command_answer, ClientSocket &s);

    static void on_command_send(ClientSocket &s);

    static void on_discard_all(std::vector<uint8_t> &command_answer, ClientSocket &s);

private:
    unsigned long long ping_count;
};


#endif //SOCKS5_CLIENT_SOCKS_ACTIONS_HPP
