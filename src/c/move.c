/** @file move.c

  @author  Melwin Svensson.
  @date    21-5-2025.

 */
#include "../include/c_proto.h"

/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */

/* Return the index in line->data that corresponds to the given column on the chunk that starts at the
 * given leftedge.  If the target column has landed on a tab, prevent the cursor from falling back
 * a row when moving forward, or from skipping a row when moving backward, by incrementing the index. */
static Ulong proper_x(linestruct *const line, int cols, Ulong *const leftedge, bool forward, Ulong column, bool *const shifted) {
  ASSERT(line);
  ASSERT(leftedge);
  Ulong index = actual_x(line->data, column);
  if (ISSET(SOFTWRAP) && line->data[index] == '\t' &&
      ((forward && wideness(line->data, index) < (*leftedge)) ||
       (!forward && (column / tabsize) == (((*leftedge) - 1) / tabsize) && (column / tabsize) < (((*leftedge) + cols - 1) / tabsize)))) {
    ++index;
    ASSIGN_IF_VALID(shifted, TRUE);
  }
  if (ISSET(SOFTWRAP)) {
    (*leftedge) = leftedge_for(cols, wideness(line->data, index), line);
  }
  return index;
}

/* Adjust the values for `file->current_x` and `file->placewewant` in case we have landed in the middle of a tab that crosses a row boundary. */
static void set_proper_index_and_pww_for(openfilestruct *const file, int cols, Ulong *const leftedge, Ulong target, bool forward) {
  ASSERT(file);
  ASSERT(leftedge);
  Ulong was_edge  = (*leftedge);
  bool  shifted   = FALSE;
  file->current_x = proper_x(file->current, cols, leftedge, forward, actual_last_column_for(file, cols, (*leftedge), target), &shifted);
  /* If the index was incremented, try going to the target column. */
  if (shifted || (*leftedge) < was_edge) {
    file->current_x = proper_x(file->current, cols, leftedge, forward, actual_last_column_for(file, cols, (*leftedge), target), &shifted);
  }
  file->placewewant = ((*leftedge) + target);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */

/* ----------------------------- To first line ----------------------------- */

/* Set `file->current` to `file->filetop`. */
void to_first_line_for(openfilestruct *const file) {
  ASSERT(file);
  file->current     = file->filetop;
  file->current_x   = 0;
  file->placewewant = 0;
  refresh_needed    = TRUE;
}

/* Move to the first line of the currently open file. */
void to_first_line(void) {
  to_first_line_for(CTX_OF);
}

/* ----------------------------- To last line ----------------------------- */

/* Set `file->current` to `file->filebot`. */
void to_last_line_for(openfilestruct *const file, int rows) {
  ASSERT(file);
  file->current     = file->filebot;
  file->current_x   = (inhelp ? 0 : strlen(file->filebot->data));
  file->placewewant = xplustabs_for(file);
  /* Set the last line of the screen as the target for the cursor. */
  file->cursor_row = (rows - 1);
  refresh_needed   = TRUE;
  focusing         = FALSE;
  recook |= perturbed;
}

/* Move to the last line of the file. */
void to_last_line(void) {
  if (IN_GUI_CTX) {
    to_last_line_for(GUI_OF, GUI_ROWS);
  }
  else {
    to_last_line_for(TUI_OF, TUI_ROWS);
  }
}

/* ----------------------------- Get edge and target ----------------------------- */

/* Determine the actual chunk and the target column. */
void get_edge_and_target_for(openfilestruct *const file, int cols, Ulong *const leftedge, Ulong *const target_column) {
  ASSERT(file);
  ASSERT(leftedge);
  ASSERT(target_column);
  Ulong shim;
  if (ISSET(SOFTWRAP)) {
    shim             = (cols * (1 + (tabsize / cols)));
    (*leftedge)      = leftedge_for(cols, xplustabs_for(file), file->current);
    (*target_column) = ((file->placewewant + shim - (*leftedge)) % cols);
  }
  else {
    (*leftedge)      = 0;
    (*target_column) = file->placewewant;
  }
}

/* Determine the actual chunk and the target column, for the currently open file.. */
void get_edge_and_target(Ulong *const leftedge, Ulong *target_column) {
  if (IN_GUI_CTX) {
    get_edge_and_target_for(GUI_OF, GUI_COLS, leftedge, target_column);
  }
  else {
    get_edge_and_target_for(TUI_OF, TUI_COLS, leftedge, target_column);
  }
}

/* ----------------------------- Do page up ----------------------------- */

/* Move up almost one screen-full in `file`, where the biggest possible move is `total_rows - 2`. */
void do_page_up_for(CTX_ARGS) {
  ASSERT(file);
  int   mustmove = ((rows < 3) ? 1 : (rows - 2));
  Ulong leftedge;
  Ulong target_column;
  /* If we're */
  if (ISSET(JUMPY_SCROLLING)) {
    file->current    = file->edittop;
    leftedge         = file->firstcolumn;
    file->cursor_row = 0;
    target_column    = 0;
  }
  else {
    get_edge_and_target_for(file, cols, &leftedge, &target_column);
  }
  /* Move up the requered number of lines or chunks.  If we can't, we're at the top of the file, so put the cursor there and get out. */
  if (go_back_chunks_for(file, cols, mustmove, &file->current, &leftedge) > 0) {
    to_first_line_for(file);
    return;
  }
  set_proper_index_and_pww_for(file, cols, &leftedge, target_column, BACKWARD);
  /* Move the viewport so that the cursor stays imobile, if possible. */
  adjust_viewport_for(STACK_CTX, STATIONARY);
  refresh_needed = TRUE;
}

/* Move up almost one screen-full for `openfile`, where the biggest possible move is `editwinrows - 2`. */
void do_page_up(void) {
  CTX_CALL(do_page_up_for);
}

/* ----------------------------- Do page down ----------------------------- */

/* Move down almost one screen-full in `file`, where the biggest possible move is `total_rows - 2`. */
void do_page_down_for(CTX_ARGS) {
  ASSERT(file);
  int   mustmove = ((rows < 3) ? 1 : (rows - 2));
  Ulong leftedge;
  Ulong target_column;
  /* If we're not in smooth scrolling mode, put the cursor at the beginning of the top line of the edit window. */
  if (ISSET(JUMPY_SCROLLING)) {
    file->current    = file->edittop;
    leftedge         = file->firstcolumn;
    file->cursor_row = 0;
    target_column    = 0;
  }
  else {
    get_edge_and_target_for(file, cols, &leftedge, &target_column);
  }
  /* Move down the required number of lines or chunks.  If we can't, we're at the bottom of the file, so put the cursor there and get out. */
  if (go_forward_chunks_for(file, cols, mustmove, &file->current, &leftedge) > 0) {
    to_last_line_for(file, rows);
    return;
  }
  set_proper_index_and_pww_for(file, cols, &leftedge, target_column, FORWARD);
  /* Move the viewport so that the cursor stays immobile, if possible. */
  adjust_viewport_for(STACK_CTX, STATIONARY);
  refresh_needed = TRUE;
}

/* Move down almost one screen-full for `openfile`, where the biggest possible move is `editwinrows - 2`. */
void do_page_down(void) {
  CTX_CALL(do_page_down_for);
}

/* ----------------------------- To top row ----------------------------- */

/* Place the cursor on the first row in the viewport. */
void to_top_row_for(openfilestruct *const file, int cols) {
  ASSERT(file);
  Ulong leftedge;
  Ulong target_column;
  get_edge_and_target_for(file, cols, &leftedge, &target_column);
  file->current = file->edittop;
  leftedge      = file->firstcolumn;
  set_proper_index_and_pww_for(file, cols, &leftedge, target_column, BACKWARD);
  place_the_cursor_for(file);
}

/* Place the cursor on the first row in the viewport. */
void to_top_row(void) {
  if (IN_GUI_CTX) {
    to_top_row_for(GUI_OF, GUI_COLS);
  }
  else {
    to_top_row_for(TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- To bottom row ----------------------------- */

/* Place the cursor on the last row in the viewport, when possible. */
void to_bottom_row_for(CTX_ARGS) {
  ASSERT(file);
  Ulong leftedge;
  Ulong target_column;
  get_edge_and_target_for(file, cols, &leftedge, &target_column);
  file->current = file->edittop;
  leftedge      = file->firstcolumn;
  go_forward_chunks_for(file, cols, (rows - 1), &file->current, &leftedge);
  set_proper_index_and_pww_for(file, cols, &leftedge, target_column, FORWARD);
  place_the_cursor_for(file);
}

/* Place the cursor on the last row in the viewport, when possible. */
void to_bottom_row(void) {
  CTX_CALL(to_bottom_row_for);
}

/* ----------------------------- Do cycle ----------------------------- */

/* Put the cursor line at the center, then the top, then the bottom in `file`. */
void do_cycle_for(CTX_ARGS) {
  ASSERT(file);
  if (cycling_aim == 0) {
    adjust_viewport_for(STACK_CTX, CENTERING);
  }
  else {
    file->cursor_row = ((cycling_aim == 1) ? 0 : (rows - 1));
    adjust_viewport_for(STACK_CTX, STATIONARY);
  }
  cycling_aim = ((cycling_aim + 1) % 3);
  draw_all_subwindows();
  full_refresh();
}

/* Put the cursor line at the center, then the top, then the bottom. */
void do_cycle(void) {
  CTX_CALL(do_cycle_for);
}

/* ----------------------------- Do center ----------------------------- */

/* Scroll the line with the cursor to the center of the screen. */
void do_center(void) {
  /* The main loop has set 'cycling_aim' to zero. */
  do_cycle();
}

/* ----------------------------- Do para begin ----------------------------- */

/* Move to the first beginning of a paragraph before the current line. */
void do_para_begin(linestruct **const line) {
  ASSERT(line);
  if ((*line)->prev) {
    DLIST_ADV_PREV(*line);
  }
  while (!begpar(*line, 0)) {
    DLIST_ADV_PREV(*line);
  }
}

/* ----------------------------- Do para end ----------------------------- */

/* Move down to the last line of the first found paragraph. */
void do_para_end(linestruct **const line) {
  ASSERT(line);
  while ((*line)->next && !inpar(*line)) {
    DLIST_ADV_NEXT(*line);
  }
  while ((*line)->next && inpar((*line)->next) && !begpar((*line)->next, 0)) {
    DLIST_ADV_NEXT(*line);
  }
}

/* ----------------------------- To para begin ----------------------------- */

/* Move up to first start of a paragraph before the current line. */
void to_para_begin_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  do_para_begin(&file->current);
  file->current_x = 0;
  edit_redraw_for(STACK_CTX, was_current, CENTERING);
}

/* Move up to first start of a paragraph before the current line. */
void to_para_begin(void) {
  CTX_CALL(to_para_begin_for);
}

/* ----------------------------- To para end ----------------------------- */

/* Move down to just after the first found end of a paragraph. */
void to_para_end_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  do_para_end(&file->current);
  /* Step beyond the last line of the paragraph, if possible.  Otherwise, move to the end of the line. */
  if (file->current->next) {
    DLIST_ADV_NEXT(file->current);
    file->current_x = 0;
  }
  else {
    file->current_x = strlen(file->current->data);
  }
  edit_redraw_for(STACK_CTX, was_current, FLOWING);
  recook |= perturbed;
}

/* Move down to just after the first found end of a paragraph. */
void to_para_end(void) {
  CTX_CALL(to_para_end_for);
}

/* ----------------------------- To prev block ----------------------------- */

/* Move to the preceding block of text. */
void to_prev_block_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  int cur_indent;
  int was_indent = line_indent(was_current);
  /* Skip backward until first nonblank line after some blank line(s). */
  while (file->current->prev) {
    /* Current line is not starting line. */
    if (file->current != was_current) {
      /* Get the indent of the current line in terms of visible columns. */
      cur_indent = line_indent(file->current);
      /* Current line is empty or contains only blank chars. */
      if (white_string(file->current->data)) {
        /* When the current line is the one after the starting line. */
        if (file->current == was_current->prev) {
          /* Iterate until we find a non empty or blank only line. */
          for (; file->current->prev && white_string(file->current->data); file->current = file->current->prev);
        }
        /* Otherwise, this is the first blank line after some amount of text
         * lines.  So we stop at the previous line, the end of the text block. */
        else {
          DLIST_ADV_NEXT(file->current);
        }
        break;
      }
      /* Line indent differs from the starting line. */
      if (cur_indent != was_indent) {
        /* Place the cursor at the bottom of the indent block, unless called from the bottom. */
        if (file->current != was_current->prev) {
          DLIST_ADV_NEXT(file->current);
        }
        break;
      }
    }
    /* Advance to the next line. */
    DLIST_ADV_PREV(file->current);
  }
  file->current_x = indent_length(file->current->data);
  edit_redraw_for(file, rows, cols, was_current, FLOWING);
  recook |= perturbed;
}

/* Move to the preceding block of text. */
void to_prev_block(void) {
  CTX_CALL(to_prev_block_for);
}

/* ----------------------------- To next block ----------------------------- */

/* Move to the next block of text inside `file`. */
void to_next_block_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  int cur_indent;
  int was_indent = line_indent(was_current);
  /* Skip forward until first nonblank line after some blank line(s). */
  while (file->current->next) {
    /* Current line is not starting line. */
    if (file->current != was_current) {
      /* Get the indent of the current line in terms of visible columns. */
      cur_indent = line_indent(file->current);
      /* Current line is empty or contains only blank chars. */
      if (white_string(file->current->data)) {
        /* When the current line is the one after the starting line. */
        if (file->current == was_current->next) {
          /* Iterate until we find a non empty or blank only line. */
          for (; file->current->next && white_string(file->current->data); file->current = file->current->next);
        }
        /* Otherwise, this is the first blank line after some amount of text
         * lines.  So we stop at the previous line, the end of the text block. */
        else {
          DLIST_ADV_PREV(file->current);
        }
        break;
      }
      /* Line indent differs from the starting line. */
      if (cur_indent != was_indent) {
        /* Place the cursor at the bottom of the indent block, unless called from the bottom. */
        if (file->current != was_current->next) {
          DLIST_ADV_PREV(file->current);
        }
        break;
      }
    }
    /* Advance to the next line. */
    DLIST_ADV_NEXT(file->current);
  }
  file->current_x = indent_length(file->current->data);
  edit_redraw_for(file, rows, cols, was_current, FLOWING);
  recook |= perturbed;
}

