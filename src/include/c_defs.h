#pragma once


/* ---------------------------------------------------------- Include's ---------------------------------------------------------- */


#include "c/ascii_defs.h"
#include <fcio/proto.h>

/* NanoX */
#include "../../config.h"
// #define ASSERT_DEBUG
// #include "c/nassert.h"

/* stdlib */
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


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


/* ----------------------------- General ----------------------------- */

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


/* ---------------------------------------------------------- Typedef's ---------------------------------------------------------- */


/* ----------------------------- General ----------------------------- */

typedef void (*FreeFuncPtr)(void *);

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

/* ----------------------------- font.c ----------------------------- */

typedef struct Font  Font;

/* ----------------------------- element_gridmap.c ----------------------------- */

typedef struct ElementGrid  ElementGrid;

/* ----------------------------- scrollbar.c ----------------------------- */

typedef struct Scrollbar  Scrollbar;

/* ----------------------------- menu.c ----------------------------- */

typedef struct CMenu  CMenu;

/* ----------------------------- element.c ----------------------------- */

typedef struct Element  Element;


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


/* ----------------------------- General ----------------------------- */

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
  AUTO_BRACKET,
  #define AUTO_BRACKET AUTO_BRACKET
  ZAP_REPLACE,
  #define ZAP_REPLACE ZAP_REPLACE
  INSERT_EMPTY_LINE,
  #define INSERT_EMPTY_LINE INSERT_EMPTY_LINE
} undo_type;

typedef enum {
  STATUSBAR_ADD,
  #define STATUSBAR_ADD STATUSBAR_ADD
  STATUSBAR_BACK,
  #define STATUSBAR_BACK STATUSBAR_BACK
  STATUSBAR_DEL,
  #define STATUSBAR_DEL STATUSBAR_DEL
  STATUSBAR_CHOP_NEXT_WORD,
  #define STATUSBAR_CHOP_NEXT_WORD STATUSBAR_CHOP_NEXT_WORD
  STATUSBAR_CHOP_PREV_WORD,
  #define STATUSBAR_CHOP_PREV_WORD STATUSBAR_CHOP_PREV_WORD
  STATUSBAR_OTHER,
  #define STATUSBAR_OTHER STATUSBAR_OTHER
} statusbar_undo_type;

/* Some extra flags for the undo function(s). */
typedef enum {
  WAS_BACKSPACE_AT_EOF = (1 << 1),
  #define WAS_BACKSPACE_AT_EOF WAS_BACKSPACE_AT_EOF
  WAS_WHOLE_LINE       = (1 << 2),
  #define WAS_WHOLE_LINE WAS_WHOLE_LINE
  INCLUDED_LAST_LINE   = (1 << 3),
  #define INCLUDED_LAST_LINE INCLUDED_LAST_LINE
  MARK_WAS_SET         = (1 << 4),
  #define MARK_WAS_SET MARK_WAS_SET
  CURSOR_WAS_AT_HEAD   = (1 << 5),
  #define CURSOR_WAS_AT_HEAD CURSOR_WAS_AT_HEAD
  HAD_ANCHOR_AT_START  = (1 << 6),
  #define HAD_ANCHOR_AT_START HAD_ANCHOR_AT_START
  SHOULD_NOT_KEEP_MARK = (1 << 7),
  #define SHOULD_NOT_KEEP_MARK SHOULD_NOT_KEEP_MARK
  INSERT_WAS_ABOVE     = (1 << 8)
  #define INSERT_WAS_ABOVE INSERT_WAS_ABOVE
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
    void      *raw;
    Scrollbar *sb;
    CMenu     *menu;
  } data_ptr;
};

