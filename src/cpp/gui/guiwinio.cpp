#include "../../include/prototypes.h"

#ifdef HAVE_GLFW

#define TOP_BAR_COLOR vec4(vec3(0.5f), 1.0f)

/* The message to be drawn at the bottom bar. */
static char *statusmsg = NULL;
/* The time the message should be shown. */
static float statustime = 0.0f;
/* The type for the currently displayed status message. */
static message_type statustype = VACUUM;

/* If the line is part of the marked region, then draw a rect that reprecents the marked region. */
void draw_marked_part(linestruct *line, const char *converted, Ulong from_col, texture_font_t *withfont) {
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
    vec4 rect, mark_color;
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
        paintlen = strlen(thetext);
      }
      if (ISSET(LINE_NUMBERS)) {
        Ulong offset = get_line_number_pixel_offset(line, withfont);
        rect.x = (string_pixel_offset(line->data, " ", start_col, withfont) + offset);
        /* Calculate the number of pixels for the rect that will represent the painted section. */
        if (converted == thetext) {
          rect.width = string_pixel_offset(thetext, " ", paintlen, withfont);
        }
        else {
          rect.width = string_pixel_offset(thetext, &converted[(thetext - converted) - 1], paintlen, withfont);
        }
      }
      else {
        rect.x = string_pixel_offset(line->data, NULL, start_col, withfont);
        /* Calculate the number of pixels for the rect that will represent the painted section. */
        if (converted == thetext) {
          rect.width = string_pixel_offset(thetext, NULL, paintlen, withfont);
        }
        else {
          rect.width = string_pixel_offset(thetext, &converted[(thetext - converted) - 1], paintlen, withfont);
        }
      }
      rect.y = line_y_pixel_offset(line, withfont);
      rect.height = FONT_HEIGHT(withfont);
      mark_color  = EDIT_BACKGROUND_COLOR + vec4(0.05f);
      mark_color.alpha = 0.45f;
      mark_color.blue += 0.30f;
      draw_rect(rect.xy(), rect.zw(), mark_color);
    }
  }
}

