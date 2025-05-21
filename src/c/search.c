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
  int value = regcomp(&search_regexp, regexp, NANO_REG_EXTENDED | (ISSET(CASE_SENSITIVE) ? 0 : REG_ICASE));
  /* If regex compilation failed, show the error message. */
  if (value != 0) {
    Ulong len = regerror(value, &search_regexp, NULL, 0);
    char *str = xmalloc(len);
    regerror(value, &search_regexp, str, len);
    print_status(AHEM, _("Bad regex \"%s\": %s"), regexp, str);
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
