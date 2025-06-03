#pragma once


/* ---------------------------------------------------------- Include's ---------------------------------------------------------- */

#include <stdbool.h>
#include "../../config.h"

#include "c/ascii_defs.h"
#include <fcio/proto.h>

/* NanoX */
#include "../../config.h"
#include "render.h"
// #define ASSERT_DEBUG
// #include "c/nassert.h"

/* stdlib */
#include <glob.h>
#include <pwd.h>
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
#include <libgen.h>
#if (defined(ENABLE_NLS) && defined(HAVE_LIBINTL_H))
# include <libintl.h>
#endif

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
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/* ncurses */
#include <ncursesw/ncurses.h>


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


/* ----------------------------- color.c ----------------------------- */

#define COLOR_8BIT(r, g, b, a)  MAX((r / 255.0f), 1), MAX((g / 255.0f), 1), MAX((b / 255.0f), 1), MAX(a, 1)

/* ----------------------------- General ----------------------------- */

#define IN_GUI_CONTEXT    (ISSET(USING_GUI) && openeditor)

#define CONTEXT_OPENFILE  (IN_GUI_CONTEXT ? openeditor->openfile : openfile)
#define CONTEXT_ROWS      (IN_GUI_CONTEXT ? openeditor->rows : editwinrows)
#define CONTEXT_COLS      (IN_GUI_CONTEXT ? openeditor->cols : editwincols)

#define TAB_BYTE_LEN                                      \
  /* The length in bytes of a tab, this depends on if     \
   * `TABS_TO_SPACES` is set, and if so the byte length   \
   * is `tabsize`.  Otherwise, its 1 for a '\t' char. */  \
  (ISSET(TABS_TO_SPACES) ? tabsize : 1)

#define RECODE_NUL_TO_LF(string, count)          \
  DO_WHILE(                                      \
    for (Ulong index=0; index<count; ++index) {  \
      if (!burst[index]) {                       \
        burst[index] = '\n';                     \
      }                                          \
    }                                            \
  )

/* Suppress warnings for __attribute__((warn_unused_result)). */
#define IGNORE_CALL_RESULT(call)  \
  DO_WHILE(                       \
    if (call) {}                  \
  )

#define ROOT_UID  (0)

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

/* ----------------------------- These will change and not remain. ----------------------------- */

