/** @file

  @author  Melwin Svensson.
  @date    9-2-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


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
  if (IN_GUI_CONTEXT) {
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
  return xplustabs_for(CONTEXT_OPENFILE);
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

/* Count the number of characters from begin to end, and return it. */
Ulong number_of_characters_in(const linestruct *const begin, const linestruct *const end) {
  ASSERT(begin);
  ASSERT(end);
  Ulong count = 0;
  /* Sum the number of characters (plus a newline) in each line. */
  DLIST_FOR_NEXT_END(begin, end->next, line) {
    count += (mbstrlen(line->data) + 1);
  }
  /* Do not count the final newline. */
  return (count - 1);
}

/* ----------------------------- Magicline ----------------------------- */

/* Append a new magic line to the end of `file`. */
void new_magicline_for(openfilestruct *const file) {
  ASSERT(file);
  file->filebot->next = make_new_node(file->filebot);
  file->filebot->next->data = COPY_OF("");
  DLIST_ADV_NEXT(file->filebot);
  ++file->totsize;
}

/* Append a new magic line to the end of the buffer. */
void new_magicline(void) {
  new_magicline_for(CONTEXT_OPENFILE);
}

/* Remove the magic line from the end of `file`, if there is one and it isn't the only line in `file`. */
void remove_magicline_for(openfilestruct *const file) {
  ASSERT(file);
  if (!*file->filebot->data && file->filebot != file->filetop) {
    if (file->current == file->filebot) {
      DLIST_ADV_PREV(file->current);
    }
    DLIST_ADV_PREV(file->filebot);
    delete_node(file->filebot->next);
    file->filebot->next = NULL;
    --file->totsize;
  }
}

/* Remove the magic line from the end of the currently open file, if there is one and it isn't the only line in it. */
void remove_magicline(void) {
  remove_magicline_for(CONTEXT_OPENFILE);
}

/* ----------------------------- Mark is before cursor ----------------------------- */

/* Return 'TRUE' when the mark is before or at the cursor, and FALSE otherwise. */
bool mark_is_before_cursor_for(openfilestruct *const file) {
  ASSERT(file);
  return (file->mark->lineno < file->current->lineno || (file->mark == file->current && file->mark_x <= file->current_x));
}

/* Return 'TRUE' when the mark is before or at the cursor, and FALSE otherwise. */
bool mark_is_before_cursor(void) {
  return mark_is_before_cursor_for(CONTEXT_OPENFILE);
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
  get_region_for(CONTEXT_OPENFILE, top, top_x, bot, bot_x);
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
  get_range_for(CONTEXT_OPENFILE, top, bot);
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

/* Returns the number of spaces the next tabstop is from `index` in `string`. */
Ulong tabstop_length(const char *const restrict string, Ulong index) {
  return (tabsize - (wideness(string, index) % tabsize));
}

// char *next_tabstop_space_str(const char *const restrict str, Ulong index, Ulong *const restrict len) {
//   ASSERT(str);
//   ASSERT(len);
//   (*len) = (tabsize - (wideness(str, index) % tabsize));
//   return fmtstr("%*s", (int)(*len), " ");
// }

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
  return tab_space_string_for(CONTEXT_OPENFILE, length);
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

/* Returns an allocated string containing the the indent of string plus one tabs worth of spaces at most, depending on how far away from the next tabstop. */
char *indent_plus_tab(const char *const restrict string) {
  ASSERT(string);
  char *ret;
  Ulong indentlen;
  Ulong tablen;
  if (!*string) {
    return construct_full_tab_string(NULL);
  }
  indentlen = indent_length(string);
  tablen    = tabstop_length(string, indentlen);
  ret       = xmalloc(indentlen + tablen + 1);
  memcpy(ret, string, indentlen);
  if (ISSET(TABS_TO_SPACES)) {
    memset((ret + indentlen), ' ', tablen);
    ret[indentlen + tablen] = '\0';
  }
  else {
    ret[indentlen]     = '\t';
    ret[indentlen + 1] = '\0';
  }
  return ret;
}
