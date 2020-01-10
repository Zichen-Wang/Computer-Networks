#include "serverio.h"


void get_pathname(char *pathname, int fd) {
    // get the pathname from socket file descriptor
    size_t len;
    char byte;
    int ret;

    // set the prefix path
    strcpy(pathname, "/tmp/");
    len = 5;

    // read byte by byte
    while ((ret = read(fd, (void *) &byte, 1)) > 0) {
        if (byte == '\0' || len == MAX_FILE_NAME_SIZE - 1)
            break;

        pathname[len++] = byte;
    }

    if (ret == -1) {
        perror("read()");
        fprintf(stderr, "read failed\n");
        fflush(stderr);
    }

    // set '\0' at the end
    pathname[len] = '\0';
}


off_t get_file_size(const char *pathname) {
    // get the file size by stat()
    struct stat st;
    if (stat(pathname, &st) == -1) {
        perror("stat()");
        fprintf(stderr, "cannot access file information; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    return st.st_size;
}
