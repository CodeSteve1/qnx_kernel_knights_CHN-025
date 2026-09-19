#ifndef BARRIER_H
#define BARRIER_H

#include <pthread.h>

/* Simple reusable two-phase barrier (used to gate the render worker
 * pool: all threads must reach it before the frame is considered
 * "drawn" and can be presented). */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int trip_count;
    int phase;
} my_barrier_t;

void my_barrier_init(my_barrier_t *b, int count);
void my_barrier_wait(my_barrier_t *b);

#endif /* BARRIER_H */
