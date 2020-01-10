#include "mysocket.h"


int create_socket(int domain, int type, int protocol) {
    int socket_fd;
    socket_fd = socket(domain, type, protocol);

    if (socket_fd == -1) {
        perror("socket()");
        fprintf(stderr, "socket creation failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
    return socket_fd;
}


void bind_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (bind(sockfd, addr, addrlen) == -1) {
        perror("bind()");
        fprintf(stderr, "bind failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


void listen_socket(int sockfd, int backlog) {
    if (listen(sockfd, backlog) == -1) {
        perror("listen()");
        fprintf(stderr, "listen failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


int accept_socket(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int new_fd;

    new_fd = accept(sockfd, addr, addrlen);
    if (new_fd == -1) {
        perror("accept()");
        fprintf(stderr, "accept failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    return new_fd;
}


void connect_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (connect(sockfd, addr, addrlen) == -1) {
        perror("connect()");
        fprintf(stderr, "connection failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


void write_to_socket(int fd, const void *buf, size_t count) {
    if (write(fd, buf, count) == -1) {
        perror("write()");
        fprintf(stderr, "write failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}
