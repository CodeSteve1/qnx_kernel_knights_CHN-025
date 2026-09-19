#ifndef SCHEDULING_H
#define SCHEDULING_H

/* Raises the calling thread's SCHED_RR priority using the offsets
 * from config.h. Call once at the start of a thread that needs
 * deterministic real-time behavior (main/display, render workers,
 * network I/O workers each get a different priority band). */
void elevate_main_thread(void);
void elevate_render_thread(void);
void elevate_io_thread(void);

/* Puts a socket fd into non-blocking mode (fcntl O_NONBLOCK + FIONBIO). */
void set_nonblocking(int fd);

#endif /* SCHEDULING_H */
