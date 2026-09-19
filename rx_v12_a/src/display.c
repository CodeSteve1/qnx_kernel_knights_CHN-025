#include "display.h"
#include "patient_data.h"
#include "config.h"
#include "barrier.h"
#include "render.h"
#include "draw.h"
#include "rtci.h"
#include "scheduling.h"
#include "time_utils.h"
#include "ai_client.h"
#include "history_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <screen/screen.h>

/* --- QNX Specific Headers (pulse notification from acquisition) --- */
#include <sys/neutrino.h>
#include <sys/dispatch.h>

/* =========================================================================
 * CRITICAL-ALERT PULSE LISTENER
 *
 * Just wakes up on each pulse from the acquisition process. The pulse
 * itself carries no state we need to keep: every frame already
 * recomputes the critical list straight from shared memory
 * (refresh_critical_patient_list), so this thread's only job is to
 * keep the QNX channel alive and draining.
 * ========================================================================= */
static void* QnxPulseListenerThread(void* arg) {
    (void)arg;
    name_attach_t *attach = name_attach(NULL, ATTACH_NAME, 0);
    if (attach == NULL) {
        perror("name_attach failed"); exit(EXIT_FAILURE);
    }
    struct _pulse pulse;
    printf("Display Process listening for Critical QNX Pulses...\n");

    while (1) {
        int rcvid = MsgReceive(attach->chid, &pulse, sizeof(pulse), NULL);
        if (rcvid == 0 && pulse.code == _PULSE_CODE_MINAVAIL) {}
    }
    return NULL;
}

/* =========================================================================
 * CRITICAL-LIST RANKING
 * ========================================================================= */
typedef struct { int id; float severity; } CritPat;

static int compare_crit(const void* a, const void* b) {
    const CritPat* pa = (const CritPat*)a;
    const CritPat* pb = (const CritPat*)b;
    if (pb->severity > pa->severity) return 1;
    if (pb->severity < pa->severity) return -1;
    return 0;
}

/* Scans every patient slot for the critical flag, scores each one
 * with the RTCI engine, sorts most-severe first, and updates the
 * global critical-grid state (g_num_critical, g_critical_patients,
 * g_crit_cols/g_crit_rows) that RenderWorker and the click handlers
 * read. crit_list is scratch space owned by the caller (sized
 * NUM_PATIENTS) so this function doesn't need to allocate per frame. */
static void refresh_critical_patient_list(CritPat crit_list[NUM_PATIENTS]) {
    int c_count = 0;
    for (int i = 0; i < NUM_PATIENTS; i++) {
        if (g_shm->patients[i].critical) {
            crit_list[c_count].id = i;
            crit_list[c_count].severity = calculate_rtci(i);
            c_count++;
        }
    }

    qsort(crit_list, c_count, sizeof(CritPat), compare_crit);

    if (c_count > NUM_RENDER_THREADS) c_count = NUM_RENDER_THREADS;
    g_num_critical = c_count;
    for (int i = 0; i < c_count; i++) g_critical_patients[i] = crit_list[i].id;

    if (g_num_critical == 0) { }
    else if (g_num_critical == 1) { g_crit_cols = 1; g_crit_rows = 1; }
    else if (g_num_critical <= 4) { g_crit_cols = 2; g_crit_rows = 2; }
    else if (g_num_critical <= 6) { g_crit_cols = 3; g_crit_rows = 2; }
    else if (g_num_critical <= 9) { g_crit_cols = 3; g_crit_rows = 3; }
    else { g_crit_cols = 5; g_crit_rows = 2; }
}

/* =========================================================================
 * INPUT HANDLING
 * ========================================================================= */

/* Pointer-down while the details modal is open: either hits the
 * "AI SUMMARY" button (kicks off ai_network_worker) or lands outside
 * the modal box (closes it). */
static void handle_modal_click(int px, int py) {
    int mx = 150, my = 120;
    int mw = g_screen_width - 300, mh = g_screen_height - 240;
    int btn_ai_w = 180, btn_ai_h = 35;
    int btn_ai_x = mx + 20, btn_ai_y = my + 175;

    if (px >= btn_ai_x && px <= btn_ai_x + btn_ai_w && py >= btn_ai_y && py <= btn_ai_y + btn_ai_h) {
        if (g_ai_state != 1) {
            pthread_t a_thread;
            pthread_create(&a_thread, NULL, ai_network_worker, (void*)(intptr_t)g_show_modal_for_patient);
            pthread_detach(a_thread);
        }
    }
    else if (px < mx || px > mx + mw || py < my || py > my + mh) {
        g_show_modal_for_patient = -1;
    }
}

