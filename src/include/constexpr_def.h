/** @file constexpr_def.h */
#pragma once
/* clang-format off */

#include <Mlib/constexpr.hpp>
#include "c_proto.h"

using std::string_view;

/* Identifiers for color options. */
// #define TITLE_BAR     0
// #define LINE_NUMBER   1
// #define GUIDE_STRIPE  2
// #define SCROLL_BAR    3
// #define SELECTED_TEXT 4
// #define SPOTLIGHTED   5
// #define MINI_INFOBAR  6
// #define PROMPT_BAR    7
// #define STATUS_BAR    8
// #define ERROR_MESSAGE 9
// #define KEY_COMBO     10
// #define FUNCTION_TAG  11
/* The color options map. */
constexpr_map<string_view, Uchar, 12> color_option_map = {
  {{"titlecolor", TITLE_BAR},
    {"numbercolor", LINE_NUMBER},
    {"stripecolor", GUIDE_STRIPE},
    {"scrollercolor", SCROLL_BAR},
    {"selectedcolor", SELECTED_TEXT},
    {"spotlightcolor", SPOTLIGHTED},
    {"minicolor", MINI_INFOBAR},
    {"promptcolor", PROMPT_BAR},
    {"statuscolor", STATUS_BAR},
    {"errorcolor", ERROR_MESSAGE},
    {"keycolor", KEY_COMBO},
    {"functioncolor", FUNCTION_TAG}}
};
/* Function to retrive the color option from a string literal. */
constexpr int retriveColorOptionFromStr(string_view str) {
  for (const auto &[key, val] : color_option_map) {
    if (key == str) {
      return val;
    }
  }
  return (Uint)-1;
}

