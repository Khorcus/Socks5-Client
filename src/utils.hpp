#ifndef SOCKS5_CLIENT_UTILS_HPP
#define SOCKS5_CLIENT_UTILS_HPP

enum status {
    CONNECTION = 1,
    COMMAND = 2,
    END = 3
};

struct udata {
    status s;
};

#endif //SOCKS5_CLIENT_UTILS_HPP
