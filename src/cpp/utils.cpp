/** @file utils.cpp */
#include "../include/prototypes.h"

/* Return the user's home directory.  We use $HOME, and if that fails, we fall back on the home directory of the effective user ID. */
// void get_homedir(void) _NOTHROW {
//   if (!homedir) {
//     const char *homenv = getenv("HOME");
//     /* When HOME isn't set,or when we're root, get the home directory from the password file instead. */
//     if (!homenv || geteuid() == ROOT_UID) {
//       const passwd *userage = getpwuid(geteuid());
//       if (userage) {
//         homenv = userage->pw_dir;
//       }
//     }
//     /* Only set homedir if some home directory could be determined, otherwise keep homedir 'NULL'. */
//     if (homenv && *homenv) {
//       homedir = copy_of(homenv);
//     }
//   }
// }

/* Return a malloc`ed 'char **' with malloc`ed 'char *' strings containing all paths in the env var 'PATH'.  Return`s NULL on failure. */
char **get_env_paths(Ulong *npaths) _NOTHROW {
  /* Get the PATH env var. */
  const char *path_env = getenv("PATH");
  if (!path_env) {
    return NULL;
  }
  return split_string_nano(path_env, ':', npaths);
}

/* Return a copy of the two given strings, welded together. */
// char *concatenate(const char *path, const char *name) _NOTHROW {
//   Ulong pathlen = strlen(path);
//   char *joined  = (char *)nmalloc(pathlen + strlen(name) + 1);
//   strcpy(joined, path);
//   strcpy((joined + pathlen), name);
//   return joined;
// }

/* Return a path copy of two strings, appending '/' to the prefix if needed. */
// char *concatenate_path(const char *prefix, const char *suffix) _NOTHROW {
//   Ulong prefix_len = strlen(prefix);
//   char *ret = NULL;
//   /* Either the prefix ends with / or suffix starts with /. */
//   if ((prefix[prefix_len - 1] == '/' && *suffix != '/') || (prefix[prefix_len - 1] != '/' && *suffix == '/')) {
//     ret = (char *)nmalloc(prefix_len + strlen(suffix) + 1);
//     sprintf(ret, "%s%s", prefix, suffix);
//   }
//   /* Both prefix and suffix has / at end and start. */
//   else if (prefix[prefix_len - 1] == '/' && *suffix == '/') {
//     ret = (char *)nmalloc(prefix_len + strlen(suffix));
//     sprintf(ret, "%s%s", prefix, (suffix + 1));
//   }
//   /* None have delimiter /. */
//   else {
//     ret = (char *)nmalloc(prefix_len + strlen(suffix) + 2);
//     sprintf(ret, "%s%s%s", prefix, "/", suffix);
//   }
//   return ret;
// }

/* Return a path copy of two strings, appending '/' to the prefix if needed. */
// const char *concat_path(const char *s1, const char *s2) _NOTHROW {
//   static char buf[PATH_MAX];
//   if (s1[strlen(s1) - 1] == '/') {
//     snprintf(buf, sizeof(buf), "%s%s", s1, s2);
//   }
//   else {
//     snprintf(buf, sizeof(buf), "%s%s%s", s1, "/", s2);
//   }
//   return buf;
// }

/* Return the number of digits that the given integer n takes up. */
// int digits(const long n) _NOTHROW {
//   if (n < 100000) {
//     if (n < 1000) {
//       if (n < 100) {
//         return 2;
//       }
//       else {
//         return 3;
//       }
//     }
//     else {
//       if (n < 10000) {
//         return 4;
//       }
//       else {
//         return 5;
//       }
//     }
//   }
//   else {
//     if (n < 10000000) {
//       if (n < 1000000) {
//         return 6;
//       }
//       else {
//         return 7;
//       }
//     }
//     else {
//       if (n < 100000000) {
//         return 8;
//       }
//       else {
//         return 9;
//       }
//     }
//   }
// }

