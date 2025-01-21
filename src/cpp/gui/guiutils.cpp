#include "../../include/prototypes.h"

#ifdef HAVE_GLFW

/* Upload a atlas texture. */
void upload_texture_atlas(texture_atlas_t *withatlas) {
  glBindTexture(GL_TEXTURE_2D, withatlas->id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, withatlas->width, withatlas->height, 0, GL_RED, GL_UNSIGNED_BYTE, withatlas->data);
}

/* Returns the width of a glyph character considering kerning from previous character. */
float glyph_width(const char *current, const char *prev, texture_font_t *withfont) {
  float ret = 0.0f;
  texture_glyph_t *glyph = texture_font_get_glyph(withfont, current);
  if (prev) {
    ret += texture_glyph_get_kerning(glyph, prev);
  }
  ret += glyph->advance_x;
  return ret;
}

/* Calculates the total length of all glyphs in a string to the index. */
float string_pixel_offset(const char *string, const char *previous_char, Ulong index, texture_font_t *withfont) {
  if (!index) {
    return 0.0f;
  }
  const char *current = &string[0];
  const char *prev = previous_char;
  float ret = 0.0f;
  for (Uint i = 0; string[index - 1] && i < (index + 1); ++i) {
    current = &string[i];
    ret += glyph_width(current, prev, withfont);
    prev = current;
    if ((i + 1) == index) {
      break;
    }
  }
  return ret;
}

float pixel_breadth(texture_font_t *withfont, const char *text) {
  float ret = 0.0f;
  for (const char *cur = text, *prev = NULL; *cur; ++cur) {
    ret += glyph_width(cur, prev, withfont);
    prev = cur;
  }
  return ret;
}

/* Returns character index from x pixel position in string. */
long index_from_mouse_x(const char *string, Uint len, texture_font_t *withfont, float start_x) {
  if (mousepos.x <= start_x) {
    return 0;
  }
  const char *cur;
  const char *prev = (ISSET(LINE_NUMBERS) ? " " : NULL);
  float st_x  = 0.0f;
  float end_x = 0.0f;
  Ulong i = 0;
  for (; i < len; ++i) {
    /* Set the start x pos to the end pos x. */
    st_x = end_x;
    cur = &string[i];
    if (*cur == '\t') {
      cur = " ";
      end_x += (glyph_width(cur, prev, withfont) * tabsize);
    }
    else {
      end_x += glyph_width(cur, prev, withfont);
    }
    prev = cur;
    if (mousepos.x > (st_x + start_x) && mousepos.x < (end_x + start_x)) {
      break;
    }
  }
  /* If the x position is bigger then the string, return the index of the last char. */
  return actual_x(string, wideness(string, i));
}

/* Returns character index from x pixel position in string. */
long index_from_mouse_x(const char *string, texture_font_t *withfont, float offset) {
  return index_from_mouse_x(string, strlen(string), withfont, offset);
}

/* Get the y pixel region that the given row span`s. */
static void get_row_y_pixel_region(texture_font_t *withfont, Uint row, Uint *top_y, Uint *bot_y, float offset) _NOTHROW {
  *bot_y = ((row * FONT_HEIGHT(withfont)) - withfont->descender + offset);
  *top_y = (*bot_y - FONT_HEIGHT(withfont));
}

/* Return the line from that a pixel is on, inside the editelement. */
linestruct *line_from_mouse_y(texture_font_t *withfont, float offset) {
  /* The current row. */
  int row = 0;
  /* The range in pixels that the row occupies. */
  Uint top_y, bot_y;
  /* The current line corresponding to the current row. */
  linestruct *line = openfile->edittop;
  while (line && ++row < editwinrows) {
    get_row_y_pixel_region(withfont, row, &top_y, &bot_y, offset);
    if ((mousepos.y > top_y && mousepos.y < bot_y) || (mousepos.y < top_y && mousepos.y < bot_y)) {
      break;
    }
    line = line->next;
  }
  return line;
}

