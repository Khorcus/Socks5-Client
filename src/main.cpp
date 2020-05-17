#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <ctime>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024


enum status {
    CONNECTION = 1,
    COMMAND = 2,
    END = 3
};

struct udata {
    status s;
};

int connect(const char *host, uint16_t port);

int make_socket_non_blocking(int fd);

int main(int argc, char *argv[]) {
    int ev_number = 32;
    struct kevent ch_list;   /* events we want to monitor */
    struct kevent ev_list[ev_number];   /* events that were triggered */
    char buf[BUF_SIZE];

    /* Check argument count */
    if (argc != 8) {
        std::cerr << "Usage: Socks5-Client socks_host socks_port server_host server_port data_count session_number time"
                  << std::endl;
        std::cerr << "Example: Socks5-Client 127.0.0.1 8080 127.0.0.1 8081 500 10 60" << std::endl;
        exit(EXIT_FAILURE);
    }

    char *socks_host = argv[1];
    uint16_t socks_port = htons(std::atoi(argv[2]));
    char *server_host = argv[3];
    uint16_t server_port = htons(std::atoi(argv[4]));
    uint16_t data_count = std::atoi(argv[5]);
    std::string test_string = std::string(data_count, 'a');
    uint16_t session_number = std::atoi(argv[6]);
    uint16_t time = std::atoi(argv[7]);
    int kq, nev;
    unsigned long long k = 0;

    /* Create a new kernel event queue */
    if ((kq = kqueue()) == -1) {
        std::cerr << "Failed to create new kernel event queue: " << std::strerror(errno) << std::endl;
    }

    for (int i = 0; i < session_number; i++) {
        int fd;
        if ((fd = connect(socks_host, socks_port)) == -1) {
            std::cerr << "Failed to connect to server: " << std::strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        if ((send(fd, "\05\01\00", 3, 0)) == -1) {
            std::cerr << "Failed to send connection request: " << std::strerror(errno) << std::endl;
            return -1;
        }
        auto *data = new struct udata;
        data->s = CONNECTION;
        EV_SET(&ch_list, fd, EVFILT_READ, EV_ADD, 0, 0, data);
        if (kevent(kq, &ch_list, 1, NULL, 0, NULL) == -1) {
            std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
        }
    }

    EV_SET(&ch_list, 1, EVFILT_TIMER, EV_ADD, 0, time * 1000, 0);
    if (kevent(kq, &ch_list, 1, NULL, 0, NULL) == -1) {
        std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
    }

    /* Loop forever */
    for (;;) {
        /* Return if received at least one event */
        nev = kevent(kq, &ch_list, 1, ev_list, ev_number, NULL);

        if (nev == -1) {
            std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
        } else if (nev > 0) {
            for (int i = 0; i < nev; i++) {
                if (ev_list[i].flags & EV_EOF) {
                    std::cerr << "Socket closed by server" << std::endl;
                    close(ev_list[i].ident);
                } else if (ev_list[i].flags & EV_ERROR) {
                    std::cerr << "EV_ERROR: " << strerror(ev_list[i].data) << std::endl;
                    exit(EXIT_FAILURE);
                } else if (ev_list[i].filter == EVFILT_TIMER) {
                    std::cout << "Число обработанных запросов: " << k << std::endl;
                    exit(0);
                } else if (ev_list[i].filter == EVFILT_READ) {
                    int fd = ev_list[i].ident;
                    auto *data = reinterpret_cast<struct udata *>(ev_list[i].udata);
                    switch (data->s) {
                        case CONNECTION: {
                            uint8_t connection_answer[2];
                            uint8_t command_request[10] = {0x05, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                            struct in_addr server_addr;

                            if ((recv(fd, connection_answer, sizeof(connection_answer), 0)) == -1) {
                                std::cerr << "Failed to read connection response: " << std::strerror(errno)
                                          << std::endl;
                                break;
                            }

                            if (connection_answer[0] != 0x05) {
                                std::cerr << "Server doesn't support VER=5" << std::endl;
                                break;
                            }

                            if (connection_answer[1] != 0x00) {
                                std::cerr << "Server doesn't support no auth METHOD" << std::endl;
                                break;
                            }

                            if (inet_aton(server_host, &server_addr) == 0) {
                                std::cerr << "Invalid server host" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                            memcpy(command_request + 4, &server_addr.s_addr, 4);
                            memcpy(command_request + 8, &server_port, 2);

                            if ((send(fd, command_request, sizeof(command_request), 0)) == -1) {
                                std::cerr << "Failed to send command request: " << std::strerror(errno) << std::endl;
                                break;
                            }
                            data->s = COMMAND;
                            EV_SET(&ch_list, fd, EVFILT_READ, EV_ADD, 0, 0, data);
                            if (kevent(kq, &ch_list, 1, NULL, 0, NULL) == -1) {
                                std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
                            }
                            break;
                        }
                        case COMMAND: {
                            uint8_t command_answer[10];

                            if ((recv(fd, command_answer, sizeof(command_answer), 0)) == -1) {
                                std::cerr << "Failed to read command response: " << std::strerror(errno) << std::endl;
                                break;
                            }

                            if (command_answer[1] != 0x00) {
                                std::cerr << "Command response error" << std::endl;
                                break;
                            }

                            if ((send(fd, test_string.c_str(), test_string.length(), 0)) == -1) {
                                std::cerr << "Failed to send test string: " << std::strerror(errno) << std::endl;
                                break;
                            }
                            data->s = END;
                            EV_SET(&ch_list, fd, EVFILT_READ, EV_ADD, 0, 0, data);
                            if (kevent(kq, &ch_list, 1, NULL, 0, NULL) == -1) {
                                std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
                            }
                            break;
                        }
                        case END: {
                            ++k;
                            while (recv(fd, buf, sizeof(buf), 0) > 0) {
                            }
                            if ((send(fd, test_string.c_str(), test_string.length(), 0)) == -1) {
                                std::cerr << "Failed to send test string: " << std::strerror(errno) << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }
    if (close(kq) == -1)
        std::cerr << "close()" << std::strerror(errno) << std::endl;

    return EXIT_SUCCESS;
}


int connect(const char *host, uint16_t port) {
    struct sockaddr_in addr;
    struct hostent *hp;
    int sfd;

    if ((hp = gethostbyname(host)) == NULL) {
        std::cerr << "Failed to resolved host: " << std::strerror(errno) << std::endl;
        return -1;
    }

    if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "Failed to open socket: " << std::strerror(errno) << std::endl;
        return -1;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

    if (connect(sfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
        std::cerr << "Failed to connect to server: " << std::strerror(errno) << std::endl;
        return -1;
    }

    if (make_socket_non_blocking(sfd) == -1) {
        std::cerr << "Failed to make socket non blocking: " << std::strerror(errno) << std::endl;
        return -1;
    }

    return sfd;
}

int make_socket_non_blocking(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
