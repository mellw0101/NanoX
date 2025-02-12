#pragma once

#include "c/ascii_defs.h"

/* NanoX */
#include "../../config.h"
#define ASSERT_DEBUG
#include "c/nassert.h"

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

/* Linux */
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>


#undef Ulong
#undef Uint
#undef Ushort
#undef Uchar
#undef Schar
#define Ulong   unsigned long
#define Uint    unsigned int
#define Ushort  unsigned short
#define Uchar   unsigned char
#define Schar   signed   char
#define BOOL    Uchar
#define wchar   wchar_t

#ifndef __cplusplus
  #define TRUE  1
  #define FALSE 0
#endif

#define SOCKLOG(...) unix_socket_debug(__VA_ARGS__)

#define UNIX_DOMAIN_SOCKET_PATH "/tmp/test"
#define BUF_SIZE 16384

#define MACRO_DO_WHILE(...) do {__VA_ARGS__} while(0)

#define S__LEN(s) (s), (sizeof(s) -1)

/* Some pthread mutex shorthands, to make usage painless. */
#define mutex_t        pthread_mutex_t
#define lock_mutex     pthread_mutex_lock
#define unlock_mutex   pthread_mutex_unlock
#define destroy_mutex  pthread_mutex_destroy
#define init_mutex     pthread_mutex_init

#ifndef __cplusplus
/* Shorthand for a thread. */
# define thread  pthread_t
#endif

#define under_mutex(mutex, action)  \
  MACRO_DO_WHILE(                   \
    pthread_mutex_lock(mutex);      \
    {                               \
      MACRO_DO_WHILE(action);       \
    }                               \
    pthread_mutex_unlock(mutex);    \
  )

#define ASSIGN_IF_VALID(ptr, value) MACRO_DO_WHILE( ((ptr) ? (*(ptr) = (value)) : ((int)0)); )
#define CALL_IF_VALID(funcptr, ...) MACRO_DO_WHILE( (funcptr) ? (funcptr)(__VA_ARGS__) : ((void)0); )

#define TIMER_START(name) \
  struct timespec name; \
  MACRO_DO_WHILE(\
    clock_gettime(CLOCK_MONOTONIC, &name);\
  )

#define TIMER_END(start, time_ms_name) \
  float time_ms_name; \
  MACRO_DO_WHILE(\
    struct timespec __timer_end;\
    clock_gettime(CLOCK_MONOTONIC, &__timer_end);\
    time_ms_name = \
      (((__timer_end.tv_sec - (start).tv_sec) * 1000000.0f) +\
      ((__timer_end.tv_nsec - (start).tv_nsec) / 1000.0f)); \
    time_ms_name = US_TO_MS(time_ms_name); \
  )

#define US_TO_MS(us)  ((us) / 1000.0f)

#define ENSURE_PTR_ARRAY_SIZE(array, cap, size)         \
  MACRO_DO_WHILE(                                       \
    if (size == cap) {                                  \
      cap *= 2;                                         \
      array = xrealloc(array, (sizeof(void *) * cap));  \
    }                                                   \
  )

#define TRIM_PTR_ARRAY(array, cap, size)              \
  MACRO_DO_WHILE(                                     \
    cap = (size + 1);                                 \
    array = xrealloc(array, (sizeof(void *) * cap));  \
  )

_BEGIN_C_LINKAGE


/* --------------------------------------------- nevhandler.c --------------------------------------------- */


/* Opaque structure that represents a event loop. */
typedef struct nevhandler nevhandler;


/* --------------------------------------------- nfdlistener.c --------------------------------------------- */


/* `Opaque`  Structure to listen to file events. */
typedef struct nfdlistener nfdlistener;

/* Structure that represents the event that the callback gets. */
typedef struct {
  Uint  mask;
  Uint  cookie;
  const char *file; /* The file this event is from. */
} nfdlistener_event;

typedef void (*nfdlistener_cb)(nfdlistener_event *);


/* --------------------------------------------- hashmap.c --------------------------------------------- */


typedef struct HashNode HashNode;
typedef struct HashMap  HashMap;

typedef void (*HashNodeValueFreeCb)(void *);


/* --------------------------------------------- synx.c --------------------------------------------- */


/* ---------------------- Enums ---------------------- */
typedef enum {
  SYNTAX_COLOR_NONE,
  SYNTAX_COLOR_RED,
  SYNTAX_COLOR_BLUE,
  #define SYNTAX_COLOR_NONE  SYNTAX_COLOR_NONE
  #define SYNTAX_COLOR_RED   SYNTAX_COLOR_RED
  #define SYNTAX_COLOR_BLUE  SYNTAX_COLOR_BLUE
} SyntaxColor;

typedef enum {
  SYNTAX_OBJECT_TYPE_NONE,
  SYNTAX_OBJECT_TYPE_KEYWORD,
  SYNTAX_OBJECT_TYPE_DEFINE,
  #define SYNTAX_OBJECT_TYPE_NONE     SYNTAX_OBJECT_TYPE_NONE
  #define SYNTAX_OBJECT_TYPE_KEYWORD  SYNTAX_OBJECT_TYPE_KEYWORD
  #define SYNTAX_OBJECT_TYPE_DEFINE   SYNTAX_OBJECT_TYPE_DEFINE
} SyntaxObjectType;


/* ---------------------- Structs ---------------------- */
typedef struct SyntaxFilePos  SyntaxFilePos;

/* A structure that reprecents a line inside a 'SyntaxFile' structure. */
typedef struct SyntaxFileLine  SyntaxFileLine;
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

/* The structure that holds the data when parsing a file. */
typedef struct SyntaxFile  SyntaxFile;
struct SyntaxFile {
  /* The absolut path to the file. */
  char *path;

  /* Hash map that holds all objects parsed from the given file. */
  HashMap *objects;

  /* First line of the file. */
  SyntaxFileLine *filetop;

  /* Last line of the file. */
  SyntaxFileLine *filebot;

  /* A ptr to stat structure for this file, useful to have. */
  struct stat *stat;
};


/* --------------------------------------------- dirs.c --------------------------------------------- */


typedef struct {
  Uchar type;         /* The type of entry this is.  Uses `dirent->d_type`. */
  char *name;         /* Name of the entry. */
  char *path;         /* The full path of the entry. */
  char *ext;          /* The extention, if any. */
  char *clean_name;   /* When `name` has a extention, this is `name` without that extention, otherwise this is `NULL`. */
  struct stat *stat;  /* Stat data for the entry. */
} directory_entry_t;

typedef struct {
  directory_entry_t **entries;
  Ulong   cap;
  Ulong   len;
  mutex_t mutex;
} directory_t;

typedef struct {
  char *path;
  directory_t *dir;
} directory_thread_data_t;


_END_C_LINKAGE
