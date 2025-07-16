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
// static void draw_marked(guieditor *const editor, linestruct *const line, const char *const restrict convert, Ulong fromcol) {
//   /* The top and bottom line where the marked region begins and ends. */
//   linestruct *top, *bot;
//   /* The start position in the top line and the end position in the bottom line. */
//   Ulong xtop, xbot;
//   /* Start index for where to draw in this line. */
//   int startcol;
//   /* End index for where to draw in this line. */
//   Ulong endcol;
//   /* Where in convert the marked region begins. */
//   const char *thetext;
//   /* The number of columns the marked region covers in this line. */
//   int paintlen = -1;
//   /* The rect we will draw, and the color of it. */
//   vec4 rect;
//   /* The top and bottom pixels for the line. */
//   float pixtop, pixbot;
//   /* If the line is at least partially selected, paint the marked part. */
//   if (line_in_marked_region(line)) {
//     get_region(&top, &xtop, &bot, &xbot);
//     /* Set xtop and xbot to reflect the start and end on this line. */
//     if ((top->lineno < line->lineno) || (xtop < from_x)) {
//       xtop = from_x;
//     }
//     if ((bot->lineno > line->lineno) || (xbot > till_x)) {
//       xbot = till_x;
//     }
//     /* Only paint if the marked part of the line is on this page. */
//     if (xtop < till_x && xbot > from_x) {
//       /* Compute the start column. */
//       startcol = (wideness(line->data, xtop) - fromcol);
//       CLAMP_MIN(startcol, 0);
//       thetext = (convert + actual_x(convert, startcol));
//       /* If the end mark is onscreen, compute how meny columns to paint. */
//       if (xbot < till_x) {
//         endcol = (wideness(line->data, xbot) - fromcol);
//         paintlen = actual_x(thetext, (endcol - startcol));
//       }
//       /* Otherwise, calculate the end index of the text on screen. */
//       if (paintlen == -1) {
//         paintlen = strlen(thetext);
//       }
//       // rect.x = (string_pixel_offset(line->data, NULL, startcol, gui_font_get_font(textfont)) + (ISSET(LINE_NUMBERS) ? get_line_number_pixel_offset(line, gui_font_get_font(textfont)) : 0));
//       rect.x = (string_pixel_offset(line->data, NULL, startcol, gui_font_get_font(textfont)) + editor->text->pos.x);
//       /* Calculate the width of the marked region in pixels. */
//       rect.width = string_pixel_offset(thetext, ((convert == thetext) ? NULL : &convert[(thetext - convert) - 1]), paintlen, gui_font_get_font(textfont));
//       /* Get the y offset for the given line. */
//       gui_font_row_top_bot(textfont, (line->lineno - editor->openfile->edittop->lineno), &pixtop, NULL);
//       // row_top_bot_pixel((line->lineno - editor->openfile->edittop->lineno), gui_font_get_font(textfont), &pixtop, NULL);
//       rect.y = (pixtop + editor->text->pos.y);
//       /* To ensure that there is no overlap and no space between lines by calculating
//        * the height of the marked box based on the top of the next line. */
//       gui_font_row_top_bot(textfont, (line->lineno - editor->openfile->edittop->lineno + 1), &pixbot, NULL);
//       // row_top_bot_pixel((line->lineno - editor->openfile->edittop->lineno + 1), gui_font_get_font(textfont), &pixbot, NULL);
//       rect.height = (pixbot - pixtop);
//       /* Draw the rect to the screen. */
//       draw_rect(rect.xy(), rect.zw(), GUI_MARKED_REGION_COLOR);
//     }
//   }
// }

