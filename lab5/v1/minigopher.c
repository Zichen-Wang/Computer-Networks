#include "minigopher.h"


int main(int argc, char *argv[]) {
    int i;
    int socket_fd;

    in_addr_t vpn_IP, server_IP;
    in_port_t vpn_port, server_port, transit_port;

    struct in_addr inp;

    struct sockaddr_in client_addr, vpn_addr, recv_addr;
    socklen_t recv_addr_len;

    unsigned char send_buf[MAXBUFSIZE], recv_buf[MAXBUFSIZE];
    size_t recv_len;

    if (argc != 5) {
        // check the number of arguments
        fprintf(stderr, "usage: %s <vpn-IP> <vpn-port> <server-IP> <server-port-number>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if (inet_aton((const char *) argv[1], &inp) == 0) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "vpn dotted IPv4 address format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // get the vpn-IP
    vpn_IP = inp.s_addr;

    if ((vpn_port = (in_port_t) atoi(argv[2])) == 0) {
        // if the format of given port number is error, terminate
        fprintf(stderr, "vpn port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // convert the vpn-port to network representation
    vpn_port = htons(vpn_port);

    if (inet_aton((const char *) argv[3], &inp) == 0) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "server dotted IPv4 address format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // get the server-IP
    server_IP = inp.s_addr;

    if ((server_port = (in_port_t) atoi(argv[4])) == 0) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "server port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // convert the server-port to network representation
    server_port = htons(server_port);

    // initialize a UDP socket file descriptor that binds any address and any port
    socket_fd = initialize_udp_sockfd(0, &client_addr, sizeof(client_addr));

    // send a vpn request consisting of server-IP and server-port
    for (i = 0; i < 4; i++)
        send_buf[i] = (server_IP >> (i << 3)) & 255;

    send_buf[4] = server_port & 255;
    send_buf[5] = (server_port >> 8) & 255;

    memset((void *) &vpn_addr, 0, sizeof(vpn_addr));
    vpn_addr.sin_family = AF_INET;
    vpn_addr.sin_addr.s_addr = vpn_IP;
    vpn_addr.sin_port = vpn_port;

    send_to_socket(socket_fd, (const void *) &send_buf, VPNREQSIZE, 0,
            (const struct sockaddr *) &vpn_addr, sizeof(vpn_addr));

    // ready for receive the response
    memset((void *) &recv_addr, 0, sizeof(recv_addr));
    recv_addr_len = sizeof(recv_addr);

    recv_len = receive_from_socket(socket_fd, (void *) recv_buf, MAXBUFSIZE, 0,
            (struct sockaddr *) &recv_addr, &recv_addr_len);

    if (recv_len == VPNACKSIZE && recv_buf[0] == VPNACKANS) {
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