/* Move to the next block of text in the currently open file.  Note that this is context safe. */
void to_next_block(void) {
  CTX_CALL(to_next_block_for);
}

/* ----------------------------- Do Up ----------------------------- */

/* Move the cursor to the preseding line or chunk, in `file`. */
void do_up_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  Ulong       edge;
  Ulong       target;
  get_edge_and_target_for(file, cols, &edge, &target);
  /* If we can't move up one line or chunk, we're at the top of the file. */
  if (go_back_chunks_for(file, cols, 1, &file->current, &edge) > 0) {
    return;
  }
  set_proper_index_and_pww_for(file, cols, &edge, target, BACKWARD);
  if (!file->cursor_row && !ISSET(JUMPY_SCROLLING) && (tabsize < cols || !ISSET(SOFTWRAP))) {
    edit_scroll_for(file, BACKWARD);
  }
  else {
    edit_redraw_for(STACK_CTX, was_current, FLOWING);
  }
  /* <Up> should not change placewewant, so restore it. */
  file->placewewant = (edge + target);
}

/* Move the cursor to the preceding line or chunk, in the currently open buffer.  Note that this is `context-safe`. */
void do_up(void) {
  CTX_CALL(do_up_for);
}

/* ----------------------------- Do down ----------------------------- */

/* Move the cursor to the next line or chunk, in `file`. */
void do_down_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  Ulong edge;
  Ulong target;
  get_edge_and_target_for(file, cols, &edge, &target);
  /* If we can't move down one line or chunk, we're at the bottom of the file. */
  if (go_forward_chunks_for(file, cols, 1, &file->current, &edge) > 0) {
    return;
  }
  set_proper_index_and_pww_for(file, cols, &edge, target, FORWARD);
  if (file->cursor_row == (rows - 1) && !ISSET(JUMPY_SCROLLING) && (tabsize < cols || !ISSET(SOFTWRAP))) {
    edit_scroll_for(file, FORWARD);
  }
  else {
    edit_redraw_for(STACK_CTX, was_current, FLOWING);
  }
  /* <Down> should not change placewewant, so restore it. */
  file->placewewant = (edge + target);
}

