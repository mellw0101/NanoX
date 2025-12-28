/** @file gui/editor/editor.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../../../include/c_proto.h"


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Editor create internal ----------------------------- */

static Editor *editor_create_internal(void) {
  Editor *editor = xmalloc(sizeof(*editor));
  /* Boolian flags. */
  editor->should_close = FALSE;
  editor->hidden       = FALSE;
  /* Vertex buffer. */
  editor->buffer = vertbuf_create();
  /* Marked region rect buffer. */
  editor->marked_region_buf = vertex_buffer_new(RECT_VERTBUF);
  /* Openfile's */
  editor->startfile = NULL;
  editor->openfile  = NULL;
  /* Element's */
  editor->main   = NULL;
  editor->gutter = NULL;
  editor->text   = NULL;
  /* Scrollbar. */
  editor->sb = NULL;
  /* Topbar. */
  editor->tb = NULL;
  /* Row's and col's. */
  editor->rows   = 0;
  editor->cols   = 0;
  editor->margin = 0;
  /* List ptr's. */
  editor->prev = NULL;
  editor->next = NULL;
  return editor;
}

/* ----------------------------- Editor get gutter width ----------------------------- */

static float editor_get_gutter_width(Editor *const editor) {
  ASSERT_EDITOR(editor);
  char *linenostr;
  float ret = 0;
  editor_confirm_margin(editor);
  if (editor->margin) {
    linenostr = fmtstr("%*lu ", (editor->margin - 1), editor->openfile->filebot->lineno);
    ret = font_breadth(textfont, linenostr);
    free(linenostr);
  }
  return ret;
}

/* ----------------------------- Editor scrollbar update routine ----------------------------- */

/* The editor's routine to fetch the data the scrollbar need's. */
static void editor_scrollbar_update_routine(void *arg, float *total_length,
  Uint *start, Uint *end, Uint *visible, Uint *current, float *t_offset, float *r_offset)
{
  ASSERT(arg);
  Editor *editor = arg;
  ASSIGN_IF_VALID(total_length, editor->text->height);
  ASSIGN_IF_VALID(start, editor->openfile->filetop->lineno);
  ASSIGN_IF_VALID(end, editor->openfile->filebot->lineno);
  ASSIGN_IF_VALID(visible, editor->rows);
  ASSIGN_IF_VALID(current, editor->openfile->edittop->lineno);
  ASSIGN_IF_VALID(t_offset, 0);
  ASSIGN_IF_VALID(r_offset, 0);
}

/* ----------------------------- Editor scrollbar moving routine ----------------------------- */

/* The editor's routine to respond to a calculated index based on the scrollbars's current position. */
static void editor_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  Editor *editor = arg;
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->edittop);
  editor->openfile->edittop = line_from_number_for(editor->openfile, index);
}

/* ----------------------------- Editor scrollbar create ----------------------------- */

