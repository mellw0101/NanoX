/** @file guieditor.cpp

  @author  Melwin Svensson.
  @date    29-1-2025.

  This file is a part of NanoX.

  This file handles everything that has to do with guieditor.
  A guieditor is NanoX's equvilent to a editor in vs-code.

 */
#include "../../include/prototypes.h"


/* ---------------------------------------------------------- Gui editor scrollbar ---------------------------------------------------------- */


/* The editor's routine to fetch the data the scrollbar need's. */
static void guieditor_scrollbar_update_routine(void *arg, float *total_length, Uint *start, Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset) {
  ASSERT(arg);
  guieditor *editor = (guieditor *)arg;
  ASSIGN_IF_VALID(total_length, editor->text->size.h);
  ASSIGN_IF_VALID(start, editor->openfile->filetop->lineno);
  ASSIGN_IF_VALID(total, editor->openfile->filebot->lineno);
  ASSIGN_IF_VALID(visible, editor->rows);
  ASSIGN_IF_VALID(current, editor->openfile->edittop->lineno);
  ASSIGN_IF_VALID(top_offset, 0);
  ASSIGN_IF_VALID(right_offset, 0);
}

/* The editor's routine to respond to a calculated index based on the scrollbars's current position. */
static void guieditor_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  guieditor *editor = (guieditor *)arg;
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->edittop);
  editor->openfile->edittop = gui_line_from_number(editor, index);
}

/* Create the editor scrollbar. */
static void guieditor_scrollbar_create(guieditor *const editor) {
  ASSERT(editor);
  ASSERT(editor->text);
  editor->sb = guiscrollbar_create(editor->text, editor, guieditor_scrollbar_update_routine, guieditor_scrollbar_moving_routine);
}


/* ---------------------------------------------------------- Gui editor topbar ---------------------------------------------------------- */


/* Create the editor topbar. */
static void guieditor_topbar_create(guieditor *const editor) {
  ASSERT(editor->main);
  ASSERT(gui);
  ASSERT(gui->uifont);
  /* Create the topbar element as a child to the main element. */
  editor->topbar = guielement_create(editor->main, FALSE);
  editor->topbar->color = EDIT_BACKGROUND_COLOR;
  guielement_move_resize(
    editor->topbar,
    vec2(editor->main->pos.x, editor->main->pos.y),
    vec2(editor->main->size.w, gui_font_height(gui->uifont) /* FONT_HEIGHT(gui->uifont) */)
  );
  /* Set relative positioning for the topbar. */
  editor->topbar->flag.set<GUIELEMENT_RELATIVE_POS>();
  editor->topbar->relative_pos = 0;
  /* Also set relative width for the topbar, so it always spans the width of the editor. */
  editor->topbar->flag.set<GUIELEMENT_RELATIVE_WIDTH>();
  editor->topbar->relative_size = 0;
}

