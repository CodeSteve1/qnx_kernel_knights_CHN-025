// main.c
//
// Entry point for the patient monitoring system. This process runs as
// one of two roles, selected by command-line flag:
//
//   -a   Acquisition: owns the shared-memory patient table, ingests
//        vitals over TCP and ECG waveform samples over UDP, and fetches
//        each patient's clinical record from the history server.
//        (see src/acquisition.c)
//
//   -d   Display: attaches to that shared memory read-only, renders the
//        patient grid via the QNX Screen API, and handles touch/mouse
//        and keyboard input, including the patient details modal and
//        on-demand AI clinical summaries. (see src/display.c)
//
// Everything else (drawing primitives, the RTCI clinical scoring engine,
// the JSON scrapers, the network clients, thread scheduling helpers) is
// implemented as a small library of modules under include/ and src/.

#include "config.h"
#include "patient_data.h"
#include "acquisition.h"
#include "display.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

static void handle_signal(int sig) {
    (void)sig;
    fprintf(stderr, "\nShutting down...\n");
    if (g_shm != NULL && g_shm != MAP_FAILED) munmap(g_shm, sizeof(SharedSystemData));
    shm_unlink(SHM_NAME);
    _exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        return run_display();
    } else if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        return run_acquisition();
    } else {
        printf("Usage:\n  %s -a  (Run Network Acquisition)\n  %s -d  (Run Critical Display)\n", argv[0], argv[0]);
    }
    return 0;
}
