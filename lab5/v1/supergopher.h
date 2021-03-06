#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>


#define VPNREQSIZE  6
#define VPNACKSIZE  3
#define VPNACKANS   3
#define VPNREJSIZE  1
#define VPNREJANS   0

#define MAXBUFSIZE  100
#define MAXSOCKIND  10
#define MAXIPSIZE   30


#define TABLEUPDATE


struct forwarding_table {
    size_t socket_index;
    in_port_t transit_port;
    in_port_t transit_port_2;
    in_addr_t client_IP;
    in_port_t client_port;
    in_addr_t server_IP;
    in_port_t server_port;
};


extern int initialize_udp_sockfd(in_port_t port_num, struct sockaddr_in *socket_addr, socklen_t socket_len);

extern void get_socket_name(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern size_t receive_from_socket(int sockfd, void *buf, size_t len, int flags,
                                  struct sockaddr *src_addr, socklen_t *addrlen);

extern void send_to_socket(int sockfd, const void *buf, size_t len, int flags,
                           const struct sockaddr *dest_addr, socklen_t addrlen);