/* Read an integer from the given string.  If it parses okay, store it in *result and return TRUE; otherwise, return FALSE. */
// bool parse_num(const char *string, long *result) _NOTHROW {
//   Ulong value;
//   char *excess;
//   /* Clear the error number so that we can check it afterward. */
//   errno = 0;
//   value = (long)strtol(string, &excess, 10);
//   if (errno == ERANGE || !*string || *excess) {
//     return FALSE;
//   }
//   *result = value;
//   return TRUE;
// }

/* Read one number (or two numbers separated by comma, period, or colon)
 * from the given string and store the number(s) in *line (and *column).
 * Return 'FALSE' on a failed parsing, and 'TRUE' otherwise. */
// bool parse_line_column(const char *string, long *line, long *column) _NOTHROW {
//   const char *comma;
//   char *firstpart;
//   bool retval;
//   while (*string == ' ') {
//     ++string;
//   }
//   comma = strpbrk(string, ",.:");
//   if (!comma) {
//     return parse_num(string, line);
//   }
//   retval = parse_num((comma + 1), column);
//   if (comma == string) {
//     return retval;
//   }
//   firstpart = copy_of(string);
//   firstpart[comma - string] = '\0';
//   retval = (parse_num(firstpart, line) && retval);
//   free(firstpart);
//   return retval;
// }

/* In the given string, recode each embedded NUL as a newline. */
// void recode_NUL_to_LF(char *string, Ulong length) _NOTHROW {
//   while (length > 0) {
//     if (!*string) {
//       *string = '\n';
//     }
//     --length;
//     ++string;
//   }
// }

/* In the given string, recode each embedded newline as a NUL, and return the number of bytes in the string. */
// Ulong recode_LF_to_NUL(char *string) _NOTHROW {
//   char *beginning = string;
//   while (*string) {
//     if (*string == '\n') {
//       *string = '\0';
//     }
//     ++string;
//   }
//   return (Ulong)(string - beginning);
// }

/* Free the memory of the given array, which should contain len elements. */
// void free_chararray(char **array, Ulong len) _NOTHROW {
//   if (!array) {
//     return;
//   }
//   while (len > 0) {
//     free(array[--len]);
//   }
//   free(array);
// }

/* Append an array onto 'array'.  Free 'append' but not any elements in it after call. */
// void append_chararray(char ***array, Ulong *len, char **append, Ulong append_len) _NOTHROW {
//   Ulong new_len = ((*len) + append_len);
//   *array = (char **)nrealloc(*array, (sizeof(char *) * (new_len + 1)));
//   for (Ulong i = 0; i < append_len; ++i) {
//     (*array)[(*len) + i] = append[i];
//   }
//   *len = new_len;
//   (*array)[*len] = NULL;
// }

/* Is the word starting at the given position in 'text' and of the given
 * length a separate word?  That is: is it not part of a longer word? */
// bool is_separate_word(Ulong position, Ulong length, const char *text) _NOTHROW {
//   const char *before = text + step_left(text, position);
//   const char *after  = text + position + length;
//   /* If the word starts at the beginning of the line OR the character before the word isn't a letter, and if
//    * the word ends at the end of the line OR the character after the word isn't a letter, we have a whole word. */
//   return ((position == 0 || !is_alpha_char(before)) && (*after == '\0' || !is_alpha_char(after)));
// }

/* Return the position of the needle in the haystack, or NULL if not found.
 * When searching backwards, we will find the last match that starts no later
 * than the given start; otherwise, we find the first match starting no earlier
 * than start.  If we are doing a regexp search, and we find a match, we fill
 * in the global variable regmatches with at most 9 subexpression matches. */
