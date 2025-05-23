/** @file rcfile.c

  @author  Melwin Svensson.
  @date    22-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#ifndef RCFILE_NAME
# define HOME_RC_NAME  ".nanoxrc"
# define RCFILE_NAME   "nanoxrc"
#else
# define HOME_RC_NAME  RCFILE_NAME
#endif

#ifdef MAXSIZE
# undef MAXSIZE
#endif

#define MAXSIZE  (PATH_MAX + 200)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Whether we're allowed to add to the last syntax.  When a file ends,
 * or when a new syntax command is seen, this bool because `FALSE`. */
static bool opensyntax = FALSE;
/* Beginning and end of a list of errors in rcfiles, if any. */
static linestruct *errors_head = NULL;
static linestruct *errors_tail = NULL;

static const rcoption rcopts[] = {
  {             "boldtext",        BOLD_TEXT},
  {             "brackets",                0},
  {       "breaklonglines", BREAK_LONG_LINES},
  {        "casesensitive",   CASE_SENSITIVE},
  {         "constantshow",    CONSTANT_SHOW},
  {                 "fill",                0},
  {           "historylog",       HISTORYLOG},
  {          "linenumbers",     LINE_NUMBERS},
  {                "magic",        USE_MAGIC},
  {                "mouse",        USE_MOUSE},
  {          "multibuffer",      MULTIBUFFER},
  {               "nohelp",          NO_HELP},
  {           "nonewlines",      NO_NEWLINES},
  {               "nowrap",          NO_WRAP}, /* Deprecated; remove in 2027. */
  {         "operatingdir",                0},
  {          "positionlog",      POSITIONLOG},
  {             "preserve",         PRESERVE},
  {                "punct",                0},
  {             "quotestr",                0},
  {           "quickblank",      QUICK_BLANK},
  {         "rawsequences",    RAW_SEQUENCES},
  {         "rebinddelete",    REBIND_DELETE},
  {               "regexp",       USE_REGEXP},
  {           "saveonexit",     SAVE_ON_EXIT},
  {              "speller",                0},
  {            "afterends",       AFTER_ENDS},
  {"allow_insecure_backup",  INSECURE_BACKUP},
  {             "atblanks",        AT_BLANKS},
  {           "autoindent",       AUTOINDENT},
  {               "backup",      MAKE_BACKUP},
  {            "backupdir",                0},
  {            "bookstyle",        BOOKSTYLE},
  {         "colonparsing",    COLON_PARSING},
  {        "cutfromcursor",  CUT_FROM_CURSOR},
  {            "emptyline",       EMPTY_LINE},
  {          "guidestripe",                0},
  {            "indicator",        INDICATOR},
  {       "jumpyscrolling",  JUMPY_SCROLLING},
  {              "locking",          LOCKING},
  {        "matchbrackets",                0},
  {              "minibar",          MINIBAR},
  {            "noconvert",       NO_CONVERT},
  {           "showcursor",      SHOW_CURSOR},
  {            "smarthome",       SMART_HOME},
  {             "softwrap",         SOFTWRAP},
  {           "stateflags",       STATEFLAGS},
  {              "tabsize",                0},
  {         "tabstospaces",   TABS_TO_SPACES},
  {           "trimblanks",      TRIM_BLANKS},
  {                 "unix",     MAKE_IT_UNIX},
  {           "whitespace",                0},
  {           "wordbounds",      WORD_BOUNDS},
  {            "wordchars",                0},
  {                  "zap",     LET_THEM_ZAP},
  {                 "zero",             ZERO},
  {           "titlecolor",                0},
  {          "numbercolor",                0},
  {          "stripecolor",                0},
  {        "scrollercolor",                0},
  {        "selectedcolor",                0},
  {       "spotlightcolor",                0},
  {            "minicolor",                0},
  {          "promptcolor",                0},
  {          "statuscolor",                0},
  {           "errorcolor",                0},
  {             "keycolor",                0},
  {        "functioncolor",                0},
  {                   NULL,                0}
};


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Send the gathered error messages (if any) to the terminal. */
void display_rcfile_errors(void) {
  FOR_EACH_LINE_NEXT(error, errors_head) {
    writeferr("%s\n", error->data);
  }
}

/* Store the given error message in a linked list, to be printed upon exit. */
void jot_error(const char *const restrict format, ...) {
  linestruct *error = make_new_node(errors_tail);
  va_list ap;
  char textbuf[MAXSIZE];
  int length = 0;
  !errors_head ? (errors_head = error) : (errors_tail->next = error);
  errors_tail = error;
  if (!startup_problem) {
    if (nanox_rc_path) {
      snprintf(textbuf, MAXSIZE, _("Mistakes in `%s`"), nanox_rc_path);
      startup_problem = copy_of(textbuf);
    }
    else {
      startup_problem = copy_of(_("Problems with history file"));
    }
  }
  if (nanox_rc_lineno > 0) {
    length = snprintf(textbuf, MAXSIZE, _("Error in '%s' on line: %lu: "), nanox_rc_path, nanox_rc_lineno);
  }
  va_start(ap, format);
  length += vsnprintf((textbuf + length), (MAXSIZE - length), _(format), ap);
  va_end(ap);
  error->data = measured_copy(textbuf, length);
}

/* Parse the next word from the string, null-terminate it, and return a pointer to the first character
 * after the null terminator.  The returned pointer will point to '\0' if we hit the end of the line. */
char *parse_next_word(char *ptr) {
  ASSERT(ptr);
  int len;
  while (!is_blank_char(ptr) && *ptr) {
    ptr += char_length(ptr);
  }
  if (!*ptr) {
    return ptr;
  }
  /* Get the length of the char we will `null-terminate`. */
  len = char_length(ptr);
  /* `Null-terminate` the ptr. */
  *(ptr) = '\0';
  /* Then advance it by the length. */
  ptr += len;
  while (is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return ptr;
}
