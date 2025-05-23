/** @file text.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Return's the length of whilespace until first non blank char in `string`. */
Ulong indentlen(const char *const restrict string) {
  ASSERT(string);
  const char *ptr = string;
  while (is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return (ptr - string);
}

/* Find the last blank in the given piece of text such that the display width to that point is at most
 * (goal + 1).  When there is no such blank, then find the first blank.  Return the index of the last
 * blank in that group of blanks. When snap_at_nl is TRUE, a newline character counts as a blank too. */
long break_line(const char *textstart, long goal, bool snap_at_nl) {
  /* The point where the last blank was found, if any. */
  const char *lastblank = NULL;
  /* An iterator through the given line of text. */
  const char *pointer = textstart;
  /* The column number that corresponds to the position of the pointer. */
  Ulong column = 0;
  /* Skip over leading whitespace, where a line should never be broken. */
  while (*pointer && is_blank_char(pointer)) {
    pointer += advance_over(pointer, &column);
  }
  /* Find the last blank that does not overshoot the target column.
   * When treating a help text, do not break in the keystrokes area. */
  while (*pointer && (long)column <= goal) {
    if (is_blank_char(pointer) && (!inhelp || column > 17 || goal < 40)) {
      lastblank = pointer;
    }
    else if (snap_at_nl && *pointer == '\n') {
      lastblank = pointer;
      break;
    }
    pointer += advance_over(pointer, &column);
  }
  /* If the whole line displays shorter than goal, we're done. */
  if ((long)column <= goal) {
    return (long)(pointer - textstart);
  }
  /* When wrapping a help text and no blank was found, force a line break. */
  if (snap_at_nl && !lastblank) {
    return (long)step_left(textstart, (pointer - textstart));
  }
  /* If no blank was found within the goal width, seek one after it. */
  while (!lastblank) {
    if (!*pointer) {
      return -1;
    }
    if (is_blank_char(pointer)) {
      lastblank = pointer;
    }
    else {
      pointer += char_length(pointer);
    }
  }
  pointer = (lastblank + char_length(lastblank));
  /* Skip any consecutive blanks after the last blank. */
  while (*pointer && is_blank_char(pointer)) {
    lastblank = pointer;
    pointer += char_length(pointer);
  }
  return (long)(lastblank - textstart);
}

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

/* ----------------------------- Indent ----------------------------- */

/* Add an indent to the given line.  TODO: Make the mark and curren x positions move exactly like vs-code. */
void indent_a_line_for(openfilestruct *const file, linestruct *const line, const char *const restrict indentation) {
  ASSERT(file);
  ASSERT(line);
  ASSERT(indentation);
  Ulong length     = strlen(line->data);
  Ulong indent_len = strlen(indentation);
  /* If the requested indentation is empty, don't change the line. */
  if (!indent_len) {
    return;
  }
  /* Inject the indentation at the begining of the line. */
  line->data = xnstrninj(line->data, length, indentation, indent_len, 0);
  /* Then increase the total size of the file by the indent length added. */
  file->totsize += indent_len;
  /* When `line` is the mark as well ensure the mark only  */
  if (line == file->mark && file->mark_x > 0) {
    file->mark_x += indent_len;
  }
  if (line == file->current && file->current_x > 0) {
    file->current_x  += indent_len;
    file->placewewant = xplustabs_for(file);
  }
}

/* Add an indent to the given line. */
void indent_a_line(linestruct *const line, const char *const restrict indentation) { 
  indent_a_line_for((ISSET(USING_GUI) ? openeditor->openfile : openfile), line, indentation);
}