// const char *strstrwrapper(const char *const haystack, const char *const needle, const char *const start) _NOTHROW {
//   if (ISSET(USE_REGEXP)) {
//     if (ISSET(BACKWARDS_SEARCH)) {
//       Ulong last_find, ceiling, far_end, floor, next_rung;
//       /* The start of the search range, and the next start. */
//       floor = 0, next_rung = 0;
//       if (regexec(&search_regexp, haystack, 1, regmatches, 0)) {
//         return NULL;
//       }
//       far_end   = strlen(haystack);
//       ceiling   = (start - haystack);
//       last_find = regmatches[0].rm_so;
//       /* A result beyond the search range also means: no match. */
//       if (last_find > ceiling) {
//         return NULL;
//       }
//       /* Move the start-of-range forward until there is no more match; then the last match found is the first match backwards. */
//       while (regmatches[0].rm_so <= (int)ceiling) {
//         floor     = next_rung;
//         last_find = regmatches[0].rm_so;
//         /* If this is the last possible match, don't try to advance. */
//         if (last_find == ceiling) {
//           break;
//         }
//         next_rung = step_right(haystack, last_find);
//         regmatches[0].rm_so = next_rung;
//         regmatches[0].rm_eo = far_end;
//         if (regexec(&search_regexp, haystack, 1, regmatches, REG_STARTEND)) {
//           break;
//         }
//       }
//       /* Find the last match again, to get possible submatches. */
//       regmatches[0].rm_so = floor;
//       regmatches[0].rm_eo = far_end;
//       if (regexec(&search_regexp, haystack, 10, regmatches, REG_STARTEND)) {
//         return NULL;
//       }
//       return (haystack + regmatches[0].rm_so);
//     }
//     /* Do a forward regex search from the starting point. */
//     regmatches[0].rm_so = (start - haystack);
//     regmatches[0].rm_eo = strlen(haystack);
//     if (regexec(&search_regexp, haystack, 10, regmatches, REG_STARTEND)) {
//       return NULL;
//     }
//     else {
//       return (haystack + regmatches[0].rm_so);
//     }
//   }
//   if (ISSET(CASE_SENSITIVE)) {
//     if (ISSET(BACKWARDS_SEARCH)) {
//       return revstrstr(haystack, needle, start);
//     }
//     else {
//       return strstr(start, needle);
//     }
//   }
//   if (ISSET(BACKWARDS_SEARCH)) {
//     return mbrevstrcasestr(haystack, needle, start);
//   }
//   else {
//     return mbstrcasestr(start, needle);
//   }
// }

/* Allocate the given amount of memory and return a pointer to it. */
void *nmalloc(Ulong howmuch) _NOTHROW {
  void *section = malloc(howmuch);
  if (!section) {
    die(_("NanoX is out of memory!\n"));
  }
  return section;
}

/* Reallocate the given section of memory to have the given size. */
void *nrealloc(void *section, const Ulong howmuch) _NOTHROW {
  section = realloc(section, howmuch);
  if (!section) {
    die(_("NanoX is out of memory!\n"));
  }
  return section;
}

/* Return an appropriately reallocated dest string holding a copy of src.  Usage: "dest = mallocstrcpy(dest, src);". */
// char *mallocstrcpy(char *dest, const char *src) _NOTHROW {
//   const Ulong count = (strlen(src) + 1);
//   dest = arealloc(dest, count);
//   constexpr_strncpy(dest, src, count);
//   return dest;
// }

/* Return an allocated copy of the first count characters of the given string, and 'null-terminate' the copy. */
// char *measured_copy(const char *string, const Ulong count) _NOTHROW {
//   char *thecopy = (char *)nmalloc(count + 1);
//   memcpy(thecopy, string, count);
//   thecopy[count] = '\0';
//   return thecopy;
// }

/* Return an allocated copy of the given string. */
// char *copy_of(const char *string) _NOTHROW {
//   return measured_copy(string, strlen(string));
// }

/* Free the string at dest and return the string at src. */
// char *free_and_assign(char *dest, char *src) _NOTHROW {
//   free(dest);
//   return src;
// }

/* When not softwrapping, nano scrolls the current line horizontally by
 * chunks ("pages").  Return the column number of the first character
 * displayed in the edit window when the cursor is at the given column. */
