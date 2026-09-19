#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

/* Bounds-checked pixel write into a 32bpp (RGBA8888) framebuffer.
 * Relies on g_screen_width/g_screen_height (patient_data.h) for
 * clipping, so any .c file using this macro must include that too. */
#define FAST_PIXEL(buf, stride, x, y, color) \
    if ((unsigned)(x) < (unsigned)g_screen_width && (unsigned)(y) < (unsigned)g_screen_height && (x) >= 0 && (y) >= 0) \
        *((uint32_t*)((buf) + (y) * (stride) + (x) * 4)) = (color);

void draw_line_fast(char *buffer, int stride, int x0, int y0, int x1, int y1, uint32_t color);
void fill_rect_fast(char *buffer, int stride, int x, int y, int w, int h, uint32_t color);

/* Writes the base-10 representation of val at buf (no NUL terminator
 * appended), returns a pointer just past the last digit written. */
char* fast_itoa(long long val, char* buf);

/* Draws text using the 3x5 bitmap font (see font.h), wrapping at
 * max_width and on '\n'. Returns the horizontal distance advanced on
 * the final line (useful for chaining several draw_text_fast calls
 * on one row, e.g. "ID:1 H:70 O:98% ..."). */
int draw_text_fast(char *buffer, int stride, int x, int y, const char *text, uint32_t color, int scale, int max_width);

#endif /* DRAW_H */
