#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <zconf.h>
#include "ClientSocket.hpp"

#define BUF_SIZE 1024
#define EV_NUMBER 32

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
//    int sent_size = 0;
//    const char *char_data = static_cast<const char *>(data);
//    while (sent_size != size) {
//        int current_sent_size = 0;
//        if ((current_sent_size = ::send(sfd, char_data + sent_size, size - sent_size, 0)) == -1) {
//            return -1;
//        }
//        sent_size += current_sent_size;
//    }
//    return sent_size;
    const char *char_data = static_cast<const char *>(data);
    if (send_data.empty()) {
        int sent_size = 0;
        if ((sent_size = ::send(sfd, char_data + sent_size, size - sent_size, 0)) == -1) {
            return -1;
        }
        if (sent_size != size) {
            send_data.append(char_data + sent_size);
            k_queue.add_write_event(sfd, this);
        }
        return sent_size;
    } else {
        send_data.append(char_data);
        return 0;
    }
}

int ClientSocket::receive(void *data, size_t size) {
    return recv(sfd, data, size, 0);
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
