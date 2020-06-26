#ifndef SOCKS5_CLIENT_CLIENT_SOCKET_HPP
#define SOCKS5_CLIENT_CLIENT_SOCKET_HPP


#include <cstdint>
#include <string>
#include "../utils.hpp"
#include "../kqueue/KQueue.hpp"


class ClientSocket {
public:
    ClientSocket();

    ClientSocket(const ClientSocket &) = delete;

    ClientSocket(ClientSocket &&src) noexcept;

    ClientSocket &operator=(const ClientSocket &) = delete;

    ClientSocket &operator=(ClientSocket &&src) noexcept;

    ~ClientSocket();

    bool connect(const char *host, uint16_t port);

    bool make_non_blocking();

    int send(const void *data, size_t size);

    int receive(void *data, size_t size);

    bool send_all(const void *data, size_t size, const std::function<void(ClientSocket &)> &f);

    bool receive_all(size_t size, const std::function<void(std::vector<uint8_t> &, ClientSocket &)> &f);

    void discard_all();

    int get_fd() const;

    status get_s() const;

    KQueue get_k_queue() const;

    const char *get_server_host() const;

    uint16_t get_server_port() const;

    std::string get_test_string() const;

    void set_test_data(const char *server_host, uint16_t server_port, std::string &test_string);

    void set_s(status s);

    void set_k_queue(const KQueue &k_queue);

    void swap(ClientSocket &other);

    //TODO: Сделать приватными
    std::vector<uint8_t> send_data;
    std::function<void()> send_f;
    std::vector<uint8_t> receive_data;
    size_t receive_size;
    std::function<void(std::vector<uint8_t>)> receive_f;

private:
    int sfd;
    status s;
    KQueue k_queue;

    const char *server_host;
    uint16_t server_port;
    std::string test_string;
};


#endif //SOCKS5_CLIENT_CLIENT_SOCKET_HPP
