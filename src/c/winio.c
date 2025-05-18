/** @file winio.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../include/c_proto.h"


/* From where in the relevant line the current row is drawn. */
Ulong from_x = 0;
/* Until where in the relevant line the current row is drawn. */
Ulong till_x = 0;
/* Whether the current line has more text after the displayed part. */
static bool has_more = FALSE;
/* Whether a row's text is narrower than the screen's width. */
bool is_shorter = TRUE;


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

#define ISO8859_CHAR   FALSE
#define ZEROWIDTH_CHAR (is_zerowidth(text))

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
