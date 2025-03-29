/** @file gui/font/utils.cpp

  @author  Melwin Svensson.
  @date    26-3-2025.

 */
#include "../../../include/prototypes.h"


/* Return the pixel where the baseline of the given `row` is.  Note that this is 0 indexed, so it starts at row 0 for the first row. */
float row_baseline_pixel(long row, texture_font_t *const font) {
  ASSERT(font);
  return ((row * FONT_HEIGHT(font)) + font->ascender);
}

void row_top_bot_pixel(long lineno, texture_font_t *const font, float *const top, float *const bot) {
  ASSERT(font);
  float baseline = row_baseline_pixel(lineno, font);
  *top = (baseline - font->ascender);
  *bot = (baseline - font->descender);
}

void line_add_cursor(long lineno, texture_font_t *const font, vertex_buffer_t *const buf, vec4 color, float xpos, float yoffset) {
  ASSERT(font);
  float top, bot, x0, x1, y0, y1;
  /* Use the NULL texture as the cursor texture, so just a rectangle. */
  texture_glyph_t *glyph = texture_font_get_glyph(font, NULL);
  ALWAYS_ASSERT(glyph);
  row_top_bot_pixel(lineno, font, &top, &bot);
  x0 = (int)xpos;
  y0 = (int)(top + yoffset);
  x1 = (int)(x0 + 1);
  y1 = (int)(bot + yoffset);
  Uint indices[] = { 0, 1, 2, 0, 2, 3 };
  vertex_t vertices[] = {
    // Position   Texture               Color
    {  x0,y0,0,   glyph->s0, glyph->t0, color.r,color.g,color.b,color.a },
    {  x0,y1,0,   glyph->s0, glyph->t1, color.r,color.g,color.b,color.a },
    {  x1,y1,0,   glyph->s1, glyph->t1, color.r,color.g,color.b,color.a },
    {  x1,y0,0,   glyph->s1, glyph->t0, color.r,color.g,color.b,color.a }
  };
  vertex_buffer_push_back(buf, vertices, 4, indices, 6);
}

