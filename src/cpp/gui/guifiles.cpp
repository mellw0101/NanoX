/** @file guifiles.cpp

  @author  Melwin Svensson.
  @date    27-1-25.

  This file is part of NanoX.

  Handeling file operations when in gui-mode, such as switching file in the current editor.

 */
#include "../../include/prototypes.h"

/* Ensure correctness when switching buffers inside when using the gui, also display any errors from the buffer. */
void gui_redecorate_after_switch(void) {
  /* When debugging is enabled, assert everything we will use. */
  ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, openeditor);
  ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, openeditor->openfile);
  /* If there is a error in the open file show it on the statusbar, and log it. */
  if (openeditor->openfile->errormessage) {
    show_statusmsg(ALERT, 2, openeditor->openfile->errormessage);
    logE("%s", openeditor->openfile->errormessage);
    free(openeditor->openfile->errormessage);
    openeditor->openfile->errormessage = NULL;
  }
  ensure_firstcolumn_is_aligned();
  currmenu       = MMOST;
  shift_held     = TRUE;
  refresh_needed = TRUE;
  openeditor->flag.set<GUIEDITOR_TOPBAR_UPDATE_ACTIVE>();
}

/* When using the gui, switch to the previous entry in the circular list of buffers. */
void gui_switch_to_prev_buffer(void) {
  ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, openeditor);
  ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, openeditor->openfile);
  /* If there is only one open buffer in this editor, there is nothing to update. */
  if (openeditor->openfile == openeditor->openfile->next) {
    show_statusmsg(AHEM, 2, "No more open file buffers");
    return;
  }
  openeditor->openfile = openfile->prev;
  /* After we have moved to the previous file in the editor, set
   * the global file pointer to match the currently open editor. */
  openfile = openeditor->openfile;
  gui_redecorate_after_switch();
}

/* When using the gui, switch to the next entry in the circular list of buffers. */
void gui_switch_to_next_buffer(void) {
  ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, openeditor);
  ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, openeditor->openfile);
  /* If there is only one open buffer in this editor, there is nothing to update. */
  if (openeditor->openfile == openeditor->openfile->next) {
    show_statusmsg(AHEM, 2, "No more open file buffers");
    return;
  }
  openeditor->openfile = openeditor->openfile->next;
  /* After we have moved to the next file in the editor, set the
   * global file pointer to match the currently open editor. */
  openfile = openeditor->openfile;
  gui_redecorate_after_switch();
}

/* Set openfile as `file`. */
void gui_set_openfile(openfilestruct *file) {
  ASSERT(file);
  openfile = file;
  openeditor = get_file_editor(file);
  openeditor->openfile = file;
  gui_redecorate_after_switch();
  ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, openeditor);
  ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, openeditor->openfile);
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
  ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, openeditor);
  ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, openeditor->openfile);
  close_buffer();
  openeditor->openfile  = openfile;
  openeditor->startfile = startfile;
}

/* Return's `TRUE` if we should terminate the gui, otherwise, return's `FALSE` when we switched to the next file instead. */
bool gui_close_and_go(void) {
  ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, openeditor);
  ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, openeditor->openfile);
  /* If the open file has a lock file, delete it first. */
  if (openeditor->openfile->lock_filename) {
    gui_delete_lockfile(openeditor->openfile->lock_filename);
  }
  /* When there is more then one file open in the open editor. */
  if (openeditor->openfile != openeditor->openfile->next) {
    gui_close_buffer();
    gui_switch_to_next_buffer();
    openeditor->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
    return FALSE;
  }
  else {
    /* If this is the only editor.  We exit. */
    if (openeditor == openeditor->next) {
      return TRUE;
    }
    else {
      close_editor();
      hide_editor(openeditor, FALSE);
      gui_redecorate_after_switch();
      return FALSE;
    }
  }
}

/* Open a new empty buffer. */
void gui_open_new_empty_buffer(void) {
  ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, openeditor);
  ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, openeditor->openfile);
  make_new_buffer();
  /* After the new buffer has been created, ensure correctness in
   * the open editor, as the startfile might have changed as well. */
  openeditor->openfile  = openfile;
  openeditor->startfile = startfile;
  gui_redecorate_after_switch();
  openeditor->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
}


