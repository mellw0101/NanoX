/** @file move.c

  @author  Melwin Svensson.
  @date    21-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Return the index in line->data that corresponds to the given column on the chunk that starts at the given leftedge.  If the target column has landed
 * on a tab, prevent the cursor from falling back a row when moving forward, or from skipping a row when moving backward, by incrementing the index. */
static Ulong proper_x(linestruct *const line, Ulong *const leftedge, bool forward, Ulong column, bool *const shifted, int total_cols) {
  ASSERT(line);
  ASSERT(leftedge);
  Ulong index = actual_x(line->data, column);
  if(ISSET(SOFTWRAP) && line->data[index] == '\t'
   && ((forward && wideness(line->data, index) < (*leftedge)) || (!forward && (column / tabsize) == (((*leftedge) - 1) / tabsize) && (column / tabsize) < (((*leftedge) + total_cols - 1) / tabsize)))){
    ++index;
    ASSIGN_IF_VALID(shifted, TRUE);
  }
  if(ISSET(SOFTWRAP)){
    (*leftedge) = leftedge_for(wideness(line->data, index), line, total_cols);
  }
  return index;
}

/* Adjust the values for `file->current_x` and `file->placewewant` in case we have landed in the middle of a tab that crosses a row boundary. */
static void set_proper_index_and_pww_for(openfilestruct *const file, Ulong *const leftedge, Ulong target, bool forward, int total_cols) {
  ASSERT(file);
  ASSERT(leftedge);
  Ulong was_edge  = (*leftedge);
  bool  shifted   = FALSE;
  file->current_x = proper_x(file->current, leftedge, forward, actual_last_column_for(file, (*leftedge), target, total_cols), &shifted, total_cols);
  /* If the index was incremented, try going to the target column. */
  if(shifted || (*leftedge) < was_edge){
    file->current_x = proper_x(file->current, leftedge, forward, actual_last_column_for(file, (*leftedge), target, total_cols), &shifted, total_cols);
  }
  file->placewewant = ((*leftedge) + target);
}

/* Adjust the values for `openfile->current_x` and `openfile->placewewant` in case we have landed in the middle of a tab that crosses a row boundary. */
// _UNUSED static inline void set_proper_index_and_pww(Ulong *const leftedge, Ulong target, bool forward) {
//   set_proper_index_and_pww_for(openfile, leftedge, target, forward, editwincols);
// }


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


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
  if (IN_GUI_CONTEXT) {
    to_first_line_for(openeditor->openfile);
  }
  else {
    to_first_line_for(openfile);
  }
}

/* Set `file->current` to `file->filebot`. */
void to_last_line_for(openfilestruct *const file, int total_rows) {
  ASSERT(file);
  file->current     = file->filebot;
  file->current_x   = (inhelp ? 0 : strlen(file->filebot->data));
  file->placewewant = xplustabs_for(file);
  /* Set the last line of the screen as the target for the cursor. */
  file->cursor_row  = (total_rows - 1);
  refresh_needed    = TRUE;
  focusing          = FALSE;
  recook |= perturbed;
}

/* Move to the last line of the file. */
void to_last_line(void) {
  if (IN_GUI_CONTEXT) {
    to_last_line_for(openeditor->openfile, openeditor->rows);
  }
  else {
    to_last_line_for(openfile, editwinrows);
  }
}

/* Determine the actual chunk and the target column. */
void get_edge_and_target_for(openfilestruct *const file, Ulong *const leftedge, Ulong *target_column, int total_cols) {
  ASSERT(file);
  ASSERT(leftedge);
  ASSERT(target_column);
  Ulong shim;
  if (ISSET(SOFTWRAP)) {
    shim             = (total_cols * (1 + (tabsize / total_cols)));
    (*leftedge)      = leftedge_for(xplustabs_for(file), file->current, total_cols);
    (*target_column) = ((file->placewewant + shim - (*leftedge)) % total_cols);
  }
  else {
    (*leftedge)      = 0;
    (*target_column) = file->placewewant;
  }
}

/* Determine the actual chunk and the target column, for the currently open file.. */
void get_edge_and_target(Ulong *const leftedge, Ulong *target_column) {
  if (IN_GUI_CONTEXT) {
    get_edge_and_target_for(openeditor->openfile, leftedge, target_column, openeditor->cols);
  }
  else { 
    get_edge_and_target_for(openfile, leftedge, target_column, editwincols);
  }
}

/* Move up almost one screen-full in `file`, where the biggest possible move is `total_rows - 2`. */
void do_page_up_for(openfilestruct *const file, int total_rows, int total_cols) {
  ASSERT(file);
  int mustmove = ((total_rows < 3) ? 1 : (total_rows - 2));
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
    get_edge_and_target_for(file, &leftedge, &target_column, total_cols);
  }
  /* Move up the requered number of lines or chunks.  If we can't, we're at the top of the file, so put the cursor there and get out. */
  if (go_back_chunks_for(file, mustmove, &file->current, &leftedge, total_cols) > 0) {
    to_first_line_for(file);
    return;
  }
  set_proper_index_and_pww_for(file, &leftedge, target_column, BACKWARD, total_cols);
  /* Move the viewport so that the cursor stays imobile, if possible. */
  adjust_viewport_for(file, STATIONARY, total_rows, total_cols);
  refresh_needed = TRUE;
}

/* Move up almost one screen-full for `openfile`, where the biggest possible move is `editwinrows - 2`. */
void do_page_up(void) {
  if (IN_GUI_CONTEXT) {
    do_page_up_for(openeditor->openfile, openeditor->rows, openeditor->cols);
  }
  else {
    do_page_up_for(openfile, editwinrows, editwincols);
  }
}

