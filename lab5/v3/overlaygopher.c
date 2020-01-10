#include "overlaygopher.h"


int overlay_socket_fd;

struct forwarding_table tab[MAXSOCKIND];
int tab_used[MAXSOCKIND], tab_tentative[MAXSOCKIND], tab_counter[MAXSOCKIND];

int socket_fd[MAXSOCKIND];
int socket_fd_used[MAXSOCKIND];

struct sockaddr_in trans_addr;

sigjmp_buf begin;

static void print_tab() {
    // this procedure prints the table
    int i;
    struct in_addr in;
    char pre_IP[MAXIPSIZE + 1], post_IP[MAXIPSIZE + 1];

    fprintf(stdout, "-------------------TABLE BEGIN-------------------\n");
    fprintf(stdout, "socket_index\tpre-IP\t\tpre-port\tpost-IP\t\tpost-port\tstatus\n");

    for (i = 0; i < MAXSOCKIND; i++)
        if (tab_used[i] == 1) {
            in.s_addr = tab[i].pre_IP;
            strncpy(pre_IP, inet_ntoa(in), MAXIPSIZE);
            pre_IP[MAXIPSIZE] = '\0';

            in.s_addr = tab[i].post_IP;
            strncpy(post_IP, inet_ntoa(in), MAXIPSIZE);
            post_IP[MAXIPSIZE] = '\0';

            fprintf(stdout, "%lu\t\t%s\t%hu\t\t%s\t%hu\t\t%s\n",
                    tab[i].socket_index, pre_IP, ntohs(tab[i].pre_port), post_IP, ntohs(tab[i].post_port),
                    tab_tentative[i] ? "tentative" : "valid");
        }

    fprintf(stdout, "--------------------TABLE END--------------------\n\n");
    fflush(stdout);
}


static void insert_entry(size_t tab_index, size_t socket_index,
        in_addr_t pre_IP, in_port_t pre_port,
        in_addr_t post_IP, in_port_t post_port) {

    // insert an entry to a table at tab_index
    tab[tab_index].socket_index = socket_index;
    tab[tab_index].pre_IP = pre_IP;
    tab[tab_index].pre_port = pre_port;
    tab[tab_index].post_IP = post_IP;
    tab[tab_index].post_port = post_port;
}


static size_t tab_find_free() {
    int i;
    for (i = 0; i < MAXSOCKIND; i += 2)
        if (tab_used[i] == 0) {
            if (i + 1 < MAXSOCKIND)
                return i;
        }

    return MAXSOCKIND;
}


static size_t socket_find_free() {
    int i;
    for (i = 0; i < MAXSOCKIND; i += 2)
        if (socket_fd_used[i] == 0) {
            if (i + 1 < MAXSOCKIND)
                return i;
        }

    return MAXSOCKIND;
}


void alarm_handler(int sig) {
    int i, fd_index;
    unsigned char send_buf[MAXBUFSIZE];

    for (i = 1; i < MAXSOCKIND; i += 2) {
        if (tab_used[i] == 1 && tab_tentative[i] == 1) {
            tab_counter[i]++;
            if (tab_counter[i] == 3) {
                fd_index = tab[i].socket_index;

                // send back an UDP packet indicating failure and remove the corresponding entries
                send_buf[0] = OVERLAYREJANS;

                memset((void *) &trans_addr, 0, sizeof(trans_addr));
                trans_addr.sin_family = AF_INET;
                trans_addr.sin_addr.s_addr = tab[i].pre_IP;
                trans_addr.sin_port = tab[i].pre_port;

                send_to_socket(overlay_socket_fd, (const void *) send_buf, OVERLAYREJSIZE, 0,
                        (const struct sockaddr *) &trans_addr, sizeof(trans_addr));

                // close the transit socket file descriptors
                close(socket_fd[fd_index]);
                close(socket_fd[fd_index - 1]);

                // remove these two entries
                tab_used[i] = 0;
                tab_used[i - 1] = 0;

                tab_tentative[i] = 0;
                tab_tentative[i - 1] = 0;

                // reset the counter
                tab_counter[i] = 0;
                tab_counter[i - 1] = 0;

                socket_fd_used[fd_index] = 0;
                socket_fd_used[fd_index - 1] = 0;

                #ifdef TABLEUPDATE
                    print_tab();
                #endif
            }
        }
    }
    alarm(OVERLAYTIMEOUT);

    // jump to the beginning of the while loop
    siglongjmp(begin, 1);
}


