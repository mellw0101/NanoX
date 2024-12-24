/** @file definitions.h */
#pragma once

#include "../include/config.h"

#ifndef _XOPEN_SOURCE_EXTENDED
#  define _XOPEN_SOURCE_EXTENDED 1
#endif

#if defined(__HAIKU__) && !defined(_DEFAULT_SOURCE)
#  define _DEFAULT_SOURCE 1
#endif
#define ROOT_UID 0

// We are using limits instead of limits.h,
// because limits.h is for c and limits is for c++
// we alse include linux/limits.h as this is a linux project
#include <limits>
// #include <linux/limits.h>

#include <cctype>
#include <cerrno>
#include <clocale>
#include <csignal>
#include <cstdio>
// #include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <execinfo.h> // For backtrace functions
#include <fcntl.h>
#include <getopt.h>
#include <glob.h>
#include <libgen.h>
#include <pwd.h>
#include <regex.h>
#include <stdio.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <sys/wait.h>
#include <termios.h>
#include <stddef.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include <ncursesw/ncurses.h>

#include <Mlib/Attributes.h>
#include <Mlib/Debug.h>
#include <Mlib/FileSys.h>
#include <Mlib/Flag.h>
#include <Mlib/Profile.h>
#include <Mlib/String.h>
#include <Mlib/Vector.h>
#include <Mlib/constexpr.hpp>
#include <Mlib/Notify.h>
#include <Mlib/def.h>

using std::hash;
using std::string;
using std::string_view;
using std::to_string;
using std::unordered_map;
using std::vector;

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

/* If we aren't using an ncurses with mouse support, then
 * exclude the mouse routines, as they are useless then. */
#ifndef NCURSES_MOUSE_VERSION
#  undef ENABLE_MOUSE
#endif

#if defined(ENABLE_WRAPPING) || defined(ENABLE_JUSTIFY)
#  define ENABLED_WRAPORJUSTIFY 1
#endif

/* Suppress warnings for __attribute__((warn_unused_result)). */
#define IGNORE_CALL_RESULT(call) \
  do {                           \
    if (call) {}                 \
  } while (0)

/* Macros for flags, indexing each bit in a small array. */
#define FLAGS(flag)    flags[((flag) / (sizeof(Ulong) * 8))]
#define FLAGMASK(flag) ((Ulong)1 << ((flag) % (sizeof(Ulong) * 8)))
#define SET(flag)      FLAGS(flag) |= FLAGMASK(flag)
#define UNSET(flag)    FLAGS(flag) &= ~FLAGMASK(flag)
#define ISSET(flag)    ((FLAGS(flag) & FLAGMASK(flag)) != 0)
#define TOGGLE(flag)   FLAGS(flag) ^= FLAGMASK(flag)

#define CALCULATE_MS_TIME(start_time) (1000 * (double)(clock() - start_time) / CLOCKS_PER_SEC)

/* Some line flags. */
#define BLOCK_COMMENT_START           1
#define BLOCK_COMMENT_END             2
#define IN_BLOCK_COMMENT              3
#define SINGLE_LINE_BLOCK_COMMENT     4
#define IS_HIDDEN                     5
#define BRACKET_START                 6
#define IN_BRACKET                    7
#define BRACKET_END                   8
#define FUNCTION_OPEN_BRACKET         9
#define DONT_PREPROSSES_LINE          10
#define PP_LINE                       11
/* Helpers to unset all line flags. */
#define LINE_BIT_FLAG_SIZE            16

/* Some other define`s. */
#define BACKWARD          FALSE
#define FORWARD           TRUE
#define YESORNO           FALSE
#define YESORALLORNO      TRUE
#define BLIND             FALSE
#define VISIBLE           TRUE
#define YES               1
#define ALL               2
#define NO                0
#define CANCEL           -1
#define JUSTFIND          0
#define REPLACING         1
#define INREGION          2
#define NORMAL            TRUE
#define SPECIAL           FALSE
#define TEMPORARY         FALSE
#define ANNOTATE          TRUE
#define NONOTES           FALSE
#define PRUNE_DUPLICATE   TRUE
#define IGNORE_DUPLICATES FALSE

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

#define THE_DEFAULT -1
#define BAD_COLOR   -2

/* Flags for indicating how a multiline regex pair apply to a line. */

