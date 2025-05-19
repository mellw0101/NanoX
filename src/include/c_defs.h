#pragma once


/* ---------------------------------------------------------- Include's ---------------------------------------------------------- */


#include "c/ascii_defs.h"
#include <fcio/proto.h>

/* NanoX */
#include "../../config.h"
// #define ASSERT_DEBUG
// #include "c/nassert.h"

/* stdlib */
#include <signal.h>
#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>

/* Linux */
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <sys/stat.h>

/* ftgl */
#include <ftgl/freetype-gl.h>
#include <ftgl/matrix4x4.h>

/* freetype */
#include <ft2build.h>
#include FT_FREETYPE_H

/* gl */
#include "../lib/include/GL/glew.h"
#include "../lib/include/GLFW/glfw3.h"

/* ncurses */
#include <ncursesw/ncurses.h>


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


/* ----------------------------- color.c ----------------------------- */

#define COLOR_8BIT(r, g, b, a)  MAX((r / 255.0f), 1), MAX((g / 255.0f), 1), MAX((b / 255.0f), 1), MAX(a, 1)

/* ----------------------------- General ----------------------------- */

/* Macros for flags, indexing each bit in a small array. */
#define FLAGS(flag)    flags[((flag) / (sizeof(Ulong) * 8))]
#define FLAGMASK(flag) ((Ulong)1 << ((flag) % (sizeof(Ulong) * 8)))
#define SET(flag)      FLAGS(flag) |= FLAGMASK(flag)
#define UNSET(flag)    FLAGS(flag) &= ~FLAGMASK(flag)
#define ISSET(flag)    ((FLAGS(flag) & FLAGMASK(flag)) != 0)
#define TOGGLE(flag)   FLAGS(flag) ^= FLAGMASK(flag)

#define MAXCHARLEN                4
/* The default width of a tab in spaces. */
#define WIDTH_OF_TAB              2
/* The default number of columns from end of line where wrapping occurs. */
#define COLUMNS_FROM_EOL          8
/* The default comment character when a syntax does not specify any. */
#define GENERAL_COMMENT_CHARACTER "#"
/* The maximum number of search/replace history strings saved. */
#define MAX_SEARCH_HISTORY        100
/* The largest Ulong number that doesn't have the high bit set. */
#define HIGHEST_POSITIVE          ((~(Ulong)0) >> 1)

// Special keycodes for when a string bind has been partially implanted
// or has an unpaired opening brace, or when a function in a string bind
// needs execution or a specified function name is invalid.
#define MORE_PLANTS                   0x4EA
#define MISSING_BRACE                 0x4EB
#define PLANTED_A_COMMAND             0x4EC
#define NO_SUCH_FUNCTION              0x4EF
/* A special keycode to signal the beginning and end of a bracketed paste. */
#define BRACKETED_PASTE_MARKER        0x4FB
/* A special keycode for when a key produces an unknown escape sequence. */
#define FOREIGN_SEQUENCE              0x4FC
/* A special keycode for plugging into the input stream after a suspension. */
#define KEY_FRESH                     0x4FE
/* A special keycode for when we get a SIGWINCH (a window resize). */
#define KEY_WINCH                     -2

/* Basic control codes. */
#define ESC_CODE 0x1B
#define DEL_CODE 0x7F

/* Total elements. */
#define NUMBER_OF_ELEMENTS  41

#define THE_DEFAULT -1
#define BAD_COLOR   -2

/* Native language support. */
#ifdef ENABLE_NLS
#  ifdef HAVE_LIBINTL_H
#    include <libintl.h>
#  endif
#  define _(string)                    gettext(string)
#  define P_(singular, plural, number) ngettext(singular, plural, number)
#else
#  define _(string)                    (char *)(string)
#  define P_(singular, plural, number) (number == 1 ? singular : plural)
#endif

/* For marking a string on which gettext() will be called later. */
#define gettext_noop(string) (string)
#define N_(string)           gettext_noop(string)

#define BACKWARD  FALSE
#define FORWARD   TRUE

#define Schar   signed char
#define BOOL    Uchar
#define wchar   wchar_t

#define SOCKLOG(...) unix_socket_debug(__VA_ARGS__)

#define UNIX_DOMAIN_SOCKET_PATH "/tmp/test"
#define BUF_SIZE 16384

#define BUF__LEN(b)  S__LEN(b)

#define ITER_SFL_TOP(syntaxfile, name, ...) \
  DO_WHILE(\
    for (SyntaxFileLine *name = syntaxfile->filetop; name; name = name->next) {\
      DO_WHILE(__VA_ARGS__); \
    } \
  )

