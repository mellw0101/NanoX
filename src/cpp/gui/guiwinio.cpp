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
        rect.x = (string_pixel_offset(line->data, " ", start_col, withfont) + get_line_number_pixel_offset(line, withfont));
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
  glUseProgram(gui->rect_shader); {
    /* Get the locations for the rect shader uniforms. */
    static int rectcolor_loc = glGetUniformLocation(gui->rect_shader, "rectcolor");
    static int elempos_loc   = glGetUniformLocation(gui->rect_shader, "elempos");
    static int elemsize_loc  = glGetUniformLocation(gui->rect_shader, "elemsize");
    /* Pass the color to the rect shader color uniform. */
    glUniform4fv(rectcolor_loc, 1, &color[0]);
    glUniform2fv(elempos_loc, 1, &pos[0]);
    glUniform2fv(elemsize_loc, 1, &size[0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
}

static void render_vertex_buffer(Uint shader, vertex_buffer_t *buf) {
  ASSERT(shader);
  ASSERT(buf);
  glEnable(GL_TEXTURE_2D);
  glUseProgram(shader); {
    glUniform1i(glGetUniformLocation(shader, "texture"), 0);
    vertex_buffer_render(buf, GL_TRIANGLES);
  }
}

/* Draw one row onto the gui window. */
static void gui_draw_row(linestruct *line, guieditor *editor, vec2 *drawpos) {
  /* When debugging is enabled, assert everything we will use. */
  ASSERT(line);
  ASSERT(drawpos);
  SyntaxObject *obj;
  const char *prev_char = NULL;
  char linenobuffer[margin + 1];
  /* If line numbers are turned on, draw them.  But only when a refresh is needed. */
  if (refresh_needed && ISSET(LINE_NUMBERS)) {
    sprintf(linenobuffer, "%*lu ", (margin - 1), line->lineno);
    vertex_buffer_add_string(editor->buffer, linenobuffer, margin, prev_char, gui->font, vec4(1.0f), drawpos);
    prev_char = " ";
  }
  /* Return early if the line is empty. */
  if (!*line->data) {
    return;
  }
  Ulong from_col  = get_page_start(wideness(line->data, ((line == editor->openfile->current) ? editor->openfile->current_x : 0)));
  char *converted = display_string(line->data, from_col, editor->cols, TRUE, FALSE);
  if (refresh_needed) {
    Ulong converted_len = strlen(converted);
    /* For c/cpp files. */
    if (ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX)) {
      if (editor->openfile->type.is_set<C_CPP>()) {
        Ulong index = 0;
        line_word_t *head = get_line_words(converted, converted_len);
        while (head) {
          line_word_t *node = head;
          head = node->next;
          if (sf) {
            obj = (SyntaxObject *)hashmap_get(sf->objects, node->str);
            if (obj) {
              if (obj->color == SYNTAX_COLOR_BLUE) {
                vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui->font, vec4(1.0f), drawpos);
                vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui->font, VEC4_8BIT(36, 114, 200, 1), drawpos);
                index = node->end;
                free_node(node);
                continue;
              }
            }
          }
          /* The global map. */
          if (test_map.find(node->str) != test_map.end()) {
            vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui->font, vec4(1.0f), drawpos);
            vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui->font, color_idx_to_vec4(test_map[node->str].color), drawpos);
            index = node->end;
          }
          /* The variable map in the language server. */
          else if (LSP->index.vars.find(node->str) != LSP->index.vars.end()) {
            for (const auto &v : LSP->index.vars[node->str]) {
              if (strcmp(tail(v.file), tail(openfile->filename)) == 0) {
                if (line->lineno >= v.decl_st && line->lineno <= v.decl_end) {
                  vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui->font, vec4(1.0f), drawpos);
                  vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui->font, color_idx_to_vec4(FG_VS_CODE_BRIGHT_CYAN), drawpos);
                  index = node->end;
                }
              }
            }
          }
          /* The define map in the language server. */
          else if (LSP->index.defines.find(node->str) != LSP->index.defines.end()) {
            vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui->font, vec4(1.0f), drawpos);
            vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui->font, color_idx_to_vec4(FG_VS_CODE_BLUE), drawpos);
            index = node->end;
          }
          /* The map for typedefined structs and normal structs. */
          else if (LSP->index.tdstructs.find(node->str) != LSP->index.tdstructs.end() || LSP->index.structs.find(node->str) != LSP->index.structs.end()) {
            vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui->font, vec4(1.0f), drawpos);
            vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui->font, color_idx_to_vec4(FG_VS_CODE_GREEN), drawpos);
            index = node->end;
          }
          /* The function name map in the language server. */
          else if (LSP->index.functiondefs.find(node->str) != LSP->index.functiondefs.end()) {
            vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui->font, vec4(1.0f), drawpos);
            vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui->font, color_idx_to_vec4(FG_VS_CODE_BRIGHT_YELLOW), drawpos);
            index = node->end;
          }
          free_node(node);
        }
        vertex_buffer_add_string(editor->buffer, (converted + index), (converted_len - index), prev_char, gui->font, vec4(1.0f), drawpos);
      }
      /* Asm syntax. */
      else if (openfile->type.is_set<ASM>()) {
        const char *comment = strchr(converted, ';');
        if (comment) {
          vec2 origin = *drawpos;
          origin.x += string_pixel_offset(converted, (ISSET(LINE_NUMBERS) ? " " : NULL), (comment - converted), gui->font);
          vertex_buffer_add_string(
            editor->buffer,
            (converted + (comment - converted)),
            (converted_len - (comment - converted)),
            ((comment - converted) ? &converted[(comment - converted) - 1] : NULL),
            gui->font,
            color_idx_to_vec4(FG_COMMENT_GREEN),
            &origin
          );
        }
        /* If the comment is not the first char. */
        if (comment - converted) {
          vertex_buffer_add_string(
            editor->buffer,
            converted,
            (comment ? (comment - converted) : converted_len),
            (ISSET(LINE_NUMBERS) ? " " : NULL),
            gui->font,
            vec4(1.0f),
            drawpos
          );
        }
      }
      /* Bash syntax. */
      else if (editor->openfile->type.is_set<BASH>()) {
        const char *comment = strchr(converted, '#');
        /* When there is a comment on this line. */
        if (comment) {
          if (converted[(comment - converted) - 1] == '$') {
            comment = NULL;
          }
          else {
            vec2 origin = *drawpos;
            origin.x += string_pixel_offset(converted, (ISSET(LINE_NUMBERS) ? " " : NULL), (comment - converted), gui->font);
            vertex_buffer_add_string(
              editor->buffer,
              (converted + (comment - converted)),
              (converted_len - (comment - converted)),
              ((comment - converted) ? &converted[(comment - converted) - 1] : NULL),
              gui->font,
              GUI_DEFAULT_COMMENT_COLOR,
              &origin
            );
          }
        }
        /* If the comment is not the first char. */
        if (comment - converted) {
          vertex_buffer_add_string(
            editor->buffer,
            converted,
            (comment ? (comment - converted) : converted_len),
            (ISSET(LINE_NUMBERS) ? " " : NULL),
            gui->font,
            GUI_WHITE_COLOR,
            drawpos
          );
        }
        else if (!comment) {
          vertex_buffer_add_string(editor->buffer, converted, converted_len, NULL, gui->font, GUI_WHITE_COLOR, drawpos);
        }
      }
      /* Otherwise just draw white text. */
      else {
        vertex_buffer_add_string(editor->buffer, converted, converted_len, prev_char, gui->font, vec4(1.0f), drawpos);
      }
    }
    /* Otherwise just draw white text. */
    else {
      vertex_buffer_add_string(editor->buffer, converted, converted_len, prev_char, gui->font, vec4(1.0f), drawpos);
    }
  }
  draw_marked_part(line, converted, from_col, gui->font);
  free(converted);
}

