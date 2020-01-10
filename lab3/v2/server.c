#include "myftpd.h"


int last_packet;
int counter;
suseconds_t timeout;

int socket_fd;
unsigned char *send_buf;
size_t block_size;

unsigned char seq_num;

struct sockaddr_in client_addr;

struct timeval start[MAXTRY];

void set_a_timer(int which, const struct itimerval *new_value, struct itimerval *old_value) {
    if (setitimer(which, new_value, old_value) == -1) {
        perror("setitimer()");
        fprintf(stderr, "cannot set a timer; terminating...\n");
        fflush(stderr);
        close(socket_fd);
        exit(1);
    }
}


void send_packet_and_set_timer() {
    // this function sends a block of data to the client and then set a timer

    struct itimerval tv;

    // if this is the last packet, increase the counter by 1
    if (last_packet == 1)
        counter++;

    // set the sequence number in the first byte of the buffer
    send_buf[0] = seq_num;

    // save the start time of this packet
    if (gettimeofday(&start[seq_num / 3], NULL) == -1) {
        perror("gettimeofday()");
        fprintf(stderr, "cannot get the time; terminating...\n");
        exit(1);
    }

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
    if (last_packet == 1 && counter == 3) {
        // if this is the last packet and the counter reaches 3, report and terminate.
        fprintf(stdout, "cannot receive the last ACK from the client\n");
        fflush(stdout);
        exit(1);
    }

    // set the next sequence number which is in the same group
    seq_num += 3;

    if (seq_num / 3 == MAXTRY) {
        fprintf(stderr, "too many retries; cannot get ACK; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // retransmit
    send_packet_and_set_timer();
}


suseconds_t send_packet_and_receive_ACK() {
    unsigned char recv;
    int ret;

    struct sockaddr_in dest_addr;
    socklen_t addrlen;

    struct itimerval tv;
    struct timeval end;

    // allocate a buffer and fill it by '3'
    // we will reset the sequence number each time before sending
    send_buf = (unsigned char *) malloc(block_size + 1);
    memset(send_buf + 1, '3', block_size);

    // send a packet and set a new timer
    send_packet_and_set_timer();

    while (1) {
        // stop and wait for ACK
        memset((void *) &dest_addr, 0, sizeof(dest_addr));
        addrlen = sizeof(dest_addr);
        ret = receive_from_socket(socket_fd, (void *) &recv, 1, 0,
                                  (struct sockaddr *) &dest_addr, &addrlen);

        if (ret == 1 && recv % 3 == seq_num % 3) {
            // this is one of expected ACKs

            // cancel the timer
            tv.it_interval.tv_sec = tv.it_interval.tv_usec = 0;
            tv.it_value.tv_sec = tv.it_value.tv_usec = 0;
            set_a_timer(ITIMER_REAL, (const struct itimerval *) &tv, NULL);

            if (gettimeofday(&end, NULL) == -1) {
                perror("gettimeofday()");
                fprintf(stderr, "cannot get the time; terminating...\n");
                fflush(stderr);
                exit(1);
            }

            break;
        }
    }

    // free the memory
    free(send_buf);

    // (recv / 3) is the index in the array
    return (end.tv_sec * 1000000 + end.tv_usec) - (start[recv / 3].tv_sec * 1000000 + start[recv / 3].tv_usec);
}
