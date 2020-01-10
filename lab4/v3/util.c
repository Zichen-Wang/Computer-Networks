#include "terve.h"


void make_header(unsigned char *buf, unsigned int t) {
    // this function is to build a header of a packet
    int i;
    buf[0] = t;
    for (i = 0; i < 4; i++)
        buf[i + 1] = (rand_num >> (i << 3)) & 255;
}


unsigned int get_rand_num(unsigned char *buf) {
    // this function is to get the random number from the header
    int i;
    unsigned int recv_rand_num;
    recv_rand_num = 0;
    for (i = 0; i < 4; i++)
        recv_rand_num += buf[i + 1] << (i << 3);

    return recv_rand_num;
}


size_t read_stdin(char *read_buf) {
    // this function is to read characters from stdin

    // if the length exceeds BUF_SIZE, the exceeded characters will be dropped
    char ch;
    size_t len;
    int ret;

    len = 0;
    while (1) {
        ret = read(STDIN_FILENO, (void *) &ch, 1);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                break;
            }
        } else if (ret == 1) {
            if (ch == '\n')
                break;
            if (len == BUF_SIZE)
                continue;
            read_buf[len++] = ch;
        } else {
            break;
        }
    }

    read_buf[len] = '\0';
    return len;
}


int get_remote_addr(char *buf, in_addr_t *ip, in_port_t *port) {
    // this function is to analyze the IP address and port number from a buffer
    // return value: 0 success; -1 failure

    char cp[BUF_SIZE + 1];
    struct in_addr inp;

    *port = 0;

    sscanf(buf, "%s%hu", cp, port);

    if (*port == 0)
        return -1;

    if (inet_aton((const char *) &cp, &inp) == -1)
        return -1;

    *ip = inp.s_addr;

    return 0;
}