/* Show a status message of `type`, for `seconds`.  Note that the type will determen if the
 * message will be shown, depending if there is a more urgent message already being shown. */
void show_statusmsg(message_type type, float seconds, const char *format, ...) {
  ASSERT(seconds);
  ASSERT(format);
  int len;
  char buffer[4096];
  va_list ap;
  /* Ignore updates when the type of this message is lower then the currently displayed message. */
  if (type < statustype && statustype > NOTICE) {
    return;
  }
  va_start(ap, format);
  len = vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  if (len > editwincols) {
    len = editwincols;
  }
  statustype = type;
  statusmsg  = free_and_assign(statusmsg, measured_copy(buffer, len));
  statustime = seconds;
}

/* Show the correct message when we toggle a flag. */
void show_toggle_statusmsg(int flag) {
  show_statusmsg(REMARK, 2, "%s %s", epithet_of_flag(flag), (ISSET(flag) ? "enabled" : "disabled"));
}

/* Draw a editor. */
void draw_editor(guieditor *editor) {
  /* When dubugging is enabled, assert everything we use. */
  // ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, editor);
  // ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, editor->openfile);
  ASSERT(editor->openfile->edittop);
  ASSERT(editor->topbar);
  ASSERT(editor->topbuf);
  ASSERT(editor->text);
  ASSERT(editor->buffer);
  ASSERT(editor->gutter);
  ASSERT(editor->scrollbar);
  ASSERT(gui->font_shader);
  ASSERT(gui);
  ASSERT(gui->font);
  ASSERT(gui->uifont);
  /* When the editor is hidden, just return. */
  if (editor->flag.is_set<GUIEDITOR_HIDDEN>()) {
    return;
  }
  /* Start at the top of the text window. */
  int row = 0;
  linestruct *line = editor->openfile->edittop;
  /* Draw the text editor element. */
  draw_element_rect(editor->text);
  /* Draw the gutter element of the editor. */
  draw_element_rect(editor->gutter);
  /* Draw the top bar for the editor.  Where the open buffer names are displayd. */
  draw_element_rect(editor->topbar);
  /* Render the topbar of the editor. */
  if (editor->flag.is_set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>()) {
    refresh_editor_topbar(editor);
    vertex_buffer_clear(editor->topbuf);
    for (Ulong i = 0; i < editor->topbar->children.size(); ++i) {
      /* Assign the child to a new ptr for readbility. */
      guielement *child = editor->topbar->children[i];
      /* Draw the child rect. */
      draw_element_rect(child);
      /* Update the data in the topbuf. */
      if (child->flag.is_set<GUIELEMENT_HAS_LABLE>()) {
        vertex_buffer_add_element_lable_offset(child, gui->uifont, editor->topbuf, vec2(pixel_breadth(gui->uifont, " "), 0));
      }
    }
    upload_texture_atlas(gui->atlas);
    editor->flag.unset<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
    editor->flag.unset<GUIEDITOR_TOPBAR_UPDATE_ACTIVE>();
  }
  else {
    /* When the active file has changed, adjust the colors of the topbar. */
    if (editor->flag.is_set<GUIEDITOR_TOPBAR_UPDATE_ACTIVE>()) {
      update_editor_topbar(editor);
      editor->flag.unset<GUIEDITOR_TOPBAR_UPDATE_ACTIVE>();
    }
    for (Ulong i = 0; i < editor->topbar->children.size(); ++i) {
      /* Assign the child to a new ptr for readbility. */
      guielement *child = editor->topbar->children[i];
      /* Draw the child rect. */
      draw_element_rect(child);
    }
  }
  render_vertex_buffer(gui->font_shader, editor->topbuf);
  /* Render the text element of the editor. */
  if (refresh_needed) {
    editor->pen.y = editor->text->pos.y;
    vertex_buffer_clear(editor->buffer);
    while (line && ++row <= editwinrows) {
      editor->pen.x = 0;
      editor->pen.y += FONT_HEIGHT(gui->font);
      gui_draw_row(line, editor, &editor->pen);
      line = line->next;
    }
    upload_texture_atlas(gui->atlas);
    if (!gui->flag.is_set<GUI_PROMPT>()) {
      add_openfile_cursor(gui->font, editor->buffer, vec4(1));
    }
  }
  else {
    while (line && ++row <= editwinrows) {
      gui_draw_row(line, editor, &editor->pen);
      line = line->next;
    }
  }
  render_vertex_buffer(gui->font_shader, editor->buffer);
  if (editor->flag.is_set<GUIEDITOR_SCROLLBAR_REFRESH_NEEDED>()) {
    update_editor_scrollbar(editor);
    editor->flag.unset<GUIEDITOR_SCROLLBAR_REFRESH_NEEDED>();
  }
  draw_element_rect(editor->scrollbar);
}

