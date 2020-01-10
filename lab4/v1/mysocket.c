#include <stdio.h>
#include <stdlib.h>
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
