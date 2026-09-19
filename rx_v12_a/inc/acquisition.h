#ifndef ACQUISITION_H
#define ACQUISITION_H

/* Runs the acquisition process: takes an exclusive lock (so only one
 * instance can run), creates/owns the shared-memory segment, seeds
 * every patient slot with defaults, fetches each patient's initial
 * clinical data, then spawns the TCP pool (vitals) and UDP workers
 * (ECG waveform) and blocks forever. Exits the process (code 1) if
 * another acquisition instance is already running. Never returns
 * under normal operation. */
int run_acquisition(void);

#endif /* ACQUISITION_H */
