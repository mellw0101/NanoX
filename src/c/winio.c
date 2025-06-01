/** @file winio.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define BRANDING        PACKAGE_STRING
#define ISO8859_CHAR    FALSE
#define ZEROWIDTH_CHAR  (is_zerowidth(text))
#define SHIM            (ISSET(ZERO) && (currmenu == MREPLACEWITH || currmenu == MYESNO) ? 1 : 0)

/* The number of bytes after which to stop painting, to avoid major slowdowns.  Note that this is only for the regex based painting system nano had. */
#define PAINT_LIMIT 2000

/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* From where in the relevant line the current row is drawn. */
Ulong from_x = 0;
/* Until where in the relevant line the current row is drawn. */
Ulong till_x = 0;
/* Whether a row's text is narrower than the screen's width. */
bool is_shorter = TRUE;
/* The number of key codes waiting in the keystroke buffer. */
Ulong waiting_codes = 0;
/* The number of keystrokes left before we blank the status bar. */
int countdown = 0;
/* Whether we are in the process of recording a macro. */
bool recording = FALSE;

/* Whether the current line has more text after the displayed part. */
static bool has_more = FALSE;
/* The starting column of the next chunk when softwrapping. */
static Ulong sequel_column = 0;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Determine the sequence number of the given buffer in the circular list. */
static int buffer_number(openfilestruct *buffer) {
  int count = 1;
  while (buffer != startfile) {
    buffer = buffer->prev;
    ++count;
  }
  return count;
}

/* ----------------------------- Curses ----------------------------- */

static void show_state_at_curses(WINDOW *const window) {
  ASSERT(window);
  waddnstr(window, (ISSET(AUTOINDENT) ? "I" : " "), 1);
  waddnstr(window, (openfile->mark ? "M" : " "), 1);
  waddnstr(window, (ISSET(BREAK_LONG_LINES) ? "L" : " "), 1);
  waddnstr(window, (recording ? "R" : " "), 1);
  waddnstr(window, (ISSET(SOFTWRAP) ? "S" : " "), 1);
}

_UNUSED static inline int mvwaddnstr_curses(WINDOW *const window, int row, int column, const char *const restrict string, int len) {
  return mvwaddnstr(window, row, column, string, len);
}

static void place_the_cursor_for_internal(openfilestruct *const file, Ulong *const out_column, int total_cols) {
  ASSERT(file);
  ASSERT(out_column);
  long row = 0;
  Ulong column = xplustabs_for(file);
  linestruct *line;
  Ulong leftedge;
  if (ISSET(SOFTWRAP)) {
    line = file->filetop;
    row -= chunk_for(file->firstcolumn, file->edittop, total_cols);
    /* Calculate how meny rows from edittop the current line is. */
    while (line && line != file->edittop) {
      row += (1 + extra_chunks_in(line, total_cols));
      CLIST_ADV_NEXT(line);
    }
    /* Add the number of wraps in the current line before the cursor. */
    row    += get_chunk_and_edge(column, file->current, &leftedge, total_cols);
    column -= leftedge;
  }
  else {
    row     = (file->current->lineno - file->edittop->lineno);
    column -= get_page_start(column, total_cols);
  }
  file->cursor_row = row;
  *out_column = column;
}

