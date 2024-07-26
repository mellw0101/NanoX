/// @file definitions.h
#pragma once

#include "../include/config.h"

#include <Mlib/Debug.h>
#include <Mlib/FileSys.h>
#include <Mlib/constexpr.hpp>
#include <Mlib/def.h>

#ifndef _XOPEN_SOURCE_EXTENDED
#    define _XOPEN_SOURCE_EXTENDED 1
#endif

#if defined(__HAIKU__) && !defined(_DEFAULT_SOURCE)
#    define _DEFAULT_SOURCE 1
#endif

#define ROOT_UID 0

// We are using limits instead of limits.h,
// because limits.h is for c and limits is for c++
// we alse include linux/limits.h as this is a linux project
#include <limits>
// #include <linux/limits.h>

#include <csignal>
#include <cstdlib>
#include <dirent.h>
#include <regex.h>
#include <string>
#include <sys/param.h>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

#include <ncursesw/ncurses.h>

//
//  Native language support.
//
#ifdef ENABLE_NLS
#    ifdef HAVE_LIBINTL_H
#        include <libintl.h>
#    endif
#    define _(string)                    gettext(string)
#    define P_(singular, plural, number) ngettext(singular, plural, number)
#else
#    define _(string)                    (char *)(string)
#    define P_(singular, plural, number) (number == 1 ? singular : plural)
#endif

/* For marking a string on which gettext() will be called later. */
#define gettext_noop(string) (string)
#define N_(string)           gettext_noop(string)

/* If we aren't using an ncurses with mouse support, then
 * exclude the mouse routines, as they are useless then. */
#ifndef NCURSES_MOUSE_VERSION
#    undef ENABLE_MOUSE
#endif

#if defined(ENABLE_WRAPPING) || defined(ENABLE_JUSTIFY)
#    define ENABLED_WRAPORJUSTIFY 1
#endif

/* Suppress warnings for __attribute__((warn_unused_result)). */
#define IGNORE_CALL_RESULT(call) \
    do                           \
    {                            \
        if (call)                \
        {}                       \
    }                            \
    while (0)

//
//  Macros for flags,
//  indexing each bit in a small array.
//
// #define FLAGS(flag)    flags[((flag) / (sizeof(unsigned) * 8))]
// #define FLAGMASK(flag) ((unsigned)1 << ((flag) % (sizeof(unsigned) * 8)))
// #define SET(flag)      FLAGS(flag) |= FLAGMASK(flag)
// #define UNSET(flag)    FLAGS(flag) &= ~FLAGMASK(flag)
// #define ISSET(flag)    ((FLAGS(flag) & FLAGMASK(flag)) != 0)
// #define TOGGLE(flag)   FLAGS(flag) ^= FLAGMASK(flag)
#define FLAGS(flag)    flags[((flag) / (sizeof(unsigned long) * 8))]
#define FLAGMASK(flag) ((unsigned long)1 << ((flag) % (sizeof(unsigned long) * 8)))
#define SET(flag)      FLAGS(flag) |= FLAGMASK(flag)
#define UNSET(flag)    FLAGS(flag) &= ~FLAGMASK(flag)
#define ISSET(flag)    ((FLAGS(flag) & FLAGMASK(flag)) != 0)
#define TOGGLE(flag)   FLAGS(flag) ^= FLAGMASK(flag)

constexpr bool BACKWARD = false;
constexpr bool FORWARD  = true;

constexpr bool YESORNO      = false;
constexpr bool YESORALLORNO = true;

constexpr short YES    = 1;
constexpr short ALL    = 2;
constexpr short NO     = 0;
constexpr short CANCEL = -1;

constexpr bool BLIND   = false;
constexpr bool VISIBLE = true;

constexpr u8 JUSTFIND  = 0;
constexpr u8 REPLACING = 1;
constexpr u8 INREGION  = 2;

constexpr bool NORMAL    = true;
constexpr bool SPECIAL   = false;
constexpr bool TEMPORARY = false;