static void draw_marked(Editor *const editor, linestruct *const line, const char *const restrict convert, Ulong fromcol) {
  RectVertex rectvert[4];
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
      // rect.x = (string_pixel_offset(line->data, NULL, startcol, gui_font_get_font(textfont)) + (ISSET(LINE_NUMBERS) ? get_line_number_pixel_offset(line, gui_font_get_font(textfont)) : 0));
      rect.x = (string_pixel_offset(line->data, NULL, startcol, gui_font_get_font(textfont)) + editor->text->x);
      /* Calculate the width of the marked region in pixels. */
      rect.width = string_pixel_offset(thetext, ((convert == thetext) ? NULL : &convert[(thetext - convert) - 1]), paintlen, gui_font_get_font(textfont));
      /* Get the y offset for the given line. */
      gui_font_row_top_bot(textfont, (line->lineno - editor->openfile->edittop->lineno), &pixtop, NULL);
      // row_top_bot_pixel((line->lineno - editor->openfile->edittop->lineno), gui_font_get_font(textfont), &pixtop, NULL);
      rect.y = (pixtop + editor->text->y);
      /* To ensure that there is no overlap and no space between lines by calculating
       * the height of the marked box based on the top of the next line. */
      gui_font_row_top_bot(textfont, (line->lineno - editor->openfile->edittop->lineno + 1), &pixbot, NULL);
      // row_top_bot_pixel((line->lineno - editor->openfile->edittop->lineno + 1), gui_font_get_font(textfont), &pixbot, NULL);
      rect.height = (pixbot - pixtop);
      shader_rect_vertex_load(rectvert, rect.x, rect.y, rect.width, rect.height, PACKED_UINT_FLOAT(0.2f, 0.2f, 0.5f, 0.45f));
      vertex_buffer_push_back(editor->marked_region_buf, rectvert, 4, RECT_INDICES, RECT_INDICES_LEN);
      /* Draw the rect to the screen. */
      // draw_rect(rect.xy(), rect.zw(), GUI_MARKED_REGION_COLOR);
    }
  }
}

/* Draw rect to the window. */
// void draw_rect(vec2 pos, vec2 size, vec4 color) {
//   glUseProgram(gui->rect_shader); {
//     /* Get the locations for the rect shader uniforms. */
//     static int rectcolor_loc = glGetUniformLocation(gui->rect_shader, "rectcolor");
//     static int elempos_loc   = glGetUniformLocation(gui->rect_shader, "elempos");
//     static int elemsize_loc  = glGetUniformLocation(gui->rect_shader, "elemsize");
//     /* Pass the color to the rect shader color uniform. */
//     glUniform4fv(rectcolor_loc, 1, &color[0]);
//     glUniform2fv(elempos_loc, 1, &pos[0]);
//     glUniform2fv(elemsize_loc, 1, &size[0]);
//     glDrawArrays(GL_TRIANGLES, 0, 6);
//   }
// }

void render_vertex_buffer(Uint shader, vertex_buffer_t *buf) {
  ASSERT(shader);
  ASSERT(buf);
  glEnable(GL_TEXTURE_2D);
  glUseProgram(shader); {
    glUniform1i(glGetUniformLocation(shader, "texture"), 0);
    vertex_buffer_render(buf, GL_TRIANGLES);
  }
}

// static void gui_draw_row_linenum(linestruct *const line, guieditor *const editor) {
//   char linenobuffer[margin + 1];
//   vec2 pen;
//   if (refresh_needed && ISSET(LINE_NUMBERS)) {
//     pen = editor->gutter->pos;
//     pen.y += gui_font_row_baseline(textfont, (line->lineno - editor->openfile->edittop->lineno));
//     sprintf(linenobuffer, "%*lu ", (margin - 1), line->lineno);
//     vertex_buffer_add_string(editor->buffer, linenobuffer, margin, NULL, gui_font_get_font(textfont), vec4(1.0f), &pen);
//   }
// }

static void gui_draw_row_linenum(linestruct *const line, Editor *const editor) {
  // static Color text_color = {1, 1, 1, 1};
  char linenobuffer[editor->margin + 1];
  float x;
  float y;
  if (refresh_needed && ISSET(LINE_NUMBERS)) {
    x = editor->gutter->x;
    y = (gui_font_row_baseline(textfont, (line->lineno - editor->openfile->edittop->lineno)) + editor->gutter->y);
    sprintf(linenobuffer, "%*lu ", (editor->margin - 1), line->lineno);
    font_vertbuf_add_mbstr(textfont, editor->buffer, linenobuffer, editor->margin, NULL, PACKED_UINT(255, 255, 255, 255), &x, &y);
  }
}

