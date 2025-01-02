#include "../../include/prototypes.h"

#ifdef HAVE_GLFW

float gui_glyph_width(const char *current, const char *prev, markup_t *markup) {
  float ret = 0.0f;
  texture_glyph_t *glyph = texture_font_get_glyph(markup->font, current);
  if (prev) {
    ret += texture_glyph_get_kerning(glyph, prev);
  }
  ret += glyph->advance_x;
  return ret;
}

/* Calculates the total length of all glyphs in a string to the index. */
float gui_calculate_string_offset(const char *string, const char *previous_char, Ulong index, markup_t *markup) {
  if (!index) {
    return 0;
  }
  const char *current = &string[0];
  const char *prev = previous_char;
  float ret = 0;
  for (Uint i = 0; string[index - 1] && i < (index + 1); ++i) {
    current = &string[i];
    ret += gui_glyph_width(current, prev, markup);
    prev = current;
    if (i + 1 == index) {
      break;
    }
  }
  return ret;
}

/* Calculate the length the line number takes up in a given line. */
float gui_calculate_lineno_offset(linestruct *line, markup_t *markup) {
  char linenobuf[margin + 1];
  sprintf(linenobuf, "%*lu ", (margin - 1), line->lineno);
  return gui_calculate_string_offset(linenobuf, NULL, margin, markup);
}

/* Calculates cursor x position for the gui. */
float gui_calculate_cursor_x(markup_t *markup) {
  float ret;
  /* Convert the line data into a display string. */
  Ulong from_col  = get_page_start(wideness(openfile->current->data, openfile->current_x));
  char *converted = display_string(openfile->current->data, from_col, editwincols, TRUE, FALSE);
  /* When line numbers are turned on we calculate the combined length of the lineno, seperator and current line data. */
  if (ISSET(LINE_NUMBERS)) {
    float offset = gui_calculate_lineno_offset(openfile->current, markup);
    ret = (gui_calculate_string_offset(converted, (char *)" ", (xplustabs() - from_col), markup) + offset);
  }
  /* Otherwise, just calculate the current line data len. */
  else {
    ret = gui_calculate_string_offset(converted, NULL, (xplustabs() - from_col), markup);
  }
  free(converted);
  return ret;
}

/* Calculates the vertical offset of a given line within the window, based on its position relative to the top of the editable area. */
float calculate_line_y_offset(linestruct *line, markup_t *markup) {
  long relative_row = (line->lineno - openfile->edittop->lineno);
  /* If the cursor is above the editelement, return one line above the entire window. */
  if (relative_row < 0) {
    return -FONT_HEIGHT(markup->font);
  }
  /* Otherwise, if the cursor is below the editelement, return one line below the entire window. */
  else if (relative_row > editwinrows) {
    return window_height;
  }
  return (relative_row * FONT_HEIGHT(markup->font));
}

/* Calculates cursor y position for the gui. */
float gui_calculate_cursor_y(markup_t *markup) {
  return calculate_line_y_offset(openfile->current, markup);
}

void add_glyph(char *current, char *previous, vertex_buffer_t *buffer, markup_t *markup, vec2 *pen) {
  texture_glyph_t *glyph = texture_font_get_glyph(markup->font, current);
  if (previous) {
    pen->x += texture_glyph_get_kerning(glyph, previous);
  }
  float r = markup->foreground_color.r;
  float g = markup->foreground_color.g;
  float b = markup->foreground_color.b;
  float a = markup->foreground_color.a;
  int x0 = (pen->x + glyph->offset_x);
  int y0 = (pen->y - glyph->offset_y);
  int x1 = (x0 + glyph->width);
  int y1 = (y0 + glyph->height);
  float s0 = glyph->s0;
  float t0 = glyph->t0;
  float s1 = glyph->s1;
  float t1 = glyph->t1;
  Uint indices[] = { 0, 1, 2, 0, 2, 3 };
  vertex_t vertices[] = {
    {(float)x0, (float)y0, 0, s0, t0, r, g, b, a},
    {(float)x0, (float)y1, 0, s0, t1, r, g, b, a},
    {(float)x1, (float)y1, 0, s1, t1, r, g, b, a},
    {(float)x1, (float)y0, 0, s1, t0, r, g, b, a}
  };
  vertex_buffer_push_back(buffer, vertices, 4, indices, 6);
  pen->x += glyph->advance_x;
  pen->y += glyph->advance_y;
}

/* Update projection for a shader. */
void update_projection_uniform(Uint shader) {
  glUseProgram(shader);
  glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, FALSE, &projection[0][0]);
}

#endif