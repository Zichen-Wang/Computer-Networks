#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>


int counter;
char buf[100];
int len;
const char *server_fifo_name = "server_queue";


extern void retxreq(int sig);

extern void send_request_to_server(
    char buffer[],
    int len,
    const char *fifo_name
);

extern void get_response_from_server(const char *fifo_name);


int main(void) {
    const char *client_fifo_name = "client_queue";
    
    // create a client FIFO
    mkfifo(client_fifo_name, 0600);
    
    // register an alarm signal handler
    signal(SIGALRM, retxreq);

    // get the data from stdin
    fgets(buf, 100, stdin);

    len = strlen(buf);
    
    // get rid of newline character at the end of string
    buf[len - 1] = '\0';

    // reset the counter to 1
    counter = 1;

    // set an alarm
    alarm(2);

    // invoke a function to send a request
    send_request_to_server(buf, len, server_fifo_name);
    
    // invoke a function to get a response
    get_response_from_server(client_fifo_name);

    return 0;
}
