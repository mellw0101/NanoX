/** @file guieditor.cpp

  @author  Melwin Svensson.
  @date    29-1-2025.

  This file is a part of NanoX.

  This file handles everything that has to do with guieditor.
  A guieditor is NanoX's equvilent to a editor in vs-code.

 */
#include "../../include/prototypes.h"

/* The callback for the topbar buttons. */
static void editor_topbar_button_callback(guielement *self, guielement_callback_type type) {
  switch (type) {
    case GUIELEMENT_ENTER_CALLBACK: {
      self->color     = vec4(1);
      self->textcolor = vec4(0, 0, 0, 1);
      break;
    }
    case GUIELEMENT_LEAVE_CALLBACK: {
      self->color     = EDIT_BACKGROUND_COLOR;
      self->textcolor = vec4(1);
      if (strcmp(self->lable, openeditor->openfile->filename) == 0) {  
        self->color += vec3(0.1);
      }
      break;
    }
    case GUIELEMENT_CLICK_CALLBACK: {
      guieditor *editor = get_element_editor(self);
      openfilestruct *file = editor->startfile;
      do {
        if (strcmp(self->lable, file->filename) == 0) {
          gui_set_openfile(file);
          break;
        }
        file = file->next;
      } while (file != editor->startfile);
      break;
    }
  }
}

/* Remove the existing buffer name buttons and create new ones based on the currently open files of `editor`. */
void update_editor_topbar(guieditor *editor) {
  openfilestruct *file = editor->startfile;
  vec2 pos = editor->topbar->pos;
  guielement *button;
  /* First remove all existing children of the topbar. */
  delete_guielement_children(editor->topbar);
  /* If there are any open files, create buttons for them. */
  if (file) {
    do {
      button = make_element_child(editor->topbar);
      if (*file->filename) {
        char *display_str = fmtstr(" %s ", file->filename);
        move_resize_element(
          button,
          pos,
          vec2((pixel_breadth(editor->font, display_str)), editor->topbar->size.h)
        );
        free(display_str);
        set_element_lable(button, file->filename);
      }
      else {
        move_resize_element(button, pos, vec2(pixel_breadth(editor->font, "Nameless"), editor->topbar->size.h));
        set_element_lable(button, "Nameless");
      }
      button->color     = EDIT_BACKGROUND_COLOR;
      button->textcolor = vec4(1);
      /* Make the element use relative positioning so that when we move the topbar the buttons follows. */
      button->flag.set<GUIELEMENT_RELATIVE_POS>();
      button->relative_pos = (button->pos - editor->topbar->pos);
      /* When there is only a single file open make all borders equal. */
      if (file == file->next) {
        set_element_borders(button, 2, vec4(0, 0, 0, 1));
      }
      /* Otherwise, when this is the first file, make the right border half size. */
      else if (file == editor->startfile) {
        set_element_borders(button, vec4(2, 1, 2, 2), vec4(0, 0, 0, 1));
      }
      /* And when this is the last file, make the left border half size. */
      else if (file->next == editor->startfile) {
        set_element_borders(button, vec4(1, 2, 2, 2), vec4(0, 0, 0, 1));
      }
      /* Else, if this is a file in the middle, make both the left and right border half size. */
      else {
        set_element_borders(button, vec4(1, 1, 2, 2), vec4(0, 0, 0, 1));
      }
      pos.x += button->size.w;
      button->callback = editor_topbar_button_callback;
      file = file->next;
    } while (file != editor->startfile);
  }
  editor->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
  move_element(editor->topbar, editor->topbar->pos);
}

