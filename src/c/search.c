/** @file search.c

  @author  Melwin Svensson.
  @date    21-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Whether we have compiled a regular expression for the search. */
static bool have_compiled_regexp = FALSE;


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Compile the given regular expression and store it in search_regexp.  Return `TRUE` if the expression is valid, and `FALSE` otherwise. */
bool regexp_init(const char *regexp) {
  Ulong len;
  char *str;
  int value = regcomp(&search_regexp, regexp, NANO_REG_EXTENDED | (ISSET(CASE_SENSITIVE) ? 0 : REG_ICASE));
  /* If regex compilation failed, show the error message. */
  if (value != 0) {
    len = regerror(value, &search_regexp, NULL, 0);
    str = xmalloc(len);
    regerror(value, &search_regexp, str, len);
    statusline(AHEM, _("Bad regex \"%s\": %s"), regexp, str);
    free(str);
    return FALSE;
  }
  have_compiled_regexp = TRUE;
  return TRUE;
}

/* Free a compiled regular expression, if one was compiled; and schedule a
 * full screen refresh when the mark is on, in case the cursor has moved. */
void tidy_up_after_search(void) {
  if (have_compiled_regexp) {
    regfree(&search_regexp);
    have_compiled_regexp = FALSE;
  }
  if (openfile->mark) {
    refresh_needed = TRUE;
  }
  recook |= perturbed;
}

/* Go to the specified `line-number` and `x` position, in `file`. */
void goto_line_posx_for(openfilestruct *const file, long lineno, Ulong x, int total_rows) {
  ASSERT(file);
  if (lineno > (file->edittop->lineno + total_rows) || (ISSET(SOFTWRAP) && lineno > file->current->lineno)) {
    recook |= perturbed;
  }
  file->current     = line_from_number_for(file, lineno);
  file->current_x   = x;
  set_pww_for(file);
  refresh_needed    = TRUE;
}

/* Go to the specified `line-number` and `x` position, in the currently open buffer. */
void goto_line_posx(long lineno, Ulong x) {
  goto_line_posx_for(CONTEXT_OPENFILE, lineno, x, CONTEXT_ROWS);
}

/* Report on the status bar that the given string was not found. */
void not_found_msg(const char *const restrict str) {
  char *disp     = display_string(str, 0, ((COLS / 2) + 1), FALSE, FALSE);
  Ulong numchars = actual_x(disp, wideness(disp, (COLS / 2)));
  statusline(AHEM, _("\"%.*s%s\" not found"), numchars, disp, (disp[numchars] == '\0') ? "" : "...");
  free(disp);
}
