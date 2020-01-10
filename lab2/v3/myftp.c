#include "client.h"


int main(int argc, char *argv[]) {
    int socket_fd;
    in_addr_t client_ip_addr;
    int tmp, i;

    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len, client_addr_len;

    int dropwhen;
    size_t num_of_recv_packages, duplicate_packages, duplicate_bytes;

    char *recv_buf, *file_content;
    size_t file_size, cur_max_size;

    char *new_alloc;

    ssize_t len;
    char exp_seq_num, recv_seq_num;

    int first_package_received, last_package_received;
    struct timeval start, end;
    double completion_time;

    if (argc != 3) {
        // if the number of arguments is not 3, terminate
        fprintf(stderr, "usage: %s <cli-ip> <dropwhen>\n", argv[0]);
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
    client_ip_addr = (in_addr_t) tmp;

    dropwhen = atoi(argv[2]);

    // create a socket file descriptor
    socket_fd = create_socket(AF_INET, SOCK_DGRAM, 0);

    // initialize the client address data structure
    memset((void *) &client_addr, 0, sizeof(client_addr));

    // use the Internet protocol v4 addresses
    client_addr.sin_family = AF_INET;

    // client_ip_addr is already in network byte order
    client_addr.sin_addr.s_addr = client_ip_addr;

    // assign an ephemeral port number
    client_addr.sin_port = htons(0);

    // bind the socket with client address data structure
    bind_socket(socket_fd, (const struct sockaddr *) &client_addr, sizeof(client_addr));

    // update the server address data structure by getsockname()
    client_addr_len = sizeof(client_addr);
    get_socket_name(socket_fd, (struct sockaddr *) &client_addr, &client_addr_len);

    // write the port number to stderr
    fprintf(stdout, "bind completed\n");
    fprintf(stdout, "client port number: %d\n", ntohs(client_addr.sin_port));
    fflush(stdout);

    // reset these parameters
    file_content = malloc(FILE_SIZE_INC);
    cur_max_size = FILE_SIZE_INC;
    file_size = 0;
    exp_seq_num = 0;

    first_package_received = last_package_received = 0;

    num_of_recv_packages = 0;
    duplicate_bytes = 0;
    duplicate_packages = 0;

    while (1) {
        // malloc a new buffer
        recv_buf = (char *) malloc(MAX_BUF_SIZE);

        memset((void *) &server_addr, 0, sizeof(server_addr));
        server_addr_len = sizeof(server_addr);
        // wait for receiving a package
        len = receive_from_socket(socket_fd, (void *) recv_buf, MAX_BUF_SIZE, 0,
                (struct sockaddr *) &server_addr, &server_addr_len);

        if (first_package_received == 0) {
            // if this is the first received package, record the start time
            first_package_received = 1;
            gettimeofday(&start, NULL);
        }

        // get the received sequence number
        recv_seq_num = recv_buf[0];

        // increase the number of received packages
        num_of_recv_packages++;

        if ((recv_seq_num == exp_seq_num) || (recv_seq_num == 2 && last_package_received == 0)) {
            // if this is the expected sequence number
            // or this is the first time receiving the last package

            if (file_size + len - 1 > cur_max_size) {
                // increase the size of file content
                new_alloc = realloc(file_content, cur_max_size + FILE_SIZE_INC);
                if (new_alloc == NULL) {
                    // if allocating failed, terminate
                    free(file_content);
                    fprintf(stderr, "cannot allocate memory; terminating...\n");
                    fflush(stderr);
                    exit(1);
                }

                file_content = new_alloc;
                cur_max_size += FILE_SIZE_INC;
            }

            // copy bytes to the local file content array
            for (i = 1; i < len; i++)
                file_content[file_size++] = recv_buf[i];

            if (recv_seq_num == 2) {
                // mark the last package received
                last_package_received = 1;
                gettimeofday(&end, NULL);
                // do not expect any package
                exp_seq_num = 255;
            } else {
                // flip the expected sequence number
                exp_seq_num ^= 1;
            }


            // if we decide to send an ACK
            if (dropwhen == -1 || num_of_recv_packages % dropwhen != 0) {
                // transmit an ACK
                send_to_socket(socket_fd, (const void *) &recv_seq_num, 1, 0,
                               (const struct sockaddr *) &server_addr, server_addr_len);
                if (recv_seq_num == 2) {
                    // if this is the last transmitted ACK, break
                    free(recv_buf);
                    close(socket_fd);
                    break;
                }
            }
        } else {
            // received a duplicate package

            duplicate_packages++;
            duplicate_bytes += len - 1;

            // if we decide to send an ACK
            if (dropwhen == -1 || num_of_recv_packages % dropwhen != 0) {
                // transmit an ACK
                send_to_socket(socket_fd, (const void *) &recv_seq_num, 1, 0,
                        (const struct sockaddr *) &server_addr, server_addr_len);
                if (recv_seq_num == 2) {
                    // if this is the last transmitted ACK, break
                    free(recv_buf);
                    close(socket_fd);
                    break;
                }
            }
        }

        // free the receive buffer, and wait for the next package
        free(recv_buf);
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "total %lu data bytes received\n", file_size);
    fprintf(stdout, "%lu duplicate bytes; %lu duplicate packages\n", duplicate_bytes, duplicate_packages);

    completion_time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000.0;

    fprintf(stdout, "completion time: %.3lf ms\n", completion_time);
    fprintf(stdout, "speed: %.3lf bps\n", 8 * file_size / (completion_time / 1000.0));

    fflush(stdout);

    return 0;
}