// Ulong get_page_start(const Ulong column) _NOTHROW {
//   if (!column || (int)(column + 2) < editwincols || ISSET(SOFTWRAP)) {
//     return 0;
//   }
//   else if (editwincols > 8) {
//     return (column - 6 - (column - 6) % (editwincols - 8));
//   }
//   else {
//     return (column - (editwincols - 2));
//   }
// }

/* Return the placewewant associated with current_x, i.e. the zero-based column position of the cursor. */
// Ulong xplustabs(void) _NOTHROW {
//   return wideness(openfile->current->data, openfile->current_x);
// }

/* Return the index in text of the character that (when displayed) will not overshoot the given column. */
// Ulong actual_x(const char *text, Ulong column) _NOTHROW {
//   /* From where we start walking through the text. */
//   const char *start = text;
//   /* The current accumulated span, in columns. */
//   Ulong width = 0;
//   /* Calculated length of the current char. */
//   int charlen;
//   while (*text) {
//     charlen = advance_over(text, &width);
//     if (width > column) {
//       break;
//     }
//     text += charlen;
//   }
//   return (text - start);
// }

/* A strnlen() with tabs and multicolumn characters factored in: how many columns wide are the first maxlen bytes of text? */
// Ulong wideness(const char *text, Ulong maxlen) _NOTHROW {
//   Ulong width = 0;
//   if (!maxlen) {
//     return 0;
//   }
//   for (Ulong charlen; *text && (maxlen > (charlen = advance_over(text, &width))); maxlen -= charlen, text += charlen);
//   return width;
// }

/* Return the number of columns that the given text occupies. */
// Ulong breadth(const char *text) _NOTHROW {
//   Ulong span = 0;
//   for (; *text; text += advance_over(text, &span));
//   return span;
// }

/* Append a new magic line to the end of the buffer. */
// void new_magicline(void) _NOTHROW {
//   openfile->filebot->next = make_new_node(openfile->filebot);
//   openfile->filebot->next->data = STRLTR_COPY_OF("");
//   openfile->filebot = openfile->filebot->next;
//   ++openfile->totsize;
// }

/* Remove the magic line from the end of the buffer, if there is one and it isn't the only line in the file. */
// void remove_magicline(void) _NOTHROW {
//   if (!openfile->filebot->data[0] && openfile->filebot != openfile->filetop) {
//     if (openfile->current == openfile->filebot) {
//       openfile->current = openfile->current->prev;
//     }
//     openfile->filebot = openfile->filebot->prev;
//     delete_node(openfile->filebot->next);
//     openfile->filebot->next = NULL;
//     --openfile->totsize;
//   }
// }

/* Return 'TRUE' when the mark is before or at the cursor, and FALSE otherwise. */
// bool mark_is_before_cursor(void) _NOTHROW {
//   return (openfile->mark->lineno < openfile->current->lineno || (openfile->mark == openfile->current && openfile->mark_x <= openfile->current_x));
// }

/* Return in (top, top_x) and (bot, bot_x) the start and end "coordinates" of the marked region. */
// void get_region(linestruct **top, Ulong *top_x, linestruct **bot, Ulong *bot_x) _NOTHROW {
//   if (mark_is_before_cursor()) {
//     *top   = openfile->mark;
//     *top_x = openfile->mark_x;
//     *bot   = openfile->current;
//     *bot_x = openfile->current_x;
//   }
//   else {
//     *bot   = openfile->mark;
//     *bot_x = openfile->mark_x;
//     *top   = openfile->current;
//     *top_x = openfile->current_x;
//   }
// }

/* Get the set of lines to work on -- either just the current line, or the
 * first to last lines of the marked region.  When the cursor (or mark) is
 * at the start of the last line of the region, exclude that line. */
