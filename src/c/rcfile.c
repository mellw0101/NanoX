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

#define COLORCOUNT  34


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Whether we're allowed to add to the last syntax.  When a file ends,
 * or when a new syntax command is seen, this bool because `FALSE`. */
// _UNUSED static bool opensyntax = FALSE;
/* The syntax that is currently being parsed. */
// static syntaxtype *nanox_rc_live_syntax;
/* The end of the color list for the current syntax. */
// static colortype *nanox_rc_lastcolor = NULL;
/* Beginning and end of a list of errors in rcfiles, if any. */
static linestruct *errors_head = NULL;
static linestruct *errors_tail = NULL;

_UNUSED static const rcoption rcopts[] = {
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


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Advance over one regular expression in the line starting at ptr, null-terminate it, and return a pointer to the succeeding text. */
static char *parse_next_regex(char *ptr) {
  char *starting_point = ptr;
  int len;
  if (/* ptr != base && */ *(ptr - 1) != '"') {
    jot_error(N_("Regex strings must begin and end with a \" character"));
    return NULL;
  }
  /* Continue until the end of the line, or until a double quote followed by end-of-line or a blank. */
  while (*ptr && (*ptr != '"' || (*(ptr + char_length(ptr)) && !is_blank_char((ptr + char_length(ptr)))))) {
    ptr += char_length(ptr);
  }
  if (!*ptr) {
    jot_error(N_("Regex strings must begin and end with a \" character"));
    return NULL;
  }
  if (ptr == starting_point) {
    jot_error(N_("Empty regex string"));
    return NULL;
  }
  /* First get the length of the char at the current position. */
  len = char_length(ptr);
  /* Null-terminate the regex. */
  *ptr = '\0';
  /* Now advance the ptr to the next char. */
  ptr += len;
  /* Now advance past all blank chars. */
  while (is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return ptr;
}

/* Gather and store the string after a comment/linter command. */
static void pick_up_name(const char *const restrict kind, char *ptr, char **const storage) {
  char *look;
  if (!*ptr) {
    jot_error(N_("Missing argument after '%s'"), kind);
    return;
  }
  /* If the argument starts with a quote, find the terminating quote. */
  if (*ptr == '"') {
    look = (ptr + strlen(ptr));
    while (*look != '"') {
      if (--look == ptr) {
        jot_error(N_("Argument of '%s' lacks closing \""), kind);
        return;
      }
    }
    *look = '\0';
    ++ptr;
  }
  *storage = realloc_strcpy(*storage, ptr);
}

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

/* Set the colors for the given interface element to the given combination. */
void set_interface_color(int element, char *combotext) {
  colortype *trio;
  /* Sanity check. */
  if (element >= NUMBER_OF_ELEMENTS) {
    return;
  }
  trio = xmalloc(sizeof(colortype));
  if (parse_combination(combotext, &trio->fg, &trio->bg, &trio->attributes)) {
    free(color_combo[element]);
    color_combo[element] = trio;
  }
  else {
    free(trio);
  }
}

/* Parse an argument, with optional quotes, after a keyword that takes one.  If the
 * next word starts with a ", we say that it ends with the last " of the line.
 * Otherwise, we interpret it as usual, so that the arguments can contain "'s too. */
char *parse_argument(char *ptr) {
  ASSERT(ptr);
  const char *ptr_save = ptr;
  char *last_quote = NULL;
  int len;
  if (*ptr != '"') {
    return parse_next_word(ptr);
  }
  while (*ptr) {
    ptr += char_length(ptr);
    if (*ptr == '"') {
      last_quote = ptr;
    }
  }
  if (!last_quote) {
    jot_error(N_("Argument '%s' has an unterminated \""), ptr_save);
    return NULL;
  }
  len = char_length(last_quote);
  *last_quote = '\0';
  ptr = (last_quote + len);
  while (is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return ptr;
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

/* Compile the given regular expression and store the result in packed
 * (when this pointer is not NULL).  Return TRUE when the expression is valid. */
bool compile(const char *const restrict expression, int rex_flags, regex_t **const packed) {
  regex_t *compiled = xmalloc(sizeof(*compiled));
  int outcome = regcomp(compiled, expression, rex_flags);
  Ulong length;
  char *message;
  if (outcome != 0) {
    length  = regerror(outcome, compiled, NULL, 0);
    message = xmalloc(length);
    regerror(outcome, compiled, message, length);
    jot_error(N_("Bad regex \"%s\": %s"), expression, message);
    free(message);
    regfree(compiled);
    free(compiled);
  }
  else {
    *packed = compiled;
  }
  return (outcome == 0);
}


/* Return the index of the color that is closest to the given RGB levels, assuming that the terminal uses the
 * 6x6x6 color cube of xterm-256color. When red == green == blue, return an index in the xterm gray scale. */
short closest_index_color(short red, short green, short blue) {
  /* Translation table, from 16 intended color levels to 6 available levels. */
  static const short level[] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5};
  /* Translation table, from 14 intended gray levels to 24 available levels. */
  static const short gray[] = {1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 15, 18, 21, 23};
  if (COLORS != 256) {
    return THE_DEFAULT;
  }
  else if (red == green && green == blue && 0 < red && red < 0xF) {
    return (232 + gray[red - 1]);
  }
  else {
    return (36 * level[red] + 6 * level[green] + level[blue] + 16);
  }
}

static int hue_from_string(const char *const restrict key, Ulong *out_index) {
  ASSERT(key);
  /* Structure to hold the hue data as pairs. */
  typedef struct {
    const char *const key;
    int value;
  } HueMapEntry;
  /* The map of the hues. */
  static const HueMapEntry map[] = {
    {"red", COLOR_RED},
    {"green", COLOR_GREEN},
    {"blue", COLOR_BLUE},
    {"yellow", COLOR_YELLOW},
    {"cyan", COLOR_CYAN},
    {"magenta", COLOR_MAGENTA},
    {"white", COLOR_WHITE},
    {"black", COLOR_BLACK},
    {"normal", THE_DEFAULT},
    {"pink", COLOR_PINK},
    {"purple", COLOR_PURPLE},
    {"mauve", COLOR_MAUVE},
    {"lagoon", COLOR_LAGOON},
    {"mint", COLOR_MINT},
    {"lime", 148},
    {"peach", 215},
    {"orange", 208},
    {"latte", 137},
    {"rosy", 175},
    {"beet", 127},
    {"plum", 98},
    {"sea", 32},
    {"sky", 111},
    {"slate", 66},
    {"teal", COLOR_TEAL},
    {"sage", 107},
    {"brown", 100},
    {"ocher", 142},
    /* 'sand' Should be used for func`s and num`s. */
    {"sand", 186},
    {"tawny", 136},
    {"brick", 166},
    {"crimson", 161},
    {"grey", COLOR_BLACK + 8},
    {"gray", COLOR_BLACK + 8}
  };
  /* Iterate over all map entries. */
  for (Ulong i=0; i<ARRAY_SIZE(map); ++i) {
    /* We found a match. */
    if (strcmp(key, map[i].key) == 0) {
      /* If the user want the index of this entry, assign `i` to `*out_index`. */
      ASSIGN_IF_VALID(out_index, i);
      /* And return the given color of this entry. */
      return map[i].value;
    }
  }
  return -1;
}

/* Return the short value corresponding to the given color name, and set
 * vivid to TRUE for a lighter color, and thick for a heavier typeface. */
short color_to_short(const char *colorname, bool *vivid, bool *thick) {
  int hue;
  Ushort r, g, b;
  Ulong index;
  if (strncmp(colorname, "bright", 6) == 0 && colorname[6]) {
    *thick = TRUE;
    *vivid = TRUE;
    colorname += 6;
  }
  else if (strncmp(colorname, "light", 5) == 0 && colorname[5]) {
    *vivid = TRUE;
    *thick = FALSE;
    colorname += 5;
  }
  else {
    *vivid = FALSE;
    *thick = FALSE;
  }
  if (colorname[0] == '#' && strlen(colorname) == 4) {
    if (*vivid) {
      jot_error(N_("Color '%s' takes no prefix"), colorname);
      return BAD_COLOR;
    }
    if (sscanf(colorname, "#%1hX%1hX%1hX", &r, &g, &b) == 3) {
      return closest_index_color(r, g, b);
    }
  }
  if ((hue = hue_from_string(colorname, &index)) == -1) {
    jot_error(N_("Color \"%s\" not understood"), colorname);
    return BAD_COLOR;
  }
  else {
    if (index > 7 && *vivid) {
      jot_error(N_("Color '%s' takes no prefix"), colorname);
      return BAD_COLOR;
    }
    else if (index > 8 && COLORS < 255) {
      jot_error(N_("Current terminal only supports 16 colors"));
      return THE_DEFAULT;
    }
    else {
      return hue;
    }
  }
}

Uint syntax_opt_type_from_str(const char *const restrict key) {
  ASSERT(key);
  /* The structure of a map entry. */
  typedef struct {
    const char *const key;
    Uint value;
  } SyntaxOptTypeMapEntry;
  /* The static map that hold the data in pairs. */
  static const SyntaxOptTypeMapEntry map[] = {
    {     "color", SYNTAX_OPT_COLOR     },
    {    "icolor", SYNTAX_OPT_ICOLOR    },
    {   "comment", SYNTAX_OPT_COMMENT   },
    {  "tabgives", SYNTAX_OPT_TABGIVES  },
    {    "linter", SYNTAX_OPT_LINTER    },
    { "formatter", SYNTAX_OPT_FORMATTER }
  };
  /* Now fetch the corrent value based on `key`. */
  for (Ulong i=0; i<ARRAY_SIZE(map); ++i) {
    if (strcmp(key, map[i].key) == 0) {
      return map[i].value;
    }
  }
  return 0;
}

/* Parse the color name (or pair of color names) in the given string.
 * Return 'FALSE' when any color name is invalid; otherwise return 'TRUE'. */
bool parse_combination(char *combotext, short *fg, short *bg, int *attributes) {
  bool  vivid;
  bool  thick;
  char *comma;
  *attributes = A_NORMAL;
  if (strncmp(combotext, "bold", 4) == 0) {
    *attributes |= A_BOLD;
    if (combotext[4] != ',') {
      jot_error(N_("An attribute requires a subsequent comma"));
      return FALSE;
    }
    combotext += 5;
  }
  if (strncmp(combotext, "italic", 6) == 0) {
    *attributes |= A_ITALIC;
    if (combotext[6] != ',') {
      jot_error(N_("An attribute requires a subsequent comma"));
      return FALSE;
    }
    combotext += 7;
  }
  comma = strchr(combotext, ',');
  if (comma) {
    *comma = '\0';
  }
  if (!comma || comma > combotext) {
    *fg = color_to_short(combotext, &vivid, &thick);
    if (*fg == BAD_COLOR) {
      return FALSE;
    }
    if (vivid && !thick && COLORS > 8) {
      fg += 8;
    }
    else if (vivid) {
      *attributes |= A_BOLD;
    }
  }
  else {
    *fg = THE_DEFAULT;
  }
  if (comma) {
    *bg = color_to_short(comma + 1, vivid, thick);
    if (*bg == BAD_COLOR) {
      return FALSE;
    }
    if (vivid && COLORS > 8) {
      bg += 8;
    }
  }
  else {
    *bg = THE_DEFAULT;
  }
  return TRUE;
}

/* Read regex strings enclosed in double quotes from the line pointed
 * at by ptr, and store them quoteless in the passed storage place. */
void grab_and_store(const char *const restrict kind, char *ptr, regexlisttype **const storage) {
  regexlisttype *lastthing;
  regexlisttype *newthing;
  const char    *regexstring;
  if (!nanox_rc_opensyntax) {
    jot_error(N_("A '%s' command requires a preceding 'syntax' command"), kind);
    return;
  }
  /* The default syntax doesn't take any file matching stuff. */
  if (strcmp(nanox_rc_live_syntax->name, "default") == 0 && *ptr != '\0') {
    jot_error(N_("The \"default\" syntax does not accept '%s' regexes"), kind);
    return;
  }
  if (!*ptr) {
    jot_error(N_("Missing regex string after '%s' command"), kind);
    return;
  }
  lastthing = *storage;
  /* If there was an earlier command, go to the last of those regexes. */
  while (lastthing && lastthing->next) {
    lastthing = lastthing->next;
  }
  /* Now gather any valid regexes and add them to the linked list. */
  while (*ptr) {
    regex_t *packed_rgx = NULL;
    regexstring         = ++ptr;
    if (!(ptr = parse_next_regex(ptr))) {
      return;
    }
    /* If the regex string is malformed, skip it. */
    if (!compile(regexstring, (NANO_REG_EXTENDED | REG_NOSUB), &packed_rgx)) {
      continue;
    }
    /* Copy the regex into a struct, and hook this in at the end. */
    newthing          = xmalloc(sizeof(*newthing));
    newthing->one_rgx = packed_rgx;
    newthing->next    = NULL;
    !lastthing ? (*storage = newthing) : (lastthing->next = newthing);
    lastthing = newthing;
  }
}

/* Parse the syntax command in the given string, and set the syntax options accordingly. */
bool parse_syntax_commands(const char *keyword, char *ptr) {
  Uint syntax_opt = syntax_opt_type_from_str(keyword);
  if (!syntax_opt) {
    return FALSE;
  }
  if (syntax_opt & SYNTAX_OPT_COLOR) {
    parse_rule(ptr, NANO_REG_EXTENDED);
  }
  else if (syntax_opt & SYNTAX_OPT_ICOLOR) {
    parse_rule(ptr, NANO_REG_EXTENDED | REG_ICASE);
  }
  else if (syntax_opt & SYNTAX_OPT_COMMENT) {
    pick_up_name("comment", ptr, &nanox_rc_live_syntax->comment);
  }
  else if (syntax_opt & SYNTAX_OPT_TABGIVES) {
    pick_up_name("tabgives", ptr, &nanox_rc_live_syntax->tabstring);
  }
  else if (syntax_opt & SYNTAX_OPT_LINTER) {
    pick_up_name("linter", ptr, &nanox_rc_live_syntax->linter);
    strip_leading_blanks_from(nanox_rc_live_syntax->linter);
  }
  else if (syntax_opt & SYNTAX_OPT_FORMATTER) {
    pick_up_name("formatter", ptr, &nanox_rc_live_syntax->formatter);
    strip_leading_blanks_from(nanox_rc_live_syntax->formatter);
  }
  return TRUE;
}

/* Parse the color specification that starts at ptr, and then the one or more regexes that
 * follow it.  For each valid regex (or start=/end= regex pair), add a rule to the current syntax. */
void parse_rule(char *ptr, int rex_flags) {
  char *names;
  char *regexstring;
  short fg;
  short bg;
  int   attributes;
  if (!*ptr) {
    jot_error(N_("Missing color name"));
    return;
  }
  names = ptr;
  ptr   = parse_next_word(ptr);
  if (!parse_combination(names, &fg, &bg, &attributes)) {
    return;
  }
  if (!*ptr) {
    jot_error(N_("Missing regex string after '%s' command"), "color");
    return;
  }
  while (*ptr) {
    /* Intermediate storage for compiled regular expressions. */
    regex_t *start_rgx = NULL;
    regex_t *end_rgx   = NULL;
    /* Container for compiled regex (pair) and the color it paints. */
    colortype *newcolor = NULL;
    /* Whether it is a start=/end= regex pair. */
    bool expectend = FALSE;
    if (strncmp(ptr, "start=", 6) == 0) {
      ptr += 6;
      expectend = TRUE;
    }
    regexstring = ++ptr;
    ptr         = parse_next_regex(ptr);
    /* When there is no regex, or it is invalid, skip this line. */
    if (!ptr || !compile(regexstring, rex_flags, &start_rgx)) {
      return;
    }
    if (expectend) {
      if (strncmp(ptr, "end=", 4) != 0) {
        jot_error(N_("\"start=\" requires a corresponding \"end=\""));
        regfree(start_rgx);
        free(start_rgx);
        return;
      }
      regexstring = ptr + 5;
      ptr         = parse_next_regex(ptr + 5);
      /* When there is no valid end= regex, abandon the rule. */
      if (!ptr || !compile(regexstring, rex_flags, &end_rgx)) {
        regfree(start_rgx);
        free(start_rgx);
        return;
      }
    }
    /* Allocate a rule, fill in the data, and link it into the list. */
    newcolor             = xmalloc(sizeof(colortype));
    newcolor->start      = start_rgx;
    newcolor->end        = end_rgx;
    newcolor->fg         = fg;
    newcolor->bg         = bg;
    newcolor->attributes = attributes;
    if (!nanox_rc_lastcolor) {
      nanox_rc_live_syntax->color = newcolor;
    }
    else {
      nanox_rc_lastcolor->next = newcolor;
    }
    newcolor->next = NULL;
    nanox_rc_lastcolor      = newcolor;
    /* For a multiline rule, give it a number and increase the count. */
    if (expectend) {
      newcolor->id = nanox_rc_live_syntax->multiscore;
      ++nanox_rc_live_syntax->multiscore;
    }
  }
}