/* Move the cursor to the next line or chunk, in the currently open buffer.  Note that this is `context-safe`. */
void do_down(void) {
  CTX_CALL(do_down_for);
}

/* ----------------------------- Do left ----------------------------- */

/* Either, move left one character in `file`.  Or when `file` has a marked region, place the cursor at the start of that region. */
void do_left_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *const was_current = file->current;
  /* If a section is highlighted and shift is not held, then place the cursor at the left side of the marked area. */
  if (file->mark && file->softmark && !shift_held) {
    /* Only adjust the cursor when the mark is the `left` one.  Otherwise, the cursor already is.  */
    if (mark_is_before_cursor_for(file)) {
      file->current   = file->mark;
      file->current_x = file->mark_x;
    }
  }
  else {
    /* When not at the start of the current line. */
    if (file->current_x > 0) {
      step_cursor_left(file);
    }
    /* Otherwise, when at the start of the current line, move one line up when possible. */
    else if (file->current != file->filetop) {
      DLIST_ADV_PREV(file->current);
      file->current_x = strlen(file->current->data);
    }
  }
  edit_redraw_for(STACK_CTX, was_current, FLOWING);
}

/* Either, move left one character in the currently open buffer.  Or when that buffer has a
 * marked region, place the cursor at the start of that region.  Note that this is `context-safe`. */