// void get_range(linestruct **top, linestruct **bot) _NOTHROW {
//   Ulong top_x, bot_x;
//   if (!openfile->mark) {
//     *top = openfile->current;
//     *bot = openfile->current;
//   }
//   else {
//     get_region(top, &top_x, bot, &bot_x);
//     if (bot_x == 0 && bot != top && !also_the_last) {
//       *bot = (*bot)->prev;
//     }
//     else {
//       also_the_last = TRUE;
//     }
//   }
// }

/* Return a pointer to the line that has the given line number. */
// linestruct *line_from_number(long number) _NOTHROW {
//   linestruct *line = openfile->current;
//   if (line->lineno > number) {
//     while (line->lineno != number) {
//       line = line->prev;
//     }
//   }
//   else {
//     while (line->lineno != number) {
//       line = line->next;
//     }
//   }
//   return line;
// }

/* Count the number of characters from begin to end, and return it. */
// Ulong number_of_characters_in(const linestruct *begin, const linestruct *end) _NOTHROW {
//   const linestruct *line;
//   Ulong count = 0;
//   /* Sum the number of characters (plus a newline) in each line. */
//   for (line = begin; line != end->next; line = line->next) {
//     count += (mbstrlen(line->data) + 1);
//   }
//   /* Do not count the final newline. */
//   return (count - 1);
// }

/* Return`s malloc`ed str containing pwd. */
char *alloced_pwd(void) {
  const char *pwd = getenv("PWD");
  if (!pwd) {
    logE("Failed to get pwd.");
    die("Failed to get pwd");
  }
  return copy_of(pwd);
}

/* Return`s malloc`ed str containing both substr`s, this also free`s both str_1 and str_2. */
char *alloc_str_free_substrs(char *str_1, char *str_2) _NOTHROW {
  Ulong len_1 = strlen(str_1);
  Ulong len_2 = strlen(str_2);
  char *ret = (char *)nmalloc(len_1 + len_2 + 1);
  memmove(ret, str_1, len_1);
  memmove((ret + len_1), str_2, len_2);
  ret[len_1 + len_2] = '\0';
  free(str_1);
  free(str_2);
  return ret;
}

/* Memsafe way to append 'const char *' to already malloc`ed 'char *'. */
void append_str(char **str, const char *appen_str) {
  Ulong slen      = strlen(*str);
  Ulong appendlen = strlen(appen_str);
  *str = (char *)nrealloc(*str, (slen + appendlen + 1));
  memmove((*str + slen), appen_str, appendlen);
  (*str)[slen + appendlen] = '\0';
}

/* Return`s either a malloc`ed str of the current file dir or NULL if inside the same dir. */
char *alloced_current_file_dir(void) {
  const char *slash = strrchr(openfile->filename, '/');
  if (!slash) {
    return NULL;
  }
  slash += 1;
  char *ret = (char *)nmalloc((slash - openfile->filename) + 1);
  memmove(ret, openfile->filename, (slash - openfile->filename));
  ret[(slash - openfile->filename)] = '\0';
  return ret;
}

/* Return`s the full path to the dir that current file is in,
 * for example if we open 'src/file.txt' then this will return
 * the full path to 'src' so '/full/path/to/src/'. */
char *alloced_full_current_file_dir(void) {
  char *pwd = alloced_pwd();
  append_str(&pwd, "/");
  char *current_file_dir = alloced_current_file_dir();
  if (current_file_dir) {
    char *p = alloc_str_free_substrs(pwd, current_file_dir);
    pwd     = p;
  }
  return pwd;
}

void alloced_remove_at(char **str, int at) {
  int slen = strlen(*str);
  memmove((*str + at), (*str + at + 1), (slen - at));
}

/* Return 'NULL' if 'needle' is not found by itself. */
const char *word_strstr(const char *data, const char *needle) {
  const int   slen  = strlen(needle);
  const char *found = strstr(data, needle);
  if (found) {
    if ((data[(found - data) + slen] == ' ' || data[(found - data) + slen] == '\t') &&
        (data == found || ((data[(found - data) - 1] == ' ') || data[(found - data) - 1] == '\t'))) {
      return found;
    }
  }
  return NULL;
}

