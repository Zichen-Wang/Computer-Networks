#include "mysocket.h"


int main(int argc, char *argv[]) {
    int socket_fd;
    int tmp;
    in_addr_t server_ip_addr;
    in_port_t server_port;

    struct sockaddr_in server_addr, client_addr;

    char send_buf[BUF_SIZE];

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

    // create a socket file descriptor
    socket_fd = create_socket(AF_INET, SOCK_DGRAM, 0);

    // initialize the client address data structure
    memset((void *) &client_addr, 0, sizeof(client_addr));

    // use the Internet protocol v4 addresses
    client_addr.sin_family = AF_INET;

    // the ip address of the client can be any one
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // assign an ephemeral port number
    client_addr.sin_port = htons(0);

    // bind the socket with client address data structure
    bind_socket(socket_fd, (const struct sockaddr *) &client_addr, sizeof(client_addr));

    // initialize the server address data structure
    memset((void *) &server_addr, 0, sizeof(server_addr));

    // use the Internet protocol v4 addresses
    server_addr.sin_family = AF_INET;

    // server_ip_addr is already in network byte order
    server_addr.sin_addr.s_addr = server_ip_addr;

    // assign the server port number
    server_addr.sin_port = htons(server_port);

    // send a message to the server address
    send_to_socket(socket_fd, (const void *) send_buf, strlen(send_buf) + 1, 0,
            (const struct sockaddr *) &server_addr, sizeof(server_addr));

    // close the socket file descriptor
    close(socket_fd);
    return 0;
}