/* Draw the top bar of the gui. */
void draw_topbar(void) {
  if (gui->flag.is_set<GUI_PROMPT>()) {
    gui->topbar->color = color_idx_to_vec4(FG_VS_CODE_RED);
  }
  else {
    gui->topbar->color = EDIT_BACKGROUND_COLOR;
  }
  /* Always draw the top-bar.  For now. */
  draw_element_rect(gui->topbar);
  /* When in prompt mode.  Only draw the prompt and the answer. */
  if (gui->flag.is_set<GUI_PROMPT>()) {
    if (refresh_needed) {
      vertex_buffer_clear(gui->topbuf);
      vec2 penpos((gui->topbar->pos.x + pixel_breadth(gui->font, " ")), (gui->topbar->pos.y + FONT_HEIGHT(gui->font) + gui->font->descender));
      vertex_buffer_add_string(gui->topbuf, prompt, strlen(prompt), NULL, gui->font, vec4(1.0f), &penpos);
      vertex_buffer_add_string(gui->topbuf, answer, strlen(answer), " ", gui->font, vec4(1.0f), &penpos);
      vec2 cursor_pos(
        (gui->topbar->pos.x + pixel_breadth(gui->font, " ") + pixel_breadth(gui->font, prompt) + string_pixel_offset(answer, " ", typing_x, gui->font)),
        gui->topbar->pos.y
      );
      add_cursor(gui->font, gui->topbuf, vec4(1.0f), cursor_pos);
    }
  }
  /* Otherwise, draw the menu elements as usual. */
  else {
    // draw_element_rect(file_menu_element);
    if (refresh_needed) {
      vertex_buffer_clear(gui->topbuf);
      // vertex_buffer_add_element_lable(file_menu_element, gui->font, gui->topbuf);
    }
    // for (auto child : file_menu_element->children) {
    //   if (!child->flag.is_set<GUIELEMENT_HIDDEN>()) {
    //     draw_element_rect(child);
    //     if (refresh_needed) {
    //       vertex_buffer_add_element_lable(child, gui->font, gui->topbuf);
    //     }
    //   }
    // }
  }
  /* If a refresh is needed, meaning that we have cleared and reinput the data in the vertex buffer. */
  if (refresh_needed) {
    upload_texture_atlas(gui->atlas);
  }
  render_vertex_buffer(gui->font_shader, gui->topbuf);
}

