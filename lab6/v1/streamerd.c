#include "streamerd.h"
#include "logger.h"


int server_udp_fd;
unsigned int mode;
int payload_size;
float lambda;
float a, delta, epsilon, beta;


static int get_filesize(const char *pathname) {
    struct stat st;
    if (stat(pathname, &st) == -1) {
        perror("stat()");
        fflush(stderr);
        return -1;
    }

    return st.st_size;
}


static void sigio_handler(int sig) {
    struct sockaddr_in client_udp_addr;
    socklen_t client_udp_addr_len;

    unsigned char recv_buf[MAX_BUF_SIZE];
    ssize_t recv_len;

    int buf_occupancy, target_buf, gamma;

    while (1) {
        memset((void *) &client_udp_addr, 0, sizeof(client_udp_addr));
        client_udp_addr_len = sizeof(client_udp_addr);

        recv_len = recvfrom(server_udp_fd, (void *) recv_buf, MAX_BUF_SIZE, MSG_DONTWAIT,
                (struct sockaddr *) &client_udp_addr, &client_udp_addr_len);

        if (recv_len == -1)
            break;

        if (recv_len != 12)
            continue;

        buf_occupancy = *(int *)(recv_buf);
        target_buf = *(int *)(recv_buf + 4);
        gamma = *(int *)(recv_buf + 8);

        //printf("%d %d %f\n", buf_occupancy, target_buf, gamma);

        if (mode == 0) {
            if (buf_occupancy < target_buf)
                lambda = lambda + a;
            else if (buf_occupancy > target_buf)
                lambda = lambda - a;
        } else if (mode == 1) {
            if (buf_occupancy < target_buf)
                lambda = lambda + a;
            else if (buf_occupancy > target_buf)
                lambda = delta * lambda;
        } else if (mode == 2) {
            lambda = lambda + epsilon * (target_buf - buf_occupancy);
        } else if (mode == 3) {
            lambda = lambda + epsilon * (target_buf - buf_occupancy)
                            - beta * (lambda - 1.0 * gamma * payload_size / 4096);
        }
    }
}


static void sprint_float(char *log_buf, void *data) {
    sprintf(log_buf, "%.6f", *(float *)(data));
}


