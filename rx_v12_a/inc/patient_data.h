#ifndef PATIENT_DATA_H
#define PATIENT_DATA_H

#include "config.h"
#include "PatientParameters.h"   /* RTCI engine: defines RawData */

/* Per-patient live vitals + latest fetched clinical record, stored in
 * shared memory so the acquisition process and the display process
 * can both see it. */
typedef struct {
    int id;
    volatile int hr;
    volatile int spo2;
    volatile int rr;
    char bp[16];
    volatile float ecg_history[GRAPH_PTS];
    volatile int history_idx;
    volatile long long last_latency;
    volatile int critical;
    RawData clinical_data;
} PatientData;

typedef struct {
    PatientData patients[NUM_PATIENTS];
} SharedSystemData;

/* ---- Shared memory pointer (mmap'd by both processes; defined in globals.c) ---- */
extern SharedSystemData* g_shm;

/* ---- Display / rendering state ---- */
extern int g_screen_width;
extern int g_screen_height;
extern volatile int g_current_page;
extern char *g_render_ptr;
extern int g_render_stride;
extern int g_render_quit;
extern int g_pulse_coid;

/* ---- Modal & AI summary state ---- */
extern volatile int g_show_modal_for_patient;
extern char g_modal_text[4096];
extern char g_ai_summary_text[4096];
extern volatile int g_ai_state; /* 0=Idle, 1=Loading from Pi 2, 2=Done */

/* ---- Multi-critical tracking state ---- */
extern volatile int g_num_critical;
extern volatile int g_critical_patients[NUM_RENDER_THREADS];
extern volatile int g_crit_cols;
extern volatile int g_crit_rows;

#endif /* PATIENT_DATA_H */
