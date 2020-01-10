#include "server.h"


int main(int argc, char *argv[]) {
    int socket_fd;
    int i;
    struct in_addr inp;
    in_addr_t server_ip_addr, recv_ip_addr;

    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len, client_addr_len;

    unsigned char recv[IP_SIZE + BUF_SIZE];
    char command_buf[BUF_SIZE + 1], *client_argv[BUF_SIZE + 1];

    size_t len;

    pid_t k;

    FILE *fp;
    struct ACL acl[ACL_SIZE];
    size_t acl_len;
    int acl_idx;
    unsigned int pubkey;

    if (argc != 2) {
        // if the number of arguments is not 2, terminate
        fprintf(stderr, "usage: %s <dotted-ipv4-address>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if (inet_aton((const char *) argv[1], &inp) == 0) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "dotted IPv4 address format error; terminating\n");
        fflush(stderr);
        exit(1);
    }

    // assign the server IP address with network byte order
    server_ip_addr = inp.s_addr;

    if ((fp = fopen("acl.txt", "r")) == NULL) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "cannot find ACL file acl.txt; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    acl_len = read_acl(fp, acl);
    fclose(fp);


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

        // receive a packet from the client
        len = receive_from_socket(socket_fd, (void *) recv, IP_SIZE + BUF_SIZE, 0,
                (struct sockaddr *) &client_addr, &client_addr_len);

        if ((acl_idx = search_acl(client_addr.sin_addr.s_addr, acl, acl_len)) == -1) {
            // if the client IP address does not exist in acl.txt, drop the request
            fprintf(stderr, "\n\n[ERROR] cannot find the public key of client %s\n\n",
                    inet_ntoa(client_addr.sin_addr));
            fflush(stderr);
            continue;
        }

        // get the public key
        pubkey = acl[acl_idx].pubkey;

        // reset the encode counter
        encode_called_times = 0;

        // call myencode() to decipher the certificate
        for (i = 0; i < len; i++)
            recv[i] = myencode(recv[i], pubkey);

        // calculate the IP address from the certificate
        recv_ip_addr = 0;
        for (i = 0; i < IP_SIZE; i++)
            recv_ip_addr += recv[i] << (i << 3);

        if (recv_ip_addr != client_addr.sin_addr.s_addr) {
            // the IP address is not matched, which means the certificate has been corrupted.
            fprintf(stderr, "\n\n[ERROR] cannot verify the certificate from client %s\n\n",
                    inet_ntoa(client_addr.sin_addr));
            fflush(stderr);
            continue;
        }

        // retrieve the command into a buffer
        len -= IP_SIZE;
        for (i = 0; i < len; i++)
            command_buf[i] = (char) recv[i + IP_SIZE];

        command_buf[len] = '\0';

        fprintf(stderr, "\n\n[INFO] command [%s] received from client %s\n\n",
                command_buf, inet_ntoa(client_addr.sin_addr));
        fflush(stderr);

        // split the command
        parse(client_argv, command_buf);

        // fork a new child
        k = fork();

        if (k == 0) {
            // child code

            // close the server socket file descriptor
            close(socket_fd);

            if (execvp((const char *) client_argv[0], client_argv) == -1) {
                // if execution failed, terminate child
                exit(1);
            }
        }
    }

    close(socket_fd);
    return 0;
}