int main(int argc, char *argv[]) {
    int i;

    int listen_tcp_fd, server_tcp_fd;
    struct sockaddr_in server_tcp_addr, client_tcp_addr;
    socklen_t client_tcp_addr_len;
    in_port_t server_tcp_port;

    int worker_pid;
    struct sockaddr_in server_udp_addr;
    socklen_t server_udp_addr_len;
    in_port_t client_udp_port;

    unsigned char send_buf[MAX_BUF_SIZE];
    size_t send_len;
    unsigned char recv_buf[MAX_BUF_SIZE];
    ssize_t recv_len;

    char pathname[MAX_PATH_SIZE];
    int audiofile_fd;
    int filesize;

    int udp_pid;

    float init_lambda;

    unsigned int seq_num;
    struct timespec req, rem;
    struct sockaddr_in client_udp_addr;
    int sleep_ret;

    sigset_t set;

    logger_ptr log;
    FILE *control_param_fp, *log_fp;

    float cur_lambda;



    if (argc != 6) {
        fprintf(stderr, "usage: %s <tcp-port> <payload-size> <init-lambda> <mode> <logfile1>\n", argv[0]);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    if ((server_tcp_port = (in_port_t) atoi(argv[1])) == 0) {
        fprintf(stderr, "tcp port number format error\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    server_tcp_port = htons(server_tcp_port);

    payload_size = atoi(argv[2]);

    init_lambda = atof(argv[3]);

    mode = atoi(argv[4]);
    if (mode < 0 || mode > 3) {
        fprintf(stderr, "method should be 0, 1, 2 or 3\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    log_fp = fopen(argv[5], "a");

    listen_tcp_fd = socket(AF_INET, SOCK_STREAM, 0);

    memset((void *) &server_tcp_addr, 0, sizeof(server_tcp_addr));
    server_tcp_addr.sin_family = AF_INET;
    server_tcp_addr.sin_addr.s_addr = INADDR_ANY;
    server_tcp_addr.sin_port = server_tcp_port;
    if (bind(listen_tcp_fd, (const struct sockaddr *) &server_tcp_addr, sizeof(server_tcp_addr)) == -1) {
        perror("bind()");
        fflush(stderr);
        close(listen_tcp_fd);
        exit(EXIT_FAILURE);
    }

    listen(listen_tcp_fd, MAX_LISTEN);

    while (1) {
        memset((void *) &client_tcp_addr, 0, sizeof(client_tcp_addr));
        client_tcp_addr_len = sizeof(client_tcp_addr);
        server_tcp_fd = accept(listen_tcp_fd, (struct sockaddr *) &client_tcp_addr, &client_tcp_addr_len);

        if (server_tcp_fd == -1) {
            perror("accept()");
            fflush(stderr);
            close(listen_tcp_fd);
            exit(EXIT_FAILURE);
        }

        worker_pid = fork();

        if (worker_pid == 0) {
            close(listen_tcp_fd);

            recv_len = read(server_tcp_fd, (void *) recv_buf, MAX_BUF_SIZE);
            if (recv_len == -1 || recv_len == 0) {
                if (recv_len == -1)
                    perror("read()");
                fprintf(stderr, "cannot read audiofile name from the client\n");
                fflush(stderr);
                close(server_tcp_fd);
                exit(EXIT_FAILURE);
            }

            strcpy(pathname, "/tmp/");
            memcpy(pathname + 5, recv_buf, recv_len * sizeof(char));
            pathname[recv_len + 5] = 0;

            audiofile_fd = open((const char *) pathname, O_RDONLY);

            if (audiofile_fd == -1) {
                perror("open()");
                fprintf(stderr, "the request to open file [%s] is failed\n", pathname);
                fflush(stderr);

                send_buf[0] = '0';
                write(server_tcp_fd, (const void *) send_buf, 1);
                close(server_tcp_fd);
                exit(EXIT_FAILURE);
            }

            filesize = get_filesize((const char *) pathname);
            if (filesize == -1) {
                close(server_tcp_fd);
                exit(EXIT_FAILURE);
            }

            server_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

            memset((void *) &server_udp_addr, 0, sizeof(server_udp_addr));
            server_udp_addr.sin_family = AF_INET;
            server_udp_addr.sin_addr.s_addr = INADDR_ANY;
            server_udp_addr.sin_port = 0;
            bind(server_udp_fd, (const struct sockaddr *) &server_udp_addr, sizeof(server_udp_addr));

            server_udp_addr_len = sizeof(server_udp_addr);
            getsockname(server_udp_fd, (struct sockaddr *) &server_udp_addr, &server_udp_addr_len);

            send_buf[0] = '2';
            *(in_port_t *)(send_buf + 1) = server_udp_addr.sin_port;
            *(int *)(send_buf + 1 + sizeof(in_port_t)) = filesize;

            write(server_tcp_fd, (const void *) send_buf, 1 + sizeof(in_port_t) + sizeof(int));

            recv_len = read(server_tcp_fd, (void *) recv_buf, MAX_BUF_SIZE);

            if (recv_len != 2) {
                if (recv_len == -1)
                    perror("read()");
                fprintf(stderr, "cannot receive client udp port number\n");
                fflush(stderr);
                close(server_tcp_fd);
                exit(EXIT_FAILURE);
            }

            client_udp_port = *(in_port_t *)(recv_buf);

            udp_pid = fork();

            if (udp_pid == 0) {
                close(server_tcp_fd);
                signal(SIGIO, sigio_handler);

                if (fcntl(server_udp_fd, F_SETOWN, getpid()) < 0) {
                    perror("fcntl(F_SETOWN)");
                    close(server_udp_fd);
                    exit(EXIT_FAILURE);
                }

                if (fcntl(server_udp_fd, F_SETFL, O_ASYNC) < 0) {
                    perror("fcntl(F_SETFL, O_ASYNC)");
                    close(server_udp_fd);
                    exit(EXIT_FAILURE);
                }

                control_param_fp = fopen("control-param.dat", "r");
                if (control_param_fp == NULL) {
                    fprintf(stderr, "cannot open control file control-param.dat\n");
                    fflush(stderr);
                    close(server_udp_fd);
                    exit(EXIT_FAILURE);
                }

                fscanf(control_param_fp, "%f%f%f%f", &a, &delta, &epsilon, &beta);
                fclose(control_param_fp);

                lambda = init_lambda;
                seq_num = 0;

                memset((void *) &client_udp_addr, 0, sizeof(client_udp_addr));
                client_udp_addr.sin_family = AF_INET;
                client_udp_addr.sin_addr.s_addr = client_tcp_addr.sin_addr.s_addr;
                client_udp_addr.sin_port = client_udp_port;

                log = logger_alloc();

                while (1) {
                    sigemptyset(&set);
                    sigaddset(&set, SIGIO);
                    sigprocmask(SIG_BLOCK, &set, NULL);

                    cur_lambda = lambda;

                    sigprocmask(SIG_UNBLOCK, &set, NULL);

                    if (cur_lambda < 1) {
                        req.tv_sec = 1;
                        req.tv_nsec = 0;
                    } else {
                        req.tv_sec = floor(1 / cur_lambda);
                        req.tv_nsec = floor(1e9 / cur_lambda - req.tv_sec * 1e9);
                    }

                    sleep_ret = nanosleep((const struct timespec *) &req, &rem);

                    while (sleep_ret == -1) {
                        if (errno == EINTR) {
                            req = rem;
                            sleep_ret = nanosleep((const struct timespec *) &req, &rem);
                        } else {
                            perror("nanosleep()");
                            fflush(stderr);
                            close(server_udp_fd);
                            exit(EXIT_FAILURE);
                        }
                    }

                    *(unsigned int *)(send_buf) = seq_num;

                    send_len = 4;

                    if (cur_lambda >= 1) {
                        recv_len = read(audiofile_fd, (void *) send_buf + 4, payload_size);
                        if (recv_len == -1 || recv_len == 0)
                            break;
                        send_len += recv_len;
                    }

                    logger_log(log, (void *) &cur_lambda, sizeof(float));

                    sendto(server_udp_fd, (const void *) send_buf, send_len, 0,
                           (const struct sockaddr *) &client_udp_addr, sizeof(client_udp_addr));

                    seq_num++;
                }

                close(server_udp_fd);

                logger_write(log, log_fp, client_udp_addr.sin_addr, client_udp_port, sprint_float);
                logger_destroy(log);

                fclose(log_fp);

                fprintf(stdout, "%s:%hu done\n", inet_ntoa(client_udp_addr.sin_addr), ntohs(client_udp_port));
                fflush(stdout);
                return 0;
            } else {
                fclose(log_fp);
                close(server_udp_fd);

                while (1) {
                    if (wait(NULL) == udp_pid) {
                        fflush(stdout);
                        send_buf[0] = '5';
                        for (i = 0; i < 5; i++)
                            write(server_tcp_fd, (const void *) send_buf, 1);
                        break;
                    }
                }
            }

            close(server_tcp_fd);
            return 0;
        } else {
            close(server_tcp_fd);
        }
    }

    fclose(log_fp);
    return 0;
}