constexpr bool ANNOTATE = true;
constexpr bool NONOTES  = false;

constexpr bool PRUNE_DUPLICATE   = true;
constexpr bool IGNORE_DUPLICATES = false;

constexpr unsigned char MAXCHARLEN = 4;
/* The default width of a tab in spaces. */
constexpr unsigned char WIDTH_OF_TAB = 4;
/* The default number of columns from end of line where wrapping occurs. */
constexpr unsigned char COLUMNS_FROM_EOL = 8;
/* The default comment character when a syntax does not specify any. */
constexpr const char *GENERAL_COMMENT_CHARACTER = "#";
/* The maximum number of search/replace history strings saved. */
constexpr unsigned char MAX_SEARCH_HISTORY = 100;
/* The largest unsigned long number that doesn't have the high bit set. */
constexpr unsigned long HIGHEST_POSITIVE = ((~(unsigned long)0) >> 1);

constexpr short THE_DEFAULT = -1;
constexpr short BAD_COLOR   = -2;

/* Flags for indicating how a multiline regex pair apply to a line. */

/* The start/end regexes don't cover this line at all. */
constexpr unsigned char NOTHING = (1 << 1);
/* The start regex matches on this line, the end regex on a later one. */
constexpr unsigned char STARTSHERE = (1 << 2);
/* The start regex matches on an earlier line, the end regex on a later one. */
constexpr unsigned char WHOLELINE = (1 << 3);
/* The start regex matches on an earlier line, the end regex on this one. */
constexpr unsigned char ENDSHERE = (1 << 4);
/* Both the start and end regexes match within this line. */
constexpr unsigned char JUSTONTHIS = (1 << 5);

/* Basic control codes. */
constexpr int ESC_CODE = 0x1B;
constexpr int DEL_CODE = 0x7F;

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

/* Special keycodes for when a string bind has been partially implanted
 * or has an unpaired opening brace, or when a function in a string bind
 * needs execution or a specified function name is invalid. */
constexpr unsigned short MORE_PLANTS       = 0x4EA;
constexpr unsigned short MISSING_BRACE     = 0x4EB;
constexpr unsigned short PLANTED_A_COMMAND = 0x4EC;
constexpr unsigned short NO_SUCH_FUNCTION  = 0x4EF;
/* A special keycode to signal the beginning and end of a bracketed paste. */
constexpr unsigned short BRACKETED_PASTE_MARKER = 0x4FB;
/* A special keycode for when a key produces an unknown escape sequence. */
constexpr unsigned short FOREIGN_SEQUENCE = 0x4FC;
/* A special keycode for plugging into the input stream after a suspension. */
constexpr unsigned short KEY_FRESH = 0x4FE;
/* A special keycode for when we get a SIGWINCH (a window resize). */
constexpr unsigned short KEY_WINCH = -2;

/* Some extra flags for the undo function. */
constexpr unsigned char WAS_BACKSPACE_AT_EOF = (1 << 1);
constexpr unsigned char WAS_WHOLE_LINE       = (1 << 2);
constexpr unsigned char INCLUDED_LAST_LINE   = (1 << 3);
constexpr unsigned char MARK_WAS_SET         = (1 << 4);
constexpr unsigned char CURSOR_WAS_AT_HEAD   = (1 << 5);
constexpr unsigned char HAD_ANCHOR_AT_START  = (1 << 6);

#include "constexpr_utils.h"

/* Enumeration types. */
typedef enum
{
    UNSPECIFIED,
    NIX_FILE,
    DOS_FILE,
    MAC_FILE
} format_type;

typedef enum
{
    VACUUM,
    HUSH,
    REMARK,
    INFO,
    NOTICE,
    AHEM,
    MILD,
    ALERT
} message_type;

typedef enum
{
    OVERWRITE,
    APPEND,
    PREPEND,
    EMERGENCY
} kind_of_writing_type;

