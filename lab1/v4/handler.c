#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>

extern int counter;
extern char buf[];
extern int len;
extern const char *server_fifo_name;

extern void send_request_to_server(
    char buffer[],
    int length,
    const char *fifo_name
);


void retxreq(int sig) {
    // increase the counter by 1
    counter++;

    if (counter > 3) {
        printf("cannot get response from the server; terminating...\n");
        exit(1);
    }
    // set a new alarm
    alarm(2);

    // invoke a function to send a request
    send_request_to_server(buf, len, server_fifo_name);
}