/* Create the editor scrollbar. */
static void editor_scrollbar_create(Editor *const editor) {
  ASSERT(editor);
  ASSERT(editor->text);
  editor->sb = scrollbar_create(editor->text, editor, editor_scrollbar_update_routine, editor_scrollbar_moving_routine);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Editor create ----------------------------- */

void editor_create(bool new_buffer) {
  Editor *node = editor_create_internal();
  if (!openeditor) {
    CLIST_INIT(node);
    starteditor     = node;
    node->openfile  = openfile;
    node->startfile = startfile;
    editor_set_rows_cols(node, gl_window_width(), gl_window_height());
    editwinrows = node->rows;
    editwincols = node->cols;
  }
  else {
    CLIST_INSERT_AFTER(node, openeditor);
    if (new_buffer) {
      make_new_buffer_for(&node->startfile, &node->openfile);
      startfile = node->startfile;
      openfile  = node->openfile;
    }
    node->openfile  = openfile;
    node->startfile = startfile;
  }
  openeditor = node;
  /* Main element. */
  openeditor->main = element_create(0, 0, gl_window_width(), gl_window_height(), FALSE);
  element_set_data_editor(openeditor->main, openeditor);
  /* Gutter element. */
  openeditor->gutter = element_create(openeditor->main->x, (openeditor->main->y + font_height(uifont)), editor_get_gutter_width(openeditor), (openeditor->main->height - font_height(uifont)), TRUE);
  element_set_parent(openeditor->gutter, openeditor->main);
  element_set_data_editor(openeditor->gutter, openeditor);
  openeditor->gutter->color = PACKED_UINT_EDIT_BACKGROUND;
  openeditor->gutter->xflags |= (ELEMENT_REL_POS | ELEMENT_REL_HEIGHT);
  openeditor->gutter->rel_y = font_height(uifont);
  /* Text element. */
  openeditor->text = element_create((openeditor->main->x + openeditor->gutter->width), (openeditor->main->y + font_height(uifont)), (openeditor->main->width - openeditor->gutter->width), (openeditor->main->height - font_height(uifont)), TRUE);
  element_set_parent(openeditor->text, openeditor->main);
  element_set_data_editor(openeditor->text, openeditor);
  openeditor->text->color = PACKED_UINT_EDIT_BACKGROUND;
  openeditor->text->xflags |= (ELEMENT_REL_POS | ELEMENT_REL_SIZE /* | ELEMENT_ROUNDED_RECT */);
  openeditor->text->rel_x = openeditor->gutter->width;
  openeditor->text->rel_y = font_height(uifont);
  // openeditor->text->cursor     = GLFW_IBEAM_CURSOR;
  openeditor->text->cursor = SDL_SYSTEM_CURSOR_TEXT;
  /* Create the text scrollbar. */
  editor_scrollbar_create(openeditor);
  /* Create the editor topbar. */
  openeditor->tb = etb_create(openeditor);
}

/* ----------------------------- Editor free ----------------------------- */

void editor_free(Editor *const editor) {
  /* Make this function `NO-OP`. */
  if (!editor) {
    return;
  }
  vertex_buffer_delete(editor->buffer);
  vertex_buffer_delete(editor->marked_region_buf);
  element_free(editor->main);
  etb_free(editor->tb);
  free(editor->sb);
  free(editor);
}

/* ----------------------------- Editor confirm margin ----------------------------- */

void editor_confirm_margin(Editor *const editor) {
  ASSERT_EDITOR(editor);
  bool keep_focus;
  int needed_margin = (digits(editor->openfile->filebot->lineno) + 1);
  if (!ISSET(LINE_NUMBERS)) {
    needed_margin = 0;
  }
  if (needed_margin != editor->margin) {
    keep_focus     = ((editor->margin > 0) && focusing);
    editor->margin = needed_margin;
    /* Ensure a proper starting column for the first screen row. */
    ensure_firstcolumn_is_aligned_for(editor->openfile, editor->cols);
    focusing = keep_focus;
    refresh_needed = TRUE;
  }
}

/* ----------------------------- Editor set rows cols ----------------------------- */

/* Calculate the row's and column's of a `editor` based on the size of it's text element. */
void editor_set_rows_cols(Editor *const editor, float width, float height) {
  ASSERT(editor);
  int rows;
  int cols;
  font_rows_cols(textfont, width, height, &rows, &cols);
  editor->rows = rows;
  editor->cols = cols;
}

/* ----------------------------- Editor from file ----------------------------- */

/* Get the editor that `file` belongs to. */
Editor *editor_from_file(openfilestruct *const file) {
  ASSERT(file);
  CLIST_ITER(starteditor, editor, CLIST_ITER(editor->startfile, afile,
    if (afile == file) {
      return editor;
    }
  ););
  log_ERR_FA("This should never happen, every openfile must be linked to a editor.");
}

/* ----------------------------- Editor hide ----------------------------- */

void editor_hide(Editor *const editor, bool hide) {
  ASSERT(editor);
  editor->hidden = hide;
  if (hide) {
    editor->main->xflags   |= ELEMENT_HIDDEN;
    editor->gutter->xflags |= ELEMENT_HIDDEN;
    editor->text->xflags   |= ELEMENT_HIDDEN;
  }
  else {
    editor->main->xflags   &= ~ELEMENT_HIDDEN;
    editor->gutter->xflags &= ~ELEMENT_HIDDEN;
    editor->text->xflags   &= ~ELEMENT_HIDDEN;
  }
  etb_show_context_menu(editor->tb, NULL, FALSE);
}

/* ----------------------------- Editor close ----------------------------- */

void editor_close(Editor *const editor) {
  ASSERT_EDITOR(editor);
  if (editor == starteditor) {
    CLIST_ADV_NEXT(starteditor);
  }
  CLIST_UNLINK(editor);
  if (editor == openeditor) {
    editor_hide(openeditor, TRUE);
    CLIST_ADV_PREV(openeditor);
    if (editor == openeditor) {
      openeditor  = NULL;
      starteditor = NULL;
      openfile   = NULL;
      startfile  = NULL;
    }
    else {
      openfile  = openeditor->openfile;
      startfile = openeditor->startfile;
      editor_hide(openeditor, FALSE);
    }
  }
  editor_free(editor);
}

/* ----------------------------- Editor resize ----------------------------- */

void editor_resize(Editor *const editor) {
  ASSERT(editor);
  ASSERT(editor->text);
  editor_confirm_margin(editor);
  editor->gutter->width = editor_get_gutter_width(editor);
  editor->text->rel_x   = editor->gutter->width;
  if (!editor->gutter->width) {
    editor->gutter->xflags |= ELEMENT_HIDDEN;
  }
  else {
    editor->gutter->xflags &= ~ELEMENT_HIDDEN;
  }
  // editor->gutter->hidden   = !editor->gutter->width;
  element_move_resize(editor->main, 0, 0, gl_window_width(), (gl_window_height() - font_height(uifont)));
  editor_set_rows_cols(editor, editor->text->width, editor->text->height);
  etb_text_refresh_needed(editor->tb);
  scrollbar_refresh(editor->sb);
}

/* ----------------------------- Editor redecorate ----------------------------- */

/* Used to ensure the correct state of an editor after we have switched to a new one or changed the currently open file. */
void editor_redecorate(Editor *const editor) {
  ASSERT_EDITOR(editor);
  /* If there is a error with the currently opened file, show it in the statusbar and log it. */
  if (editor->openfile->errormessage) {
    statusline(ALERT, "%s", editor->openfile->errormessage);
    log_ERR_NF("%s", editor->openfile->errormessage);
    free(editor->openfile->errormessage);
    editor->openfile->errormessage = NULL;
  }
  ensure_firstcolumn_is_aligned_for(editor->openfile, editor->cols);
  currmenu       = MMOST;
  shift_held     = TRUE;
  refresh_needed = TRUE;
  scrollbar_refresh(editor->sb);
}

/* ----------------------------- Editor switch to prev ----------------------------- */

/* Switch to the previous editor.  */
void editor_switch_to_prev(void) {
  ASSERT_EDITOR(openeditor);
  /* If there is only one editor open, just print a message and return. */
  if (CLIST_SINGLE(openeditor)) {
    statusline(AHEM, _("Only one editor open"));
    return;
  }
  CLIST_ADV_PREV(openeditor);
  openfile  = openeditor->openfile;
  startfile = openeditor->startfile;
  editor_hide(openeditor->next, TRUE);
  editor_hide(openeditor, FALSE);
  editor_redecorate(openeditor);
  editor_resize(openeditor);
  editwinrows = openeditor->rows;
}

/* ----------------------------- Editor switch to next ----------------------------- */

/* Switch to the next editor. */
void editor_switch_to_next(void) {
  ASSERT_EDITOR(openeditor);
  /* When there is only a single open editor, just tell the user and return. */
  if (CLIST_SINGLE(openeditor)) {
    statusline(AHEM, _("Only one editor open"));
    return;
  }
  CLIST_ADV_NEXT(openeditor);
  openfile  = openeditor->openfile;
  startfile = openeditor->startfile;
  editor_hide(openeditor->prev, TRUE);
  editor_hide(openeditor, FALSE);
  editor_redecorate(openeditor);
  editor_resize(openeditor);
  editwinrows = openeditor->rows;
}

/* ----------------------------- Editor switch openfile to prev ----------------------------- */

/* Within the currently open editor, switch to the prev buffer. */
void editor_switch_openfile_to_prev(void) {
  ASSERT_EDITOR(openeditor);
  /* If there is only one open buffer in the currently open editor, there is nothing to do. */
  if (CLIST_SINGLE(openeditor->openfile)) {
    statusline(AHEM, _("No more open file buffers in the current editor"));
    return;
  }
  CLIST_ADV_PREV(openeditor->openfile);
  openfile  = openeditor->openfile;
  startfile = openeditor->startfile;
  editor_redecorate(openeditor);
  editor_resize(openeditor);
  etb_active_refresh_needed(openeditor->tb);
}

/* ----------------------------- Editor switch openfile to next ----------------------------- */

/* Within the currently open editor, switch to the prev buffer. */
void editor_switch_openfile_to_next(void) {
  ASSERT_EDITOR(openeditor);
  ASSERT(openeditor->tb);
  /* If there is only one open file in the currently active editor, print a msg telling the user and return. */
  if (CLIST_SINGLE(openeditor->openfile)) {
    statusline(AHEM, _("No more open file buffers in the current editor"));
    return;
  }
  CLIST_ADV_NEXT(openeditor->openfile);
  openfile = openeditor->openfile;
  editor_redecorate(openeditor);
  editor_resize(openeditor);
  etb_active_refresh_needed(openeditor->tb);
}

/* ----------------------------- Editor set open ----------------------------- */

/* Set `openedit` to editor, if its not already. */
void editor_set_open(Editor *const editor) {
  ASSERT_EDITOR(openeditor);
  ASSERT_EDITOR(editor);
  /* Return early if editor is already the open editor. */
  if (editor == openeditor) {
    return;
  }
  /* Ensure the global ptr's to the openfile and startfile are set as the new open editor. */
  openfile  = editor->openfile;
  startfile = editor->startfile;
  openeditor  = editor;
  editor_redecorate(editor);
  editor_resize(editor);
}

/* ----------------------------- Editor check should close ----------------------------- */

/* Check if any editor's has it's `should_close` flag set, and if so close them. */
void editor_check_should_close(void) {
  bool close_starteditor = FALSE;
  CLIST_ITER(starteditor, editor,
    ASSERT_EDITOR(editor);
    if (editor->should_close) {
      if (editor == starteditor) {
        close_starteditor = TRUE;
      }
      else {
        editor_close(editor);
      }
    }
  );
  if (close_starteditor) {
    editor_close(starteditor);
  }
}

/* ----------------------------- Editor close open buffer ----------------------------- */

/* Close the currently open editor's currently open buffer. */
void editor_close_open_buffer(void) {
  ASSERT_EDITOR(openeditor);
  // free_one_buffer(openeditor->openfile, &openeditor->openfile, &openeditor->startfile);
  close_buffer();
  // openeditor->openfile  = openfile;
  // openeditor->startfile = startfile;
  etb_entries_refresh_needed(openeditor->tb);
}

/* ----------------------------- Editor open new empty buffer ----------------------------- */

/* Open a new empty `openfilestruct *` in the currently open editor. */
void editor_open_new_empty_buffer(void) {
  ASSERT_EDITOR(openeditor);
  /* Make a new buffer for the currently open editor. */
  make_new_buffer();
  editor_redecorate(openeditor);
  editor_resize(openeditor);
  etb_entries_refresh_needed(openeditor->tb);
}

/* ----------------------------- Editor update all ----------------------------- */

void editor_update_all(void) {
  ASSERT(starteditor);
  ASSERT(openeditor);
  CLIST_ITER(starteditor, editor,
    editor_redecorate(editor);
    editor_resize(editor);
  );
}

/* ----------------------------- Editor get page start ----------------------------- */

Ulong editor_get_page_start(Editor *const editor, const Ulong column) {
  ASSERT(editor);
  if (!column || (int)(column + 2) < editor->cols || ISSET(SOFTWRAP)) {
    return 0;
  }
  else if (editor->cols > 8) {
    return (column - 6 - (column - 6) % (editor->cols - 8));
  }
  else {
    return (column - (editor->cols - 2));
  }
}

/* ----------------------------- Editor cursor x pos ----------------------------- */

float editor_cursor_x_pos(Editor *const editor, linestruct *const line, Ulong index) {
  ASSERT(editor);
  ASSERT(line);
  Ulong from_col  = editor_get_page_start(openeditor, wideness(line->data, index));
  char *converted = display_string(line->data, from_col, editor->cols, TRUE, FALSE);
  // float ret = (string_pixel_offset(converted, NULL, (wideness(line->data, index) - from_col), gui_font_get_font(textfont)) + editor->text->pos.x);
  float ret = (font_wideness(textfont, converted, (wideness(line->data, index) - from_col)) + editor->text->x);
  free(converted);
  return ret;
}

/* ----------------------------- Editor get text line ----------------------------- */

linestruct *editor_get_text_line(Editor *const editor, float y_pos) {
  ASSERT(editor);
  ASSERT(editor->text);
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->edittop);
  long row;
  font_row_from_pos(textfont, editor->text->y, (editor->text->y + editor->text->height), y_pos, &row);
  return line_from_number_for(editor->openfile, lclamp((editor->openfile->edittop->lineno + row), editor->openfile->edittop->lineno, editor->openfile->filebot->lineno));
}

