#ifndef LOGGER_H
#define LOGGER_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>


#define INIT_CAPACITY 50000


typedef unsigned long long ull;

typedef struct entry {
    void* data;
    ull timestamp;
} entry_t, *entry_ptr;

typedef struct logger {
    int size;
    int capacity;
    entry_ptr entries;
} logger_t, *logger_ptr;


logger_ptr logger_alloc();

int logger_destroy(logger_ptr logger);

int logger_log(logger_ptr logger, void* data, size_t data_size);

int logger_write(logger_ptr logger, FILE *fp, struct in_addr ip, in_port_t port,
        void (*print)(char *, void *));


#endif //LOGGER_H
