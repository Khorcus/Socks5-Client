#ifndef SOCKS5_CLIENT_ACTIONS_HPP
#define SOCKS5_CLIENT_ACTIONS_HPP


class Actions {
public:
    virtual void on_read_event(int fd, void *udata) = 0;

    virtual void on_write_event(int fd, void *udata) = 0;
};


#endif //SOCKS5_CLIENT_ACTIONS_HPP
