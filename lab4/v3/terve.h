#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>


#define BUF_SIZE        50
#define HEADER_SIZE     5

#define HEADER_INIT     5
#define HEADER_ACK      6
#define HEADER_REJ      7
#define HEADER_MSG      8
#define HEADER_TRM      9
#define HEADER_HRBT     10

#define INITIATION_STATE    0
#define DECISION_STATE      1
#define REQUEST_SENT_STATE  2
#define MESSAGE_STATE       3

#define HEARTBEAT_INTERVAL  5


extern unsigned int state;

extern sigjmp_buf begin;
extern int socket_fd;


extern struct sockaddr_in remote_addr;

extern unsigned int re_init_cnt, heartbeat_cnt;

extern unsigned int rand_num;


extern void send_init_request();

extern void send_heartbeat();

extern void terve_alarm(int sig);

extern void terve_quit(int sig);

extern void terve_msg_receive(int sig);

extern void make_header(unsigned char *buf, unsigned int t);

extern unsigned int get_rand_num(unsigned char *buf);

extern size_t read_stdin(char *read_buf);

extern int get_remote_addr(char *buf, in_addr_t *ip, in_port_t *port);

extern int create_socket(int domain, int type, int protocol);

extern void bind_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

extern void send_to_socket(
        int sockfd,
        const void *buf,
        size_t len,
        int flags,
        const struct sockaddr *dest_addr, socklen_t addrlen);

extern ssize_t receive_from_socket(
        int sockfd,
        void *buf,
        size_t len,
        int flags,
        struct sockaddr *src_addr,
        socklen_t *addrlen);