/* The start/end regexes don't cover this line at all. */
#define NOTHING    (1 << 1)
/* The start regex matches on this line, the end regex on a later one. */
#define STARTSHERE (1 << 2)
/* The start regex matches on an earlier line, the end regex on a later one. */
#define WHOLELINE  (1 << 3)
/* The start regex matches on an earlier line, the end regex on this one. */
#define ENDSHERE   (1 << 4)
/* Both the start and end regexes match within this line. */
#define JUSTONTHIS (1 << 5)

/* Basic control codes. */
#define ESC_CODE 0x1B
#define DEL_CODE 0x7F

/* Codes for "modified" Arrow keys, beyond KEY_MAX of ncurses. */
#define CONTROL_LEFT         0x401
#define CONTROL_RIGHT        0x402
#define CONTROL_UP           0x403
#define CONTROL_DOWN         0x404
#define CONTROL_HOME         0x405
#define CONTROL_END          0x406
#define CONTROL_DELETE       0x40D
#define SHIFT_CONTROL_LEFT   0x411
#define SHIFT_CONTROL_RIGHT  0x412
#define SHIFT_CONTROL_UP     0x413
#define SHIFT_CONTROL_DOWN   0x414
#define SHIFT_CONTROL_HOME   0x415
#define SHIFT_CONTROL_END    0x416
#define CONTROL_SHIFT_DELETE 0x41D
#define ALT_LEFT             0x421
#define ALT_RIGHT            0x422
#define ALT_UP               0x423
#define ALT_DOWN             0x424
#define ALT_HOME             0x425
#define ALT_END              0x426
#define ALT_PAGEUP           0x427
#define ALT_PAGEDOWN         0x428
#define ALT_INSERT           0x42C
#define ALT_DELETE           0x42D
#define SHIFT_ALT_LEFT       0x431
#define SHIFT_ALT_RIGHT      0x432
#define SHIFT_ALT_UP         0x433
#define SHIFT_ALT_DOWN       0x434
// #define SHIFT_LEFT 0x451
// #define SHIFT_RIGHT 0x452
#define SHIFT_UP             0x453
#define SHIFT_DOWN           0x454
#define SHIFT_HOME           0x455
#define SHIFT_END            0x456
#define SHIFT_PAGEUP         0x457
#define SHIFT_PAGEDOWN       0x458
#define SHIFT_DELETE         0x45D
#define SHIFT_TAB            0x45F
#define CONTROL_BSP          0x460

#define FOCUS_IN             0x491
#define FOCUS_OUT            0x499

#define MOD_KEY_SHIFT 0
#define MOD_KEY_ALT   1
#define MOD_KEY_CTRL  2

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

/* Some extra flags for the undo function. */
#define WAS_BACKSPACE_AT_EOF          (1 << 1)
#define WAS_WHOLE_LINE                (1 << 2)
#define INCLUDED_LAST_LINE            (1 << 3)
#define MARK_WAS_SET                  (1 << 4)
#define CURSOR_WAS_AT_HEAD            (1 << 5)
#define HAD_ANCHOR_AT_START           (1 << 6)

#define TASK_STRUCT(name, ...) \
  typedef struct {             \
    __VA_ARGS__                \
  } name;

#define LIST_STRUCT(name, ...) \
  typedef struct name {        \
    __VA_ARGS__                \
  } name;

#define ASM_FUNCTION(ret) extern "C" ret __attribute__((__nodebug__, __nothrow__))

#if defined (__aarch64__)
#define atomic_xchg(ptr, value) __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST)
#else
#define atomic_xchg(ptr, value) asm_atomic_xchg(ptr, value)
#endif

/* Used to encode both parts when enclosing a region. */
#define ENCLOSE_DELIM ":;:"

#define IS_POINTER_TYPE(x) \
  _Generic((x), \
    void *: 1, \
    const void *: 1, \
    char *: 1, \
    const char *: 1, \
    int *: 1, \
    const int *: 1, \
    long *: 1, \
    const long *: 1, \
    float *: 1, \
    const float *: 1, \
    double *: 1, \
    const double *: 1, \
    default: 0 \
  )

#define STRLTRLEN(str) (sizeof(str) - 1)
/* Make copy of a string literal. */
#define STRLTR_COPY_OF(str)                        \
  [](void) _NO_EXCEPT -> char * {                  \
    char *__strptr = (char *)nmalloc(sizeof(str)); \
    memcpy(__strptr, str, sizeof(str));            \
    return __strptr;                               \
  }()

