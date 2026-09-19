#ifndef DISPLAY_H
#define DISPLAY_H

/* Runs the display process: opens the shared-memory segment created
 * by the acquisition process (read-only), sets up the QNX Screen
 * window/buffers, spawns the render worker pool and the
 * critical-pulse listener thread, then runs the main event/render
 * loop at ~30 FPS until the window is closed or Escape is pressed.
 * Exits the process (code 1) if the acquisition process hasn't been
 * started yet (no shared memory to attach to). Never returns under
 * normal operation. */
int run_display(void);

#endif /* DISPLAY_H */
