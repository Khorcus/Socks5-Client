#ifndef SOCKS5_CLIENT_SOCKS_ACTIONS_HPP
#define SOCKS5_CLIENT_SOCKS_ACTIONS_HPP

#include <cstdint>
#include <string>
#include "../kqueue/Actions.hpp"
#include "../utils.hpp"
#include "../kqueue/KQueue.hpp"

class SocketActions : public Actions {
public:
    SocketActions(const char *server_host, uint16_t server_port, std::string test_string, KQueue  k_queue);

    void on_read_event(int fd, void *udata) override;

    unsigned long long get_ping_count() const;

private:
    unsigned long long ping_count;
    const char *server_host;
    uint16_t server_port;
    const std::string test_string;
    KQueue k_queue;
};


#endif //SOCKS5_CLIENT_SOCKS_ACTIONS_HPP