/* ----------------------------- Editor get text index ----------------------------- */

Ulong editor_get_text_index(Editor *const editor, linestruct *const line, float x_pos) {
  ASSERT(editor);
  ASSERT(line);
  return font_index_from_pos(textfont, line->data, strlen(line->data), x_pos, editor->text->x);
}

/* ----------------------------- Editor get text line index ----------------------------- */

void editor_get_text_line_index(Editor *const editor, float x_pos,
  float y_pos, linestruct **const outline, Ulong *const outindex)
{
  ASSERT(editor);
  ASSERT(outline);
  ASSERT(outindex);
  (*outline)  = editor_get_text_line(editor, y_pos);
  (*outindex) = editor_get_text_index(editor, (*outline), x_pos);
}

/* ----------------------------- Editor open buffer ----------------------------- */

/* Open a new buffer in the currently open editor using `path`. */
void editor_open_buffer(const char *const restrict path) {
  ASSERT_EDITOR(openeditor);
  ASSERT(path);
  openfilestruct *was_openfile = GUI_OF;
  /* In this case we should always terminate apon the file not existing as this function
   * should never be called in this case.  Note that this should be handeled before
   * the call to this function, because this function has one job, to open a file. */
  ALWAYS_ASSERT(file_exists(path));
  if (!open_buffer_for(&GUI_SF, &GUI_OF, GUI_RC, path, TRUE)) {
    return;
  }
  /* If the buffer this was called from is empty, then the newly opened one should replace it. */
  if (!*was_openfile->filename && !was_openfile->totsize) {
    close_buffer_for(was_openfile, &GUI_SF, &GUI_OF);
  }
  editor_redecorate(openeditor);
  editor_resize(openeditor);
  etb_entries_refresh_needed(openeditor->tb);
}

