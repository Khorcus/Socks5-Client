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

#define BUF_SIZE 1024

int tcp_open(const char *host, int port);

void send_buf_to_socket(int sfd, const char *buf, int len);

int make_socket_non_blocking(int fd);

int main(int argc, char *argv[]) {
    struct kevent chlist[2];   /* events we want to monitor */
    struct kevent evlist[2];   /* events that were triggered */
    char buf[BUF_SIZE];
    int sfd, kq, nev, i;

    /* Check argument count */
    if (argc != 3) {
        std::cerr << "Usage: Socks5-Client host port" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* Open a connection to a host:port pair */
    if ((sfd = tcp_open(argv[1], atoi(argv[2]))) == -1) {
        exit(EXIT_FAILURE);
    }

    /* Create a new kernel event queue */
    if ((kq = kqueue()) == -1)
        std::cerr << "Failed to create new kernel event queue" << std::endl;

    /* Initialise kevent structures */
    EV_SET(&chlist[0], sfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0); /* any incoming host data in the socket */
    EV_SET(&chlist[1], fileno(stdin), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
           0); /* any user data in the standard input stream */

    /* Loop forever */
    for (;;) {
        /* Return if received at least one event */
        nev = kevent(kq, chlist, 2, evlist, 2, NULL);

        if (nev == -1) {
            std::cerr << "Failed to kevent" << std::endl;
        } else if (nev > 0) {
            if (evlist[0].flags & EV_EOF) {
                std::cerr << "Socket closed by server" << std::endl;
                exit(EXIT_FAILURE);
            }

            for (i = 0; i < nev; i++) {
                if (evlist[i].flags & EV_ERROR) {
                    std::cerr << "EV_ERROR: " << strerror(evlist[i].data) << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (evlist[i].ident == sfd) {
                    /* We have data from the host */
                    memset(buf, 0, BUF_SIZE);
                    if (read(sfd, buf, BUF_SIZE) == -1)
                        std::cerr << "Failed to read data from server" << std::endl;
                    std::cout << buf;
                } else if (evlist[i].ident == fileno(stdin)) {
                    /* We have data from stdin */
                    memset(buf, 0, BUF_SIZE);
                    fgets(buf, BUF_SIZE, stdin);
                    send_buf_to_socket(sfd, buf, strlen(buf));
                }
            }
        }
    }
    if (close(kq) == -1)
        std::cerr << "close()" << std::endl;

    return EXIT_SUCCESS;
}


int tcp_open(const char *host, int port) {
    struct sockaddr_in addr;
    struct hostent *hp;
    int sfd;

    if ((hp = gethostbyname(host)) == NULL) {
        std::cerr << "Failed to resolved host" << std::endl;
        return -1;
    }

    if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "Failed to open socket" << std::endl;
        return -1;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

    if (connect(sfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        return -1;
    }

    if (make_socket_non_blocking(sfd) == -1) {
        std::cerr << "Failed to make socket non blocking" << std::endl;
        return -1;
    }

    return sfd;
}

int make_socket_non_blocking(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
        std::cerr << "Failed to make socket non blocking" << std::endl;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void send_buf_to_socket(int sfd, const char *buf, int len) {
    int sent_bytes, pos;
    pos = 0;
    do {
        if ((sent_bytes = send(sfd, buf + pos, len - pos, 0)) == -1)
            std::cerr << "Failed to send data" << std::endl;
        pos += sent_bytes;
    } while (sent_bytes > 0);
}