void do_left(void) {
  CTX_CALL(do_left_for);
}

/* ----------------------------- Do right ----------------------------- */

/* Either, move right one character in `file`.  Or when `file` has a marked region, place the cursor at the end of that region. */
void do_right_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  /* If a section is highlighted and shift is not held, then place the cursor at the end of the marked region. */
  if (file->mark && file->softmark && !shift_held) {
    /* Only adjust the cursor when the mark is after the cursor.  Otherwise, the cursor is already in the correct position. */
    if (!mark_is_before_cursor_for(file)) {
      file->current   = file->mark;
      file->current_x = file->mark_x;
    }
  }
  else {
    /* When not already at the end of the current line. */
    if (file->current->data[file->current_x]) {
      step_cursor_right(file);
    }
    /* Otherwise, when at the end of the current line, move one line down when possible. */
    else if (file->current != file->filebot) {
      DLIST_ADV_NEXT(file->current);
      file->current_x = 0;
    }
  }
  edit_redraw_for(STACK_CTX, was_current, FLOWING);
}

/* Either, move right one character in the currently open buffer.  Or when that buffer has a
 * marked region, place the cursor at the end of that region.  Note that this is `context-safe`. */
void do_right(void) {
  CTX_CALL(do_right_for);
}

/* ----------------------------- Do prev word ----------------------------- */

