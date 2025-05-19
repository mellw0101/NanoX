/** @file

  @author  Melwin Svensson.
  @date    9-2-2025.

 */
#include "../include/c_proto.h"

void die(const char *format, ...) {
  char *msg;
  va_list ap;
  va_start(ap, format);
  msg = valstr(format, ap, NULL);
  va_end(ap);
  if (die_cb) {
    die_cb("%s", msg);
  }
  else {
    fprintf(stderr, "%s", msg);
    exit(1);
  }
}

/* Return a ptr to the last part of a path, if there are no '/' in the path, just return `path`. */
// const char *tail(const char *const path) {
//   ASSERT(path);
//   const char *slash = strrchr(path, '/');
//   if (slash) {
//     return (slash + 1);
//   }
//   else {
//     return path;
//   }
// }

/* Return the extention of `path`, if any.  Otherwise, return NULL. */
// const char *ext(const char *const path) {
//   ASSERT(path);
//   const char *pathtail = tail(path);
//   /* If the tail of the path starts with '.', then this is not a extention. */
//   if (*pathtail == '.') {
//     return NULL;
//   }
//   return (strrchr(pathtail, '.'));
// }

/* Return's a allocated ptr of `howmeny` threads. */
thread_t *get_nthreads(Ulong howmeny) {
  return xmalloc(sizeof(thread_t) * howmeny);
}

/* Free's a `NULL-TERMINATED` char array. */
void free_nulltermchararray(char **const argv) {
  char **copy = argv;
  /* Make this function `NULL-SAFE`. */
  if (!argv) {
    return;
  }
  while (*copy) {
    free(*copy);
    ++copy;
  }
  free(argv);
}

/**
  Return's line pointer by number using optimized traversal.
  1. From current position.
  2. From file start.
  3. From file end.
  Chooses shortest path to target line.
 */
linestruct *file_line_from_number(openfilestruct *const file, long number) {
  ASSERT(file->current);
  ASSERT(file->filetop);
  ASSERT(file->filebot);
  /* Always assert that number is a valid number in the context of the open file. */
  ALWAYS_ASSERT(number >= 0 && number <= file->filebot->lineno);
  /* Set the starting line to the cursor line of the currently active file in editor. */
  linestruct *line = file->current;
  /* Get the distance from the start and end line as well as from the cursor. */
  long dist_from_current = labs(line->lineno - number);
  long dist_from_end     = (file->filebot->lineno - number);
  /* Start from the closest line to number. */
  if (number < dist_from_current && number < dist_from_end) {
    line = file->filetop;
  }
  else if (dist_from_end < dist_from_current) {
    line = file->filebot;
  }
  /* Move the line ptr to the correct line. */
  while (line->lineno > number) {
    line = line->prev;
  }
  while (line->lineno < number) {
    line = line->next;
  }
  return line;
}

/* Free the memory of the given array, which should contain len elements. */
void free_chararray(char **array, Ulong len) {
  if (!array) {
    return;
  }
  while (len > 0) {
    free(array[--len]);
  }
  free(array);
}

/* When not softwrapping, nano scrolls the current line horizontally by
 * chunks ("pages").  Return the column number of the first character
 * displayed in the edit window when the cursor is at the given column. */
Ulong get_page_start(const Ulong column) {
  if (!column || (int)(column + 2) < editwincols || ISSET(SOFTWRAP)) {
    return 0;
  }
  else if (editwincols > 8) {
    return (column - 6 - (column - 6) % (editwincols - 8));
  }
  else {
    return (column - (editwincols - 2));
  }
}

/* A strnlen() with tabs and multicolumn characters factored in: how many columns wide are the first maxlen bytes of text? */
Ulong wideness(const char *text, Ulong maxlen) {
  Ulong width = 0;
  if (!maxlen) {
    return 0;
  }
  for (Ulong charlen; *text && (maxlen > (charlen = advance_over(text, &width))); maxlen -= charlen, text += charlen);
  return width;
}

/* Return the index in text of the character that (when displayed) will not overshoot the given column. */
Ulong actual_x(const char *text, Ulong column) {
  /* From where we start walking through the text. */
  const char *start = text;
  /* The current accumulated span, in columns. */
  Ulong width = 0;
  /* Calculated length of the current char. */
  int charlen;
  while (*text) {
    charlen = advance_over(text, &width);
    if (width > column) {
      break;
    }
    text += charlen;
  }
  return (text - start);
}

/* Return the number of columns that the given text occupies. */
Ulong breadth(const char *text) {
  Ulong span = 0;
  for (; *text; text += advance_over(text, &span));
  return span;
}

/* For functions that are used by the tui and gui, this prints a status message correctly. */
void print_status(message_type type, const char *const restrict format, ...) {
  ASSERT(format);
  char *msg;
  va_list ap;
  va_start(ap, format);
  msg = valstr(format, ap, NULL);
  va_end(ap);
  if (ISSET(USING_GUI)) {
    statusbar_msg(type, "%s", msg);
  }
  else if (!ISSET(NO_NCURSES)) {
    statusline_curses(type, "%s", msg);
  }
  free(msg);
}
