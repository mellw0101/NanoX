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
static void draw_marked(guieditor *const editor, linestruct *const line, const char *const restrict convert, Ulong fromcol) {
  /* The top and bottom line where the marked region begins and ends. */
  linestruct *top, *bot;
  /* The start position in the top line and the end position in the bottom line. */
  Ulong xtop, xbot;
  /* Start index for where to draw in this line. */
  int startcol;
  /* End index for where to draw in this line. */
  Ulong endcol;
  /* Where in convert the marked region begins. */
  const char *thetext;
  /* The number of columns the marked region covers in this line. */
  int paintlen = -1;
  /* The rect we will draw, and the color of it. */
  vec4 rect;
  /* The top and bottom pixels for the line. */
  float pixtop, pixbot;
  /* If the line is at least partially selected, paint the marked part. */
  if (line_in_marked_region(line)) {
    get_region(&top, &xtop, &bot, &xbot);
    /* Set xtop and xbot to reflect the start and end on this line. */
    if ((top->lineno < line->lineno) || (xtop < from_x)) {
      xtop = from_x;
    }
    if ((bot->lineno > line->lineno) || (xbot > till_x)) {
      xbot = till_x;
    }
    /* Only paint if the marked part of the line is on this page. */
    if (xtop < till_x && xbot > from_x) {
      /* Compute the start column. */
      startcol = (wideness(line->data, xtop) - fromcol);
      CLAMP_MIN(startcol, 0);
      thetext = (convert + actual_x(convert, startcol));
      /* If the end mark is onscreen, compute how meny columns to paint. */
      if (xbot < till_x) {
        endcol = (wideness(line->data, xbot) - fromcol);
        paintlen = actual_x(thetext, (endcol - startcol));
      }
      /* Otherwise, calculate the end index of the text on screen. */
      if (paintlen == -1) {
        paintlen = strlen(thetext);
      }
      rect.x = (string_pixel_offset(line->data, NULL, startcol, gui->font) + (ISSET(LINE_NUMBERS) ? get_line_number_pixel_offset(line, gui->font) : 0));
      /* Calculate the width of the marked region in pixels. */
      rect.width = string_pixel_offset(thetext, ((convert == thetext) ? NULL : &convert[(thetext - convert) - 1]), paintlen, gui->font);
      /* Get the y offset for the given line. */
      row_top_bot_pixel((line->lineno - editor->openfile->edittop->lineno), gui->font, &pixtop, NULL);
      rect.y = (pixtop + editor->text->pos.y);
      /* To ensure that there is no overlap and no space between lines by calculating
       * the height of the marked box based on the top of the next line. */
      row_top_bot_pixel((line->lineno - editor->openfile->edittop->lineno + 1), gui->font, &pixbot, NULL);
      rect.height = (pixbot - pixtop);
      /* Draw the rect to the screen. */
      draw_rect(rect.xy(), rect.zw(), GUI_MARKED_REGION_COLOR);
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

void render_vertex_buffer(Uint shader, vertex_buffer_t *buf) {
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
  char linenobuffer[margin + 1], *converted;
  Ulong from_col, converted_len;
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
  from_col  = get_page_start(wideness(line->data, ((line == editor->openfile->current) ? editor->openfile->current_x : 0)));
  converted = display_string(line->data, from_col, editor->cols, TRUE, FALSE);
  drawpos->x = editor->text->pos.x;
  if (refresh_needed) {
    converted_len = strlen(converted);
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
              else if (obj->color == SYNTAX_COLOR_GREEN) {
                vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui->font, vec4(1.0f), drawpos);
                vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui->font, VEC4_8BIT(13, 188, 121, 1), drawpos);
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
      /* AT&T asm syntax. */
      else if (editor->openfile->type.is_set<ATNT_ASM>()) {
        vec4 color;
        Ulong index = 0;
        line_word_t *head, *node;
        const char *comment = strchr(converted, '#');
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
        if (!comment || (comment - converted)) {
          head = get_line_words(converted, (comment ? (comment - converted) : converted_len));
          while (head) {
            node = head;
            head = node->next;
            if (syntax_map_exists(node->str, &color)) {
              vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui->font, vec4(1.0f), drawpos);
              vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui->font, color, drawpos);
              index = node->end;
            }
            free_node(node);
          }
          vertex_buffer_add_string(editor->buffer, (converted + index), ((comment ? (comment - converted) : converted_len) - index), prev_char, gui->font, 1, drawpos);
        }
      }
      /* Nasm syntax. */
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
        if (!comment || (comment - converted)) {
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
  draw_marked(editor, line, converted, from_col);
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
  if ((type < statustype) && (statustype > NOTICE)) {
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
  ASSERT(editor->openfile->edittop);
  ASSERT(editor->topbar);
  ASSERT(editor->topbuf);
  ASSERT(editor->text);
  ASSERT(editor->buffer);
  ASSERT(editor->gutter);
  // ASSERT(editor->scrollbar);
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
  guielement_draw(editor->text);
  /* Draw the gutter element of the editor. */
  guielement_draw(editor->gutter);
  /* Draw the top bar for the editor.  Where the open buffer names are displayd. */
  guielement_draw(editor->topbar);
  /* Render the topbar of the editor. */
  if (editor->flag.is_set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>()) {
    guieditor_refresh_topbar(editor);
    vertex_buffer_clear(editor->topbuf);
    for (Ulong i=0; i<editor->topbar->children.size(); ++i) {
      /* Assign the child to a new ptr for readbility. */
      guielement *child = editor->topbar->children[i];
      /* Draw the child rect. */
      guielement_draw(child);
      /* Update the data in the topbuf. */
      if (child->flag.is_set<GUIELEMENT_HAS_LABLE>()) {
        vertex_buffer_add_element_lable_offset(child, gui->uifont, editor->topbuf, vec2(pixbreadth(gui->uifont, " "), 0));
      }
    }
    editor->flag.unset<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
    editor->flag.unset<GUIEDITOR_TOPBAR_UPDATE_ACTIVE>();
  }
  else {
    /* When the active file has changed, adjust the colors of the topbar. */
    if (editor->flag.is_set<GUIEDITOR_TOPBAR_UPDATE_ACTIVE>()) {
      guieditor_update_active_topbar(editor);
      editor->flag.unset<GUIEDITOR_TOPBAR_UPDATE_ACTIVE>();
    }
    for (Ulong i = 0; i < editor->topbar->children.size(); ++i) {
      /* Assign the child to a new ptr for readbility. */
      guielement *child = editor->topbar->children[i];
      /* Draw the child rect. */
      guielement_draw(child);
    }
  }
  upload_texture_atlas(gui->uiatlas);
  render_vertex_buffer(gui->font_shader, editor->topbuf);
  /* Render the text element of the editor. */
  if (refresh_needed) {
    vertex_buffer_clear(editor->buffer);
    while (line && ++row <= editwinrows) {
      editor->pen.x = 0;
      editor->pen.y = (row_baseline_pixel((line->lineno - editor->openfile->edittop->lineno), gui->font) + editor->text->pos.y);
      gui_draw_row(line, editor, &editor->pen);
      line = line->next;
    }
    if (!gui->flag.is_set<GUI_PROMPT>()) {
      if ((editor->openfile->current->lineno - editor->openfile->edittop->lineno) >= 0) {
        line_add_cursor((editor->openfile->current->lineno - editor->openfile->edittop->lineno), gui->font, editor->buffer, vec4(1), cursor_pixel_x_pos(gui->font), editor->text->pos.y);
      }
    }
  }
  else {
    while (line && ++row <= editwinrows) {
      gui_draw_row(line, editor, &editor->pen);
      line = line->next;
    }
  }
  upload_texture_atlas(gui->atlas);
  render_vertex_buffer(gui->font_shader, editor->buffer);
  guiscrollbar_draw(editor->sb);
}

/* Draw the top bar of the gui. */
void draw_topbar(void) {
  if (gui->flag.is_set<GUI_PROMPT>()) {
    guielement_draw(gui->promptmenu->element);
    /* Only refresh the promptmenu buffer if it has changed. */
    if (gui->promptmenu->flag.refresh_needed) {
      vertex_buffer_clear(gui->promptmenu->buffer);
      vec2 penpos((gui->promptmenu->element->pos.x + pixbreadth(gui->uifont, " ")), (row_baseline_pixel(0, gui->uifont) + gui->promptmenu->element->pos.y));
      vertex_buffer_add_string(gui->promptmenu->buffer, prompt, strlen(prompt), NULL, gui->uifont, vec4(1), &penpos);
      vertex_buffer_add_string(gui->promptmenu->buffer, answer, strlen(answer), " ", gui->uifont, vec4(1), &penpos);
      line_add_cursor(
        0,
        gui->uifont,
        gui->promptmenu->buffer,
        1,
        (gui->promptmenu->element->pos.x + pixbreadth(gui->uifont, " ") + pixbreadth(gui->uifont, prompt) + string_pixel_offset(answer, " ", typing_x, gui->uifont)),
        gui->promptmenu->element->pos.y
      );
      gui->promptmenu->flag.refresh_needed = FALSE;
    }
    upload_texture_atlas(gui->uiatlas);
    render_vertex_buffer(gui->font_shader, gui->promptmenu->buffer);
  }
}

/* Draw the gui suggestmenu. */
void draw_suggestmenu(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->completions);
  /* Only draw the suggestmenu if there are any available suggestions. */
  if (cvec_len(gui->suggestmenu->completions)) {
    gui_suggestmenu_resize();
    /* Draw the main element of the suggestmenu. */
    guielement_draw(gui->suggestmenu->element);
    /* Highlight the selected entry in the suggestmenu when its on the screen. */
    gui_suggestmenu_draw_selected();
    /* Draw the scrollbar of the suggestmenu. */
    guiscrollbar_draw(gui->suggestmenu->sb);
    /* Draw the text of the suggestmenu entries. */
    gui_suggestmenu_draw_text();
  }
}

/* Draw the bottom bar of the gui. */
void draw_botbar(void) {
  guielement_draw(gui->botbar);
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
      float msgwidth = (pixbreadth(gui->uifont, statusmsg) + pixbreadth(gui->uifont, "  "));
      vertex_buffer_clear(gui->statusbuf);
      guielement_move_resize(
        gui->statusbar,
        vec2((((float)gui->width / 2) - (msgwidth / 2)), (gui->height - gui->botbar->size.h - gui->statusbar->size.h)),
        vec2(msgwidth, FONT_HEIGHT(gui->uifont))
      );
      vec2 penpos((gui->statusbar->pos.x + pixbreadth(gui->uifont, " ")), (row_baseline_pixel(0, gui->uifont) + gui->statusbar->pos.y));
      vertex_buffer_add_string(gui->statusbuf, statusmsg, strlen(statusmsg), " ", gui->uifont, 1, &penpos);
    }
    guielement_draw(gui->statusbar);
    upload_texture_atlas(gui->uiatlas);
    render_vertex_buffer(gui->font_shader, gui->statusbuf);
  }
}