typedef enum
{
    CENTERING,
    FLOWING,
    STATIONARY
} update_type;

/* The kinds of undo actions.
 * ADD...REPLACE must come first. */
typedef enum
{
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
    OTHER
} undo_type;

/* Structure types. */
typedef struct colortype
{
    short id;          // An ordinal number (if this color combo is for a multiline regex).
    short fg;          // This combo's foreground color.
    short bg;          // This combo's background color.
    short pairnum;     // The pair number for this foreground/background color
                       // combination.
    int attributes;    // Pair number and brightness composed into ready-to-use
                       // attributes.
    regex_t   *start;  // The compiled regular expression for 'start=', or the only one.
    regex_t   *end;    // The compiled regular expression for 'end=', if any.
    colortype *next;   // Next color combination.
} colortype;

typedef struct regexlisttype
{
    regex_t       *one_rgx; /* A regex to match things that imply a certain syntax. */
    regexlisttype *next;    /* The next regex. */
} regexlisttype;

typedef struct augmentstruct
{
    char          *filename;  //  The file where the syntax is extended.
    long           lineno;    //  The number of the line of the extendsyntax command.
    char          *data;      //  The text of the line.
    augmentstruct *next;      //  Next node.
} augmentstruct;

typedef struct syntaxtype
{
    char *name;                    // The name of this syntax.
    char *filename;                // File where the syntax is defined, or NULL if not an
                                   // included file.
    unsigned long  lineno;         // The line number where the 'syntax' command was found.
    augmentstruct *augmentations;  // List of extendsyntax commands to apply when loaded.
    regexlisttype *extensions;     // The list of extensions that this syntax applies to.
    regexlisttype *headers;        // The list of headerlines that this syntax applies to.
    regexlisttype *magics;         // The list of libmagic results that this syntax applies to.
    char          *linter;         // The command with which to lint this type of file.
    char          *formatter;      // The command with which to format/modify/arrange this
                                   // type of file.
    char *tabstring;               // What the Tab key should produce; NULL for default
                                   // behavior.
    char *comment;                 // The line comment prefix (and postfix) for this type of
                                   // file.
    colortype  *color;             // The colors and their regexes used in this syntax.
    short       multiscore;        // How many multiline regex strings this syntax has.
    syntaxtype *next;              // Next syntax.
} syntaxtype;

typedef struct lintstruct
{
    lintstruct *next;  // Next error.
    lintstruct *prev;  // Previous error.

    long  lineno;      // Line number of the error.
    long  colno;       // Column # of the error.
    char *msg;         // Error message text.
    char *filename;    // Filename.
} lintstruct;

/* More structure types. */

typedef struct linestruct
{
    /* Next node. */
    linestruct *next;
    /* Previous node. */
    linestruct *prev;
    /* The text of this line. */
    char *data;
    /* The number of this line. */
    long lineno;
    /* Array of which multi-line regexes apply to this line. */
    short *multidata;
    /* Whether the user has placed an anchor at this line. */
    bool has_anchor;
} linestruct;

typedef struct groupstruct
{
    /* The next group, if any. */
    groupstruct *next;
    /* First line of group. */
    long top_line;
    /* Last line of group. */
    long bottom_line;
    /* String data used to restore the affected lines; one per line. */
    char **indentations;

} groupstruct;

typedef struct undostruct
{
    undo_type     type;        /* The operation type that this undo item is for. */
    int           xflags;      /* Some flag data to mark certain corner cases. */
    long          head_lineno; /* The line number where the operation began or ended. */
    unsigned long head_x;      /* The x position where the operation began or ended. */
    char         *strdata;     /* String data to help restore the affected line. */
    unsigned long wassize;     /* The file size before the action. */
    unsigned long newsize;     /* The file size after the action. */
    groupstruct  *grouping;    /* Undo info specific to groups of lines. */
    linestruct   *cutbuffer;   /* A copy of the cutbuffer. */
    long          tail_lineno; /* Mostly the line number of the current line; sometimes something else. */
    unsigned long tail_x;      /* The x position corresponding to the above line number. */
    undostruct   *next;        /* A pointer to the undo item of the preceding action. */
} undostruct;

