#include <sys/time.h>
#include <signal.h>
#include "mysocket.h"


#define RTT_weight 0.7
#define RTTPRINT 1

#define MAXTRY 80


extern int last_packet;
extern suseconds_t timeout;

extern int socket_fd;
extern size_t block_size;

extern unsigned char seq_num;

extern struct sockaddr_in client_addr;

extern void set_a_timer(int which, const struct itimerval *new_value, struct itimerval *old_value);
extern suseconds_t send_packet_and_receive_ACK();
extern void retxreq(int sig);
