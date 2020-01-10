#include "mysocket.h"


int main(int argc, char *argv[]) {
    int socket_fd, new_socket_fd;
    int tmp;
    in_addr_t server_ip_addr;

    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len, client_addr_len;

    char *recv, *client_argv[BUF_SIZE];

    pid_t k;

    // set a random seed
    srand(time(NULL));

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
    socket_fd = create_socket(AF_INET, SOCK_STREAM, 0);

    // initialize the server address data structure
    memset((void *) &server_addr, 0, sizeof(server_addr));

    // use the Internet protocol v4 addresses
    server_addr.sin_family = AF_INET;

    // server_ip_addr is in network byte order
    server_addr.sin_addr.s_addr = server_ip_addr;

    // assign an ephemeral port number
    server_addr.sin_port = htons(0);

    // bind the socket with server address data structure
    bind_socket(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    // update the server address data structure by getsockname()
    server_addr_len = sizeof(server_addr);
    get_socket_name(socket_fd, (struct sockaddr *) &server_addr, &server_addr_len);

    // write the port number to stderr
    fprintf(stderr, "bind completed\n");
    fprintf(stderr, "server port number: %d\n", ntohs(server_addr.sin_port));
    fflush(stderr);

    // mark the server socket as a passive socket
    listen_socket(socket_fd, 5);

    while (1) {
        // accept a new client request
        memset((void *) &client_addr, 0, sizeof(client_addr));
        client_addr_len = sizeof(client_addr);
        new_socket_fd = accept_socket(socket_fd, (struct sockaddr *) &client_addr, &client_addr_len);

        // read data from new socket file descriptor
        recv = read_from_socket(new_socket_fd);

        if (rand() % 2 == 0) {
            // ignore the request
            fprintf(stderr, "\n[INFO] ignored the request [%s] from client %s:%d\n\n",
                    recv,
                    inet_ntoa(client_addr.sin_addr),
                    ntohs(client_addr.sin_port));
            fflush(stderr);

            close(new_socket_fd);
            continue;
        }

        // parse the command line
        parse(client_argv, recv);

        // fork a new child
        k = fork();

        if (k == 0) {
            // child code

            // close the server socket file descriptor
            close(socket_fd);

            // redirect STDOUT to the new socket file descriptor
            dup2(new_socket_fd, STDOUT_FILENO);

            // close the duplicated new socket file descriptor,
            // because STDOUT has already been pointed to the new socket file
            close(new_socket_fd);
            if (execvp((const char *)client_argv[0], client_argv) == -1) {
                // if execution failed, terminate child
                exit(1);
            }
        } else {
            // parent code

            // close the new socket file descriptor
            close(new_socket_fd);

            // free the receive buffer
            free(recv);
        }

    }

    close(socket_fd);
    return 0;
}
