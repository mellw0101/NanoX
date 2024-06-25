/// @file definitions.h
#pragma once

#include "../include/config.h"

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
#include <linux/limits.h>

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

/* Macros for flags, indexing each bit in a small array. */
#define FLAGS(flag)    flags[((flag) / (sizeof(unsigned) * 8))]
#define FLAGMASK(flag) ((unsigned)1 << ((flag) % (sizeof(unsigned) * 8)))
#define SET(flag)      FLAGS(flag) |= FLAGMASK(flag)
#define UNSET(flag)    FLAGS(flag) &= ~FLAGMASK(flag)
#define ISSET(flag)    ((FLAGS(flag) & FLAGMASK(flag)) != 0)
#define TOGGLE(flag)   FLAGS(flag) ^= FLAGMASK(flag)

constexpr auto BACKWARD = false;
constexpr auto FORWARD  = true;

constexpr auto YESORNO      = false;
constexpr auto YESORALLORNO = true;

constexpr auto YES    = 1;
constexpr auto ALL    = 2;
constexpr auto NO     = 0;
constexpr auto CANCEL = -1;

constexpr auto BLIND   = false;
constexpr auto VISIBLE = true;

constexpr auto JUSTFIND  = 0;
constexpr auto REPLACING = 1;
constexpr auto INREGION  = 2;

constexpr auto NORMAL    = true;
constexpr auto SPECIAL   = false;
constexpr auto TEMPORARY = false;

constexpr auto ANNOTATE = true;
constexpr auto NONOTES  = false;

constexpr auto PRUNE_DUPLICATE   = true;
constexpr auto IGNORE_DUPLICATES = false;

constexpr auto MAXCHARLEN = 4;

//
//  The default width of a tab in spaces.
//
constexpr auto WIDTH_OF_TAB = 4;

//
//  The default number of columns from end of line where wrapping occurs.
//
constexpr auto COLUMNS_FROM_EOL = 8;

//
//  The default comment character when a syntax does not specify any.
//
constexpr auto GENERAL_COMMENT_CHARACTER = "#";

//
//  The maximum number of search/replace history strings saved.
//
constexpr auto MAX_SEARCH_HISTORY = 100;

//
//  The largest size_t number that doesn't have the high bit set.
//
constexpr auto HIGHEST_POSITIVE = ((~(u64)0) >> 1);

constexpr auto THE_DEFAULT = -1;
constexpr auto BAD_COLOR   = -2;

//  Flags for indicating how a multiline regex pair apply to a line.

//
//  The start/end regexes don't cover this line at all.
//
constexpr auto NOTHING = (1 << 1);
//
//  The start regex matches on this line, the end regex on a later one.
//
constexpr auto STARTSHERE = (1 << 2);
//
//  The start regex matches on an earlier line, the end regex on a later one.
//
constexpr auto WHOLELINE = (1 << 3);
//
//  The start regex matches on an earlier line, the end regex on this one.
//
constexpr auto ENDSHERE = (1 << 4);
//
//  Both the start and end regexes match within this line.
//
constexpr auto JUSTONTHIS = (1 << 5);

//
//  Basic control codes.
//
constexpr auto ESC_CODE = 0x1B;
constexpr auto DEL_CODE = 0x7F;

//
//  Codes for "modified" Arrow keys, beyond KEY_MAX of ncurses.
//
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

#define FOCUS_IN             0x491
#define FOCUS_OUT            0x499

//
//  Special keycodes for when a string bind has been partially implanted
//  or has an unpaired opening brace, or when a function in a string bind
//  needs execution or a specified function name is invalid.
//
constexpr auto MORE_PLANTS       = 0x4EA;
constexpr auto MISSING_BRACE     = 0x4EB;
constexpr auto PLANTED_A_COMMAND = 0x4EC;
constexpr auto NO_SUCH_FUNCTION  = 0x4EF;
//
//  A special keycode to signal the beginning and end of a bracketed paste.
//
constexpr auto BRACKETED_PASTE_MARKER = 0x4FB;
//
//  A special keycode for when a key produces an unknown escape sequence.
//
constexpr auto FOREIGN_SEQUENCE = 0x4FC;
//
//  A special keycode for plugging into the input stream after a suspension.
//
constexpr auto KEY_FRESH = 0x4FE;
//
//  A special keycode for when we get a SIGWINCH (a window resize).
//
constexpr auto KEY_WINCH = -2;

//
//  Some extra flags for the undo function.
//
constexpr auto WAS_BACKSPACE_AT_EOF = (1 << 1);
constexpr auto WAS_WHOLE_LINE       = (1 << 2);
constexpr auto INCLUDED_LAST_LINE   = (1 << 3);
constexpr auto MARK_WAS_SET         = (1 << 4);
constexpr auto CURSOR_WAS_AT_HEAD   = (1 << 5);
constexpr auto HAD_ANCHOR_AT_START  = (1 << 6);

#include "constexpr_utils.h"

//
//  Enumeration types.
//
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

//
//  The kinds of undo actions.
//  ADD...REPLACE must come first.
//
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

//
//  Structure types.
//
typedef struct colortype
{
    s16 id;            // An ordinal number (if this color combo is for a multiline regex).
    s16 fg;            // This combo's foreground color.
    s16 bg;            // This combo's background color.
    s16 pairnum;       // The pair number for this foreground/background color
                       // combination.
    s32 attributes;    // Pair number and brightness composed into ready-to-use
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
    s8            *filename;  //  The file where the syntax is extended.
    s64            lineno;    //  The number of the line of the extendsyntax command.
    s8            *data;      //  The text of the line.
    augmentstruct *next;      //  Next node.
} augmentstruct;

