#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>


#define BUF_SIZE 1000


extern void parse(char *argv[], char *buffer);
extern void join(int argc, const char *argv[], char *buffer);
extern int create_socket(int domain, int type, int protocol);
extern void bind_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern void get_socket_name(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern void listen_socket(int sockfd, int backlog);
extern int accept_socket(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern void connect_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern char * read_from_socket(int fd);
extern void write_to_socket(int fd, const void *buf, size_t count);