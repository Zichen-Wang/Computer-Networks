#include "server.h"

int last_package;
int counter;
suseconds_t timeout;

int socket_fd;
char *send_buf;
size_t block_size;

struct sockaddr_in client_addr;


void set_a_timer(int which, const struct itimerval *new_value, struct itimerval *old_value) {
    if (setitimer(which, new_value, old_value) == -1) {
        perror("setitimer()");
        fprintf(stderr, "cannot set a timer; terminating...\n");
        fflush(stderr);
        close(socket_fd);
        exit(1);
    }
}


void send_package_and_set_timer() {
    // this function sends a block of data to the client and then set a timer

    struct itimerval tv;

    // if this is the last package, increase the counter by 1
    if (last_package == 1)
        counter++;

    // send the data to the client
    send_to_socket(socket_fd, (const void *) send_buf, block_size + 1, 0,
                   (const struct sockaddr *) &client_addr, sizeof(client_addr));

    // set a one-shot timer
    tv.it_interval.tv_sec = tv.it_interval.tv_usec = 0;
    tv.it_value.tv_sec = 0;
    tv.it_value.tv_usec = timeout;

    set_a_timer(ITIMER_REAL, (const struct itimerval *) &tv, NULL);
}


void retxreq(int sig) {
    // signal SIGALRM handler
    if (last_package == 1 && counter == 3) {
        // if this is the last package and the counter reaches 3, report and terminate.
        fprintf(stdout, "cannot receive the last ACK from the client\n");
        fflush(stdout);
        exit(1);
    }

    // retransmit
    send_package_and_set_timer();
}


void send_package_and_receive_ACK(char seq_num) {
    char recv;
    int ret;

    struct sockaddr_in dest_addr;
    socklen_t addrlen;

    struct itimerval tv;

    send_buf = (char *)malloc(block_size + 1);
    memset(send_buf + 1, '3', block_size);

    // set the sequence number in the first byte of the buffer
    send_buf[0] = seq_num;

    // send a package and set a new timer
    send_package_and_set_timer();

    while (1) {
        // stop and wait for ACK
        memset((void *) &dest_addr, 0, sizeof(dest_addr));
        addrlen = sizeof(dest_addr);
        ret = receive_from_socket(socket_fd, (void *) &recv, 1, 0,
                                  (struct sockaddr *) &dest_addr, &addrlen);

        if (ret == 1 && recv == seq_num) {
            // this is the expected ACK

            // cancel the timer
            tv.it_interval.tv_sec = tv.it_interval.tv_usec = 0;
            tv.it_value.tv_sec = tv.it_value.tv_usec = 0;
            set_a_timer(ITIMER_REAL, (const struct itimerval *) &tv, NULL);

            break;
        }
    }

    // free the memory
    free(send_buf);
}