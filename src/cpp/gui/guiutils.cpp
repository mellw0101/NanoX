#include "../../include/prototypes.h"

#ifdef HAVE_GLFW

/* Upload a atlas texture. */
void upload_texture_atlas(texture_atlas_t *atlas) {
  ASSERT(atlas);
  glBindTexture(GL_TEXTURE_2D, atlas->id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas->width, atlas->height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas->data);
}

/* Returns the width of a glyph character considering kerning from previous character. */
float glyph_width(const char *current, const char *prev, texture_font_t *font) {
  ASSERT(current);
  ASSERT(font);
  float ret = 0;
  texture_glyph_t *glyph = texture_font_get_glyph(font, current);
  ALWAYS_ASSERT(glyph);
  if (prev) {
    ret += texture_glyph_get_kerning(glyph, prev);
  }
  ret += glyph->advance_x;
  return ret;
}

/* Calculates the total length of all glyphs in a string to the index. */
float string_pixel_offset(const char *string, const char *previous_char, Ulong index, texture_font_t *font) {
  ASSERT(string);
  ASSERT(font);
  const char *current = &string[0];
  const char *prev = previous_char;
  float ret = 0.0f;
  /* When the passed index is zero, just return zero. */
  if (!index) {
    return 0.0f;
  }
  for (Uint i = 0; string[index - 1] && i < (index + 1); ++i) {
    current = &string[i];
    ret += glyph_width(current, prev, font);
    prev = current;
    if ((i + 1) == index) {
      break;
    }
  }
  return ret;
}

float pixel_breadth(texture_font_t *font, const char *text) {
  float ret = 0.0f;
  for (const char *cur = text, *prev = NULL; *cur; ++cur) {
    ret += glyph_width(cur, prev, font);
    prev = cur;
  }
  return ret;
}

/* Returns character index from x pixel position in string. */
long index_from_mouse_x(const char *string, Uint len, texture_font_t *font, float offset) {
  ASSERT(string);
  ASSERT(font);
  const char *cur;
  const char *prev = (ISSET(LINE_NUMBERS) ? " " : NULL);
  float st_x  = 0.0f;
  float end_x = 0.0f;
  Ulong i;
  if (mouse_gui_get_x() <= (double)offset) {
    return 0;
  }
  for (i=0; i<len; ++i) {
    /* Set the start x pos to the end pos x. */
    st_x = end_x;
    cur = &string[i];
    if (*cur == '\t') {
      cur = " ";
      end_x += (glyph_width(cur, prev, font) * tabsize);
    }
    else {
      end_x += glyph_width(cur, prev, font);
    }
    prev = cur;
    if (mouse_gui_get_x() > (double)(st_x + offset) && mouse_gui_get_x() < (double)(end_x + offset)) {
      break;
    }
  }
  /* If the x position is bigger then the string, return the index of the last char. */
  return actual_x(string, wideness(string, i));
}

/* Returns character index from x pixel position in string. */
long index_from_mouse_x(const char *string, texture_font_t *font, float offset) {
  return index_from_mouse_x(string, strlen(string), font, offset);
}

/* Get the y pixel region that the given row span`s. */
static void get_row_y_pixel_region(texture_font_t *font, Uint row, Uint *top_y, Uint *bot_y, float offset) _NOTHROW {
  ASSERT(font);
  ASSERT(top_y);
  ASSERT(bot_y);
  *bot_y = ((row * FONT_HEIGHT(font)) - font->descender + offset);
  *top_y = (*bot_y - FONT_HEIGHT(font));
}

/* Return the line from that a pixel is on, inside the editelement. */
linestruct *line_from_mouse_y(texture_font_t *font, float offset) {
  ASSERT(font);
  /* The current row. */
  int row = 0;
  /* The range in pixels that the row occupies. */
  Uint top_y, bot_y;
  /* The current line corresponding to the current row. */
  linestruct *line = openeditor->openfile->edittop;
  while (line && ++row < openeditor->rows) {
    get_row_y_pixel_region(font, row, &top_y, &bot_y, offset);
    if ((mouse_gui_get_y() > top_y && mouse_gui_get_y() < bot_y) || (mouse_gui_get_y() < top_y && mouse_gui_get_y() < bot_y)) {
      break;
    }
    line = line->next;
  }
  return line;
}

