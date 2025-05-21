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


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* From where in the relevant line the current row is drawn. */
Ulong from_x = 0;
/* Until where in the relevant line the current row is drawn. */
Ulong till_x = 0;
/* Whether the current line has more text after the displayed part. */
static bool has_more = FALSE;
/* Whether a row's text is narrower than the screen's width. */
bool is_shorter = TRUE;
/* The number of key codes waiting in the keystroke buffer. */
Ulong waiting_codes = 0;
/* The number of keystrokes left before we blank the status bar. */
int countdown = 0;
/* Whether we are in the process of recording a macro. */
bool recording = FALSE;


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


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


bool get_has_more(void) {
  return has_more;
}

/* Get the column number after leftedge where we can break the given linedata,
 * and return it.  (This will always be at most editwincols after leftedge)
 * When kickoff is TRUE, start at the beginning of the linedata; otherwise,
 * continue from where the previous call left off.  Set end_of_line to TRUE
 * when end-of-line is reached while searching for a possible breakpoint. */
Ulong get_softwrap_breakpoint(const char *linedata, Ulong leftedge, bool *kickoff, bool *end_of_line) {
  /* Pointer at the current character in this line's data. */
  static const char *text;
  /* Column position that corresponds to the above pointer. */
  static Ulong column;
  /* The place at or before which text must be broken. */
  Ulong rightside = (leftedge + editwincols);
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
  return ((editwincols > 1) ? breaking_col : (column - 1));
}

/* Return the row number of the softwrapped chunk in the given line that the given column is on, relative
 * to the first row (zero-based).  If leftedge isn't NULL, return in it the leftmost column of the chunk. */
