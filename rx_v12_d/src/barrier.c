#include "barrier.h"

void my_barrier_init(my_barrier_t *b, int count) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->count = b->trip_count = count;
    b->phase = 0;
}

void my_barrier_wait(my_barrier_t *b) {
    pthread_mutex_lock(&b->mutex);
    int my_phase = b->phase;
    b->count--;
    if (b->count == 0) {
        b->phase = !b->phase;
        b->count = b->trip_count;
        pthread_cond_broadcast(&b->cond);
    } else {
        while (b->phase == my_phase) pthread_cond_wait(&b->cond, &b->mutex);
    }
    pthread_mutex_unlock(&b->mutex);
}