/* Draw one row onto the gui window. */
// static void gui_draw_row(linestruct *line, guieditor *editor, vec2 *drawpos) {
//   /* When debugging is enabled, assert everything we will use. */
//   ASSERT(line);
//   ASSERT(editor);
//   ASSERT(drawpos);
//   SyntaxObject *obj;
//   const char *prev_char = NULL;
//   char /* linenobuffer[margin + 1], */ *converted;
//   Ulong from_col, converted_len;
//   /* Draw the number of this line. */
//   gui_draw_row_linenum(line, editor);
//   /* Return early if the line is empty. */
//   if (!*line->data) {
//     return;
//   }
//   from_col  = get_page_start(wideness(line->data, ((line == editor->openfile->current) ? editor->openfile->current_x : 0)));
//   converted = display_string(line->data, from_col, editor->cols, TRUE, FALSE);
//   drawpos->x = editor->text->pos.x;
//   if (refresh_needed) {
//     converted_len = strlen(converted);
//     /* For c/cpp files. */
//     if (ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX)) {
//       if ((editor->openfile->is_c_file || editor->openfile->is_cxx_file) /* editor->openfile->type.is_set<C_CPP>() */) {
//         Ulong index = 0;
//         line_word_t *head = get_line_words(converted, converted_len);
//         while (head) {
//           line_word_t *node = head;
//           head = node->next;
//           if (sf) {
//             obj = (SyntaxObject *)hashmap_get(sf->objects, node->str);
//             if (obj) {
//               if (obj->color == SYNTAX_COLOR_BLUE) {
//                 vertex_buffer_add_mbstr(editor->buffer, (converted + index), (node->start - index), prev_char, textfont, vec4(1.0f), drawpos);
//                 vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), VEC4_8BIT(36, 114, 200, 1), drawpos);
//                 index = node->end;
//                 free_node(node);
//                 continue;
//               }
//               else if (obj->color == SYNTAX_COLOR_GREEN) {
//                 vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//                 vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), VEC4_8BIT(13, 188, 121, 1), drawpos);
//                 index = node->end;
//                 free_node(node);
//                 continue;
//               }
//             }
//           }
//           /* The global map. */
//           if (test_map.find(node->str) != test_map.end()) {
//             vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//             vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(test_map[node->str].color), drawpos);
//             index = node->end;
//           }
//           /* The variable map in the language server. */
//           else if (LSP->index.vars.find(node->str) != LSP->index.vars.end()) {
//             for (const auto &v : LSP->index.vars[node->str]) {
//               if (strcmp(tail(v.file), tail(openfile->filename)) == 0) {
//                 if (line->lineno >= v.decl_st && line->lineno <= v.decl_end) {
//                   vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//                   vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(FG_VS_CODE_BRIGHT_CYAN), drawpos);
//                   index = node->end;
//                 }
//               }
//             }
//           }
//           /* The define map in the language server. */
//           else if (LSP->index.defines.find(node->str) != LSP->index.defines.end()) {
//             vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//             vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(FG_VS_CODE_BLUE), drawpos);
//             index = node->end;
//           }
//           /* The map for typedefined structs and normal structs. */
//           else if (LSP->index.tdstructs.find(node->str) != LSP->index.tdstructs.end() || LSP->index.structs.find(node->str) != LSP->index.structs.end()) {
//             vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//             vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(FG_VS_CODE_GREEN), drawpos);
//             index = node->end;
//           }
//           /* The function name map in the language server. */
//           else if (LSP->index.functiondefs.find(node->str) != LSP->index.functiondefs.end()) {
//             vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//             vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(FG_VS_CODE_BRIGHT_YELLOW), drawpos);
//             index = node->end;
//           }
//           free_node(node);
//         }
//         vertex_buffer_add_string(editor->buffer, (converted + index), (converted_len - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//       }
//       /* AT&T asm syntax. */
//       else if (editor->openfile->is_atnt_asm_file /* editor->openfile->type.is_set<ATNT_ASM>() */) {
//         vec4 color;
//         Ulong index = 0;
//         line_word_t *head, *node;
//         const char *comment = strchr(converted, '#');
//         if (comment) {
//           vec2 origin = *drawpos;
//           origin.x += string_pixel_offset(converted, (ISSET(LINE_NUMBERS) ? " " : NULL), (comment - converted), gui_font_get_font(textfont));
//           vertex_buffer_add_string(
//             editor->buffer,
//             (converted + (comment - converted)),
//             (converted_len - (comment - converted)),
//             ((comment - converted) ? &converted[(comment - converted) - 1] : NULL),
//             gui_font_get_font(textfont),
//             GUI_DEFAULT_COMMENT_COLOR,
//             &origin
//           );
//         }
//         /* If the comment is not the first char. */
//         if (!comment || (comment - converted)) {
//           head = get_line_words(converted, (comment ? (comment - converted) : converted_len));
//           while (head) {
//             node = head;
//             head = node->next;
//             if (syntax_map_exists(ATNT_ASM, node->str, &color)) {
//               vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//               vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color, drawpos);
//               index = node->end;
//             }
//             free_node(node);
//           }
//           vertex_buffer_add_string(editor->buffer, (converted + index), ((comment ? (comment - converted) : converted_len) - index), prev_char, gui_font_get_font(textfont), 1, drawpos);
//         }
//       }
//       /* Nasm syntax. */
//       else if (openfile->is_nasm_file /* openfile->type.is_set<ASM>() */) {
//         const char *comment = strchr(converted, ';');
//         if (comment) {
//           vec2 origin = *drawpos;
//           origin.x += string_pixel_offset(converted, (ISSET(LINE_NUMBERS) ? " " : NULL), (comment - converted), gui_font_get_font(textfont));
//           vertex_buffer_add_string(
//             editor->buffer,
//             (converted + (comment - converted)),
//             (converted_len - (comment - converted)),
//             ((comment - converted) ? &converted[(comment - converted) - 1] : NULL),
//             gui_font_get_font(textfont),
//             GUI_DEFAULT_COMMENT_COLOR,
//             &origin
//           );
//         }
//         /* If the comment is not the first char. */
//         if (!comment || (comment - converted)) {
//           vertex_buffer_add_string(
//             editor->buffer,
//             converted,
//             (comment ? (comment - converted) : converted_len),
//             (ISSET(LINE_NUMBERS) ? " " : NULL),
//             gui_font_get_font(textfont),
//             vec4(1.0f),
//             drawpos
//           );
//         }
//       }
//       /* Bash syntax. */
//       else if (openfile->is_bash_file /* editor->openfile->type.is_set<BASH>() */) {
//         const char *comment = strchr(converted, '#');
//         /* When there is a comment on this line. */
//         if (comment) {
//           if (converted[(comment - converted) - 1] == '$') {
//             comment = NULL;
//           }
//           else {
//             vec2 origin = *drawpos;
//             origin.x += string_pixel_offset(converted, (ISSET(LINE_NUMBERS) ? " " : NULL), (comment - converted), gui_font_get_font(textfont));
//             vertex_buffer_add_mbstr(
//               editor->buffer,
//               (converted + (comment - converted)),
//               (converted_len - (comment - converted)),
//               ((comment - converted) ? &converted[(comment - converted) - 1] : NULL),
//               // gui_font_get_font(textfont),
//               textfont,
//               GUI_DEFAULT_COMMENT_COLOR,
//               &origin
//             );
//           }
//         }
//         /* If the comment is not the first char. */
//         if (!comment || (comment - converted)) {
//           vertex_buffer_add_mbstr(
//             editor->buffer,
//             converted,
//             (comment ? (comment - converted) : converted_len),
//             (ISSET(LINE_NUMBERS) ? " " : NULL),
//             textfont,
//             GUI_WHITE_COLOR,
//             drawpos
//           );
//         }
//       }
//       /* Otherwise just draw white text. */
//       else {
//         vertex_buffer_add_string(editor->buffer, converted, converted_len, prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//       }
//     }
//     /* Otherwise just draw white text. */
//     else {
//       vertex_buffer_add_string(editor->buffer, converted, converted_len, prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
//     }
//   }
//   draw_marked(editor, line, converted, from_col);
//   free(converted);
// }