#define ELEMENT_CHILDREN_ITER(element, iter, name, ...)                \
  /* Iterate thrue all children of `element`.  And perform any action. */  \
  DO_WHILE(                                                                \
    for (int iter=0; iter<cvec_len((element)->children); ++i) {               \
      Element *name = cvec_get((element)->children, i);                             \
      DO_WHILE(__VA_ARGS__);                                               \
    }                                                                      \
  )

/* ----------------------------- element.c ----------------------------- */

#define dp_raw /* Shorthand to access the `void *` inside an `Element` structure. */ data_ptr.raw
#define dp_sb /* Shorthand to access the `Scrollbar *` inside an `Element` structure. */ data_ptr.sb
#define dp_menu /* Shorthand to access the `Menu *` inside an `Element` structure. */ data_ptr.menu
#define dp_file /* Shorthand to access the `openfilestruct *` inside an `Element` structure. */ data_ptr.file
#define dp_editor /* Shorthand to access the `openfilestruct *` inside an `Element` structure. */ data_ptr.editor


/* ---------------------------------------------------------- Typedef's ---------------------------------------------------------- */


/* ----------------------------- General ----------------------------- */

typedef void (*FreeFuncPtr)(void *);
typedef void (*functionptrtype)(void);

/* ----------------------------- nevhandler.c ----------------------------- */

/* Opaque structure that represents a event loop. */
typedef struct nevhandler nevhandler;

/* ----------------------------- nfdlistener.c ----------------------------- */

/* `Opaque`  Structure to listen to file events. */
typedef struct nfdlistener  nfdlistener;

/* ----------------------------- scrollbar.c ----------------------------- */

typedef void (*ScrollbarUpdateFunc)(void *, float *total_length, Uint *start, Uint *end, Uint *visible, Uint *current, float *top_offset, float *right_offset);
typedef void (*ScrollbarMovingFunc)(void *, long);

/* ----------------------------- menu.c ----------------------------- */

typedef void (*MenuPositionFunc)(void *, float width, float height, float *const x, float *const y);
typedef void (*MenuAcceptFunc)(void *, const char *const restrict lable, int index);


/* ---------------------------------------------------------- Forward declaration's ---------------------------------------------------------- */


/* ----------------------------- General ----------------------------- */

typedef struct colortype             colortype;
typedef struct regexlisttype         regexlisttype;
typedef struct augmentstruct         augmentstruct;
typedef struct syntaxtype            syntaxtype;
typedef struct lintstruct            lintstruct;
typedef struct linestruct            linestruct;
typedef struct groupstruct           groupstruct;
typedef struct undostruct            undostruct;
typedef struct statusbar_undostruct  statusbar_undostruct;
typedef struct poshiststruct         poshiststruct;
typedef struct openfilestruct        openfilestruct;
typedef struct keystruct             keystruct;
typedef struct funcstruct            funcstruct;

/* ----------------------------- gui/font.c ----------------------------- */

typedef struct Font  Font;

/* ----------------------------- gui/element_gridmap.c ----------------------------- */

typedef struct ElementGrid  ElementGrid;

/* ----------------------------- gui/scrollbar.c ----------------------------- */

typedef struct Scrollbar  Scrollbar;

/* ----------------------------- gui/menu.c ----------------------------- */

typedef struct CMenu  CMenu;

/* ----------------------------- gui/element.c ----------------------------- */

typedef struct Element  Element;

/* ----------------------------- gui/editor/editor.c ----------------------------- */

typedef struct Editor  Editor;

/* ----------------------------- gui/editor/topbar.c ----------------------------- */

typedef struct EditorTb   EditorTb;

/* ----------------------------- gui/suggestmenu.c ----------------------------- */

typedef struct SuggestMenu  SuggestMenu;


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


/* ----------------------------- General ----------------------------- */