typedef struct poshiststruct
{
    /* The full path plus name of the file. */
    char *filename;
    /* The line where the cursor was when we closed the file. */
    long linenumber;
    /* The column where the cursor was. */
    long columnnumber;
    /* The next item of position history. */
    poshiststruct *next;
} poshiststruct;

typedef struct openfilestruct
{
    /* The file's name. */
    char *filename;
    /* The file's first line. */
    linestruct *filetop;
    /* The file's last line. */
    linestruct *filebot;
    /* The current top of the edit window for this file. */
    linestruct *edittop;
    /* The current line for this file. */
    linestruct *current;
    /* The file's total number of characters. */
    unsigned long totsize;
    /* The starting column of the top line of the edit
     * window.  When not in softwrap mode, it's always zero. */
    unsigned long firstcolumn;
    /* The file's x-coordinate position. */
    unsigned long current_x;
    /* The file's x position we would like. */
    unsigned long placewewant;
    /* The row in the edit window that the cursor is on. */
    long cursor_row;
    /* The file's stat information from when it was opened or last saved. */
    struct stat *statinfo;
    /* The line for prepending stuff to during automatic hard-wrapping. */
    linestruct *spillage_line;
    /* The line in the file where the mark is set; NULL if not set. */
    linestruct *mark;
    /* The mark's x position in the above line. */
    unsigned long mark_x;
    /* Whether a marked region was made by holding Shift. */
    bool softmark;
    /* The file's format -- Unix or DOS or Mac. */
    format_type fmt;
    /* The path of the lockfile, if we created one. */
    char *lock_filename;
    /* The top of the undo list. */
    undostruct *undotop;
    /* The current (i.e. next) level of undo. */
    undostruct *current_undo;
    /* The undo item at which the file was last saved. */
    undostruct *last_saved;
    /* The type of the last action the user performed. */
    undo_type last_action;
    /* Whether the file has been modified. */
    bool modified;
    /* The syntax that applies to this file, if any. */
    syntaxtype *syntax;
    /* The ALERT message (if any) that occurred when opening the file. */
    char *errormessage;
    /* The next open file, if any. */
    openfilestruct *next;
    /* The preceding open file, if any. */
    openfilestruct *prev;

} openfilestruct;

typedef struct rcoption
{
    /* The name of the rcfile option. */
    const char *name;
    /* The flag associated with it, if any. */
    long flag;
} rcoption;

typedef struct keystruct
{
    /* The string that describes the keystroke, like "^C" or "M-R". */
    const char *keystr;
    /* The integer that, together with meta, identifies the keystroke. */
    int keycode;
    /* The menus in which this keystroke is bound. */
    int menus;
    /* The function to which this keystroke is bound. */
    CFuncPtr func;
    /* If a toggle, what we're toggling. */
    int toggle;
    /* The how-manieth toggle this is, in order to be able to keep them in sequence. */
    int ordinal;
    /* The string of keycodes to which this shortcut is expanded. */
    char *expansion;
    /* Next in the list. */
    keystruct *next;
} keystruct;

typedef struct funcstruct
{
    void (*func)();          /* The actual function to call. */
    const char *tag;         /* The function's help-line label, for example "Where Is". */
    const char *phrase;      /* The function's description for in the help viewer. */
    bool        blank_after; /* Whether to distance this function from the next in the
                                help viewer. */
    int         menus;       /* In what menus this function applies. */
    funcstruct *next;        /* Next item in the list. */
} funcstruct;

typedef struct completionstruct
{
    char             *word;
    completionstruct *next;
} completionstruct;

constexpr unsigned char NANO_REG_EXTENDED = 1;
#define SYSCONFDIR "/etc"