static void gui_draw_row(linestruct *line, Editor *editor, vec2 *drawpos) {
  /* When debugging is enabled, assert everything we will use. */
  ASSERT(line);
  ASSERT(editor);
  ASSERT(drawpos);
  SyntaxObject *obj;
  const char *prev_char = NULL;
  char /* linenobuffer[margin + 1], */ *converted;
  Ulong from_col, converted_len;
  /* Draw the number of this line. */
  gui_draw_row_linenum(line, editor);
  /* Return early if the line is empty. */
  if (!*line->data) {
    return;
  }
  from_col  = get_page_start(wideness(line->data, ((line == editor->openfile->current) ? editor->openfile->current_x : 0)), editor->cols);
  converted = display_string(line->data, from_col, editor->cols, TRUE, FALSE);
  if (!converted || !*converted) {
    return;
  }
  drawpos->x = editor->text->x;
  if (refresh_needed) {
    converted_len = strlen(converted);
    /* For c/cpp files. */
    if (ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX)) {
      if ((editor->openfile->is_c_file || editor->openfile->is_cxx_file) /* editor->openfile->type.is_set<C_CPP>() */) {
        Ulong index = 0;
        /* For some wierd reason this causes a crash on startup sometimes witch is wierd as this runs alot and never crashes otherwise. */
        line_word_t *head = get_line_words(converted, converted_len);
        while (head) {
          line_word_t *node = head;
          head = node->next;
          if (sf) {
            obj = (SyntaxObject *)hashmap_get(sf->objects, node->str);
            if (obj) {
              if (obj->color == SYNTAX_COLOR_BLUE) {
                vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
                vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), VEC4_8BIT(36, 114, 200, 1), drawpos);
                index = node->end;
                free_node(node);
                continue;
              }
              else if (obj->color == SYNTAX_COLOR_GREEN) {
                vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
                vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), VEC4_8BIT(13, 188, 121, 1), drawpos);
                index = node->end;
                free_node(node);
                continue;
              }
            }
          }
          /* The global map. */
          if (test_map.find(node->str) != test_map.end()) {
            vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
            vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(test_map[node->str].color), drawpos);
            index = node->end;
          }
          /* The variable map in the language server. */
          else if (LSP->index.vars.find(node->str) != LSP->index.vars.end()) {
            for (const auto &v : LSP->index.vars[node->str]) {
              if (strcmp(tail(v.file), tail(openfile->filename)) == 0) {
                if (line->lineno >= v.decl_st && line->lineno <= v.decl_end) {
                  vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
                  vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(FG_VS_CODE_BRIGHT_CYAN), drawpos);
                  index = node->end;
                }
              }
            }
          }
          /* The define map in the language server. */
          else if (LSP->index.defines.find(node->str) != LSP->index.defines.end()) {
            vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
            vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(FG_VS_CODE_BLUE), drawpos);
            index = node->end;
          }
          /* The map for typedefined structs and normal structs. */
          else if (LSP->index.tdstructs.find(node->str) != LSP->index.tdstructs.end() || LSP->index.structs.find(node->str) != LSP->index.structs.end()) {
            vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
            vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(FG_VS_CODE_GREEN), drawpos);
            index = node->end;
          }
          /* The function name map in the language server. */
          else if (LSP->index.functiondefs.find(node->str) != LSP->index.functiondefs.end()) {
            vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
            vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color_idx_to_vec4(FG_VS_CODE_BRIGHT_YELLOW), drawpos);
            index = node->end;
          }
          free_node(node);
        }
        vertex_buffer_add_string(editor->buffer, (converted + index), (converted_len - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
      }
      /* AT&T asm syntax. */
      else if (editor->openfile->is_atnt_asm_file /* editor->openfile->type.is_set<ATNT_ASM>() */) {
        vec4 color;
        Ulong index = 0;
        line_word_t *head, *node;
        const char *comment = strchr(converted, '#');
        if (comment) {
          vec2 origin = *drawpos;
          origin.x += string_pixel_offset(converted, (ISSET(LINE_NUMBERS) ? " " : NULL), (comment - converted), gui_font_get_font(textfont));
          vertex_buffer_add_string(
            editor->buffer,
            (converted + (comment - converted)),
            (converted_len - (comment - converted)),
            ((comment - converted) ? &converted[(comment - converted) - 1] : NULL),
            gui_font_get_font(textfont),
            GUI_DEFAULT_COMMENT_COLOR,
            &origin
          );
        }
        /* If the comment is not the first char. */
        if (!comment || (comment - converted)) {
          head = get_line_words(converted, (comment ? (comment - converted) : converted_len));
          while (head) {
            node = head;
            head = node->next;
            if (syntax_map_exists(ATNT_ASM, node->str, &color)) {
              vertex_buffer_add_string(editor->buffer, (converted + index), (node->start - index), prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
              vertex_buffer_add_string(editor->buffer, (converted + node->start), node->len, prev_char, gui_font_get_font(textfont), color, drawpos);
              index = node->end;
            }
            free_node(node);
          }
          vertex_buffer_add_string(editor->buffer, (converted + index), ((comment ? (comment - converted) : converted_len) - index), prev_char, gui_font_get_font(textfont), 1, drawpos);
        }
      }
      /* Nasm syntax. */
      else if (openfile->is_nasm_file /* openfile->type.is_set<ASM>() */) {
        const char *comment = strchr(converted, ';');
        if (comment) {
          vec2 origin = *drawpos;
          origin.x += string_pixel_offset(converted, (ISSET(LINE_NUMBERS) ? " " : NULL), (comment - converted), gui_font_get_font(textfont));
          vertex_buffer_add_string(
            editor->buffer,
            (converted + (comment - converted)),
            (converted_len - (comment - converted)),
            ((comment - converted) ? &converted[(comment - converted) - 1] : NULL),
            gui_font_get_font(textfont),
            GUI_DEFAULT_COMMENT_COLOR,
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
            gui_font_get_font(textfont),
            vec4(1.0f),
            drawpos
          );
        }
      }
      /* Bash syntax. */
      else if (openfile->is_bash_file /* editor->openfile->type.is_set<BASH>() */) {
        const char *comment = strchr(converted, '#');
        /* When there is a comment on this line. */
        if (comment) {
          if (converted[(comment - converted) - 1] == '$') {
            comment = NULL;
          }
          else {
            vec2 origin = *drawpos;
            origin.x += string_pixel_offset(converted, (ISSET(LINE_NUMBERS) ? " " : NULL), (comment - converted), gui_font_get_font(textfont));
            vertex_buffer_add_mbstr(
              editor->buffer,
              (converted + (comment - converted)),
              (converted_len - (comment - converted)),
              ((comment - converted) ? &converted[(comment - converted) - 1] : NULL),
              // gui_font_get_font(textfont),
              textfont,
              GUI_DEFAULT_COMMENT_COLOR,
              &origin
            );
          }
        }
        /* If the comment is not the first char. */
        if (!comment || (comment - converted)) {
          vertex_buffer_add_mbstr(
            editor->buffer,
            converted,
            (comment ? (comment - converted) : converted_len),
            (ISSET(LINE_NUMBERS) ? " " : NULL),
            textfont,
            GUI_WHITE_COLOR,
            drawpos
          );
        }
      }
      /* Otherwise just draw white text. */
      else {
        // vertex_buffer_add_string(editor->buffer, converted, converted_len, prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
        font_vertbuf_add_mbstr(textfont, editor->buffer, converted, converted_len, prev_char, PACKED_UINT_FLOAT(1, 1, 1, 1), &drawpos->x, &drawpos->y);
      }
    }
    /* Otherwise just draw white text. */
    else {
      vertex_buffer_add_string(editor->buffer, converted, converted_len, prev_char, gui_font_get_font(textfont), vec4(1.0f), drawpos);
    }
    draw_marked(editor, line, converted, from_col);
  }
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
  statusline_gui_timed(REMARK, 2, "%s %s", epithet_of_flag(flag), (ISSET(flag) ? "enabled" : "disabled"));
}