/* The kinds of undo actions.  ADD...REPLACE must come first. */
typedef enum {
  ADD,
  ENTER,
  BACK,
  DEL,
  JOIN,
  REPLACE,
  SPLIT_BEGIN,
  SPLIT_END,
  INDENT,
  UNINDENT,
  COMMENT,
  UNCOMMENT,
  PREFLIGHT,
  ZAP,
  CUT,
  CUT_TO_EOF,
  COPY,
  PASTE,
  INSERT,
  COUPLE_BEGIN,
  COUPLE_END,
  OTHER,
  MOVE_LINE_UP,
  MOVE_LINE_DOWN,
  ENCLOSE,
  AUTO_BRACKET,
  ZAP_REPLACE,
  INSERT_EMPTY_LINE,
  #define ADD                ADD
  #define ENTER              ENTER
  #define BACK               BACK
  #define DEL                DEL
  #define JOIN               JOIN
  #define REPLACE            REPLACE
  #define SPLIT_BEGIN        SPLIT_BEGIN
  #define SPLIT_END          SPLIT_END
  #define INDENT             INDENT
  #define UNINDENT           UNINDENT
  #define COMMENT            COMMENT
  #define UNCOMMENT          UNCOMMENT
  #define PREFLIGHT          PREFLIGHT
  #define ZAP                ZAP
  #define CUT                CUT
  #define CUT_TO_EOF         CUT_TO_EOF
  #define COPY               COPY
  #define PASTE              PASTE
  #define INSERT             INSERT
  #define COUPLE_BEGIN       COUPLE_BEGIN
  #define COUPLE_END         COUPLE_END
  #define OTHER              OTHER
  #define MOVE_LINE_UP       MOVE_LINE_UP
  #define MOVE_LINE_DOWN     MOVE_LINE_DOWN
  #define ENCLOSE            ENCLOSE
  #define AUTO_BRACKET       AUTO_BRACKET
  #define ZAP_REPLACE        ZAP_REPLACE
  #define INSERT_EMPTY_LINE  INSERT_EMPTY_LINE
} undo_type;

typedef enum {
  STATUSBAR_ADD,
  STATUSBAR_BACK,
  STATUSBAR_DEL,
  STATUSBAR_CHOP_NEXT_WORD,
  STATUSBAR_CHOP_PREV_WORD,
  STATUSBAR_OTHER,
  #define STATUSBAR_ADD             STATUSBAR_ADD
  #define STATUSBAR_BACK            STATUSBAR_BACK
  #define STATUSBAR_DEL             STATUSBAR_DEL
  #define STATUSBAR_CHOP_NEXT_WORD  STATUSBAR_CHOP_NEXT_WORD
  #define STATUSBAR_CHOP_PREV_WORD  STATUSBAR_CHOP_PREV_WORD
  #define STATUSBAR_OTHER           STATUSBAR_OTHER
} statusbar_undo_type;

/* Some extra flags for the undo function(s). */
typedef enum {
  WAS_BACKSPACE_AT_EOF = (1 << 1),
  WAS_WHOLE_LINE       = (1 << 2),
  INCLUDED_LAST_LINE   = (1 << 3),
  MARK_WAS_SET         = (1 << 4),
  CURSOR_WAS_AT_HEAD   = (1 << 5),
  HAD_ANCHOR_AT_START  = (1 << 6),
  SHOULD_NOT_KEEP_MARK = (1 << 7),
  INSERT_WAS_ABOVE     = (1 << 8)
  #define WAS_BACKSPACE_AT_EOF  WAS_BACKSPACE_AT_EOF
  #define WAS_WHOLE_LINE        WAS_WHOLE_LINE
  #define INCLUDED_LAST_LINE    INCLUDED_LAST_LINE
  #define MARK_WAS_SET          MARK_WAS_SET
  #define CURSOR_WAS_AT_HEAD    CURSOR_WAS_AT_HEAD
  #define HAD_ANCHOR_AT_START   HAD_ANCHOR_AT_START
  #define SHOULD_NOT_KEEP_MARK  SHOULD_NOT_KEEP_MARK
  #define INSERT_WAS_ABOVE      INSERT_WAS_ABOVE
} undo_modifier_type;

typedef enum {
  UNSPECIFIED,
  NIX_FILE,
  DOS_FILE,
  MAC_FILE
  #define UNSPECIFIED  UNSPECIFIED
  #define NIX_FILE     NIX_FILE
  #define DOS_FILE     DOS_FILE
  #define MAC_FILE     MAC_FILE
} format_type;

typedef enum {
  VACUUM,
  #define VACUUM VACUUM
  HUSH,
  #define HUSH HUSH
  REMARK,
  #define REMARK REMARK
  INFO,
  #define INFO INFO
  NOTICE,
  #define NOTICE NOTICE
  AHEM,
  #define AHEM AHEM
  MILD,
  #define MILD MILD
  ALERT
  #define ALERT ALERT
} message_type;

