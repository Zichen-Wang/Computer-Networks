#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>


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


ssize_t receive_from_socket(int sockfd, void *buf, size_t len, int flags,
        struct sockaddr *src_addr, socklen_t *addrlen) {

    ssize_t ret;
    ret = recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (ret == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recvfrom()");
            fprintf(stderr, "cannot receive a packet; terminating...\n");
            fflush(stderr);
            exit(1);
        }
    }

    return ret;
}


void send_to_socket(int sockfd, const void *buf, size_t len, int flags,
                    const struct sockaddr *dest_addr, socklen_t addrlen) {
    ssize_t ret;
    ret = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    if (ret == -1) {
        perror("sendto()");
        fprintf(stderr, "cannot send a packet; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    if (ret != len) {
        fprintf(stderr, "not sent all bytes; %lu/%lu bytes sent\n", ret, len);
        fflush(stderr);
        exit(1);
    }
}
