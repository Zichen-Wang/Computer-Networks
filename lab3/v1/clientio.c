#include "clientio.h"


void write_to_file(int fd, const void *buf, size_t count) {
    // write buffer data to the local file
    if (write(fd, buf, count) == -1) {
        perror("write()");
        fprintf(stderr, "write to file failed; terminating...\n");
        fflush(stderr);
        exit(1);
    }
}
