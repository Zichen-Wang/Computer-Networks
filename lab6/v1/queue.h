#ifndef QUEUE_H
#define QUEUE_H


#include <stdlib.h>


struct circular_queue {
    unsigned char *buf;
    int capacity, size;
    int head, tail;
};


struct circular_queue * queue_alloc(int);

void queue_destroy(struct circular_queue *);

int enqueue(struct circular_queue *, unsigned char);

int dequeue(struct circular_queue *, unsigned char *);


#endif //QUEUE_H