/* Remove the existing buffer name buttons and create new ones based on the currently open files of `editor`. */
void guieditor_refresh_topbar(guieditor *const editor) {
  /* When debugging is enabled, assert everything we use. */
  ASSERT(editor->topbar);
  ASSERT(gui);
  ASSERT(gui->uifont);
  /* Start at the topbar position. */
  vec2 pos = editor->topbar->pos;
  guielement *button;
  /* First remove all existing children of the topbar. */
  guielement_delete_children(editor->topbar);
  /* If there are any open files, create buttons for them. */
  CLIST_ITER(editor->startfile, file,
    button = guielement_create(editor->topbar);
    button->cursor_type = GLFW_HAND_CURSOR;
    if (*file->filename) {
      guielement_move_resize(button, pos, vec2((pixbreadth(gui_font_get_font(gui->uifont), file->filename) + pixbreadth(gui_font_get_font(gui->uifont), "  ")), editor->topbar->size.h));
      guielement_set_lable(button, file->filename);
    }
    else {
      guielement_move_resize(button, pos, vec2(pixbreadth(gui_font_get_font(gui->uifont), " Nameless "), editor->topbar->size.h));
      guielement_set_lable(button, "Nameless");
    }
    /* Set a diffrent color on the button that holds the editor openfile. */
    if (file == editor->openfile) {
      button->color = EDITOR_TOPBAR_BUTTON_ACTIVE_COLOR;
    }
    /* Otherwise, just set the inactive color. */
    else {
      button->color = EDITOR_TOPBAR_BUTTON_INACTIVE_COLOR;
    }
    button->textcolor = GUI_WHITE_COLOR;
    /* Make the element use relative positioning so that when we move the topbar the buttons follows. */
    button->flag.set<GUIELEMENT_RELATIVE_POS>();
    button->relative_pos = (button->pos - editor->topbar->pos);
    /* When there is only a single file open make all borders equal. */
    if (file == file->next) {
      guielement_set_borders(button, 1, /* GUI_WHITE_COLOR */ vec4(vec3(0.5), 1));
    }
    /* Otherwise, when this is the first file, make the right border half size. */
    else if (file == editor->startfile) {
      guielement_set_borders(button, vec4(1, 0, 1, 1), vec4(vec3(0.5), 1));
    }
    /* When this is the last file. */
    else if (file->next == editor->startfile) {
      guielement_set_borders(button, vec4(1, 1, 1, 1), vec4(vec3(0.5), 1));
    }
    /* Else, if this is a file in the middle or the last file, make both the left and right border half size. */
    else {
      guielement_set_borders(button, vec4(1, 0, 1, 1), vec4(vec3(0.5), 1));
    }
    pos.x += button->size.w;
    guielement_set_file_data(button, file);
  );
}

/* Set all elements color in topbar, setting the active one to the active color. */
void guieditor_update_active_topbar(guieditor *editor) {
  ASSERT(editor);
  ASSERT(editor->topbar);
  guielement *button;
  /* Iterate over every open file in the editor, if any. */
  for (Ulong i=0; i<editor->topbar->children.size(); ++i) {
    button = editor->topbar->children[i];
    if (guielement_has_file_data(button)) {
      if (button->data.file == editor->openfile) {
        button->color = EDITOR_TOPBAR_BUTTON_ACTIVE_COLOR;
      }
      else {
        button->color = EDITOR_TOPBAR_BUTTON_INACTIVE_COLOR;
      }
    }
  }
}

