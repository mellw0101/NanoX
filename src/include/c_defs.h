#pragma once


/* ---------------------------------------------------------- Include's ---------------------------------------------------------- */


#include "c/ascii_defs.h"
#include <fcio/proto.h>

/* NanoX */
#include "../../config.h"
// #define ASSERT_DEBUG
// #include "c/nassert.h"

/* stdlib */
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


#define Schar   signed   char
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


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


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

/* ----------------------------- font.c ----------------------------- */

typedef struct Font  Font;

/* ----------------------------- element_gridmap.c ----------------------------- */

typedef struct {
  int x;
  int y;
} ElementGridpos;

typedef struct ElementGrid  ElementGrid;

/* ----------------------------- color.c ----------------------------- */

typedef struct {
  float r;
  float g;
  float b;
  float a;
} Color;

/* ----------------------------- scrollbar.c ----------------------------- */

typedef struct Scrollbar  Scrollbar;

/* ----------------------------- element.c ----------------------------- */

typedef struct Element  Element;

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
  } data_ptr;
};

