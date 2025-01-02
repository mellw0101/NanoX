#include "../../include/prototypes.h"

/* If the line is part of the marked region, then draw a rect that reprecents the marked region. */
void draw_marked_part(linestruct *line, const char *converted, Ulong from_col, markup_t *markup) {
  /* If the line is at least partially selected, paint the marked part. */
  if (line_in_marked_region(line)) {
    /* The top and bot line where the marked region begins and ends. */
    linestruct *top, *bot;
    /* The x pos in the top and bot line. */
    Ulong top_x, bot_x;
    /* Start index for where to draw in this line. */
    int start_col;
    /* Where in converted the painting starts. */
    const char *thetext;
    /* The number of columns to paint. */
    int paintlen = -1;
    /* The rect to draw. */
    vec4 rect;
    get_region(&top, &top_x, &bot, &bot_x);
    if (top->lineno < line->lineno || top_x < from_x) {
      top_x = from_x;
    }
    if (bot->lineno > line->lineno || bot_x > till_x) {
      bot_x = till_x;
    }
    /* Only paint if the marked part of the line is on this page. */
    if (top_x < till_x && bot_x > from_x) {
      /* Compute on witch column to start painting. */
      start_col = (wideness(line->data, top_x) - from_col);
      if (start_col < 0) {
        start_col = 0;
      }
      thetext = (converted + actual_x(converted, start_col));
      /* If the end of the mark is onscreen, compute how meny characters to paint. */
      if (bot_x < till_x) {
        Ulong end_col = (wideness(line->data, bot_x) - from_col);
        paintlen = actual_x(thetext, (end_col - start_col));
      }
      /* Otherwise, calculate the end index of the text on screen. */
      if (paintlen == -1) {
        paintlen = (strlen(converted) - start_col);
      }
      if (ISSET(LINE_NUMBERS)) {
        Ulong offset = gui_calculate_lineno_offset(line, markup);
        rect.x = (gui_calculate_string_offset(line->data, " ", start_col, markup) + offset);
        /* Calculate the number of pixels for the rect that will represent the painted section. */
        if (converted == thetext) {
          rect.width = gui_calculate_string_offset(thetext, " ", paintlen, markup);
        }
        else {
          rect.width = gui_calculate_string_offset(thetext, &converted[(thetext - converted) - 1], paintlen, markup);
        }
      }
      else {
        rect.x = gui_calculate_string_offset(line->data, NULL, start_col, markup);
        /* Calculate the number of pixels for the rect that will represent the painted section. */
        if (converted == thetext) {
          rect.width = gui_calculate_string_offset(thetext, NULL, paintlen, markup);
        }
        else {
          rect.width = gui_calculate_string_offset(thetext, &converted[(thetext - converted) - 1], paintlen, markup);
        }
      }
      rect.y = (calculate_line_y_offset(line, markup) - markup->font->descender);
      rect.height = FONT_HEIGHT(markup->font);
      draw_rect(rect.xy(), rect.zw(), vec4(0.0f, 0.0f, 0.8f, 0.25f));
    }
  }
}

/* Draw rect to the window. */
void draw_rect(vec2 pos, vec2 size, vec4 color) {
  glUseProgram(rectshader);
  {
    /* Pass the color to the rect shader color uniform. */
    glUniform4fv(glGetUniformLocation(rectshader, "rectcolor"), 1, &color[0]);
    glUniform2fv(glGetUniformLocation(rectshader, "elempos"), 1, &pos[0]);
    glUniform2fv(glGetUniformLocation(rectshader, "elemsize"), 1, &size[0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
}

/* Resize an element as well as correctly adjust it in the gridmap. */
void resize_element(uielementstruct *e, vec2 size) {
  gridmap.remove(e);
  e->size = size;
  gridmap.set(e);
}

/* Resize an element as well as correctly adjust it in the gridmap. */
void move_element(uielementstruct *e, vec2 pos) {
  gridmap.remove(e);
  e->pos = pos;
  gridmap.set(e);
}

/* Render the editelement. */
void draw_editelement(void) {
  int row = 0;
  char *converted = NULL;
  char *cur_char;
  char *prev_char = NULL;
  char linenobuffer[margin + 1];
  linestruct *line = openfile->edittop;
  /* Draw the edit element first. */
  draw_rect(editelement->pos, editelement->size, vec4(0.2f, 0.2f, 0.2f, 1.0f));
  /* Then draw the gutter. */
  draw_rect(gutterelement->pos, gutterelement->size, vec4(0.2f, 0.2f, 0.2f, 1.0f));
  /* Now handle the text. */
  pen.y = 0;
  vertex_buffer_clear(vertbuf);
  while (line && ++row <= editwinrows) {
    /* Reset the pen pos. */
    pen.x = 0;
    pen.y += (font[GUI_NORMAL_TEXT].font->height - font[GUI_NORMAL_TEXT].font->linegap);
    /* If line numbers are turned on, draw them. */
    if (ISSET(LINE_NUMBERS)) {
      sprintf(linenobuffer, "%*lu ", (margin - 1), line->lineno);
      for (Ulong i = 0; i < margin; ++i) {
        cur_char = &linenobuffer[i];
        add_glyph(cur_char, prev_char, vertbuf, &font[GUI_NORMAL_TEXT], &pen);
        prev_char = cur_char;
      }
    }
    /* If there is atleast one char in the line, draw it. */
    if (*line->data) {
      Ulong from_col = get_page_start(wideness(line->data, ((line == openfile->current) ? openfile->current_x : 0)));
      converted = display_string(line->data, from_col, editwincols, TRUE, FALSE);
      for (Ulong i = 0; converted[i]; ++i) {
        cur_char = (converted + i);
        add_glyph(cur_char, prev_char, vertbuf, &font[GUI_NORMAL_TEXT], &pen);
        prev_char = cur_char;
      }
      draw_marked_part(line, converted, from_col, &font[GUI_NORMAL_TEXT]);
      free(converted);
    }
    line = line->next;
  }
  glBindTexture(GL_TEXTURE_2D, atlas->id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas->width, atlas->height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas->data);
  /* Cursor (we use the black character (NULL) as texture). */
  texture_glyph_t *glyph = texture_font_get_glyph(font[GUI_NORMAL_TEXT].font, NULL);
  float r = font[GUI_NORMAL_TEXT].foreground_color.r;
  float g = font[GUI_NORMAL_TEXT].foreground_color.g;
  float b = font[GUI_NORMAL_TEXT].foreground_color.b;
  float a = font[GUI_NORMAL_TEXT].foreground_color.a;
  int x0 = gui_calculate_cursor_x(&font[GUI_NORMAL_TEXT]);
  int y0 = (gui_calculate_cursor_y(&font[GUI_NORMAL_TEXT]) - font[GUI_NORMAL_TEXT].font->descender);
  int x1 = (x0 + 2);
  int y1 = (y0 + font[GUI_NORMAL_TEXT].font->height - font[GUI_NORMAL_TEXT].font->linegap);
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
  vertex_buffer_push_back(vertbuf, vertices, 4, indices, 6);
  glEnable(GL_TEXTURE_2D);
  glUseProgram(fontshader);
  {
    glUniform1i(glGetUniformLocation(fontshader, "texture"), 0);
    vertex_buffer_render(vertbuf, GL_TRIANGLES);
  }
}