/* ----------------------------- Editor close a open buffer ----------------------------- */

void editor_close_a_open_buffer(openfilestruct *const file) {
  ASSERT(file);
  Editor *editor;
  ALWAYS_ASSERT((editor = editor_from_file(file)));
  if (file->lock_filename) {
    delete_lockfile(file->lock_filename);
  }
  if (!CLIST_SINGLE(file)) {
    close_buffer_for(file, &editor->startfile, &editor->openfile);
    if (editor == openeditor) {
      openfile  = editor->openfile;
      startfile = editor->startfile;
    }
    etb_entries_refresh_needed(editor->tb);
    editor_redecorate(editor);
    editor_resize(editor);
  }
  else {
    if (CLIST_SINGLE(editor)) {
      gl_window_should_quit();
    }
    else {
      editor->should_close = TRUE;
    }
  }
}

/* ----------------------------- Editor buffer save ----------------------------- */

/* Save any given buffer of any editor editor. */
void editor_buffer_save(openfilestruct *const file) {
  ASSERT(file);
  Editor *editor;
  openfilestruct *was_open;
  ALWAYS_ASSERT(editor = editor_from_file(file));
  /* If file is not the currently open buffer of the host editor.
   * Then save the currently open buffer so we can restore it later. */
  if (file != editor->openfile) {
    was_open = editor->openfile;
    editor->openfile = file;
    do_savefile_for(&editor->startfile, &editor->openfile, editor->cols);
    editor->openfile = was_open;
  }
  else {
    do_savefile_for(&editor->startfile, &editor->openfile, editor->cols);
  }
}

