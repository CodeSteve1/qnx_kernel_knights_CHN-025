#ifndef CONFIG_H
#define CONFIG_H

/* ---- Patient grid / paging ---- */
#define NUM_PATIENTS 100
#define PATIENTS_PER_PAGE 10
#define NUM_PAGES (NUM_PATIENTS / PATIENTS_PER_PAGE)

#define NUM_COLS 2
#define NUM_ROWS 5

/* ---- Network ports ---- */
#define TCP_START_PORT 5000
#define UDP_START_PORT 6000
#define LOCK_PORT 4999

/* ---- ECG waveform ---- */
#define GRAPH_PTS 400
#define NUM_RENDER_THREADS PATIENTS_PER_PAGE

/* ---- TCP acquisition pool ---- */
#define TCP_POOL_SIZE 10
#define PATIENTS_PER_TCP_THREAD (NUM_PATIENTS / TCP_POOL_SIZE)

/* ---- Second Pi: history & AI server ---- */
#define HISTORY_SERVER_IP "172.17.155.173"  /* Pi 2 IP running server_v1.py */
#define HISTORY_SERVER_PORT 5000

/* ---- SCHED_RR priority offsets ---- */
#define IO_THREAD_PRIORITY_OFFSET 10
#define RENDER_THREAD_PRIORITY_OFFSET 20
#define MAIN_THREAD_PRIORITY_OFFSET 25

/* ---- Input key codes ---- */
#define KEY_DOWN 0x00000001
#define SYM_RIGHT 0xf024
#define SYM_LEFT  0xf023
#define SYM_X11_RIGHT 0xff53
#define SYM_X11_LEFT  0xff51

/* ---- Shared memory / QNX pulse channel names ---- */
#define SHM_NAME "/patient_data_v2"
#define ATTACH_NAME "CriticalDisplayService"

#endif /* CONFIG_H */
