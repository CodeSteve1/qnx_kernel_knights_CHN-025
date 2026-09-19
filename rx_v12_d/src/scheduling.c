#include "scheduling.h"
#include "config.h"

#include <pthread.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/ioctl.h>

void elevate_main_thread(void) {
    struct sched_param sp; int min_prio = sched_get_priority_min(SCHED_RR);
    sp.sched_priority = min_prio + MAIN_THREAD_PRIORITY_OFFSET;
    pthread_setschedparam(pthread_self(), SCHED_RR, &sp);
}

void elevate_render_thread(void) {
    struct sched_param sp; int min_prio = sched_get_priority_min(SCHED_RR);
    sp.sched_priority = min_prio + RENDER_THREAD_PRIORITY_OFFSET;
    pthread_setschedparam(pthread_self(), SCHED_RR, &sp);
}

void elevate_io_thread(void) {
    struct sched_param sp; int min_prio = sched_get_priority_min(SCHED_RR);
    sp.sched_priority = min_prio + IO_THREAD_PRIORITY_OFFSET;
    pthread_setschedparam(pthread_self(), SCHED_RR, &sp);
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    int on = 1;
    ioctl(fd, FIONBIO, &on);
}