Ulong get_chunk_and_edge(Ulong column, linestruct *line, Ulong *leftedge) {
  Ulong end_col;
  Ulong current_chunk = 0;
  Ulong start_col     = 0;
  bool end_of_line    = FALSE;
  bool kickoff        = TRUE;
  while (TRUE) {
    end_col = get_softwrap_breakpoint(line->data, start_col, &kickoff, &end_of_line);
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
Ulong extra_chunks_in(linestruct *line) {
  return get_chunk_and_edge((Ulong)-1, line, NULL);
}

/* Return the row of the softwrapped chunk of the given line that column is on, relative to the first row (zero-based). */
Ulong chunk_for(Ulong column, linestruct *line) {
  return get_chunk_and_edge(column, line, NULL);
}

/* Return the leftmost column of the softwrapped chunk of the given line that the given column is on. */
Ulong leftedge_for(Ulong column, linestruct *line) {
  Ulong leftedge;
  get_chunk_and_edge(column, line, &leftedge);
  return leftedge;
}

/* Try to move up nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move up, which will be zero if we completely succeeded. */
int go_back_chunks_for(openfilestruct *const file, int nrows, linestruct **const line, Ulong *const leftedge) {
  int i;
  Ulong chunk;
  if (ISSET(SOFTWRAP)) {
    /* Recede through the requested number of chunks. */
    for (i=nrows; i>0; --i) {
      chunk       = chunk_for((*leftedge), (*line));
      (*leftedge) = 0;
      if ((int)chunk >= i) {
        return go_forward_chunks_for(file, (chunk - i), line, leftedge);
      }
      else if ((*line) == file->filetop) {
        break;
      }
      i -= chunk;
      CLIST_ADV_PREV(*line);
      (*leftedge) = HIGHEST_POSITIVE;
    }
    if ((*leftedge) == HIGHEST_POSITIVE) {
      (*leftedge) = leftedge_for((*leftedge), (*line));
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
  return go_back_chunks_for(openfile, nrows, line, leftedge);
}

/* Try to move down nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move down, which will be zero if we completely succeeded. */
int go_forward_chunks_for(openfilestruct *const file, int nrows, linestruct **const line, Ulong *const leftedge) {
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
      current_leftedge = get_softwrap_breakpoint((*line)->data, current_leftedge, &kickoff, &eol);
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
  return go_forward_chunks_for(openfile, nrows, line, leftedge);
}

void ensure_firstcolumn_is_aligned_for(openfilestruct *const file) {
  ASSERT(file);
  if (ISSET(SOFTWRAP)) {
    file->firstcolumn = leftedge_for(file->firstcolumn, file->edittop);
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
  // if (ISSET(SOFTWRAP)) {
  //   openfile->firstcolumn = leftedge_for(openfile->firstcolumn, openfile->edittop);
  // }
  // else {
  //   openfile->firstcolumn = 0;
  // }
  // /* If smooth scrolling is on, make sure the viewport doesn't center. */
  // focusing = FALSE;
  ensure_firstcolumn_is_aligned_for(openfile);
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
  Ulong allocsize = (((ISSET(USING_GUI) ? (editwincols) : COLS) + stowaways) * MAXCHARLEN + 1);
  /* The displayable string we will return. */
  char *converted = xmalloc(allocsize);
  /* Current position in converted. */
  Ulong index = 0;
  /* The column number just beyond the last shown character. */
  Ulong beyond = (column + span);
  text += start_x;
  if (span > HIGHEST_POSITIVE) {
    // logE("Span has underflowed.");
    // statusline(ALERT, "Span has underflowed -- please report a bug");
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
        for (int i = 0; i < whitelen[0];) {
          converted[index++] = whitespace[i++];
        }
      }
      else {
        converted[index++] = ' ';
      }
      ++column;
      /* Fill the tab up with the required number of spaces. */
      while (column % tabsize != 0 && column < beyond) {
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
    wchar_t wc;
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
bool line_needs_update_for(openfilestruct *const file, Ulong old_column, Ulong new_column) {
  if (file->mark) {
    return TRUE;
  }
  else {
    return (get_page_start(old_column) != get_page_start(new_column));
  }
}

/* Check whether the mark is on, or whether old_column and new_column are on different "pages"
 * (in softwrap mode, only the former applies), which means that the relevant line needs to be redrawn. */
bool line_needs_update(Ulong old_column, Ulong new_column) {
  return line_needs_update_for(openfile, old_column, new_column);
}

/* Return 'TRUE' if there are fewer than a screen's worth of lines between the line at line number
 * was_lineno (and column was_leftedge, if we're in softwrap mode) and the line at current[current_x]. */
bool less_than_a_screenful_for(openfilestruct *const file, Ulong was_lineno, Ulong was_leftedge) {
  int rows_left;
  Ulong leftedge;
  linestruct *line;
  if (ISSET(SOFTWRAP)) {
    line = file->current;
    leftedge  = leftedge_for(xplustabs_for(file), file->current);
    rows_left = go_back_chunks_for(file, (editwinrows - 1), &line, &leftedge);
    return (rows_left > 0 || line->lineno < (long)was_lineno || (line->lineno == (long)was_lineno && leftedge <= was_leftedge));
  }
  else {
    return ((int)(file->current->lineno - was_lineno) < editwinrows);
  }
}

/* Return 'TRUE' if there are fewer than a screen's worth of lines between the line at line number was_lineno
 * (and column was_leftedge, if we're in softwrap mode) and the line at `openfile->current[openfile->current_x]`. */
bool less_than_a_screenful(Ulong was_lineno, Ulong was_leftedge) {
  return less_than_a_screenful_for(openfile, was_lineno, was_leftedge);
}

/* When in softwrap mode, and the given column is on or after the breakpoint of a softwrapped
 * chunk, shift it back to the last column before the breakpoint.  The given column is relative
 * to the given leftedge in current.  The returned column is relative to the start of the text. */
Ulong actual_last_column_for(openfilestruct *const file, Ulong leftedge, Ulong column) {
  ASSERT(file);
  bool  kickoff, last_chunk;
  Ulong end_col;
  if (ISSET(SOFTWRAP)) {
    kickoff    = TRUE;
    last_chunk = FALSE;
    end_col    = (get_softwrap_breakpoint(file->current->data, leftedge, &kickoff, &last_chunk) - leftedge);
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
  return actual_last_column_for(openfile, leftedge, column);
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
  return current_is_above_screen_for(openfile);
}

/* Return `TRUE` if `file->current[file->current_x]` is beyond the viewport. */
bool current_is_below_screen_for(openfilestruct *const file) {
  ASSERT(file);
  linestruct *line;
  Ulong leftedge;
  if (ISSET(SOFTWRAP)) {
    line     = file->edittop;
    leftedge = file->firstcolumn;
    /* If current[current_x] is more than a screen's worth of lines after edittop at column firstcolumn, it's below the screen. */
    return (go_forward_chunks_for(file, (editwinrows - 1 - SHIM), &line, &leftedge) == 0 && (line->lineno < file->current->lineno
     || (line->lineno == file->current->lineno && leftedge < leftedge_for(xplustabs_for(file), file->current))));
  }
  return (file->current->lineno >= (file->edittop->lineno + editwinrows - SHIM));
}

/* Return `TRUE` if `openfile->current[openfile->current_x]` is beyond the viewport. */
bool current_is_below_screen(void) {
  return current_is_above_screen_for(openfile);
}

/* Return TRUE if current[current_x] is outside the viewport. */
bool current_is_offscreen_for(openfilestruct *const file) {
  ASSERT(file);
  return (current_is_above_screen_for(file) || current_is_below_screen_for(file));
}

/* Return TRUE if current[current_x] is outside the viewport. */
bool current_is_offscreen(void) {
  return current_is_offscreen_for(openfile);
}

/* Move edittop so that current is on the screen.  manner says how:
 * STATIONARY means that the cursor should stay on the same screen row,
 * CENTERING means that current should end up in the middle of the screen,
 * and FLOWING means that it should scroll no more than needed to bring
 * current into view. */
void adjust_viewport_for(openfilestruct *const file, update_type manner) {
  ASSERT(file);
  int goal = 0;
  if (manner == STATIONARY) {
    goal = file->cursor_row;
  }
  else if (manner == CENTERING) {
    goal = (editwinrows / 2);
  }
  else if (!current_is_above_screen_for(file)) {
    goal = (editwinrows - 1 - SHIM);
  }
  file->edittop = file->current;
  if (ISSET(SOFTWRAP)) {
    file->firstcolumn = leftedge_for(xplustabs_for(file), file->current);
  }
  /* Move edittop back goal rows, starting at current[current_x]. */
  go_back_chunks_for(file, goal, &file->edittop, &file->firstcolumn);
}

/* Move edittop so that current is on the screen.  manner says how:
 * STATIONARY means that the cursor should stay on the same screen row,
 * CENTERING means that current should end up in the middle of the screen,
 * and FLOWING means that it should scroll no more than needed to bring
 * current into view. */
void adjust_viewport(update_type manner) {
  adjust_viewport_for(openfile, manner);
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

void statusline_curses(message_type type, const char *const restrict msg, ...) {
  va_list ap;
  va_start(ap, msg);
  statusline_curses_va(type, msg, ap);
  va_end(ap);
}

/* If path is NULL, we're in normal editing mode, so display the current
 * version of nano, the current filename, and whether the current file
 * has been modified on the title bar.  If path isn't NULL, we're either
 * in the file browser or the help viewer, so show either the current
 * directory or the title of help text, that is: whatever is in path. */
void titlebar_curses(const char *path) {
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
void place_the_cursor_for_curses(openfilestruct *const file) {
  long        row = 0;
  Ulong       column = xplustabs_for(file);
  linestruct *line;
  Ulong       leftedge;
  if (ISSET(SOFTWRAP)) {
    line = file->edittop;
    row -= chunk_for(file->firstcolumn, file->edittop);
    /* Calculate how meny rows from edittop to current use. */
    while (line && line != file->current) {
      row += (1 + extra_chunks_in(line));
      line = line->next;
    }
    /* Add the number of wraps in the current line before the cursor. */
    row    += get_chunk_and_edge(column, file->current, &leftedge);
    column -= leftedge;
  }
  else {
    row = (file->current->lineno - file->edittop->lineno);
    column -= get_page_start(column);
  }
  if (row < editwinrows) {
    wmove(midwin, row, (margin + column));
  }
  else {
    statusline_curses(ALERT, "Misplaced cursor -- please report a bug");
  }
  /* Only needed for NetBSD curses. */
# ifdef _CURSES_H_
  wnoutrefresh(midwin);
# endif
  file->cursor_row = row;
}

/* Redetermine `openfile->cursor_row` form the current position relative to `openfile->edittop`, and put the cursor in the edit window at (`openfile->cursor_row`, `openfile->current_x`) */
void place_the_cursor_curses(void) {
  place_the_cursor_for_curses(openfile);
}

/* Warn the user on the status bar and pause for a moment, so that the message can be noticed and read.  Ncurses version. */
void warn_and_briefly_pause_curses(const char *const restrict message) {
  blank_bottombars_curses();
  statusline_curses(ALERT, "%s", message);
  lastmessage = VACUUM;
  napms(1500);
}