/* Draw the bottom bar of the gui. */
void draw_botbar(void) {
  draw_element_rect(gui->botbar);
}

/* Draw the status bar for the gui. */
void draw_statusbar(void) {
  if (statustype != VACUUM) {
    /* Check it the message has been shown for the set time. */
    statustime -= (1.0f / frametimer.fps);
    if (statustime < 0) {
      /* If the set time has elapsed, then reset the status element. */
      statustype = VACUUM;
      gui->statusbar->flag.set<GUIELEMENT_HIDDEN>();
      return;
    }
    if (refresh_needed) {
      gui->statusbar->flag.unset<GUIELEMENT_HIDDEN>();
      float msgwidth = (pixel_breadth(gui->font, statusmsg) + pixel_breadth(gui->font, "  "));
      vertex_buffer_clear(gui->statusbuf);
      move_resize_element(
        gui->statusbar,
        vec2((((float)gui->width / 2) - (msgwidth / 2)), (gui->height - gui->botbar->size.h - gui->statusbar->size.h)),
        vec2(msgwidth, FONT_HEIGHT(gui->font))
      );
      vec2 penpos((gui->statusbar->pos.x + pixel_breadth(gui->font, " ")), (gui->statusbar->pos.y + FONT_HEIGHT(gui->font) + gui->font->descender));
      vertex_buffer_add_string(gui->statusbuf, statusmsg, strlen(statusmsg), " ", gui->font, vec4(1.0f), &penpos);
      upload_texture_atlas(gui->atlas);
    }
    draw_element_rect(gui->statusbar);
    render_vertex_buffer(gui->font_shader, gui->statusbuf);
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
    if (!monitor) {
      log_error_gui("%s: Monitor is invalid.\n", __func__);
      return;
    }
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (!mode) {
      log_error_gui("%s: Mode is invalid.\n", __func__);
      return;
    }
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
