#include "rtci.h"
#include "patient_data.h"

#include <stdio.h>

/* Implemented by the external RTCI engine (linked separately). */
extern int calculate_total_apache_ii(RawData raw_data);
extern int calculate_total_news2(RawData data);

float calculate_rtci(int p_idx) {
    RawData current_state = g_shm->patients[p_idx].clinical_data;

    current_state.hr = g_shm->patients[p_idx].hr;
    current_state.spo2 = g_shm->patients[p_idx].spo2;
    current_state.rr = g_shm->patients[p_idx].rr;

    int sys = 120, dia = 80;
    if (sscanf((const char*)g_shm->patients[p_idx].bp, "%d/%d", &sys, &dia) == 2) {
        current_state.sbp = sys;
    } else {
        current_state.sbp = 120;
    }

    int news2 = calculate_total_news2(current_state);
    int apache2 = calculate_total_apache_ii(current_state);

    return (float)news2 + ((float)apache2 * 0.15f);
}
