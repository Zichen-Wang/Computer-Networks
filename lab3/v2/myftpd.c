#include "myftpd.h"


int main(int argc, char *argv[]) {
    size_t file_size, ack_size;

    struct sockaddr_in server_addr;
    int tmp;

    suseconds_t curRTT, newRTT;

    in_addr_t client_ip_addr;
    in_port_t client_port;

    if (argc != 6) {
        // if the number of arguments is not 6, terminate
        fprintf(stderr, "usage: %s <file_size> <block_size> <initRTT> <cli-ip> <cli-port>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if ((tmp = inet_addr(argv[4])) == -1) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "client dotted IPv4 address format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // assign the client ip address with network byte order
    client_ip_addr = (in_addr_t) tmp;

    if ((client_port = (in_port_t) atoi(argv[5])) == 0) {
        // if the format of given port number is error, terminate
        fprintf(stderr, "client port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // get the numbers from the arguments
    file_size = (size_t) atoi(argv[1]);
    block_size = (size_t) atoi(argv[2]);
    curRTT = atoi(argv[3]);

    // create a socket file descriptor
    socket_fd = create_socket(AF_INET, SOCK_DGRAM, 0);

    // initialize the server address data structure
    memset((void *) &server_addr, 0, sizeof(server_addr));

    // use the Internet protocol v4 addresses
    server_addr.sin_family = AF_INET;

    // the ip address of the server can be any one
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // assign an ephemeral port number
    server_addr.sin_port = htons(0);

    // bind the socket with server address data structure
    bind_socket(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    // register an alarm signal handler
    if (signal(SIGALRM, retxreq) == SIG_ERR) {
        perror("signal()");
        fprintf(stderr, "cannot register an alarm signal handler; terminating...\n");
        fflush(stderr);
        close(socket_fd);
        exit(1);
    }

    // initialize the client address data structure
    memset((void *) &client_addr, 0, sizeof(client_addr));

    // use the Internet protocol v4 addresses
    client_addr.sin_family = AF_INET;

    // client_ip_addr is already in network byte order
    client_addr.sin_addr.s_addr = client_ip_addr;

    // assign the client port number
    client_addr.sin_port = htons(client_port);

    // reset the size of acknowledged data
    ack_size = 0;

    // reset the initial sequence number
    seq_num = 0;

    while (1) {
        // calculate the timeout
        timeout = (suseconds_t) (1.2 * curRTT);

        if (ack_size + block_size < file_size) {
            // this is not the last packet
            last_packet = 0;

            // send the packet and wait for ACK
            newRTT = send_packet_and_receive_ACK();

            // increase the size of acknowledged data
            ack_size += block_size;

            // reset the initial sequence number
            if (seq_num % 3 == 1)
                seq_num = 0;
            else
                seq_num = 1;

        } else {
            // this is the last packet
            last_packet = 1;

            // update the remaining block size
            block_size = file_size - ack_size;

            // reset the initial sequence number
            seq_num = 2;

            // send the packet and wait for ACK
            newRTT = send_packet_and_receive_ACK();

            // increase the size of acknowledged data
            ack_size += block_size;
        }

        if (RTTPRINT == 1) {
            fprintf(stdout, "newRTT: %lu microseconds\n", newRTT);
            fprintf(stdout, "curRTT(old): %lu microseconds\n", curRTT);

            curRTT = (suseconds_t) (RTT_weight * curRTT + (1 - RTT_weight) * newRTT);

            fprintf(stdout, "curRTT(updated): %lu microseconds\n", curRTT);
            fprintf(stdout, "\n");
            fflush(stdout);
        } else {
            curRTT = (suseconds_t) (RTT_weight * curRTT + (1 - RTT_weight) * newRTT);
        }

        if (last_packet == 1)
            break;
    }

    return 0;
}
