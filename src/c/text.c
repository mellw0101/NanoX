/** @file text.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Toggle the mark for `file`. */
void do_mark_for(openfilestruct *const file) {
  ASSERT(file);
  if (!file->mark) {
    file->mark     = file->current;
    file->mark_x   = file->current_x;
    file->softmark = FALSE;
    statusbar_all(_("Mark Set"));
  }
  else {
    file->mark = NULL;
    statusbar_all(_("Mark Unset"));
    refresh_needed = TRUE;
  }
}

/* Toggle the mark for the currently open file. */
void do_mark(void) {
  if (ISSET(USING_GUI)) {
    do_mark_for(openeditor->openfile);
  }
  else {
    do_mark_for(openfile);
  }
}

/* Return's the length of whilespace until first non blank char in `string`. */
Ulong indentlen(const char *const restrict string) {
  ASSERT(string);
  const char *ptr = string;
  while (*ptr && is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return (ptr - string);
}

/* Discard `undo-items` that are newer then `thisitem` in `buffer`, or all if `thisitem` is `NULL`. */
void discard_until_in_buffer(openfilestruct *const buffer, const undostruct *const thisitem) {
  undostruct  *dropit = buffer->undotop;
  groupstruct *group, *next;
  while (dropit && dropit != thisitem) {
    buffer->undotop = dropit->next;
    free(dropit->strdata);
    free(dropit->cutbuffer);
    group = dropit->grouping;
    while (group) {
      next = group->next;
      free_chararray(group->indentations, (group->bottom_line - group->top_line + 1));
      free(group);
      group = next;
    }
    free(dropit);
    dropit = buffer->undotop;
  }
  /* Adjust the pointer to the top of the undo struct. */
  buffer->current_undo = (undostruct *)thisitem;
  /* Prevent a chain of edition actions from continuing. */
  buffer->last_action = OTHER;
}

/* Discard undo items that are newer than the given one, or all if NULL. */
void discard_until(const undostruct *thisitem) {
  discard_until_in_buffer(openfile, thisitem);
}
