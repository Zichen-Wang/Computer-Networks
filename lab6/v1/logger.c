#include "logger.h"


logger_ptr logger_alloc() {
    logger_ptr logger = malloc(sizeof(logger_t));
    if (logger == NULL) {
        return NULL;
    }
    logger -> size = 0;
    logger -> capacity = INIT_CAPACITY;
    logger -> entries = malloc(sizeof(entry_t) * INIT_CAPACITY);
    return logger;
}

int logger_destroy(logger_ptr logger) {
    int i;
    if (logger == NULL) {
        return -1;
    }

    for (i = 0; i < logger -> size; i++)
        free((logger -> entries[i]).data);

    free(logger -> entries);
    free(logger);

    return 0;
}

int logger_log(logger_ptr logger, void *data, size_t data_size) {
    struct timeval tv;
    if (logger == NULL || logger -> size == logger -> capacity) {
        return -1;
    }

    (logger -> entries[logger -> size]).data = malloc(data_size);
    memcpy((logger -> entries[logger -> size]).data, data, data_size);

    gettimeofday(&tv, NULL);
    (logger -> entries[logger -> size]).timestamp = tv.tv_sec * 1000000ull + tv.tv_usec;

    ++(logger -> size);
    return 0;
}

int logger_write(logger_ptr logger, FILE *fp, struct in_addr ip, in_port_t port,
        void (*print_to_buf)(char *, void *)) {

    int i;
    char log_buf[100];

    if (logger == NULL) {
        return -1;
    }

    for (i = 0; i < logger -> size; i++) {
        sprintf(log_buf, "%s\t%hu\t%llu\t", inet_ntoa(ip), ntohs(port), logger -> entries[i].timestamp);
        print_to_buf(log_buf + strlen(log_buf), logger -> entries[i].data);
        fprintf(fp, "%s\n", log_buf);
    }

    return 0;
}