/* Return the line and the index in the line, from a x and y position. */
linestruct *line_and_index_from_mousepos(texture_font_t *const font, Ulong *const index) {
  /* When debugging is enabled, assert everything we use. */
  ASSERT(openeditor->openfile->edittop);
  // ASSERT(openeditor->text);
  ASSERT(font);
  ASSERT(index);
  linestruct *line = line_from_cursor_pos(openeditor);
  *index = index_from_pix_xpos(line->data, mouse_gui_get_x(), openeditor->text->x, font);
  return line;
}

/* Calculate the length the line number takes up in a given line. */
float get_line_number_pixel_offset(linestruct *line, texture_font_t *font) {
  ASSERT(line);
  ASSERT(font);
  char linenobuf[margin + 1];
  sprintf(linenobuf, "%*lu ", (margin - 1), line->lineno);
  return string_pixel_offset(linenobuf, NULL, margin, font);
}

/* Calculates cursor x position for the gui. */
float line_pixel_x_pos(linestruct *line, Ulong index, texture_font_t *font) {
  ASSERT(line);
  ASSERT(font);
  float ret = 0;
  /* Convert the line data into a display string. */
  Ulong from_col  = get_page_start(wideness(line->data, index), openeditor->cols);
  char *converted = display_string(line->data, from_col, openeditor->cols, TRUE, FALSE);
  /* When line numbers are turned on we calculate the combined length of the lineno, seperator and current line data. */
  if (ISSET(LINE_NUMBERS)) {
    ret = get_line_number_pixel_offset(line, font);
  }
  ret += string_pixel_offset(converted, NULL, (wideness(line->data, index) - from_col), font);
  free(converted);
  return ret;
}

/* Calculates cursor x position for the gui. */
float cursor_pixel_x_pos(texture_font_t *font) {
  ASSERT(font);
  return line_pixel_x_pos(openeditor->openfile->current, openeditor->openfile->current_x, font);
}

/* Calculates the vertical offset of a given line within the window, based on its position relative to the top of the editable area. */
float line_y_pixel_offset(linestruct *line, texture_font_t *font) {
  ASSERT(openeditor->openfile->edittop);
  ASSERT(openeditor->text);
  ASSERT(gui);
  ASSERT(line);
  ASSERT(font);
  long relative_row = (line->lineno - openeditor->openfile->edittop->lineno + 1);
  /* If the cursor is above the editelement, return one line above the entire window. */
  if (relative_row < 0) {
    return -FONT_HEIGHT(font);
  }
  /* Otherwise, if the cursor is below the editelement, return one line below the entire window. */
  else if (relative_row > openeditor->rows) {
    return gl_window_height();
  }
  return ((relative_row * FONT_HEIGHT(font)) + openeditor->text->y);
}

/* Calculates cursor y position for the gui. */
float cursor_pixel_y_pos(texture_font_t *font) {
  return line_y_pixel_offset(openeditor->openfile->current, font);
}