/* Draw a editor. */
// void draw_editor(guieditor *editor) {
//   /* When dubugging is enabled, assert everything we use. */
//   ASSERT(editor);
//   ASSERT(editor->openfile);
//   ASSERT(editor->openfile->edittop);
//   ASSERT(editor->text);
//   ASSERT(editor->buffer);
//   ASSERT(editor->gutter);
//   ASSERT(gui->font_shader);
//   ASSERT(gui);
//   ASSERT(textfont);
//   ASSERT(uifont);
//   vec2 pen = 0;
//   int row = 0;
//   /* Start at the top of the text window. */
//   linestruct *line = editor->openfile->edittop;
//   /* When the editor is hidden, just return. */
//   if (editor->flag.is_set<GUIEDITOR_HIDDEN>()) {
//     return;
//   }
//   /* Draw the text editor element. */
//   gui_element_draw(editor->text);
//   /* Draw the gutter element of the editor. */
//   gui_element_draw(editor->gutter);
//   /* Render the text element of the editor. */
//   if (refresh_needed) {
//     vertex_buffer_clear(editor->buffer);
//     while (line && row++ < (int)editor->rows) {
//       pen.x = editor->text->pos.x;
//       pen.y = (gui_font_row_baseline(textfont, (line->lineno - editor->openfile->edittop->lineno)) + editor->text->pos.y);
//       gui_draw_row(line, editor, &pen);
//       line = line->next;
//     }
//     if (!gui->flag.is_set<GUI_PROMPT>() && ((editor->openfile->current->lineno - editor->openfile->edittop->lineno) >= 0)) {
//       line_add_cursor(
//         (editor->openfile->current->lineno - editor->openfile->edittop->lineno),
//         textfont,
//         editor->buffer,
//         vec4(1),
//         gui_editor_cursor_x_pos(editor, editor->openfile->current, editor->openfile->current_x),
//         // cursor_pixel_x_pos(gui_font_get_font(textfont)),
//         editor->text->pos.y);
//     }
//   }
//   else {
//     while (line && row++ < (int)editor->rows) {
//       gui_draw_row(line, editor, &pen);
//       line = line->next;
//     }
//   }
//   upload_texture_atlas(gui_font_get_atlas(textfont));
//   render_vertex_buffer(gui->font_shader, editor->buffer);
//   /* Draw the top bar for the editor.  Where the open buffer names are displayd. */
//   gui_etb_draw(editor->etb);
//   gui_scrollbar_draw(editor->sb);
// }