/* Create a new editor. */
void make_new_editor(bool new_buffer) {
  guieditor *newnode = (guieditor *)nmalloc(sizeof(*newnode));
  /* If this is the first editor. */
  if (!openeditor) {
    /* Make the first editor the only element in the list. */
    newnode->prev = newnode;
    newnode->next = newnode;
    starteditor   = newnode;
    /* When creating the first editor use this openfile and startfile that was made when we started. */
    newnode->openfile  = openfile;
    newnode->startfile = startfile;
  }
  /* Otherwise, if there is already an existing editor. */
  else {
    /* Add the new editor after the current one in the list. */
    newnode->prev          = openeditor;
    newnode->next          = openeditor->next;
    openeditor->next->prev = newnode;
    openeditor->next       = newnode;
    if (new_buffer) {
      openfile  = NULL;
      startfile = NULL;
      make_new_buffer();
    }
    newnode->openfile  = openfile;
    newnode->startfile = startfile;
  }
  openeditor           = newnode;
  openeditor->buffer   = vertex_buffer_new("vertex:3f,tex_coord:2f,color:4f");
  openeditor->topbuf   = vertex_buffer_new("vertex:3f,tex_coord:2f,color:4f");
  openeditor->openfile = openfile;
  openeditor->font     = gui->font;
  openeditor->next     = NULL;
  openeditor->prev     = NULL;
  openeditor->pen      = 0.0f;
  /* Create the main editor element. */
  openeditor->main = make_element(
    vec2(0, (gui->topbar->pos.x + gui->topbar->size.h)),
    vec2(gui->width, (gui->height - gui->topbar->size.h)),
    0,
    EDIT_BACKGROUND_COLOR,
    FALSE
  );
  /* Create the topbar element as a child to the main element. */
  openeditor->topbar = make_element_child(openeditor->main);
  openeditor->topbar->color = vec4(1);
  move_resize_element(
    openeditor->topbar,
    vec2(openeditor->main->pos.x, openeditor->main->pos.y),
    vec2(openeditor->main->size.w, FONT_HEIGHT(openeditor->font))
  );
  /* Create the gutter element as a child to the main element. */
  openeditor->gutter = make_element_child(openeditor->main);
  openeditor->gutter->color = EDIT_BACKGROUND_COLOR;
  move_resize_element(
    openeditor->gutter,
    vec2(0.0f, (openeditor->main->pos.y + openeditor->topbar->size.h)),
    vec2((FONT_WIDTH(openeditor->font) * margin + 1), (openeditor->main->size.h - openeditor->topbar->size.h))
  );
  /* Set relative positioning for the gutter, so it follows the editor. */
  openeditor->gutter->flag.set<GUIELEMENT_RELATIVE_POS>();
  openeditor->gutter->relative_pos = vec2(0, openeditor->topbar->size.h);
  /* Create the text element as a child to the main element. */
  openeditor->text = make_element_child(openeditor->main);
  openeditor->text->color = EDIT_BACKGROUND_COLOR;
  move_resize_element(
    openeditor->text,
    vec2((openeditor->main->pos.x + openeditor->gutter->size.w), (openeditor->main->pos.y + openeditor->topbar->size.h)),
    vec2((openeditor->main->size.w - openeditor->gutter->size.w), (openeditor->main->size.h - openeditor->topbar->size.h))
  );
  openeditor->flag = bit_flag_t<GUIEDITOR_FLAGSIZE>();
  update_editor_topbar(openeditor);
}

/* Delete the data of a editor. */
void delete_editor(guieditor *editor) {
  if (!editor) {
    return;
  }
  vertex_buffer_delete(editor->buffer);
  delete_element(editor->gutter);
  delete_element(editor->text);
  free(editor);
  editor = NULL;
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

/* If the element `e` has any relation to an editor, return that editor. */
guieditor *get_element_editor(guielement *e) {
  guieditor *editor = starteditor;
  do {
    if (is_ancestor(e, editor->main)) {
      return editor;
    }
    editor = editor->next;
  } while (editor != starteditor);
  return NULL;
}

/* Get the editor that `file` belongs to. */
guieditor *get_file_editor(openfilestruct *file) {
  guieditor *editor = starteditor;
  openfilestruct *check;
  /* Check all editors. */
  do {
    check = editor->startfile;
    /* Check all files in the editor, if any. */
    if (check) {
      do {
        if (check == file) {
          return editor;
        }
        check = check->next;
      } while (check != editor->startfile);
    }
    editor = editor->next;
  } while (editor != starteditor);
  die("%s: The passed file does not belong to any editor, something is very wrong.\n", __func__);
  return NULL;
}