#define STRLTR_WRITE(fd, str) write(fd, str, (sizeof(str) - 1))

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#include "constexpr_def.h"

#define NOTREBOUND (first_sc_for(MMAIN, do_help) && first_sc_for(MMAIN, do_help)->keycode == 0x07)

/* Bool def. */
#ifdef TRUE
  #undef TRUE
#endif
#ifdef FALSE
  #undef FALSE
#endif
#ifdef __cplusplus
  #define TRUE true
  #define FALSE false
#else
  #define TRUE 1
  #define FALSE 0
#endif

#define _NO_EXCEPT noexcept(TRUE)

/* clang-format off */

/* Enumeration types. */
typedef enum {
  #define FILE_TYPE_SIZE 8
  C_CPP,
  #define C_CPP C_CPP
  ASM,
  #define ASM ASM
  BASH,
  #define BASH BASH
  GLSL,
  #define GLSL GLSL
  SYSTEMD_SERVICE
  #define SYSTEMD_SERVICE SYSTEMD_SERVICE
} file_type;

typedef enum {
  LINENUMBER_STYLING
  #define LINENUMBER_STYLING LINENUMBER_STYLING
  #define LINENUMBER_STYLING_DEFAULT TRUE
} config_opt_type;

typedef enum {
  LOCAL_VAR_SYNTAX = 1,
  CLASS_SYNTAX,
  CLASS_METHOD_SYNTAX,
  DEFAULT_TYPE_SYNTAX,
  CONTROL_SYNTAX,
  IS_WORD_STRUCT,
  STRUCT_SYNTAX,
  IS_WORD_CLASS,
  DEFINE_SYNTAX,
  DEFINE_PARAM_SYNTAX,
  LSP_FUNC,
  LSP_FUNC_PARAM,
  ASM_REG,
  ASM_INSTRUCT,
  ASM_CONTROL
} syntax_type;

