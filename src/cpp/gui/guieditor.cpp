/** @file guieditor.cpp

  @author  Melwin Svensson.
  @date    29-1-2025.

  This file is a part of NanoX.

  This file handles everything that has to do with guieditor.
  A guieditor is NanoX's equvilent to a editor in vs-code.

 */
#include "../../include/prototypes.h"


/* Create the editor scrollbar. */
static void guieditor_scrollbar_create(guieditor *editor) {
  ASSERT(editor->text);
  editor->scrollbar = make_element_child(editor->text);
  set_element_editor_data(editor->scrollbar, editor);
  editor->scrollbar->flag.set<GUIELEMENT_ABOVE>();
  move_resize_element(editor->scrollbar, 10, 10);
  editor->scrollbar->flag.set<GUIELEMENT_REVERSE_RELATIVE_X_POS>();
  editor->scrollbar->relative_pos.x = editor->scrollbar->size.w;
  editor->scrollbar->flag.set<GUIELEMENT_RELATIVE_Y_POS>();
  editor->scrollbar->color = GUI_WHITE_COLOR;
}

/* Create the editor topbar. */
static void guieditor_topbar_create(guieditor *editor) {
  ASSERT(editor->main);
  ASSERT(gui);
  ASSERT(gui->uifont);
  /* Create the topbar element as a child to the main element. */
  editor->topbar = make_element_child(editor->main, FALSE);
  editor->topbar->color = EDIT_BACKGROUND_COLOR;
  move_resize_element(
    editor->topbar,
    vec2(editor->main->pos.x, editor->main->pos.y),
    vec2(editor->main->size.w, FONT_HEIGHT(gui->uifont))
  );
  /* Set relative positioning for the topbar. */
  editor->topbar->flag.set<GUIELEMENT_RELATIVE_POS>();
  editor->topbar->relative_pos = 0;
  /* Also set relative width for the topbar, so it always spans the width of the editor. */
  editor->topbar->flag.set<GUIELEMENT_RELATIVE_WIDTH>();
  editor->topbar->relative_size = 0;
}

/* Remove the existing buffer name buttons and create new ones based on the currently open files of `editor`. */
void refresh_editor_topbar(guieditor *editor) {
  /* When debugging is enabled, assert everything we use. */
  ASSERT(editor->topbar);
  ASSERT(gui);
  ASSERT(gui->uifont);
  /* Start at the topbar position. */
  vec2 pos = editor->topbar->pos;
  guielement *button;
  /* First remove all existing children of the topbar. */
  delete_guielement_children(editor->topbar);
  /* If there are any open files, create buttons for them. */
  ITER_OVER_ALL_OPENFILES(editor->startfile, file,
    button = make_element_child(editor->topbar);
    if (*file->filename) {
      move_resize_element(button, pos, vec2((pixbreadth(gui->uifont, file->filename) + pixbreadth(gui->uifont, "  ")), editor->topbar->size.h));
      set_element_lable(button, file->filename);
    }
    else {
      move_resize_element(button, pos, vec2(pixbreadth(gui->uifont, " Nameless "), editor->topbar->size.h));
      set_element_lable(button, "Nameless");
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
      set_element_borders(button, 2, GUI_BLACK_COLOR);
    }
    /* Otherwise, when this is the first file, make the right border half size. */
    else if (file == editor->startfile) {
      set_element_borders(button, vec4(2, 1, 2, 2), GUI_BLACK_COLOR);
    }
    /* When this is the last file. */
    else if (file->next == editor->startfile) {
      set_element_borders(button, vec4(1, 2, 2, 2), GUI_BLACK_COLOR);
    }
    /* Else, if this is a file in the middle or the last file, make both the left and right border half size. */
    else {
      set_element_borders(button, vec4(1, 1, 2, 2), GUI_BLACK_COLOR);
    }
    pos.x += button->size.w;
    set_element_file_data(button, file);
  );
}