/* Add one glyph to 'buffer' to be rendered.  At position pen. */
void add_glyph(const char *current, const char *prev, vertex_buffer_t *buf, texture_font_t *font, vec4 color, vec2 *penpos) {
  ASSERT(current);
  ASSERT(buf);
  ASSERT(font);
  ASSERT(penpos);
  float x0, x1, y0, y1;
  Uint indices[] = { 0, 1, 2, 0, 2, 3 };
  vertex_t vertices[4];
  texture_glyph_t *glyph = texture_font_get_glyph(font, current);
  ALWAYS_ASSERT(glyph);
  if (prev) {
    penpos->x += texture_glyph_get_kerning(glyph, prev);
  }
  x0 = (int)(penpos->x + glyph->offset_x);
  y0 = (int)(penpos->y - glyph->offset_y);
  x1 = (int)(x0 + glyph->width);
  y1 = (int)(y0 + glyph->height);
  vertices[0] = { x0,y0, glyph->s0,glyph->t0, color.r,color.g,color.b,color.a };
  vertices[1] = { x0,y1, glyph->s0,glyph->t1, color.r,color.g,color.b,color.a };
  vertices[2] = { x1,y1, glyph->s1,glyph->t1, color.r,color.g,color.b,color.a };
  vertices[3] = { x1,y0, glyph->s1,glyph->t0, color.r,color.g,color.b,color.a };
  vertex_buffer_push_back(buf, vertices, 4, indices, 6);
  penpos->x += glyph->advance_x;
}

void vertex_buffer_add_string(vertex_buffer_t *buf, const char *string, Ulong slen, const char *previous, texture_font_t *font, vec4 color, vec2 *penpos) {
  ASSERT(buf);
  ASSERT(string);
  ASSERT(font);
  ASSERT(penpos);
  const char *cur;
  const char *prev = previous;
  for (Ulong i = 0; i < slen; ++i) {
    cur = &string[i];
    add_glyph(cur, prev, buf, font, color, penpos);
    prev = cur;
  }
}

void vertex_buffer_add_mbstr(vertex_buffer_t *buf, const char *string, Ulong len, const char *previous, Font *const font, vec4 color, vec2 *penpos) {
  ASSERT(buf);
  ASSERT(string);
  ASSERT(font);
  ASSERT(penpos);
  const char *cur = string;
  const char *prev = previous;
  while (*cur && cur < (string + len)) {
    add_glyph(cur, prev, buf, font_get_font(font), color, penpos);
    prev = cur;
    cur += char_length(cur);
  }
}


/* Add a cursor in `buf` using the NULL texture from `font` in the color of `color`, at `at`. */
void add_cursor(texture_font_t *font, vertex_buffer_t *buf, vec4 color, vec2 at) {
  ASSERT(font);
  ASSERT(buf);
  /* Use the NULL texture as the cursor texture, so just a rectangle. */
  texture_glyph_t *glyph = texture_font_get_glyph(font, NULL);
  ALWAYS_ASSERT(glyph);
  float x0 = (int)at.x;
  float y0 = (int)(at.y - FONT_HEIGHT(font));
  float x1 = (int)(x0 + 1);
  float y1 = (int)(y0 + FONT_HEIGHT(font));
  Uint indices[] = { 0, 1, 2, 0, 2, 3 };
  vertex_t vertices[] = {
    // Position   Texture               Color
    {  x0,y0,   glyph->s0, glyph->t0, color.r,color.g,color.b,color.a },
    {  x0,y1,   glyph->s0, glyph->t1, color.r,color.g,color.b,color.a },
    {  x1,y1,   glyph->s1, glyph->t1, color.r,color.g,color.b,color.a },
    {  x1,y0,   glyph->s1, glyph->t0, color.r,color.g,color.b,color.a }
  };
  vertex_buffer_push_back(buf, vertices, 4, indices, 6);
}

/* Add cursor to buffer at `openfile->current_x` in `openfile->current->data`. */
void add_openfile_cursor(texture_font_t *font, vertex_buffer_t *buf, vec4 color) {
  add_cursor(font, buf, color, vec2(cursor_pixel_x_pos(font), cursor_pixel_y_pos(font)));
}

/* Update projection for a shader. */
// void update_projection_uniform(Uint shader) {
//   ASSERT(gui);
//   ASSERT(gui->projection);
//   ALWAYS_ASSERT(shader);
//   glUseProgram(shader);
//   {
//     glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, FALSE, gui->projection->data);
//   }
// }

