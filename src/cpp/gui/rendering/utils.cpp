/** @file renutils.c

  @author  Melwin Svensson.
  @date    25-3-2025.

 */
#include "../../../include/prototypes.h"


/* ----------------------------- pixbreadth ----------------------------- */

float pixnbreadth_prev(const char *const restrict string, long len, const char *const restrict prev_char) {
  ASSERT(string);
  ASSERT(len != 0);
  float ret;
  const char *ch, *prev;
  for (ret=0, ch=string, prev=prev_char; len>(ch - string); ++ch) {
    ret += glyph_width(ch, prev, gui_font_get_font(gui->font));
    prev = ch;
  }
  return ret;
}

float pixnbreadth(const char *const restrict string, long len) {
  ASSERT(string);
  ASSERT(len != 0);
  return pixnbreadth_prev(string, len, NULL);
}

float pixbreadth(texture_font_t *const font, const char *const restrict string) {
  ASSERT(font);
  ASSERT(string);
  float ret = 0;
  for (const char *ch=string, *prev=NULL; *ch; ++ch) {
    ret += glyph_width(ch, prev, font);
    prev = ch;
  }
  return ret;
}

float pixbreadth(GuiFont *const font, const char *const restrict string) {
  ASSERT(font);
  ASSERT(string);
  float ret = 0;
  for (const char *ch=string, *prev=NULL; *ch; ++ch) {
    ret += glyph_width(ch, prev, gui_font_get_font(font));
    prev = ch;
  }
  return ret;
}