/* Draw rect to the window. */
void draw_rect(vec2 pos, vec2 size, vec4 color) {
  glUseProgram(rectshader); {
    /* Get the locations for the rect shader uniforms. */
    static int rectcolor_loc = glGetUniformLocation(rectshader, "rectcolor");
    static int elempos_loc   = glGetUniformLocation(rectshader, "elempos");
    static int elemsize_loc  = glGetUniformLocation(rectshader, "elemsize");
    /* Pass the color to the rect shader color uniform. */
    glUniform4fv(rectcolor_loc, 1, &color[0]);
    glUniform2fv(elempos_loc, 1, &pos[0]);
    glUniform2fv(elemsize_loc, 1, &size[0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
}

/* Draw a ui-element`s rect. */
void draw_uielement_rect(uielementstruct *element) {
  draw_rect(element->pos, element->size, element->color);
}

/* Resize an element as well as correctly adjust it in the gridmap. */
void resize_element(uielementstruct *e, vec2 size) {
  gridmap.remove(e);
  e->size = size;
  gridmap.set(e);
}

/* Move an element as well as correctly adjust it in the gridmap. */
void move_element(uielementstruct *e, vec2 pos) {
  gridmap.remove(e);
  e->pos = pos;
  gridmap.set(e);
}

/* Move and resize an element and correctly set it in the gridmap. */
void move_resize_element(uielementstruct *e, vec2 pos, vec2 size) {
  gridmap.remove(e);
  e->pos  = pos;
  e->size = size;
  gridmap.set(e);
}

static void render_vertex_buffer(Uint shader, vertex_buffer_t *buffer) {
  glEnable(GL_TEXTURE_2D);
  glUseProgram(shader);
  {
    static int texture_loc = glGetUniformLocation(shader, "texture");
    glUniform1i(texture_loc, 0);
    vertex_buffer_render(buffer, GL_TRIANGLES);
  }
}

/* Draw one row onto the gui window. */
static void gui_draw_row(linestruct *line, texture_font_t *withfont, vec2 *draw_pos) {
  const char *prev_char = NULL;
  char linenobuffer[margin + 1];
  /* If line numbers are turned on, draw them.  But only when a refresh is needed. */
  if (refresh_needed && ISSET(LINE_NUMBERS)) {
    sprintf(linenobuffer, "%*lu ", (margin - 1), line->lineno);
    vertex_buffer_add_string(vertbuf, linenobuffer, margin, prev_char, markup.font, vec4(1.0f), draw_pos);
    prev_char = " ";
  }
  /* Return early if the line is empty. */
  if (!*line->data) {
    return;
  }
  Ulong from_col  = get_page_start(wideness(line->data, ((line == openfile->current) ? openfile->current_x : 0)));
  char *converted = display_string(line->data, from_col, editwincols, TRUE, FALSE);
  if (refresh_needed) {
    Ulong converted_len = strlen(converted);
    /* For c/cpp files. */
    if (ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX) && openfile->type.is_set<C_CPP>()) {
      Ulong index = 0;
      line_word_t *head = get_line_words(converted, converted_len);
      while (head) {
        line_word_t *node = head;
        head = node->next;
        /* The global map. */
        if (test_map.find(node->str) != test_map.end()) {
          vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), draw_pos);
          vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(test_map[node->str].color), draw_pos);
          index = node->end;
        }
        /* The variable map in the language server. */
        else if (LSP->index.vars.find(node->str) != LSP->index.vars.end()) {
          for (const auto &v : LSP->index.vars[node->str]) {
            if (strcmp(tail(v.file), tail(openfile->filename)) == 0) {
              if (line->lineno >= v.decl_st && line->lineno <= v.decl_end) {
                vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), draw_pos);
                vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(FG_VS_CODE_BRIGHT_CYAN), draw_pos);
                index = node->end;
              }
            }
          }
        }
        /* The define map in the language server. */
        else if (LSP->index.defines.find(node->str) != LSP->index.defines.end()) {
          vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), draw_pos);
          vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(FG_VS_CODE_BLUE), draw_pos);
          index = node->end;
        }
        /* The map for typedefined structs and normal structs. */
        else if (LSP->index.tdstructs.find(node->str) != LSP->index.tdstructs.end() || LSP->index.structs.find(node->str) != LSP->index.structs.end()) {
          vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), draw_pos);
          vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(FG_VS_CODE_GREEN), draw_pos);
          index = node->end;
        }
        /* The function name map in the language server. */
        else if (LSP->index.functiondefs.find(node->str) != LSP->index.functiondefs.end()) {
          vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), draw_pos);
          vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(FG_VS_CODE_BRIGHT_YELLOW), draw_pos);
          index = node->end;
        }
        free_node(node);
      }
      vertex_buffer_add_string(vertbuf, (converted + index), (converted_len - index), prev_char, markup.font, vec4(1.0f), draw_pos);
    }
    /* Otherwise just draw white text. */
    else {
      vertex_buffer_add_string(vertbuf, converted, converted_len, prev_char, markup.font, vec4(1.0f), draw_pos);
    }
  }
  draw_marked_part(line, converted, from_col, markup.font);
  free(converted);
}

/* Show a status message of `type`, for `seconds`.  Note that the type will determen if the
 * message will be shown, depending if there is a more urgent message already being shown. */
void show_statusmsg(message_type type, float seconds, const char *format, ...) {
  bool tolong = FALSE;
  int len;
  char buffer[4096];
  va_list ap;
  va_start(ap, format);
  len = vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  if (len > editwincols) {
    tolong = TRUE;
  }
  statustype = type;
  statusmsg  = free_and_assign(statusmsg, measured_copy(buffer, (tolong ? editwincols : len)));
  statustime = seconds;
}

/* Render the editelement. */
void draw_editelement(void) {
  int row = 0;
  linestruct *line = openfile->edittop;
  /* Draw the edit element first. */
  draw_uielement_rect(editelement);
  /* Then draw the gutter. */
  draw_uielement_rect(gutterelement);
  /* When required, clear the vertex. */
  if (refresh_needed) {
    /* Now handle the text. */
    pen.y = editelement->pos.y;
    vertex_buffer_clear(vertbuf);
  }
  while (line && ++row <= editwinrows) {
    if (refresh_needed) {
      /* Only set the pen if required. */
      pen.x = 0;
      pen.y += FONT_HEIGHT(markup.font);
    }
    gui_draw_row(line, markup.font, &pen);
    line = line->next;
  }
  if (refresh_needed) {
    upload_texture_atlas(atlas);
    if (!guiflag.is_set<GUI_PROMPT>()) {
      /* Add the cursor to the buffer, when not in prompt-mode. */
      add_openfile_cursor(markup.font, vertbuf, vec4(1.0f));
    }
  }
  render_vertex_buffer(fontshader, vertbuf);
}

