/** @file cut.c

  @author  Melwin Svensson.
  @date    4-6-2025.

 */
#include "../include/c_proto.h"


/* Delete the character at the current position, and add or update an undo item for the given action. */
void expunge_for(openfilestruct *const file, int cols, undo_type action) {
  ASSERT(file);
  int charlen;
  Ulong line_len;
  Ulong old_amount;
  linestruct *joining;
  set_pww_for(file);
  /* When in the middle of a line, delete the current character. */
  if (file->current->data[file->current_x]) {
    charlen    = char_length(file->current->data + file->current_x);
    line_len   = strlen(file->current->data + file->current_x);
    old_amount = (ISSET(SOFTWRAP) ? extra_chunks_in(file->current, cols) : 0);
    /* If the type of action changed or the cursor moved to a different line, create a new undo item, otherwise update the existing item. */
    if (action != file->last_action || file->current->lineno != file->current_undo->head_lineno) {
      add_undo_for(file, action, NULL);
    }
    else {
      update_undo_for(file, action);
    }
    /* Move the remainder of the line "in", over the current character. */
    memmove(&file->current->data[file->current_x], &file->current->data[file->current_x + charlen], (line_len - charlen + 1));
    /* When softwrapping, a changed number of chunks requires a refresh. */
    if (ISSET(SOFTWRAP) && extra_chunks_in(file->current, cols) != old_amount) {
      refresh_needed = TRUE;
    }
    /* Adjust the mark if it is after the cursor on the current line. */
    if (file->mark == file->current && file->mark_x > file->current_x) {
      file->mark_x -= charlen;
    }
  }
  /* Otherwise, when not at end of buffer, join this line with the next. */
  else if (file->current != file->filebot) {
    joining = file->current->next;
    /* If there is a magic line, and we're before it: don't eat it. */
    if (joining == file->filebot && file->current_x && !ISSET(NO_NEWLINES)) {
      if (action == BACK) {
        add_undo_for(file, BACK, NULL);
      }
      return;
    }
    add_undo_for(file, action, NULL);
    /* Adjust the mark if it is on the line that will be "eaten". */
    if (file->mark == joining) {
      file->mark    = file->current;
      file->mark_x += file->current_x;
    }
    file->current->has_anchor |= joining->has_anchor;
    /* Add the content of the next line to that of the current one. */
    file->current->data = xstrcat(file->current->data, joining->data);
    unlink_node_for(file, joining);
    /* Two lines were joined, so do a renumbering and refresh the screen. */
    renumber_from(file->current);
    refresh_needed = TRUE;
  }
  /* We're at the end-of-file: nothing to do. */
  else {
    return;
  }
  if (!refresh_needed) {
    check_the_multis_for(file, file->current);
  }
  if (!refresh_needed && !ISSET(USING_GUI)) {
    update_line_curses_for(file, file->current, file->current_x);
  }
  /* Adjust the file size, and remember it for a possible redo. */
  --file->totsize;
  file->current_undo->newsize = file->totsize;
  set_modified_for(file);
}

/* Delete the character at the current position, and add or update an undo item for the given action. */
void expunge(undo_type action) {
  if (IN_GUI_CONTEXT) {
    expunge_for(openeditor->openfile, openeditor->cols, action);
  }
  else {
    expunge_for(openfile, editwincols, action);
  }
}
