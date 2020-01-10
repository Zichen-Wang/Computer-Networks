#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>


extern void parse(char *argv[], char *buffer);
extern void join(int argc, const char *argv[], char *buffer);
extern int create_socket(int domain, int type, int protocol);
extern void bind_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern void get_socket_name(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern ssize_t receive_from_socket(int sockfd, void *buf, size_t len, int flags,
                                   struct sockaddr *src_addr, socklen_t *addrlen);
extern void send_to_socket(int sockfd, const void *buf, size_t len, int flags,
                           const struct sockaddr *dest_addr, socklen_t addrlen);

