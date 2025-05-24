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

/* Returns the value connected to `key` and assign the index of that value to `*out_index`, or if none return `-1`. */
static int hue_from_string(const char *const restrict key, Ulong *out_index) {
  ASSERT(key);
  /* Structure to hold the hue data as pairs. */
  typedef struct {
    const char *const key;
    int value;
  } MapEntry;
  /* The map of the hues. */
  static const MapEntry map[] = {
    { "red",     COLOR_RED         },
    { "green",   COLOR_GREEN       },
    { "blue",    COLOR_BLUE        },
    { "yellow",  COLOR_YELLOW      },
    { "cyan",    COLOR_CYAN        },
    { "magenta", COLOR_MAGENTA     },
    { "white",   COLOR_WHITE       },
    { "black",   COLOR_BLACK       },
    { "normal",  THE_DEFAULT       },
    { "pink",    COLOR_PINK        },
    { "purple",  COLOR_PURPLE      },
    { "mauve",   COLOR_MAUVE       },
    { "lagoon",  COLOR_LAGOON      },
    { "mint",    COLOR_MINT        },
    { "lime",    148               },
    { "peach",   215               },
    { "orange",  208               },
    { "latte",   137               },
    { "rosy",    175               },
    { "beet",    127               },
    { "plum",    98                },
    { "sea",     32                },
    { "sky",     111               },
    { "slate",   66                },
    { "teal",    COLOR_TEAL        },
    { "sage",    107               },
    { "brown",   100               },
    { "ocher",   142               },
    { "sand",    186               },
    { "tawny",   136               },
    { "brick",   166               },
    { "crimson", 161               },
    { "grey",    (COLOR_BLACK + 8) },
    { "gray",    (COLOR_BLACK + 8) }
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

/* Returns the value connected to `key`, or `-1` if none. */
static int color_opt_from_string(const char *const restrict key) {
  ASSERT(key);
  /* Map entry structure. */
  typedef struct {
    const char *const key;
    int value;
  } MapEntry;
  /* Map of color opts. */
  static const MapEntry map[] = {
    { "titlecolor",     TITLE_BAR     },
    { "numbercolor",    LINE_NUMBER   },
    { "stripecolor",    GUIDE_STRIPE  },
    { "scrollercolor",  SCROLL_BAR    },
    { "selectedcolor",  SELECTED_TEXT },
    { "spotlightcolor", SPOTLIGHTED   },
    { "minicolor",      MINI_INFOBAR  },
    { "promptcolor",    PROMPT_BAR    },
    { "statuscolor",    STATUS_BAR    },
    { "errorcolor",     ERROR_MESSAGE },
    { "keycolor",       KEY_COMBO     },
    { "functioncolor",  FUNCTION_TAG  }
  };
  /* Get the correct value based on the passed key. */
  for (Ulong i=0; i<ARRAY_SIZE(map); ++i) {
    if (strcmp(key, map[i].key) == 0) {
      return map[i].value;
    }
  }
  return -1;
}

/* Returns the value connected to `key`, or `0` if none. */
static Uint config_opt_from_string(const char *const restrict key) {
  ASSERT(key);
  /* Map entry structure. */
  typedef struct {
    const char *const key;
    Uint value;
  } MapEntry;
  /* The map holding the pairs. */
  static const MapEntry map[] = {
    { "operatingdir",  OPERATINGDIR     },
    { "fill",          FILL             },
    { "matchbrackets", MATCHBRACKETS    },
    { "whitespace",    WHITESPACE       },
    { "punct",         PUNCT            },
    { "brackets",      BRACKETS         },
    { "quotestr",      QUOTESTR         },
    { "speller",       SPELLER          },
    { "backupdir",     BACKUPDIR        },
    { "wordchars",     WORDCHARS        },
    { "guidestripe",   GUIDESTRIPE      },
    { "tabsize",       CONF_OPT_TABSIZE }
  };
  /* Get the value based on the passed `key`. */
  for (Ulong i=0; i<ARRAY_SIZE(map); ++i) {
    if (strcmp(key, map[i].key) == 0) {
      return map[i].value;
    }
  }
  return 0;
}

/* Returns the value connected to `key`, or `0` if none. */
static Uint syntax_opt_type_from_str(const char *const restrict key) {
  ASSERT(key);
  /* The structure of a map entry. */
  typedef struct {
    const char *const key;
    Uint value;
  } MapEntry;
  /* The static map that hold the data in pairs. */
  static const MapEntry map[] = {
    { "color",     SYNTAX_OPT_COLOR     },
    { "icolor",    SYNTAX_OPT_ICOLOR    },
    { "comment",   SYNTAX_OPT_COMMENT   },
    { "tabgives",  SYNTAX_OPT_TABGIVES  },
    { "linter",    SYNTAX_OPT_LINTER    },
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

/* Read and interpret one of the two nanorc files. */
static void parse_one_nanorc(void) {
  FILE *rcstream = fopen(nanox_rc_path, "rb");
  /* If opening the file succeeded, parse it.  Otherwise, only complain if the file actually exists. */
  if (rcstream) {
    parse_rcfile(rcstream, FALSE, TRUE);
  }
  else if (errno != ENOENT) {
    jot_error(N_("Error reading %s: %s"), nanox_rc_path, strerror(errno));
  }
}

/* Returns `TRUR` if we there exists a rc file at `path/name`. */
static bool have_nanorc(const char *path, const char *name) {
  if (!path) {
    return FALSE;
  }
  free(nanox_rc_path);
  nanox_rc_path = concatenate(path, name);
  return is_good_file(nanox_rc_path);
}

/* Expand globs in the passed name, and parse the resultant files. */
static void parse_includes(char *ptr) {
  char  *pattern;
  char  *expanded;
  glob_t files;
  int    result;
  check_for_nonempty_syntax();
  pattern = ptr;
  if (*pattern == '"') {
    pattern++;
  }
  ptr = parse_argument(ptr);
  if (strlen(pattern) > PATH_MAX) {
    jot_error(N_("Path is too long"));
    return;
  }
  /* Expand a tilde first, then try to match the globbing pattern. */
  expanded = real_dir_from_tilde(pattern);
  result   = glob(expanded, (GLOB_ERR | GLOB_NOCHECK), NULL, &files);
  /* If there are matches, process each of them. Otherwise, only report an error if it's something other than zero matches. */
  if (!result) {
    for (Ulong i=0; i<files.gl_pathc; ++i) {
      parse_one_include(files.gl_pathv[i], NULL);
    }
  }
  else if (result != GLOB_NOMATCH) {
    jot_error(N_("Error expanding %s: %s"), pattern, strerror(errno));
  }
  globfree(&files);
  free(expanded);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Send the gathered error messages (if any) to the terminal. */
void display_rcfile_errors(void) {
  DLIST_FOR_NEXT(errors_head, error) {
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

/* Verify that a syntax definition contains at least one color command. */
void check_for_nonempty_syntax(void) {
  if (nanox_rc_opensyntax && !nanox_rc_seen_color_command) {
    Ulong current_lineno = nanox_rc_lineno;
    nanox_rc_lineno      = nanox_rc_live_syntax->lineno;
    jot_error(N_("Syntax \"%s\" has no color commands"), nanox_rc_live_syntax->name);
    nanox_rc_lineno = current_lineno;
  }
  nanox_rc_opensyntax = FALSE;
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

/* Parse the next syntax name and its possible extension regexes from the
 * line at ptr, and add it to the global linked list of color syntaxes. */
void begin_new_syntax(char *ptr) {
  char *nameptr = ptr;
  /* Check that the syntax name is not empty. */
  if (!*ptr || (*ptr == '"' && !(*(ptr + 1) || *(ptr + 1) == '"'))) {
    jot_error(N_("Missing syntax name"));
    return;
  }
  ptr = parse_next_word(ptr);
  /* Check that quotes around the name are either paired or absent. */
  if ((*nameptr == '\x22') ^ (nameptr[strlen(nameptr) - 1] == '\x22')) {
    jot_error(N_("Unpaired quote in syntax name"));
    return;
  }
  /* If the name is quoted, strip the quotes. */
  if (*nameptr == '\x22') {
    nameptr++;
    nameptr[strlen(nameptr) - 1] = '\0';
  }
  /* Redefining the "none" syntax is not allowed. */
  if (strcmp(nameptr, "none") == 0) {
    jot_error(N_("The \"none\" syntax is reserved"));
    return;
  }
  /* Initialize a new syntax struct. */
  nanox_rc_live_syntax                = xmalloc(sizeof(syntaxtype));
  nanox_rc_live_syntax->name          = copy_of(nameptr);
  nanox_rc_live_syntax->filename      = copy_of(nanox_rc_path);
  nanox_rc_live_syntax->lineno        = nanox_rc_lineno;
  nanox_rc_live_syntax->augmentations = NULL;
  nanox_rc_live_syntax->extensions    = NULL;
  nanox_rc_live_syntax->headers       = NULL;
  nanox_rc_live_syntax->magics        = NULL;
  nanox_rc_live_syntax->linter        = NULL;
  nanox_rc_live_syntax->formatter     = NULL;
  nanox_rc_live_syntax->tabstring     = NULL;
  nanox_rc_live_syntax->comment       = copy_of(GENERAL_COMMENT_CHARACTER);
  nanox_rc_live_syntax->color         = NULL;
  nanox_rc_live_syntax->multiscore    = 0;
  /* Hook the new syntax in at the top of the list. */
  nanox_rc_live_syntax->next  = syntaxes;
  syntaxes                    = nanox_rc_live_syntax;
  nanox_rc_opensyntax         = TRUE;
  nanox_rc_seen_color_command = FALSE;
  /* The default syntax should have no associated extensions. */
  if (strcmp(nanox_rc_live_syntax->name, "default") == 0 && *ptr) {
    jot_error(N_("The \"default\" syntax does not accept extensions"));
    return;
  }
  /* If there seem to be extension regexes, pick them up. */
  if (*ptr) {
    grab_and_store("extension", ptr, &nanox_rc_live_syntax->extensions);
  }
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

/* Verify that the given file exists, is not a folder nor a device. */
bool is_good_file(const char *const restrict file) {
  struct stat rcinfo;
  /* First check that the file exists and is readable. */
  if (access(file, R_OK) != 0) {
    return FALSE;
  }
  /* If the thing exists, it may be neither a directory nor a device. */
  if (stat(file, &rcinfo) != -1 && (S_ISDIR(rcinfo.st_mode) || S_ISCHR(rcinfo.st_mode) || S_ISBLK(rcinfo.st_mode))) {
    jot_error(S_ISDIR(rcinfo.st_mode) ? N_("\"%s\" is a directory") : N_("\"%s\" is a device file"), file);
    return FALSE;
  }
  return TRUE;
}

/* Partially parse the syntaxes in the given file, or (when syntax is not NULL) fully parse one specific syntax from the file. */
void parse_one_include(char *file, syntaxtype *syntax) {
  char          *was_nanorc = nanox_rc_path;
  Ulong          was_lineno = nanox_rc_lineno;
  FILE          *rcstream;
  augmentstruct *extra;
  /* Don't open directories, character files, or block files. */
  if (access(file, R_OK) == 0 && !is_good_file(file)) {
    return;
  }
  rcstream = fopen(file, "rb");
  if (!rcstream) {
    jot_error(N_("Error reading %s: %s"), file, strerror(errno));
    return;
  }
  /* Use the name and line number position of the included syntax file while parsing it, so we can know where any errors in it are. */
  nanox_rc_path = file;
  nanox_rc_lineno = 0;
  /* If this is the first pass, parse only the prologue. */
  if (!syntax) {
    parse_rcfile(rcstream, TRUE, TRUE);
    nanox_rc_path = was_nanorc;
    nanox_rc_lineno = was_lineno;
    return;
  }
  nanox_rc_live_syntax = syntax;
  nanox_rc_lastcolor   = NULL;
  /* Fully parse the given syntax (as it is about to be used). */
  parse_rcfile(rcstream, TRUE, FALSE);
  extra = syntax->augmentations;
  /* Apply any stored extendsyntax commands. */
  while (extra) {
    char *keyword = extra->data;
    char *therest = parse_next_word(extra->data);
    nanox_rc_path        = extra->filename;
    nanox_rc_lineno        = extra->lineno;
    if (!parse_syntax_commands(keyword, therest)) {
      jot_error(N_("Command \"%s\" not understood"), keyword);
    }
    extra = extra->next;
  }
  free(syntax->filename);
  syntax->filename = NULL;
  nanox_rc_path    = was_nanorc;
  nanox_rc_lineno  = was_lineno;
}

/* Parse the rcfile, once it has been opened successfully at rcstream, and close it afterwards.
 * If just_syntax is TRUE, allow the file to to contain only color syntax commands. */
void parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only) {
  char *buffer = NULL;
  Ulong size   = 0;
  long  length = 0;
  while ((length = getline(&buffer, &size, rcstream)) > 0) {
    char *ptr, *keyword, *option, *argument;
    bool  drop_open = FALSE;
    int   set       = 0;
    Ulong i;
    nanox_rc_lineno++;
    /* If doing a full parse, skip to after the 'syntax' command. */
    if (just_syntax && !intros_only && nanox_rc_lineno <= nanox_rc_live_syntax->lineno) {
      continue;
    }
    /* Strip the terminating newline and possibly a carriage return. */
    if (buffer[length - 1] == '\n') {
      buffer[--length] = '\0';
    }
    if (length > 0 && buffer[length - 1] == '\r') {
      buffer[--length] = '\0';
    }
    ptr = buffer;
    while (isblank((Uchar)*ptr)) {
      ptr++;
    }
    /* If the line is empty or a comment, skip to next line. */
    if (!*ptr || *ptr == '#') {
      continue;
    }
    /* Otherwise, skip to the next space. */
    keyword = ptr;
    ptr     = parse_next_word(ptr);
    /* Handle extending first... */
    if (!just_syntax && strcmp(keyword, "extendsyntax") == 0) {
      augmentstruct *newitem, *extra;
      char          *syntaxname = ptr;
      syntaxtype    *sntx;
      check_for_nonempty_syntax();
      ptr = parse_next_word(ptr);
      for (sntx = syntaxes; sntx; sntx = sntx->next) {
        if (strcmp(sntx->name, syntaxname) == 0) {
          break;
        }
      }
      if (!sntx) {
        jot_error(N_("Could not find syntax \"%s\" to extend"), syntaxname);
        continue;
      }
      keyword  = ptr;
      argument = copy_of(ptr);
      ptr      = parse_next_word(ptr);
      /* File-matching commands need to be processed immediately;
       * other commands are stored for possible later processing. */
      if (strcmp(keyword, "header") == 0 || strcmp(keyword, "magic") == 0) {
        free(argument);
        nanox_rc_live_syntax = sntx;
        nanox_rc_opensyntax  = TRUE;
        drop_open   = TRUE;
      }
      else {
        newitem           = xmalloc(sizeof(augmentstruct));
        newitem->filename = copy_of(nanox_rc_path);
        newitem->lineno   = nanox_rc_lineno;
        newitem->data     = argument;
        newitem->next     = NULL;
        if (sntx->augmentations) {
          extra = sntx->augmentations;
          while (extra->next) {
            extra = extra->next;
          }
          extra->next = newitem;
        }
        else {
          sntx->augmentations = newitem;
        }
        continue;
      }
    }
    /* Try to parse the keyword. */
    if (strcmp(keyword, "syntax") == 0) {
      if (intros_only) {
        check_for_nonempty_syntax();
        begin_new_syntax(ptr);
      }
      else {
        break;
      }
    }
    else if (strcmp(keyword, "header") == 0) {
      if (intros_only) {
        grab_and_store("header", ptr, &nanox_rc_live_syntax->headers);
      }
    }
    else if (strcmp(keyword, "magic") == 0) {
#ifdef HAVE_LIBMAGIC
      if (intros_only) {
        grab_and_store("magic", ptr, &live_syntax->magics);
      }
#endif
    }
    else if (just_syntax && (strcmp(keyword, "set") == 0 || strcmp(keyword, "unset") == 0 ||
                             strcmp(keyword, "bind") == 0 || strcmp(keyword, "unbind") == 0 ||
                             strcmp(keyword, "include") == 0 || strcmp(keyword, "extendsyntax") == 0)) {
      if (intros_only) {
        jot_error(N_("Command \"%s\" not allowed in included file"), keyword);
      }
      else {
        break;
      }
    }
    else if (intros_only && (strcmp(keyword, "color") == 0 || strcmp(keyword, "icolor") == 0 ||
                             strcmp(keyword, "comment") == 0 || strcmp(keyword, "tabgives") == 0 ||
                             strcmp(keyword, "linter") == 0 || strcmp(keyword, "formatter") == 0)) {
      if (!nanox_rc_opensyntax) {
        jot_error(N_("A '%s' command requires a preceding 'syntax' command"), keyword);
      }
      if (strstr("icolor", keyword)) {
        nanox_rc_seen_color_command = TRUE;
      }
      continue;
    }
    else if (parse_syntax_commands(keyword, ptr)) {
      ;
    }
    else if (strcmp(keyword, "include") == 0) {
      parse_includes(ptr);
    }
    else {
      if (strcmp(keyword, "set") == 0) {
        set = 1;
      }
      else if (strcmp(keyword, "unset") == 0) {
        set = -1;
      }
      else if (strcmp(keyword, "bind") == 0) {
        parse_binding(ptr, TRUE);
      }
      else if (strcmp(keyword, "unbind") == 0) {
        parse_binding(ptr, FALSE);
      }
      else if (intros_only) {
        jot_error(N_("Command \"%s\" not understood"), keyword);
      }
    }
    if (drop_open) {
      nanox_rc_opensyntax = FALSE;
    }
    if (set == 0) {
      continue;
    }
    check_for_nonempty_syntax();
    if (*ptr == '\0') {
      jot_error(N_("Missing option"));
      continue;
    }
    option = ptr;
    ptr    = parse_next_word(ptr);
    /* Find the just parsed option name among the existing names. */
    for (i = 0; rcopts[i].name; i++) {
      if (strcmp(option, rcopts[i].name) == 0) {
        break;
      }
    }
    if (!rcopts[i].name) {
      jot_error(N_("Unknown option: %s"), option);
      continue;
    }
    /* If the option has a flag, set it or unset it, as requested. */
    if (rcopts[i].flag) {
      if (set == 1) {
        SET(rcopts[i].flag);
      }
      else {
        UNSET(rcopts[i].flag);
      }
      continue;
    }
    /* An option that takes an argument cannot be unset. */
    if (set == -1) {
      jot_error(N_("Cannot unset option \"%s\""), option);
      continue;
    }
    if (*ptr == '\0') {
      jot_error(N_("Option \"%s\" requires an argument"), option);
      continue;
    }
    argument = ptr;
    if (*argument == '"') {
      argument++;
    }
    ptr = parse_argument(ptr);
    /* When in a UTF-8 locale, ignore arguments with invalid sequences. */
    if (using_utf8() && mbstowcs(NULL, argument, 0) == (Ulong)-1) {
      jot_error(N_("Argument is not a valid multibyte string"));
      continue;
    }
    // const int colorOption = retriveColorOptionFromStr(option);
    int colorOption = color_opt_from_string(option);
    (colorOption != -1) ? set_interface_color(colorOption, argument) : ((void)0);
    Uint configOption = config_opt_from_string(option);
    // const Uint configOption = retriveConfigOptionFromStr(option);
    if (!configOption) {
      ;
    }
    else if (configOption & OPERATINGDIR) {
      operating_dir = realloc_strcpy(operating_dir, argument);
    }
    else if (configOption & FILL) {
      if (!parse_num(argument, &fill)) {
        jot_error(N_("Requested fill size \"%s\" is invalid"), argument);
        fill = -COLUMNS_FROM_EOL;
      }
    }
    else if (configOption & MATCHBRACKETS) {
      if (has_blank_char(argument)) {
        jot_error(N_("Non-blank characters required"));
      }
      else if (mbstrlen(argument) % 2 != 0) {
        jot_error(N_("Even number of characters required"));
      }
      else {
        matchbrackets = realloc_strcpy(matchbrackets, argument);
      }
    }
    else if (configOption & WHITESPACE) {
      if (mbstrlen(argument) != 2 || breadth(argument) != 2) {
        jot_error(N_("Two single-column characters required"));
      }
      else {
        whitespace  = realloc_strcpy(whitespace, argument);
        whitelen[0] = char_length(whitespace);
        whitelen[1] = char_length(whitespace + whitelen[0]);
      }
    }
    else if (configOption & PUNCT) {
      if (has_blank_char(argument)) {
        jot_error(N_("Non-blank characters required"));
      }
      else {
        punct = realloc_strcpy(punct, argument);
      }
    }
    else if (configOption & BRACKETS) {
      if (has_blank_char(argument)) {
        jot_error(N_("Non-blank characters required"));
      }
      else {
        brackets = realloc_strcpy(brackets, argument);
      }
    }
    else if (configOption & QUOTESTR) {
      quotestr = realloc_strcpy(quotestr, argument);
    }
    else if (configOption & SPELLER) {
      alt_speller = realloc_strcpy(alt_speller, argument);
    }
    else if (configOption & BACKUPDIR) {
      backup_dir = realloc_strcpy(backup_dir, argument);
    }
    else if (configOption & WORDCHARS) {
      word_chars = realloc_strcpy(word_chars, argument);
    }
    else if (configOption & GUIDESTRIPE) {
      if (!parse_num(argument, &stripe_column) || stripe_column <= 0) {
        jot_error(N_("Guide column \"%s\" is invalid"), argument);
        stripe_column = 0;
      }
    }
    else if (configOption & CONF_OPT_TABSIZE) {
      if (!parse_num(argument, &tabsize) || tabsize <= 0) {
        jot_error(N_("Requested tab size \"%s\" is invalid"), argument);
        tabsize = -1;
      }
    }
  }
  if (intros_only) {
    check_for_nonempty_syntax();
  }
  fclose(rcstream);
  free(buffer);
  nanox_rc_lineno = 0;
}

/* Process the nanorc file that was specified on the command line (if any),
 * and otherwise the system-wide rcfile followed by the user's rcfile. */
void do_rcfiles(void) {
  if (custom_nanorc) {
    nanox_rc_path = get_full_path(custom_nanorc);
    if (!nanox_rc_path || access(nanox_rc_path, F_OK) != 0) {
      die(_("Specified rcfile does not exist\n"));
    }
  }
  else {
    nanox_rc_path = realloc_strcpy(nanox_rc_path, SYSCONFDIR "/nanorc");
  }
  if (is_good_file(nanox_rc_path)) {
    parse_one_nanorc();
  }
  if (!custom_nanorc) {
    const char *xdgconfdir = getenv("XDG_CONFIG_HOME");
    get_homedir();
    /* Now try to find a nanorc file in the user's home directory or in the XDG configuration directories, and process the first one found. */
    if (have_nanorc(homedir, "/" HOME_RC_NAME) || have_nanorc(xdgconfdir, "/nano/" RCFILE_NAME) || have_nanorc(homedir, "/.config/nano/" RCFILE_NAME)) {
      parse_one_nanorc();
    }
    else if (!homedir && !xdgconfdir) {
      jot_error(N_("I can't find my home directory!  Wah!"));
    }
  }
  check_vitals_mapped();
  free(nanox_rc_path);
  nanox_rc_path = NULL;
}