/* Return the line and the index in the line, from a x and y position. */
linestruct *line_and_index_from_mousepos(texture_font_t *withfont, Ulong *index) {
  /* If outside editelement confinement. */
  if (mousepos.y < editelement->pos.y) {
    *index = 0;
    return openfile->edittop;
  }
  linestruct *line = line_from_mouse_y(withfont, editelement->pos.y);
  if (line) {
    *index = index_from_mouse_x(line->data, withfont, editelement->pos.x);
  }
  return line;
}

/* Calculate the length the line number takes up in a given line. */
float get_line_number_pixel_offset(linestruct *line, texture_font_t *withfont) {
  char linenobuf[margin + 1];
  sprintf(linenobuf, "%*lu ", (margin - 1), line->lineno);
  return string_pixel_offset(linenobuf, NULL, margin, withfont);
}

/* Calculates cursor x position for the gui. */
float line_pixel_x_pos(linestruct *line, Ulong index, texture_font_t *withfont) {
  float ret = 0.0f;
  /* Convert the line data into a display string. */
  Ulong from_col  = get_page_start(wideness(line->data, index));
  char *converted = display_string(line->data, from_col, editwincols, TRUE, FALSE);
  /* When line numbers are turned on we calculate the combined length of the lineno, seperator and current line data. */
  if (ISSET(LINE_NUMBERS)) {
    float offset = get_line_number_pixel_offset(line, withfont);
    ret = (string_pixel_offset(converted, " ", (wideness(line->data, index) - from_col), withfont) + offset);
  }
  /* Otherwise, just calculate the current line data len. */
  else {
    ret = string_pixel_offset(converted, NULL, (wideness(line->data, index) - from_col), withfont);
  }
  free(converted);
  return ret;
}

/* Calculates cursor x position for the gui. */
float cursor_pixel_x_pos(texture_font_t *withfont) {
  return line_pixel_x_pos(openfile->current, openfile->current_x, withfont);
}

/* Calculates the vertical offset of a given line within the window, based on its position relative to the top of the editable area. */
float line_y_pixel_offset(linestruct *line, texture_font_t *withfont) {
  long relative_row = (line->lineno - openfile->edittop->lineno);
  /* If the cursor is above the editelement, return one line above the entire window. */
  if (relative_row < 0) {
    return -FONT_HEIGHT(withfont);
  }
  /* Otherwise, if the cursor is below the editelement, return one line below the entire window. */
  else if (relative_row > editwinrows) {
    return window_height;
  }
  return ((relative_row * FONT_HEIGHT(withfont)) - withfont->descender + editelement->pos.y);
}

/* Calculates cursor y position for the gui. */
float cursor_pixel_y_pos(texture_font_t *withfont) {
  return line_y_pixel_offset(openfile->current, withfont);
}

/* Add one glyph to 'buffer' to be rendered.  At position pen. */
void add_glyph(const char *current, const char *previous, vertex_buffer_t *buffer, texture_font_t *withfont, vec4 color, vec2 *penpos) {
  texture_glyph_t *glyph = texture_font_get_glyph(withfont, current);
  if (previous) {
    penpos->x += texture_glyph_get_kerning(glyph, previous);
  }
  int x0 = (penpos->x + glyph->offset_x);
  int y0 = (penpos->y - glyph->offset_y);
  int x1 = (x0 + glyph->width);
  int y1 = (y0 + glyph->height);
  Uint indices[] = { 0, 1, 2, 0, 2, 3 };
  vertex_t vertices[] = {
    {(float)x0, (float)y0, 0, glyph->s0, glyph->t0, color.r, color.g, color.b, color.a},
    {(float)x0, (float)y1, 0, glyph->s0, glyph->t1, color.r, color.g, color.b, color.a},
    {(float)x1, (float)y1, 0, glyph->s1, glyph->t1, color.r, color.g, color.b, color.a},
    {(float)x1, (float)y0, 0, glyph->s1, glyph->t0, color.r, color.g, color.b, color.a}
  };
  vertex_buffer_push_back(buffer, vertices, 4, indices, 6);
  penpos->x += glyph->advance_x;
}

void vertex_buffer_add_string(vertex_buffer_t *buffer, const char *string, Ulong slen, const char *previous, texture_font_t *withfont, vec4 color, vec2 *penpos) {
  const char *cur;
  const char *prev = previous;
  for (Ulong i = 0; i < slen; ++i) {
    cur = &string[i];
    add_glyph(cur, prev, buffer, withfont, color, penpos);
    prev = cur;
  }
}