/* Create a new editor. */
void make_new_editor(bool new_buffer) {
  guieditor *node;
  MALLOC_STRUCT(node);
  /* If this is the first editor. */
  if (!openeditor) {
    /* Make the first editor the only element in the list. */
    node->prev  = node;
    node->next  = node;
    starteditor = node;
    /* When creating the first editor use this openfile and startfile that was made when we started. */
    node->openfile  = openfile;
    node->startfile = startfile;
    openfile  = node->openfile;
    startfile = node->startfile;
  }
  /* Otherwise, if there is already an existing editor. */
  else {
    /* Add the new editor after the current one in the list. */
    node->prev             = openeditor;
    node->next             = openeditor->next;
    openeditor->next->prev = node;
    openeditor->next       = node;
    if (new_buffer) {
      openfile  = NULL;
      startfile = NULL;
      make_new_buffer();
    }
    node->openfile  = openfile;
    node->startfile = startfile;
  }
  openeditor           = node;
  openeditor->buffer   = make_new_font_buffer();
  openeditor->topbuf   = make_new_font_buffer();
  openeditor->openfile = openfile;
  openeditor->pen      = 0;
  openeditor->rows     = 0;
  /* Create the main editor element. */
  openeditor->main = guielement_create(
    0.0f,
    vec2(gui->width, (gui->height - gui->botbar->size.h)),
    0.0f,
    EDIT_BACKGROUND_COLOR,
    FALSE
  );
  /* Create the topbar element for the editor. */
  guieditor_topbar_create(openeditor);
  /* Create the gutter element as a child to the main element. */
  openeditor->gutter = guielement_create(openeditor->main);
  openeditor->gutter->color = EDIT_BACKGROUND_COLOR;
  guielement_move_resize(
    openeditor->gutter,
    vec2(0.0f, (openeditor->main->pos.y + openeditor->topbar->size.h)),
    vec2(get_line_number_pixel_offset(openeditor->openfile->filetop, gui_font_get_font(gui->font)), (openeditor->main->size.h - openeditor->topbar->size.h))
  );
  /* Set relative positioning for the gutter, so it follows the editor. */
  openeditor->gutter->flag.set<GUIELEMENT_RELATIVE_POS>();
  openeditor->gutter->relative_pos = vec2(0, openeditor->topbar->size.h);
  /* Also set relative height for the gutter element so that it follows the editors height. */
  openeditor->gutter->flag.set<GUIELEMENT_RELATIVE_HEIGHT>();
  openeditor->gutter->relative_size = 0;
  /* Create the text element as a child to the main element. */
  openeditor->text = guielement_create(openeditor->main);
  openeditor->text->color = EDIT_BACKGROUND_COLOR;
  guielement_move_resize(
    openeditor->text,
    vec2((openeditor->main->pos.x + openeditor->gutter->size.w), (openeditor->main->pos.y + openeditor->topbar->size.h)),
    vec2((openeditor->main->size.w - openeditor->gutter->size.w), (openeditor->main->size.h - openeditor->topbar->size.h))
  );
  /* Set relative positioning for the text elemenent, so it follows the editor. */
  openeditor->text->flag.set<GUIELEMENT_RELATIVE_POS>();
  openeditor->text->relative_pos = vec2(openeditor->gutter->size.w, openeditor->topbar->size.h);
  /* Also set relative width and height for the text element so that it follows the size of the editor. */
  openeditor->text->flag.set<GUIELEMENT_RELATIVE_WIDTH>();
  openeditor->text->flag.set<GUIELEMENT_RELATIVE_HEIGHT>();
  openeditor->text->relative_size = 0;
  openeditor->text->cursor_type = GLFW_IBEAM_CURSOR;
  /* Set the editor data ptr of all elements as this editor. */
  guielement_set_editor_data(openeditor->main, openeditor);
  guielement_set_editor_data(openeditor->topbar, openeditor);
  guielement_set_editor_data(openeditor->gutter, openeditor);
  guielement_set_editor_data(openeditor->text, openeditor);
  openeditor->flag = bit_flag_t<GUIEDITOR_FLAGSIZE>();
  openeditor->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
  guieditor_scrollbar_create(openeditor);
}

/* Delete the data of a editor. */
void guieditor_free(guieditor *const editor) {
  ASSERT(editor);
  if (editor->buffer) {
    vertex_buffer_delete(editor->buffer);
    editor->buffer = NULL;
  }
  guielement_free(editor->main);
  free(editor->sb);
  free(editor);
}

/* Close the currently active editor. */
void guieditor_close(void) {
  guieditor *editor = openeditor;
  if (editor == starteditor) {
    starteditor = starteditor->next;
  }
  CLIST_UNLINK(editor);
  openeditor = editor->prev;
  if (openeditor == editor) {
    openeditor  = NULL;
    starteditor = NULL;
  }
  else {
    openfile  = openeditor->openfile;
    startfile = openeditor->startfile;
  }
  guieditor_free(editor);
}

void guieditor_hide(guieditor *const editor, bool hide) {
  ASSERT(editor);
  if (hide) {
    editor->flag.set<GUIEDITOR_HIDDEN>();
  }
  else {
    editor->flag.unset<GUIEDITOR_HIDDEN>();
  }
  guielement_set_flag_recurse(editor->main, hide, GUIELEMENT_HIDDEN);
}

/* Switch to the previous editor.  */
void guieditor_switch_to_prev(void) {
  ASSERT(openeditor);
  /* If there is only one editor open, just print a message and return. */
  if (CLIST_SINGLE(openeditor)) {
    show_statusmsg(MILD, 2, "Only one editor open");
    return;
  }
  CLIST_ADV_PREV(openeditor);
  openfile  = openeditor->openfile;
  startfile = openeditor->startfile;
  guieditor_hide(openeditor->next, TRUE);
  guieditor_hide(openeditor, FALSE);
  guieditor_redecorate(openeditor);
  guieditor_resize(openeditor);
  editwinrows = openeditor->rows;
}