/* Add a ui element`s lable to the vertex-buffer. */
// void vertex_buffer_add_element_lable(guielement *element, texture_font_t *font, vertex_buffer_t *buf) {
//   ASSERT(element);
//   ASSERT(font);
//   ASSERT(buf);
//   ASSERT(element->flag.is_set<GUIELEMENT_HAS_LABLE>());
//   ASSERT(element->lable);
//   vec2 pos(element->pos.x, (element->pos.y + FONT_HEIGHT(font) + font->descender));
//   vertex_buffer_add_string(buf, element->lable, element->lablelen, NULL, font, element->textcolor, &pos);
// }

// /* Add a ui element`s lable to the vertex-buffer, offset by `offset`. */
// void vertex_buffer_add_element_lable_offset(guielement *element, texture_font_t *font, vertex_buffer_t *buf, vec2 offset) {
//   ASSERT(element);
//   ASSERT(font);
//   ASSERT(buf);
//   ASSERT(element->flag.is_set<GUIELEMENT_HAS_LABLE>());
//   ASSERT(element->lable);
//   vec2 pos(element->pos.x, (element->pos.y + FONT_HEIGHT(font) + font->descender));
//   pos += offset;
//   vertex_buffer_add_string(buf, element->lable, element->lablelen, NULL, font, element->textcolor, &pos);
// }

// /* Returns 'TRUE' when 'ancestor' is an ancestor to e or is e itself. */
// bool is_ancestor(guielement *e, guielement *ancestor) {
//   if (!e || !ancestor) {
//     return FALSE;
//   }
//   guielement *element = e;
//   while (element) {
//     if (element == ancestor) {
//       return TRUE;
//     }
//     element = element->parent;
//   }
//   return FALSE;
// }

/* Return's a `vec4` of the values that a color index used for terminal use represents. */
vec4 color_idx_to_vec4(int index) _NOTHROW {
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
    case FG_COMMENT_GREEN: {
      return VEC4_8BIT(0, 77, 0, 1);
    }
    case EDITOR_TOPBAR_BUTTON_ENTER: {
      return vec4(0.18f, 0.18f, 0.18f, 1);
    }
    default: {
      return 1;
    }
  }
}

/**
  Return's line pointer by number using optimized traversal.
  1. From current position.
  2. From file start.
  3. From file end.
  Chooses shortest path to target line.
 */
// linestruct *gui_line_from_number(guieditor *editor, long number) {
//   ASSERT(editor);
//   ASSERT(editor->openfile->current);
//   ASSERT(editor->openfile->filetop);
//   ASSERT(editor->openfile->filebot);
//   /* Always assert that number is a valid number in the context of the open file. */
//   ALWAYS_ASSERT(number >= 0 && number <= editor->openfile->filebot->lineno);
//   /* Set the starting line to the cursor line of the currently active file in editor. */
//   linestruct *line = editor->openfile->current;
//   /* Get the distance from the start and end line as well as from the cursor. */
//   long dist_from_current = labs(line->lineno - number);
//   long dist_from_end     = (editor->openfile->filebot->lineno - number);
//   /* Start from the closest line to number. */
//   if (number < dist_from_current && number < dist_from_end) {
//     line = editor->openfile->filetop;
//   }
//   else if (dist_from_end < dist_from_current) {
//     line = editor->openfile->filebot;
//   }
//   /* Move the line ptr to the correct line. */
//   while (line->lineno > number) {
//     line = line->prev;
//   }
//   while (line->lineno < number) {
//     line = line->next;
//   }
//   return line;
// }

/* Get the line number coresponding to the relative `ypos` in `editor` text element. */
// long get_lineno_from_scrollbar_position(guieditor *editor, float ypos) {
//   ASSERT(editor);
//   ASSERT(editor->openfile->filebot);
//   ASSERT(editor->text);
//   return index_from_scrollbar_pos(editor->text->size.h, editor->openfile->filetop->lineno, editor->openfile->filebot->lineno, editor->rows, ypos);
// }

#endif