/* ----------------------------- Editor text line marked region ----------------------------- */

/* TODO: Here we really need to ensure that we make `from_x` and `till_x` non globals, as this will be bad later. */
void editor_text_line_marked_region(Editor *const editor,
  linestruct *const line, const char *const restrict data, Ulong from_col)
{
  ASSERT_EDITOR(editor);
  ASSERT(line);
  ASSERT(data);
  RectVertex vert[4];
  linestruct *top;
  linestruct *bot;
  Ulong xtop;
  Ulong xbot;
  int startcol;
  int endcol;
  int paintlen = -1;
  const char *thetext;
  float x;
  float y;
  float width;
  float height;
  float pix_top;
  float pix_bot;
  /* We only perform any action when the line is part of the marked region. */
  if (line_in_marked_region_for(editor->openfile, line)) {
    get_region_for(editor->openfile, &top, &xtop, &bot, &xbot);
    if (top->lineno < line->lineno || xtop < from_x) {
      xtop = from_x; 
    }
    if (bot->lineno > line->lineno || xbot > till_x) {
      xbot = till_x;
    }
    /* Only paint if the marked part of the line is on this page. */
    if (xtop < till_x && xbot > from_x) {
      startcol = (wideness(line->data, xtop) - from_col);
      if (startcol < 0) {
        startcol = 0;
      }
      thetext = (data + actual_x(data, startcol));
      /* If the end mark is onscreen, compute the number of columns we will mark. */
      if (xbot < till_x) {
        endcol   = (wideness(line->data, xbot) - from_col);
        paintlen = actual_x(thetext, (endcol - startcol));
      }
      if (paintlen == -1) {
        paintlen = STRLEN(thetext);
      }
      font_row_top_bot(textfont, (line->lineno - editor->openfile->edittop->lineno),     &pix_top, NULL);
      font_row_top_bot(textfont, (line->lineno - editor->openfile->edittop->lineno + 1), &pix_bot, NULL);
      /* Position */
      x = (font_wideness(textfont, line->data, startcol) + editor->text->x);
      y = (pix_top + editor->text->y);
      /* Size */
      width  = font_wideness(textfont, thetext, paintlen);
      height = (pix_bot - pix_top);
      /* Create the rect vertex entries, then add then to the marked region buffer. */
      shader_rect_vertex_load(vert, x, y, width, height, PACKED_UINT_FLOAT(.2F, .2F, .5F, .45F));
      vertex_buffer_push_back(editor->marked_region_buf, vert, 4, ARRAY__LEN(RECT_INDICES));
    }
  }
}

