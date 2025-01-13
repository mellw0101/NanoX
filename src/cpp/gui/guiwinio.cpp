#include "../../include/prototypes.h"

#ifdef HAVE_GLFW

#define TOP_BAR_COLOR vec4(vec3(0.5f), 1.0f)

/* If the line is part of the marked region, then draw a rect that reprecents the marked region. */
void draw_marked_part(linestruct *line, const char *converted, Ulong from_col, texture_font_t *font) {
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
        Ulong offset = get_line_number_pixel_offset(line, font);
        rect.x = (string_pixel_offset(line->data, " ", start_col, font) + offset);
        /* Calculate the number of pixels for the rect that will represent the painted section. */
        if (converted == thetext) {
          rect.width = string_pixel_offset(thetext, " ", paintlen, font);
        }
        else {
          rect.width = string_pixel_offset(thetext, &converted[(thetext - converted) - 1], paintlen, font);
        }
      }
      else {
        rect.x = string_pixel_offset(line->data, NULL, start_col, font);
        /* Calculate the number of pixels for the rect that will represent the painted section. */
        if (converted == thetext) {
          rect.width = string_pixel_offset(thetext, NULL, paintlen, font);
        }
        else {
          rect.width = string_pixel_offset(thetext, &converted[(thetext - converted) - 1], paintlen, font);
        }
      }
      rect.y = line_y_pixel_offset(line, font);
      rect.height = FONT_HEIGHT(font);
      mark_color = EDIT_BACKGROUND_COLOR + vec4(0.05f);
      mark_color.alpha = 0.35f;
      mark_color.blue += 0.20f;
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
  glUseProgram(shader); {
    static int texture_loc = glGetUniformLocation(shader, "texture");
    glUniform1i(texture_loc, 0);
    vertex_buffer_render(buffer, GL_TRIANGLES);
  }
}

/* Draw one row onto the gui window. */
static void gui_draw_row(linestruct *line, texture_font_t *font, vec2 *pen) {
  const char *prev_char = NULL;
  char linenobuffer[margin + 1];
  /* If line numbers are turned on, draw them.  But only when a refresh is needed. */
  if (refresh_needed && ISSET(LINE_NUMBERS)) {
    sprintf(linenobuffer, "%*lu ", (margin - 1), line->lineno);
    vertex_buffer_add_string(vertbuf, linenobuffer, margin, prev_char, markup.font, vec4(1.0f), pen);
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
          vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), pen);
          vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(test_map[node->str].color), pen);
          index = node->end;
        }
        /* The variable map in the language server. */
        else if (LSP->index.vars.find(node->str) != LSP->index.vars.end()) {
          for (const auto &v : LSP->index.vars[node->str]) {
            if (strcmp(tail(v.file), tail(openfile->filename)) == 0) {
              if (line->lineno >= v.decl_st && line->lineno <= v.decl_end) {
                vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), pen);
                vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(FG_VS_CODE_BRIGHT_CYAN), pen);
                index = node->end;
              }
            }
          }
        }
        /* The define map in the language server. */
        else if (LSP->index.defines.find(node->str) != LSP->index.defines.end()) {
          vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), pen);
          vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(FG_VS_CODE_BLUE), pen);
          index = node->end;
        }
        /* The map for typedefined structs and normal structs. */
        else if (LSP->index.tdstructs.find(node->str) != LSP->index.tdstructs.end() || LSP->index.structs.find(node->str) != LSP->index.structs.end()) {
          vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), pen);
          vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(FG_VS_CODE_GREEN), pen);
          index = node->end;
        }
        /* The function name map in the language server. */
        else if (LSP->index.functiondefs.find(node->str) != LSP->index.functiondefs.end()) {
          vertex_buffer_add_string(vertbuf, (converted + index), (node->start - index), prev_char, markup.font, vec4(1.0f), pen);
          vertex_buffer_add_string(vertbuf, (converted + node->start), node->len, prev_char, markup.font, color_idx_to_vec4(FG_VS_CODE_BRIGHT_YELLOW), pen);
          index = node->end;
        }
        free_node(node);
      }
      vertex_buffer_add_string(vertbuf, (converted + index), (converted_len - index), prev_char, markup.font, vec4(1.0f), pen);
    }
    /* Otherwise just draw white text. */
    else {
      vertex_buffer_add_string(vertbuf, converted, converted_len, prev_char, markup.font, vec4(1.0f), pen);
    }
  }
  draw_marked_part(line, converted, from_col, markup.font);
  free(converted);
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
    /* Only set the pen if required. */
    if (refresh_needed) {
      /* Reset the pen pos. */
      pen.x = 0;
      pen.y += FONT_HEIGHT(markup.font);
    }
    gui_draw_row(line, markup.font, &pen);
    line = line->next;
  }
  if (refresh_needed) {
    upload_texture_atlas(atlas);
    /* Add the cursor to the buffer. */
    add_cursor(markup.font, vertbuf, vec4(1.0f));
  }
  render_vertex_buffer(fontshader, vertbuf);
}

/* Draw the top menu bar. */
void draw_top_bar(void) {
  draw_uielement_rect(top_bar);
  if (guiflag.is_set<GUI_PROMPT>()) {
    vertex_buffer_clear(topbuf);
  }
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
    if (refresh_needed) {
      upload_texture_atlas(atlas);
    }
  }
  render_vertex_buffer(fontshader, topbuf);
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