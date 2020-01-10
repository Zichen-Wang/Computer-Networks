#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>


void send_request_to_server(char buffer[], int length, const char *fifo_name) {
    int fifo_fd;

    // open the server FIFO with write only permission
    // if it exists, block until the server opens it with read only permission
    fifo_fd = open(fifo_name, O_WRONLY);
    
    if (fifo_fd == -1) {
        // if it does not exist, terminate
        printf("cannot open the server FIFO\n");
        exit(1);
    }
    
    // write the data into the server FIFO
    if (write(fifo_fd, (const void *) buffer, length) == -1) {
        perror("write()");
        fflush(stderr);
        exit(1);
    }
    
    // close the server FIFO file descriptor
    close(fifo_fd);
}
