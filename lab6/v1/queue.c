#include "queue.h"


struct circular_queue * queue_alloc(int capacity) {
    struct circular_queue *cq;
    cq = malloc(sizeof(struct circular_queue));
    cq -> buf = malloc(capacity * sizeof(unsigned char));
    cq -> capacity = capacity;
    cq -> size = 0;
    cq -> head = cq -> tail = 0;
    return cq;
}


void queue_destroy(struct circular_queue * cq) {
    if (cq == NULL)
        return;
    free(cq -> buf);
    free(cq);
}


int enqueue(struct circular_queue *cq, unsigned char c) {
    if (cq -> size == cq -> capacity)
        return -1;
    cq -> buf[cq -> tail] = c;
    cq -> tail = (cq -> tail + 1) % (cq -> capacity);
    ++(cq -> size);
    return 0;
}


int dequeue(struct circular_queue *cq, unsigned char *ret) {
    if (cq -> size == 0)
        return -1;
    *ret = cq -> buf[cq -> head];
    cq -> head = (cq -> head + 1) % (cq -> capacity);
    --(cq -> size);
    return 0;
}
