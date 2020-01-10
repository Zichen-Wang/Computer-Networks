#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define BUF_SIZE    30
#define IP_SIZE     4

#define HOSTNAME_SIZE   100


extern unsigned int decode_called_times;


extern size_t join(int argc, const char *argv[], unsigned char *buffer);

extern int create_socket(int domain, int type, int protocol);

extern void bind_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

extern void send_to_socket(
        int sockfd,
        const void *buf,
        size_t len,
        int flags,
        const struct sockaddr *dest_addr, socklen_t addrlen);

extern char mydecode(char x, unsigned int prikey);

extern in_addr_t get_local_ip_addr();
