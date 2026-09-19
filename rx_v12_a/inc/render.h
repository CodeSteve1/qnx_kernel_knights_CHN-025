#ifndef RENDER_H
#define RENDER_H

#include "barrier.h"

/* Synchronize the main display loop (display.c) with the pool of
 * per-cell render worker threads: render_start_barrier releases the
 * workers to draw one frame, render_end_barrier signals back that
 * they're all done before the buffer is presented. Both are
 * initialized once, in run_display(). */
extern my_barrier_t render_start_barrier;
extern my_barrier_t render_end_barrier;

/* Renders one grid cell (vitals line, ECG waveform, DETAILS button)
 * per frame for a single patient, gated by the two barriers above.
 * arg is the render thread's slot index (0..NUM_RENDER_THREADS-1),
 * which maps to either a page slot or a critical-alert grid slot
 * depending on g_num_critical. */
void* RenderWorker(void* arg);

#endif /* RENDER_H */