/* Move to the previous word in `file`. */
void do_prev_word_for(openfilestruct *const file, bool allow_punct) {
  ASSERT(file);
  /* The indent length of the current line. */
  Ulong indentlen = indent_length(file->current->data);
  /* The x position we started at. */
  Ulong start_x = file->current_x;
  /* Is set to true every time we see a word char. */
  bool seen_a_word = FALSE;
  /* If we started at the beginning of the current line. */
  bool started_at_zero = !start_x;
  /* Move backward until we pass over the start of a word. */
  while (TRUE) {
    /* Step one true step left, meaning we will take one step left, and then advance over all zero-width chars. */
    step_cursor_left(file);
    /* We are at the beginning of the current line. */
    if (!file->current_x) {
      /* When we started at the beggining of the current line, move to the end of the preceeding line, if possible. */
      if (started_at_zero && file->current->prev) {
        DLIST_ADV_PREV(file->current);
        file->current_x = strlen(file->current->data);
        /* Set the starting x position to the new x position. */
        start_x = file->current_x;
        /* Update `indentlen` to reflect the new current line's indent length. */
        indentlen = indent_length(file->current->data);
        /* Set started at zero to `FALSE`. */
        started_at_zero = FALSE;
      }
      /* Otherwise, when we did not start at the beginning, we stop at the beginning. */
      else {
        break;
      }
    }
    /* Else, when we are at the indent length of the current line, and we did not start here. */
    else if (file->current_x == indentlen && start_x != indentlen) {
      break;
    }
    /* The current char under the cursor is either, a general word char, or language specific word char. */
    if (IS_WORD_CHAR(file, allow_punct) || is_lang_word_char(file)) {
      seen_a_word = TRUE;
      /* If we are at the start of the line, and the char under the cursor is a word char, we must be at the word start. */
      if (!file->current_x) {
        break;
      }
    }
    /* We are now at a non word char, in other words we found the start of
     * a word, so move right once so we end up at the start of the word. */
    else if (seen_a_word) {
      step_cursor_right(file);
      break;
    }
  }
}