int main(int argc, char *argv[]) {
    int i, fd_index;
    int transit_socket_fd, transit_socket_2_fd;
    in_port_t overlay_port;

    struct sockaddr_in overlay_addr, recv_addr;
    socklen_t recv_addr_len;
    in_addr_t pre_IP, post_IP;
    in_port_t pre_port, post_port, post_overlay_port, transit_port;

    size_t free_table_index;

    size_t free_socket_index;

    socklen_t trans_addr_len;

    fd_set rfds;
    int select_return_value, max_socket_fd;

    unsigned char recv_buf[MAXBUFSIZE], send_buf[MAXBUFSIZE];
    size_t recv_len;

    sigset_t set;

    if (argc != 2) {
        // check the number of arguments
        fprintf(stderr, "usage: %s <overlay-port>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if ((overlay_port = (in_port_t) atoi(argv[1])) == 0) {
        // if the format of given port number is error, terminate
        fprintf(stderr, "overlay port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // convert the overlay-port to network representation
    overlay_port = htons(overlay_port);

    // initialize a udp socket binding to any address with vpn-port
    overlay_socket_fd = initialize_udp_sockfd(overlay_port, &overlay_addr, sizeof(overlay_addr));

    // reset tab_used array, tab_tentative array and tab_counter
    memset(tab_used, 0, sizeof(tab_used));
    memset(tab_tentative, 0, sizeof(tab_tentative));
    memset(tab_counter, 0, sizeof(tab_counter));

    // reset socket_used array
    memset(socket_fd_used, 0, sizeof(socket_fd_used));

    signal(SIGALRM, alarm_handler);
    alarm(OVERLAYTIMEOUT);

    while (1) {
        // set the nonlocal goto label
        sigsetjmp(begin, 1);

        // reset the read file descriptor set
        FD_ZERO(&rfds);

        // monitor the overlay socket file descriptor
        FD_SET(overlay_socket_fd, &rfds);

        // find the maximum socket file descriptor
        max_socket_fd = overlay_socket_fd;

        for (i = 0; i < MAXSOCKIND; i++)
            if (tab_used[i] == 1) {
                // monitor each transit port socket file descriptor
                // and update the maximum socket file descriptor
                fd_index = tab[i].socket_index;

                FD_SET(socket_fd[fd_index], &rfds);
                if (socket_fd[fd_index] > max_socket_fd)
                    max_socket_fd = socket_fd[fd_index];
        }

        // use select to monitor these file descriptors
        select_return_value = select(max_socket_fd + 1, &rfds, NULL, NULL, NULL);

        if (select_return_value == -1) {
            if (errno != EINTR) {
                perror("select()");
                fflush(stderr);
            }
        } else if (select_return_value > 0) {
            // block the alarm
            sigemptyset(&set);
            sigaddset(&set, SIGALRM);
            sigprocmask(SIG_BLOCK, &set, NULL);

            for (i = 0; i < MAXSOCKIND; i++) {
                if (tab_used[i] == 1) {
                    // if this is an used entry
                    fd_index = tab[i].socket_index;
                    if (FD_ISSET(socket_fd[fd_index], &rfds)) {
                        // if there is a new packet reaching transit-port
                        memset((void *) &recv_addr, 0, sizeof(recv_addr));
                        recv_addr_len = sizeof(recv_addr);
                        recv_len = receive_from_socket(socket_fd[fd_index], (void *) recv_buf, MAXBUFSIZE, 0,
                                (struct sockaddr *) &recv_addr, &recv_addr_len);

                        if (recv_len == OVERLAYACKSIZE
                            && recv_buf[0] == OVERLAYACKANS
                            && recv_addr.sin_addr.s_addr == tab[i].post_IP
                            && tab_tentative[i] == 1) {
                            // if this is an ACK packet from the post router,
                            // send back an ACK packet and update the post transit port

                            // get the port number of transit-port
                            trans_addr_len = sizeof(trans_addr);
                            get_socket_name(socket_fd[fd_index - 1], (struct sockaddr *) &trans_addr, &trans_addr_len);
                            transit_port = trans_addr.sin_port;

                            // send back the transit port to the previous router
                            send_buf[0] = OVERLAYACKANS;
                            send_buf[1] = transit_port & 255;
                            send_buf[2] = (transit_port >> 8) & 255;

                            memset((void *) &trans_addr, 0, sizeof(trans_addr));
                            trans_addr.sin_family = AF_INET;
                            trans_addr.sin_addr.s_addr = tab[i].pre_IP;
                            trans_addr.sin_port = tab[i].pre_port;

                            send_to_socket(overlay_socket_fd, (const void *) send_buf, OVERLAYACKSIZE, 0,
                                    (const struct sockaddr *) &trans_addr, sizeof(trans_addr));

                            // update the post port
                            tab[i].post_port = recv_buf[1] + (recv_buf[2] << 8);
                            tab[i - 1].post_port = recv_buf[1] + (recv_buf[2] << 8);

                            // reset the tentative status
                            tab_tentative[i] = 0;

                            // reset the counter
                            tab_counter[i] = 0;
                            tab_counter[i - 1] = 0;

                            #ifdef TABLEUPDATE
                                print_tab();
                            #endif
                        } else if (recv_len == OVERLAYREJSIZE
                            && recv_buf[0] == OVERLAYREJANS
                            && recv_addr.sin_addr.s_addr == tab[i].post_IP
                            && tab_tentative[i] == 1) {
                            // if this is an REJ packet from the post router,
                            // send back an UDP packet indicating failure and remove the corresponding entries

                            send_buf[0] = OVERLAYREJANS;

                            memset((void *) &trans_addr, 0, sizeof(trans_addr));
                            trans_addr.sin_family = AF_INET;
                            trans_addr.sin_addr.s_addr = tab[i].pre_IP;
                            trans_addr.sin_port = tab[i].pre_port;

                            send_to_socket(overlay_socket_fd, (const void *) send_buf, OVERLAYREJSIZE, 0,
                                    (const struct sockaddr *) &trans_addr, sizeof(trans_addr));

                            // close the transit socket file descriptors
                            close(socket_fd[fd_index]);
                            close(socket_fd[fd_index - 1]);

                            // remove these two entries
                            tab_used[i] = 0;
                            tab_used[i - 1] = 0;

                            tab_tentative[i] = 0;
                            tab_tentative[i - 1] = 0;

                            // reset the counter
                            tab_counter[i] = 0;
                            tab_counter[i - 1] = 0;

                            socket_fd_used[fd_index] = 0;
                            socket_fd_used[fd_index - 1] = 0;

                            #ifdef TABLEUPDATE
                                print_tab();
                            #endif
                        } else {
                            if (fd_index % 2 == 0
                                && tab[i].pre_IP == recv_addr.sin_addr.s_addr
                                && tab_tentative[i + 1] == 0) {
                                // if this is a data packet from the previous router or client
                                // and the post port number is not pending update

                                // verify that the packet's IP matches pre-IP

                                if (tab_tentative[i] == 1) {
                                    // if pre port number is pending update, update the pre port

                                    // reset the tentative status
                                    tab_tentative[i] = 0;
                                    tab[i].pre_port = recv_addr.sin_port;
                                    tab[i + 1].pre_port = recv_addr.sin_port;
                                    //#ifdef TABLEUPDATE
                                        print_tab();
                                    //#endif
                                }

                                if (tab[i].pre_port == recv_addr.sin_port) {
                                    // verify that the packet's port number matches pre-port

                                    memset((void *) &trans_addr, 0, sizeof(trans_addr));
                                    trans_addr.sin_family = AF_INET;
                                    trans_addr.sin_addr.s_addr = tab[i].post_IP;
                                    trans_addr.sin_port = tab[i].post_port;

                                    // transit the packet to the post router or dst server
                                    send_to_socket(socket_fd[fd_index + 1], (const void *) recv_buf, recv_len, 0,
                                            (const struct sockaddr *) &trans_addr, sizeof(trans_addr));
                                }
                            }

                            if (fd_index % 2 == 1
                                && tab[i].post_IP == recv_addr.sin_addr.s_addr
                                && tab[i].post_port == recv_addr.sin_port
                                && tab_tentative[i - 1] == 0) {
                                // if this is a data packet from the post router or server

                                // verify that the packet's IP matches post-IP
                                // and the packet's port number matches post-port
                                // and the pre port number is not pending update

                                memset((void *) &trans_addr, 0, sizeof(trans_addr));
                                trans_addr.sin_family = AF_INET;
                                trans_addr.sin_addr.s_addr = tab[i].pre_IP;
                                trans_addr.sin_port = tab[i].pre_port;

                                // transit the packet to the pre router or src client
                                send_to_socket(socket_fd[fd_index - 1], (const void *) recv_buf, recv_len, 0,
                                        (const struct sockaddr *) &trans_addr, sizeof(trans_addr));
                            }
                        }
                    }
                }
            }

            if (FD_ISSET(overlay_socket_fd, &rfds)) {
                // if there is a new packet reaching overlay-port
                memset((void *) &recv_addr, 0, sizeof(recv_addr));
                recv_addr_len = sizeof(recv_addr);
                recv_len = receive_from_socket(overlay_socket_fd, (void *) recv_buf, MAXBUFSIZE, 0,
                                               (struct sockaddr *) &recv_addr, &recv_addr_len);

                if ((free_table_index = tab_find_free()) != MAXSOCKIND
                    && (free_socket_index = socket_find_free()) != MAXSOCKIND) {
                    // if there is a place for two new entries
                    if (recv_len > 0 && recv_len == 1 + (recv_buf[0] + 1) * 8) {
                        // get the pre router's IP
                        pre_IP = recv_addr.sin_addr.s_addr;

                        // get the pre router's port
                        pre_port = recv_addr.sin_port;

                        // calculate the post router's IP
                        post_IP = 0;
                        for (i = 0; i < 4; i++)
                            post_IP += recv_buf[i + 10] << (i << 3);

                        if (recv_buf[0] > 1) {
                            // if this is not the last router
                            // set the post router's port 0 (dummy value)
                            post_port = 0;
                        } else {
                            // if this is the last router
                            // update it by dest server's port
                            post_port = recv_buf[15] + (recv_buf[16] << 8);
                        }

                        // initialize a new UDP socket file descriptor for transit-port
                        transit_socket_fd = initialize_udp_sockfd(0, &trans_addr, sizeof(trans_addr));

                        // initialize a new UDP socket file descriptor for transit-port-2
                        transit_socket_2_fd = initialize_udp_sockfd(0, &trans_addr, sizeof(trans_addr));

                        // set the socket index and insert the first entry
                        socket_fd[free_socket_index] = transit_socket_fd;
                        insert_entry(free_table_index, free_socket_index,
                                pre_IP, pre_port,
                                post_IP, post_port);

                        tab_used[free_table_index] = 1;
                        tab_tentative[free_table_index] = 1;
                        tab_counter[free_table_index] = 0;
                        socket_fd_used[free_socket_index] = 1;

                        // set the socket index and insert the second entry
                        socket_fd[free_socket_index + 1] = transit_socket_2_fd;
                        insert_entry(free_table_index + 1, free_socket_index + 1,
                                pre_IP, pre_port,
                                post_IP, post_port);

                        tab_used[free_table_index + 1] = 1;
                        tab_counter[free_table_index + 1] = 0;
                        socket_fd_used[free_socket_index + 1] = 1;

                        if (recv_buf[0] > 1) {
                            // if this is not the last router

                            // set the second entry tentative
                            tab_tentative[free_table_index + 1] = 1;

                            // send a overlay request to the post router
                            post_overlay_port = recv_buf[15] + (recv_buf[16] << 8);
                            send_buf[0] = recv_buf[0] - 1;
                            for (i = 9; i < recv_len; i++)
                                send_buf[i - 8] = recv_buf[i];

                            memset((void *) &trans_addr, 0, sizeof(trans_addr));
                            trans_addr.sin_family = AF_INET;
                            trans_addr.sin_addr.s_addr = post_IP;
                            trans_addr.sin_port = post_overlay_port;

                            send_to_socket(transit_socket_2_fd, (const void *) send_buf, recv_len - 8, 0,
                                    (const struct sockaddr *) &trans_addr, sizeof(trans_addr));
                        } else {
                            // if this is the last router
                            // reset the tentative status of the second entry
                            tab_tentative[free_table_index + 1] = 0;

                            // get the port number of transit-port
                            trans_addr_len = sizeof(trans_addr);
                            get_socket_name(transit_socket_fd, (struct sockaddr *) &trans_addr, &trans_addr_len);
                            transit_port = trans_addr.sin_port;

                            // send back an ACK
                            send_buf[0] = OVERLAYACKANS;
                            send_buf[1] = transit_port & 255;
                            send_buf[2] = (transit_port >> 8) & 255;


                            send_to_socket(overlay_socket_fd, (const void *) send_buf, OVERLAYACKSIZE, 0,
                                           (const struct sockaddr *) &recv_addr, sizeof(recv_addr));
                        }

                        #ifdef TABLEUPDATE
                            print_tab();
                        #endif
                    } else {
                        // if received an unknown packet, send back an UDP packet indicating failure
                        send_buf[0] = OVERLAYREJANS;
                        send_to_socket(overlay_socket_fd, (const void *) send_buf, OVERLAYREJSIZE, 0,
                                (const struct sockaddr *) &recv_addr, sizeof(recv_addr));
                    }
                } else {
                    // if the table is full, send back an UDP packet indicating failure
                    send_buf[0] = OVERLAYREJANS;
                    send_to_socket(overlay_socket_fd, (const void *) send_buf, OVERLAYREJSIZE, 0,
                            (const struct sockaddr *) &recv_addr, sizeof(recv_addr));
                }
            }
            // unblock the alarm
            sigprocmask(SIG_UNBLOCK, &set, NULL);
        }
    }

    return 0;
}