/* Identifiers for the different menus. */
typedef enum {
  MMAIN = (1 << 0),
  #define MMAIN MMAIN
  MWHEREIS = (1 << 1),
  #define MWHEREIS MWHEREIS
  MREPLACE = (1 << 2),
  #define MREPLACE MREPLACE
  MREPLACEWITH = (1 << 3),
  #define MREPLACEWITH MREPLACEWITH
  MGOTOLINE = (1 << 4),
  #define MGOTOLINE MGOTOLINE
  MWRITEFILE = (1 << 5),
  #define MWRITEFILE MWRITEFILE
  MINSERTFILE = (1 << 6),
  #define MINSERTFILE MINSERTFILE
  MEXECUTE = (1 << 7),
  #define MEXECUTE MEXECUTE
  MHELP = (1 << 8),
  #define MHELP MHELP
  MSPELL = (1 << 9),
  #define MSPELL MSPELL
  MBROWSER = (1 << 10),
  #define MBROWSER MBROWSER
  MWHEREISFILE = (1 << 11),
  #define MWHEREISFILE MWHEREISFILE
  MGOTODIR = (1 << 12),
  #define MGOTODIR MGOTODIR
  MYESNO = (1 << 13),
  #define MYESNO MYESNO
  MLINTER = (1 << 14),
  #define MLINTER MLINTER
  MFINDINHELP = (1 << 15),
  #define MFINDINHELP MFINDINHELP
  MMOST = (MMAIN | MWHEREIS | MREPLACE | MREPLACEWITH | MGOTOLINE | MWRITEFILE | MINSERTFILE | MEXECUTE | MWHEREISFILE | MGOTODIR | MFINDINHELP | MSPELL | MLINTER),
  #define MMOST MMOST
  MSOME = (MMOST | MBROWSER)
  #define MSOME MSOME
} menu_type;

/* Identifiers for the different flags. */
typedef enum {
  /* This flag is not used as this is part of a bitfield and 0 is not a unique
  * value, in terms of bitwise operations as the default all non set value is 0. */
  DONTUSE,
  #define DONTUSE DONTUSE
  CASE_SENSITIVE,
  #define CASE_SENSITIVE CASE_SENSITIVE
  CONSTANT_SHOW,
  #define CONSTANT_SHOW CONSTANT_SHOW
  NO_HELP,
  #define NO_HELP NO_HELP
  NO_WRAP,
  #define NO_WRAP NO_WRAP
  AUTOINDENT,
  #define AUTOINDENT AUTOINDENT
  VIEW_MODE,
  #define VIEW_MODE VIEW_MODE
  USE_MOUSE,
  #define USE_MOUSE USE_MOUSE
  USE_REGEXP,
  #define USE_REGEXP USE_REGEXP
  SAVE_ON_EXIT,
  #define SAVE_ON_EXIT SAVE_ON_EXIT
  CUT_FROM_CURSOR,
  #define CUT_FROM_CURSOR CUT_FROM_CURSOR
  BACKWARDS_SEARCH,
  #define BACKWARDS_SEARCH BACKWARDS_SEARCH
  MULTIBUFFER,
  #define MULTIBUFFER MULTIBUFFER
  REBIND_DELETE,
  #define REBIND_DELETE REBIND_DELETE
  RAW_SEQUENCES,
  #define RAW_SEQUENCES RAW_SEQUENCES
  NO_CONVERT,
  #define NO_CONVERT NO_CONVERT
  MAKE_BACKUP,
  #define MAKE_BACKUP MAKE_BACKUP
  INSECURE_BACKUP,
  #define INSECURE_BACKUP INSECURE_BACKUP
  NO_SYNTAX,
  #define NO_SYNTAX NO_SYNTAX
  PRESERVE,
  #define PRESERVE PRESERVE
  HISTORYLOG,
  #define HISTORYLOG HISTORYLOG
  RESTRICTED,
  #define RESTRICTED RESTRICTED
  SMART_HOME,
  #define SMART_HOME SMART_HOME
  WHITESPACE_DISPLAY,
  #define WHITESPACE_DISPLAY WHITESPACE_DISPLAY
  TABS_TO_SPACES,
  #define TABS_TO_SPACES TABS_TO_SPACES
  QUICK_BLANK,
  #define QUICK_BLANK QUICK_BLANK
  WORD_BOUNDS,
  #define WORD_BOUNDS WORD_BOUNDS
  NO_NEWLINES,
  #define NO_NEWLINES NO_NEWLINES
  BOLD_TEXT,
  #define BOLD_TEXT BOLD_TEXT
  SOFTWRAP,
  #define SOFTWRAP SOFTWRAP
  POSITIONLOG,
  #define POSITIONLOG POSITIONLOG
  LOCKING,
  #define LOCKING LOCKING
  NOREAD_MODE,
  #define NOREAD_MODE NOREAD_MODE
  MAKE_IT_UNIX,
  #define MAKE_IT_UNIX MAKE_IT_UNIX
  TRIM_BLANKS,
  #define TRIM_BLANKS TRIM_BLANKS
  SHOW_CURSOR,
  #define SHOW_CURSOR SHOW_CURSOR
  LINE_NUMBERS,
  #define LINE_NUMBERS LINE_NUMBERS
  AT_BLANKS,
  #define AT_BLANKS AT_BLANKS
  AFTER_ENDS,
  #define AFTER_ENDS AFTER_ENDS
  LET_THEM_ZAP,
  #define LET_THEM_ZAP LET_THEM_ZAP
  BREAK_LONG_LINES,
  #define BREAK_LONG_LINES BREAK_LONG_LINES
  JUMPY_SCROLLING,
  #define JUMPY_SCROLLING JUMPY_SCROLLING
  EMPTY_LINE,
  #define EMPTY_LINE EMPTY_LINE
  INDICATOR,
  #define INDICATOR INDICATOR
  BOOKSTYLE,
  #define BOOKSTYLE BOOKSTYLE
  COLON_PARSING,
  #define COLON_PARSING COLON_PARSING
  STATEFLAGS,
  #define STATEFLAGS STATEFLAGS
  USE_MAGIC,
  #define USE_MAGIC USE_MAGIC
  MINIBAR,
  #define MINIBAR MINIBAR
  ZERO,
  #define ZERO ZERO
  MODERN_BINDINGS,
  #define MODERN_BINDINGS MODERN_BINDINGS
  EXPERIMENTAL_FAST_LIVE_SYNTAX,
  #define EXPERIMENTAL_FAST_LIVE_SYNTAX EXPERIMENTAL_FAST_LIVE_SYNTAX
  SUGGEST,
  #define SUGGEST SUGGEST
  SUGGEST_INLINE,
  #define SUGGEST_INLINE SUGGEST_INLINE
  USING_GUI,
  #define USING_GUI USING_GUI
  NO_NCURSES,
  #define NO_NCURSES NO_NCURSES
} flag_type;

