#include "server.h"


unsigned int encode_called_times;


char myencode(char x, unsigned int pubkey) {
    char ret;
    ret = x ^ ((pubkey >> (encode_called_times << 3)) & 255);
    encode_called_times = (encode_called_times + 1) & 3;

    return ret;
}


void parse(char *argv[], char *buffer) {
    char *token;
    int argc;

    // reset the arg counter
    argc = 0;

    // get the first token
    token = strtok(buffer, " ");

    while (token != NULL) {
        // if the current token is not NULL, save this token
        argv[argc++] = token;

        // get the next token
        token = strtok(NULL, " ");
    }

    // make sure argv ends with NULL
    argv[argc] = NULL;
}


void get_socket_name(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    if (getsockname(sockfd, addr, addrlen) == -1) {
        perror("getsockname()");
        fprintf(stderr, "get socket name failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


size_t receive_from_socket(int sockfd, void *buf, size_t len, int flags,
                            struct sockaddr *src_addr, socklen_t *addrlen) {
    ssize_t ret;
    ret = recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (ret == -1) {
        perror("recvfrom()");
        fprintf(stderr, "cannot receive a packet; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    return ret;
}


size_t read_acl(FILE *fp, struct ACL *acl) {
    char cp[BUF_SIZE];
    size_t len;
    struct in_addr inp;
    unsigned int key;

    len = 0;

    // read the file until EOF
    while (fscanf(fp, "%s%u", cp, &key) == 2) {
        if (inet_aton((const char *) &cp, &inp) == -1)
            continue;

        if (len == ACL_SIZE)
            break;
        // store the network byte order IP address and its public key
        acl[len].ip_addr = inp.s_addr;
        acl[len].pubkey = key;
        len++;
    }

    return len;
}


int search_acl(in_addr_t ip, struct ACL *acl, size_t len) {
    int i;
    // traverse the list of IP address to find the public key
    for (i = 0; i < len; i++) {
        if (acl[i].ip_addr == ip)
            return i;
    }
    return -1;
}
