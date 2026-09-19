#ifndef AI_CLIENT_H
#define AI_CLIENT_H

/* Fetches an AI-generated clinical summary for a patient from the
 * history/AI server (Pi 2) over TCP and stores it in
 * g_ai_summary_text. arg is the patient id, cast from int via
 * pthread_create's void* argument. Updates g_ai_state: 1 while
 * loading, 2 when done (success or error message). Intended to be
 * run detached from a UI click handler; model inference can take up
 * to ~2 minutes, hence the long socket timeout. */
void* ai_network_worker(void* arg);

#endif /* AI_CLIENT_H */