/* ----------------------------- synx.c ----------------------------- */

typedef enum {
  SYNTAX_COLOR_NONE,
  SYNTAX_COLOR_RED,
  SYNTAX_COLOR_BLUE,
  SYNTAX_COLOR_GREEN,
  #define SYNTAX_COLOR_NONE   SYNTAX_COLOR_NONE
  #define SYNTAX_COLOR_RED    SYNTAX_COLOR_RED
  #define SYNTAX_COLOR_BLUE   SYNTAX_COLOR_BLUE
  #define SYNTAX_COLOR_GREEN  SYNTAX_COLOR_GREEN
} SyntaxColor;

typedef enum {
  /* General types. */
  SYNTAX_OBJECT_TYPE_NONE,
  SYNTAX_OBJECT_TYPE_KEYWORD,
  
  /* C specific types. */
  SYNTAX_OBJECT_TYPE_C_MACRO,
  SYNTAX_OBJECT_TYPE_C_STRUCT,

  /* Defines for all types. */
  #define SYNTAX_OBJECT_TYPE_NONE       SYNTAX_OBJECT_TYPE_NONE
  #define SYNTAX_OBJECT_TYPE_KEYWORD    SYNTAX_OBJECT_TYPE_KEYWORD
  #define SYNTAX_OBJECT_TYPE_C_MACRO    SYNTAX_OBJECT_TYPE_C_MACRO
  #define SYNTAX_OBJECT_TYPE_C_STRUCT   SYNTAX_OBJECT_TYPE_C_STRUCT
} SyntaxObjectType;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


/* ----------------------------- General ----------------------------- */

typedef struct {
  float x, y, z;    /* Position. */
  float s, t;       /* Tex. */
  float r, g, b, a; /* Color. */
} vertex_t;

struct colortype {
  short id;         /* An ordinal number (if this color combo is for a multiline regex). */
  short fg;         /* This combo's foreground color. */
  short bg;         /* This combo's background color. */
  short pairnum;    /* The pair number for this foreground/background color combination. */
  int attributes;   /* Pair number and brightness composed into ready-to-use attributes. */
  regex_t *start;   /* The compiled regular expression for 'start=', or the only one. */
  regex_t *end;     /* The compiled regular expression for 'end=', if any. */
  colortype *next;  /* Next color combination. */
};

struct regexlisttype {
  regex_t *one_rgx;     /* A regex to match things that imply a certain syntax. */
  regexlisttype *next;  /* The next regex. */
};

struct augmentstruct {
  char *filename;       /* The file where the syntax is extended. */
  long  lineno;         /* The number of the line of the extendsyntax command. */
  char *data;           /* The text of the line. */
  augmentstruct *next;  /* Next node. */
};

