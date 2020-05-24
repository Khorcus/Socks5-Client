#ifndef SOCKS5_CLIENT_KQUEUE_HPP
#define SOCKS5_CLIENT_KQUEUE_HPP


#include <sys/event.h>
#include "Actions.hpp"


class KQueue {
public:
    KQueue();
    explicit KQueue(int ev_number);
    ~KQueue();
    bool init();
    bool add_read_event(int fd, void* data);
    bool add_timer_event(uint16_t time);
    void start_loop(Actions* actions);


private:
    int kq;
    int ev_number;
    struct kevent ch_list;
    struct kevent *ev_list;
};


#endif //SOCKS5_CLIENT_KQUEUE_HPP