/* Identifiers for the different configuration options. */
// #define OPERATINGDIR     (1 << 0)
// #define FILL             (1 << 1)
// #define MATCHBRACKETS    (1 << 2)
// #define WHITESPACE       (1 << 3)
// #define PUNCT            (1 << 4)
// #define BRACKETS         (1 << 5)
// #define QUOTESTR         (1 << 6)
// #define SPELLER          (1 << 7)
// #define BACKUPDIR        (1 << 8)
// #define WORDCHARS        (1 << 9)
// #define GUIDESTRIPE      (1 << 10)
// #define CONF_OPT_TABSIZE (1 << 11)
/* The configuration options map. */
constexpr_map<string_view, Ushort, 12> config_option_map = {{
  {  "operatingdir", OPERATINGDIR     },
  {          "fill", FILL             },
  { "matchbrackets", MATCHBRACKETS    },
  {    "whitespace", WHITESPACE       },
  {         "punct", PUNCT            },
  {      "brackets", BRACKETS         },
  {      "quotestr", QUOTESTR         },
  {       "speller", SPELLER          },
  {     "backupdir", BACKUPDIR        },
  {     "wordchars", WORDCHARS        },
  {   "guidestripe", GUIDESTRIPE      },
  {       "tabsize", CONF_OPT_TABSIZE }
}};
/* Function to retrive a color option from a string literal. */
constexpr int retriveConfigOptionFromStr(string_view str) {
  for (const auto &[key, val] : config_option_map) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

// /* Identifiers for the different syntax options. */
// #define SYNTAX_OPT_COLOR     (1 << 0)
// #define SYNTAX_OPT_ICOLOR    (1 << 1)
// #define SYNTAX_OPT_COMMENT   (1 << 2)
// #define SYNTAX_OPT_TABGIVES  (1 << 3)
// #define SYNTAX_OPT_LINTER    (1 << 4)
// #define SYNTAX_OPT_FORMATTER (1 << 5)
/* The syntax options map. */
constexpr_map<string_view, Uchar, 6> syntax_option_map = {{
  {     "color", SYNTAX_OPT_COLOR     },
  {    "icolor", SYNTAX_OPT_ICOLOR    },
  {   "comment", SYNTAX_OPT_COMMENT   },
  {  "tabgives", SYNTAX_OPT_TABGIVES  },
  {    "linter", SYNTAX_OPT_LINTER    },
  { "formatter", SYNTAX_OPT_FORMATTER }
}};
/* Function to retrive a syntax option from a string literal. */
constexpr Uchar retriveSyntaxOptionFromStr(string_view str) {
  for (const auto &[key, val] : syntax_option_map) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

// /* Identifiers for the different flags. */
// typedef enum {
//   /* This flag is not used as this is part of a bitfield and 0 is not a unique
//   * value, in terms of bitwise operations as the default all non set value is 0. */
//   DONTUSE,
//   #define DONTUSE DONTUSE
//   CASE_SENSITIVE,
//   #define CASE_SENSITIVE CASE_SENSITIVE
//   CONSTANT_SHOW,
//   #define CONSTANT_SHOW CONSTANT_SHOW
//   NO_HELP,
//   #define NO_HELP NO_HELP
//   NO_WRAP,
//   #define NO_WRAP NO_WRAP
//   AUTOINDENT,
//   #define AUTOINDENT AUTOINDENT
//   VIEW_MODE,
//   #define VIEW_MODE VIEW_MODE
//   USE_MOUSE,
//   #define USE_MOUSE USE_MOUSE
//   USE_REGEXP,
//   #define USE_REGEXP USE_REGEXP
//   SAVE_ON_EXIT,
//   #define SAVE_ON_EXIT SAVE_ON_EXIT
//   CUT_FROM_CURSOR,
//   #define CUT_FROM_CURSOR CUT_FROM_CURSOR
//   BACKWARDS_SEARCH,
//   #define BACKWARDS_SEARCH BACKWARDS_SEARCH
//   MULTIBUFFER,
//   #define MULTIBUFFER MULTIBUFFER
//   REBIND_DELETE,
//   #define REBIND_DELETE REBIND_DELETE
//   RAW_SEQUENCES,
//   #define RAW_SEQUENCES RAW_SEQUENCES
//   NO_CONVERT,
//   #define NO_CONVERT NO_CONVERT
//   MAKE_BACKUP,
//   #define MAKE_BACKUP MAKE_BACKUP
//   INSECURE_BACKUP,
//   #define INSECURE_BACKUP INSECURE_BACKUP
//   NO_SYNTAX,
//   #define NO_SYNTAX NO_SYNTAX
//   PRESERVE,
//   #define PRESERVE PRESERVE
//   HISTORYLOG,
//   #define HISTORYLOG HISTORYLOG
//   RESTRICTED,
//   #define RESTRICTED RESTRICTED
//   SMART_HOME,
//   #define SMART_HOME SMART_HOME
//   WHITESPACE_DISPLAY,
//   #define WHITESPACE_DISPLAY WHITESPACE_DISPLAY
//   TABS_TO_SPACES,
//   #define TABS_TO_SPACES TABS_TO_SPACES
//   QUICK_BLANK,
//   #define QUICK_BLANK QUICK_BLANK
//   WORD_BOUNDS,
//   #define WORD_BOUNDS WORD_BOUNDS
//   NO_NEWLINES,
//   #define NO_NEWLINES NO_NEWLINES
//   BOLD_TEXT,
//   #define BOLD_TEXT BOLD_TEXT
//   SOFTWRAP,
//   #define SOFTWRAP SOFTWRAP
//   POSITIONLOG,
//   #define POSITIONLOG POSITIONLOG
//   LOCKING,
//   #define LOCKING LOCKING
//   NOREAD_MODE,
//   #define NOREAD_MODE NOREAD_MODE
//   MAKE_IT_UNIX,
//   #define MAKE_IT_UNIX MAKE_IT_UNIX
//   TRIM_BLANKS,
//   #define TRIM_BLANKS TRIM_BLANKS
//   SHOW_CURSOR,
//   #define SHOW_CURSOR SHOW_CURSOR
//   LINE_NUMBERS,
//   #define LINE_NUMBERS LINE_NUMBERS
//   AT_BLANKS,
//   #define AT_BLANKS AT_BLANKS
//   AFTER_ENDS,
//   #define AFTER_ENDS AFTER_ENDS
//   LET_THEM_ZAP,
//   #define LET_THEM_ZAP LET_THEM_ZAP
//   BREAK_LONG_LINES,
//   #define BREAK_LONG_LINES BREAK_LONG_LINES
//   JUMPY_SCROLLING,
//   #define JUMPY_SCROLLING JUMPY_SCROLLING
//   EMPTY_LINE,
//   #define EMPTY_LINE EMPTY_LINE
//   INDICATOR,
//   #define INDICATOR INDICATOR
//   BOOKSTYLE,
//   #define BOOKSTYLE BOOKSTYLE
//   COLON_PARSING,
//   #define COLON_PARSING COLON_PARSING
//   STATEFLAGS,
//   #define STATEFLAGS STATEFLAGS
//   USE_MAGIC,
//   #define USE_MAGIC USE_MAGIC
//   MINIBAR,
//   #define MINIBAR MINIBAR
//   ZERO,
//   #define ZERO ZERO
//   MODERN_BINDINGS,
//   #define MODERN_BINDINGS MODERN_BINDINGS
//   EXPERIMENTAL_FAST_LIVE_SYNTAX,
//   #define EXPERIMENTAL_FAST_LIVE_SYNTAX EXPERIMENTAL_FAST_LIVE_SYNTAX
//   SUGGEST,
//   #define SUGGEST SUGGEST
//   SUGGEST_INLINE,
//   #define SUGGEST_INLINE SUGGEST_INLINE
//   USING_GUI,
//   #define USING_GUI USING_GUI
//   NO_NCURSES,
//   #define NO_NCURSES NO_NCURSES
// } flag_type;
/* The flag map. */
constexpr_map<string_view, Uchar, 94> flagOptionsMap = {{
  {               "-A", SMART_HOME      },
  {      "--smarthome", SMART_HOME      },
  {               "-B", MAKE_BACKUP     },
  {         "--backup", MAKE_BACKUP     },
  {               "-C", INSECURE_BACKUP },
  {      "--backupdir", INSECURE_BACKUP },
  {               "-D", BOLD_TEXT       },
  {       "--boldtext", BOLD_TEXT       },
  {               "-E", TABS_TO_SPACES  },
  {   "--tabstospaces", TABS_TO_SPACES  },
  {               "-F", MULTIBUFFER     },
  {    "--multibuffer", MULTIBUFFER     },
  {               "-G", LOCKING         },
  {        "--locking", LOCKING         },
  {               "-H", HISTORYLOG      },
  {     "--historylog", HISTORYLOG      },
  {               "-J", NO_WRAP         },
  {    "--guidestripe", NO_WRAP         },
  {               "-K", RAW_SEQUENCES   },
  {   "--rawsequences", RAW_SEQUENCES   },
  {               "-L", NO_NEWLINES     },
  {     "--nonewlines", NO_NEWLINES     },
  {               "-M", TRIM_BLANKS     },
  {     "--trimblanks", TRIM_BLANKS     },
  {               "-N", NO_CONVERT      },
  {      "--noconvert", NO_CONVERT      },
  {               "-O", BOOKSTYLE       },
  {      "--bookstyle", BOOKSTYLE       },
  {               "-P", POSITIONLOG     },
  {    "--positionlog", POSITIONLOG     },
  {               "-Q", NO_SYNTAX       },
  {       "--quotestr", NO_SYNTAX       },
  {               "-R", RESTRICTED      },
  {     "--restricted", RESTRICTED      },
  {               "-S", SOFTWRAP        },
  {       "--softwrap", SOFTWRAP        },
  {               "-U", QUICK_BLANK     },
  {     "--quickblank", QUICK_BLANK     },
  {               "-W", WORD_BOUNDS     },
  {     "--wordbounds", WORD_BOUNDS     },
  {               "-Z", LET_THEM_ZAP    },
  {            "--zap", LET_THEM_ZAP    },
  {               "-a", AT_BLANKS       },
  {       "--atblanks", AT_BLANKS       },
  {               "-c", CONSTANT_SHOW   },
  {   "--constantshow", CONSTANT_SHOW   },
  {               "-d", REBIND_DELETE   },
  {   "--rebinddelete", REBIND_DELETE   },
  {               "-e", EMPTY_LINE      },
  {      "--emptyline", EMPTY_LINE      },
  {               "-g", SHOW_CURSOR     },
  {     "--showcursor", SHOW_CURSOR     },
  {               "-h", NO_HELP         },
  {           "--help", NO_HELP         },
  {               "-i", AUTOINDENT      },
  {     "--autoindent", AUTOINDENT      },
  {               "-j", JUMPY_SCROLLING },
  { "--jumpyscrolling", JUMPY_SCROLLING },
  {               "-k", CUT_FROM_CURSOR },
  {  "--cutfromcursor", CUT_FROM_CURSOR },
  {               "-l", LINE_NUMBERS    },
  {    "--linenumbers", LINE_NUMBERS    },
  {               "-m", USE_MOUSE       },
  {          "--mouse", USE_MOUSE       },
  {               "-n", NOREAD_MODE     },
  {         "--noread", NOREAD_MODE     },
  {               "-p", PRESERVE        },
  {       "--preserve", PRESERVE        },
  {               "-q", INDICATOR       },
  {      "--indicator", INDICATOR       },
  {               "-t", SAVE_ON_EXIT    },
  {     "--saveonexit", SAVE_ON_EXIT    },
  {               "-u", MAKE_IT_UNIX    },
  {           "--unix", MAKE_IT_UNIX    },
  {               "-v", VIEW_MODE       },
  {           "--view", VIEW_MODE       },
  {               "-w", NO_WRAP         },
  {         "--nowrap", NO_WRAP         },
  {               "-x", NO_HELP         },
  {         "--nohelp", NO_HELP         },
  {               "-y", AFTER_ENDS      },
  {      "--afterends", AFTER_ENDS      },
  {                "/", MODERN_BINDINGS },
  { "--modernbindings", MODERN_BINDINGS },
  {                "@", COLON_PARSING   },
  {   "--colonparsing", COLON_PARSING   },
  {                "%", STATEFLAGS      },
  {     "--stateflags", STATEFLAGS      },
  {                "_", MINIBAR         },
  {        "--minibar", MINIBAR         },
  {                "0", ZERO            },
  {           "--zero", ZERO            },
  {                "!", USE_MAGIC       },
  {          "--magic", USE_MAGIC       }
}};
/* Function to retrive flags from a string literal. */
constexpr Uint retriveFlagFromStr(string_view str) {
  for (const auto &[key, value] : flagOptionsMap) {
    if (key == str) {
      return value;
    }
  }
  return 0;
}

/* Identifiers for the different command line options. */
#define CLI_OPT_IGNORERCFILE   (1 << 0)
#define CLI_OPT_VERSION        (1 << 1)
#define CLI_OPT_HELP           (1 << 2)
#define CLI_OPT_SYNTAX         (1 << 3)
#define CLI_OPT_RCFILE         (1 << 4)
#define CLI_OPT_GUIDESTRIPE    (1 << 5)
#define CLI_OPT_WORDCHARS      (1 << 6)
#define CLI_OPT_TABSIZE        (1 << 7)
#define CLI_OPT_OPERATINGDIR   (1 << 8)
#define CLI_OPT_FILL           (1 << 9)
#define CLI_OPT_SPELLER        (1 << 10)
#define CLI_OPT_LISTSYNTAX     (1 << 11)
#define CLI_OPT_BACKUPDIR      (1 << 12)
#define CLI_OPT_BREAKLONGLINES (1 << 13)
#define CLI_OPT_GUI            (1 << 14)
#define CLI_OPT_SAFE           (1 << 15)
#define CLI_OPT_TEST           (1 << 16)
/* The command line options map. */
constexpr_map<std::string_view, Uint, 28> cliOptionMap = {{
  {               "-I", CLI_OPT_IGNORERCFILE   },
  {  "--ignorercfiles", CLI_OPT_IGNORERCFILE   },
  {               "-V", CLI_OPT_VERSION        },
  {        "--version", CLI_OPT_VERSION        },
  {               "-h", CLI_OPT_HELP           },
  {           "--help", CLI_OPT_HELP           },
  {               "-Y", CLI_OPT_SYNTAX         },
  {         "--syntax", CLI_OPT_SYNTAX         },
  {               "-X", CLI_OPT_WORDCHARS      },
  {      "--wordchars", CLI_OPT_WORDCHARS      },
  {               "-f", CLI_OPT_RCFILE         },
  {         "--rcfile", CLI_OPT_RCFILE         },
  {               "-T", CLI_OPT_TABSIZE        },
  {        "--tabsize", CLI_OPT_TABSIZE        },
  {         "--rcfile", CLI_OPT_RCFILE         },
  {               "-o", CLI_OPT_OPERATINGDIR   },
  {   "--operatingdir", CLI_OPT_OPERATINGDIR   },
  {               "-r", CLI_OPT_FILL           },
  {           "--fill", CLI_OPT_FILL           },
  {               "-s", CLI_OPT_SPELLER        },
  {        "--speller", CLI_OPT_SPELLER        },
  {               "-z", CLI_OPT_LISTSYNTAX     },
  {   "--listsyntaxes", CLI_OPT_LISTSYNTAX     },
  {               "-b", CLI_OPT_BREAKLONGLINES },
  { "--breaklonglines", CLI_OPT_BREAKLONGLINES },
  {            "--gui", CLI_OPT_GUI            },
  {           "--safe", CLI_OPT_SAFE           },
  {           "--test", CLI_OPT_TEST           }
}};
constexpr Uint retriveCliOptionFromStr(string_view str) {
  for (const auto &[key, val] : cliOptionMap) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

#define NUMBER_OF_FLAGS 51
constexpr const char *const DEFAULT_RESPONSE_ON_NONE = "Ehm...";

/* This is a masterpiece of a map.
 * I have succesfully made the way to get the description of a flag,
 * instant and without any overhead,
 * while using less memory then a unordered map.
 * USAGE:
 * - (&epithetOfFlagMap[flag].value[0]) will return the underlying ptr to the
 * description of the flag The (flag / description) map. */
constexpr_map<Uint, string_view, NUMBER_OF_FLAGS> epithetOfFlagMap = {
  {{DONTUSE, DEFAULT_RESPONSE_ON_NONE},
    {CASE_SENSITIVE, DEFAULT_RESPONSE_ON_NONE},
    {CONSTANT_SHOW, "Constant cursor position display"},
    {NO_HELP, "Help mode"},
    {NO_WRAP, DEFAULT_RESPONSE_ON_NONE},
    {AUTOINDENT, "Auto indent"},
    {VIEW_MODE, DEFAULT_RESPONSE_ON_NONE},
    {USE_MOUSE, "Mouse support"},
    {USE_REGEXP, DEFAULT_RESPONSE_ON_NONE},
    {SAVE_ON_EXIT, DEFAULT_RESPONSE_ON_NONE},
    {CUT_FROM_CURSOR, "Cut to end"},
    {BACKWARDS_SEARCH, DEFAULT_RESPONSE_ON_NONE},
    {MULTIBUFFER, DEFAULT_RESPONSE_ON_NONE},
    {REBIND_DELETE, DEFAULT_RESPONSE_ON_NONE},
    {RAW_SEQUENCES, DEFAULT_RESPONSE_ON_NONE},
    {NO_CONVERT, DEFAULT_RESPONSE_ON_NONE},
    {MAKE_BACKUP, DEFAULT_RESPONSE_ON_NONE},
    {INSECURE_BACKUP, DEFAULT_RESPONSE_ON_NONE},
    {NO_SYNTAX, "Color syntax highlighting"},
    {PRESERVE, DEFAULT_RESPONSE_ON_NONE},
    {HISTORYLOG, DEFAULT_RESPONSE_ON_NONE},
    {RESTRICTED, DEFAULT_RESPONSE_ON_NONE},
    {SMART_HOME, "Smart home key"},
    {WHITESPACE_DISPLAY, "Whitespace display"},
    {TABS_TO_SPACES, "Conversion of typed tabs to spaces"},
    {QUICK_BLANK, DEFAULT_RESPONSE_ON_NONE},
    {WORD_BOUNDS, DEFAULT_RESPONSE_ON_NONE},
    {NO_NEWLINES, DEFAULT_RESPONSE_ON_NONE},
    {BOLD_TEXT, DEFAULT_RESPONSE_ON_NONE},
    {SOFTWRAP, "Soft wrapping of overlong lines"},
    {POSITIONLOG, DEFAULT_RESPONSE_ON_NONE},
    {LOCKING, DEFAULT_RESPONSE_ON_NONE},
    {NOREAD_MODE, DEFAULT_RESPONSE_ON_NONE},
    {MAKE_IT_UNIX, DEFAULT_RESPONSE_ON_NONE},
    {TRIM_BLANKS, DEFAULT_RESPONSE_ON_NONE},
    {SHOW_CURSOR, DEFAULT_RESPONSE_ON_NONE},
    {LINE_NUMBERS, "Line numbering"},
    {AT_BLANKS, DEFAULT_RESPONSE_ON_NONE},
    {AFTER_ENDS, DEFAULT_RESPONSE_ON_NONE},
    {LET_THEM_ZAP, DEFAULT_RESPONSE_ON_NONE},
    {BREAK_LONG_LINES, "Hard wrapping of overlong lines"},
    {JUMPY_SCROLLING, DEFAULT_RESPONSE_ON_NONE},
    {EMPTY_LINE, DEFAULT_RESPONSE_ON_NONE},
    {INDICATOR, DEFAULT_RESPONSE_ON_NONE},
    {BOOKSTYLE, DEFAULT_RESPONSE_ON_NONE},
    {COLON_PARSING, DEFAULT_RESPONSE_ON_NONE},
    {STATEFLAGS, DEFAULT_RESPONSE_ON_NONE},
    {USE_MAGIC, DEFAULT_RESPONSE_ON_NONE},
    {MINIBAR, DEFAULT_RESPONSE_ON_NONE},
    {ZERO, "Hidden interface"},
    {MODERN_BINDINGS, DEFAULT_RESPONSE_ON_NONE}}
};
/* Return the textual description that corresponds to the given flag. */
constexpr const char *epithet_of_flag(const Uint flag) {
  return &epithetOfFlagMap[flag].value[0];
}

/* Identifiers for the different menus. */
// typedef enum {
//   MMAIN = (1 << 0),
//   #define MMAIN MMAIN
//   MWHEREIS = (1 << 1),
//   #define MWHEREIS MWHEREIS
//   MREPLACE = (1 << 2),
//   #define MREPLACE MREPLACE
//   MREPLACEWITH = (1 << 3),
//   #define MREPLACEWITH MREPLACEWITH
//   MGOTOLINE = (1 << 4),
//   #define MGOTOLINE MGOTOLINE
//   MWRITEFILE = (1 << 5),
//   #define MWRITEFILE MWRITEFILE
//   MINSERTFILE = (1 << 6),
//   #define MINSERTFILE MINSERTFILE
//   MEXECUTE = (1 << 7),
//   #define MEXECUTE MEXECUTE
//   MHELP = (1 << 8),
//   #define MHELP MHELP
//   MSPELL = (1 << 9),
//   #define MSPELL MSPELL
//   MBROWSER = (1 << 10),
//   #define MBROWSER MBROWSER
//   MWHEREISFILE = (1 << 11),
//   #define MWHEREISFILE MWHEREISFILE
//   MGOTODIR = (1 << 12),
//   #define MGOTODIR MGOTODIR
//   MYESNO = (1 << 13),
//   #define MYESNO MYESNO
//   MLINTER = (1 << 14),
//   #define MLINTER MLINTER
//   MFINDINHELP = (1 << 15),
//   #define MFINDINHELP MFINDINHELP
//   MMOST = (MMAIN | MWHEREIS | MREPLACE | MREPLACEWITH | MGOTOLINE | MWRITEFILE | MINSERTFILE | MEXECUTE | MWHEREISFILE | MGOTODIR | MFINDINHELP | MSPELL | MLINTER),
//   #define MMOST MMOST
//   MSOME = (MMOST | MBROWSER)
//   #define MSOME MSOME
// } menu_type;
/* The menus map. */
constexpr_map<string_view, Ushort, 16> menu_name_map = {{
  {        "main", MMAIN                               },
  {      "search", MWHEREIS                            },
  {     "replace", MREPLACE                            },
  { "replacewith", MREPLACEWITH                        },
  {       "yesno", MYESNO                              },
  {    "gotoline", MGOTOLINE                           },
  {    "writeout", MWRITEFILE                          },
  {      "insert", MINSERTFILE                         },
  {     "execute", MEXECUTE                            },
  {        "help", MHELP                               },
  {       "spell", MSPELL                              },
  {      "linter", MLINTER                             },
  {     "browser", MBROWSER                            },
  { "whereisfile", MWHEREISFILE                        },
  {     "gotodir", MGOTODIR                            },
  {         "all", (MMOST | MBROWSER | MHELP | MYESNO) }
}};
/* Function to retrive the menu option from a string literal. */
constexpr Uint nameToMenu(string_view str) {
  for (const auto &[key, val] : menu_name_map) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}
constexpr string_view menu_to_name(const Ushort value) {
  for (const auto &[key, val] : menu_name_map) {
    if (val == value) {
      return key;
    }
  }
  return "boooo";
}

constexpr_map<string_view, Uint, 14> toggleOptionMap = {
  {{"nohelp", NO_HELP},
    {"zero", ZERO},
    {"constantshow", CONSTANT_SHOW},
    {"replacewith", MREPLACEWITH},
    {"softwrap", SOFTWRAP},
    {"linenumbers", LINE_NUMBERS},
    {"whitespacedisplay", WHITESPACE_DISPLAY},
    {"nosyntax", NO_SYNTAX},
    {"smarthome", SMART_HOME},
    {"autoindent", AUTOINDENT},
    {"cutfromcursor", CUT_FROM_CURSOR},
    {"breaklonglines", BREAK_LONG_LINES},
    {"tabstospaces", TABS_TO_SPACES},
    {"mouse", USE_MOUSE}}
};
constexpr Uint retriveToggleOptionFromStr(string_view str) {
  for (const auto &[key, value] : toggleOptionMap) {
    if (key == str) {
      return value;
    }
  }
  return 0;
}

#define CS_STRUCT    (1 << 0)
#define CS_ENUM      (1 << 1)
#define CS_INT       (1 << 2)
#define CS_VOID      (1 << 3)
#define CS_LONG      (1 << 4)
#define CS_CHAR      (1 << 5)
#define CS_CLASS     (1 << 6)
#define CS_BOOL      (1 << 7)
#define CS_SIZEOF    (1 << 8)
#define CS_SHORT     (1 << 9)
#define CS_NAMESPACE (1 << 10)
#define CS_STATIC    (1 << 11)
#define CS_UNSIGNED  (1 << 12)
#define CS_CONST     (1 << 13)
#define CS_NULL      (1 << 14)
#define CS_TRUE      (1 << 15)
#define CS_FALSE     (1 << 16)
#define CS_SIZE_T    (1 << 17)
#define CS_SSIZE_T   (1 << 18)
#define CS_IF        (1 << 19)
#define CS_CASE      (1 << 20)
#define CS_ELSE      (1 << 21)
#define CS_SWITCH    (1 << 22)
#define CS_TYPEDEF   (1 << 23)
#define CS_FOR       (1 << 24)
#define CS_WHILE     (1 << 25)
#define CS_RETURN    (1 << 26)
#define CS_INCLUDE   (1 << 27)
#define CS_DEFINE    (1 << 28)
#define CS_BREAK     (1 << 29)
#define CS_DO        (1 << 30)
#define CS_USING     (1 << 31)
constexpr_map<std::string_view, Uint, 31> c_syntax_map = {
  {{"struct", CS_STRUCT},
    {"enum", CS_ENUM},
    {"int", CS_INT},
    {"void", CS_VOID},
    {"long", CS_LONG},
    {"char", CS_CHAR},
    {"include", CS_INCLUDE},
    {"define", CS_DEFINE},
    {"class", CS_CLASS},
    {"bool", CS_BOOL},
    {"size_t", CS_SIZE_T},
    {"ssize_t", CS_SSIZE_T},
    {"short", CS_SHORT},
    {"namespace", CS_NAMESPACE},
    {"static", CS_STATIC},
    {"unsigned", CS_UNSIGNED},
    {"const", CS_CONST},
    {"NULL", CS_NULL},
    {"TRUE", CS_TRUE},
    {"FALSE", CS_FALSE},
    {"if", CS_IF},
    {"case", CS_CASE},
    {"else", CS_ELSE},
    {"switch", CS_SWITCH},
    {"typedef", CS_TYPEDEF},
    {"for", CS_FOR},
    {"while", CS_WHILE},
    {"return", CS_RETURN},
    {"sizeof", CS_SIZEOF},
    {"break", CS_BREAK},
    {"do", CS_DO}}
};
constexpr Uint retrieve_c_syntax_type(string_view str) {
  for (const auto &[key, val] : c_syntax_map) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

constexpr Uint hash_string(const char *str, int h = 0) {
  return !str[h] ? 5381 : (hash_string(str, h + 1) * 33) ^ str[h];
}

constexpr Uint operator""_uint_hash(const char *str, unsigned long) {
  return hash_string(str);
}

#define PP_define  1
#define PP_if      2
#define PP_endif   3
#define PP_ifndef  4
#define PP_pragma  5
#define PP_ifdef   6
#define PP_else    7
#define PP_include 8
#define PP_undef   9
constexpr_map<std::string_view, Uint, 9> c_preprossesor_map = {
  {{"define", 1},
    {"if", 2},
    {"endif", 3},
    {"ifndef", 4},
    {"pragma", 5},
    {"ifdef", 6},
    {"else", 7},
    {"include", 8},
    {"undef", 9}}
};
constexpr Uint retrieve_preprossesor_type(string_view str) {
  for (const auto &[key, val] : c_preprossesor_map) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

#define STRSTR(return_str, haystack, needle)    \
  do {                                          \
    return_str = NULL;                          \
    if (!*needle) {                             \
      return_str = (haystack);                  \
      break;                                    \
    }                                           \
    for (const char *h = (haystack); *h; ++h) { \
      const char *n     = (needle);             \
      const char *start = h;                    \
      while (*start && *n && *start == *n) {    \
        ++start;                                \
        ++n;                                    \
      }                                         \
      if (!*n) {                                \
        return_str = h;                         \
        break;                                  \
      }                                         \
    }                                           \
  }                                             \
  while (FALSE)

#define ADV_PTR_BY_CH(ptr, ch)     for (; *ptr && (*ptr != ch); ptr++);

#define ADV_PTR_TO_CH(ptr, ch)     for (; *ptr && (*ptr != ch); ptr++)
#define adv_ptr_to_ch(ptr, ch)     for (; *ptr && (*ptr != ch); ptr++)

#define ADV_PTR(ptr, ...)          for (; *ptr && __VA_ARGS__; ptr++)
#define DCR_PTR(ptr, until, ...)   for (; ptr > until && __VA_ARGS__; ptr--)

#define adv_ptr(ptr, ...)          ADV_PTR(ptr, __VA_ARGS__)
#define dcr_ptr(ptr, until, ...)   DCR_PTR(ptr, until, __VA_ARGS__)
#define ADV_TO_NEXT_WORD(ptr)      ADV_PTR(ptr, (*ptr == ' ' || *ptr == '\t'))
#define adv_ptr_to_next_word(ptr)  adv_ptr(ptr, (*ptr == ' ' || *ptr == '\t'))
#define adv_ptr_past_word(ptr)     adv_ptr(ptr, (*ptr != ' ' && *ptr != '\t'))
#define ADV_PAST_WORD(ptr)         ADV_PTR(ptr, (*ptr != ' ' && *ptr != '\t'))

#define adv_to_next_ch(ptr)        for (; *ptr && (*ptr == ' ' || *ptr == '\t'); ptr++)
#define ADV_TO_NEXT_CH(ptr)        for (; *ptr && (*ptr == ' ' || *ptr == '\t'); ptr++)
#define dcr_to_prev_ch(ptr, until) dcr_ptr(ptr, until, (*ptr == ' ' || *ptr == '\t'))
#define DCR_TO_PREV_CH(ptr, until) dcr_ptr(ptr, until, (*ptr == ' ' || *ptr == '\t'))
#define dcr_to_prev_ch_on_fail(ptr, until, apon_failure) \
  dcr_to_prev_ch(ptr, until);                            \
  if (ptr == until) {                                    \
    apon_failure                                         \
  }
#define dcr_past_prev_word(ptr, until) dcr_ptr(ptr, until, (*ptr != ' ' && *ptr != '\t'))
#define DCR_PAST_PREV_WORD(ptr, until) dcr_ptr(ptr, until, (*ptr != ' ' && *ptr != '\t'))

#define ptr_to_next_word(p)            for (; *p && !is_word_char(p, FALSE); p++)