/* Pointer-down over the patient grid (no modal open): checks whether
 * the click landed on a cell's DETAILS button and, if so, kicks off
 * fetch_worker for that patient. */
static void handle_grid_click(int px, int py) {
    int clicked_p_id = -1;
    int limit = g_num_critical > 0 ? g_num_critical : PATIENTS_PER_PAGE;
    int cols = g_num_critical > 0 ? g_crit_cols : NUM_COLS;
    int rows = g_num_critical > 0 ? g_crit_rows : NUM_ROWS;

    for (int i=0; i<limit; i++) {
        int cell_w = g_screen_width / cols;
        int cell_h = g_screen_height / rows;
        int cx = (i % cols) * cell_w;
        int cy = (i / cols) * cell_h;

        int btn_w = 110, btn_h = 35;
        int btn_x = cx + cell_w - btn_w - 15;
        int btn_y = cy + 10;

        if (px >= btn_x && px <= btn_x+btn_w && py >= btn_y && py <= btn_y+btn_h) {
            int p_idx = (g_num_critical > 0) ? g_critical_patients[i] : (g_current_page * PATIENTS_PER_PAGE) + i;
            clicked_p_id = g_shm->patients[p_idx].id;
            break;
        }
    }
    if (clicked_p_id != -1) {
        pthread_t f_thread;
        pthread_create(&f_thread, NULL, fetch_worker, (void*)(intptr_t)clicked_p_id);
        pthread_detach(f_thread);
    }
}

static void handle_pointer_click(int px, int py) {
    if (g_show_modal_for_patient != -1) handle_modal_click(px, py);
    else handle_grid_click(px, py);
}

/* Page navigation (n/d/Right-arrow, p/a/Left-arrow) and Esc-to-quit. */
static void handle_keyboard_key(int sym) {
    if (sym == 'n' || sym == 'd' || sym == SYM_RIGHT || sym == SYM_X11_RIGHT) {
        g_current_page = (g_current_page + 1) % NUM_PAGES;
    } else if (sym == 'p' || sym == 'a' || sym == SYM_LEFT || sym == SYM_X11_LEFT) {
        g_current_page = (g_current_page - 1 + NUM_PAGES) % NUM_PAGES;
    } else if (sym == 27) exit(0);
}

/* Drains all pending Screen events for this frame and dispatches
 * them: window close, pointer clicks, and key presses. */
static void process_screen_events(screen_context_t screen_ctx, screen_event_t event) {
    while (!screen_get_event(screen_ctx, event, 0)) {
        int event_type; screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &event_type);
        if (event_type == SCREEN_EVENT_NONE) break;
        if (event_type == SCREEN_EVENT_CLOSE) exit(0);

        if (event_type == SCREEN_EVENT_POINTER) {
            int buttons;
            screen_get_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &buttons);
            if (buttons) {
                int pos[2]; screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, pos);
                handle_pointer_click(pos[0], pos[1]);
            }
        }

        if (event_type == SCREEN_EVENT_KEYBOARD) {
            int flags;
            screen_get_event_property_iv(event, SCREEN_PROPERTY_FLAGS, &flags);
            if (flags & KEY_DOWN) {
                int sym;
                screen_get_event_property_iv(event, SCREEN_PROPERTY_SYM, &sym);
                handle_keyboard_key(sym);
            }
        }
    }
}

/* =========================================================================
 * OVERLAY RENDERING (drawn on top of the per-cell grid RenderWorker
 * threads already produced for this frame)
 * ========================================================================= */

/* Bottom-right corner: current page indicator, or a red banner while
 * any patient is critical (the grid is showing the critical layout
 * instead of the normal paged one). */