struct syntaxtype {
  char *name;                    /* The name of this syntax. */
  char *filename;                /* File where the syntax is defined, or NULL if not an included file. */
  Ulong lineno;                  /* The line number where the 'syntax' command was found. */
  augmentstruct *augmentations;  /* List of extendsyntax commands to apply when loaded. */
  regexlisttype *extensions;     /* The list of extensions that this syntax applies to. */
  regexlisttype *headers;        /* The list of headerlines that this syntax applies to. */
  regexlisttype *magics;         /* The list of libmagic results that this syntax applies to. */
  char *linter;                  /* The command with which to lint this type of file. */
  char *formatter;               /* The command with which to format/modify/arrange this type of file. */
  char *tabstring;               /* What the Tab key should produce; NULL for default behavior. */
  char *comment;                 /* The line comment prefix (and postfix) for this type of file. */
  colortype *color;              /* The colors and their regexes used in this syntax. */
  short multiscore;              /* How many multiline regex strings this syntax has. */
  syntaxtype *next;              /* Next syntax. */
};

struct lintstruct {
  lintstruct *next;  /* Next error. */
  lintstruct *prev;  /* Previous error. */
  long  lineno;      /* Line number of the error. */
  long  colno;       /* Column # of the error. */
  char *msg;         /* Error message text. */
  char *filename;    /* Filename. */
};

struct linestruct {
  bool is_block_comment_start   : 1;
  bool is_block_comment_end     : 1;
  bool is_in_block_comment      : 1;
  bool is_single_block_comment  : 1;
  bool is_hidden                : 1;
  bool is_bracket_start         : 1;
  bool is_in_bracket            : 1;
  bool is_bracket_end           : 1;
  bool is_function_open_bracket : 1;
  bool is_dont_preprocess_line  : 1;
  bool is_pp_line               : 1;
  
  linestruct *next; /* Next node. */
  linestruct *prev; /* Previous node. */
  char *data;       /* The text of this line. */
  long lineno;      /* The number of this line. */
  short *multidata; /* Array of which multi-line regexes apply to this line. */
  bool has_anchor;  /* Whether the user has placed an anchor at this line. */
  /* The state of the line. */
  // bit_flag_t<LINE_BIT_FLAG_SIZE> flags;
  /* Some short-hands to simplyfiy linestruct loop`s. */
  #define FOR_EACH_LINE_NEXT(name, start) for (linestruct *name = start; name; name = name->next)
  #define FOR_EACH_LINE_PREV(name, start) for (linestruct *name = start; name; name = name->prev)
  #define FOREACH_FROMTO_NEXT(name, start, end) for (linestruct *name = start; name != end->next && name; name = name->next)
  #define FOREACH_FROMTO_PREV(name, start, end) for (linestruct *name = start; name != end->prev && name; name = name->prev)
  #define CONST_FOREACH_FROMTO_NEXT(name, start, end) for (const linestruct *name = start; name != end->next && name; name = name->next)
  #define CONST_FOREACH_FROMTO_PREV(name, start, end) for (const linestruct *name = start; name != end->prev && name; name = name->prev)
  /* Usefull line helpers. */
  #define line_indent(line) wideness(line->data, indent_length(line->data))
};

struct groupstruct {
  groupstruct *next;   /* The next group, if any. */
  long top_line;       /* First line of group. */
  long bottom_line;    /* Last line of group. */
  char **indentations; /* String data used to restore the affected lines; one per line. */
};

struct undostruct {
  undo_type type;        /* The `operation type` that this undo item is for. */
  int xflags;            /* Some `flag data` to mark certain corner cases. */
  long head_lineno;      /* The line number where the operation began or ended. */
  Ulong head_x;          /* The `x position` where the operation `began` or `ended`. */
  char *strdata;         /* String data to help restore the affected line. */
  Ulong wassize;         /* The file size before the action. */
  Ulong newsize;         /* The file size after the action. */
  groupstruct *grouping; /* Undo info specific to groups of lines. */
  linestruct *cutbuffer; /* A copy of the cutbuffer. */
  long tail_lineno;      /* Mostly the line number of the current line; sometimes something else. */
  Ulong tail_x;          /* The x position corresponding to the above line number. */
  undostruct *next;      /* A pointer to the undo item of the preceding action. */
};

struct statusbar_undostruct {
  statusbar_undo_type type;
  int xflags;
  Ulong head_x;
  Ulong tail_x; /* Used to indicate the end pos of this action or sometimes used to hold the length of something. */
  char *answerdata;
  statusbar_undostruct *next;
};

struct poshiststruct {
  char *filename;      /* The full path plus name of the file. */
  long linenumber;     /* The line where the cursor was when we closed the file. */
  long columnnumber;   /* The column where the cursor was. */
  poshiststruct *next; /* The next item of position history. */
};

