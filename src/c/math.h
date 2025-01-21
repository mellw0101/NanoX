#pragma once

#define round_short(x) ((x) >= 0 ? (short)((x) + 0.5) : (short)((x) - 0.5))

/* Return`s a rounded xterm-256 scale value from a 8-bit rgb value.  */
#define xterm_byte_scale(bit) round_short(((double)bit / 255) * 5)

/* Return the xterm-256 index for a given 8bit rgb value. */
#define xterm_color_index(r, g, b) \
  short(16 + (36 * xterm_byte_scale(r)) + (6 * xterm_byte_scale(g)) + xterm_byte_scale(b))
