/*
 * There is no specific character at the end of the output for legacy apps.
 * However, if no process has the FIFO open for writing, read() will return 0
 * to indicate end of file. Therefore, the program may able to keep reading
 * until nread (the return value of read()) is 0, which means it encounters 
 * the end of file. More details would be in Lab1Answers.pdf.
 */

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
        printf("cannot open the server FIFO; terminating...\n");
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
    
    for (i = 0; i < response_len; i++)
        putchar(response[i]);

    // free the array for receiving the response
    free(response);
}


int main(void) {
    const char *server_fifo_name = "server_queue";
    const char *client_fifo_name = "client_queue";
    char buf[100];
    int len;

    // create a client FIFO
    mkfifo(client_fifo_name, 0600);

    // get the data from stdin
    fgets(buf, 100, stdin);

    len = strlen(buf);
    
    // get rid of newline character at the end of string
    buf[len - 1] = '\0';
    
    // invoke a function to send a request
    send_request_to_server(buf, len, server_fifo_name);
    
    // invoke a function to get a response
    get_response_from_server(client_fifo_name);

    return 0;
}
