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

/* Return a copy of the two given strings, welded together. */
char *concatenate(const char *path, const char *name) {
  Ulong pathlen = strlen(path);
  char *joined  = xmalloc(pathlen + strlen(name) + 1);
  strcpy(joined, path);
  strcpy((joined + pathlen), name);
  return joined;
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

/* Append an `char * array` onto `array`.  Free `append` but not any elements in it after the call. */
void append_chararray(char ***const array, Ulong *const len, char **const append, Ulong append_len) {
  ASSERT(array);
  ASSERT(len);
  ASSERT(append);
  Ulong new_len = ((*len) + append_len);
  (*array) = xrealloc((*array), (_PTRSIZE * (new_len + 1)));
  for (Ulong i=0; i<append_len; ++i) {
    (*array)[(*len) + i] = append[i];
  }
  (*array)[new_len] = NULL;
  (*len) = new_len;
}

/* Return an appropriately reallocated dest string holding a copy of
 * `src`.  Usage: "dest = mallocstrcpy(dest, src, strlen(src));". */
char *realloc_strncpy(char *dest, const char *const restrict src, Ulong length) {
  dest = xrealloc(dest, (length + 1));
  memcpy(dest, src, length);
  dest[length] = '\0';
  return dest;
}

/* Return an appropriately reallocated dest string holding a
 * copy of src.  Usage: "dest = mallocstrcpy(dest, src);". */
char *realloc_strcpy(char *dest, const char *const restrict src) {
  return realloc_strncpy(dest, src, strlen(src));
}

/* Return the user's home directory.  We use $HOME, and if that fails, we fall back on the home directory of the effective user ID. */
void get_homedir(void) {
  const char *homenv;
  const struct passwd *userage;
  if (!homedir) {
    homenv = getenv("HOME");
    /* When HOME isn't set,or when we're root, get the home directory from the password file instead. */
    if (!homenv || geteuid() == ROOT_UID) {
      userage = getpwuid(geteuid());
      if (userage) {
        homenv = userage->pw_dir;
      }
    }
    /* Only set homedir if some home directory could be determined, otherwise keep homedir 'NULL'. */
    if (homenv && *homenv) {
      homedir = copy_of(homenv);
    }
  }
}

/* Returns a line pointer by number using optimized traversal.
 *  1. From current position.
 *  2. From file start.
 *  3. From file end.
 * Chooses shortest path to target line. */
linestruct *line_from_number_for(openfilestruct *const file, long number) {
  ASSERT(file->current);
  ASSERT(file->filetop);
  ASSERT(file->filebot);
  /* Always assert that number is a valid number in the context of the open file. */
  ALWAYS_ASSERT(number >= file->filetop->lineno && number <= file->filebot->lineno);
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

/* Returns a `linestruct` pointer from the currently open file, while also ensuring context correct operation. */
linestruct *line_from_number(long number) {
  if (ISSET(USING_GUI)) {
    return line_from_number_for(openeditor->openfile, number);
  }
  else {
    return line_from_number_for(openfile, number);
  }
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

/* In the given string, recode each embedded NUL as a newline. */
void recode_NUL_to_LF(char *string, Ulong length) {
  while (length > 0) {
    if (!*string) {
      *string = '\n';
    }
    --length;
    ++string;
  }
}

/* In the given string, recode each embedded newline as a NUL, and return the number of bytes in the string. */
Ulong recode_LF_to_NUL(char *string) {
  char *beginning = string;
  while (*string) {
    if (*string == '\n') {
      *string = '\0';
    }
    ++string;
  }
  return (string - beginning);
}

/* When not softwrapping, nano scrolls the current line horizontally by
 * chunks ("pages").  Return the column number of the first character
 * displayed in the edit window when the cursor is at the given column. */
Ulong get_page_start(Ulong column, int total_cols) {
  if (!column || (int)(column + 2) < total_cols || ISSET(SOFTWRAP)) {
    return 0;
  }
  else if (total_cols > 8) {
    return (column - 6 - ((column - 6) % (total_cols - 8)));
  }
  else {
    return (column - (total_cols - 2));
  }
}

/* Return the placewewant associated with `file->current_x`, i.e. the zero-based column position of the cursor. */
Ulong xplustabs_for(openfilestruct *const file) {
  return wideness(file->current->data, file->current_x);
}

/* Return the placewewant associated with current_x, i.e. the zero-based column position of the cursor. */
Ulong xplustabs(void) {
  return wideness(openfile->current->data, openfile->current_x);
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
void statusline_all(message_type type, const char *const restrict format, ...) {
  ASSERT(format);
  va_list ap;
  va_start(ap, format);
  if (ISSET(USING_GUI)) {
    statusline_gui_va(type, format, ap);
  }
  else if (!ISSET(NO_NCURSES)) {
    statusline_curses_va(type, format, ap);
  }
  va_end(ap);
}

/* Print a normal `msg`, that conforms to the current contex, tui or gui. */
void statusbar_all(const char *const restrict msg) {
  if (ISSET(USING_GUI)) {
    statusbar_gui(msg);
  }
  else if (!ISSET(NO_NCURSES)) {
    statusbar_curses(msg);
  }
}

/* ----------------------------- Magicline ----------------------------- */

/* Append a new magic line to the end of `file`. */
void new_magicline_for(openfilestruct *const file) {
  ASSERT(file);
  file->filebot->next = make_new_node(file->filebot);
  file->filebot->next->data = COPY_OF("");
  CLIST_ADV_NEXT(file->filebot);
  ++file->totsize;
}

/* Append a new magic line to the end of the buffer. */
void new_magicline(void) {
  new_magicline_for(ISSET(USING_GUI) ? openeditor->openfile : openfile);
  // openfile->filebot->next = make_new_node(openfile->filebot);
  // openfile->filebot->next->data = COPY_OF("");
  // openfile->filebot = openfile->filebot->next;
  // ++openfile->totsize;
}

/* Remove the magic line from the end of `file`, if there is one and it isn't the only line in `file`. */
void remove_magicline_for(openfilestruct *const file) {
  ASSERT(file);
  if (!*file->current->data && file->filebot != file->filetop) {
    if (file->current == file->filebot) {
      CLIST_ADV_PREV(file->current);
    }
    CLIST_ADV_PREV(file->filebot);
    delete_node(file->filebot->next);
    file->filebot->next = NULL;
    --file->totsize;
  }
}

/* Remove the magic line from the end of the currently open file, if there is one and it isn't the only line in it. */
void remove_magicline(void) {
  remove_magicline_for(ISSET(USING_GUI) ? openeditor->openfile : openfile);
  // if (!openfile->filebot->data[0] && openfile->filebot != openfile->filetop) {
  //   if (openfile->current == openfile->filebot) {
  //     openfile->current = openfile->current->prev;
  //   }
  //   openfile->filebot = openfile->filebot->prev;
  //   delete_node(openfile->filebot->next);
  //   openfile->filebot->next = NULL;
  //   --openfile->totsize;
  // }
}

/* ----------------------------- Mark is before cursor ----------------------------- */

/* Return 'TRUE' when the mark is before or at the cursor, and FALSE otherwise. */
bool mark_is_before_cursor_for(openfilestruct *const file) {
  ASSERT(file);
  return (file->mark->lineno < file->current->lineno || (file->mark == file->current && file->mark_x <= file->current_x));
}

/* Return 'TRUE' when the mark is before or at the cursor, and FALSE otherwise. */
bool mark_is_before_cursor(void) {
  return mark_is_before_cursor_for(openfile);
}

/* ----------------------------- Get region ----------------------------- */

/* Return in (top, top_x) and (bot, bot_x) the start and end "coordinates" of the marked region. */
void get_region_for(openfilestruct *const file, linestruct **const top, Ulong *const top_x, linestruct **const bot, Ulong *const bot_x) {
  ASSERT(file);
  ASSERT(top || top_x || bot || bot_x);
  if (mark_is_before_cursor_for(file)) {
    ASSIGN_IF_VALID(top,   file->mark);
    ASSIGN_IF_VALID(top_x, file->mark_x);
    ASSIGN_IF_VALID(bot,   file->current);
    ASSIGN_IF_VALID(bot_x, file->current_x);
  }
  else {
    ASSIGN_IF_VALID(bot,   file->mark);
    ASSIGN_IF_VALID(bot_x, file->mark_x);
    ASSIGN_IF_VALID(top,   file->current);
    ASSIGN_IF_VALID(top_x, file->current_x);
  }
}

/* Return in (top, top_x) and (bot, bot_x) the start and end "coordinates" of the marked region. */
void get_region(linestruct **const top, Ulong *const top_x, linestruct **const bot, Ulong *const bot_x) {
  get_region_for(openfile, top, top_x, bot, bot_x);
}

/* ----------------------------- Get range ----------------------------- */

/* Get the set of lines to work on -- either just the current line, or the first to last lines of the marked
 * region.  When the cursor (or mark) is at the start of the last line of the region, exclude that line. */
void get_range_for(openfilestruct *const file, linestruct **const top, linestruct **const bot) {
  ASSERT(file);
  ASSERT(top);
  ASSERT(bot);
  Ulong top_x;
  Ulong bot_x;
  /* No mark is set. */
  if (!openfile->mark) {
    (*top) = openfile->current;
    (*bot) = openfile->current;
  }
  else {
    get_region_for(file, top, &top_x, bot, &bot_x);
    if (bot_x == 0 && bot != top && !also_the_last) {
      (*bot) = (*bot)->prev;
    }
    else {
      also_the_last = TRUE;
    }
  }
}

/* Get the set of lines to work on -- either just the current line, or the first to last lines of the marked
 * region.  When the cursor (or mark) is at the start of the last line of the region, exclude that line. */
void get_range(linestruct **const top, linestruct **const bot) {
  get_range_for(openfile, top, bot);
}

/* Read one number (or two numbers separated by comma, period, or colon) from the given string and store
 * the number(s) in *line (and *column).  Return 'FALSE' on a failed parsing, and 'TRUE' otherwise. */
bool parse_line_column(const char *string, long *const line, long *const column) {
  ASSERT(string);
  ASSERT(line);
  ASSERT(column);
  const char *comma;
  char *firstpart;
  bool retval;
  while (*string == ' ') {
    ++string;
  }
  comma = strpbrk(string, ",.:");
  if (!comma) {
    return parse_num(string, line);
  }
  retval = parse_num((comma + 1), column);
  if (comma == string) {
    return retval;
  }
  firstpart = copy_of(string);
  firstpart[comma - string] = '\0';
  retval = (parse_num(firstpart, line) && retval);
  free(firstpart);
  return retval;
}

/* Returns an allocated string of spaces that completes the current tab stop in `file->current->data`,
 * up to one `tabsize` in length.  The number of spaces is calculated based on `file->current_x` so
 * that when injecting the string into `file->current->data` we end up at the next tab boundary. */
char *tab_space_string_for(openfilestruct *const file, Ulong *length) {
  ASSERT(file);
  ASSERT(length);
  (*length) = (tabsize - (xplustabs_for(file) % tabsize));
  return fmtstr("%*s", (int)(*length), " ");
}

/* Almost exactly like `tab_space_string_for()`, but correctly passes either
 * `openeditor->openfile` or `openfile` depending on the current context. */
char *tab_space_string(Ulong *length) {
  if (ISSET(USING_GUI)) {
    return tab_space_string_for(openeditor->openfile, length);
  }
  else {
    return tab_space_string_for(openfile, length);
  }
}

/* Returns an allocated string containing either a string of just ' ' chars with the
 * length `tabsize` if `TABS_TO_SPACES` is set or a string just contaning a '\t' char. */
char *construct_full_tab_string(Ulong *length) {
  if (ISSET(TABS_TO_SPACES)) {
    ASSIGN_IF_VALID(length, tabsize);
    return fmtstr("%*s", (int)tabsize, " ");
  }
  else {
    ASSIGN_IF_VALID(length, STRLEN("\t"));
    return COPY_OF("\t");
  }
}

/* ----------------------------- Set placewewant ----------------------------- */

/* Set `file->placewewant` to the visualy correct column based on `file->current_x` in `file->current->data`. */
void set_pww_for(openfilestruct *const file) {
  ASSERT(file);
  file->placewewant = xplustabs_for(file);
}

/* Set `openfile->placewewant` to the visualy correct column based on `openfile->current_x` in
 * `openfile->current->data`.  Note that this is context safe, and works for both the gui and tui. */
void set_pww(void) {
  set_pww_for(CONTEXT_OPENFILE);
}

/* ----------------------------- Set cursor to end of line ----------------------------- */

/* Correctly sets `file->current_x` to the end of `file->current->data` and correctly sets `file->placewant` to ensure visual correctness. */
void set_cursor_to_eol_for(openfilestruct *const file) {
  ASSERT(file);
  file->current_x = strlen(file->current->data);
  set_pww_for(file);
}

/* Correctly sets `openfile->current_x` to the end of `openfile->current->data` and correctly sets `openfile->placewant`
 * to ensure visual correctness.  Note that this is context safe, and works in both the gui and tui. */
void set_cursor_to_eol(void) {
  set_cursor_to_eol_for(CONTEXT_OPENFILE);
}

/* ----------------------------- Set mark ----------------------------- */

/* Set the mark at specific line and column for `file`. */
void set_mark_for(openfilestruct *const file, long lineno, Ulong x) {
  ASSERT(file);
  file->mark   = line_from_number_for(file, lineno);
  file->mark_x = x;
}

/* Set the mark at specific line and column for the currently open file.  Note that this is `context-safe`, and works in both the `gui` and `tui`. */
void set_mark(long lineno, Ulong x) {
  set_mark_for(CONTEXT_OPENFILE, lineno, x);
}
