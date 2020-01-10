#include "mysocket.h"
#include "serverio.h"


int main(int argc, char *argv[]) {
    int socket_fd, new_socket_fd, file_fd;
    in_port_t server_port;

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    size_t block_size;

    char pathname[MAX_FILE_NAME_SIZE];
    pid_t k;

    char control_message, *block_buf;
    int ret;

    if (argc != 3) {
        // if the number of arguments is not 2, terminate
        fprintf(stderr, "usage: %s <blocksize> <srv-port>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    // get the block_size
    block_size = atoi(argv[1]);

    //if blocksize exceeds MAXBUFSZM, set to MAXBUFSZM
    if (block_size > MAXBUFSZM)
        block_size = MAXBUFSZM;

    if ((server_port = (in_port_t) atoi(argv[2])) == 0) {
        // if the format of given port number is error, terminate
        fprintf(stderr, "server port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // create a socket file descriptor
    socket_fd = create_socket(AF_INET, SOCK_STREAM, 0);

    // initialize the server address data structure
    memset((void *) &server_addr, 0, sizeof(server_addr));

    // use the Internet protocol v4 addresses
    server_addr.sin_family = AF_INET;

    // server_ip_addr binds any IPv4 address
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);

    // assign the server port number
    server_addr.sin_port = htons(server_port);

    // bind the socket with server address data structure
    bind_socket(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    fprintf(stderr, "bind completed\n");
    fflush(stderr);

    // mark the server socket as a passive socket
    listen_socket(socket_fd, 5);

    while (1) {
        // accept a new client request
        memset((void *) &client_addr, 0, sizeof(client_addr));
        client_addr_len = sizeof(client_addr);
        new_socket_fd = accept_socket(socket_fd, (struct sockaddr *) &client_addr, &client_addr_len);

        fprintf(stderr, "\n[INFO] request received from client %s:%d\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));
        fflush(stderr);

        // fork a new child
        k = fork();

        if (k == 0) {
            // child code

            // close the server listening socket file descriptor
            close(socket_fd);

            // read the pathname from full duplex socket
            get_pathname(pathname, new_socket_fd);

            // try to open this file
            file_fd = open((const char *) pathname, O_RDONLY);

            if (file_fd == -1) {
                // open error; it might be "No such file or directory"
                perror("open()");
                fprintf(stderr, "the request to open file [%s] is failed\n", pathname);
                fflush(stderr);

                // set the control message to '0' and send it
                control_message = '0';
                write_to_socket(new_socket_fd, (const char *) &control_message, 1);
                close(new_socket_fd);
                return 0;
            }

            if (get_file_size((const char *) pathname) == 0) {
                // if the file is empty

                // set the control message to '1' and send it
                control_message = '1';
                write_to_socket(new_socket_fd, (const char *) &control_message, 1);
                close(new_socket_fd);
                return 0;
            }

            // otherwise

            // set the control message to '2' and send it
            control_message = '2';
            write_to_socket(new_socket_fd, (const char *) &control_message, 1);

            // allocate a buffer with size block_size
            block_buf = (char *) malloc(block_size);

            // read from local file and write to socket
            while ((ret = read(file_fd, (void *) block_buf, block_size)) > 0) {
                write_to_socket(new_socket_fd, (const void *) block_buf, (size_t) ret);
            }

            if (ret == -1) {
                perror("read()");
                fprintf(stderr, "read failed\n");
                fflush(stderr);
            }

            close(file_fd);
            close(new_socket_fd);

            free(block_buf);

            return 0;

        } else {
            // parent code

            // close the new accepted file descriptor
            close(new_socket_fd);
        }
    }

    close(socket_fd);
    return 0;
}
