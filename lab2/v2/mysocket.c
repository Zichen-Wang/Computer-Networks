#include "mysocket.h"


void parse(char *argv[], char *buffer) {
    char *token;
    int argc;

    // reset the arg counter
    argc = 0;

    // get the first token
    token = strtok(buffer, " ");

    while (token != NULL) {
        // if the current token is not NULL, save this token
        argv[argc++] = token;

        // get the next token
        token = strtok(NULL, " ");
    }

    // make sure argv ends with NULL
    argv[argc] = NULL;
}


void join(int argc, const char *argv[], char *buffer) {
    int i, len, arg_len;

    // initialize the length of buffer
    len = 0;
    buffer[0] = '\0';

    for (i = 3; i < argc; i++) {
        arg_len = strlen(argv[i]);
        // if too many arguments, stop copying
        if (len + arg_len + 1 > BUF_SIZE)
            break;
        // copy the argument into buffer
        memcpy(buffer + len, argv[i], arg_len);
        len += arg_len;
        // add a space at the end of each argument
        buffer[len++] = ' ';
    }

    // len is absolutely larger than 0, so modify the last space by a NULL character
    buffer[len - 1] = '\0';
}


int create_socket(int domain, int type, int protocol) {
    int socket_fd;
    socket_fd = socket(domain, type, protocol);

    if (socket_fd == -1) {
        perror("socket()");
        fprintf(stderr, "socket creation failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
    return socket_fd;
}


void bind_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (bind(sockfd, addr, addrlen) == -1) {
        perror("bind()");
        fprintf(stderr, "bind failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


void get_socket_name(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    if (getsockname(sockfd, addr, addrlen) == -1) {
        perror("getsockname()");
        fprintf(stderr, "get socket name failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


void listen_socket(int sockfd, int backlog) {
    if (listen(sockfd, backlog) == -1) {
        perror("listen()");
        fprintf(stderr, "listen failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


int accept_socket(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int new_fd;

    new_fd = accept(sockfd, addr, addrlen);
    if (new_fd == -1) {
        perror("accept()");
        fprintf(stderr, "accept failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    return new_fd;
}


void connect_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (connect(sockfd, addr, addrlen) == -1) {
        perror("connect()");
        fprintf(stderr, "connection failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}


char * read_from_socket(int fd) {
    // if no error, this function will always return an address of a string with '\0'
    // the size of the buffer address is dynamically allocated

    ssize_t ret;
    char byte, *buffer, *new_alloc;
    size_t len, max_size;

    len = 0;
    // initialize a buffer
    buffer = malloc(BUF_SIZE);
    max_size = BUF_SIZE;

    while (1) {
        // read one byte
        ret = read(fd, (void *) &byte, 1);

        if (ret == -1) {
            // read error;
            perror("read()");
            fprintf(stderr, "read failed; terminating...\n");
            fflush(stderr);
            exit(1);
        } else if (ret == 0) {
            // read an EOF
            buffer[len++] = '\0';
            break;
        } else if (ret == 1) {
            // read one byte
            buffer[len++] = byte;

            // encounter '\0'; stop reading
            if (byte == '\0')
                break;

            // if the buffer is full, reallocate for the next read
            if (len == max_size) {
                new_alloc = realloc(buffer, max_size + BUF_SIZE);

                if (new_alloc == NULL) {
                    // if allocating failed, terminate
                    free(buffer);
                    fprintf(stderr, "cannot allocate memory; terminating...\n");
                    fflush(stderr);
                    exit(1);
                }

                buffer = new_alloc;
                max_size += BUF_SIZE;
            }
        }
    }

    return buffer;
}


void write_to_socket(int fd, const void *buf, size_t count) {
    if (write(fd, buf, count) == -1) {
        perror("write()");
        fprintf(stderr, "write failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}