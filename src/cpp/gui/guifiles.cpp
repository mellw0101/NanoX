/** @file guifiles.cpp

  @author  Melwin Svensson.
  @date    27-1-25.

  This file is part of NanoX.

  Handeling file operations when in gui-mode, such as switching file in the current editor.

 */
#include "../../include/prototypes.h"

/* Ensure correctness when switching buffers inside when using the gui, also display any errors from the buffer. */
static void gui_redecorate_after_switch(void) {
  ensure_firstcolumn_is_aligned();
  currmenu       = MMOST;
  shift_held     = TRUE;
  refresh_needed = TRUE;
  if (openfile->errormessage) {
    show_statusmsg(ALERT, 2, openfile->errormessage);
    free(openfile->errormessage);
    openfile->errormessage = NULL;
  }
}

/* When using the gui, switch to the previous entry in the circular list of buffers. */
void gui_switch_to_prev_buffer(void) {
  /* If there is only one open buffer in this editor, there is nothing to update. */
  if (openfile == openfile->next) {
    show_statusmsg(AHEM, 2, "No more open file buffers");
    return;
  }
  openfile = openfile->prev;
  openeditor->openfile = openfile;
  gui_redecorate_after_switch();
}

/* When using the gui, switch to the next entry in the circular list of buffers. */
void gui_switch_to_next_buffer(void) {
  /* If there is only one open buffer in this editor, there is nothing to update. */
  if (openfile == openfile->next) {
    show_statusmsg(AHEM, 2, "No more open file buffers");
    return;
  }
  openfile = openfile->next;
  openeditor->openfile = openfile;
  gui_redecorate_after_switch();
}

/* Set openfile as `file`. */
void gui_set_openfile(openfilestruct *file) {
  openfile = file;
  openeditor = get_file_editor(file);
  openeditor->openfile = file;
  gui_redecorate_after_switch();
}

/* Delete the lock file when in gui mode, this will show any error on the gui.  Returns `TRUE` on success, and `FALSE` otherwise. */
bool gui_delete_lockfile(const char *lockfile) {
  if (unlink(lockfile) < 0 && errno != ENOENT) {
    show_statusmsg(MILD, 2, "Error deleting lock file %s: %s", lockfile, strerror(errno));
    return FALSE;
  }
  return TRUE;
}

/* Return's `TRUE` if we should terminate the gui, otherwise, return's `FALSE` when we switched to the next file instead. */
bool gui_close_and_go(void) {
  if (openeditor->openfile->lock_filename) {
    gui_delete_lockfile(openeditor->openfile->lock_filename);
  }
  /* When there is more then one file open in the open editor. */
  if (openeditor->openfile != openeditor->openfile->next) {
    gui_switch_to_next_buffer();
    free_one_buffer(openeditor->openfile->prev, &openeditor->startfile, &openeditor->openfile);
    update_editor_topbar(openeditor);
    return FALSE;
  }
  else {
    return TRUE;
  }
}