/* Add a cursor in `buf` using the NULL texture from `font` in the color of `color`, at `at`. */
void add_cursor(texture_font_t *font, vertex_buffer_t *buf, vec4 color, vec2 at) {
  /* Use the NULL texture as the cursor texture, so just a rectangle. */
  texture_glyph_t *glyph = texture_font_get_glyph(font, NULL);
  float x0 = (int)at.x;
  float y0 = (int)at.y;
  float x1 = (int)(x0 + 1);
  float y1 = (int)(y0 + FONT_HEIGHT(font));
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

/* Add cursor to buffer at `openfile->current_x` in `openfile->current->data`. */
void add_openfile_cursor(texture_font_t *font, vertex_buffer_t *buf, vec4 color) {
  add_cursor(font, buf, color, vec2(cursor_pixel_x_pos(font), cursor_pixel_y_pos(font)));
}

/* Update projection for a shader. */
void update_projection_uniform(Uint shader) {
  glUseProgram(shader);
  {
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, FALSE, projection.data);
  }
}

/* Get ui element from current mouse position. */
uielementstruct *element_from_mousepos(void) {
  ivec2 pos(mousepos);
  if (gridmap.contains(pos)) {
    return gridmap.get(pos);
  }
  return NULL;
}

/* Add a ui element`s lable to the vertex-buffer. */
void vertex_buffer_add_element_lable(uielementstruct *element, texture_font_t *withfont, vertex_buffer_t *buffer) {
  if (!element->flag.is_set<UIELEMENT_HAS_LABLE>()) {
    die("%s: Element had no text.\n", __func__);
  }
  vec2 pos(element->pos.x, (element->pos.y + FONT_HEIGHT(withfont) + withfont->descender));
  vertex_buffer_add_string(buffer, element->lable, element->lablelen, NULL, withfont, element->textcolor, &pos);
}

/* Returns 'TRUE' when 'ancestor' is an ancestor to e or is e itself. */
bool is_ancestor(uielementstruct *e, uielementstruct *ancestor) {
  uielementstruct *element = e;
  while (element) {
    if (element == ancestor) {
      return TRUE;
    }
    element = element->parent;
  }
  return FALSE;
}

/* Return's a `vec4` of scaled 8bit values to the scale of `0-1.0f`. */
vec4 color_idx_to_vec4(int index) {
#define PF8BIT(bit_value) (bit_value / 255.0f)
#define VEC4_8BIT(r, g, b, a) vec4(PF8BIT(r), PF8BIT(g), PF8BIT(b), a)
  switch (index) {
    case FG_VS_CODE_RED: {
      return VEC4_8BIT(205, 49, 49, 1);
    }
    case FG_VS_CODE_GREEN: {
      return VEC4_8BIT(13, 188, 121, 1);
    }
    case FG_VS_CODE_YELLOW: {
      return VEC4_8BIT(229, 229, 16, 1);
    }
    case FG_VS_CODE_BLUE: {
      return VEC4_8BIT(36, 114, 200, 1);
    }
    case FG_VS_CODE_MAGENTA: {
      return VEC4_8BIT(188, 63, 188, 1);
    }
    case FG_VS_CODE_CYAN: {
      return VEC4_8BIT(17, 168, 205, 1);
    }
    case FG_VS_CODE_BRIGHT_RED: {
      return VEC4_8BIT(241, 76, 76, 1);
    }
    case FG_VS_CODE_BRIGHT_GREEN: {
      return VEC4_8BIT(35, 209, 139, 1);
    }
    case FG_VS_CODE_BRIGHT_YELLOW: {
      return VEC4_8BIT(245, 245, 67, 1);
    }
    case FG_VS_CODE_BRIGHT_BLUE: {
      return VEC4_8BIT(59, 142, 234, 1);
    }
    case FG_VS_CODE_BRIGHT_MAGENTA: {
      return VEC4_8BIT(214, 112, 214, 1);
    }
    case FG_VS_CODE_BRIGHT_CYAN: {
      return VEC4_8BIT(41, 184, 219, 1);
    }
    default: {
      return vec4(1.0f);
    }
  }
}

#endif
