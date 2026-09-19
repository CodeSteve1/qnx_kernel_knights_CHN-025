#include "render.h"
#include "patient_data.h"
#include "draw.h"
#include "scheduling.h"
#include "config.h"

#include <stdint.h>

my_barrier_t render_start_barrier;
my_barrier_t render_end_barrier;

void* RenderWorker(void* arg) {
    elevate_render_thread();
    int thread_idx = (int)(intptr_t)arg;

    uint32_t c_ecg = 0xFF00FF64, c_hr = 0xFF00FF64, c_spo2 = 0xFF00BFFF;
    uint32_t c_bp = 0xFFFFFFFF, c_rr = 0xFFFFD700;
    uint32_t c_lat = 0xFF88CCFF; // Light blue for latency

    while (1) {
        my_barrier_wait(&render_start_barrier);
        if (g_render_quit) break;

        int p_idx, cx, cy, cell_w, cell_h;

        if (g_num_critical > 0) {
            int total_cells = g_crit_cols * g_crit_rows;
            if (thread_idx >= total_cells) { my_barrier_wait(&render_end_barrier); continue; }

            int col = thread_idx % g_crit_cols;
            int row = thread_idx / g_crit_cols;
            cell_w = g_screen_width / g_crit_cols;
            cell_h = g_screen_height / g_crit_rows;
            cx = col * cell_w; cy = row * cell_h;

            for (int y = cy; y < cy + cell_h; y++) {
                uint32_t *row_ptr = (uint32_t *)(g_render_ptr + y * g_render_stride);
                for (int x = cx; x < cx + cell_w; x++) row_ptr[x] = 0xFF0C0C0C;
            }

            if (thread_idx >= g_num_critical) { my_barrier_wait(&render_end_barrier); continue; }
            p_idx = g_critical_patients[thread_idx];

        } else {
            p_idx = (g_current_page * PATIENTS_PER_PAGE) + thread_idx;
            int col = thread_idx % NUM_COLS;
            int row = thread_idx / NUM_COLS;
            cell_w = g_screen_width / NUM_COLS;
            cell_h = g_screen_height / NUM_ROWS;
            cx = col * cell_w; cy = row * cell_h;

            for (int y = cy; y < cy + cell_h; y++) {
                uint32_t *row_ptr = (uint32_t *)(g_render_ptr + y * g_render_stride);
                for (int x = cx; x < cx + cell_w; x++) row_ptr[x] = 0xFF0C0C0C;
            }
        }

        int scale_x = cell_w / 140;
        int scale_y = (cell_h * 0.3f) / 6.0f;
        int font_scale = (scale_x < scale_y) ? scale_x : scale_y;
        if (font_scale < 1) font_scale = 1; if (font_scale > 5) font_scale = 5;

        uint32_t c_border = g_shm->patients[p_idx].critical ? 0xFFFF0000 : 0xFF333333;
        draw_line_fast(g_render_ptr, g_render_stride, cx, cy + cell_h - 1, cx + cell_w, cy + cell_h - 1, c_border);
        draw_line_fast(g_render_ptr, g_render_stride, cx + cell_w - 1, cy, cx + cell_w - 1, cy + cell_h, c_border);
        if (g_shm->patients[p_idx].critical) {
            draw_line_fast(g_render_ptr, g_render_stride, cx, cy, cx + cell_w, cy, c_border);
            draw_line_fast(g_render_ptr, g_render_stride, cx, cy, cx, cy + cell_h, c_border);
        }

        char txt_buf[32]; char *p_str; int cursor = cx + 4;
        uint32_t c_id_text = g_shm->patients[p_idx].critical ? 0xFFFF0000 : 0xFFFFFFFF;

        p_str = txt_buf; *p_str++='I'; *p_str++='D'; *p_str++=':'; p_str = fast_itoa(g_shm->patients[p_idx].id, p_str); *p_str++=' '; *p_str='\0';
        cursor += draw_text_fast(g_render_ptr, g_render_stride, cursor, cy + 4, txt_buf, c_id_text, font_scale, 999);

        p_str = txt_buf; *p_str++='H'; *p_str++=':'; p_str = fast_itoa(g_shm->patients[p_idx].hr, p_str); *p_str++=' '; *p_str='\0';
        cursor += draw_text_fast(g_render_ptr, g_render_stride, cursor, cy + 4, txt_buf, c_hr, font_scale, 999);

        p_str = txt_buf; *p_str++='O'; *p_str++=':'; p_str = fast_itoa(g_shm->patients[p_idx].spo2, p_str); *p_str++='%'; *p_str++=' '; *p_str='\0';
        cursor += draw_text_fast(g_render_ptr, g_render_stride, cursor, cy + 4, txt_buf, c_spo2, font_scale, 999);

        p_str = txt_buf; *p_str++='R'; *p_str++='R'; *p_str++=':'; p_str = fast_itoa(g_shm->patients[p_idx].rr, p_str); *p_str++=' '; *p_str='\0';
        cursor += draw_text_fast(g_render_ptr, g_render_stride, cursor, cy + 4, txt_buf, c_rr, font_scale, 999);

        p_str = txt_buf; *p_str++='B'; *p_str++='P'; *p_str++=':';
        char* bp_src = g_shm->patients[p_idx].bp; while(*bp_src) *p_str++ = *bp_src++; *p_str++=' '; *p_str='\0';
        cursor += draw_text_fast(g_render_ptr, g_render_stride, cursor, cy + 4, txt_buf, c_bp, font_scale, 999);

        // --- LATENCY DISPLAY ---
        p_str = txt_buf; *p_str++='L'; *p_str++='A'; *p_str++='T'; *p_str++=':';
        p_str = fast_itoa(g_shm->patients[p_idx].last_latency, p_str);
        *p_str++='m'; *p_str++='s'; *p_str++=' '; *p_str='\0';
        cursor += draw_text_fast(g_render_ptr, g_render_stride, cursor, cy + 4, txt_buf, c_lat, font_scale, 999);
        // -----------------------

        int btn_w = 110, btn_h = 35;
        int btn_x = cx + cell_w - btn_w - 15;
        int btn_y = cy + 10;
        fill_rect_fast(g_render_ptr, g_render_stride, btn_x, btn_y, btn_w, btn_h, 0xFFFFFFFF);
        draw_text_fast(g_render_ptr, g_render_stride, btn_x + 14, btn_y + 8, "DETAILS", 0xFF000000, 2, btn_w);

        int baseline_y = cy + (cell_h * 0.70f);
        float amplitude = cell_h * 0.35f;
        float step_x = (float)cell_w / GRAPH_PTS;
        int head = g_shm->patients[p_idx].history_idx, gap = GRAPH_PTS / 15;

        for (int p = 1; p < GRAPH_PTS; p++) {
            int prev_p = p - 1, in_gap = 0, prev_in_gap = 0;
            if (head + gap < GRAPH_PTS) {
                if (p >= head && p <= head + gap) in_gap = 1;
                if (prev_p >= head && prev_p <= head + gap) prev_in_gap = 1;
            } else {
                if (p >= head || p <= (head + gap) % GRAPH_PTS) in_gap = 1;
                if (prev_p >= head || prev_p <= (head + gap) % GRAPH_PTS) prev_in_gap = 1;
            }
            if (in_gap || prev_in_gap) continue;
            int px1 = cx + (int)(prev_p * step_x); int py1 = baseline_y - (int)(g_shm->patients[p_idx].ecg_history[prev_p] * amplitude);
            int px2 = cx + (int)(p * step_x); int py2 = baseline_y - (int)(g_shm->patients[p_idx].ecg_history[p] * amplitude);
            if (py1 < cy) py1 = cy; else if (py1 >= cy + cell_h) py1 = cy + cell_h - 1;
            if (py2 < cy) py2 = cy; else if (py2 >= cy + cell_h) py2 = cy + cell_h - 1;
            draw_line_fast(g_render_ptr, g_render_stride, px1, py1, px2, py2, c_ecg);
        }
        my_barrier_wait(&render_end_barrier);
    }
    return NULL;
}