/* Set all elements color in topbar, setting the active one to the active color. */
void update_editor_topbar(guieditor *editor) {
  ASSERT(editor);
  ASSERT(editor->topbar);
  guielement *button;
  /* Iterate over every open file in the editor, if any. */
  for (Ulong i = 0; i < editor->topbar->children.size(); ++i) {
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

/* Update the `scroll-bar's` position and height.  */
void update_editor_scrollbar(guieditor *editor) {
  ASSERT(editor);
  ASSERT(editor->openfile->edittop);
  ASSERT(editor->openfile->filebot);
  ASSERT(editor->text);
  ASSERT(editor->scrollbar);
  float height, ypos;
  calculate_scrollbar(editor->text->size.h, editor->openfile->filetop->lineno, editor->openfile->filebot->lineno, editor->rows, editor->openfile->edittop->lineno, &height, &ypos);
  editor->scrollbar->relative_pos.y = ypos;
  /* If the height of the scrollbar is the entire size of the text element, then hide the scrollbar and return. */
  if (height == editor->text->size.h) {
    editor->scrollbar->flag.set<GUIELEMENT_HIDDEN>();
    return;
  }
  /* Otherwise, show the scrollbar. */
  else {
    editor->scrollbar->flag.unset<GUIELEMENT_HIDDEN>();
  }
  move_resize_element(
    editor->scrollbar,
    vec2(editor->scrollbar->pos.x, (editor->text->pos.y + ypos)),
    vec2(editor->scrollbar->size.w, height)
  );
}

/* Create a new editor. */
void make_new_editor(bool new_buffer) {
  guieditor *node = (guieditor *)nmalloc(sizeof(*node));
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
  openeditor->main = make_element(
    0,
    vec2(gui->width, (gui->height - gui->botbar->size.h)),
    0,
    EDIT_BACKGROUND_COLOR,
    FALSE
  );
  /* Create the topbar element for the editor. */
  guieditor_topbar_create(openeditor);
  /* Create the gutter element as a child to the main element. */
  openeditor->gutter = make_element_child(openeditor->main);
  openeditor->gutter->color = EDIT_BACKGROUND_COLOR;
  move_resize_element(
    openeditor->gutter,
    vec2(0.0f, (openeditor->main->pos.y + openeditor->topbar->size.h)),
    vec2(get_line_number_pixel_offset(openeditor->openfile->filetop, gui->font)/* FONT_WIDTH(gui->font) * margin + 1) */, (openeditor->main->size.h - openeditor->topbar->size.h))
  );
  /* Set relative positioning for the gutter, so it follows the editor. */
  openeditor->gutter->flag.set<GUIELEMENT_RELATIVE_POS>();
  openeditor->gutter->relative_pos = vec2(0, openeditor->topbar->size.h);
  /* Also set relative height for the gutter element so that it follows the editors height. */
  openeditor->gutter->flag.set<GUIELEMENT_RELATIVE_HEIGHT>();
  openeditor->gutter->relative_size = 0;
  /* Create the text element as a child to the main element. */
  openeditor->text = make_element_child(openeditor->main);
  openeditor->text->color = EDIT_BACKGROUND_COLOR;
  move_resize_element(
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
  set_element_editor_data(openeditor->main, openeditor);
  set_element_editor_data(openeditor->topbar, openeditor);
  set_element_editor_data(openeditor->gutter, openeditor);
  set_element_editor_data(openeditor->text, openeditor);
  openeditor->flag = bit_flag_t<GUIEDITOR_FLAGSIZE>();
  openeditor->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
  guieditor_scrollbar_create(openeditor);
  openeditor->flag.set<GUIEDITOR_SCROLLBAR_REFRESH_NEEDED>();
}

/* Delete the data of a editor. */
void delete_editor(guieditor *editor) {
  ASSERT(editor);
  if (editor->buffer) {
    vertex_buffer_delete(editor->buffer);
    editor->buffer = NULL;
  }
  delete_element(editor->gutter);
  delete_element(editor->text);
  free(editor);
}

/* Close the currently active editor. */
void close_editor(void) {
  guieditor *editor = openeditor;
  if (editor == starteditor) {
    starteditor = starteditor->next;
  }
  editor->prev->next = editor->next;
  editor->next->prev = editor->prev;
  openeditor = editor->prev;
  if (openeditor == editor) {
    openeditor  = NULL;
    starteditor = NULL;
  }
  else {
    openfile  = openeditor->openfile;
    startfile = openeditor->startfile;
  }
  delete_editor(editor);
}

/* Free a circular list of buffers. */
void free_editor_buffers(guieditor *editor) {
  /* Save the current open and start file. */
  openfilestruct *was_openfile  = openfile;
  openfilestruct *was_startfile = startfile;
  /* Then set the editors start and open file as the open and start file. */
  openfile  = editor->openfile;
  startfile = editor->startfile;
  /* Now close all buffers. */
  while (openfile) {
    close_buffer();
  }
  /* Then we restore the open and start file. */
  openfile  = was_openfile;
  startfile = was_startfile;
}

void hide_editor(guieditor *editor, bool hide) {
  // ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, editor);
  if (hide) {
    editor->flag.set<GUIEDITOR_HIDDEN>();
  }
  else {
    editor->flag.unset<GUIEDITOR_HIDDEN>();
  }
  set_element_flag_recurse(editor->main, hide, GUIELEMENT_HIDDEN);
}

/* Switch to the previous editor.  */
void switch_to_prev_editor(void) {
  if (openeditor == openeditor->next) {
    show_statusmsg(MILD, 2, "Only one editor open");
    return;
  }
  openeditor = openeditor->prev;
  openfile   = openeditor->openfile;
  startfile  = openeditor->startfile;
  hide_editor(openeditor->next, TRUE);
  hide_editor(openeditor, FALSE);
  gui_redecorate_after_switch();
}

/* Switch to the next editor. */
void switch_to_next_editor(void) {
  if (openeditor == openeditor->next) {
    show_statusmsg(MILD, 2, "Only one editor open");
    return;
  }
  openeditor = openeditor->next;
  openfile   = openeditor->openfile;
  startfile  = openeditor->startfile;
  hide_editor(openeditor->prev, TRUE);
  hide_editor(openeditor, FALSE);
  gui_redecorate_after_switch();
}

/* Set `openeditor` to editor, if its not already. */
void set_openeditor(guieditor *editor) {
  /* Return early if editor is already the open editor. */
  if (editor == openeditor) {
    return;
  }
  openfile   = editor->openfile;
  startfile  = editor->startfile;
  openeditor = editor;
  gui_redecorate_after_switch();
}

/* If the element `e` has any relation to an editor, return that editor. */
guieditor *get_element_editor(guielement *e) {
  guieditor *editor = starteditor;
  if (editor) {
    do {
      if (is_ancestor(e, editor->main)) {
        return editor;
      }
      editor = editor->next;
    } while (editor != starteditor);
  }
  return NULL;
}

/* Get the editor that `file` belongs to. */
guieditor *get_file_editor(openfilestruct *file) {
  ITER_OVER_ALL_OPENEDITORS(starteditor, editor, ITER_OVER_ALL_OPENFILES(editor->startfile, afile,
    if (afile == file) {
      return editor;
    }
  ););
  return NULL;
}

/* Calculate the amount of rows that fit in the text element of `editor` based on the current font. */
void guieditor_calculate_rows(guieditor *const editor) {
  ASSERT(editor);
  ASSERT(editor->text);
  Uint row = 0;
  float top, bot;
  while (TRUE) {
    row_top_bot_pixel(row, gui->font, &top, &bot);
    if (top > editor->text->size.h) {
      break;
    }
    ++row;
  }
  editor->rows = row;
}

/* Return the edittop line number based on the current position of the editor's scrollbar. */
long guieditor_lineno_from_scrollbar_pos(guieditor *const editor) {
  ASSERT(editor);
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->filetop);
  ASSERT(editor->openfile->filebot);
  ASSERT(editor->text);
  ASSERT(editor->scrollbar);
  return index_from_scrollbar_pos(
    editor->text->size.h,
    editor->openfile->filetop->lineno,
    editor->openfile->filebot->lineno,
    editor->rows,
    (editor->scrollbar->pos.y - editor->text->pos.y)
  );
}

/* Set the editor's currently openfile's edittop based on the current position of the scrollbar. */
void guieditor_set_edittop_from_scrollbar_pos(guieditor *const editor) {
  ASSERT(editor);
  ASSERT(editor->openfile);
  ASSERT(editor->openfile->edittop);
  editor->openfile->edittop = gui_line_from_number(editor, guieditor_lineno_from_scrollbar_pos(editor));
}
