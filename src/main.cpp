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

int connect(const char *host, int port);

void send_buf_to_socket(int sfd, const char *buf, int len);

int make_socket_non_blocking(int fd);

int test_server(const char *socks_host, uint16_t socks_port, const char *server_host, uint16_t server_port);

int main(int argc, char *argv[]) {
    int ev_number = 32;
    struct kevent ch_list;   /* events we want to monitor */
    struct kevent ev_list[ev_number];   /* events that were triggered */
    char buf[BUF_SIZE];

    /* Check argument count */
    if (argc != 5) {
        std::cerr << "Usage: Socks5-Client socks_host socks_port server_host server_port" << std::endl;
        exit(EXIT_FAILURE);
    }

    char *socks_host = argv[1];
    uint16_t socks_port = htons(atoi(argv[2]));
    char *server_host = argv[3];
    uint16_t server_port = htons(atoi(argv[4]));
    int kq, nev, i;

    /* Create a new kernel event queue */
    if ((kq = kqueue()) == -1)
        std::cerr << "Failed to create new kernel event queue: " << std::strerror(errno) << std::endl;

    /* Initialise kevent structures */
//    EV_SET(&ch_list[0], sfd, EVFILT_READ, EV_ADD , 0, 0, 0); /* any incoming host data in the socket */
//    EV_SET(&ch_list[1], fileno(stdin), EVFILT_READ, EV_ADD , 0, 0,0); /* any user data in the standard input stream */
    EV_SET(&ch_list, 1, EVFILT_TIMER, EV_ADD, 0, 2000, 0);
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
            for (i = 0; i < nev; i++) {
                if (ev_list[i].flags & EV_EOF) {
                    std::cerr << "Socket closed by server" << std::endl;
                    EV_SET(&ch_list, ev_list[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                    if (kevent(kq, &ch_list, 1, NULL, 0, NULL) == -1) {
                        std::cerr << "Failed to kevent: " << std::strerror(errno) << std::endl;
                    }
                } else if (ev_list[i].flags & EV_ERROR) {
                    std::cerr << "EV_ERROR: " << strerror(ev_list[i].data) << std::endl;
                    exit(EXIT_FAILURE);
                } else if (ev_list[i].filter == EVFILT_TIMER) {
                    test_server(socks_host, socks_port, server_host, server_port);
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

//    if (make_socket_non_blocking(sfd) == -1) {
//        std::cerr << "Failed to make socket non blocking: " << std::strerror(errno) << std::endl;
//        return -1;
//    }

    return sfd;
}

int make_socket_non_blocking(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void send_buf_to_socket(int sfd, const char *buf, int len) {
    int sent_bytes, pos;
    pos = 0;
    do {
        if ((sent_bytes = send(sfd, buf + pos, len - pos, 0)) == -1)
            std::cerr << "Failed to send data: " << std::strerror(errno) << std::endl;
        pos += sent_bytes;
    } while (sent_bytes > 0);
}

int test_server(const char *socks_host, uint16_t socks_port, const char *server_host, uint16_t server_port) {
    struct in_addr server_addr;
    int sfd;
    char test_word[3] = {'k', 'e', 'k'};

    uint8_t connection_answer[2];
    uint8_t command_answer[10];
    uint8_t command_request[10] = {0x05, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    /* Open a connection to a host:port pair */
    if ((sfd = connect(socks_host, socks_port)) == -1) {
        std::cerr << "Failed to connect to server: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    if ((send(sfd, "\05\01\00", 3, 0)) == -1) {
        std::cerr << "Failed to send connection request: " << std::strerror(errno) << std::endl;
        return -1;
    }

    if ((recv(sfd, connection_answer, sizeof(connection_answer), 0)) == -1) {
        std::cerr << "Failed to read connection response: " << std::strerror(errno) << std::endl;
        return -1;
    }

    if (connection_answer[0] != 0x05) {
        std::cerr << "Server doesn't support VER=5" << std::endl;
        return -1;
    }

    if (connection_answer[1] != 0x00) {
        std::cerr << "Server doesn't support no auth METHOD" << std::endl;
        return -1;
    }

    if (inet_aton(server_host, &server_addr) == 0) {
        std::cerr << "Invalid server host" << std::endl;
        exit(EXIT_FAILURE);
    }
    memcpy(command_request + 4, &server_addr.s_addr, 4);
    memcpy(command_request + 8, &server_port, 2);

    if ((send(sfd, command_request, sizeof(command_request), 0)) == -1) {
        std::cerr << "Failed to send command request: " << std::strerror(errno) << std::endl;
        return -1;
    }

    if ((recv(sfd, command_answer, sizeof(command_answer), 0)) == -1) {
        std::cerr << "Failed to read command response: " << std::strerror(errno) << std::endl;
        return -1;
    }

    if (command_answer[1] != 0x00) {
        std::cerr << "Command response error" << std::endl;
        return -1;
    }

    if ((send(sfd, test_word, 3, 0)) == -1) {
        std::cerr << "Failed to send test word: " << std::strerror(errno) << std::endl;
        return -1;
    }

    if ((recv(sfd, test_word, sizeof(connection_answer), 0)) == -1) {
        std::cerr << "Failed to read test word: " << std::strerror(errno) << std::endl;
        return -1;
    }
    std::cout << "Test word: " << test_word << std::endl;

    return 0;
}