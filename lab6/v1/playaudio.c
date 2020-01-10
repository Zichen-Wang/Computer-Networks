#include "playaudio.h"
#include "queue.h"
#include "mulaw.h"
#include "logger.h"


static struct circular_queue *cq;
static int client_udp_fd;
struct in_addr server_ip;
static in_port_t server_udp_port;
static int gamma, target_buf;
static pthread_mutex_t lock;
static int done;

static logger_ptr log;


static void * producer(void *arg) {
    int i;
    struct sockaddr_in src_udp_addr, dst_udp_addr;
    socklen_t src_udp_addr_len;

    unsigned char recv_buf[MAX_BUF_SIZE];
    ssize_t recv_len;

    unsigned char send_buf[MAX_BUF_SIZE];

    while (1) {
        memset((void *) &src_udp_addr, 0, sizeof(src_udp_addr));
        src_udp_addr_len = sizeof(src_udp_addr);

        recv_len = recvfrom(client_udp_fd, (void *) recv_buf, MAX_BUF_SIZE, 0,
                (struct sockaddr *) &src_udp_addr, &src_udp_addr_len);

        if (recv_len == -1) {
            if (errno != EINTR) {
                perror("recvfrom()");
                fflush(stderr);
            }
            continue;
        }

        if (recv_len == 0)
            continue;

        pthread_mutex_lock(&lock);

        *(int *)(send_buf) = cq -> size;
        logger_log(log, (void *) &(cq -> size), sizeof(ull));

        pthread_mutex_unlock(&lock);

        *(int *)(send_buf + 4) = target_buf;
        *(int *)(send_buf + 8) = gamma;

        memset((void *) &dst_udp_addr, 0, sizeof(dst_udp_addr));
        dst_udp_addr.sin_family = AF_INET;
        dst_udp_addr.sin_addr = server_ip;
        dst_udp_addr.sin_port = server_udp_port;

        sendto(client_udp_fd, (void *) send_buf, 12, 0,
               (const struct sockaddr *) &dst_udp_addr, sizeof(dst_udp_addr));

        pthread_mutex_lock(&lock);
        for (i = 4; i < recv_len; i++) {
            if (enqueue(cq, recv_buf[i]) == -1) {
                fprintf(stderr, "cannot enqueue\n");
                fflush(stderr);
                break;
            }
        }
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}


static void sigquit_handler(int sig) {
    done = 1;
}


static struct in_addr get_local_ip() {
    char name[50];
    struct addrinfo *result;

    if (gethostname(name, sizeof(name))) {
        perror("gethostname(): invalid hostname");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    if (getaddrinfo(name, NULL, NULL, &result)) {
        perror("getaddrinfo(): invalid hostname");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    return ((struct sockaddr_in *) result -> ai_addr) -> sin_addr;
}


static void sprint_int(char *log_buf, void *data) {
    sprintf(log_buf, "%d", *(int *)(data));
}


int main(int argc, char *argv[]) {
    int i;
    int client_tcp_fd;
    struct sockaddr_in client_tcp_addr, server_tcp_addr;
    in_port_t server_tcp_port;

    unsigned char send_buf[MAX_BUF_SIZE];
    size_t send_len;
    unsigned char recv_buf[MAX_BUF_SIZE];
    ssize_t recv_len;

    char audiofile[MAX_PATH_SIZE];
    //int filesize;


    struct sockaddr_in client_udp_addr;
    socklen_t client_udp_addr_len;
    in_port_t client_udp_port;

    int worker_pid;
    struct sigaction act;

    ssize_t buf_size;

    pthread_t producer_tid;

    struct timespec req, rem;
    int sleep_ret;

    sigset_t set;

    size_t mulaw_size;
    unsigned char *mulaw_buf;

    FILE *fp;

    if (argc != 9) {
        fprintf(stderr, "usage: %s <tcp-ip> <tcp-port> <audiofile> <block-size> <gamma> <buf-size> <target-buf> "
                        "<logfile2>\n", argv[0]);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    if (inet_aton((const char *) argv[1], &server_ip) == 0) {
        fprintf(stderr, "dotted IPv4 address format error\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    if ((server_tcp_port = (in_port_t) atoi(argv[2])) == 0) {
        fprintf(stderr, "tcp port number format error\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    server_tcp_port = htons(server_tcp_port);

    strncpy(audiofile, argv[3], MAX_PATH_SIZE);

    //block_size = atoi(argv[4]);
    gamma = atoi(argv[5]);
    buf_size = atoi(argv[6]);
    target_buf = atoi(argv[7]);

    fp = fopen(argv[8], "w");


    client_tcp_fd = socket(AF_INET, SOCK_STREAM, 0);

    memset((void *) &client_tcp_addr, 0, sizeof(client_tcp_addr));
    client_tcp_addr.sin_family = AF_INET;
    client_tcp_addr.sin_addr.s_addr = INADDR_ANY;
    client_tcp_addr.sin_port = 0;
    if (bind(client_tcp_fd, (const struct sockaddr *) &client_tcp_addr, sizeof(client_tcp_addr)) == -1) {
        perror("bind()");
        fflush(stderr);
        close(client_tcp_fd);
        exit(EXIT_FAILURE);
    }

    memset((void *) &server_tcp_addr, 0, sizeof(server_tcp_addr));
    server_tcp_addr.sin_family = AF_INET;
    server_tcp_addr.sin_addr = server_ip;
    server_tcp_addr.sin_port = server_tcp_port;

    if (connect(client_tcp_fd, (const struct sockaddr *) &server_tcp_addr, sizeof(server_tcp_addr)) == -1) {
        perror("connect()");
        fflush(stderr);
        close(client_tcp_fd);
        exit(EXIT_FAILURE);
    }

    send_len = strlen(audiofile);
    memcpy(send_buf, audiofile, send_len);

    write(client_tcp_fd, (const void *) send_buf, send_len);

    recv_len = read(client_tcp_fd, (void *) recv_buf, MAX_BUF_SIZE);

    if (recv_len == -1 || recv_len == 0) {
        if (recv_len == -1)
            perror("read()");
        fprintf(stderr, "cannot get response from the server\n");
        fflush(stderr);
        close(client_tcp_fd);
        exit(EXIT_FAILURE);
    }

    if (recv_len == 1 && recv_buf[0] == '0') {
        fprintf(stdout, "the server cannot verify the audio file\n");
        fflush(stdout);
        close(client_tcp_fd);
        return 0;
    }

    if (recv_len == 1 + sizeof(in_port_t) + sizeof(int) && recv_buf[0] == '2') {
        server_udp_port = *(in_port_t *)(recv_buf + 1);
        //filesize = *(int *)(recv_buf + 1 + sizeof(in_port_t));
    } else {
        fprintf(stdout, "received unknown messages from the server\n");
        fflush(stdout);
        close(client_tcp_fd);
        return 0;
    }

    client_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    client_udp_addr.sin_family = AF_INET;
    client_udp_addr.sin_addr.s_addr = INADDR_ANY;
    client_udp_addr.sin_port = 0;
    bind(client_udp_fd, (const struct sockaddr *) &client_udp_addr, sizeof(client_udp_addr));

    client_udp_addr_len = sizeof(client_udp_addr);
    getsockname(client_udp_fd, (struct sockaddr *) &client_udp_addr, &client_udp_addr_len);
    client_udp_port = client_udp_addr.sin_port;

    *(in_port_t *)(send_buf) = client_udp_port;

    write(client_tcp_fd, (const void *) send_buf, 2);

    worker_pid = fork();

    if (worker_pid == 0) {
        close(client_tcp_fd);

        memset((void *) &act, 0, sizeof(act));
        act.sa_handler = sigquit_handler;

        sigaction(SIGQUIT, (const struct sigaction *) &act, NULL);

        cq = queue_alloc(buf_size);
        done = 0;

        log = logger_alloc();

        if (pthread_create(&producer_tid, NULL, &producer, NULL) != 0) {
            perror("pthread_create()");
            fflush(stderr);
            close(client_udp_fd);
            exit(EXIT_FAILURE);
        }

        mulawopen(&mulaw_size);
        mulaw_buf = (unsigned char *) malloc(mulaw_size);

        while (1) {
            if (gamma == 1) {
                req.tv_sec = 1;
                req.tv_nsec = 0;
            } else {
                req.tv_sec = 0;
                req.tv_nsec = 1000000000 / gamma;
            }

            sleep_ret = nanosleep((const struct timespec *) &req, &rem);

            while (sleep_ret == -1) {
                if (errno == EINTR) {
                    req = rem;
                    sleep_ret = nanosleep((const struct timespec *) &req, &rem);
                } else {
                    perror("nanosleep()");
                    fflush(stderr);
                    close(client_udp_fd);
                    exit(EXIT_FAILURE);
                }
            }

            sigemptyset(&set);
            sigaddset(&set, SIGQUIT);
            sigprocmask(SIG_BLOCK, &set, NULL);

            pthread_mutex_lock(&lock);

            if (cq -> size >= mulaw_size) {
                logger_log(log, (void *) &(cq -> size), sizeof(ull));
                for (i = 0; i < mulaw_size; i++) {
                    if (dequeue(cq, mulaw_buf + i) == -1) {
                        fprintf(stderr, "cannot dequeue\n");
                        fflush(stderr);
                        close(client_udp_fd);
                        exit(EXIT_FAILURE);
                    }
                }
                mulawwrite(mulaw_buf);
            } else {
                if (done == 1) {
                    pthread_mutex_unlock(&lock);
                    sigprocmask(SIG_UNBLOCK, &set, NULL);
                    break;
                }
            }

            pthread_mutex_unlock(&lock);

            sigprocmask(SIG_UNBLOCK, &set, NULL);
        }

        mulawclose();
        free(mulaw_buf);

        queue_destroy(cq);

        logger_write(log, fp, get_local_ip(), client_udp_port, sprint_int);
        logger_destroy(log);

    } else {
        close(client_udp_fd);
        recv_len = read(client_tcp_fd, (void *) recv_buf, MAX_BUF_SIZE);

        if (recv_len >= 1 && recv_len <= 5 && recv_buf[0] == '5') {
            kill(worker_pid, SIGQUIT);
        }

        close(client_tcp_fd);
        wait(NULL);
    }

    fclose(fp);
    return 0;
}
