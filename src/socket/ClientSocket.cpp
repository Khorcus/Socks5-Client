#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <zconf.h>
#include "ClientSocket.hpp"

#define BUF_SIZE 1024
#define EV_NUMBER 32

//TODO: Все инициализировать

ClientSocket::ClientSocket()
        : sfd(), s() {}

ClientSocket::~ClientSocket() {
    shutdown(sfd, SHUT_RDWR);
    close(sfd);
}

ClientSocket::ClientSocket(ClientSocket &&src) noexcept : sfd(-1), s() {
    src.swap(*this);
}

ClientSocket &ClientSocket::operator=(ClientSocket &&src) noexcept {
    src.swap(*this);
    return *this;
}

bool ClientSocket::connect(const char *host, uint16_t port) {
    struct sockaddr_in addr{};
    struct hostent *hp;

    if ((hp = gethostbyname(host)) == NULL) {
        std::cerr << "Failed to resolved host: " << std::strerror(errno) << std::endl;
        return false;
    }

    if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "Failed to open socket: " << std::strerror(errno) << std::endl;
        return false;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

    if (::connect(sfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
        std::cerr << "Failed to connect to server: " << std::strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool ClientSocket::make_non_blocking() {
    int flags;
    if ((flags = fcntl(sfd, F_GETFL, 0)) != -1) {
        if (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) != -1) {
            return true;
        }
    }
    std::cerr << "Failed to make socket non blocking: " << std::strerror(errno) << std::endl;
    return false;
}

int ClientSocket::send(const void *data, size_t size) {
    return ::send(sfd, data, size, 0);
}

int ClientSocket::receive(void *data, size_t size) {
    return recv(sfd, data, size, 0);
}

bool ClientSocket::send_all(const void *data, size_t size, const std::function<void(ClientSocket &)> &f) {
    int sent_size = 0;
    send_data.reserve(size);
    if ((sent_size = send(data, size)) == -1) {
        if (f) {
            //TODO: Прокинуть ошибку
            f(*this);
        } else {
            return false;
        }
    }
    if (sent_size != size) {
        const auto *uint_data = static_cast<const uint8_t *>(data);
        send_data.insert(send_data.end(), uint_data + sent_size, uint_data + size);
        k_queue.add_write_event(sfd, this);
    } else {
        if (f) {
            f(*this);
        } else {
            return true;
        }
    }
    return true;
}

bool ClientSocket::receive_all(size_t size, const std::function<void(std::vector<uint8_t> &, ClientSocket &)> &f) {
    int receive_sizea = 0;
    receive_data.reserve(size);
    if ((receive_sizea = receive(receive_data.data(), size)) == -1) {
        if (f) {
            //TODO: Прокинуть ошибку
            f(receive_data, *this);
        } else {
            return false;
        }
    }
    if (receive_sizea != size) {
        this->receive_size = size;
        k_queue.add_read_event(sfd, this);
    } else {
        if (f) {
            f(receive_data, *this);
        } else {
            return false;
        }
    }
    return true;
}

void ClientSocket::discard_all() {
    char buf[BUF_SIZE];
    while (recv(sfd, buf, sizeof(buf), 0) > 0) {
    }
}

int ClientSocket::get_fd() const {
    return sfd;
}

status ClientSocket::get_s() const {
    return s;
}

const char *ClientSocket::get_server_host() const {
    return server_host;
}

uint16_t ClientSocket::get_server_port() const {
    return server_port;
}

std::string ClientSocket::get_test_string() const {
    return test_string;
}

KQueue ClientSocket::get_k_queue() const {
    return k_queue;
}

void ClientSocket::set_test_data(const char *server_host, uint16_t server_port, std::string &test_string) {
    this->server_host = server_host;
    this->server_port = server_port;
    this->test_string = test_string;
}

void ClientSocket::set_s(status s) {
    this->s = s;
}

void ClientSocket::set_k_queue(const KQueue &k_queue) {
    this->k_queue = k_queue;
}


void ClientSocket::swap(ClientSocket &other) {
    std::swap(sfd, other.sfd);
    std::swap(s, other.s);
}