/* Draw a `scroll bar` on the righthand side of the edit window. */
static void draw_scrollbar_curses(void) {
  int fromline     = (openfile->edittop->lineno - 1);
  int totallines   = openfile->filebot->lineno;
  int coveredlines = editwinrows;
  linestruct *line;
  int lowest, highest, extras;
  if (ISSET(SOFTWRAP)) {
    line = openfile->edittop;
    extras = (extra_chunks_in(line, editwincols) - chunk_for(openfile->firstcolumn, line, editwincols));
    while ((line->lineno + extras) < (fromline + editwinrows) && line->next) {
      line = line->next;
      extras += extra_chunks_in(line, editwincols);
    }
    coveredlines = (line->lineno - fromline);
  }
  lowest  = ((fromline * editwinrows) / totallines);
  highest = (lowest + (editwinrows * coveredlines) / totallines);
  if (editwinrows > totallines && !ISSET(SOFTWRAP)) {
    highest = editwinrows;
  }
  for (int row=0; row<editwinrows; ++row) {
    if (!ISSET(NO_NCURSES)) {
      bardata[row] = (' ' | interface_color_pair[SCROLL_BAR] | ((row < lowest || row > highest) ? A_NORMAL : A_REVERSE));
      mvwaddch(midwin, row, (COLS - 1), bardata[row]);
    }
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


bool get_has_more(void) {
  return has_more;
}

/* Get the column number after leftedge where we can break the given linedata,
 * and return it.  (This will always be at most `total_cols` after leftedge)
 * When kickoff is TRUE, start at the beginning of the linedata; otherwise,
 * continue from where the previous call left off.  Set end_of_line to TRUE
 * when end-of-line is reached while searching for a possible breakpoint. */
Ulong get_softwrap_breakpoint(const char *linedata, Ulong leftedge, bool *kickoff, bool *end_of_line, int total_cols) {
  /* Pointer at the current character in this line's data. */
  static const char *text;
  /* Column position that corresponds to the above pointer. */
  static Ulong column;
  /* The place at or before which text must be broken. */
  Ulong rightside = (leftedge + total_cols);
  /* The column where text can be broken, when there's no better. */
  Ulong breaking_col = rightside;
  /* The column position of the last seen whitespace character. */
  Ulong last_blank_col = 0;
  /* A pointer to the last seen whitespace character in text. */
  const char *farthest_blank = NULL;
  /* Initialize the static variables when it's another line. */
  if (*kickoff) {
    text = linedata;
    column = 0;
    *kickoff = FALSE;
  }
  /* First find the place in text where the current chunk starts. */
  while (*text && column < leftedge) {
    text += advance_over(text, &column);
  }
  /* Now find the place in text where this chunk should end. */
  while (*text && column <= rightside) {
    /* When breaking at blanks, do it *before* the target column. */
    if (ISSET(AT_BLANKS) && is_blank_char(text) && column < rightside) {
      farthest_blank = text;
      last_blank_col = column;
    }
    breaking_col = ((*text == '\t') ? rightside : column);
    text += advance_over(text, &column);
  }
  /* If we didn't overshoot the limit, we've found a breaking point;
   * and we've reached EOL if we didn't even *reach* the limit. */
  if (column <= rightside) {
    *end_of_line = (column < rightside);
    return column;
  }
  /* If we're softwrapping at blanks and we found at least one blank, break
   * after that blank -- if it doesn't overshoot the screen's edge. */
  if (farthest_blank) {
    Ulong aftertheblank = last_blank_col;
    Ulong onestep = advance_over(farthest_blank, &aftertheblank);
    if (aftertheblank <= rightside) {
      text = (farthest_blank + onestep);
      column = aftertheblank;
      return aftertheblank;
    }
    /* If it's a tab that overshoots, break at the screen's edge. */
    if (*farthest_blank == '\t') {
      breaking_col = rightside;
    }
  }
  /* Otherwise, break at the last character that doesn't overshoot. */
  return ((total_cols > 1) ? breaking_col : (column - 1));
}

/* Return the row number of the softwrapped chunk in the given line that the given column is on, relative
 * to the first row (zero-based).  If leftedge isn't NULL, return in it the leftmost column of the chunk. */
Ulong get_chunk_and_edge(Ulong column, linestruct *line, Ulong *leftedge, int total_cols) {
  Ulong end_col;
  Ulong current_chunk = 0;
  Ulong start_col     = 0;
  bool end_of_line    = FALSE;
  bool kickoff        = TRUE;
  while (TRUE) {
    end_col = get_softwrap_breakpoint(line->data, start_col, &kickoff, &end_of_line, total_cols);
    /* When the column is in range or we reached end-of-line, we're done. */
    if (end_of_line || (start_col <= column && column < end_col)) {
      if (leftedge) {
        *leftedge = start_col;
      }
      return current_chunk;
    }
    start_col = end_col;
    ++current_chunk;
  }
}

/* Return how many extra rows the given line needs when softwrapping. */
Ulong extra_chunks_in(linestruct *const line, int total_cols) {
  return get_chunk_and_edge((Ulong)-1, line, NULL, total_cols);
}

/* Return the row of the softwrapped chunk of the given line that column is on, relative to the first row (zero-based). */
Ulong chunk_for(Ulong column, linestruct *const line, int total_cols) {
  return get_chunk_and_edge(column, line, NULL, total_cols);
}

/* Return the leftmost column of the softwrapped chunk of the given line that the given column is on. */
Ulong leftedge_for(Ulong column, linestruct *const line, int total_cols) {
  Ulong leftedge;
  get_chunk_and_edge(column, line, &leftedge, total_cols);
  return leftedge;
}

/* Try to move up nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move up, which will be zero if we completely succeeded. */
int go_back_chunks_for(openfilestruct *const file, int nrows, linestruct **const line, Ulong *const leftedge, int total_cols) {
  int i;
  Ulong chunk;
  if (ISSET(SOFTWRAP)) {
    /* Recede through the requested number of chunks. */
    for (i=nrows; i>0; --i) {
      chunk       = chunk_for((*leftedge), (*line), total_cols);
      (*leftedge) = 0;
      if ((int)chunk >= i) {
        return go_forward_chunks_for(file, (chunk - i), line, leftedge, total_cols);
      }
      else if ((*line) == file->filetop) {
        break;
      }
      i -= chunk;
      CLIST_ADV_PREV(*line);
      (*leftedge) = HIGHEST_POSITIVE;
    }
    if ((*leftedge) == HIGHEST_POSITIVE) {
      (*leftedge) = leftedge_for((*leftedge), (*line), total_cols);
    }
  }
  else {
    for (i=nrows; i>0 && (*line)->prev; --i) {
      CLIST_ADV_PREV(*line);
    }
  }
  return i;
}

/* Try to move up nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move up, which will be zero if we completely succeeded. */
int go_back_chunks(int nrows, linestruct **const line, Ulong *const leftedge) {
  if (ISSET(USING_GUI)) {
    return go_back_chunks_for(openeditor->openfile, nrows, line, leftedge, openeditor->cols);
  }
  else {
    return go_back_chunks_for(openfile, nrows, line, leftedge, editwincols);
  }
}

/* Try to move down nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move down, which will be zero if we completely succeeded. */
int go_forward_chunks_for(openfilestruct *const file, int nrows, linestruct **const line, Ulong *const leftedge, int total_cols) {
  int i;
  Ulong current_leftedge;
  bool kickoff;
  bool eol;
  if (ISSET(SOFTWRAP)) {
    current_leftedge = (*leftedge);
    kickoff = TRUE;
    /* Advance thrue the requested number of chunks. */
    for (i=nrows; i>0; --i) {
      eol = FALSE;
      current_leftedge = get_softwrap_breakpoint((*line)->data, current_leftedge, &kickoff, &eol, total_cols);
      if (!eol) {
        continue;
      }
      else if ((*line) == file->filebot) {
        break;
      }
      CLIST_ADV_NEXT(*line);
      current_leftedge = 0;
      kickoff = TRUE;
    }
    /* Only change leftedge when we actually could move. */
    if (i < nrows) {
      (*leftedge) = current_leftedge;
    }
  }
  else {
    for (i=nrows; i>0 && (*line)->next; --i) {
      CLIST_ADV_NEXT(*line);
    }
  }
  return i;
}

/* Try to move down nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move down, which will be zero if we completely succeeded. */
int go_forward_chunks(int nrows, linestruct **const line, Ulong *const leftedge) {
  if (ISSET(USING_GUI)) { 
    return go_forward_chunks_for(openeditor->openfile, nrows, line, leftedge, openeditor->cols);
  }
  else {
    return go_forward_chunks_for(openfile, nrows, line, leftedge, editwincols);
  }
}

void ensure_firstcolumn_is_aligned_for(openfilestruct *const file, int total_cols) {
  ASSERT(file);
  if (ISSET(SOFTWRAP)) {
    file->firstcolumn = leftedge_for(file->firstcolumn, file->edittop, total_cols);
  }
  else {
    file->firstcolumn = 0;
  }
  /* If smooth scrolling is on, make sure the viewport doesn't center. */
  focusing = FALSE;
}

/* Ensure that firstcolumn is at the starting column of the softwrapped chunk
 * it's on.  We need to do this when the number of columns of the edit window
 * has changed, because then the width of softwrapped chunks has changed. */
void ensure_firstcolumn_is_aligned(void) {
  if (ISSET(USING_GUI)) {
    ensure_firstcolumn_is_aligned_for(openeditor->openfile, openeditor->cols);
  }
  else {
    ensure_firstcolumn_is_aligned_for(openfile, editwincols);
  }
}

/* Convert text into a string that can be displayed on screen.
 * The caller wants to display text starting with the given column, and
 * extending for at most span columns. The returned string is dynamically
 * allocated, and should be freed. If isdata is TRUE, the caller might put "<"
 * at the beginning or ">" at the end of the line if it's too long. If isprompt
 * is TRUE, the caller might put ">" at the end of the line if it's too long. */
char *display_string(const char *text, Ulong column, Ulong span, bool isdata, bool isprompt) {
  /* The beginning of the text, to later determine the covered part. */
  const char *origin = text;
  /* The index of the first character that the caller wishes to show. */
  Ulong start_x = actual_x(text, column);
  /* The actual column where that first character starts. */
  Ulong start_col = wideness(text, start_x);
  /* The number of zero-width characters for which to reserve space. */
  Ulong stowaways = 20;
  /* The amount of memory to reserve for the displayable string. */
  Ulong allocsize = (((ISSET(USING_GUI) ? (span + 20) : COLS) + stowaways) * MAXCHARLEN + 1);
  /* The displayable string we will return. */
  char *converted = xmalloc(allocsize);
  /* Current position in converted. */
  Ulong index = 0;
  /* The column number just beyond the last shown character. */
  Ulong beyond = (column + span);
  text += start_x;
  if (span > HIGHEST_POSITIVE) {
    statusline_all(ALERT, "Span has underflowed -- please report a bug");
    converted[0] = '\0';
    return converted;
  }
  /* If the first character starts before the left edge, or would be overwritten by a "<" token, then show placeholders instead. */
  if ((start_col < column || (start_col > 0 && isdata && !ISSET(SOFTWRAP))) && *text && *text != '\t') {
    if (is_cntrl_char(text)) {
      if (start_col < column) {
        converted[index++] = control_mbrep(text, isdata);
        ++column;
        text += char_length(text);
      }
    }
    else if (is_doublewidth(text)) {
      if (start_col == column) {
        converted[index++] = ' ';
        ++column;
      }
      /* Display the right half of a two-column character as ']'. */
      converted[index++] = ']';
      ++column;
      text += char_length(text);
    }
  }
  while (*text && (column < beyond || ZEROWIDTH_CHAR)) {
    /* A plain printable ASCII character is one byte, one column. */
    if ((*text > 0x20 && *text != DEL_CODE) || ISO8859_CHAR) {
      converted[index++] = *(text++);
      ++column;
      continue;
    }
    /* Show a space as a visible character, or as a space. */
    if (*text == ' ') {
      if (ISSET(WHITESPACE_DISPLAY)) {
        for (int i=whitelen[0]; i<(whitelen[0] + whitelen[1]);) {
          converted[index++] = whitespace[i++];
        }
      }
      else {
        converted[index++] = ' ';
      }
      ++column;
      ++text;
      continue;
    }
    /* Show a tab as a visible character plus spaces, or as just spaces. */
    if (*text == '\t') {
      if (ISSET(WHITESPACE_DISPLAY) && (index > 0 || !isdata || !ISSET(SOFTWRAP) || column % tabsize == 0 || column == start_col)) {
        for (int i=0; i<whitelen[0];) {
          converted[index++] = whitespace[i++];
        }
      }
      else {
        converted[index++] = ' ';
      }
      ++column;
      /* Fill the tab up with the required number of spaces. */
      while ((column % tabsize) != 0 && column < beyond) {
        converted[index++] = ' ';
        ++column;
      }
      ++text;
      continue;
    }
    /* Represent a control character with a leading caret. */
    if (is_cntrl_char(text)) {
      converted[index++] = '^';
      converted[index++] = control_mbrep(text, isdata);
      text += char_length(text);
      column += 2;
      continue;
    }
    int charlength, charwidth;
    wchar wc;
    /* Convert a multibyte character to a single code. */
    charlength = mbtowide(&wc, text);
    /* Represent an invalid character with the Replacement Character. */
    if (charlength < 0) {
      converted[index++] = '\xEF';
      converted[index++] = '\xBF';
      converted[index++] = '\xBD';
      ++text;
      ++column;
      continue;
    }
    /* Determine whether the character takes zero, one, or two columns. */
    charwidth = wcwidth(wc);
    /* Watch the number of zero-widths, to keep ample memory reserved. */
    if (charwidth == 0 && --stowaways == 0) {
      stowaways = 40;
      allocsize += stowaways * MAXCHARLEN;
      converted = xrealloc(converted, allocsize);
    }
    /* On a Linux console, skip zero-width characters, as it would show them WITH a width, thus messing up the display.  See bug #52954. */
    if (on_a_vt && charwidth == 0) {
      text += charlength;
      continue;
    }
    /* For any valid character, just copy its bytes. */
    for (; charlength > 0; --charlength) {
      converted[index++] = *(text++);
    }
    /* If the codepoint is unassigned, assume a width of one. */
    column += (charwidth < 0 ? 1 : charwidth);
  }
  /* If there is more text than can be shown, make room for the ">". */
  if (column > beyond || (*text && (isprompt || (isdata && !ISSET(SOFTWRAP))))) {
    do {
      index = step_left(converted, index);
    } while (is_zerowidth(converted + index));
    /* Display the left half of a two-column character as '['. */
    if (is_doublewidth(converted + index)) {
      converted[index++] = '[';
    }
    has_more = TRUE;
  }
  else {
    has_more = FALSE;
  }
  is_shorter = (column < beyond);
  /* Null-terminate the converted string. */
  converted[index] = '\0';
  /* Remember what part of the original text is covered by converted. */
  from_x = start_x;
  till_x = (text - origin);
  return converted;
}

/* Check whether the mark is on, or whether old_column and new_column are on different "pages"
 * (in softwrap mode, only the former applies), which means that the relevant line needs to be redrawn. */
bool line_needs_update_for(openfilestruct *const file, Ulong old_column, Ulong new_column, int total_cols) {
  if (file->mark) {
    return TRUE;
  }
  else {
    return (get_page_start(old_column, total_cols) != get_page_start(new_column, total_cols));
  }
}

/* Check whether the mark is on, or whether old_column and new_column are on different "pages"
 * (in softwrap mode, only the former applies), which means that the relevant line needs to be redrawn. */
bool line_needs_update(Ulong old_column, Ulong new_column) {
  if (ISSET(USING_GUI)) {
    return line_needs_update_for(openeditor->openfile, old_column, new_column, openeditor->cols);
  }
  else {
    return line_needs_update_for(openfile, old_column, new_column, editwincols);
  }
}

/* Return 'TRUE' if there are fewer than a screen's worth of lines between the line at line number
 * was_lineno (and column was_leftedge, if we're in softwrap mode) and the line at current[current_x]. */
bool less_than_a_screenful_for(openfilestruct *const file, Ulong was_lineno, Ulong was_leftedge, int total_rows, int total_cols) {
  int rows_left;
  Ulong leftedge;
  linestruct *line;
  if (ISSET(SOFTWRAP)) {
    line = file->current;
    leftedge  = leftedge_for(xplustabs_for(file), file->current, total_cols);
    rows_left = go_back_chunks_for(file, (total_rows - 1), &line, &leftedge, total_cols);
    return (rows_left > 0 || line->lineno < (long)was_lineno || (line->lineno == (long)was_lineno && leftedge <= was_leftedge));
  }
  else {
    return ((int)(file->current->lineno - was_lineno) < total_rows);
  }
}

/* Return 'TRUE' if there are fewer than a screen's worth of lines between the line at line number was_lineno
 * (and column was_leftedge, if we're in softwrap mode) and the line at `openfile->current[openfile->current_x]`. */
bool less_than_a_screenful(Ulong was_lineno, Ulong was_leftedge) {
  if (ISSET(USING_GUI)) {
    return less_than_a_screenful_for(openeditor->openfile, was_lineno, was_leftedge, openeditor->rows, openeditor->cols);
  }
  else {
    return less_than_a_screenful_for(openfile, was_lineno, was_leftedge, editwinrows, editwincols);
  }
}

/* When in softwrap mode, and the given column is on or after the breakpoint of a softwrapped
 * chunk, shift it back to the last column before the breakpoint.  The given column is relative
 * to the given leftedge in current.  The returned column is relative to the start of the text. */
Ulong actual_last_column_for(openfilestruct *const file, Ulong leftedge, Ulong column, int total_cols) {
  ASSERT(file);
  bool  kickoff, last_chunk;
  Ulong end_col;
  if (ISSET(SOFTWRAP)) {
    kickoff    = TRUE;
    last_chunk = FALSE;
    end_col    = (get_softwrap_breakpoint(file->current->data, leftedge, &kickoff, &last_chunk, total_cols) - leftedge);
    /* If we're not on the last chunk, we're one column past the end of the row.  Shifting back one column
     * might put us in the middle of a multi-column character, but 'actual_x()' will fix that later. */
    if (!last_chunk) {
      --end_col;
    }
    if (column > end_col) {
      column = end_col;
    }
  }
  return (leftedge + column);
}

/* When in softwrap mode, and the given column is on or after the breakpoint of a softwrapped
 * chunk, shift it back to the last column before the breakpoint.  The given column is relative
 * to the given leftedge in current.  The returned column is relative to the start of the text. */
Ulong actual_last_column(Ulong leftedge, Ulong column) {
  if (ISSET(USING_GUI)) {
    return actual_last_column_for(openeditor->openfile, leftedge, column, openeditor->cols);
  }
  else {
    return actual_last_column_for(openfile, leftedge, column, editwincols);
  }
}

/* Return TRUE if current[current_x] is before the viewport. */
bool current_is_above_screen_for(openfilestruct *const file) {
  ASSERT(file);
  if (ISSET(SOFTWRAP)) {
    return (file->current->lineno < file->edittop->lineno || (file->current->lineno == file->edittop->lineno && xplustabs_for(file) < file->firstcolumn));
  }
  return (file->current->lineno < file->edittop->lineno);
}

/* Return TRUE if current[current_x] is before the viewport. */
bool current_is_above_screen(void) {
  if (ISSET(USING_GUI)) {
    return current_is_above_screen_for(openeditor->openfile);
  }
  else {
    return current_is_above_screen_for(openfile);
  }
}

/* Return `TRUE` if `file->current[file->current_x]` is beyond the viewport. */
bool current_is_below_screen_for(openfilestruct *const file, int total_rows, int total_cols) {
  ASSERT(file);
  linestruct *line;
  Ulong leftedge;
  if (ISSET(SOFTWRAP)) {
    line     = file->edittop;
    leftedge = file->firstcolumn;
    /* If current[current_x] is more than a screen's worth of lines after edittop at column firstcolumn, it's below the screen. */
    return (go_forward_chunks_for(file, (total_rows - 1 - SHIM), &line, &leftedge, total_cols) == 0 && (line->lineno < file->current->lineno
     || (line->lineno == file->current->lineno && leftedge < leftedge_for(xplustabs_for(file), file->current, total_cols))));
  }
  return (file->current->lineno >= (file->edittop->lineno + total_rows - SHIM));
}

/* Return `TRUE` if `openfile->current[openfile->current_x]` is beyond the viewport. */
bool current_is_below_screen(void) {
  if (ISSET(USING_GUI)) {
    return current_is_below_screen_for(openeditor->openfile, openeditor->rows, openeditor->cols);
  }
  else {
    return current_is_below_screen_for(openfile, editwinrows, editwincols);
  }
}

/* Return TRUE if current[current_x] is outside the viewport. */
bool current_is_offscreen_for(openfilestruct *const file, int total_rows, int total_cols) {
  ASSERT(file);
  return (current_is_above_screen_for(file) || current_is_below_screen_for(file, total_rows, total_cols));
}

/* Return TRUE if current[current_x] is outside the viewport. */
bool current_is_offscreen(void) {
  if (ISSET(USING_GUI)) {
    return current_is_offscreen_for(openeditor->openfile, openeditor->rows, openeditor->cols);
  }
  else {
    return current_is_offscreen_for(openfile, editwinrows, editwincols);
  }
}

/* Move edittop so that current is on the screen.  manner says how:  STATIONARY means that the cursor
 * should stay on the same screen row, CENTERING means that current should end up in the middle of the
 * screen, and FLOWING means that it should scroll no more than needed to bring current into view. */
void adjust_viewport_for(openfilestruct *const file, update_type manner, int total_rows, int total_cols) {
  ASSERT(file);
  int goal = 0;
  if (manner == STATIONARY) {
    goal = file->cursor_row;
  }
  else if (manner == CENTERING) {
    goal = (total_rows / 2);
  }
  else if (!current_is_above_screen_for(file)) {
    goal = (total_rows - 1 - SHIM);
  }
  file->edittop = file->current;
  if (ISSET(SOFTWRAP)) {
    file->firstcolumn = leftedge_for(xplustabs_for(file), file->current, total_cols);
  }
  /* Move edittop back goal rows, starting at current[current_x]. */
  go_back_chunks_for(file, goal, &file->edittop, &file->firstcolumn, total_cols);
}

/* Move edittop so that current is on the screen.  manner says how:  STATIONARY means that the cursor
 * should stay on the same screen row, CENTERING means that current should end up in the middle of the
 * screen, and FLOWING means that it should scroll no more than needed to bring current into view. */
void adjust_viewport(update_type manner) {
  if (ISSET(USING_GUI)) {
    adjust_viewport_for(openeditor->openfile, manner, openeditor->rows, openeditor->cols);
  }
  else {
    adjust_viewport_for(openfile, manner, editwinrows, editwincols);
  }
}

/* Redetermine `cursor_row` from the position of current relative to edittop, and put the cursor in the edit window at (cursor_row, "current_x"). */
void place_the_cursor_for(openfilestruct *const file) {
  ASSERT(file);
  Ulong column;
  if (ISSET(USING_GUI)) {
    place_the_cursor_for_internal(file, &column, editor_from_file(file)->cols);
  }
  else if (!ISSET(NO_NCURSES)) {
    place_the_cursor_curses_for(openfile);
  }
}

/* Redetermine `cursor_row` from the position of current relative to edittop, and put the cursor in the edit window at (cursor_row, "current_x"). */
void place_the_cursor(void) {
  Ulong column;
  if (ISSET(USING_GUI)) {
    place_the_cursor_for_internal(openeditor->openfile, &column, openeditor->cols);
  }
  else if (!ISSET(NO_NCURSES)) {
    place_the_cursor_curses_for(openfile);
  }
}

/* Ensure that the status bar will be wiped upon the next keystroke. */
void set_blankdelay_to_one(void) {
  countdown = 1;
}

/* Return the number of key codes waiting in the keystroke buffer. */
Ulong waiting_keycodes(void) {
  return waiting_codes;
}

/* Scroll the edit window one row in the given direction, and draw the relevant content on the resultant blank row. */
void edit_scroll_for(openfilestruct *const file, bool direction) {
  ASSERT(file);
  linestruct *line;
  Ulong leftedge;
  int nrows = 1;
  /* Move the top line of the edit window one row up or down. */
  if (direction == BACKWARD) {
    go_back_chunks(1, &file->edittop, &file->firstcolumn);
  }
  else {
    go_forward_chunks(1, &file->edittop, &file->firstcolumn);
  }
  /* If using the gui, return early. */
  if (ISSET(USING_GUI)) {
    return;
  }
  else if (!ISSET(NO_NCURSES)) {
    /* Actually scroll the text of the edit window one row up or down. */
    scrollok(midwin, TRUE);
    wscrl(midwin, ((direction == BACKWARD) ? -1 : 1));
    scrollok(midwin, FALSE);
  }
  /* If we're not on the first "page" (when not softwrapping), or the mark
   * is on, the row next to the scrolled region needs to be redrawn too. */
  if (line_needs_update_for(file, file->placewewant, 0, editwincols) && nrows < editwinrows) {
    ++nrows;
  }
  /* If we scrolled backward, the top row needs to be redrawn. */
  line     = file->edittop;
  leftedge = file->firstcolumn;
  /* If we scrolled forward, the bottom row needs to be redrawn. */
  if (direction == FORWARD) {
    go_forward_chunks_for(file, (editwinrows - nrows), &line, &leftedge, editwincols);
  }
  if (sidebar) {
    draw_scrollbar_curses();
  }
  if (ISSET(SOFTWRAP)) {
    /* Compensate for the earlier chunks of a softwrapped line. */
    nrows += chunk_for(leftedge, line, editwincols);
    /* Don't compensate for the chunks that are offscreen. */
    if (line == file->edittop) {
      nrows -= chunk_for(file->firstcolumn, line, editwincols);
    }
  }
  /* Draw new content on the blank row (and on the bordering row too when it was deemed necessary). */
  while (nrows > 0 && line) {
    nrows -= update_line_curses(line, ((line == file->current) ? file->current_x : 0));
    line = line->next;
  }
}

/* Scroll the edit window one row in the given direction, and draw the relevant content on the resultant blank row. */
void edit_scroll(bool direction) {
  edit_scroll_for(CONTEXT_OPENFILE, direction);
}

/* Update any lines between old_current and current that need to be updated.  Use this if we've moved without changing any text. */
void edit_redraw(linestruct *const old_current, update_type manner) {
  ASSERT(old_current);
  Ulong was_pww = openfile->placewewant;
  openfile->placewewant = xplustabs();
  /* If the current line is offscreen, scroll until it's onscreen. */
  if (current_is_offscreen()) {
    adjust_viewport(ISSET(JUMPY_SCROLLING) ? CENTERING : manner);
    refresh_needed = TRUE;
    return;
  }
  /* Return early if running in gui mode. */
  if (ISSET(USING_GUI)) {
    return;
  }
  /* If the mark is on, update all lines between old_current and current. */
  if (openfile->mark) {
    linestruct *line = old_current;
    while (line != openfile->current) {
      update_line_curses(line, 0);
      line = ((line->lineno > openfile->current->lineno) ? line->prev : line->next);
    }
  }
  else {
    /* Otherwise, update old_current only if it differs from current and was horizontally scrolled. */
    if (old_current != openfile->current && get_page_start(was_pww, editwincols) > 0) {
      update_line_curses(old_current, 0);
    }
  }
  /* Update current if the mark is on or it has changed "page", or if it differs from old_current and needs to be horizontally scrolled. */
  if (line_needs_update(was_pww, openfile->placewewant) || (old_current != openfile->current && get_page_start(openfile->placewewant, editwincols) > 0)) {
    update_line_curses(openfile->current, openfile->current_x);
  }
}

/* Refresh the screen without changing the position of lines.  Use this if we've moved and changed text. */
void edit_refresh(void) {
  linestruct *line;
  int row = 0;
  /* If the current line is out of view, get it back on screen. */
  if (current_is_offscreen()) {
    adjust_viewport((focusing || ISSET(JUMPY_SCROLLING)) ? CENTERING : FLOWING);
  }
  /* When needed and useful, initialize the colors for the current syntax. */
  if (!ISSET(NO_NCURSES) && openfile->syntax && !have_palette && !ISSET(NO_SYNTAX) && has_colors()) {
    prepare_palette();
  }
  /* When the line above the viewport does not have multidata, recalculate all. */
  recook |= (ISSET(SOFTWRAP) && openfile->edittop->prev && !openfile->edittop->prev->multidata);
  if (recook) {
    precalc_multicolorinfo();
    perturbed = FALSE;
    recook    = FALSE;
  }
  /* Only draw sidebar when approptiet, i.e: when there is more then one ROWS worth of data. */
  if (sidebar && openfile->filebot->lineno > editwinrows) {
    draw_scrollbar_curses();
  }
  line = openfile->edittop;
  while (row < editwinrows && line) {
    row += update_line_curses(line, ((line == openfile->current) ? openfile->current_x : 0));
    line = line->next;
  }
  while (row < editwinrows) {
    blank_row_curses(midwin, row);
    /* If full linenumber bar is enabled, then draw it. */
    if (config->linenumber.fullverticalbar) {
      mvwaddchcolor(midwin, row, (margin - 1), ACS_VLINE, config->linenumber.barcolor);
    }
    /* Only draw sidebar when on and when the openfile is longer then editwin rows. */
    if (sidebar && openfile->filebot->lineno > editwinrows) {
      mvwaddch(midwin, row, (COLS - 1), bardata[row]);
    }
    ++row;
  }
  place_the_cursor();
  wnoutrefresh(midwin);
  refresh_needed = FALSE;
}

/* If path is NULL, we're in normal editing mode, so display the current
 * version of nano, the current filename, and whether the current file
 * has been modified on the title bar.  If path isn't NULL, we're either
 * in the file browser or the help viewer, so show either the current
 * directory or the title of help text, that is: whatever is in path. */
void titlebar(const char *path) {
  /* The width of the diffrent title-bar elements, in columns. */
  Ulong verlen;
  Ulong prefixlen;
  Ulong pathlen;
  Ulong statelen;
  /* The width of the `Modified` would take up. */
  Ulong pluglen = 0;
  /* The position at witch the center part of the title-bar starts. */
  Ulong offset = 0;
  /* What is shown in the top left corner. */
  const char *upperleft = "";
  /* What is shown before the path -- `DIR:` or nothing. */
  const char *prefix = "";
  /* The state of the current buffer -- `Modified`, `View` or ``. */
  const char *state = "";
  /* The presentable for of the pathname. */
  char *caption;
  /* The buffer sequence number plus the total buffer count. */
  char *ranking = NULL;
  /* If the screen is to small, there is no title bar. */
  if (!topwin) {
    return;
  }
  wattron(topwin, interface_color_pair[TITLE_BAR]);
  blank_titlebar_curses();
  as_an_at = FALSE;
  /**
   * Do as Pico:
   *   if there is not enough width available for all items,
   *   first sacrifice the version string, then eat up the side spaces,
   *   then sacrifice the prefix, and only then start dottifying.
   */
  /* Figure out the path, prefix and state strings. */
  if (currmenu == MLINTER) {
    prefix = _("Linting --");
    path = openfile->filename;
  }
  else {
    if (!inhelp && path) {
      prefix = _("DIR:");
    }
    else {
      if (!inhelp) {
        /* If there are/were multiple buffers, show witch out of how meny. */
        if (more_than_one) {
          ranking = xmalloc(24);
          sprintf(ranking, "[%i/%i]", buffer_number(openfile), buffer_number(startfile->prev));
          upperleft = ranking;
        }
        else {
          upperleft = BRANDING;
        }
        if (!*openfile->filename) {
          path = _("New buffer");
        }
        else {
          path = openfile->filename;
        }
        if (ISSET(VIEW_MODE)) {
          state = _("View");
        }
        else if (ISSET(STATEFLAGS)) {
          state = _("+.xxxxx");
        }
        else if (ISSET(RESTRICTED)) {
          state = _("Restricted");
        }
        else {
          pluglen = (breadth(_("Modified")) + 1);
        }
      }
    }
  }
  /* Determine the width of the four elements, including their padding. */
  verlen    = (breadth(upperleft) + 3);
  prefixlen = breadth(prefix);
  if (prefixlen > 0) {
    ++prefixlen;
  }
  pathlen  = breadth(path);
  statelen = (breadth(state) + 2);
  if (statelen > 2) {
    ++pathlen;
  }
  /* Only print the version message when there is room for it. */
  if ((int)(verlen + prefixlen + pathlen + pluglen + statelen) <= COLS) {
    mvwaddstr(topwin, 0, 2, upperleft);
  }
  else {
    verlen = 2;
    /* If things don't fit yet, give up the placeholder. */
    if ((int)(verlen + prefixlen + pathlen + pluglen + statelen) > COLS) {
      pluglen = 0;
    }
    /* If things still don't fit, give up the side spaces. */
    if ((int)(verlen + prefixlen + pathlen + pluglen + statelen) > COLS) {
      verlen    = 0;
      statelen -= 2;
    }
  }
  free(ranking);
  /* If we have side spaces left, center the path name. */
  if (verlen > 0) {
    offset = (verlen + (COLS - (verlen + pluglen + statelen) - (prefixlen + pathlen)) / 2);
  }
  /* Only print the prefix when there is room for it. */
  if ((int)(verlen + prefixlen + pathlen + pluglen + statelen) <= COLS) {
    mvwaddstr(topwin, 0, offset, prefix);
    if (prefixlen > 0) {
      waddnstr(topwin, S__LEN(" "));
    }
  }
  else {
    wmove(topwin, 0, offset);
  }
  /* Print the full path if there's room, otherwise, dottify it. */
  if ((int)(pathlen + pluglen + statelen) <= COLS) {
    caption = display_string(path, 0, pathlen, FALSE, FALSE);
    waddstr(topwin, caption);
    free(caption);
  }
  else if ((int)(5 + statelen) <= COLS) {
    waddnstr(topwin, S__LEN("..."));
    caption = display_string(path, (3 + pathlen - COLS + statelen), (COLS - statelen), FALSE, FALSE);
    waddstr(topwin, caption);
    free(caption);
  }
  /* When requested, show on the title-bar, the state of three options and the state of the mark and whether a macro is beeing recorded.  */
  if (*state && ISSET(STATEFLAGS) && !ISSET(VIEW_MODE)) {
    if (openfile->modified && COLS > 1) {
      waddnstr(topwin, S__LEN(" *"));
    }
    if ((int)statelen < COLS) {
      wmove(topwin, 0, (COLS + 2 - statelen));
      show_state_at_curses(topwin);
    }
  }
  else {
    /* If there's room, right-align the state word, otherwise, clip it. */
    if (statelen > 0 && (int)statelen <= COLS) {
      mvwaddstr(topwin, 0, (COLS - statelen), state);
    }
    else {
      mvwaddnstr(topwin, 0, 0, state, actual_x(state, COLS));
    }
  }
  wattroff(topwin, interface_color_pair[TITLE_BAR]);
  wrefresh(topwin);
}

/* Blank all lines of the middle portion of the screen (the edit window). */
void blank_edit(void) {
  /* Only perform any action when in ncurses mode for now. */
  if (!ISSET(NO_NCURSES)) {
    for (int row=0; row<editwinrows; ++row) {
      blank_row_curses(midwin, row);
    }
  }
}

/* ----------------------------- Curses ----------------------------- */

void blank_row_curses(WINDOW *const window, int row) {
  ASSERT(window);
  ASSERT(row >= 0);
  wmove(window, row, 0);
  wclrtoeol(window);
}

/* Blank the first line of the top portion of the screen.  Using ncurses. */
void blank_titlebar_curses(void) {
  mvwprintw(topwin, 0, 0, "%*s", COLS, " ");
}

void blank_statusbar_curses(void) {
  blank_row_curses(footwin, 0);
}

/* Blank out the two help lines (when they are present).  Ncurses version. */
void blank_bottombars_curses(void) {
  if (!ISSET(NO_HELP) && LINES > 5) {
    blank_row_curses(footwin, 1);
    blank_row_curses(footwin, 2);
  }
}

/* Display the given message on the status bar, but only if its importance is higher than that of a message that is already there.  Ncurses version. */
void statusline_curses_va(message_type type, const char *const restrict format, va_list ap) {
  static Ulong start_col = 0;
  bool showed_whitespace = ISSET(WHITESPACE_DISPLAY);
  char *compound;
  char *message;
  bool bracketed;
  int color;
  va_list copy;
  if (type >= AHEM) {
    waiting_codes = 0;
  }
  if (type < lastmessage && lastmessage > NOTICE) {
    return;
  }
  /* Construct the message from the arguments. */
  compound = xmalloc(MAXCHARLEN * COLS + 1);
  va_copy(copy, ap);
  vsnprintf(compound, (MAXCHARLEN * COLS + 1), format, copy);
  va_end(copy);
  if (isendwin()) {
    writeferr("%s\n", compound);
    free(compound);
  }
  else {
    if (!we_are_running && type == ALERT && openfile && !openfile->fmt && !openfile->errormessage && !CLIST_SINGLE(openfile)) {
      openfile->errormessage = copy_of(compound);
    }
    /* On a one row terminal, ensure any changes in the edit window are written first, to prevent them from overwriting the message. */
    if (LINES == 1 && type < INFO) {
      wnoutrefresh(midwin);
    }
    /* If there are multiple alert messages, add trailng dot's first. */
    if (lastmessage == ALERT) {
      if (start_col > 4) {
        wmove(footwin, 0, (COLS + 2 - start_col));
        wattron(footwin, interface_color_pair[ERROR_MESSAGE]);
        waddnstr(footwin, S__LEN("..."));
        wattroff(footwin, interface_color_pair[ERROR_MESSAGE]);
        wnoutrefresh(footwin);
        start_col = 0;
        napms(100);
        beep();
      }
      free(compound);
      return;
    }
    else if (type > NOTICE) {
      if (type == ALERT) {
        beep();
      }
      color = ERROR_MESSAGE;
    }
    else if (type == NOTICE) {
      color = SELECTED_TEXT;
    }
    else {
      color = STATUS_BAR;
    }
    lastmessage = type;
    blank_statusbar_curses();
    UNSET(WHITESPACE_DISPLAY);
    message = display_string(compound, 0, COLS, FALSE, FALSE);
    if (showed_whitespace) {
      SET(WHITESPACE_DISPLAY);
    }
    start_col = ((COLS - breadth(message)) / 2);
    bracketed = (start_col > 1);
    wmove(footwin, 0, (bracketed ? (start_col - 2) : start_col));
    wattron(footwin, interface_color_pair[color]);
    if (bracketed) {
      waddnstr(footwin, S__LEN("[ "));
      waddstr(footwin, message);
      waddnstr(footwin, S__LEN(" ]"));
    }
    else {
      waddstr(footwin, message);
    }
    wattroff(footwin, interface_color_pair[color]);
    /* Tell `footwin` to refresh. */
    wrefresh(footwin);
    free(compound);
    free(message);
    /* When requested, wipe the statusbar after just one keystroke, otherwise wipe after 20. */
    countdown = (ISSET(QUICK_BLANK) ? 1 : 20);
  }
}

/* Display the given message on the status bar, but only if its importance is higher than that of a message that is already there.  Ncurses version. */
void statusline_curses(message_type type, const char *const restrict msg, ...) {
  va_list ap;
  va_start(ap, msg);
  statusline_curses_va(type, msg, ap);
  va_end(ap);
}

/* Display a normal message on the status bar, quietly.  Ncurses version. */
void statusbar_curses(const char *const restrict msg) {
  statusline_curses(HUSH, "%s", msg);
}

/* Draw a bar at the bottom with some minimal state information. */
void minibar_curses(void) {
  char *thename         = NULL;
  char *shortname;
  char *position;
  char *number_of_lines = NULL;
  char *ranking         = NULL;
  char *successor       = NULL;
  char *location        = xmalloc(44);
  char *hexadecimal     = xmalloc(9);
  Ulong namewidth;
  Ulong placewidth;
  Ulong count;
  Ulong tallywidth = 0;
  Ulong padding    = 2;
  wchar widecode;
  /* Draw the colored bar over the full width of the screen. */
  wattron(footwin, interface_color_pair[config->minibar_color]);
  mvwprintw(footwin, 0, 0, "%*s", COLS, " ");
  if (*openfile->filename) {
    as_an_at = FALSE;
    thename = display_string(openfile->filename, 0, COLS, FALSE, FALSE);
  }
  else {
    thename = copy_of(_("(nameless)"));
  }
  snprintf(location, 44, "%zi,%zi", openfile->current->lineno, (xplustabs() + 1));
  placewidth = strlen(location);
  namewidth  = breadth(thename);
  /* If the file name is relativly long drop the side spaces. */
  if ((int)(namewidth + 19) > COLS) {
    padding = 0;
  }
  /* Display the name of the current file (dottifying it if it doesn't fit), plus a star when the file has been modified. */
  if (COLS > 4) {
    if ((int)namewidth > (COLS - 2)) {
      shortname = display_string(thename, (namewidth - COLS + 5), (COLS - 5), FALSE, FALSE);
      wmove(footwin, 0, 0);
      waddnstr(footwin, S__LEN("..."));
      waddstr(footwin, shortname);
      free(shortname);
    }
    else {
      mvwaddstr(footwin, 0, padding, thename);
    }
    waddnstr(footwin, (openfile->modified ? " *": "  "), 2);
  }
  /* Right after reading or writing a file, display its number of lines.  Otherwise, when there are multiple buffers, display an [x/n] counter. */
  if (report_size && COLS > 35) {
    count           = (openfile->filebot->lineno - !*openfile->filebot->data);
    number_of_lines = xmalloc(49);
    if (openfile->fmt == NIX_FILE || openfile->fmt == UNSPECIFIED) {
      snprintf(number_of_lines, 49, P_(" (%zu line)", " (%zu lines)", count), count);
    }
    else {
      snprintf(number_of_lines, 49, P_(" (%zu line, %s)", " (%zu lines, %s)", count), count, ((openfile->fmt == DOS_FILE) ? "DOS" : "Mac"));
    }
    tallywidth = breadth(number_of_lines);
    if ((int)(namewidth + tallywidth + 11) < COLS) {
      waddstr(footwin, number_of_lines);
    }
    else {
      tallywidth = 0;
    }
    report_size = FALSE;
  }
  else if (!CLIST_SINGLE(openfile) && COLS > 35) {
    ranking = xmalloc(24);
    snprintf(ranking, 24, " [%i/%i]", buffer_number(openfile), buffer_number(startfile->prev));
    if ((int)(namewidth + placewidth + breadth(ranking) + 32) < COLS) {
      waddstr(footwin, ranking);
    }
  }
  /* Display the line/column position of the cursor. */
  if (ISSET(CONSTANT_SHOW) && (int)(namewidth + tallywidth + placewidth + 34) < COLS) {
    mvwaddstr(footwin, 0, (COLS - 29 - placewidth), location);
  }
  /* Display the hexadecimal code of the character under the cursor, plus the code of up to two succeeding zero-width characters. */
  if (ISSET(CONSTANT_SHOW) && (int)(namewidth + tallywidth + 30) < COLS) {
    position = (openfile->current->data + openfile->current_x);
    if (!*position) {
      snprintf(hexadecimal, 9, (openfile->current->next ? using_utf8() ? "U+000A" : "  0x0A" : "  ----"));
    }
    else if (*position == NL) {
      snprintf(hexadecimal, 9, "  0x00");
    }
    else if ((Uchar)*position < 0x80 && using_utf8()) {
      snprintf(hexadecimal, 9, "U+%04X", (Uchar)*position);
    }
    else if (using_utf8() && mbtowide(&widecode, position) > 0) {
      snprintf(hexadecimal, 9, "U+%04X", (int)widecode);
    }
    else {
      snprintf(hexadecimal, 9, "  0x%02X", (Uchar)*position);
    }
    mvwaddstr(footwin, 0, (COLS - 25), hexadecimal);
    successor = (position + char_length(position));
    if (*position && *successor && is_zerowidth(successor) && mbtowide(&widecode, successor) > 0) {
      snprintf(hexadecimal, 9, "|%04X", (int)widecode);
      waddstr(footwin, hexadecimal);
      successor += char_length(successor);
      if (*successor && is_zerowidth(successor) && mbtowide(&widecode, successor) > 0) {
        snprintf(hexadecimal, 9, "|%04X", (int)widecode);
        waddstr(footwin, hexadecimal);
      }
    }
    else {
      successor = NULL;
    }
  }
  /* Display the state of three flags, and the state of the macro and mark. */
  if (ISSET(STATEFLAGS) && !successor && (int)(namewidth + tallywidth + 14 + (2 * padding)) < COLS) {
    wmove(footwin, 0, (COLS - 13 - padding));
    show_state_at_curses(footwin);
  }
  /* Display how meny precent the current line is into the file. */
  if ((int)(namewidth + 6) < COLS) {
    snprintf(location, 44, "%.1f%%", ((double)(100 * openfile->current->lineno) / openfile->filebot->lineno));
    mvwaddstr(footwin, 0, (COLS - 6 - padding), location);
  }
  wattroff(footwin, interface_color_pair[config->minibar_color]);
  wrefresh(footwin);
  free(number_of_lines);
  free(hexadecimal);
  free(location);
  free(thename);
  free(ranking);
}

/* Write a key's representation plus a minute description of its function to the screen.  For example,
 * the key could be "^C" and its tag "Cancel". Key plus tag may occupy at most width columns. */
void post_one_key_curses(const char *const restrict keystroke, const char *const restrict tag, int width) {
  wattron(footwin, interface_color_pair[KEY_COMBO]);
  waddnstr(footwin, keystroke, actual_x(keystroke, width));
  wattroff(footwin, interface_color_pair[KEY_COMBO]);
  /* If the remaining space is to small, skip the description. */
  width -= breadth(keystroke);
  if (width < 2) {
    return;
  }
  waddch(footwin, ' ');
  wattron(footwin, interface_color_pair[FUNCTION_TAG]);
  waddnstr(footwin, tag, actual_x(tag, (width - 1)));
  wattroff(footwin, interface_color_pair[FUNCTION_TAG]);
}

/* Display the shortcut list corresponding to the menu on the last to rows of the bottom portion of the window.  The shortcuts are shown in pairs. */
void bottombars_curses(int menu) {
  Ulong index     = 0;
  Ulong number    = 0;
  Ulong itemwidth = 0;
  Ulong width;
  const keystruct *s;
  funcstruct *f;
  /* Set the global variable to the given menu. */
  currmenu = menu;
  /* Set the global variable to the given menu. */
  if (ISSET(NO_HELP) || LINES < (ISSET(ZERO) ? 3 : ISSET(MINIBAR) ? 4 : 5)) {
    return;
  }
  /* Determine how meny shortcuts must be shown. */
  number = shown_entries_for(menu);
  /* Compute the width of each keyname-plus-explanation pair. */
  itemwidth = (COLS / ((number + 1) / 2));
  /* If there is no room, don't do anything. */
  if (!itemwidth) {
    return;
  }
  blank_bottombars_curses();
  /* Display the first number of shortcuts in the given menu that have a key combination assigned to them. */
  for (f=allfuncs, index=0; index<number; f=f->next) {
    width = itemwidth;
    if (!(f->menus & menu) || !(s = first_sc_for(menu, f->func))) {
      continue;
    }
    wmove(footwin, (1 + (index % 2)), ((index / 2) * itemwidth));
    /* When the number is uneven the penultimate item can be double width. */
    if ((number % 2) == 1 && (index + 2) == number) {
      width += itemwidth;
    }
    /* For the last two items, use all the remaining space. */
    if ((index + 2) >= number) {
      width += (COLS % itemwidth);
    }
    post_one_key_curses(s->keystr, _(f->tag), width);
    ++index;
  }
  wrefresh(footwin);
}

/* Redetermine `file->cursor_row` form the current position relative to `file->edittop`, and put the cursor in the edit window at (`file->cursor_row`, `file->current_x`) */
void place_the_cursor_curses_for(openfilestruct *const file) {
  ASSERT(file);
  Ulong column;
  place_the_cursor_for_internal(file, &column, editwincols);
  if (file->cursor_row < editwinrows) {
    wmove(midwin, file->cursor_row, (margin + column));
  }
  else {
    statusline_curses(ALERT, "Misplaced cursor -- please report a bug");
  }
  /* Only needed for NetBSD curses. */
# ifdef _CURSES_H_
  wnoutrefresh(midwin);
# endif
}

/* Warn the user on the status bar and pause for a moment, so that the message can be noticed and read.  Ncurses version. */
void warn_and_briefly_pause_curses(const char *const restrict message) {
  blank_bottombars_curses();
  statusline_curses(ALERT, "%s", message);
  lastmessage = VACUUM;
  napms(1500);
}

/* Draw the marked region of `row`. */
void draw_row_marked_region_for_curses(openfilestruct *const file, int row, const char *const restrict converted, linestruct *const line, Ulong from_col) {
  ASSERT(file);
  ASSERT(row >= 0 && row < editwinrows);
  /* The lines where the marked region begins and ends. */
  linestruct *top;
  linestruct *bot;
  /* The x positions where the marked region begins and ends. */
  Ulong top_x;
  Ulong bot_x;
  /* The column where the painting starts.  Zero-based. */
  int start_col;
  /* The place in `converted` from where painting starts. */
  const char *thetext;
  /* The number of characters to paint.  Negative means all. */
  int paintlen = -1;
  Ulong endcol;
  /* If the line is at least partialy selected, paint the marked part. */
  if (line_in_marked_region_for(file, line)) {
    /* Get the full region of the mark. */
    get_region_for(file, &top, &top_x, &bot, &bot_x);
    /* If the marked region is from the start of this line. */
    if (top->lineno < line->lineno || top_x < from_x) {
      top_x = from_x;
    }
    /* If the marked region is below the end of this line. */
    if (bot->lineno > line->lineno || bot_x > till_x) {
      bot_x = till_x;
    }
    /* Only paint if the marked part of the line is on this page. */
    if (top_x < till_x && bot_x > from_x) {
      /* Compute on witch screen column to start painting. */
      start_col = (wideness(line->data, top_x) - from_col);
      CLAMP_MIN(start_col, 0);
      thetext = (converted + actual_x(converted, start_col));
      /* If the end of the mark if onscreen, compute how menu characters to paint.  Otherwise, just paint all. */
      if (bot_x < till_x) {
        endcol = (wideness(line->data, bot_x) - from_col);
        paintlen = actual_x(thetext, (endcol - start_col));
      }
      wattron(midwin, interface_color_pair[config->selectedtext_color]);
      mvwaddnstr(midwin, row, (margin + start_col), thetext, paintlen);
      wattroff(midwin, interface_color_pair[config->selectedtext_color]);
    }
  }
}

/* Draw the marked region of `row`. */
void draw_row_marked_region_curses(int row, const char *const restrict converted, linestruct *const line, Ulong from_col) {
  draw_row_marked_region_for_curses(openfile, row, converted, line, from_col);
}

/* Tell curses to unconditionally redraw whatever was on the screen. */
void full_refresh_curses(void) {
  wrefresh(curscr);
}

/* Wipe the status bar clean and include this in the next screen update. */
void wipe_statusbar_curses(void) {
  lastmessage = VACUUM;
  if ((ISSET(ZERO) || ISSET(MINIBAR) || LINES == 1) && currmenu == MMAIN) {
    return;
  }
  blank_row_curses(footwin, 0);
  wnoutrefresh(footwin);
}

/* Draw the given text on the given row of the edit window.  line is the line to be drawn, and converted
 * is the actual string to be written with tabs and control characters replaced by strings of regular
 * characters.  'from_col' is the column number of the first character of this "page". */
void draw_row_curses_for(openfilestruct *const file, int row, const char *const restrict converted, linestruct *const line, Ulong from_col) {
  ASSERT(file);
  render_line_text(row, converted, line, from_col);
  if (ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX)) {
    apply_syntax_to_line(row, converted, line, from_col);
  }
  /* If there are color rules (and coloring is turned on), apply them. */
  else if (file->syntax && !ISSET(NO_SYNTAX)) {
    const colortype *varnish = file->syntax->color;
    /* If there are multiline regexes, make sure this line has a cache. */
    if (file->syntax->multiscore > 0 && !line->multidata) {
      line->multidata = xmalloc(file->syntax->multiscore * sizeof(short));
    }
    /* Iterate through all the coloring regexes. */
    for (; varnish; varnish = varnish->next) {
      /* Where in the line we currently begin looking for a match. */
      Ulong index = 0;
      /* The starting column of a piece to paint.  Zero-based. */
      int start_col = 0;
      /* The number of characters to paint. */
      int paintlen = 0;
      /* The place in converted from where painting starts. */
      const char *thetext;
      /* The match positions of a single-line regex. */
      regmatch_t match;
      /* The first line before line that matches 'start'. */
      const linestruct *start_line = line->prev;
      /* The match positions of the start and end regexes. */
      regmatch_t startmatch, endmatch;
      /* First case: varnish is a single-line expression. */
      if (!varnish->end) {
        while (index < PAINT_LIMIT && index < till_x) {
          /* If there is no match, go on to the next line. */
          if (regexec(varnish->start, &line->data[index], 1, &match, (index == 0) ? 0 : REG_NOTBOL) != 0) {
            break;
          }
          /* Translate the match to the beginning of the line. */
          match.rm_so += index;
          match.rm_eo += index;
          index = match.rm_eo;
          /* If the match is offscreen to the right, this rule is
           * done. */
          if (match.rm_so >= (int)till_x) {
            break;
          }
          /* If the match has length zero, advance over it. */
          if (match.rm_so == match.rm_eo) {
            if (line->data[index] == '\0') {
              break;
            }
            index = step_right(line->data, index);
            continue;
          }
          /* If the match is offscreen to the left, skip to next. */
          if (match.rm_eo <= (int)from_x) {
            continue;
          }
          if (match.rm_so > (int)from_x) {
            start_col = wideness(line->data, match.rm_so) - from_col;
          }
          thetext  = converted + actual_x(converted, start_col);
          paintlen = actual_x(thetext, wideness(line->data, match.rm_eo) - from_col - start_col);
          midwin_mv_add_nstr_wattr(row, (margin + start_col), thetext, paintlen, varnish->attributes);
        }
        continue;
      }
      /* Second case: varnish is a multiline expression.  Assume nothing gets painted until proven otherwise below. */
      line->multidata[varnish->id] = NOTHING;
      if (start_line && !start_line->multidata) {
        statusline_all(ALERT, "Missing multidata -- please report a bug");
      }
      else {
        /* If there is an unterminated start match before the current line, we need to look for an end match first. */
        if (start_line && (start_line->multidata[varnish->id] == WHOLELINE || start_line->multidata[varnish->id] == STARTSHERE)) {
          /* If there is no end on this line, paint whole line, and be done. */
          if (regexec(varnish->end, line->data, 1, &endmatch, 0) == REG_NOMATCH) {
            midwin_mv_add_nstr_wattr(row, margin, converted, -1, varnish->attributes);
            line->multidata[varnish->id] = WHOLELINE;
            continue;
          }
          /* Only if it is visible, paint the part to be coloured. */
          if (endmatch.rm_eo > (int)from_x) {
            paintlen = actual_x(converted, wideness(line->data, endmatch.rm_eo) - from_col);
            midwin_mv_add_nstr_wattr(row, margin, converted, paintlen, varnish->attributes);
          }
          line->multidata[varnish->id] = ENDSHERE;
        }
      }
      /* Second step: look for starts on this line, but begin looking only after an end match, if there is one. */
      index = (paintlen == 0) ? 0 : endmatch.rm_eo;
      while (index < PAINT_LIMIT && regexec(varnish->start, line->data + index, 1, &startmatch, (index == 0) ? 0 : REG_NOTBOL) == 0) {
        /* Make the match relative to the beginning of the line. */
        startmatch.rm_so += index;
        startmatch.rm_eo += index;
        if (startmatch.rm_so > (int)from_x) {
          start_col = wideness(line->data, startmatch.rm_so) - from_col;
        }
        thetext = converted + actual_x(converted, start_col);
        if (regexec(varnish->end, line->data + startmatch.rm_eo, 1, &endmatch, (startmatch.rm_eo == 0) ? 0 : REG_NOTBOL) == 0) {
          /* Make the match relative to the beginning of the line. */
          endmatch.rm_so += startmatch.rm_eo;
          endmatch.rm_eo += startmatch.rm_eo;
          /* Only paint the match if it is visible on screen and it is more than zero characters long. */
          if (endmatch.rm_eo > (int)from_x && endmatch.rm_eo > startmatch.rm_so) {
            paintlen = actual_x(thetext, wideness(line->data, endmatch.rm_eo) - from_col - start_col);
            midwin_mv_add_nstr_wattr(row, margin + start_col, thetext, paintlen, varnish->attributes);
            line->multidata[varnish->id] = JUSTONTHIS;
          }
          index = endmatch.rm_eo;
          /* If both start and end match are anchors, advance. */
          if (startmatch.rm_so == startmatch.rm_eo && endmatch.rm_so == endmatch.rm_eo) {
            if (!line->data[index]) {
              break;
            }
            index = step_right(line->data, index);
          }
          continue;
        }
        /* Paint the rest of the line, and we're done. */
        midwin_mv_add_nstr_wattr(row, margin + start_col, thetext, -1, varnish->attributes);
        line->multidata[varnish->id] = STARTSHERE;
        break;
      }
    }
  }
  if (stripe_column > (long)from_col && !inhelp && (!sequel_column || stripe_column <= (long)sequel_column) && stripe_column <= (long)(from_col + editwincols)) {
    long  target_column = (stripe_column - from_col - 1);
    Ulong target_x      = actual_x(converted, target_column);
    char  striped_char[MAXCHARLEN];
    Ulong charlen = 1;
    if (*(converted + target_x)) {
      charlen       = collect_char((converted + target_x), striped_char);
      target_column = wideness(converted, target_x);
#ifdef USING_OLDER_LIBVTE
    }
    else if ((target_column + 1) == editwincols) {
      /* Defeat a VTE bug -- see https://sv.gnu.org/bugs/?55896. */
# ifdef ENABLE_UTF8
      if (using_utf8()) {
        striped_char[0] = '\xC2';
        striped_char[1] = '\xA0';
        charlen         = 2;
      }
    else
# endif
      striped_char[0] = '.';
#endif
    }
    else {
      striped_char[0] = ' ';
    }
    if (ISSET(NO_NCURSES)) {
      
    }
    else {
      mv_add_nstr_color(midwin, row, (margin + target_column), striped_char, charlen, GUIDE_STRIPE);
    }
  }
  draw_row_marked_region_curses(row, converted, line, from_col);
}

/* Draw the given text on the given row of the edit window.  line is the line to be drawn, and converted
 * is the actual string to be written with tabs and control characters replaced by strings of regular
 * characters.  'from_col' is the column number of the first character of this "page". */
void draw_row_curses(int row, const char *const restrict converted, linestruct *const line, Ulong from_col) {
  draw_row_curses_for(openfile, row, converted, line, from_col);
}

/* Redraw the given line so that the character at the given index is visible -- if necessary, scroll the line
 * horizontally (when not softwrapping). Return the number of rows "consumed" (relevant when softwrapping). */
int update_line_curses_for(openfilestruct *const file, linestruct *const line, Ulong index) {
  ASSERT(file);
  ASSERT(line);
  /* The row in the edit window we will be updating. */
  int row;
  /* The data of the line with tabs and control characters expanded. */
  char *converted;
  /* From which column a horizontally scrolled line is displayed. */
  Ulong from_col;
  if (ISSET(SOFTWRAP)) {
    return update_softwrapped_line_curses_for(file, line);
  }
  sequel_column = 0;
  row = (line->lineno - file->edittop->lineno);
  from_col = get_page_start(wideness(line->data, index), editwincols);
  /* Expand the piece to be drawn to its representable form, and draw it. */
  converted = display_string(line->data, from_col, editwincols, TRUE, FALSE);
  draw_row_curses_for(file, row, converted, line, from_col);
  free(converted);
  if (!ISSET(NO_NCURSES)) {
    if (from_col > 0) {
      mvwaddchwattr(midwin, row, margin, '<', hilite_attribute);
    }
    if (has_more) {
      mvwaddchwattr(midwin, row, (COLS - 1), '>', hilite_attribute);
    }
  }
  if (spotlighted && line == file->current) {
    spotlight_curses_for(file, light_from_col, light_to_col);
  }
  return 1;
}

/* Redraw the given line so that the character at the given index is visible -- if necessary, scroll the line
 * horizontally (when not softwrapping). Return the number of rows "consumed" (relevant when softwrapping). */
int update_line_curses(linestruct *const line, Ulong index) {
  return update_line_curses_for(openfile, line, index);
}

/* Redraw all the chunks of the given line (as far as they fit onscreen), unless it's edittop,
 * which will be displayed from column firstcolumn.  Return the number of rows that were "consumed". */
int update_softwrapped_line_curses_for(openfilestruct *const file, linestruct *const line) {
  ASSERT(file);
  ASSERT(line);
  /* The first row in the edit window that gets updated. */
  int starting_row;
  /* The row in the edit window we will write to. */
  int row = 0;
  /* An iterator needed to find the relevent row. */
  linestruct *someline = file->edittop;
  /* The starting column of the current chunk. */
  Ulong from_col = 0;
  /* The end column of the current_chunk. */
  Ulong to_col = 0;
  /* The data of the chunk with tabs and controll chars expanded. */
  char *converted;
  /* This tells the softwrapping rutine to start at begining-of-line. */
  bool kickoff = TRUE;
  /* Becomes 'TRUE' when the last chunk of the line has been reached. */
  bool end_of_line = FALSE;
  if (line == file->edittop) {
    from_col = file->firstcolumn;
  }
  else {
    row -= chunk_for(file->firstcolumn, file->edittop, editwincols);
  }
  /* Find out on which screen row the target line should be shown. */
  while (someline != line && someline) {
    row += (1 + extra_chunks_in(someline, editwincols));
    someline = someline->next;
  }
  /* If the first chunk is offscreen, don't even try to display it. */
  if (row < 0 || row >= editwinrows) {
    return 0;
  }
  starting_row = row;
  while (!end_of_line && row < editwinrows) {
    to_col = get_softwrap_breakpoint(line->data, from_col, &kickoff, &end_of_line, editwincols);
    sequel_column = (end_of_line ? 0 : to_col);
    /* Convert the chunk to its displayable form and draw it. */
    converted = display_string(line->data, from_col, (to_col - from_col), TRUE, FALSE);
    draw_row_curses_for(file, row++, converted, line, from_col);
    free(converted);
    from_col = to_col;
  }
  if (spotlighted && line == file->current) {
    spotlight_softwrapped_curses(light_from_col, light_to_col);
  }
  return (row - starting_row);
}

/* Redraw all the chunks of the given line (as far as they fit onscreen), unless it's edittop,
 * which will be displayed from column firstcolumn.  Return the number of rows that were "consumed". */
int update_softwrapped_line_curses(linestruct *const line) {
  return update_softwrapped_line_curses_for(openfile, line);
}

/* ----------------------------- Spotlight curses ----------------------------- */

/* Highlight the text between the given two columns on the current line. */
void spotlight_curses_for(openfilestruct *const file, Ulong from_col, Ulong to_col) {
  ASSERT(file);
  Ulong right_edge = (get_page_start(from_col, editwincols) + editwincols);
  bool  overshoots = (to_col > right_edge);
  char *word;
  place_the_cursor_curses_for(file);
  /* Limit the end column to the edge of the screen. */
  if (overshoots) {
    to_col = right_edge;
  }
  /* If the target text is of zero length, highlight a space instead. */
  if (to_col == from_col) {
    word = COPY_OF(" ");
    ++to_col;
  }
  else {
    word = display_string(file->current->data, from_col, (to_col - from_col), FALSE, overshoots);
  }
  wattron(midwin, interface_color_pair[SPOTLIGHTED]);
  waddnstr(midwin, word, actual_x(word, to_col));
  if (overshoots) {
    mvwaddch(midwin, file->cursor_row, (COLS - 1 - sidebar), '>');
  }
  wattroff(midwin, interface_color_pair[SPOTLIGHTED]);
  free(word);
}

/* Highlight the text between the given two columns on the current line. */
void spotlight_curses(Ulong from_col, Ulong to_col) {
  spotlight_curses_for(openfile, from_col, to_col);
}

/* ----------------------------- Spotlight softwrapped curses ----------------------------- */

/* Highlight the text between the given two columns on the current line. */
void spotlight_softwrapped_curses_for(openfilestruct *const file, Ulong from_col, Ulong to_col) {
  ASSERT(file);
  long  row;
  Ulong leftedge = leftedge_for(from_col, file->current, editwincols);
  Ulong break_col;
  bool  end_of_line = FALSE;
  bool  kickoff     = TRUE;
  char *word;
  place_the_cursor_curses_for(file);
  row = file->cursor_row;
  while (row < editwinrows) {
    break_col = get_softwrap_breakpoint(file->current->data, leftedge, &kickoff, &end_of_line, editwincols);
    /* If the highlighting ends on this chunk, we can stop after it. */
    if (break_col >= to_col) {
      end_of_line = TRUE;
      break_col   = to_col;
    }
    /* If the target text is of zero length, highlight a space instead. */
    if (break_col == from_col) {
      word = COPY_OF(" ");
      break_col++;
    }
    else {
      word = display_string(file->current->data, from_col, (break_col - from_col), FALSE, FALSE);
    }
    wattron(midwin, interface_color_pair[SPOTLIGHTED]);
    waddnstr(midwin, word, actual_x(word, break_col));
    wattroff(midwin, interface_color_pair[SPOTLIGHTED]);
    free(word);
    if (end_of_line) {
      break;
    }
    wmove(midwin, ++row, margin);
    leftedge = break_col;
    from_col = break_col;
  }
}

/* Highlight the text between the given two columns on the current line. */
void spotlight_softwrapped_curses(Ulong from_col, Ulong to_col) {
  spotlight_softwrapped_curses_for(openfile, from_col, to_col);
}