/* ----------------------------- Editor text line ----------------------------- */

/* TODO: Change the name of this later. */
void editor_text_line(Editor *const editor, linestruct *const line) {
  ASSERT_EDITOR(editor);
  ASSERT(line);
  float x;
  float y;
  char *data;
  Ulong from_col;
  if (refresh_needed) {
    if (ISSET(LINE_NUMBERS)) {
      x = editor->gutter->x;
      y = (font_row_baseline(textfont, (line->lineno - editor->openfile->edittop->lineno)) + editor->gutter->y);
      data = fmtstr("%*lu ", (editor->margin - 1), line->lineno);
      font_vertbuf_add_mbstr(textfont, editor->buffer, data, editor->margin, NULL, PACKED_UINT_WHITE, &x, &y);
      free(data);
    }
    /* If the line has any text on it. */
    if (*line->data) {
      from_col = get_page_start(
        wideness(line->data, ((line == editor->openfile->current) ? editor->openfile->current_x : 0)),
        editor->cols
      );
      data = display_string(line->data, from_col, editor->cols, TRUE, FALSE);
      x = editor->text->x;
      y = (font_row_baseline(textfont, (line->lineno - editor->openfile->edittop->lineno)) + editor->text->y);
      font_vertbuf_add_mbstr(textfont, editor->buffer, data, STRLEN(data), NULL, PACKED_UINT_WHITE, &x, &y);
      editor_text_line_marked_region(editor, line, data, from_col);
      free(data);
    }
  }
}

/* ----------------------------- Editor draw ----------------------------- */

void editor_draw(Editor *const editor) {
  ASSERT_EDITOR(editor);
  int row = 0;
  linestruct *line;
  if (editor->hidden) {
    return;
  }
  line = editor->openfile->edittop;
  /* Draw the editor elements. */
  element_draw(editor->gutter);
  element_draw(editor->text);
  if (refresh_needed) {
    vertex_buffer_clear(editor->buffer);
    vertex_buffer_clear(editor->marked_region_buf);
    while (line && row++<editor->rows) {
      editor_text_line(editor, line);
      DLIST_ADV_NEXT(line);
    }
    /* If the prompt-menu is not active, and the current line is on screen, then add the cursor. */
    if (!promptmenu_active() && editor->openfile->current->lineno >= editor->openfile->edittop->lineno
    && editor->openfile->current->lineno < (editor->openfile->edittop->lineno + editor->rows))
    {
      font_add_cursor(
        textfont,
        editor->buffer,
        (editor->openfile->current->lineno - editor->openfile->edittop->lineno),
        PACKED_UINT_WHITE,
        editor_cursor_x_pos(editor, editor->openfile->current, editor->openfile->current_x),
        editor->text->y
      );
    }
  }
  vertex_buffer_render(editor->marked_region_buf, GL_TRIANGLES);
  render_vertbuf(textfont, editor->buffer);
  /* Draw the top-bar of the editor. */
  etb_draw(editor->tb);
  scrollbar_draw(editor->sb);
}

/* ----------------------------- editor_number_of_open_files ----------------------------- */

Ulong editor_number_of_open_files(Editor *editor) {
  Ulong ret = 0;
  openfilestruct *start = editor->startfile;
  openfilestruct *file  = start;
  if (file) {
    do {
      ++ret;
      CLIST_ADV_NEXT(file);
    } while (file != start);
  }
  return ret;
}