typedef enum {
  UNSPECIFIED,
  #define UNSPECIFIED UNSPECIFIED
  NIX_FILE,
  #define NIX_FILE NIX_FILE
  DOS_FILE,
  #define DOS_FILE DOS_FILE
  MAC_FILE
  #define MAC_FILE MAC_FILE
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

typedef enum {
  OVERWRITE,
  #define OVERWRITE OVERWRITE
  APPEND,
  #define APPEND APPEND
  PREPEND,
  #define PREPEND PREPEND
  EMERGENCY
  #define EMERGENCY EMERGENCY
} kind_of_writing_type;

typedef enum {
  CENTERING,
  #define CENTERING CENTERING
  FLOWING,
  #define FLOWING FLOWING
  STATIONARY
  #define STATIONARY STATIONARY
} update_type;

/* The kinds of undo actions.  ADD...REPLACE must come first. */
typedef enum {
  ADD,
  #define ADD ADD
  ENTER,
  #define ENTER ENTER
  BACK,
  #define BACK BACK
  DEL,
  #define DEL DEL
  JOIN,
  #define JOIN JOIN
  REPLACE,
  #define REPLACE REPLACE
  SPLIT_BEGIN,
  #define SPLIT_BEGIN SPLIT_BEGIN
  SPLIT_END,
  #define SPLIT_END SPLIT_END
  INDENT,
  #define INDENT INDENT
  UNINDENT,
  #define UNINDENT UNINDENT
  COMMENT,
  #define COMMENT COMMENT
  UNCOMMENT,
  #define UNCOMMENT UNCOMMENT
  PREFLIGHT,
  #define PREFLIGHT PREFLIGHT
  ZAP,
  #define ZAP ZAP
  CUT,
  #define CUT CUT
  CUT_TO_EOF,
  #define CUT_TO_EOF CUT_TO_EOF
  COPY,
  #define COPY COPY
  PASTE,
  #define PASTE PASTE
  INSERT,
  #define INSERT INSERT
  COUPLE_BEGIN,
  #define COUPLE_BEGIN COUPLE_BEGIN
  COUPLE_END,
  #define COUPLE_END COUPLE_END
  OTHER,
  #define OTHER OTHER
  MOVE_LINE_UP,
  #define MOVE_LINE_UP MOVE_LINE_UP
  MOVE_LINE_DOWN,
  #define MOVE_LINE_DOWN MOVE_LINE_DOWN
  ENCLOSE,
  #define ENCLOSE ENCLOSE
  AUTO_BRACKET
  #define AUTO_BRACKET AUTO_BRACKET
} undo_type;

/* Structure types. */
typedef struct colortype {
  short id;        /* An ordinal number (if this color combo is for a multiline regex). */
  short fg;        /* This combo's foreground color. */
  short bg;        /* This combo's background color. */
  short pairnum;   /* The pair number for this foreground/background color combination. */
  int attributes;  /* Pair number and brightness composed into ready-to-use attributes. */
  regex_t *start;  /* The compiled regular expression for 'start=', or the only one. */
  regex_t *end;    /* The compiled regular expression for 'end=', if any. */
  colortype *next; /* Next color combination. */
} colortype;

typedef struct regexlisttype {
  regex_t *one_rgx;    /* A regex to match things that imply a certain syntax. */
  regexlisttype *next; /* The next regex. */
} regexlisttype;

typedef struct augmentstruct { 
  char *filename;      /* The file where the syntax is extended. */
  long  lineno;        /* The number of the line of the extendsyntax command. */
  char *data;          /* The text of the line. */
  augmentstruct *next; /* Next node. */
} augmentstruct;

typedef struct syntaxtype {
  char *name;                   /* The name of this syntax. */
  char *filename;               /* File where the syntax is defined, or nullptr if not an included file. */
  Ulong lineno;                 /* The line number where the 'syntax' command was found. */
  augmentstruct *augmentations; /* List of extendsyntax commands to apply when loaded. */
  regexlisttype *extensions;    /* The list of extensions that this syntax applies to. */
  regexlisttype *headers;       /* The list of headerlines that this syntax applies to. */
  regexlisttype *magics;        /* The list of libmagic results that this syntax applies to. */
  char *linter;                 /* The command with which to lint this type of file. */
  char *formatter;              /* The command with which to format/modify/arrange this type of file. */
  char *tabstring;              /* What the Tab key should produce; nullptr for default behavior. */
  char *comment;                /* The line comment prefix (and postfix) for this type of file. */
  colortype *color;             /* The colors and their regexes used in this syntax. */
  short multiscore;             /* How many multiline regex strings this syntax has. */
  syntaxtype *next;             /* Next syntax. */
} syntaxtype;

typedef struct lintstruct {
  lintstruct *next; /* Next error. */
  lintstruct *prev; /* Previous error. */
  long  lineno;     /* Line number of the error. */
  long  colno;      /* Column # of the error. */
  char *msg;        /* Error message text. */
  char *filename;   /* Filename. */
} lintstruct;

/* More structure types. */
typedef struct linestruct {
  linestruct *next; /* Next node. */
  linestruct *prev; /* Previous node. */
  char *data;       /* The text of this line. */
  long lineno;      /* The number of this line. */
  short *multidata; /* Array of which multi-line regexes apply to this line. */
  bool has_anchor;  /* Whether the user has placed an anchor at this line. */
  /* The state of the line. */
  bit_flag_t<LINE_BIT_FLAG_SIZE> flags;
  /* Some short-hands to simplyfiy linestruct loop`s. */
  #define FOR_EACH_LINE_NEXT(name, start) for (linestruct *name = start; name; name = name->next)
  #define FOR_EACH_LINE_PREV(name, start) for (linestruct *name = start; name; name = name->prev)
  /* Usefull line helpers. */
  #define line_indent(line) wideness(line->data, indent_length(line->data))
} linestruct;

typedef struct groupstruct {
  groupstruct *next;   /* The next group, if any. */
  long top_line;       /* First line of group. */
  long bottom_line;    /* Last line of group. */
  char **indentations; /* String data used to restore the affected lines; one per line. */
} groupstruct;

typedef struct undostruct {
  undo_type type;        /* The operation type that this undo item is for. */
  int xflags;            /* Some flag data to mark certain corner cases. */
  long head_lineno;      /* The line number where the operation began or ended. */
  Ulong head_x;          /* The x position where the operation began or ended. */
  char *strdata;         /* String data to help restore the affected line. */
  Ulong wassize;         /* The file size before the action. */
  Ulong newsize;         /* The file size after the action. */
  groupstruct *grouping; /* Undo info specific to groups of lines. */
  linestruct *cutbuffer; /* A copy of the cutbuffer. */
  long tail_lineno;      /* Mostly the line number of the current line; sometimes something else. */
  Ulong tail_x;          /* The x position corresponding to the above line number. */
  undostruct *next;      /* A pointer to the undo item of the preceding action. */
} undostruct;

typedef struct poshiststruct {
  char *filename;      /* The full path plus name of the file. */
  long linenumber;     /* The line where the cursor was when we closed the file. */
  long columnnumber;   /* The column where the cursor was. */
  poshiststruct *next; /* The next item of position history. */
} poshiststruct;

typedef struct openfilestruct {
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
  linestruct *mark;           /* The line in the file where the mark is set; nullptr if not set. */
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
  bit_flag_t<FILE_TYPE_SIZE> type;
  openfilestruct *next;       /* The next open file, if any. */
  openfilestruct *prev;       /* The preceding open file, if any. */
} openfilestruct;

typedef struct coloroption {
  const char *name; /* Name of the option. */
  int name_len;     /* Length of the name. */
  int color_index;  /* Index of the color. */
} coloroption;

typedef struct configstruct {
  struct {
    int color;            /* Line number color. */
    int attr;             /* Line number attribute. */
    int barcolor;         /* If verticalbar or fullverticalbar is set then this is the color of that bar. */
    bool verticalbar;     /* TRUE if user wants vertical bar next to linenumbers. */
    bool fullverticalbar; /* TRUE if user wants vertican bar next to linenumbers no matter the current amount off lines. */
  } linenumber;
  int minibar_color;            /* Minibar color. */
  int selectedtext_color;       /* Selected text color. */
} configstruct;

typedef struct configfilestruct {
  char *filepath;    /* Full path to the config file. */
  configstruct data; /* Holds data while reading the config file. */
} configfilestruct;

typedef struct rcoption {
  const char *name; /* The name of the rcfile option. */
  long flag;        /* The flag associated with it, if any. */
} rcoption;

typedef struct keystruct {
  const char *keystr; /* The string that describes the keystroke, like "^C" or "M-R". */
  int keycode;        /* The integer that, together with meta, identifies the keystroke. */
  int menus;          /* The menus in which this keystroke is bound. */
  void (*func)();     /* The function to which this keystroke is bound. */
  int toggle;         /* If a toggle, what we're toggling. */
  int ordinal;        /* The how-manieth toggle this is, in order to be able to keep them in sequence. */
  char *expansion;    /* The string of keycodes to which this shortcut is expanded. */
  keystruct *next;    /* Next in the list. */
} keystruct;

typedef struct funcstruct {
  void (*func)(void); /* The actual function to call. */
  const char *tag;    /* The function's help-line label, for example "Where Is". */
  const char *phrase; /* The function's description for in the help viewer. */
  bool blank_after;   /* Whether to distance this function from the next in the help viewer. */
  int menus;          /* In what menus this function applies. */
  funcstruct *next;   /* Next item in the list. */
} funcstruct;

typedef struct completionstruct {
  char *word;
  completionstruct *next;
} completionstruct;

enum syntax_flag_t { NEXT_WORD_ALSO = 1 };

typedef struct line_word_t {
  char        *str;
  Ushort       start;
  Ushort       end;
  Ushort       len;
  line_word_t *next;
  #define free_node(node) free(node->str), free(node)
} line_word_t;

typedef struct variable_t {
  char       *type  = nullptr;
  char       *name  = nullptr;
  char       *value = nullptr;
  variable_t *next  = nullptr;
  variable_t *prev  = nullptr;
} variable_t;

struct glob_var_t {
  char *type;
  char *name;
  char *value;
};

struct local_var_t {
  char *type;
  char *name;
  char *value;
  int   decl_line;
  int   scope_end;
};

typedef struct pause_sub_threads_guard_t pause_sub_threads_guard_t;

/* This is from file: 'threadpool.cpp'. */
void lock_pthread_mutex(pthread_mutex_t *mutex, bool lock);
/* RAII complient way to lock a pthread mutex.  This struct will lock
 * the mutex apon its creation, and unlock it when it goes out of scope. */
struct pthread_mutex_guard_t {
  pthread_mutex_t *mutex = nullptr;
  explicit pthread_mutex_guard_t(pthread_mutex_t *m)
      : mutex(m) {
    if (!mutex) {
      logE("A 'nullptr' was passed to 'pthread_mutex_guard_t'.");
    }
    else {
      lock_pthread_mutex(mutex, true);
    }
  }
  ~pthread_mutex_guard_t(void) {
    if (mutex) {
      lock_pthread_mutex(mutex, false);
    }
  }
  pthread_mutex_guard_t(const pthread_mutex_guard_t &)            = delete;
  pthread_mutex_guard_t &operator=(const pthread_mutex_guard_t &) = delete;
};

#define vec vec_t
template <class T>
class vec_t {
 public:
  vec(initializer_list<T> init_list) {
    pthread_mutex_init(&mutex, nullptr);
    pthread_mutex_guard_t guard(&mutex);
    size    = init_list.size();
    cap     = size * 2;
    data    = (T *)malloc(cap * sizeof(T));
    Ulong i = 0;
    for (const auto &element : init_list) {
      data[i++] = element;
    }
  }

  vec(void)
      : cap(10)
      , size(0) {
    pthread_mutex_init(&mutex, nullptr);
    pthread_mutex_guard_t guard(&mutex);
    data = (T *)malloc(sizeof(T) * cap);
  }

  vec(const T *array, Ulong len = 0) {
    pthread_mutex_init(&mutex, nullptr);
    pthread_mutex_guard_t guard(&mutex);
    PROFILE_FUNCTION;
    if constexpr (std::is_same<char, T>::value) {
      size  = len ?: strlen(array);
      cap   = size * 2;
      data  = (char *)malloc(cap);
      int i = 0;
      for (; i < size; (data[i] = array[i]), i++);
      data[i] = '\0';
    }
    else {
      size = 0;
      cap  = 10;
      data = (T *)malloc(sizeof(T) * cap);
      for (; array[size];
           (size == cap) ? cap *= 2, data = (T *)realloc(data, sizeof(T) * cap) : 0, data[size] = array[size], size++);
      data[size] = nullptr;
    }
  }

  vec<T> &operator<<=(const T &value) {
    this->push_back(value);
    return *this;
  }

  vec<char> &operator<<=(const char *str) {
    pthread_mutex_guard_t guard(&this->mutex);
    Ulong         str_size = strlen(str);
    Ulong         nsize    = this->size + str_size;
    if (nsize >= this->cap) {
      this->cap = nsize;
      this->resize();
    }
    memmove(this->data + this->size, str, str_size);
    this->size        = nsize;
    this->data[nsize] = '\0';
    return *this;
  }

  ~vec(void) {
    {
      pthread_mutex_guard_t guard(&mutex);
      free(data);
    }
    pthread_mutex_destroy(&mutex);
  }

  void push_back(const T &value) {
    pthread_mutex_guard_t guard(&mutex);
    if (cap == size) {
      resize();
    }
    data[size++] = value;
  }

  void pop_back(void) {
    pthread_mutex_guard_t guard(&mutex);
    if (size > 0) {
      --size;
    }
  }

  T &operator[](Ulong index) {
    pthread_mutex_guard_t guard(&mutex);
    if (index >= size || index < 0) {
      logE("Invalid index: '%lu'.", index);
    }
    return data[index];
  }

  Ulong get_size(void) const {
    return size;
  }

  Ulong get_cap(void) const {
    return cap;
  }

  T *begin(void) const {
    return data;
  }

  T *end(void) const {
    return data + size;
  }

  T *find(const T &value) {
    for (T *it = begin(); it != end(); ++it) {
      if constexpr (std::is_same<T, char *>::value) {
        if (strcmp(*it, value) == 0) {
          return it;
        }
      }
      else {
        if (*it == value) {
          return it;
        }
      }
    }
    return end();
  }

 private:
  void resize(void) {
    cap *= 2;
    data = (T *)realloc(data, sizeof(T) * cap);
  }

  pthread_mutex_t mutex;
  T              *data;
  Ulong   cap;
  Ulong   size;
};
#define vec vec_t

typedef struct {
  char       *full_function;
  char       *name;
  char       *return_type;
  variable_t *params;
  int         number_of_params;
  char      **attributes;
  int         number_of_attributes;
  int         start_bracket;
  int         end_braket;
} function_info_t;

struct var_t {
  string type;
  string name;
  string value;
  int    decl_line;
  int    scope_end;
  string file;
};

struct class_info_t {
  string         name;
  vector<var_t>  variables;
  vector<string> methods;
};

struct syntax_data_t {
  int color;
  int from_line = -1;
  int to_line   = -1;
  int type      = -1;
};

#define NANO_REG_EXTENDED 1
#define SYSCONFDIR        "/etc"
