#include "terve.h"


unsigned int state;

sigjmp_buf begin;
int socket_fd;

struct sockaddr_in remote_addr;

unsigned int re_init_cnt, heartbeat_cnt;

unsigned int rand_num;


int main(int argc, char *argv[]) {
    int i;
    in_port_t local_port;

    struct sockaddr_in local_addr;

    in_addr_t remote_ip;
    in_port_t remote_port;

    char read_buf[BUF_SIZE + 1];
    size_t read_len;
    unsigned char send_buf[HEADER_SIZE + BUF_SIZE];


    if (argc < 2) {
        // if the number of arguments is less than 4, terminate
        fprintf(stderr, "usage: %s <port-number>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if ((local_port = (in_port_t) atoi(argv[1])) == 0) {
        // if the format of given port number is error, terminate
        fprintf(stderr, "server port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    srand(time(NULL));

    // create a socket file descriptor
    socket_fd = create_socket(AF_INET, SOCK_DGRAM, 0);

    // initialize the local address data structure
    memset((void *) &local_addr, 0, sizeof(local_addr));

    // use the Internet protocol v4 addresses
    local_addr.sin_family = AF_INET;

    // the IP address of the local can be any one
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // set the port number
    local_addr.sin_port = htons(local_port);

    // bind the socket with local address data structure
    bind_socket(socket_fd, (const struct sockaddr *) &local_addr, sizeof(local_addr));

    signal(SIGALRM, terve_alarm);
    signal(SIGIO, terve_msg_receive);
    signal(SIGQUIT, terve_quit);

    // set the process ID or process group ID
    // that will receive SIGIO and SIGURG signals for events on the file descriptor fd
    if (fcntl(socket_fd, F_SETOWN, getpid()) < 0) {
        perror("fcntl F_SETOWN");
        exit(1);
    }

    // allow receipt of asynchronous I/O signals
    if (fcntl(socket_fd, F_SETFL, O_ASYNC) < 0) {
        perror("fcntl F_SETFL, O_ASYNC");
        exit(1);
    }

    while (1) {
        // set the nonlocal goto label
        sigsetjmp(begin, 1);

        if (state == INITIATION_STATE) {
            fprintf(stdout, "#ready: ");
            fflush(stdout);

            // if at init state, the app will block at read()
            read_len = read_stdin(read_buf);

            if (read_len == 0)
                continue;

            // analyze the remote IP address and port number
            if (get_remote_addr(read_buf, &remote_ip, &remote_port) == -1) {
                fprintf(stderr, "invalid IP address or port number\n");
                fprintf(stderr, "format: <dotted-ip-address> <port-num>\n");
                fflush(stderr);
                continue;
            }

            // prepare the data structure
            memset((void *) &remote_addr, 0, sizeof(remote_addr));

            // use the Internet protocol v4 addresses
            remote_addr.sin_family = AF_INET;

            // the IP address of the local can be any one
            remote_addr.sin_addr.s_addr = remote_ip;

            // set the port number
            remote_addr.sin_port = htons(remote_port);

            // set a unsigned 32-bit random number
            rand_num = rand();

            // init the resend counter
            re_init_cnt = 0;

            // send an initiation request
            send_init_request();

            // change the state
            state = REQUEST_SENT_STATE;
        } else if (state == REQUEST_SENT_STATE) {
            // pause the process
            pause();
        } else if (state == DECISION_STATE) {
            fprintf(stdout, "#ready: ");
            fflush(stdout);

            // the user needs to make a 'y' or 'n' decision at this state
            read_len = read_stdin(read_buf);

            // if the input is not 'y' or 'n', retry
            if (read_len != 1 || (read_buf[0] != 'y' && read_buf[0] != 'n')) {
                fprintf(stderr, "please enter 'y' or 'n'\n");
                fflush(stderr);
                continue;
            }

            if (read_buf[0] == 'y')
                make_header(send_buf, HEADER_ACK);
            else
                make_header(send_buf, HEADER_REJ);

            // send the ACK packet
            send_to_socket(socket_fd, (const void *) send_buf, HEADER_SIZE, 0,
                    (const struct sockaddr *) &remote_addr, sizeof(remote_addr));

            // change the state
            if (read_buf[0] == 'y') {
                state = MESSAGE_STATE;

                // reset the heartbeat count
                heartbeat_cnt = 0;

                // send the heartbeat
                send_heartbeat();
            } else {
                state = INITIATION_STATE;
            }

        } else if (state == MESSAGE_STATE) {
            fprintf(stdout, "#your msg: ");
            fflush(stdout);

            // read a new message; block on read()
            read_len = read_stdin(read_buf);

            if (read_len == 0)
                continue;

            // send a new message
            make_header(send_buf, HEADER_MSG);

            for (i = 0; i < read_len; i++)
                send_buf[i + HEADER_SIZE] = read_buf[i];

            send_to_socket(socket_fd, (const void *) send_buf, HEADER_SIZE + read_len, 0,
                    (const struct sockaddr *) &remote_addr, sizeof(remote_addr));
        }
    }

    close(socket_fd);
    fprintf(stdout, "\n");
    return 0;
}