static void render_status_overlay(void) {
    if (g_num_critical == 0) {
        char page_str[16]; char* ps = page_str;
        *ps++ = 'P'; *ps++ = ':'; *ps++ = ' ';
        ps = fast_itoa(g_current_page + 1, ps);
        *ps++ = '/'; *ps++ = '1'; *ps++ = '0'; *ps = '\0';
        draw_text_fast(g_render_ptr, g_render_stride, g_screen_width - 150, g_screen_height - 30, page_str, 0xFFFF00FF, 3, 999);
    } else {
        draw_text_fast(g_render_ptr, g_render_stride, g_screen_width - 320, g_screen_height - 30, "CRITICAL ALERT(S)", 0xFFFF0000, 3, 999);
    }
}

/* Patient details modal: bullet-point clinical profile, the AI
 * SUMMARY button, and (depending on g_ai_state) either a spinner /
 * scanner-bar loading animation or the AI assessment text once it
 * has arrived. No-op when no modal is open. */
static void render_modal_overlay(int anim_tick) {
    if (g_show_modal_for_patient == -1) return;

    int mx = 150, my = 120;
    int mw = g_screen_width - 300, mh = g_screen_height - 240;

    // Modal Background
    fill_rect_fast(g_render_ptr, g_render_stride, mx, my, mw, mh, 0xFF112233);

    // Modal Borders
    draw_line_fast(g_render_ptr, g_render_stride, mx, my, mx+mw, my, 0xFFFFFFFF);
    draw_line_fast(g_render_ptr, g_render_stride, mx, my+mh, mx+mw, my+mh, 0xFFFFFFFF);
    draw_line_fast(g_render_ptr, g_render_stride, mx, my, mx, my+mh, 0xFFFFFFFF);
    draw_line_fast(g_render_ptr, g_render_stride, mx+mw, my, mx+mw, my+mh, 0xFFFFFFFF);

    // Modal Header
    char hdr[96]; sprintf(hdr, "PATIENT %d CLINICAL PROFILE (CLICK OUTSIDE BUTTON TO CLOSE)", g_show_modal_for_patient);
    draw_text_fast(g_render_ptr, g_render_stride, mx + 20, my + 15, hdr, 0xFF00BFFF, 3, mw - 40);

    // Patient details bullet points
    draw_text_fast(g_render_ptr, g_render_stride, mx + 20, my + 45, g_modal_text, 0xFF00FF00, 2, mw - 40);

    // "AI SUMMARY" Button
    int btn_ai_w = 180, btn_ai_h = 35;
    int btn_ai_x = mx + 20, btn_ai_y = my + 175;
    uint32_t btn_col = (g_ai_state == 1) ? 0xFF888888 : 0xFF007ACC;
    fill_rect_fast(g_render_ptr, g_render_stride, btn_ai_x, btn_ai_y, btn_ai_w, btn_ai_h, btn_col);
    draw_text_fast(g_render_ptr, g_render_stride, btn_ai_x + 18, btn_ai_y + 9, "AI SUMMARY", 0xFFFFFFFF, 2, btn_ai_w);

    int ai_area_y = btn_ai_y + btn_ai_h + 20;

    // REAL-TIME LOADING ANIMATION
    if (g_ai_state == 1) {
        const char spin_chars[] = "|/-\\";
        char spin_c = spin_chars[(anim_tick / 4) % 4];
        char status_msg[64];
        snprintf(status_msg, sizeof(status_msg), "GENERATING AI SUMMARY [%c] ...", spin_c);
        draw_text_fast(g_render_ptr, g_render_stride, mx + 20, ai_area_y, status_msg, 0xFFFFFF00, 2, mw - 40);

        // Animated glowing scanner bar
        int bar_x = mx + 20, bar_y = ai_area_y + 22;
        int bar_w = 320, bar_h = 8;
        fill_rect_fast(g_render_ptr, g_render_stride, bar_x, bar_y, bar_w, bar_h, 0xFF222222);

        int slider_w = 60;
        int slider_pos = (anim_tick * 5) % (bar_w - slider_w);
        fill_rect_fast(g_render_ptr, g_render_stride, bar_x + slider_pos, bar_y, slider_w, bar_h, 0xFF00FF64);
    }
    // RENDER AI SUMMARY RESULT
    else if (g_ai_state == 2) {
        draw_text_fast(g_render_ptr, g_render_stride, mx + 20, ai_area_y, "AI CLINICAL ASSESSMENT:", 0xFF00BFFF, 2, mw - 40);
        draw_text_fast(g_render_ptr, g_render_stride, mx + 20, ai_area_y + 20, g_ai_summary_text, 0xFFFFFF00, 2, mw - 40);
    }
}

