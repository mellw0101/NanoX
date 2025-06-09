/** @file gui/editor/editor.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../../../include/c_proto.h"



/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static Editor *editor_create_internal(void) {
  Editor *editor = xmalloc(sizeof(*editor));
  /* Boolian flags. */
  editor->should_close = FALSE;
  editor->hidden       = FALSE;
  /* Vertex buffer. */
  editor->buffer = vertbuf_create();
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

/* The editor's routine to fetch the data the scrollbar need's. */
static void editor_scrollbar_update_routine(void *arg, float *total_length, Uint *start, Uint *end, Uint *visible, Uint *current, float *t_offset, float *r_offset) {
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

/* The editor's routine to respond to a calculated index based on the scrollbars's current position. */
static void editor_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  Editor *editor = arg;
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->edittop);
  editor->openfile->edittop = line_from_number_for(editor->openfile, index);
}

/* Create the editor scrollbar. */
static void editor_scrollbar_create(Editor *const editor) {
  ASSERT(editor);
  ASSERT(editor->text);
  editor->sb = scrollbar_create(editor->text, editor, editor_scrollbar_update_routine, editor_scrollbar_moving_routine);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


void editor_create(bool new_buffer) {
  Editor *node = editor_create_internal();
  if (!openeditor) {
    CLIST_INIT(node);
    starteditor     = node;
    node->openfile  = openfile;
    node->startfile = startfile;
    editor_set_rows_cols(node, gui_width, gui_height);
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
  openeditor->main = element_create(0, 0, gui_width, gui_height, FALSE);
  element_set_editor_data(openeditor->main, openeditor);
  /* Gutter element. */
  openeditor->gutter = element_create(openeditor->main->x, (openeditor->main->y + gui_font_height(uifont)), editor_get_gutter_width(openeditor), (openeditor->main->height - gui_font_height(uifont)), TRUE);
  element_set_parent(openeditor->gutter, openeditor->main);
  element_set_editor_data(openeditor->gutter, openeditor);
  openeditor->gutter->color               = PACKED_UINT_EDIT_BACKGROUND;
  openeditor->gutter->has_relative_pos    = TRUE;
  openeditor->gutter->has_relative_height = TRUE;
  openeditor->gutter->relative_y          = gui_font_height(uifont);
  /* Text element. */
  openeditor->text = element_create((openeditor->main->x + openeditor->gutter->width), (openeditor->main->y + gui_font_height(uifont)), (openeditor->main->width - openeditor->gutter->width), (openeditor->main->height - gui_font_height(uifont)), TRUE);
  element_set_parent(openeditor->text, openeditor->main);
  element_set_editor_data(openeditor->text, openeditor);
  openeditor->text->color               = PACKED_UINT_EDIT_BACKGROUND;
  openeditor->text->has_relative_pos    = TRUE;
  openeditor->text->has_relative_width  = TRUE;
  openeditor->text->has_relative_height = TRUE;
  openeditor->text->relative_x          = openeditor->gutter->width;
  openeditor->text->relative_y          = gui_font_height(uifont);
  openeditor->text->cursor              = GLFW_IBEAM_CURSOR;
  /* Create the text scrollbar. */
  editor_scrollbar_create(openeditor);
  /* Create the editor topbar. */
  openeditor->tb = etb_create(openeditor);
}

void editor_free(Editor *const editor) {
  /* Make this function `NO-OP`. */
  if (!editor) {
    return;
  }
  vertex_buffer_delete(editor->buffer);
  element_free(editor->main);
  etb_free(editor->tb);
  free(editor->sb);
  free(editor);
}

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

/* Calculate the row's and column's of a `editor` based on the size of it's text element. */
void editor_set_rows_cols(Editor *const editor, float width, float height) {
  ASSERT(editor);
  int rows;
  int cols;
  gui_font_rows_cols(textfont, width, height, &rows, &cols);
  editor->rows = rows;
  editor->cols = cols;
}

/* Get the editor that `file` belongs to. */
Editor *editor_from_file(openfilestruct *const file) {
  ASSERT(file);
  CLIST_ITER(starteditor, editor, CLIST_ITER(editor->startfile, afile,
    if (afile == file) {
      return editor;
    }
  ););
  ALWAYS_ASSERT_MSG(0, "This should never happen, every openfile must be linked to a editor.");
  return NULL;
}

void editor_hide(Editor *const editor, bool hide) {
  ASSERT(editor);
  editor->hidden         = hide;
  editor->main->hidden   = hide;
  editor->gutter->hidden = hide;
  editor->text->hidden   = hide;
  etb_show_context_menu(editor->tb, NULL, FALSE);
}

void editor_close(Editor *const editor) {
  ASSERT(editor);
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
    }
    else {
      openfile  = openeditor->openfile;
      startfile = openeditor->startfile;
      editor_hide(openeditor, FALSE);
    }
  }
  editor_free(editor);
}

void editor_resize(Editor *const editor) {
  ASSERT(editor);
  ASSERT(editor->text);
  editor_confirm_margin(editor);
  editor->gutter->width    = editor_get_gutter_width(editor);
  editor->text->relative_x = editor->gutter->width;
  editor->gutter->hidden   = !editor->gutter->width;
  element_move_resize(editor->main, 0, 0, gui_width, (gui_height - gui_font_height(uifont)));
  editor_set_rows_cols(editor, editor->text->width, editor->text->height);
  etb_text_refresh_needed(editor->tb);
  scrollbar_refresh_needed(editor->sb);
}