/* Move down almost one screen-full in `file`, where the biggest possible move is `total_rows - 2`. */
void do_page_down_for(openfilestruct *const file, int total_rows, int total_cols) {
  ASSERT(file);
  int mustmove = ((total_rows < 3) ? 1 : (total_rows - 2));
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
    get_edge_and_target_for(file, &leftedge, &target_column, total_cols);
  }
  /* Move down the required number of lines or chunks.  If we can't, we're at the bottom of the file, so put the cursor there and get out. */
  if (go_forward_chunks_for(file, mustmove, &file->current, &leftedge, total_cols) > 0) {
    to_last_line_for(file, total_rows);
    return;
  }
  set_proper_index_and_pww_for(file, &leftedge, target_column, FORWARD, total_cols);
  /* Move the viewport so that the cursor stays immobile, if possible. */
  adjust_viewport_for(file, STATIONARY, total_rows, total_cols);
  refresh_needed = TRUE;
}

/* Move down almost one screen-full for `openfile`, where the biggest possible move is `editwinrows - 2`. */
void do_page_down(void) {
  if (IN_GUI_CONTEXT) {
    do_page_down_for(GUI_CONTEXT);
  }
  else {
    do_page_down_for(TUI_CONTEXT);
  }
}

/* Place the cursor on the first row in the viewport. */
void to_top_row_for(openfilestruct *const file, int total_cols) {
  ASSERT(file);
  Ulong leftedge;
  Ulong target_column;
  get_edge_and_target_for(file, &leftedge, &target_column, total_cols);
  file->current = file->edittop;
  leftedge      = file->firstcolumn;
  set_proper_index_and_pww_for(file, &leftedge, target_column, BACKWARD, total_cols);
  place_the_cursor_for(file);
}

/* Place the cursor on the first row in the viewport. */
void to_top_row(void) {
  if (IN_GUI_CONTEXT) {
    to_top_row_for(openeditor->openfile, openeditor->cols);
  }
  else {
    to_top_row_for(openfile, editwincols);
  }
}

/* Place the cursor on the last row in the viewport, when possible. */
void to_bottom_row_for(openfilestruct *const file, int total_rows, int total_cols) {
  ASSERT(file);
  Ulong leftedge;
  Ulong target_column;
  get_edge_and_target_for(file, &leftedge, &target_column, total_cols);
  file->current = file->edittop;
  leftedge      = file->firstcolumn;
  go_forward_chunks_for(file, (total_rows - 1), &file->current, &leftedge, total_cols);
  set_proper_index_and_pww_for(file, &leftedge, target_column, FORWARD, total_cols);
  place_the_cursor_for(file);
}

/* Place the cursor on the last row in the viewport, when possible. */
void to_bottom_row(void) {
  if (IN_GUI_CONTEXT) {
    to_bottom_row_for(openeditor->openfile, openeditor->rows, openeditor->cols);
  }
  else {
    to_bottom_row_for(openfile, editwinrows, editwincols);
  }
}

/* Put the cursor line at the center, then the top, then the bottom in `file`. */
void do_cycle_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  if (cycling_aim == 0) {
    adjust_viewport_for(file, CENTERING, rows, cols);
  }
  else {
    file->cursor_row = ((cycling_aim == 1) ? 0 : (rows - 1));
    adjust_viewport_for(file, STATIONARY, rows, cols);
  }
  cycling_aim = ((cycling_aim + 1) % 3);
  draw_all_subwindows();
  full_refresh();
}

/* Put the cursor line at the center, then the top, then the bottom. */
void do_cycle(void) {
  if (IN_GUI_CONTEXT) {
    do_cycle_for(GUI_CONTEXT);
  }
  else {
    do_cycle_for(TUI_CONTEXT);
  }
  // do_cycle_for(CONTEXT_OPENFILE, CONTEXT_ROWS, CONTEXT_COLS);
}

/* Scroll the line with the cursor to the center of the screen. */
void do_center(void) {
  /* The main loop has set 'cycling_aim' to zero. */
  do_cycle();
}

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

/* Move up to first start of a paragraph before the current line. */
void to_para_begin_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  linestruct *was_current = file->current;
  do_para_begin(&file->current);
  file->current_x = 0;
  edit_redraw_for(file, rows, cols, was_current, CENTERING);
}

/* Move up to first start of a paragraph before the current line. */
void to_para_begin(void) {
  if (IN_GUI_CONTEXT) {
    to_para_begin_for(GUI_CONTEXT);
  }
  else {
    to_para_begin_for(TUI_CONTEXT);
  }
}

/* Move down to just after the first found end of a paragraph. */
void to_para_end_for(openfilestruct *const file, int rows, int cols) {
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
  edit_redraw_for(file, rows, cols, was_current, FLOWING);
  recook |= perturbed;
}

/* Move down to just after the first found end of a paragraph. */
void to_para_end(void) {
  if (IN_GUI_CONTEXT) {
    to_para_end_for(GUI_CONTEXT);
  }
  else {
    to_para_end_for(TUI_CONTEXT);
  }
}

/* Move to the preceding block of text. */
void to_prev_block_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  linestruct *const was_current = file->current;
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
  if (IN_GUI_CONTEXT) {
    to_prev_block_for(GUI_CONTEXT);
  }
  else {
    to_prev_block_for(TUI_CONTEXT);
  }
}

/* Move to the next block of text inside `file`. */
void to_next_block_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  linestruct *const was_current = file->current;
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
  if (IN_GUI_CONTEXT) {
    to_next_block_for(GUI_CONTEXT);
  }
  else {
    to_next_block_for(TUI_CONTEXT);
  }
}