struct openfilestruct {
  /* Boolian flags. */
  bool is_c_file        : 1;
  bool is_cxx_file      : 1;
  bool is_nasm_file     : 1;
  bool is_atnt_asm_file : 1;
  bool is_bash_file     : 1;
  bool is_glsl_file     : 1;
  bool is_systemd_file  : 1;
  bool is_nanox_file    : 1;

  char *filename;             /* The file's name. */
  linestruct *filetop;        /* The file's first line. */
  linestruct *filebot;        /* The file's last line. */
  linestruct *edittop;        /* The current top of the edit window for this file. */
  linestruct *current;        /* The current line for this file. */
  Ulong totsize;              /* The file's total number of characters. */
  Ulong firstcolumn;          /* The starting column of the top line of the edit window.  When not in softwrap mode, it's always zero. */
  Ulong current_x;            /* The file's x-coordinate position. */
  Ulong placewewant;          /* The file's x position we would like. */
  long cursor_row;            /* The row in the edit window that the cursor is on. */
  struct stat *statinfo;      /* The file's stat information from when it was opened or last saved. */
  linestruct *spillage_line;  /* The line for prepending stuff to during automatic hard-wrapping. */
  linestruct *mark;           /* The line in the file where the mark is set; NULL if not set. */
  Ulong mark_x;               /* The mark's x position in the above line. */
  bool softmark;              /* Whether a marked region was made by holding Shift. */
  format_type fmt;            /* The file's format -- Unix or DOS or Mac. */
  char *lock_filename;        /* The path of the lockfile, if we created one. */
  undostruct *undotop;        /* The top of the undo list. */
  undostruct *current_undo;   /* The current (i.e. next) level of undo. */
  undostruct *last_saved;     /* The undo item at which the file was last saved. */
  undo_type last_action;      /* The type of the last action the user performed. */
  bool modified;              /* Whether the file has been modified. */
  syntaxtype *syntax;         /* The syntax that applies to this file, if any. */
  char *errormessage;         /* The ALERT message (if any) that occurred when opening the file. */

  /* What type of file this is, in terms of syntax and family of language. */
  // bit_flag_t<FILE_TYPE_SIZE> type;

  openfilestruct *next;       /* The next open file, if any. */
  openfilestruct *prev;       /* The preceding open file, if any. */
};

struct keystruct {
  const char *keystr; /* The string that describes the keystroke, like "^C" or "M-R". */
  int keycode;        /* The integer that, together with meta, identifies the keystroke. */
  int menus;          /* The menus in which this keystroke is bound. */
  void (*func)(void); /* The function to which this keystroke is bound. */
  int toggle;         /* If a toggle, what we're toggling. */
  int ordinal;        /* The how-manieth toggle this is, in order to be able to keep them in sequence. */
  char *expansion;    /* The string of keycodes to which this shortcut is expanded. */
  keystruct *next;    /* Next in the list. */
};

struct funcstruct {
  void (*func)(void); /* The actual function to call. */
  const char *tag;    /* The function's help-line label, for example "Where Is". */
  const char *phrase; /* The function's description for in the help viewer. */
  bool blank_after;   /* Whether to distance this function from the next in the help viewer. */
  int menus;          /* In what menus this function applies. */
  funcstruct *next;   /* Next item in the list. */
};

/* ----------------------------- nfdlistener.c ----------------------------- */

/* Structure that represents the event that the callback gets. */
typedef struct {
  Uint  mask;
  Uint  cookie;
  const char *file; /* The file this event is from. */
} nfdlistener_event;
typedef void (*nfdlistener_cb)(nfdlistener_event *);

/* ----------------------------- synx.c ----------------------------- */

typedef struct SyntaxFilePos  SyntaxFilePos;
/* A structure that reprecents a position inside a `SyntaxFile` structure. */
struct SyntaxFilePos {
  int row;
  int column;
};

typedef struct SyntaxFileLine  SyntaxFileLine;
/* A structure that reprecents a line inside a 'SyntaxFile' structure. */
struct SyntaxFileLine {
  /* The next line in the double linked list of lines. */
  SyntaxFileLine *next;
  
  /* The previous line in the double linked list of lines. */
  SyntaxFileLine *prev;
  
  char *data;
  Ulong len;
  long  lineno;
};

typedef struct SyntaxObject  SyntaxObject;
/* The values in the hashmap that `syntax_file_t` uses. */
struct SyntaxObject {
  char *file;

  /* Only used when there is more then one object with the same name. */
  SyntaxObject *next;
  SyntaxObject *prev;

  /* The color this should be draw as. */
  SyntaxColor color;

  /* Type of syntax this object this is. */
  SyntaxObjectType type;

  /* The `position` in the `SyntaxFile` structure that this object is at. */
  SyntaxFilePos *pos;