/* Used to ensure the correct state of an editor after we have switched to a new one or changed the currently open file. */
void editor_redecorate(Editor *const editor) {
  ASSERT_EDITOR(editor);
  /* If there is a error with the currently opened file, show it in the statusbar and log it. */
  if (editor->openfile->errormessage) {
    // show_statusmsg(ALERT, 2, editor->openfile->errormessage);
    // logE("%s", editor->openfile->errormessage);
    free(editor->openfile->errormessage);
    editor->openfile->errormessage = NULL;
  }
  ensure_firstcolumn_is_aligned_for(editor->openfile, editor->cols);
  currmenu       = MMOST;
  shift_held     = TRUE;
  refresh_needed = TRUE;
  scrollbar_refresh_needed(editor->sb);
}

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

/* Close the currently open editor's currently open buffer. */
void editor_close_open_buffer(void) {
  ASSERT_EDITOR(openeditor);
  // free_one_buffer(openeditor->openfile, &openeditor->openfile, &openeditor->startfile);
  close_buffer();
  // openeditor->openfile  = openfile;
  // openeditor->startfile = startfile;
  etb_entries_refresh_needed(openeditor->tb);
}

/* Open a new empty `openfilestruct *` in the currently open editor. */
void editor_open_new_empty_buffer(void) {
  ASSERT_EDITOR(openeditor);
  /* Make a new buffer for the currently open editor. */
  make_new_buffer();
  editor_redecorate(openeditor);
  editor_resize(openeditor);
  etb_entries_refresh_needed(openeditor->tb);
}

void editor_update_all(void) {
  ASSERT(starteditor);
  ASSERT(openeditor);
  CLIST_ITER(starteditor, editor,
    editor_redecorate(editor);
    editor_resize(editor);
  );
}

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

float editor_cursor_x_pos(Editor *const editor, linestruct *const line, Ulong index) {
  ASSERT(editor);
  ASSERT(line);
  Ulong from_col  = editor_get_page_start(openeditor, wideness(line->data, index));
  char *converted = display_string(line->data, from_col, editor->cols, TRUE, FALSE);
  // float ret = (string_pixel_offset(converted, NULL, (wideness(line->data, index) - from_col), gui_font_get_font(gui->font)) + editor->text->pos.x);
  float ret = (font_wideness(textfont, converted, (wideness(line->data, index) - from_col)) + editor->text->x);
  free(converted);
  return ret;
}

linestruct *editor_get_text_line(Editor *const editor, float y_pos) {
  ASSERT(editor);
  ASSERT(editor->text);
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->edittop);
  long row;
  gui_font_row_from_pos(textfont, editor->text->y, (editor->text->y + editor->text->height), y_pos, &row);
  return line_from_number_for(editor->openfile, lclamp((editor->openfile->edittop->lineno + row), editor->openfile->edittop->lineno, editor->openfile->filebot->lineno));
}

Ulong editor_get_text_index(Editor *const editor, linestruct *const line, float x_pos) {
  ASSERT(editor);
  ASSERT(line);
  return gui_font_index_from_pos(textfont, line->data, strlen(line->data), x_pos, editor->text->x);
}

void editor_get_text_line_index(Editor *const editor, float x_pos, float y_pos, linestruct **const outline, Ulong *const outindex) {
  ASSERT(editor);
  ASSERT(outline);
  ASSERT(outindex);
  (*outline)  = editor_get_text_line(editor, y_pos);
  (*outindex) = editor_get_text_index(editor, (*outline), x_pos);
}

/* Open a new buffer in the currently open editor using `path`. */
void editor_open_buffer(const char *const restrict path) {
  ASSERT_EDITOR(openeditor);
  ASSERT(path);
  openfilestruct *was_openfile = openeditor->openfile;
  openfilestruct *new_openfile;
  /* In this case we should always terminate apon the file not existing as this function
   * should never be called in this case.  Note that this should be handeled before
   * the call to this function, because this function has one job, to open a file. */
  ALWAYS_ASSERT(file_exists(path));
  openfile = openeditor->openfile;
  if (!open_buffer(path, TRUE)) {
    return;
  }
  /* If the buffer this was called from is empty, then the newly opened one should replace it. */
  if (!*was_openfile->filename && !was_openfile->totsize) {
    new_openfile = openfile;
    free_one_buffer(was_openfile, &openeditor->openfile, &openeditor->startfile);
    /* Make the new buffer the currently open one. */
    openfile = new_openfile;
  }
  openeditor->openfile = openfile;
  editor_redecorate(openeditor);
  editor_resize(openeditor);
  etb_entries_refresh_needed(openeditor->tb);
}

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
      openfile = editor->openfile;
      startfile = editor->startfile;
    }
    etb_entries_refresh_needed(editor->tb);
    editor_redecorate(editor);
    editor_resize(editor);
  }
  else {
    if (CLIST_SINGLE(editor)) {
      glfwSetWindowShouldClose(gui_window, TRUE);
    }
    else {
      editor->should_close = TRUE;
    }
  }
}