/* Draw a editor. */
void draw_editor(Editor *editor) {
  /* When dubugging is enabled, assert everything we use. */
  ASSERT(editor);
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->edittop);
  ASSERT(editor->text);
  ASSERT(editor->buffer);
  ASSERT(editor->gutter);
  ASSERT(font_shader);
  ASSERT(gui);
  ASSERT(textfont);
  ASSERT(uifont);
  vec2 pen = 0;
  int row = 0;
  /* Start at the top of the text window. */
  linestruct *line = editor->openfile->edittop;
  /* When the editor is hidden, just return. */
  if (editor->hidden) {
    return;
  }
  /* Draw the text editor element. */
  element_draw(editor->text);
  /* Draw the gutter element of the editor. */
  element_draw(editor->gutter);
  /* Render the text element of the editor. */
  if (refresh_needed) {
    writef("hello\n");
    vertex_buffer_clear(editor->buffer);
    vertex_buffer_clear(editor->marked_region_buf);
    while (line && row++ < (int)editor->rows) {
      pen.x = editor->text->x;
      pen.y = (gui_font_row_baseline(textfont, (line->lineno - editor->openfile->edittop->lineno)) + editor->text->y);
      gui_draw_row(line, editor, &pen);
      line = line->next;
    }
    if (!gui->flag.is_set<GUI_PROMPT>() && ((editor->openfile->current->lineno - editor->openfile->edittop->lineno) >= 0)) {
      line_add_cursor(
        (editor->openfile->current->lineno - editor->openfile->edittop->lineno),
        textfont,
        editor->buffer,
        vec4(1),
        editor_cursor_x_pos(editor, editor->openfile->current, editor->openfile->current_x),
        // cursor_pixel_x_pos(gui_font_get_font(textfont)),
        editor->text->y
      );
    }
  }
  // else {
  //   while (line && row++ < (int)editor->rows) {
  //     gui_draw_row(line, editor, &pen);
  //     line = line->next;
  //   }
  // }
  // upload_texture_atlas(gui_font_get_atlas(textfont));
  // render_vertex_buffer(gui->font_shader, editor->buffer);
  vertex_buffer_render(editor->marked_region_buf, GL_TRIANGLES);
  render_vertbuf(textfont, editor->buffer);
  /* Draw the top bar for the editor.  Where the open buffer names are displayd. */
  etb_draw(editor->tb);
  scrollbar_draw(editor->sb);
}