typedef struct syntaxtype
{
    char *name;                    // The name of this syntax.
    char *filename;                // File where the syntax is defined, or NULL if not an
                                   // included file.
    size_t         lineno;         // The line number where the 'syntax' command was found.
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

    s64 lineno;        // Line number of the error.
    s64 colno;         // Column # of the error.
    s8 *msg;           // Error message text.
    s8 *filename;      // Filename.
} lintstruct;

// More structure types.
typedef struct linestruct
{
    linestruct *next;        // Next node.
    linestruct *prev;        // Previous node.
    s8         *data;        // The text of this line.
    s64         lineno;      // The number of this line.
    s16        *multidata;   // Array of which multi-line regexes apply to this line.
    bool        has_anchor;  // Whether the user has placed an anchor at this line.
} linestruct;

typedef struct groupstruct
{
    groupstruct *next;  // The next group, if any.

    s64  top_line;      // First line of group.
    s64  bottom_line;   // Last line of group.
    s8 **indentations;  // String data used to restore the affected lines; one
                        // per line.
} groupstruct;

typedef struct undostruct
{
    undo_type type;
    /* The operation type that this undo item is for. */
    int xflags;
    /* Some flag data to mark certain corner cases. */
    ssize_t head_lineno;
    /* The line number where the operation began or ended. */
    size_t head_x;
    /* The x position where the operation began or ended. */
    char *strdata;
    /* String data to help restore the affected line. */
    size_t wassize;
    /* The file size before the action. */
    size_t newsize;
    /* The file size after the action. */
    groupstruct *grouping;
    /* Undo info specific to groups of lines. */
    linestruct *cutbuffer;
    /* A copy of the cutbuffer. */
    ssize_t tail_lineno;
    /* Mostly the line number of the current line; sometimes something else. */
    size_t tail_x;
    /* The x position corresponding to the above line number. */
    struct undostruct *next;
    /* A pointer to the undo item of the preceding action. */
} undostruct;

typedef struct poshiststruct
{
    char *filename;
    /* The full path plus name of the file. */
    ssize_t linenumber;
    /* The line where the cursor was when we closed the file. */
    ssize_t columnnumber;
    /* The column where the cursor was. */
    struct poshiststruct *next;
    /* The next item of position history. */
} poshiststruct;

typedef struct openfilestruct
{
    char       *filename;       // The file's name.
    linestruct *filetop;        // The file's first line.
    linestruct *filebot;        // The file's last line.
    linestruct *edittop;        // The current top of the edit window for this file.
    linestruct *current;        // The current line for this file.
    size_t      totsize;        // The file's total number of characters.
    size_t      firstcolumn;    // The starting column of the top line of the edit
                                // window When not in softwrap mode, it's always zero.
    size_t       current_x;     // The file's x-coordinate position.
    size_t       placewewant;   // The file's x position we would like.
    ssize_t      cursor_row;    // The row in the edit window that the cursor is on.
    struct stat *statinfo;      // The file's stat information from when it was
                                // opened or last saved.
    linestruct *spillage_line;  // The line for prepending stuff to during
                                // automatic hard-wrapping.
    linestruct *mark;           // The line in the file where the mark is set; NULL if not set.
    size_t      mark_x;         // The mark's x position in the above line.
    bool        softmark;       // Whether a marked region was made by holding Shift.
    format_type fmt;            // The file's format -- Unix or DOS or Mac.
    char       *lock_filename;  // The path of the lockfile, if we created one.
    undostruct *undotop;        // The top of the undo list.
    undostruct *current_undo;   // The current (i.e. next) level of undo.
    undostruct *last_saved;     // The undo item at which the file was last saved.
    undo_type   last_action;    // The type of the last action the user performed.
    bool        modified;       // Whether the file has been modified.
    syntaxtype *syntax;         // The syntax that applies to this file, if any.
    char       *errormessage;   // The ALERT message (if any) that occurred when opening
                                // the file.
    openfilestruct *next;       // The next open file, if any.
    openfilestruct *prev;       // The preceding open file, if any.
} openfilestruct;

typedef struct rcoption
{
    const s8 *name;  // The name of the rcfile option.
    long      flag;  // The flag associated with it, if any.
} rcoption;

typedef struct keystruct
{
    const s8 *keystr;      // The string that describes the keystroke, like "^C" or "M-R".
    s32       keycode;     // The integer that, together with meta, identifies the
                           // keystroke.
    s32 menus;             // The menus in which this keystroke is bound.
    void (*func)();        // The function to which this keystroke is bound.
    s32 toggle;            // If a toggle, what we're toggling.
    s32 ordinal;           // The how-manieth toggle this is, in order to be able to
                           // keep them in sequence.
    s8        *expansion;  // The string of keycodes to which this shortcut is expanded.
    keystruct *next;       // Next in the list.
} keystruct;

typedef struct funcstruct
{
    void (*func)();        /* The actual function to call. */
    const s8 *tag;         /* The function's help-line label, for example "Where Is". */
    const s8 *phrase;      /* The function's description for in the help viewer. */
    bool      blank_after; /* Whether to distance this function from the next in the
                              help viewer. */
    s32         menus;     /* In what menus this function applies. */
    funcstruct *next;      /* Next item in the list. */
} funcstruct;

typedef struct completionstruct
{
    s8               *word;
    completionstruct *next;
} completionstruct;

constexpr auto NANO_REG_EXTENDED = 1;
#define SYSCONFDIR "/etc"
