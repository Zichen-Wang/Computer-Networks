#include "terve.h"


void send_init_request() {
    // this function is used to send a initiation request
    unsigned char send_buf[HEADER_SIZE];
    make_header(send_buf, HEADER_INIT);
    alarm(5);
    send_to_socket(socket_fd, (const void *) send_buf, HEADER_SIZE, 0,
            (const struct sockaddr *) &remote_addr, sizeof(remote_addr));
}


void send_heartbeat() {
    // this function is used to send heartbeat request
    unsigned char send_buf[HEADER_SIZE];
    make_header(send_buf, HEADER_HRBT);
    alarm(HEARTBEAT_INTERVAL);
    send_to_socket(socket_fd, (const void *) send_buf, HEADER_SIZE, 0,
            (const struct sockaddr *) &remote_addr, sizeof(remote_addr));
}


void terve_alarm(int sig) {
    // SIGALRM signal handler
    if (state == REQUEST_SENT_STATE) {
        if (re_init_cnt == 2) {
            // if the resend counter reaches 2, report failure and go back to init state
            fprintf(stdout, "#failure: %s %u\n",
                    inet_ntoa(remote_addr.sin_addr),
                    ntohs(remote_addr.sin_port));
            fflush(stdout);
            state = INITIATION_STATE;
        } else {
            send_init_request();
            re_init_cnt++;
        }

    } else if (state == MESSAGE_STATE) {
        if (heartbeat_cnt == 3) {
            // if the heartbeat count reaches 3, report that peer might be offline and terminate session
            // heartbeat count will reset to zero if receiving a heartbeat ACK
            fprintf(stdout, "\n#warning: peer %s %u might be offline; terminating session...",
                    inet_ntoa(remote_addr.sin_addr),
                    ntohs(remote_addr.sin_port));
            fflush(stdout);

            // send a SIGQUIT signal to itself
            kill(getpid(), SIGQUIT);
        } else {
            // otherwise, resend a heartbeat request
            send_heartbeat();
            heartbeat_cnt++;
        }
    }
}


void terve_quit(int sig) {
    // SIGQUIT signal handler
    unsigned char send_buf[HEADER_SIZE];
    if (state == MESSAGE_STATE) {
        make_header(send_buf, HEADER_TRM);

        send_to_socket(socket_fd, (const void *) send_buf, HEADER_SIZE, 0,
                (const struct sockaddr *) &remote_addr, sizeof(remote_addr));

        // reset the heartbeat count
        heartbeat_cnt = 0;

        // cancel the alarm
        alarm(0);
    }

    fprintf(stdout, "\n");
    fflush(stdout);
    exit(0);
}


static void drop_remaining_packets() {
    unsigned char recv_buf[HEADER_SIZE + BUF_SIZE];
    struct sockaddr_in recv_addr;
    socklen_t recv_addr_len;

    memset((void *) &recv_addr, 0, sizeof(recv_addr));
    recv_addr_len = sizeof(recv_addr);

    while (receive_from_socket(socket_fd, (void *) recv_buf, HEADER_SIZE + BUF_SIZE, MSG_DONTWAIT,
            (struct sockaddr *) &recv_addr, &recv_addr_len) != -1)
        ;
}