  /* A ptr to unique data to this type of object. */
  void *data;

  /* Function ptr to be called to free the `data`, or `NULL`. */
  FreeFuncPtr freedata;
};

typedef struct SyntaxFileError  SyntaxFileError;
/* This reprecents a error found during parsing of a `SyntaxFile`. */
struct SyntaxFileError {
  /* This struct is a double linked list, this is most usefull when
  * we always keep the errors sorted by position in the file. */
  SyntaxFileError *next;
  SyntaxFileError *prev;

  /* The file where this error is from. */
  char *file;

  /* A string describing the error, or `NULL`. */
  char *msg;

  /* The position in the `SyntaxFile` where this error happened. */
  SyntaxFilePos *pos;
};

typedef struct SyntaxFile  SyntaxFile;
/* The structure that holds the data when parsing a file. */
struct SyntaxFile {
  /* The absolut path to the file. */
  char *path;
  
  /* First line of the file. */
  SyntaxFileLine *filetop;
  
  /* Last line of the file. */
  SyntaxFileLine *filebot;
  
  /* First error of the file, if any. */
  SyntaxFileError *errtop;
  
  /* Last error of the file, if any. */
  SyntaxFileError *errbot;
  
  /* A ptr to stat structure for this file, useful to have. */
  struct stat *stat;

  /* Hash map that holds all objects parsed from the given file. */
  HashMap *objects;
};

/* ----------------------------- csyntax.c ----------------------------- */

typedef struct {
  CVec *args;
  char *expanded;  /* What this macro expands to, or in other words the value of the macro. */
  
  /* The exact row and column the expanded macro decl starts. */
  SyntaxFilePos *expandstart;

  /* The exact row and column the expanded macro decl ends. */
  SyntaxFilePos *expandend;

  bool empty : 1;  /* Is set to `TRUE` if the macro does not have anything after its name. */
} CSyntaxMacro;

typedef struct {
  SyntaxFilePos *bodystpos;
  SyntaxFilePos *bodyendpos;
  bool forward_decl : 1;  /* This struct is a forward declaration, and does not actuly declare anything. */
} CSyntaxStruct;

/* ----------------------------- dirs.c ----------------------------- */

typedef struct {
  char *path;
  directory_t *dir;
} directory_thread_data_t;

/* ----------------------------- element_gridmap.c ----------------------------- */

typedef struct {
  int x;
  int y;
} ElementGridpos;

/* ----------------------------- color.c ----------------------------- */

typedef struct {
  float r;
  float g;
  float b;
  float a;
} Color;

/* ----------------------------- element.c ----------------------------- */

struct Element {
  /* Boolian flags. */
  bool hidden                     : 1;
  bool has_lable                  : 1;
  bool has_relative_pos           : 1;
  bool has_relative_x_pos         : 1;
  bool has_relative_y_pos         : 1;
  bool has_reverse_relative_pos   : 1;
  bool has_reverse_relative_x_pos : 1;
  bool has_reverse_relative_y_pos : 1;
  bool has_relative_width         : 1;
  bool has_relative_height        : 1;
  bool is_border                  : 1;
  bool has_borders                : 1;
  bool not_in_gridmap             : 1;
  bool has_raw_data               : 1;
  bool has_file_data              : 1;
  bool has_editor_data            : 1;
  bool has_sb_data                : 1;
  bool has_menu_data              : 1;
  bool is_above                   : 1;

  Ushort layer;

  float x;
  float y;
  float relative_x;
  float relative_y;
  float width;
  float height;
  float relative_width;
  float relative_height;

  Color *color;
  Color *text_color;

  char *lable;
  Ulong lable_len;

  Element *parent;
  CVec *children;

  int cursor;

  union {
    void           *raw;
    Scrollbar      *sb;
    CMenu          *menu;
    openfilestruct *file;
    Editor         *editor;
  } data_ptr;
};

/* ----------------------------- gui/editor/editor.c ----------------------------- */

struct Editor {
  /* Boolian flag's. */
  bool should_close : 1;  /* This is used to ensure safe closure of the editor. */
  bool hidden       : 1;

  vertex_buffer_t *buffer;
  openfilestruct *startfile;
  openfilestruct *openfile;

  Element *main;
  Element *gutter;
  Element *text;

  Scrollbar *sb;  
  EditorTb *tb;
  
  int rows;
  int cols;
  int margin;

  Editor *prev;
  Editor *next;
};

/* ----------------------------- gui/suggestmenu.c ----------------------------- */

struct SuggestMenu {
  CMenu *menu;
  /* The current string used to search, and its length. */
  char  buf[128];
  int   len;
};