/* Retrieve a malloc`ed 'char **' containing the output of cmd,
 * in line format.  This function also allows to input a refrece to
 * an 'Uint' to retrieve the line count.  Note that each line
 * is malloc`ed as well and will need to be free`d, as does the entire
 * array.  Return`s 'NULL' apon failure. */
char **retrieve_exec_output(const char *cmd, Uint *n_lines) {
  FILE *prog = popen(cmd, "r");
  if (!prog) {
    logE("Failed to run command: '%s'.", cmd);
    return NULL;
  }
  static char buf[PATH_MAX];
  Uint   size  = 0;
  Uint   cap   = 10;
  char **lines = (char **)nmalloc(sizeof(char *) * cap);
  char  *copy  = NULL;
  while (fgets(buf, sizeof(buf), prog)) {
    Uint len = strlen(buf);
    buf[len - 1] == '\n' ? buf[--len] = '\0' : 0;
    copy = measured_copy(buf, len);
    (size == cap) ? ((cap *= 2), (lines = arealloc(lines, (sizeof(char *) * cap)))) : 0;
    lines[size++] = copy;
  }
  pclose(prog);
  n_lines ? *n_lines = size : 0;
  lines[size] = NULL;
  return lines;
}

const char *word_strstr_array(const char *str, const char **substrs, Uint count, Uint *index) {
  const char *first = NULL;
  for (Uint i = 0; i < count; i++) {
    const char *match = word_strstr(str, substrs[i]);
    if (match && (!first || match < first)) {
      first = match;
      index ? *index = i : 0;
    }
  }
  return first;
}

const char *strstr_array(const char *str, const char **substrs, Uint count, Uint *index) {
  const char *first = NULL;
  for (Uint i = 0; i < count; ++i) {
    const char *match = strstr(str, substrs[i]);
    if (match && (!first || match < first)) {
      first = match;
      index ? (*index = i) : 0;
    }
  }
  return first;
}

const char *string_strstr_array(const char *str, const vector<string> &substrs, Uint *index) {
  const char *first = NULL;
  for (Uint i = 0; i < substrs.size(); i++) {
    const char *match = strstr(str, substrs[i].c_str());
    if (match && (!first || match < first)) {
      first = match;
      index ? *index = i : 0;
    }
  }
  return first;
}

// string tern_statement(const string &str, string *if_true, string *if_false) {
//   static constexpr char rule_count        = 2;
//   static const char    *rules[rule_count] = {"?", ":"};
//   const char           *start             = &str[0];
//   const char           *end               = start;
//   const char           *found             = NULL;
//   string                ret               = "";
//   Uint                  index;
//   found = strstr_array(start, rules, rule_count, &index);
//   if (!found) {
//     return "";
//   }
//   if (index != 0) {
//     return "";
//   }
//   found = end;
//   ADV_TO_NEXT_WORD(start);
//   ret = string(start, (end - start));
//   NLOG("%s\n", ret.c_str());
//   return "";
// }

/* Set the mark at specific line and column. */
// void set_mark(long lineno, Ulong pos_x) _NOTHROW {
//   if (lineno < openfile->filebot->lineno) {
//     openfile->mark = line_from_number(lineno);
//   }
//   else {
//     openfile->mark = openfile->filebot;
//   }
//   openfile->mark_x = pos_x;
// }

/* Function used for sorting strings by length.  Note that this should be passed to `qsort()` */
int qsort_strlen(const void *a, const void *b) {
  const char *lhs = *(const char **)a;
  const char *rhs = *(const char **)b;
  long lhs_len = strlen(lhs);
  long rhs_len = strlen(rhs);
  if (lhs_len == rhs_len) {
    return strcmp(lhs, rhs);
  }
  return (lhs_len - rhs_len); 
}
