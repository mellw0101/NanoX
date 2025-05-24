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
_UNUSED static inline void set_proper_index_and_pww(Ulong *const leftedge, Ulong target, bool forward) {
  set_proper_index_and_pww_for(openfile, leftedge, target, forward, editwincols);
}


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
  if (ISSET(USING_GUI)) {
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
  if (ISSET(USING_GUI)) {
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
  if (ISSET(USING_GUI)) {
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
  if (ISSET(USING_GUI)) {
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
  if (ISSET(USING_GUI)) {
    do_page_down_for(openeditor->openfile, openeditor->rows, openeditor->cols);
  }
  else {
    do_page_down_for(openfile, editwinrows, editwincols);
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
  if (ISSET(USING_GUI)) {
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
  if (ISSET(USING_GUI)) {
    to_bottom_row_for(openeditor->openfile, openeditor->rows, openeditor->cols);
  }
  else {
    to_bottom_row_for(openfile, editwinrows, editwincols);
  }
}

/* Move to the first beginning of a paragraph before the current line. */
void do_para_begin(linestruct **const line) {
  ASSERT(line);
  if ((*line)->prev) {
    CLIST_ADV_PREV(*line);
  }
  while (!begpar(*line, 0)) {
    CLIST_ADV_PREV(*line);
  }
}

/* Move down to the last line of the first found paragraph. */
void do_para_end(linestruct **const line) {
  ASSERT(line);
  while ((*line)->next && !inpar(*line)) {
    CLIST_ADV_NEXT(*line);
  }
  while ((*line)->next && inpar((*line)->next) && !begpar((*line)->next, 0)) {
    CLIST_ADV_NEXT(*line);
  } 
}