/* Switch to the next editor. */
void guieditor_switch_to_next(void) {
  ASSERT(openeditor);
  /* When there is only a single open editor, just tell the user and return. */
  if (CLIST_SINGLE(openeditor)) {
    show_statusmsg(MILD, 2, "Only one editor open");
    return;
  }
  CLIST_ADV_NEXT(openeditor);
  openfile  = openeditor->openfile;
  startfile = openeditor->startfile;
  guieditor_hide(openeditor->prev, TRUE);
  guieditor_hide(openeditor, FALSE);
  guieditor_redecorate(openeditor);
  guieditor_resize(openeditor);
  editwinrows = openeditor->rows;
}

/* Within the currently open editor, switch to the prev buffer. */
void guieditor_switch_openfile_to_prev(void) {
  ASSERT(openeditor);
  ASSERT(openeditor->openfile);
  /* If there is only one open buffer in the currently open editor, there is nothing to do. */
  if (CLIST_SINGLE(openeditor->openfile)) {
    show_statusmsg(AHEM, 2, "No more open file buffers in the current editor");
    return;
  }
  CLIST_ADV_PREV(openeditor->openfile);
  openfile = openeditor->openfile;
  guieditor_redecorate(openeditor);
  guieditor_resize(openeditor);
}

/* Within the currently open editor, switch to the prev buffer. */
void guieditor_switch_openfile_to_next(void) {
  ASSERT(openeditor);
  ASSERT(openeditor->openfile);
  /* If there is only one open file in the currently active editor, print a msg telling the user and return. */
  if (CLIST_SINGLE(openeditor->openfile)) {
    show_statusmsg(AHEM, 2, "No more open file buffers in the current editor");
    return;
  }
  CLIST_ADV_NEXT(openeditor->openfile);
  openfile = openeditor->openfile;
  guieditor_redecorate(openeditor);
  guieditor_resize(openeditor);
}

/* Set `openeditor` to editor, if its not already. */
void guieditor_set_open(guieditor *const editor) {
  ASSERT(openeditor);
  ASSERT(editor);
  /* Return early if editor is already the open editor. */
  if (editor == openeditor) {
    return;
  }
  /* Ensure the global ptr's to the openfile and startfile are set as the new open editor. */
  openfile   = editor->openfile;
  startfile  = editor->startfile;
  openeditor = editor;
  guieditor_redecorate(editor);
  guieditor_resize(editor);
}

/* If the element `e` has any relation to an editor, return that editor. */
guieditor *guieditor_from_element(guielement *e) {
  if (guielement_has_editor_data(e)) {
    return e->data.editor;
  }
  else if (guielement_has_editor_data(e->parent)) {
    return e->parent->data.editor;
  }
  return NULL;
}

/* Get the editor that `file` belongs to. */
guieditor *guieditor_from_file(openfilestruct *file) {
  ASSERT(file);
  ITER_OVER_ALL_OPENEDITORS(starteditor, editor, ITER_OVER_ALL_OPENFILES(editor->startfile, afile,
    if (afile == file) {
      return editor;
    }
  ););
  ALWAYS_ASSERT_MSG(0, "This should never happen, every openfile must be linked to a editor.");
  return NULL;
}

/* Calculate the amount of rows that fit in the text element of `editor` based on the current font. */
void guieditor_calculate_rows(guieditor *const editor) {
  ASSERT(editor);
  ASSERT(editor->text);
  Uint row = 0;
  float top, bot;
  while (TRUE) {
    // row_top_bot_pixel(row, gui_font_get_font(gui->font), &top, &bot);
    gui_font_row_top_bot(gui->font, row, &top, &bot);
    if (top > editor->text->size.h) {
      break;
    }
    ++row;
  }
  editor->rows = row;
  if (editor == openeditor) {
    editwinrows = (editor->rows - 1);
  }
}

