#include "mysocket.h"


int main(int argc, char *argv[]) {
    int socket_fd;
    int tmp;
    in_addr_t server_ip_addr;

    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len, client_addr_len;

    char recv[BUF_SIZE], *client_argv[BUF_SIZE];
    int len;

    pid_t k;

    if (argc != 2) {
        // if the number of arguments is not 2, terminate
        fprintf(stderr, "usage: %s <dotted-ipv4-address>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if ((tmp = inet_addr(argv[1])) == -1) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "dotted IPv4 address format error; terminating\n");
        fflush(stderr);
        exit(1);
    }

    // assign the server ip address with network byte order
    server_ip_addr = (in_addr_t) tmp;

    // create a socket file descriptor
    socket_fd = create_socket(AF_INET, SOCK_DGRAM, 0);

    // initialize the server address data structure
    memset((void *) &server_addr, 0, sizeof(server_addr));

    // use the Internet protocol v4 addresses
    server_addr.sin_family = AF_INET;

    // server_ip_addr is already in network byte order
    server_addr.sin_addr.s_addr = server_ip_addr;

    // assign an ephemeral port number
    server_addr.sin_port = htons(0);

    // bind the socket with server address data structure
    bind_socket(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    // update the server address data structure by getsockname()
    server_addr_len = sizeof(server_addr);
    getsockname(socket_fd, (struct sockaddr *) &server_addr, &server_addr_len);

    // write the port number to stderr
    fprintf(stderr, "bind completed\n");
    fprintf(stderr, "server port number: %d\n", ntohs(server_addr.sin_port));
    fflush(stderr);

    while (1) {
        // be ready to receive data from a client
        memset((void *) &client_addr, 0, sizeof(client_addr));
        client_addr_len = sizeof(client_addr);
        len = receive_from_socket(socket_fd, (void *) recv, BUF_SIZE, 0,
                (struct sockaddr *) &client_addr, &client_addr_len);

        // display info to indicate the server got the message
        fprintf(stderr, "\n[INFO] message [%s] received from client %s:%d\n\n",
                recv,
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));
        fflush(stderr);

        if (len == 0)
            continue;

        // parse the command line
        recv[len - 1] = '\0';
        parse(client_argv, recv);

        // fork a new child
        k = fork();

        if (k == 0) {
            // child code

            // close the server socket file descriptor
            close(socket_fd);
            if (execvp((const char *)client_argv[0], client_argv) == -1) {
                // if execution failed, terminate child
                exit(1);
            }
        } else {
            // parent code
            // do nothing and wait for the next message
        }
    }

    close(socket_fd);
    return 0;
}
