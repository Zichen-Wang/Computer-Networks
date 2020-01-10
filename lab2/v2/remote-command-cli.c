#include "mysocket.h"


int main(int argc, char *argv[]) {
    int socket_fd;
    int tmp;
    in_addr_t server_ip_addr;
    in_port_t server_port;

    struct sockaddr_in server_addr;

    char send_buf[BUF_SIZE], *recv;

    fd_set rfds;
    struct timeval tv;
    int retval, counter;

    char byte;

    if (argc < 4) {
        // if the number of arguments is less than 4, terminate
        fprintf(stderr, "usage: %s <server-dotted-ipv4-address> <port> <command> [<command arg> ...]\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if ((tmp = inet_addr(argv[1])) == -1) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "server dotted IPv4 address format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // join these arguments into the buffer
    join(argc, (const char **)argv, send_buf);

    // assign the server ip address with network byte order
    server_ip_addr = (in_addr_t) tmp;

    if ((server_port = (in_port_t) atoi(argv[2])) == 0) {
        // if the format of given port number is error, terminate
        fprintf(stderr, "server port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    counter = 0;
    while (1) {

        // create a socket file descriptor
        socket_fd = create_socket(AF_INET, SOCK_STREAM, 0);

        // initialize the server address data structure
        memset((void *) &server_addr, 0, sizeof(server_addr));

        // use the Internet protocol v4 addresses
        server_addr.sin_family = AF_INET;

        // server_ip_addr is already in network byte order
        server_addr.sin_addr.s_addr = server_ip_addr;

        // assign the server port number
        server_addr.sin_port = htons(server_port);

        // connect to the server
        connect_socket(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));

        // send a request to the server
        write_to_socket(socket_fd, (const void *) send_buf, strlen(send_buf) + 1);

        // increase the counter by 1
        counter++;

        // print the counter
        fprintf(stdout, "try %d:\n\n", counter);
        fflush(stdout);

        // set the read fds to be monitored
        FD_ZERO(&rfds);
        FD_SET(socket_fd, &rfds);

        // assign the timeout value
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        // use select to monitor the read fds
        retval = select(socket_fd + 1, &rfds, NULL, NULL, &tv);

        if (retval == -1) {
            // select failed
            perror("select()");
            fprintf(stderr, "select failed; terminating...\n");
            fflush(stderr);
            exit(1);
        } else if (retval) {
            // data available

            // read first byte
            tmp = read(socket_fd, (void *) &byte, 1);

            if (tmp == -1) {
                // read error
                perror("read()");
                fflush(stderr);
            } else if (tmp == 0) {
                // server is closed
                fprintf(stdout, "received EOF; server is closed\n\n");
                fflush(stdout);
            } else if (tmp == 1) {
                // data is available

                // read the remaining data from the socket
                recv = read_from_socket(socket_fd);

                // close the socket file descriptor
                close(socket_fd);

                // print the first byte
                putchar(byte);

                // print the remaining data
                fprintf(stdout, "%s", recv);

                // free the memory
                free(recv);

                // break the loop
                break;
            }
        } else {
            // timeout
            fprintf(stdout, "read timeout\n\n");
        }

        // close the socket file descriptor
        close(socket_fd);
        if (counter == 3) {
            fprintf(stdout, "cannot get response from the server\n");
            fflush(stdout);
            break;
        }
    }

    return 0;
}