/* Raw ptr rendering. */
#define R_3(color, start, end)     midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, start), start, (end - start), color)
/* Raw ptr rendering with len. */
#define R_LEN_3(color, ptr, len)   midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, ptr), ptr, len, color)
/* Render one char based on char_ptr. */
#define R_CHAR_2(color, char_ptr)  midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, char_ptr), char_ptr, 1, color)
/* Index based rendering. */
#define C_3(color, start, end)     render_part(start, end, color)
/* Index based rendering derived from ptr pos in line. */
#define C_PTR_3(color, start, end) render_part((start - line->data), (end - line->data), color)
/* Print a warning at end of line. */
#define W_1(str) \
  midwin_mv_add_nstr_color(row, (wideness(line->data, till_x) + margin + 1), str, str##_sllen, FG_YELLOW)
/* Print a error at end of line. */
#define E_1(str) \
  midwin_mv_add_nstr_color(row, (wideness(line->data, till_x) + margin + 1), str, const_strlen(str), FG_VS_CODE_RED)
#define SUGGEST_1(str)      \
  midwin_mv_add_nstr_color( \
    openfile->cursor_row, (xplustabs() + margin), (str + suggest_len), (strlen(str) - suggest_len), FG_SUGGEST_GRAY)

/* Main rendering caller. */
#define rendr(opt, ...) PP_CAT(opt##_, PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#define RENDR(opt, ...) PP_CAT(opt##_, PP_NARG(__VA_ARGS__))(__VA_ARGS__)


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

/* Identifiers for the different configuration options. */
#define OPERATINGDIR     (1 << 0)
#define FILL             (1 << 1)
#define MATCHBRACKETS    (1 << 2)
#define WHITESPACE       (1 << 3)
#define PUNCT            (1 << 4)
#define BRACKETS         (1 << 5)
#define QUOTESTR         (1 << 6)
#define SPELLER          (1 << 7)
#define BACKUPDIR        (1 << 8)
#define WORDCHARS        (1 << 9)
#define GUIDESTRIPE      (1 << 10)
#define CONF_OPT_TABSIZE (1 << 11)

/* Special keycodes for when a string bind has been partially implanted
 * or has an unpaired opening brace, or when a function in a string bind
 * needs execution or a specified function name is invalid. */
#define MORE_PLANTS        0x4EA
#define MISSING_BRACE      0x4EB
#define PLANTED_A_COMMAND  0x4EC
#define NO_SUCH_FUNCTION   0x4EF
/* A special keycode to signal the beginning and end of a bracketed paste. */
#define BRACKETED_PASTE_MARKER  0x4FB
/* A special keycode for when a key produces an unknown escape sequence. */
#define FOREIGN_SEQUENCE  0x4FC
/* A special keycode for plugging into the input stream after a suspension. */
#define KEY_FRESH  0x4FE
/* A special keycode for when we get a SIGWINCH (a window resize). */
#define KEY_WINCH  -2

/* Some other define`s. */
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

/* Basic control codes. */
#define ESC_CODE  (0x1B)
#define DEL_CODE  (0x7F)

/* Total elements. */
#define NUMBER_OF_ELEMENTS  (41)

#define THE_DEFAULT  (-1)
#define BAD_COLOR    (-2)

/* Used to encode both parts when enclosing a region. */
#define ENCLOSE_DELIM ":;:"

/* Native language support. */
#ifdef ENABLE_NLS
# define _(string)                     gettext(string)
# define P_(singular, plural, number)  ngettext(singular, plural, number)
#else
# define _(string)                     (char *)(string)
# define P_(singular, plural, number)  (number == 1 ? singular : plural)
#endif

/* For marking a string on which gettext() will be called later. */
#define gettext_noop(string)  (string)
#define N_(string)            gettext_noop(string)

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

/* ----------------------------- color.c ----------------------------- */

#define PACKED_UINT_VS_CODE_RED      PACKED_UINT(205, 49, 49, 255)
#define PACKED_UINT_EDIT_BACKGROUND  PACKED_UINT_FLOAT(0.1f, 0.1f, 0.1f, 1.0f)
#define PACKED_UINT_DEFAULT_BORDERS  PACKED_UINT_FLOAT(0.5f, 0.5f, 0.5f, 1.0f)

/* Base colors. */
#define FG_BLUE                              12
#define FG_GREEN                             13
#define FG_MAGENTA                           14
#define FG_LAGOON                            15
#define FG_YELLOW                            16
#define FG_RED                               17
#define FG_PINK                              18
#define FG_TEAL                              19
#define FG_MINT                              20
#define FG_PURPLE                            21
#define FG_MAUVE                             22
/* Vs-code colors. */
#define FG_VS_CODE_RED                       23
#define FG_VS_CODE_GREEN                     24
#define FG_VS_CODE_YELLOW                    25
#define FG_VS_CODE_BLUE                      26
#define FG_VS_CODE_MAGENTA                   27
#define FG_VS_CODE_CYAN                      28
#define FG_VS_CODE_WHITE                     29
#define FG_VS_CODE_BRIGHT_RED                30
#define FG_VS_CODE_BRIGHT_GREEN              31
#define FG_VS_CODE_BRIGHT_YELLOW             32
#define FG_VS_CODE_BRIGHT_BLUE               33
#define FG_VS_CODE_BRIGHT_MAGENTA            34
#define FG_VS_CODE_BRIGHT_CYAN               35
#define FG_COMMENT_GREEN                     36
#define FG_SUGGEST_GRAY                      37
/* Bg vs-code colors. */
#define BG_VS_CODE_RED                       38
#define BG_VS_CODE_BLUE                      39
#define BG_VS_CODE_GREEN                     40
/* Total elements. */
#define NUMBER_OF_ELEMENTS  (41)

/* Some color indexes. */
#define COLOR_LAGOON                         38
#define COLOR_PINK                           204
#define COLOR_TEAL                           35
#define COLOR_MINT                           48
#define COLOR_PURPLE                         163
#define COLOR_MAUVE                          134

/* Some base constexpr xterm index colors. */
static const short XTERM_GREY_1 = xterm_grayscale_color_index(80,  80,  80);
static const short XTERM_GREY_2 = xterm_grayscale_color_index(130, 130, 130);
static const short XTERM_GREY_3 = xterm_grayscale_color_index(180, 180, 180);
/* Vs-code colors. */
static const short VS_CODE_RED            = xterm_color_index(205, 49, 49);
static const short VS_CODE_GREEN          = xterm_color_index(13, 188, 121);
static const short VS_CODE_YELLOW         = xterm_color_index(229, 229, 16);
static const short VS_CODE_BLUE           = xterm_color_index(36, 114, 200);
static const short VS_CODE_MAGENTA        = xterm_color_index(188, 63, 188);
static const short VS_CODE_CYAN           = xterm_color_index(17, 168, 205);
static const short VS_CODE_WHITE          = xterm_color_index(229, 229, 229);
static const short VS_CODE_BRIGHT_RED     = xterm_color_index(241, 76, 76);
static const short VS_CODE_BRIGHT_GREEN   = xterm_color_index(35, 209, 139);
static const short VS_CODE_BRIGHT_YELLOW  = xterm_color_index(245, 245, 67);
static const short VS_CODE_BRIGHT_BLUE    = xterm_color_index(59, 142, 234);
static const short VS_CODE_BRIGHT_MAGENTA = xterm_color_index(214, 112, 214);
static const short VS_CODE_BRIGHT_CYAN    = xterm_color_index(41, 184, 219);
static const short COMMENT_GREEN          = xterm_color_index(0, 77, 0);

#define ENCODE_RGB_VALUE(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
static int encoded_idx_color[NUMBER_OF_ELEMENTS][2] = {
  {  0, ENCODE_RGB_VALUE(255, 255, 255) },
  { -1, -1 },
  {  ENCODE_RGB_VALUE( 36, 114, 200), 0 },
  { -1, -1 },
  {  0, ENCODE_RGB_VALUE(255, 255, 255) },
  { -1, -1 },
  {  0, ENCODE_RGB_VALUE(255, 255, 255) },
  { -1, -1 },
  { -1, -1 },
  {  ENCODE_RGB_VALUE(255, 255, 255), ENCODE_RGB_VALUE(205, 49, 49) },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { -1, -1 },
  { ENCODE_RGB_VALUE(205,  49,  49), -1 },
  { ENCODE_RGB_VALUE( 13, 188, 121), -1 },
  { ENCODE_RGB_VALUE(229, 229,  16), -1 },
  { ENCODE_RGB_VALUE( 36, 114, 200), -1 },
  { ENCODE_RGB_VALUE(188,  63, 188), -1 },
  { ENCODE_RGB_VALUE( 17, 168, 205), -1 },
  { ENCODE_RGB_VALUE(229, 229, 229), -1 },
  { ENCODE_RGB_VALUE(241,  76,  76), -1 },
  { ENCODE_RGB_VALUE( 35, 209, 139), -1 },
  { ENCODE_RGB_VALUE(245, 245,  67), -1 },
  { ENCODE_RGB_VALUE( 59, 142, 234), -1 },
  { ENCODE_RGB_VALUE(214, 112, 214), -1 },
  { ENCODE_RGB_VALUE( 41, 184, 219), -1 },
  { ENCODE_RGB_VALUE(  0,  77,   0), -1 },
  { ENCODE_RGB_VALUE( 80,  80,  80), -1 },
  { -1, ENCODE_RGB_VALUE(205,  49,  49) },
  { -1, ENCODE_RGB_VALUE( 36, 114, 200) },
  { -1, ENCODE_RGB_VALUE( 13, 188, 121) },
};

#define FG_VS_CODE_START   FG_VS_CODE_RED
#define FG_VS_CODE_END     (BG_VS_CODE_RED - 1)

#define BG_VS_CODE_START  BG_VS_CODE_RED
#define BG_VS_CODE_END    BG_VS_CODE_GREEN

static const short color_array[] = {
  VS_CODE_RED,
  VS_CODE_GREEN,
  VS_CODE_YELLOW,
  VS_CODE_BLUE,
  VS_CODE_MAGENTA,
  VS_CODE_CYAN,
  VS_CODE_WHITE,
  VS_CODE_BRIGHT_RED,
  VS_CODE_BRIGHT_GREEN,
  VS_CODE_BRIGHT_YELLOW,
  VS_CODE_BRIGHT_BLUE,
  VS_CODE_BRIGHT_MAGENTA,
  VS_CODE_BRIGHT_CYAN,
  COMMENT_GREEN,
  XTERM_GREY_1
};

static const short bg_vs_code_color_array[] = {
  VS_CODE_RED,
  VS_CODE_BLUE,
  VS_CODE_GREEN
};
#define BG_COLOR(index) bg_vs_code_color_array[index - BG_VS_CODE_START]

#define XTERM_DIRECT_SOFT_BLUE 24464


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
typedef struct coloroption           coloroption;
typedef struct configstruct          configstruct;
typedef struct configfilestruct      configfilestruct;
typedef struct rcoption              rcoption;
typedef struct keystruct             keystruct;
typedef struct funcstruct            funcstruct;
typedef struct completionstruct      completionstruct;

/* ----------------------------- synx.c ----------------------------- */

/* A structure that reprecents a position inside a `SyntaxFile` structure. */
typedef struct SyntaxFilePos    SyntaxFilePos;
/* A structure that reprecents a line inside a 'SyntaxFile' structure. */
typedef struct SyntaxFileLine   SyntaxFileLine;
/* The values in the hashmap that `syntax_file_t` uses. */
typedef struct SyntaxObject     SyntaxObject;
/* This reprecents a error found during parsing of a `SyntaxFile`. */
typedef struct SyntaxFileError  SyntaxFileError;
/* The structure that holds the data when parsing a file. */
typedef struct SyntaxFile       SyntaxFile;

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

/* ----------------------------- gui/statusbar.c ----------------------------- */

typedef struct Statusbar  Statusbar;


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
  HUSH,
  REMARK,
  INFO,
  NOTICE,
  AHEM,
  MILD,
  ALERT
# define VACUUM  VACUUM
# define HUSH    HUSH
# define REMARK  REMARK
# define INFO    INFO
# define NOTICE  NOTICE
# define AHEM    AHEM
# define MILD    MILD
# define ALERT   ALERT
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

typedef enum {
  TITLE_BAR = 0,
  LINE_NUMBER,
  GUIDE_STRIPE,
  SCROLL_BAR,
  SELECTED_TEXT,
  SPOTLIGHTED,
  MINI_INFOBAR,
  PROMPT_BAR,
  STATUS_BAR,
  ERROR_MESSAGE,
  KEY_COMBO,
  FUNCTION_TAG,
  /* Identifiers for color options. */
# define TITLE_BAR      TITLE_BAR
# define LINE_NUMBER    LINE_NUMBER
# define GUIDE_STRIPE   GUIDE_STRIPE
# define SCROLL_BAR     SCROLL_BAR
# define SELECTED_TEXT  SELECTED_TEXT
# define SPOTLIGHTED    SPOTLIGHTED
# define MINI_INFOBAR   MINI_INFOBAR
# define PROMPT_BAR     PROMPT_BAR
# define STATUS_BAR     STATUS_BAR
# define ERROR_MESSAGE  ERROR_MESSAGE
# define KEY_COMBO      KEY_COMBO
# define FUNCTION_TAG   FUNCTION_TAG
} color_option;

typedef enum {
  OVERWRITE,
  APPEND,
  PREPEND,
  EMERGENCY
  #define OVERWRITE  OVERWRITE
  #define APPEND     APPEND
  #define PREPEND    PREPEND
  #define EMERGENCY  EMERGENCY
} kind_of_writing_type;

typedef enum {
  CENTERING,
  FLOWING,
  STATIONARY
  #define CENTERING   CENTERING
  #define FLOWING     FLOWING
  #define STATIONARY  STATIONARY
} update_type;

typedef enum {
  SYNTAX_OPT_COLOR     = (1 << 0),
  SYNTAX_OPT_ICOLOR    = (1 << 1),
  SYNTAX_OPT_COMMENT   = (1 << 2),
  SYNTAX_OPT_TABGIVES  = (1 << 3),
  SYNTAX_OPT_LINTER    = (1 << 4),
  SYNTAX_OPT_FORMATTER = (1 << 5),
  /* Identifiers for the different syntax options. */
# define SYNTAX_OPT_COLOR      SYNTAX_OPT_COLOR
# define SYNTAX_OPT_ICOLOR     SYNTAX_OPT_ICOLOR
# define SYNTAX_OPT_COMMENT    SYNTAX_OPT_COMMENT
# define SYNTAX_OPT_TABGIVES   SYNTAX_OPT_TABGIVES
# define SYNTAX_OPT_LINTER     SYNTAX_OPT_LINTER
# define SYNTAX_OPT_FORMATTER  SYNTAX_OPT_FORMATTER
} SyntaxOptType;

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

struct coloroption {
  const char *name;  /* Name of the option. */
  int name_len;      /* Length of the name. */
  int color_index;   /* Index of the color. */
};

struct configstruct {
  struct {
    int color;            /* Line number color. */
    int attr;             /* Line number attribute. */
    int barcolor;         /* If verticalbar or fullverticalbar is set then this is the color of that bar. */
    bool verticalbar;     /* TRUE if user wants vertical bar next to linenumbers. */
    bool fullverticalbar; /* TRUE if user wants vertican bar next to linenumbers no matter the current amount off lines. */
  } linenumber;
  struct {
    int color; /* Prompt bar color. */
  } prompt;
  int minibar_color;            /* Minibar color. */
  int selectedtext_color;       /* Selected text color. */
};

struct configfilestruct {
  char *filepath;     /* Full path to the config file. */
  configstruct data;  /* Holds data while reading the config file. */
};

struct rcoption {
  const char *name;  /* The name of the rcfile option. */
  long flag;         /* The flag associated with it, if any. */
};

struct keystruct {
  const char *keystr;  /* The string that describes the keystroke, like "^C" or "M-R". */
  int keycode;         /* The integer that, together with meta, identifies the keystroke. */
  int menus;           /* The menus in which this keystroke is bound. */
  void (*func)(void);  /* The function to which this keystroke is bound. */
  int toggle;          /* If a toggle, what we're toggling. */
  int ordinal;         /* The how-manieth toggle this is, in order to be able to keep them in sequence. */
  char *expansion;     /* The string of keycodes to which this shortcut is expanded. */
  keystruct *next;     /* Next in the list. */
};

struct funcstruct {
  void (*func)(void);  /* The actual function to call. */
  const char *tag;     /* The function's help-line label, for example "Where Is". */
  const char *phrase;  /* The function's description for in the help viewer. */
  bool blank_after;    /* Whether to distance this function from the next in the help viewer. */
  int menus;           /* In what menus this function applies. */
  funcstruct *next;    /* Next item in the list. */
};

struct completionstruct {
  char *word;
  completionstruct *next;
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

struct SyntaxFilePos {
  int row;
  int column;
};

struct SyntaxFileLine {
  /* The next line in the double linked list of lines. */
  SyntaxFileLine *next;
  
  /* The previous line in the double linked list of lines. */
  SyntaxFileLine *prev;
  
  char *data;
  Ulong len;
  long  lineno;
};

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

/* ----------------------------- gui/element.c ----------------------------- */

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

  Uint color;
  Uint text_color;

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

/* ----------------------------- gui/statusbar.c ----------------------------- */

struct Statusbar {
  /* Boolian flags. */
  bool text_refresh_needed : 1;

  char *msg;
  float time;
  message_type type;

  vertex_buffer_t *buffer;

  Element *element;
};
