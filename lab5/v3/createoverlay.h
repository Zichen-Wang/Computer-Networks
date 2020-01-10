#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <arpa/inet.h>


#define OVERLAYTIMEOUT  30

#define OVERLAYACKSIZE  3
#define OVERLAYACKANS   3
#define OVERLAYREJSIZE  1
#define OVERLAYREJANS   0

#define MAXBUFSIZE      5000


extern int initialize_udp_sockfd(in_port_t port_num, struct sockaddr_in *socket_addr, socklen_t socket_len);

extern void get_socket_name(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern size_t receive_from_socket(int sockfd, void *buf, size_t len, int flags,
                                  struct sockaddr *src_addr, socklen_t *addrlen);

extern void send_to_socket(int sockfd, const void *buf, size_t len, int flags,
                           const struct sockaddr *dest_addr, socklen_t addrlen);