/* Move to the start of the previous word in the currently open buffer.  Note that this is `context-saft` */
void do_prev_word(void) {
  do_prev_word_for(CTX_OF, ISSET(WORD_BOUNDS));
}

/* ----------------------------- To prev word ----------------------------- */

/* Move to the previous word in `file`, and update the screen afterwards. */
void to_prev_word_for(CTX_ARGS, bool allow_punct) {
  ASSERT(file);
  linestruct *was_current = file->current;
  do_prev_word_for(file, allow_punct);
  edit_redraw_for(STACK_CTX, was_current, FLOWING);
}

/* Move to the previous word in the currently open buffer, and
 * update the screen afterwards.  Note that this is `context-safe`. */
void to_prev_word(void) {
  CTX_CALL_WARGS(to_prev_word_for, ISSET(WORD_BOUNDS));
}

/* ----------------------------- Do next word ----------------------------- */

/* Move to the next word in `file`.  If `after_ends` is `TRUE`, stop at the ends
 * of words instead of at their beginnings.  Returns `TRUE` if we started at a word.
 * And if `allow_punct` is `TRUE`, then punctuations are considered word chars. */
bool do_next_word_for(CTX_ARG_OF, bool after_ends, bool allow_punct) {
  ASSERT(file);
  bool started_on_word = (IS_WORD_CHAR(file, allow_punct) || is_lang_word_char(file));
  bool seen_space      = !started_on_word;
  bool seen_word       =  started_on_word;
  bool started_at_eol  = !file->current->data[file->current_x];
  /* Move forward until we reach the start, or end of a word.  Depending on `after_ends`. */
  while (TRUE) {
    /* Step one true step right, meaning step one char to the right
     * in file, then advance over all following zero-width chars. */
    step_cursor_right(file);
    /* We are at the end of the current line. */
    if (!file->current->data[file->current_x]) {
      /* When we started at the end of the current line and the current line is not the
       * last line in the file, move to the start of the first word at the next line. */
      if (started_at_eol && file->current->next) {
        DLIST_ADV_NEXT(file->current);
        file->current_x = indent_length(file->current->data);
        started_at_eol = FALSE;
      }
      else {
        break;
      }
    }
    /* Stopping after words. */
    if (after_ends) {
      /* The current char under the cursor is either, a general word char, or language specific word char. */
      if (IS_WORD_CHAR(file, allow_punct) || is_lang_word_char(file)) {
        seen_word = TRUE;
      }
      /* We are now at a non word char, in other words we found the start of
       * a word, so move right once so we end up at the start of the word. */ 
      else if (seen_word) {
        break;
      }
    }
    /* Stopping at the start of words. */
    else {
      /* The current char under the cursur in neither a word char, nor a language specific word char. */
      if (!IS_WORD_CHAR(file, allow_punct) && !is_lang_word_char(file)) {
        seen_space = TRUE;
      }
      /* We are now at a word char, or a language specific word char. */
      else if (seen_space) {
        break;
      }
    }
  }
  return started_on_word;
}

