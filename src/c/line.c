/** @file line.c

  @author  Melwin Svensson.
  @date    21-5-2025.

 */
#include "../include/c_proto.h"


/* Return 'TRUE' when 'line' is part of the marked region. */
bool line_in_marked_region_for(openfilestruct *const file, linestruct *const line) {
  ASSERT(file);
  ASSERT(line);
  return (file->mark
   && ((line->lineno >= file->mark->lineno && line->lineno <= file->current->lineno)
   || (line->lineno <= file->mark->lineno && line->lineno >= file->current->lineno)));
}

/* Return 'TRUE' when 'line' is part of the marked region. */
bool line_in_marked_region(linestruct *const line) {
  return line_in_marked_region_for(CTX_OF, line);
}

char *line_last_mbchr(const linestruct *const line) {
  ASSERT(line);
  return (line->data + step_left(line->data, strlen(line->data)));
}

/* ----------------------------- Move line ----------------------------- */

/* Move a single line up/down by simply swapping data ptrs. */
void move_line_data(linestruct *const line, bool up) {
  DLIST_SAFE_ATOMIC_SWAP_FIELD(line, data, up);
  // /* Up */
  // if (up && line->prev) {
  //   DLIST_ATOMIC_SWAP_FIELD_PREV(line, data);
  // }
  // /* Down */
  // else if (!up && line->next) {
  //   DLIST_ATOMIC_SWAP_FIELD_NEXT(line, data);
  // }
}

/* ----------------------------- Move lines up ----------------------------- */

/* Call function to move lines up one line, either one line, or all selected lines in `file`. */
void move_lines_up_for(openfilestruct *const file) {
  ASSERT(file);
  linestruct *top;
  linestruct *bot;
  /* Multi-line move. */
  if (file->mark && file->current != file->mark) {
    /* Mark is top. */
    if (mark_is_before_cursor_for(file)) {
      top = file->mark;
      bot = file->current;
    }
    /* Current is top. */
    else {
      top = file->current;
      bot = file->mark;
    }
    /* Only perform the move when top is not the top of the file. */
    if (top->prev) {
      add_undo_for(file, MOVE_LINE_UP, NULL);
      /* Start at the line above top, and move it until it's the bottom line. */
      DLIST_FOR_NEXT_END(top->prev, bot, line) {
        move_line_data(line, DOWN);
      }
      set_modified_for(file);
      /* Move both current and mark lines up by one. */
      DLIST_ADV_PREV(file->current);
      DLIST_ADV_PREV(file->mark);
      /* Also indecate that the mark should not be removed. */
      keep_mark      = TRUE;
      refresh_needed = TRUE;
    }
  }
  /* Single-line move.  Only allow this move when not already at the beginning of the file. */
  else if (file->current->prev) {
    add_undo_for(file, MOVE_LINE_UP, NULL);
    move_line_data(file->current, UP);
    set_modified_for(file);
    /* Move the current line up once, so it aligns with the moved data. */
    DLIST_ADV_PREV(file->current);
    /* When there was a single line marked region, also move
     * the mark up and indecate the mark should not be removed. */
    if (file->mark) {
      DLIST_ADV_PREV(file->mark);
      keep_mark = TRUE;
    }
    refresh_needed = TRUE;
  }
}

/* Call function to move lines up one line, either one line, or all selected
 * lines in the currently open buffer.  Note that this is `context-safe`. */
void move_lines_up(void) {
  move_lines_up_for(CTX_OF);
}

/* ----------------------------- Move lines down ----------------------------- */

/* Call function to move lines down one line, either one line, or all selected lines in `file`. */
void move_lines_down_for(openfilestruct *const file) {
  ASSERT(file);
  linestruct *top;
  linestruct *bot;
  /* Multi-line move. */
  if (file->mark && file->mark != file->current) {
    /* Mark is top. */
    if (mark_is_before_cursor_for(file)) {
      top = file->mark;
      bot = file->current;
    }
    /* Current is top. */
    else {
      top = file->current;
      bot = file->mark;
    }
    /* Only perform any moving when not already at the bottom of the file. */
    if (bot->next) {
      add_undo_for(file, MOVE_LINE_DOWN, NULL);
      /* Start at the line after bottom, and move the data up until we reach the top line. */
      DLIST_FOR_PREV_END(bot->next, top, line) {
        move_line_data(line, UP);
      }
      set_modified_for(file);
      /* Move both the current and mark lines down by one. */
      DLIST_ADV_NEXT(file->current);
      DLIST_ADV_NEXT(file->mark);
      /* Indecate that the mark should not be removed. */
      keep_mark      = TRUE;
      refresh_needed = TRUE;
    }
  }
  /* Singe line move.  Only allow this move when not already at the end of the file. */
  else if (file->current->next) {
    add_undo_for(file, MOVE_LINE_DOWN, NULL);
    move_line_data(file->current, DOWN);
    set_modified_for(file);
    /* Move the current line down once, so it aligns with the moved data. */
    DLIST_ADV_NEXT(file->current);
    /* When there was a single line marked region, also move the
     * mark down and indecate the mark should not be removed. */
    if (file->mark) {
      DLIST_ADV_NEXT(file->mark);
      keep_mark = TRUE;
    }
    refresh_needed = TRUE;
  }
}

/* Call function to move lines down one line, either one line, or all selected
 * lines in the currently open buffer.  Note that this is `context-safe`. */
void move_lines_down(void) {
  move_lines_down_for(CTX_OF);
}

