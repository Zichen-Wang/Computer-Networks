#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


static int create_socket(int domain, int type, int protocol) {
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


static void bind_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (bind(sockfd, addr, addrlen) == -1) {
        perror("bind()");
        fprintf(stderr, "bind failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


int initialize_udp_sockfd(in_port_t port_num, struct sockaddr_in *socket_addr, socklen_t socket_len) {
    int socket_fd;

    // create a socket file descriptor for vpn server
    socket_fd = create_socket(AF_INET, SOCK_DGRAM, 0);

    // initialize the server address data structure
    memset((void *) socket_addr, 0, socket_len);

    // use the Internet protocol v4 addresses
    socket_addr -> sin_family = AF_INET;

    // server_ip_addr binds any IPv4 address
    (socket_addr -> sin_addr).s_addr = INADDR_ANY;

    // assign the server port number
    socket_addr -> sin_port = port_num;

    // bind the socket with server address data structure
    bind_socket(socket_fd, (const struct sockaddr *) socket_addr, socket_len);

    return socket_fd;
}


void get_socket_name(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    if (getsockname(sockfd, addr, addrlen) == -1) {
        perror("getsockname()");
        fprintf(stderr, "get socket name failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}