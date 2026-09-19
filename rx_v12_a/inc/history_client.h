#ifndef HISTORY_CLIENT_H
#define HISTORY_CLIENT_H

/* Fetches the full clinical profile (name, diagnosis, allergies,
 * history, doctor notes, ...) for a patient from the history server
 * and formats it into g_modal_text, then opens the details modal
 * for that patient (sets g_show_modal_for_patient). arg is the
 * patient id, cast from int via pthread_create's void* argument. */
void* fetch_worker(void* arg);

/* Synchronous, startup-time fetch: pulls a patient's baseline
 * clinical parameters from the history server into
 * g_shm->patients[p_idx].clinical_data, so the RTCI engine has real
 * data to score from the moment acquisition starts. */
void fetch_initial_clinical_data(int p_idx);

#endif /* HISTORY_CLIENT_H */
