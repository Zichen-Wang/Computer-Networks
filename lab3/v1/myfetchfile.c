#include "mysocket.h"
#include "clientio.h"


int main(int argc, char *argv[]) {
    int socket_fd, file_fd;
    int tmp, i;
    in_addr_t server_ip_addr;
    in_port_t server_port;

    struct sockaddr_in server_addr;

    char control_message, *block_buf;
    struct timeval start, end;

    int ret;
    char hostname[MAX_HOSTNAME_SIZE], *username;
    char filename[MAX_FILENAME_SIZE];
    size_t len, filesize;

    double completion_time;

    if (argc != 4) {
        // if the number of arguments is less than 4, terminate
        fprintf(stderr, "usage: %s <filename> <srv-ip> <srv-port>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if ((tmp = inet_addr(argv[2])) == -1) {
        // if the format of given ipv4 address is error, terminate
        fprintf(stderr, "server dotted IPv4 address format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // assign the server ip address with network byte order
    server_ip_addr = (in_addr_t) tmp;

    if ((server_port = (in_port_t) atoi(argv[3])) == 0) {
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

    // server_ip_addr is already in network byte order
    server_addr.sin_addr.s_addr = server_ip_addr;

    // assign the server port number
    server_addr.sin_port = htons(server_port);

    // connect to the server
    connect_socket(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    // send the filename to the server
    write_to_socket(socket_fd, (const void *) argv[1], strlen(argv[1]) + 1);

    // read control message
    ret = read(socket_fd, (void *) &control_message, 1);

    if (ret == -1) {
        // read failed
        perror("read()");
        fprintf(stderr, "cannot read control message; terminating...\n");
        fflush(stderr);
        close(socket_fd);
        exit(1);
    } else if (ret == 0) {
        // server is closed without sending control message
        fprintf(stderr, "server is closed; cannot read control message; terminating...\n");
        fflush(stderr);
        close(socket_fd);
        exit(1);
    }

    if (control_message == '0') {
        // control message is '0'
        fprintf(stdout, "the requested file does not exist (or is inaccessible) on the server\n");
        fflush(stdout);
    } else if (control_message == '1') {
        // control message is '1'
        fprintf(stdout, "the requested file is empty\n");
        fflush(stdout);
    } else if (control_message == '2') {
        // control message is '2'
        fprintf(stdout, "start receiving the requested file\n\n");
        fflush(stdout);

        // recore the start time
        gettimeofday(&start, NULL);

        // get the machine name
        if (gethostname(hostname, MAX_HOSTNAME_SIZE) == -1) {
            perror("gethostname()");
            fprintf(stderr, "cannot get host name; terminating...\n");
            fflush(stderr);
            close(socket_fd);
            exit(1);
        }

        // get the user name
        username = getlogin();

        // make up the filename
        strcpy(filename, "/tmp/");
        len = 5;
        for (i = 0; argv[1][i] != '\0'; i++) {
            if (len == MAX_FILENAME_SIZE - 1)
                break;
            filename[len++] = argv[1][i];
        }

        for (i = 0; hostname[i] != '\0'; i++) {
            if (len == MAX_FILENAME_SIZE - 1)
                break;
            if (hostname[i] == '.')
                break;
            filename[len++] = hostname[i];
        }

        for (i = 0; username[i] != '\0'; i++) {
            if (len == MAX_FILENAME_SIZE - 1)
                break;
            filename[len++] = username[i];
        }

        // create a new file
        file_fd = open((const char *) filename, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (file_fd == -1) {
            // create failed
            perror("open()");
            fprintf(stderr, "cannot create a regular file; terminating...\n");
            fflush(stderr);
            close(socket_fd);
            exit(1);
        }

        // allocate a block buffer with size MAXBUFSZM
        block_buf = (char *) malloc(MAXBUFSZM);
        filesize = 0;
        // use loop to read from socket and write to local disk
        while ((ret = read(socket_fd, (void *) block_buf, MAXBUFSZM)) > 0) {
            write_to_file(file_fd, (const void *) block_buf, (size_t) ret);
            filesize += ret;
        }

        if (ret == -1) {
            // read from socket failed
            perror("read()");
            fprintf(stderr, "cannot read data from socket; terminating...\n");
            fflush(stderr);
            close(socket_fd);
            exit(1);
        }

        // record the end time
        gettimeofday(&end, NULL);

        completion_time = (end.tv_sec * 1000 + end.tv_usec / 1000.0) - (start.tv_sec * 1000 + start.tv_usec / 1000.0);

        fprintf(stdout, "completion time: %lf ms\n", completion_time);
        fprintf(stdout, "speed: %lf bps\n", 8 * filesize / (completion_time / 1000.0));
        fprintf(stdout, "file size: %ld bytes\n", filesize);

        close(file_fd);
    }

    close(socket_fd);
    return 0;
}