/* Move to the next word in the currently open file.  If `after_ends` is `TRUE`, stop at
 * the ends of words instead of at their beginnings.  Returns `TRUE` if we started at a
 * word.  And if `allow_punct` is `TRUE`, then punctuations are considered word chars. */
bool do_next_word(bool after_ends) {
  return do_next_word_for(CTX_OF, after_ends, ISSET(WORD_BOUNDS));
}

/* ----------------------------- To next word ----------------------------- */

/* Move to the next word in `file`.  If the `AFTER_ENDS` flag is set, stop at
 * the end of words instead of at the beginning.  Update the screen afterwards. */
void to_next_word_for(CTX_ARGS, bool after_ends, bool allow_punct) {
  ASSERT(file);
  linestruct *was_current = file->current;
  do_next_word_for(file, after_ends, allow_punct);
  edit_redraw_for(STACK_CTX, was_current, FLOWING);
}

/* Move to the next word in the currently open buffer.  If the `AFTER_ENDS` flag is set,
 * stop at the end of words instead of at the beginning.  Update the screen_afterwards. */
void to_next_word(void) {
  CTX_CALL_WARGS(to_next_word_for, ISSET(AFTER_ENDS), ISSET(WORD_BOUNDS));
}

/* ----------------------------- Do home ----------------------------- */

/* Move to the beginning of the current line (or soft-wrapped chunk).  When enabled, do smart-home. 
 * When soft-wrapping, go to the beginning of the full line when already at the start of the chunk. */
void do_home_for(CTX_ARGS) {
  ASSERT(file);
  bool moved_off_chunk = TRUE;
  bool moved           = FALSE;
  Ulong leftedge = 0;
  Ulong left_x   = 0;
  /* Save the current line in `file`. */
  linestruct *was_current = file->current;
  /* Save the current column position in `file`. */
  Ulong was_column = xplustabs_for(file);
  /* Save the indent length of the current line. */
  Ulong indent_len = indent_length(file->current->data);
  /* Softwrapping is enabled. */
  if (ISSET(SOFTWRAP)) {
    leftedge = leftedge_for(cols, was_column, file->current);
    left_x   = proper_x(file->current, cols, &leftedge, BACKWARD, leftedge, NULL);
  }  
  /* Smart-home is enabled. */
  if (ISSET(SMART_HOME) && file->current->data[indent_len]) {
    /* If we're exactly on the indent, move fully home.  Otherwise, when not softwrapping
     * or not after the first nonblank chunk, move to the first nonblank character. */
    if (file->current_x == indent_len) {
      file->current_x = 0;
      moved           = TRUE;
    }
    else if (left_x <= indent_len) {
      file->current_x = indent_len;
      moved           = TRUE;
    }
  }
  /* If we haven't moved. */
  if (!moved) {
    if (ISSET(SOFTWRAP)) {
      /* If already at the left edge of the screen, move fully home.  Otherwise, move to the left edge. */
      if (file->current_x == left_x) {
        file->current_x = 0;
      }
      else {
        file->current_x   = left_x;
        file->placewewant = leftedge;
        moved_off_chunk   = FALSE;
      }
    }
    else {
      /* If column and indent is the same, move to the beginning. */
      if (file->current_x == indent_len || !file->current->data[indent_len]) {
        file->current_x = 0;
      }
      else {
        file->current_x = indent_len;
      }
    }
  }
  if (moved_off_chunk) {
    set_pww_for(file);
  }
  /* When in curses mode, ensure the visual state is updated. */
  if (IN_CURSES_CTX) {
    /* If we changed chunk, we might be offscreen.  Otherwise, update current if the mark is on or we changed `page`. */
    if (ISSET(SOFTWRAP) && moved_off_chunk) {
      edit_redraw_for(STACK_CTX, was_current, FLOWING);
    }
    else if (line_needs_update_for(file, cols, was_column, file->placewewant)) {
      update_line_curses_for(file, file->current, file->current_x);
    }
  }
}

/* Move to the beginning of the current line (or soft-wrapped chunk).  When enabled, do smart-home. 
 * When soft-wrapping, go to the beginning of the full line when already at the start of the chunk. */
