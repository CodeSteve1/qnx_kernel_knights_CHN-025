#include "draw.h"
#include "font.h"
#include "patient_data.h" /* g_screen_width / g_screen_height, used by FAST_PIXEL */

#include <stdlib.h> /* abs */

void draw_line_fast(char *buffer, int stride, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    int max_pixels = g_screen_width + g_screen_height;
    while (max_pixels-- > 0) {
        FAST_PIXEL(buffer, stride, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void fill_rect_fast(char *buffer, int stride, int x, int y, int w, int h, uint32_t color) {
    for (int iy = y; iy < y + h; iy++) {
        for (int ix = x; ix < x + w; ix++) {
            FAST_PIXEL(buffer, stride, ix, iy, color);
        }
    }
}

char* fast_itoa(long long val, char* buf) {
    if (val == 0) { *buf++ = '0'; return buf; }
    if (val < 0) { *buf++ = '-'; val = -val; }
    char temp[32]; char *tp = temp;
    while (val > 0) { *tp++ = (val % 10) + '0'; val /= 10; }
    while (tp > temp) { *buf++ = *--tp; }
    return buf;
}

int draw_text_fast(char *buffer, int stride, int x, int y, const char *text, uint32_t color, int scale, int max_width) {
    int cursor_x = x, cursor_y = y;
    uint8_t bits[5];

    for (const char *p = text; *p; p++) {
        if (*p == '\n') {
            cursor_y += 6 * scale;
            cursor_x = x;
            continue;
        }
        if (cursor_x + 4 * scale > x + max_width) {
            cursor_y += 6 * scale;
            cursor_x = x;
        }
        if (cursor_y + 5 * scale >= g_screen_height) break;

        get_font_bits(*p, bits);
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                if (bits[row] & (1 << (2 - col))) {
                    for (int sy = 0; sy < scale; sy++)
                        for (int sx = 0; sx < scale; sx++)
                            FAST_PIXEL(buffer, stride, cursor_x + col * scale + sx, cursor_y + row * scale + sy, color);
                }
            }
        }
        cursor_x += 4 * scale;
    }
    return cursor_x - x;
}
