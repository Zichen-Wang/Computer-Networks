#include "client.h"


int main(int argc, char *argv[]) {
    int i;
    int socket_fd;
    unsigned int prikey;

    struct in_addr inp;

    in_addr_t server_ip_addr, client_ip_addr;
    in_port_t server_port;

    struct sockaddr_in server_addr, client_addr;

    unsigned char command[BUF_SIZE], send_buf[IP_SIZE + BUF_SIZE];
    size_t len;

    if (argc < 5) {
        // if the number of arguments is less than 4, terminate
        fprintf(stderr, "usage: %s <server-dotted-ipv4-address> <port> <prikey> <command> [<command arg> ...]\n",
                argv[0]);
        fflush(stderr);
        exit(1);
    }

    if (inet_aton((const char *) argv[1], &inp) == 0) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "server dotted IPv4 address format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // assign the server IP address with network byte order
    server_ip_addr = inp.s_addr;

    if ((server_port = (in_port_t) atoi(argv[2])) == 0) {
        // if the format of given port number is error, terminate
        fprintf(stderr, "server port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    prikey = atoi(argv[3]);

    // join these arguments into the buffer
    len = join(argc, (const char **) argv, command);

    // create a socket file descriptor
    socket_fd = create_socket(AF_INET, SOCK_DGRAM, 0);

    // initialize the client address data structure
    memset((void *) &client_addr, 0, sizeof(client_addr));

    // use the Internet protocol v4 addresses
    client_addr.sin_family = AF_INET;

    // the IP address of the client can be any one
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

    // put the client's IP address before the command in the plaintext
    client_ip_addr = get_local_ip_addr();
    for (i = 0; i < IP_SIZE; i++)
        send_buf[i] = (client_ip_addr >> (i << 3)) & 255;

    // put the command in the plaintext following the 4 bytes IP address
    for (i = 0; i < len; i++)
        send_buf[i + IP_SIZE] = command[i];

    len += IP_SIZE;

    // call mydecode() to make a certificate
    decode_called_times = 0;
    for (i = 0; i < len; i++)
        send_buf[i] = mydecode(send_buf[i], prikey);

    // send a message to the server address
    send_to_socket(socket_fd, (const void *) send_buf, len, 0,
            (const struct sockaddr *) &server_addr, sizeof(server_addr));

    // close the socket file descriptor
    close(socket_fd);
    return 0;
}