void do_home(void) {
  CTX_CALL(do_home_for);
}

/* ----------------------------- Do end ----------------------------- */

/* Move to the end of the current line (or soft-wrapped chunk) in `file`.  When
 * soft-wrapping and already at the end of a `chunk`, go to the end of the full line. */
void do_end_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  bool moved_off_chunk = TRUE;
  bool kickoff         = TRUE;
  bool last_chunk      = FALSE;
  Ulong was_column = xplustabs_for(file);
  Ulong line_len   = strlen(file->current->data);
  Ulong leftedge;
  Ulong rightedge;
  Ulong right_x;
  if (ISSET(SOFTWRAP)) {
    leftedge   = leftedge_for(cols, was_column, file->current);
    rightedge  = get_softwrap_breakpoint(cols, file->current->data, leftedge, &kickoff, &last_chunk);
    /* If we're on the last chunk, we're already at the end of the line.  Otherwise, we're one culumn past the end of the line.
     * Shifting backwards one column might put us in the middle of a milti-column character, but `actual_x()` will fix that.
     * Why even do this?  This can be solved easily by `step_left()`? */
    if (!last_chunk) {
      --rightedge;
    }
    right_x = actual_x(file->current->data, rightedge);
    /* If already at the right edge of the screen, move fully to the end of the line.  Otherwise, move to the right edge. */
    if (file->current_x == right_x) {
      file->current_x = line_len;
    }
    else {
      file->current_x   = right_x;
      file->placewewant = rightedge;
      moved_off_chunk   = FALSE;
    }
  }
  else {
    file->current_x = line_len;
  }
  if (moved_off_chunk) {
    set_pww_for(file);
  }
  /* When running in curses-mode, ensure these updates are updated visualy. */
  if (IN_CURSES_CTX) {
    /* If we changed chunk, we might be offscreen.  Otherwise, update current if the mark is on or we changed `page`. */
    if (ISSET(SOFTWRAP) && moved_off_chunk) {
      edit_redraw_for(STACK_CTX, was_current, FLOWING);
    }
    else if (line_needs_update_for(file, cols, was_column, file->placewewant)) {
      update_line_curses_for(file, file->current, file->current_x);
    }
  }
}

/* Move to the end of the current line (or soft-wrapped chunk) in `file`.  When
 * soft-wrapping and already at the end of a `chunk`, go to the end of the full line. */
void do_end(void) {
  CTX_CALL(do_end_for);
}

/* ----------------------------- Do scroll up ----------------------------- */

/* Scroll up one line or chunk without moving the cursor textwise.  This is not true...? */
void do_scroll_up_for(CTX_ARGS) {
  ASSERT(file);
  /* When the top of the file is onscreen, we can't scroll. */
  if (!file->edittop->prev && !file->firstcolumn) {
    return;
  }
  /* For now this drags the cursor at the bottom of the screen, and this will be
   * changed to support an offscreen cursor, like any editor as this is anoying. */
  if (file->cursor_row == (rows - 1)) {
    do_up_for(STACK_CTX);
  }
  if (rows > 1) {
    edit_scroll_for(file, BACKWARD);
  }
}

/* Scroll up one line or chunk without moving the cursor textwise.  This is not true...?  Note that this is `context-safe`. */
void do_scroll_up(void) {
  CTX_CALL(do_scroll_up_for);
}

/* ----------------------------- Do scroll down ----------------------------- */

/* Scroll down one line or chunk without moving the cursor textwise.  This is not true...? */
void do_scroll_down_for(CTX_ARGS) {
  ASSERT(file);
  if (!file->cursor_row) {
    do_down_for(STACK_CTX);
  }
  if (rows > 1 && (file->edittop->next || (ISSET(SOFTWRAP)
   && (extra_chunks_in(cols, file->edittop) > chunk_for(cols, file->firstcolumn, file->edittop))))) {
    edit_scroll_for(file, FORWARD);
  }
}

/* Scroll down one line or chunk without moving the cursor textwise.  This is not true...?  Note that this is `context-safe`. */
void do_scroll_down(void) {
  CTX_CALL(do_scroll_down_for);
}