void terve_msg_receive(int sig) {
    // SIGIO signal handler
    int i;
    unsigned char recv_buf[HEADER_SIZE + BUF_SIZE];
    ssize_t recv_len;

    struct sockaddr_in recv_addr;
    socklen_t recv_addr_len;

    memset((void *) &recv_addr, 0, sizeof(recv_addr));
    recv_addr_len = sizeof(recv_addr);

    while ((recv_len = receive_from_socket(socket_fd, (void *) recv_buf, HEADER_SIZE + BUF_SIZE, MSG_DONTWAIT,
            (struct sockaddr *) &recv_addr, &recv_addr_len)) != -1) {
        // receive all pending messages at one signal
        if (state == INITIATION_STATE) {
            // if at init state, expect to receive session request packet
            if (recv_len != HEADER_SIZE)
                continue;
            if (recv_buf[0] == HEADER_INIT) {
                // session request packet

                state = DECISION_STATE;
                remote_addr = recv_addr;

                rand_num = get_rand_num(recv_buf);

                fprintf(stdout, "\n#session request from: %s %u\n",
                        inet_ntoa(recv_addr.sin_addr),
                        ntohs(recv_addr.sin_port));
                fflush(stdout);

                // drop the remaining packets before transition
                drop_remaining_packets();
                siglongjmp(begin, 1);
            }
        } else if (state == REQUEST_SENT_STATE) {
            // if at request sent state, expect to receive ack (rej) packets and other session request packets
            if (recv_len != HEADER_SIZE)
                continue;
            if (recv_buf[0] == HEADER_INIT) {
                // session request packets
                fprintf(stdout, "#session request from: %s %u\n",
                        inet_ntoa(recv_addr.sin_addr),
                        ntohs(recv_addr.sin_port));
                fflush(stdout);
            } else if (recv_buf[0] == HEADER_ACK) {
                // session ack packet
                if (get_rand_num(recv_buf) == rand_num) {
                    // if receive an ack packet

                    // cancel the alarm
                    alarm(0);

                    state = MESSAGE_STATE;
                    remote_addr = recv_addr;
                    fprintf(stdout, "#success: %s %u\n",
                            inet_ntoa(recv_addr.sin_addr),
                            ntohs(recv_addr.sin_port));
                    fflush(stdout);

                    // drop the remaining packets before transition
                    drop_remaining_packets();

                    // reset the heartbeat count
                    heartbeat_cnt = 0;

                    // send the heartbeat
                    send_heartbeat();

                    siglongjmp(begin, 1);
                }
            } else if (recv_buf[0] == HEADER_REJ) {
                // session rej packet
                if (get_rand_num(recv_buf) == rand_num) {
                    // cancel the alarm
                    alarm(0);

                    state = INITIATION_STATE;
                    fprintf(stdout, "#failure: %s %u (connection rejected)\n",
                            inet_ntoa(recv_addr.sin_addr),
                            ntohs(recv_addr.sin_port));
                    fflush(stdout);

                    // drop the remaining packets before transition
                    drop_remaining_packets();
                    siglongjmp(begin, 1);
                }
            }
        } else if (state == MESSAGE_STATE) {
            // if at message state, expect to
            // receive session request packets, termination packets, heartbeat packets and message packets

            // do not make a transition at this state
            if (recv_len == HEADER_SIZE) {
                if (recv_buf[0] == HEADER_INIT) {
                    // session request packet
                    fprintf(stdout, "\n#session request from: %s %u\n",
                            inet_ntoa(recv_addr.sin_addr),
                            ntohs(recv_addr.sin_port));
                    fprintf(stdout, "#your msg: ");
                    fflush(stdout);
                } else if (recv_buf[0] == HEADER_TRM) {
                    // session termination packet
                    fprintf(stdout, "\n#session termination received\n");
                    fflush(stdout);

                    // reset the heartbeat count
                    heartbeat_cnt = 0;

                    // cancel the alarm
                    alarm(0);

                    // terminate the app
                    exit(0);
                } else if (recv_buf[0] == HEADER_HRBT) {
                    // heartbeat packet, reset the heartbeat counter
                    if (get_rand_num(recv_buf) == rand_num)
                        heartbeat_cnt = 0;
                }
            } else if (recv_len > HEADER_SIZE) {
                // message packet
                if (recv_buf[0] == HEADER_MSG) {
                    if (get_rand_num(recv_buf) == rand_num) {
                        fprintf(stdout, "\n#received msg: ");
                        for (i = HEADER_SIZE; i < recv_len; i++)
                            fprintf(stdout, "%c", recv_buf[i]);
                        fprintf(stdout, "\n");
                        fprintf(stdout, "#your msg: ");
                        fflush(stdout);
                    }
                }
            }
        }
    }
}