/* Draw the top bar of the gui. */
void draw_topbar(void) {
  if (gui->flag.is_set<GUI_PROMPT>()) {
    gui_promptmenu_resize();
    // gui_element_draw(gui->promptmenu->element);
    element_draw(gui->promptmenu->element);
    gui_promptmenu_draw_selected();
    // gui_scrollbar_draw(gui->promptmenu->sb);
    scrollbar_draw(gui->promptmenu->sb);
    gui_promptmenu_draw_text();
  }
}

/* Draw the gui suggestmenu. */
void draw_suggestmenu(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->menu);
  menu_draw(gui->suggestmenu->menu);
}

/* Draw the bottom bar of the gui. */
void draw_botbar(void) {
  element_draw(gui->botbar);
}

/* Draw the status bar for the gui. */
void draw_statusbar(void) {
  float msg_width;
  float x;
  float y;
  if (statustype != VACUUM) {
    /* Check it the message has been shown for the set time. */
    statustime -= (1.0f / frametimer.fps);
    if (statustime < 0) {
      /* If the set time has elapsed, then reset the status element. */
      statustype = VACUUM;
      // gui->statusbar->flag.set<GUIELEMENT_HIDDEN>();
      // gui->statusbar->hidden = TRUE;
      gui->statusbar->xflags |= ELEMENT_HIDDEN;
      return;
    }
    if (refresh_needed) {
      // gui->statusbar->flag.unset<GUIELEMENT_HIDDEN>();
      // gui->statusbar->hidden = FALSE;
      gui->statusbar->xflags &= ~ELEMENT_HIDDEN;
      msg_width = (font_breadth(uifont, statusmsg) + font_breadth(uifont, "  "));
      vertex_buffer_clear(gui->statusbuf);
      // gui_element_move_resize(
      //   gui->statusbar,
      //   vec2((((float)gui->width / 2) - (msgwidth / 2)), (gui->height - gui->botbar->size.h - gui->statusbar->size.h)),
      //   vec2(msgwidth, gui_font_height(uifont) /* FONT_HEIGHT(uifont) */)
      // );
      // vec2 penpos((gui->statusbar->pos.x + pixbreadth(gui_font_get_font(uifont), " ")), (/* row_baseline_pixel(0, uifont) */ gui_font_row_baseline(uifont, 0) + gui->statusbar->pos.y));
      element_move_resize(gui->statusbar, ((gui_width / 2) - (msg_width / 2)), (gui_height - gui->botbar->height - gui->statusbar->height), msg_width, gui_font_height(uifont));
      // vertex_buffer_add_string(gui->statusbuf, statusmsg, strlen(statusmsg), " ", gui_font_get_font(uifont), 1, &penpos);
      font_vertbuf_add_mbstr(uifont, gui->statusbuf, statusmsg, strlen(statusmsg), " ", PACKED_UINT(255, 255, 255, 255), &x, &y);
    }
    // gui_element_draw(gui->statusbar);
    element_draw(gui->statusbar);
    upload_texture_atlas(gui_font_get_atlas(uifont));
    render_vertex_buffer(font_shader, gui->statusbuf);
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
  // static ivec4 rect = 0.0f;
  // GLFWmonitor *monitor;
  // const GLFWvidmode *mode;
  if (!is_fullscreen) {
    // /* Save the window size and position. */
    // glfwGetWindowPos(window, &rect.x, &rect.y);
    // glfwGetWindowSize(window, &rect.width, &rect.height);
    // /* Get monitor size. */
    // monitor = glfwGetPrimaryMonitor();
    // if (!monitor) {
    //   log_error_gui("%s: Monitor is invalid.\n", __func__);
    //   return;
    // }
    // mode = glfwGetVideoMode(monitor);
    // if (!mode) {
    //   log_error_gui("%s: Mode is invalid.\n", __func__);
    //   return;
    // }
    /* Set the window pos and size. */
    // glfwSetWindowAttrib(window, GLFW_AUTO_ICONIFY, FALSE);
    glfwMaximizeWindow(window);
    // glfwSetWindowMonitor(window, NULL, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
  }
  else {
    // writef("x: %d\n", rect.x);
    // writef("x: %d\n", rect.y);
    /* Set the window pos and size. */
    // glfwSetWindowMonitor(window, NULL, rect.x, rect.y, rect.width, rect.height, GLFW_DONT_CARE);
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
