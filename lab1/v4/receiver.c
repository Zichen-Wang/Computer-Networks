#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>


void get_response_from_server(const char *fifo_name) {
    int fifo_fd;
    char buffer[10000];

    ssize_t nread;
    char *response;
    char *new_alloc;
    int i, response_len;

    // open the client FIFO with read only permission
    // it will block until the server's child process opens this FIFO with 
    // write only permission
    fifo_fd = open(fifo_name, O_RDONLY);

    if (fifo_fd == -1) {
        // if failed, terminate
        printf("cannot open the client FIFO; terminating...");
        exit(1);
    }
    
    // cancel the alarm
    alarm(0);

    response = NULL;
    response_len = 0;
    while ((nread = read(fifo_fd, (void *)buffer, 10000)) > 0) {
        // keep reading from the FIFO until EOF

        // increase the array for receiving the response
        new_alloc = realloc(response, response_len + nread);

        if (new_alloc == NULL) {
            // if allocating failed, terminate
            free(response);
            printf("cannot allocate memory; terminating...");
            exit(1);
        }

        response = new_alloc;
        // copy bytes to the array
        for (i = 0; i < nread; i++)
            response[response_len++] = buffer[i];
    }
    
    // output the response
    for (i = 0; i < response_len; i++)
        putchar(response[i]);

    // free the array for receiving the response
    free(response);
}
