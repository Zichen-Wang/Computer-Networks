#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>


#define BUF_SIZE    30
#define IP_SIZE     4
#define ACL_SIZE    1000


struct ACL {
    in_addr_t ip_addr;
    unsigned int pubkey;
};


extern unsigned int encode_called_times;


extern size_t read_acl(FILE *fp, struct ACL *acl);

extern int search_acl(in_addr_t ip, struct ACL *acl, size_t len);

extern void parse(char *argv[], char *buffer);

extern int create_socket(int domain, int type, int protocol);

extern void bind_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

extern void get_socket_name(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern size_t receive_from_socket(
        int sockfd,
        void *buf,
        size_t len,
        int flags,
        struct sockaddr *src_addr,
        socklen_t *addrlen);

extern char myencode(char x, unsigned int pubkey);
