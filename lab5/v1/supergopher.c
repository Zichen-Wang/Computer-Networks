#include "supergopher.h"


static void print_tab(const struct forwarding_table *tab, const int *tab_used) {
    // this procedure prints the table
    int i;
    struct in_addr in;
    char client_IP[MAXIPSIZE + 1], server_IP[MAXIPSIZE + 1];

    fprintf(stdout, "-------------------TABLE BEGIN-------------------\n");
    fprintf(stdout, "socket-index\ttrans-port\ttrans-port-2\tcli-IP\t\tcli-port\tserver-IP\tserver-port\n");

    for (i = 0; i < MAXSOCKIND; i++)
        if (tab_used[i] == 1) {
            in.s_addr = tab[i].client_IP;
            strncpy(client_IP, inet_ntoa(in), MAXIPSIZE);
            client_IP[MAXIPSIZE] = '\0';

            in.s_addr = tab[i].server_IP;
            strncpy(server_IP, inet_ntoa(in), MAXIPSIZE);
            server_IP[MAXIPSIZE] = '\0';

            fprintf(stdout, "%lu\t\t%hu\t\t%hu\t\t%s\t%hu\t\t%s\t%hu\n",
                    tab[i].socket_index, ntohs(tab[i].transit_port), ntohs(tab[i].transit_port_2),
                    client_IP, ntohs(tab[i].client_port), server_IP, ntohs(tab[i].server_port));
        }

    fprintf(stdout, "--------------------TABLE END--------------------\n\n");
    fflush(stdout);
}

static void insert_entry(struct forwarding_table *tab, size_t tab_index,
        size_t socket_index, in_port_t transit_port, in_port_t transit_port_2,
        in_addr_t client_IP, in_port_t client_port,
        in_addr_t server_IP, in_port_t server_port) {

    // insert an entry to a table at tab_index
    tab[tab_index].socket_index = socket_index;
    tab[tab_index].transit_port = transit_port;
    tab[tab_index].transit_port_2 = transit_port_2;
    tab[tab_index].client_IP = client_IP;
    tab[tab_index].client_port = client_port;
    tab[tab_index].server_IP = server_IP;
    tab[tab_index].server_port = server_port;
}


static size_t tab_find_free(const int *tab_used) {
    int i;
    for (i = 0; i < MAXSOCKIND; i += 2)
        if (tab_used[i] == 0) {
            if (i + 1 < MAXSOCKIND)
                return i;
        }

    return MAXSOCKIND;
}


static size_t socket_find_free(const int *socket_fd_used) {
    int i;
    for (i = 0; i < MAXSOCKIND; i += 2)
        if (socket_fd_used[i] == 0) {
            if (i + 1 < MAXSOCKIND)
                return i;
        }

    return MAXSOCKIND;
}


