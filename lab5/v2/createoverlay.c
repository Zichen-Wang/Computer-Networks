#include "createoverlay.h"


int main(int argc, char *argv[]) {
    int i, j;
    int socket_fd;

    in_addr_t router1_IP;
    in_port_t router1_port, router_port, transit_port;

    struct in_addr inp;

    struct sockaddr_in client_addr, overlay_addr, recv_addr;
    socklen_t recv_addr_len;

    unsigned char send_buf[MAXBUFSIZE], recv_buf[MAXBUFSIZE];
    size_t send_len, recv_len;

    if (!(argc >= 5 && argc % 2 == 1)) {
        // check the number of arguments
        fprintf(stderr, "usage: %s <router1-IP> <router1-port> <router2-IP> <router2-port> ... "
                        "<routerk-IP> <routerk-port> <dst-IP> <dst-port>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if ((argc - 1) / 2 - 1 >= 256) {
        // check k
        fprintf(stderr, "at most 255 routers are allowed; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    send_len = 0;
    send_buf[send_len++] = (argc - 1) / 2 - 1;
    for (i = 1; i < argc; i += 2) {
        if (inet_aton((const char *) argv[i], &inp) == 0) {
            // if the format of given ipv4 address is error, terminate
            fprintf(stderr, "overlay dotted IPv4 address format error; terminating...\n");
            fflush(stderr);
            exit(1);
        }

        if (i == 1)
            router1_IP = inp.s_addr;

        send_buf[send_len++] = '#';

        for (j = 0; j < 4; j++)
            send_buf[send_len++] = (inp.s_addr >> (j << 3)) & 255;

        if ((router_port = (in_port_t) atoi(argv[i + 1])) == 0) {
            // if the format of given port number is error, terminate
            fprintf(stderr, "overlay port number format error; terminating...\n");
            fflush(stderr);
            exit(1);
        }

        router_port = htons(router_port);

        if (i == 1)
            router1_port = router_port;

        send_buf[send_len++] = '#';
        send_buf[send_len++] = router_port & 255;
        send_buf[send_len++] = (router_port >> 8) & 255;

    }

    // initialize a UDP socket file descriptor that binds any address and any port
    socket_fd = initialize_udp_sockfd(0, &client_addr, sizeof(client_addr));

    memset((void *) &overlay_addr, 0, sizeof(overlay_addr));
    overlay_addr.sin_family = AF_INET;
    overlay_addr.sin_addr.s_addr = router1_IP;
    overlay_addr.sin_port = router1_port;

    send_to_socket(socket_fd, (const void *) &send_buf, send_len, 0,
                   (const struct sockaddr *) &overlay_addr, sizeof(overlay_addr));


    // ready for receive the response
    memset((void *) &recv_addr, 0, sizeof(recv_addr));
    recv_addr_len = sizeof(recv_addr);

    recv_len = receive_from_socket(socket_fd, (void *) recv_buf, MAXBUFSIZE, 0,
                                   (struct sockaddr *) &recv_addr, &recv_addr_len);

    if (recv_len == OVERLAYACKSIZE && recv_buf[0] == OVERLAYACKANS) {
        // if the payload of received UDP packet contains ANS 3, print the transit-port
        transit_port = recv_buf[1] + (recv_buf[2] << 8);
        fprintf(stdout, "transit-port: %hu\n", ntohs(transit_port));
        fflush(stdout);
    } else {
        // otherwise, print an error message
        fprintf(stdout, "the request has been rejected\n");
        fflush(stdout);
    }

    close(socket_fd);
    return 0;
}