/* Draw the top menu bar. */
void draw_top_bar(void) {
  if (guiflag.is_set<GUI_PROMPT>()) {
    top_bar->color = color_idx_to_vec4(FG_VS_CODE_RED);
  }
  else {
    top_bar->color = EDIT_BACKGROUND_COLOR;
  }
  /* Always draw the top-bar.  For now. */
  draw_uielement_rect(top_bar);
  /* When in prompt mode.  Only draw the prompt and the answer. */
  if (guiflag.is_set<GUI_PROMPT>()) {
    if (refresh_needed) {
      vertex_buffer_clear(topbuf);
      vec2 penpos((top_bar->pos.x + pixel_breadth(markup.font, " ")), (top_bar->pos.y + FONT_HEIGHT(markup.font) + markup.font->descender));
      vertex_buffer_add_string(topbuf, prompt, strlen(prompt), NULL, markup.font, vec4(1.0f), &penpos);
      vertex_buffer_add_string(topbuf, answer, strlen(answer), " ", markup.font, vec4(1.0f), &penpos);
      vec2 cursor_pos(
        (top_bar->pos.x + pixel_breadth(markup.font, " ") + pixel_breadth(markup.font, prompt) + string_pixel_offset(answer, " ", typing_x, markup.font)),
        top_bar->pos.y
      );
      add_cursor(markup.font, topbuf, vec4(1.0f), cursor_pos);
    }
  }
  /* Otherwise, draw the menu elements as usual. */
  else {
    draw_uielement_rect(file_menu_element);
    if (refresh_needed) {
      vertex_buffer_clear(topbuf);
      vertex_buffer_add_element_lable(file_menu_element, markup.font, topbuf);
    }
    for (auto child : file_menu_element->children) {
      if (!child->flag.is_set<UIELEMENT_HIDDEN>()) {
        draw_uielement_rect(child);
        if (refresh_needed) {
          vertex_buffer_add_element_lable(child, markup.font, topbuf);
        }
      }
    }
  }
  /* If a refresh is needed, meaning that we have cleared and reinput the data in the vertex buffer. */
  if (refresh_needed) {
    upload_texture_atlas(atlas);
  }
  render_vertex_buffer(fontshader, topbuf);
}

/* Draw the bottom bar. */
void draw_botbar(void) {
  if (statustype != VACUUM) {
    /* Check it the message has been shown for the set time. */
    statustime -= (1.0f / frametimer.fps);
    if (statustime < 0) {
      /* If the set time has elapsed, then reset the status element. */
      statustype = VACUUM;
      botbar->flag.set<UIELEMENT_HIDDEN>();
      return;
    }
    if (refresh_needed) {
      botbar->flag.unset<UIELEMENT_HIDDEN>();
      float msgwidth = (pixel_breadth(markup.font, statusmsg) + pixel_breadth(markup.font, "  "));
      vertex_buffer_clear(botbuf);
      move_resize_element(
        botbar,
        vec2((((float)window_width / 2) - (msgwidth / 2)), (window_height - FONT_HEIGHT(markup.font))),
        vec2(msgwidth, FONT_HEIGHT(markup.font))
      );
      vec2 penpos((botbar->pos.x + pixel_breadth(markup.font, " ")), (botbar->pos.y + FONT_HEIGHT(markup.font) + markup.font->descender));
      vertex_buffer_add_string(botbuf, statusmsg, strlen(statusmsg), " ", markup.font, vec4(1.0f), &penpos);
      upload_texture_atlas(atlas);
    }
    draw_uielement_rect(botbar);
    render_vertex_buffer(fontshader, botbuf);
  }
}

/* Toggle fullscreen state. */
void do_fullscreen(GLFWwindow *window) {
  static bool is_fullscreen = FALSE;
  static ivec4 rect = 0.0f;
  if (!is_fullscreen) {
    /* Save the window size and position. */
    glfwGetWindowPos(window, &rect.x, &rect.y);
    glfwGetWindowSize(window, &rect.width, &rect.height);
    /* Get monitor size. */
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    /* Set the window pos and size. */
    glfwSetWindowAttrib(window, GLFW_AUTO_ICONIFY, FALSE);
    glfwMaximizeWindow(window);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
  }
  else {
    /* Set the window pos and size. */
    glfwSetWindowMonitor(window, NULL, rect.x, rect.y, rect.width, rect.height, GLFW_DONT_CARE);
    glfwRestoreWindow(window);
  }
  /* Toggle the fullscreen flag. */
  is_fullscreen = !is_fullscreen;
}

#endif
