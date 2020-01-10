#include "client.h"


unsigned int decode_called_times;


char mydecode(char x, unsigned int prikey) {
    char ret;
    ret = x ^ ((prikey >> (decode_called_times << 3)) & 255);
    decode_called_times = (decode_called_times + 1) & 3;

    return ret;
}


size_t join(int argc, const char *argv[], unsigned char *buffer) {
    int i, j;
    size_t len, arg_len;

    // initialize the length of buffer
    len = 0;

    for (i = 4; i < argc; i++) {
        arg_len = strlen(argv[i]);
        // if too many arguments, stop copying

        for (j = 0; j < arg_len; j++) {
            if (len == BUF_SIZE)
                return len;
            buffer[len++] = argv[i][j];
        }
        // add a space at the end of each argument
        if (i < argc - 1 && len < BUF_SIZE)
            buffer[len++] = ' ';
    }

    return len;
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


in_addr_t get_local_ip_addr() {
    char name[HOSTNAME_SIZE];
    struct addrinfo *result;

    if (gethostname(name, sizeof(name))) {
        perror("gethostname(): invalid hostname");
        fflush(stderr);
        exit(1);
    }

    if (getaddrinfo(name, NULL, NULL, &result)) {
        perror("getaddrinfo(): invalid hostname");
        fflush(stderr);
        exit(1);
    }

    return (((struct sockaddr_in *) result -> ai_addr) -> sin_addr).s_addr;
}