static inline const GLFWvidmode *glfw_get_primary_monitor_mode(void) {
  GLFWmonitor *monitor;
  const GLFWvidmode *mode;
  /* Get primary monitor. */
  monitor = glfwGetPrimaryMonitor();
  if (!monitor) {
    writeferr("%s: Monitor is invalid.\n", __func__);
    return NULL;
  }
  mode = glfwGetVideoMode(monitor);
  if (!mode) {
    writeferr("%s: Mode is invalid.\n", __func__);
    return NULL;
  }
  return mode;
} 

/* Toggle fullscreen state. */
void do_fullscreen(GLFWwindow *window) {
  static bool is_fullscreen = FALSE;
  static ivec4 rect = 0.0f;
  GLFWmonitor *monitor;
  const GLFWvidmode *mode;
  if (!is_fullscreen) {
    /* Save the window size and position. */
    glfwGetWindowPos(window, &rect.x, &rect.y);
    glfwGetWindowSize(window, &rect.width, &rect.height);
    /* Get monitor size. */
    monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
      log_error_gui("%s: Monitor is invalid.\n", __func__);
      return;
    }
    mode = glfwGetVideoMode(monitor);
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

/* Get the refreshrate of the current monitor. */
int glfw_get_framerate(void) {
  const GLFWvidmode *mode = glfw_get_primary_monitor_mode();
  return (mode ? mode->refreshRate : -1);
}

#endif