int main(int argc, char *argv[]) {
    int i, fd_index;
    int vpn_socket_fd, transit_socket_fd, transit_socket_2_fd;
    in_port_t vpn_port;

    struct sockaddr_in vpn_addr, recv_addr;
    socklen_t recv_addr_len;
    in_addr_t client_IP, server_IP;
    in_port_t server_port, transit_port, transit_port_2;


    struct forwarding_table tab[MAXSOCKIND];
    int tab_used[MAXSOCKIND];
    size_t free_table_index;

    int socket_fd[MAXSOCKIND];
    int socket_fd_used[MAXSOCKIND];
    size_t free_socket_index;

    struct sockaddr_in trans_addr;
    socklen_t trans_addr_len;

    fd_set rfds;
    int select_return_value, max_socket_fd;

    unsigned char recv_buf[MAXBUFSIZE], send_buf[MAXBUFSIZE];
    size_t recv_len;

    if (argc != 2) {
        // check the number of arguments
        fprintf(stderr, "usage: %s <vpn-port>\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    if ((vpn_port = (in_port_t) atoi(argv[1])) == 0) {
        // if the format of given port number is error, terminate
        fprintf(stderr, "vpn port number format error; terminating...\n");
        fflush(stderr);
        exit(1);
    }

    // convert the vpn-port to network representation
    vpn_port = htons(vpn_port);

    // initialize a udp socket binding to any address with vpn-port
    vpn_socket_fd = initialize_udp_sockfd(vpn_port, &vpn_addr, sizeof(vpn_addr));

    // reset tab_used array and tab_tentative array
    memset(tab_used, 0, sizeof(tab_used));

    // reset socket_used array
    memset(socket_fd_used, 0, sizeof(socket_fd_used));

    while (1) {
        // reset the read file descriptor set
        FD_ZERO(&rfds);

        // monitor the vpn socket file descriptor
        FD_SET(vpn_socket_fd, &rfds);

        // find the maximum socket file descriptor
        max_socket_fd = vpn_socket_fd;

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
            perror("select()");
            fflush(stderr);
        } else if (select_return_value > 0) {
            for (i = 0; i < MAXSOCKIND; i++)
                if (tab_used[i] == 1) {
                    fd_index = tab[i].socket_index;
                    if (FD_ISSET(socket_fd[fd_index], &rfds)) {
                        // if there is a new packet reaching transit-port or transit-port-2
                        memset((void *) &recv_addr, 0, sizeof(recv_addr));
                        recv_addr_len = sizeof(recv_addr);
                        recv_len = receive_from_socket(socket_fd[fd_index], (void *) recv_buf, MAXBUFSIZE, 0,
                                                       (struct sockaddr *) &recv_addr, &recv_addr_len);

                        if (fd_index % 2 == 0 && tab[i].client_IP == recv_addr.sin_addr.s_addr) {
                            // if this is a data packet from the client

                            // verify that the packet's IP matches cli-IP
                            if (tab[i].client_port == 0) {
                                // if client-port is 0 (dummy), update it
                                tab[i].client_port = recv_addr.sin_port;
                                tab[i + 1].client_port = recv_addr.sin_port;
                                #ifdef TABLEUPDATE
                                    print_tab(tab, tab_used);
                                #endif
                            }

                            if (tab[i].client_port == recv_addr.sin_port) {
                                // verify that the packet's port number matches cli-port
                                memset((void *) &trans_addr, 0, sizeof(trans_addr));
                                trans_addr.sin_family = AF_INET;
                                trans_addr.sin_addr.s_addr = tab[i].server_IP;
                                trans_addr.sin_port = tab[i].server_port;

                                // transit the packet to the target server
                                send_to_socket(socket_fd[fd_index + 1], (const void *) recv_buf, recv_len, 0,
                                               (const struct sockaddr *) &trans_addr, sizeof(trans_addr));
                            }
                        }

                        if (fd_index % 2 == 1
                            && tab[i].server_IP == recv_addr.sin_addr.s_addr
                            && tab[i].server_port == recv_addr.sin_port
                            && tab[i].client_port != 0) {

                            // if this is a data packet from the server

                            // verify that the packet's IP matches server-IP
                            // and the packet's port number matches server-port
                            // and the client port number is not a dummy value

                            memset((void *) &trans_addr, 0, sizeof(trans_addr));
                            trans_addr.sin_family = AF_INET;
                            trans_addr.sin_addr.s_addr = tab[i].client_IP;
                            trans_addr.sin_port = tab[i].client_port;

                            // transit the packet to the client
                            send_to_socket(socket_fd[fd_index - 1], (const void *) recv_buf, recv_len, 0,
                                           (const struct sockaddr *) &trans_addr, sizeof(trans_addr));
                        }
                    }
                }

            if (FD_ISSET(vpn_socket_fd, &rfds)) {
                // if there is a new vpn client request
                // receive the UDP request packet
                memset((void *) &recv_addr, 0, sizeof(recv_addr));
                recv_addr_len = sizeof(recv_addr);
                recv_len = receive_from_socket(vpn_socket_fd, (void *) recv_buf, MAXBUFSIZE, 0,
                        (struct sockaddr *) &recv_addr, &recv_addr_len);

                if ((free_table_index = tab_find_free(tab_used)) != MAXSOCKIND
                    && (free_socket_index = socket_find_free(socket_fd_used)) != MAXSOCKIND) {

                    // if there is a place for two new entries
                    if (recv_len == VPNREQSIZE) {
                        // if the packet is a valid UDP request packet (4-byte IP, 2-byte port)

                        client_IP = recv_addr.sin_addr.s_addr;

                        server_IP = 0;
                        for (i = 0; i < 4; i++)
                            server_IP += recv_buf[i] << (i << 3);

                        server_port = recv_buf[4] + (recv_buf[5] << 8);

                        // initialize a new UDP socket file descriptor for transit-port
                        transit_socket_fd = initialize_udp_sockfd(0, &trans_addr, sizeof(trans_addr));
                        // get the port number of transit-port
                        trans_addr_len = sizeof(trans_addr);
                        get_socket_name(transit_socket_fd, (struct sockaddr *) &trans_addr, &trans_addr_len);
                        transit_port = trans_addr.sin_port;

                        // initialize a new UDP socket file descriptor for transit-port-2
                        transit_socket_2_fd = initialize_udp_sockfd(0, &trans_addr, sizeof(trans_addr));
                        // get the port number of transit-port-2
                        trans_addr_len = sizeof(trans_addr);
                        get_socket_name(transit_socket_2_fd, (struct sockaddr *) &trans_addr, &trans_addr_len);
                        transit_port_2 = trans_addr.sin_port;

                        // set the socket index and insert the first entry
                        socket_fd[free_socket_index] = transit_socket_fd;
                        insert_entry(tab, free_table_index,
                                free_socket_index, transit_port, transit_port_2,
                                client_IP, 0,
                                server_IP, server_port);

                        tab_used[free_table_index] = 1;
                        socket_fd_used[free_socket_index] = 1;

                        // set the socket index and insert the second entry
                        socket_fd[free_socket_index + 1] = transit_socket_2_fd;
                        insert_entry(tab, free_table_index + 1,
                                free_socket_index + 1, transit_port, transit_port_2,
                                client_IP, 0,
                                server_IP, server_port);

                        tab_used[free_table_index + 1] = 1;
                        socket_fd_used[free_socket_index + 1] = 1;

                        // send back an ACK
                        send_buf[0] = VPNACKANS;
                        send_buf[1] = transit_port & 255;
                        send_buf[2] = (transit_port >> 8) & 255;

                        send_to_socket(vpn_socket_fd, (const void *) send_buf, VPNACKSIZE, 0,
                                (const struct sockaddr *) &recv_addr, sizeof(recv_addr));

                        // print the update of the table
                        #ifdef TABLEUPDATE
                            print_tab(tab, tab_used);
                        #endif

                    } else {
                        // if received an unknown packet, send back a rejected packet
                        send_buf[0] = VPNREJANS;
                        send_to_socket(vpn_socket_fd, (const void *) send_buf, VPNREJSIZE, 0,
                                (const struct sockaddr *) &recv_addr, sizeof(recv_addr));
                    }
                } else {
                    // if the table is full, send back an UDP packet indicating failure
                    send_buf[0] = VPNREJANS;
                    send_to_socket(vpn_socket_fd, (const void *) send_buf, VPNREJSIZE, 0,
                            (const struct sockaddr *) &recv_addr, sizeof(recv_addr));
                }
            }
        }
    }

    return 0;
}