#include <iostream>
#include <zconf.h>
#include "KQueue.hpp"

KQueue::KQueue() : KQueue(32) {}

KQueue::KQueue(int ev_number)
        : kq(), ev_number(ev_number), ch_list(), ev_list() {}

bool KQueue::init() {
    ev_list.reserve(ev_number);
    if ((kq = kqueue()) == -1) {
        std::cerr << "Failed to create new kernel event queue: " << std::strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool KQueue::add_read_event(int fd, void *data) {
    EV_SET(&ch_list, fd, EVFILT_READ, EV_ADD, 0, 0, data);
    if (kevent(kq, &ch_list, 1, NULL, 0, NULL) == -1) {
        std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool KQueue::add_timer_event(uint16_t time) {
    EV_SET(&ch_list, 1, EVFILT_TIMER, EV_ADD, 0, time, 0);
    if (kevent(kq, &ch_list, 1, NULL, 0, NULL) == -1) {
        std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
        return false;
    }
    return true;
}

void KQueue::start_loop(Actions *actions) {
    int nev;
    for (;;) {
        if ((nev = kevent(kq, &ch_list, 1, ev_list.data(), ev_number, NULL)) == -1) {
            std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
        } else if (nev > 0) {
            for (int i = 0; i < nev; i++) {
                if (ev_list[i].flags & EV_EOF) {
                    std::cerr << "ClientSocket closed by server" << std::endl;
                    close(ev_list[i].ident);
                    return;
                } else if (ev_list[i].flags & EV_ERROR) {
                    std::cerr << "EV_ERROR: " << strerror(ev_list[i].data) << std::endl;
                    return;
                } else if (ev_list[i].filter == EVFILT_TIMER) {
                    return;
                } else if (ev_list[i].filter == EVFILT_READ) {
                    int fd = ev_list[i].ident;
                    actions->on_read_event(fd, ev_list[i].udata);
                }
            }
        }
    }
}


