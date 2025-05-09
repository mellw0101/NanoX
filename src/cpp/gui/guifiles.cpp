/** @file guifiles.cpp

  @author  Melwin Svensson.
  @date    27-1-25.

  This file is part of NanoX.

  Handeling file operations when in gui-mode, such as switching file in the current editor.

 */
#include "../../include/prototypes.h"


/* When using the gui, switch to the previous entry in the circular list of buffers. */
void gui_switch_to_prev_buffer(void) {
  /* If there is only one open buffer in this editor, there is nothing to update. */
  if (openeditor->openfile == openeditor->openfile->next) {
    show_statusmsg(AHEM, 2, "No more open file buffers");
    return;
  }
  openeditor->openfile = openfile->prev;
  /* After we have moved to the previous file in the editor, set
   * the global file pointer to match the currently open editor. */
  openfile = openeditor->openfile;
  gui_editor_redecorate(openeditor);
}

/* When using the gui, switch to the next entry in the circular list of buffers. */
void gui_switch_to_next_buffer(void) {
  /* If there is only one open buffer in this editor, there is nothing to update. */
  if (openeditor->openfile == openeditor->openfile->next) {
    show_statusmsg(AHEM, 2, "No more open file buffers");
    return;
  }
  openeditor->openfile = openeditor->openfile->next;
  /* After we have moved to the next file in the editor, set the
   * global file pointer to match the currently open editor. */
  openfile = openeditor->openfile;
  gui_editor_redecorate(openeditor);
}

/* Delete the lock file when in gui mode, this will show any error on the gui.  Returns `TRUE` on success, and `FALSE` otherwise. */
bool gui_delete_lockfile(const char *lockfile) {
  ASSERT(lockfile);
  if (unlink(lockfile) < 0 && errno != ENOENT) {
    show_statusmsg(MILD, 2, "Error deleting lock file %s: %s", lockfile, strerror(errno));
    return FALSE;
  }
  return TRUE;
}

/* Close the active buffer when using gui. */
void gui_close_buffer(void) {
  close_buffer();
  openeditor->openfile  = openfile;
  openeditor->startfile = startfile;
}

/* Return's `TRUE` if we should terminate the gui, otherwise, return's `FALSE` when we switched to the next file instead. */
bool gui_close_and_go(void) {
  /* If the open file has a lock file, delete it first. */
  if (openeditor->openfile->lock_filename) {
    gui_delete_lockfile(openeditor->openfile->lock_filename);
  }
  /* When there is more then one file open in the open editor. */
  if (openeditor->openfile != openeditor->openfile->next) {
    gui_close_buffer();
    gui_editor_redecorate(openeditor);
    return FALSE;
  }
  else {
    /* If this is the only editor.  We exit. */
    if (openeditor == openeditor->next) {
      return TRUE;
    }
    else {
      gui_editor_close();
      gui_editor_hide(openeditor, FALSE);
      gui_editor_redecorate(openeditor);
      gui_editor_resize(openeditor);
      return FALSE;
    }
  }
}