/* Determen the size of the gutter based on the total size of the last line number. */
static void guieditor_set_gutter_width(guieditor *const editor) {
  ASSERT(editor);
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->filebot);
  char *linenostr;
  confirm_margin();
  linenostr = fmtstr("%*lu ", (margin - 1), editor->openfile->filebot->lineno);
  editor->gutter->size.w = pixbreadth(gui_font_get_font(gui->font), linenostr);
  free(linenostr);
}

/* Resize `editor` to the size of the gui, this needs to be changed later when we add a grid to hold editors. */
void guieditor_resize(guieditor *const editor) {
  ASSERT(gui);
  ASSERT(gui->botbar);
  ASSERT(editor);
  ASSERT(editor->main);
  ASSERT(editor->text);
  ASSERT(editor->gutter);
  if (!ISSET(LINE_NUMBERS)) {
    editor->text->relative_pos.x = 0;
    editor->gutter->flag.set<GUIELEMENT_HIDDEN>();
  }
  else {
    guieditor_set_gutter_width(editor);
    editor->text->relative_pos.x = editor->gutter->size.w;
    editor->gutter->flag.unset<GUIELEMENT_HIDDEN>();
  }
  guielement_move_resize(editor->main, 0, vec2(gui->width, (gui->height - gui->botbar->size.h)));
  guieditor_calculate_rows(editor);
  guiscrollbar_refresh_needed(editor->sb);
}

/* Used to ensure the correct state of an editor after we have switched to a new one or changed the currently open file. */
void guieditor_redecorate(guieditor *const editor) {
  ASSERT(editor);
  ASSERT(editor->openfile);
  /* If there is a error with the currently opened file, show it in the statusbar and log it. */
  if (editor->openfile->errormessage) {
    show_statusmsg(ALERT, 2, editor->openfile->errormessage);
    logE("%s", editor->openfile->errormessage);
    free(editor->openfile->errormessage);
    editor->openfile->errormessage = NULL;
  }
  ensure_firstcolumn_is_aligned();
  currmenu       = MMOST;
  shift_held     = TRUE;
  refresh_needed = TRUE;
  editor->flag.set<GUIEDITOR_TOPBAR_UPDATE_ACTIVE>();
  editor->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
  guiscrollbar_refresh_needed(editor->sb);
}

/* Open a new empty `openfilestruct *` in the currently open editor. */
void guieditor_open_new_empty_buffer(void) {
  ASSERT(openeditor);
  ASSERT(openeditor->openfile);
  /* Make a new buffer related to the global pointer. */
  make_new_buffer();
  /* Set the open editor's currently open file to the global ptr. */
  openeditor->openfile = openfile;
  guieditor_redecorate(openeditor);
  guieditor_resize(openeditor);
}

/* Close the currently open editor's currently open buffer. */
void guieditor_close_open_buffer(void) {
  ASSERT(openeditor);
  ASSERT(openeditor->openfile);
  close_buffer();
  openeditor->openfile  = openfile;
  openeditor->startfile = startfile;
}

/* Open a new buffer in the currently open editor using `path`. */
void guieditor_open_buffer(const char *const restrict path) {
  ASSERT(openeditor);
  ASSERT(openeditor->openfile);
  ASSERT(path);
  openfilestruct *was_openfile = openfile;
  openfilestruct *new_openfile;
  /* In this case we should always terminate apon the file not existing as this function
   * should never be called in this case.  Note that this should be handeled before
   * the call to this function, because this function has one job, to open a file. */
  ALWAYS_ASSERT(file_exists(path));
  open_buffer(path, TRUE);
  /* If the buffer this was called from is empty, then the newly opened one should replace it. */
  if (!*was_openfile->filename && !was_openfile->totsize) {
    new_openfile = openfile;
    free_one_buffer(was_openfile, &openeditor->openfile, &openeditor->startfile);
    /* Make the new buffer the currently open one. */
    openfile = new_openfile;
  }
  openeditor->openfile = openfile;
  guieditor_redecorate(openeditor);
  guieditor_resize(openeditor);
}

void guieditor_update_all(void) {
  ASSERT(starteditor);
  ASSERT(openeditor);
  CLIST_ITER(starteditor, editor,
    guieditor_redecorate(editor);
    guieditor_resize(editor);
  );
}