/* =========================================================================
 * MAIN WORKFLOW
 * ========================================================================= */
int run_display(void) {
    printf("Starting Display Process...\n");
    elevate_main_thread();
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) { perror("shm_open failed. Run Acquisition (-a) first."); exit(1); }
    g_shm = mmap(NULL, sizeof(SharedSystemData), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (g_shm == MAP_FAILED) { perror("mmap failed"); exit(1); }

    pthread_t listener; pthread_create(&listener, NULL, QnxPulseListenerThread, NULL);

    // --- QNX Screen window & double buffer setup ---
    screen_context_t screen_ctx; screen_window_t screen_win; screen_buffer_t screen_buf[2];
    screen_create_context(&screen_ctx, SCREEN_APPLICATION_CONTEXT);

    int display_count = 0;
    screen_get_context_property_iv(screen_ctx, SCREEN_PROPERTY_DISPLAY_COUNT, &display_count);
    if (display_count > 0) {
        screen_display_t *displays = calloc(display_count, sizeof(screen_display_t));
        screen_get_context_property_pv(screen_ctx, SCREEN_PROPERTY_DISPLAYS, (void **)displays);
        int dims[2] = {0, 0};
        screen_get_display_property_iv(displays[0], SCREEN_PROPERTY_SIZE, dims);
        if (dims[0] > 0 && dims[1] > 0) { g_screen_width = dims[0]; g_screen_height = dims[1]; }
        free(displays);
    }

    screen_create_window(&screen_win, screen_ctx);
    int rect[2] = {g_screen_width, g_screen_height};
    int format = SCREEN_FORMAT_RGBA8888, usage = SCREEN_USAGE_READ | SCREEN_USAGE_WRITE, swap_interval = 1;
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_SIZE, rect);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_FORMAT, &format);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_SWAP_INTERVAL, &swap_interval);
    screen_create_window_buffers(screen_win, 2);
    screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&screen_buf);

    screen_event_t event; screen_create_event(&event);

    // --- Render worker pool ---
    my_barrier_init(&render_start_barrier, NUM_RENDER_THREADS + 1);
    my_barrier_init(&render_end_barrier, NUM_RENDER_THREADS + 1);

    for (int i = 0; i < NUM_RENDER_THREADS; i++) {
        pthread_t t; pthread_create(&t, NULL, RenderWorker, (void*)(intptr_t)i);
    }

    const int TARGET_FPS = 30; const int FRAME_TIME_MS = 1000 / TARGET_FPS;
    long long last_frame_ts = get_current_time_ms();
    int buffer_idx = 0; CritPat crit_list[NUM_PATIENTS];
    int anim_tick = 0;

    // --- Main event / render loop ---
    while (1) {
        process_screen_events(screen_ctx, event);

        // Pace to TARGET_FPS
        long long current_time = get_current_time_ms();
        long long elapsed = current_time - last_frame_ts;
        if (elapsed < FRAME_TIME_MS) usleep((FRAME_TIME_MS - elapsed) * 1000);
        last_frame_ts = get_current_time_ms();
        anim_tick++;

        // Rank critical patients & decide grid layout for this frame
        refresh_critical_patient_list(crit_list);

        // Hand the back buffer to the render worker pool
        screen_get_buffer_property_pv(screen_buf[buffer_idx], SCREEN_PROPERTY_POINTER, (void **)&g_render_ptr);
        screen_get_buffer_property_iv(screen_buf[buffer_idx], SCREEN_PROPERTY_STRIDE, &g_render_stride);

        my_barrier_wait(&render_start_barrier);
        my_barrier_wait(&render_end_barrier);

        // Overlays drawn by the main thread after the grid is done
        render_status_overlay();
        render_modal_overlay(anim_tick);

        // Present
        int dirty_rects[4] = {0, 0, g_screen_width, g_screen_height};
        screen_post_window(screen_win, screen_buf[buffer_idx], 1, dirty_rects, 0);
        buffer_idx = (buffer_idx + 1) % 2;
    }
    return 0;
}
