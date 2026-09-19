#ifndef FONT_H
#define FONT_H

#include <stdint.h>

/* Fills bits[5] with a 3x5 bitmap glyph for character c (each row is
 * a 3-bit mask, bit 2 = leftmost column). Lowercase is folded to
 * uppercase; unsupported characters (including space) render blank. */
void get_font_bits(char c, uint8_t bits[5]);

#endif /* FONT_H */
