#include <sys/time.h>
#include <signal.h>
#include "mysocket.h"


extern int last_package;
extern int counter;
extern suseconds_t timeout;

extern int socket_fd;
extern char *send_buf;
extern size_t block_size;

extern struct sockaddr_in client_addr;


extern void set_a_timer(int which, const struct itimerval *new_value, struct itimerval *old_value);
extern void send_package_and_receive_ACK(char seq_num);
extern void retxreq(int sig);
extern void send_package_and_set_timer();