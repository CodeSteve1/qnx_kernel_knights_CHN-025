#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>

/* Current wall-clock time in milliseconds (CLOCK_REALTIME). Used for
 * network latency stamping and frame-rate pacing. */
static inline long long get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

#endif /* TIME_UTILS_H */
