#include "patient_data.h"

#include <stddef.h>

SharedSystemData* g_shm = NULL;

int g_screen_width = 1920;
int g_screen_height = 1080;
volatile int g_current_page = 0;
char *g_render_ptr = NULL;
int g_render_stride = 0;
int g_render_quit = 0;
int g_pulse_coid = -1;

volatile int g_show_modal_for_patient = -1;
char g_modal_text[4096] = {0};
char g_ai_summary_text[4096] = {0};
volatile int g_ai_state = 0;

volatile int g_num_critical = 0;
volatile int g_critical_patients[NUM_RENDER_THREADS];
volatile int g_crit_cols = 1;
volatile int g_crit_rows = 1;
