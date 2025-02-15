#ifndef _C_PROTO__H
#define _C_PROTO__H

#include "c_defs.h"
#include "c/nfdreader.h"
#include "c/nevhandler.h"
#include "c/nfdlistener.h"

_BEGIN_C_LINKAGE

extern int unix_socket_fd;

void unix_socket_connect(const char *path);
void unix_socket_debug(const char *format, ...);

/* This callback will be called upon when a fatal error occurs.
* That means this function should terminate the application. */
static void (*die_cb)(const char *, ...) = NULL;
static inline void set_c_die_callback(void (*cb)(const char *, ...)) {
  die_cb = cb;
}


/* ----------------------------------------------- hashmap.c ----------------------------------------------- */


HashMap *hashmap_create(void);
void     hashmap_free(HashMap *const map);
void     hashmap_set_free_value_callback(HashMap *const map, HashNodeValueFreeCb callback);
void     hashmap_insert(HashMap *map, const char *key, void *value);
void    *hashmap_get(HashMap *map, const char *key);
void     hashmap_remove(HashMap *map, const char *key);
int      hashmap_size(HashMap *const map);
int      hashmap_cap(HashMap *const map);
void     hashmap_forall(HashMap *const map, void (*action)(const char *const __restrict key, void *value));
void     hashmap_clear(HashMap *const map);


/* ------------ Tests ------------ */


void hashmap_thread_test(void);


/* ----------------------------------------------- xstring.c ----------------------------------------------- */


char *xstrl_copy(const char *string, Ulong len);
char *xstr_copy(const char *string);
void *xmeml_copy(const void *data, Ulong len);
char *valstr(const char *format, va_list ap);

#ifndef __cplusplus
char  *fmtstr(const char *format, ...) __THROW _RETURNS_NONNULL _NODISCARD _PRINTFLIKE(1, 2);
char  *concatpath(const char *const __restrict s1, const char *const __restrict s2) __THROW _RETURNS_NONNULL _NODISCARD _NONNULL(1, 2);
char  *measured_copy(const char *const __restrict string, Ulong len) _RETURNS_NONNULL _NODISCARD _NONNULL(1);
char  *copy_of(const char *const __restrict string) _RETURNS_NONNULL _NODISCARD _NONNULL(1);
char **split_string(const char *const string, const char delim, bool allow_empty, Ulong *n) _RETURNS_NONNULL _NODISCARD _NONNULL(1);
#endif


/* ----------------------------------------------- utils.c ----------------------------------------------- */


#if !defined(__cplusplus)
  const char *tail(const char *const path);
  const char *ext(const char *const path);
  void        die(const char *format, ...);
  thread     *get_nthreads(Ulong howmeny);
#endif
void free_nulltermchararray(char **const argv);


/* ----------------------------------------------- mem.c ----------------------------------------------- */


void *xmalloc(Ulong howmush)
 _RETURNS_NONNULL _NODISCARD;

void *xrealloc(void *ptr, Ulong howmush)
 _RETURNS_NONNULL _NODISCARD;

void *xcalloc(Ulong elemno, Ulong elemsize)
 _RETURNS_NONNULL _NODISCARD;


/* ----------------------------------------------- syntax/synx.c ----------------------------------------------- */


/* ------------ SyntaxFilePos ------------ */


SyntaxFilePos *syntaxfilepos_create(int row, int column) __THROW _RETURNS_NONNULL;
void           syntaxfilepos_free(SyntaxFilePos *const pos) __THROW _NONNULL(1);


/* ------------ SyntaxFileLine ------------ */


SyntaxFileLine *syntaxfileline_create(SyntaxFileLine *const prev);
void            syntaxfileline_free(SyntaxFileLine *const line);
void            syntaxfileline_unlink(SyntaxFileLine *const line);
void            syntaxfileline_free_lines(SyntaxFileLine *head);
void            syntaxfileline_from_str(const char *const string, SyntaxFileLine **head, SyntaxFileLine **tail) __THROW _NONNULL(1, 2, 3);


/* ------------ SyntaxObject ------------ */


SyntaxObject *syntaxobject_create(void);
void          syntaxobject_free(SyntaxObject *const obj);
void          syntaxobject_unlink(SyntaxObject *const obj);
void          syntaxobject_free_objects(void *ptr);
void          syntaxobject_setdata(SyntaxObject *const sfobj, void *const data, FreeFuncPtr freedatafunc);
void          syntaxobject_setpos(SyntaxObject *const obj, int row, int column);
void          syntaxobject_setcolor(SyntaxObject *const obj, SyntaxColor color);
void          syntaxobject_settype(SyntaxObject *const obj, SyntaxObjectType type);


/* ------------ SyntaxFile ------------ */


SyntaxFileError *syntaxfileerror_create(SyntaxFileError *const prev) __THROW;
void             syntaxfileerror_free(SyntaxFileError *const err) __THROW _NONNULL(1);
void             syntaxfileerror_unlink(SyntaxFileError *const err);
void             syntaxfileerror_free_errors(SyntaxFileError *head);


/* ------------ SyntaxFile ------------ */


SyntaxFile *syntaxfile_create(void);
void        syntaxfile_free(SyntaxFile *const sf);
void        syntaxfile_read(SyntaxFile *const sf, const char *const __restrict path);
void        syntaxfile_adderror(SyntaxFile *const sf, int row, int column, const char *const __restrict msg);
void        syntaxfile_addobject(SyntaxFile *const sf, const char *const __restrict key, SyntaxObject *const value);


/* ------------ Tests ------------ */


void syntaxfile_test_read(void);


/* ----------------------------------------------- files.c ----------------------------------------------- */


bool file_exists(const char *const __restrict path);
bool non_exec_file_exists(const char *const __restrict path);
void statalloc(const char *const __restrict path, struct stat **ptr);


/* ----------------------------------------------- fd.c ----------------------------------------------- */


bool lock_fd(int fd, short lock_type);
bool unlock_fd(int fd);
int  opennetfd(const char *address, Ushort port);
int  unixfd(void);


/* ----------------------------------------------- socket.c ----------------------------------------------- */


extern int serverfd;
extern int globclientfd;


void nanox_fork_socket(void);
int  nanox_socketrun(void);
int  nanox_socket_client(void);

/* ----------------------------------------------- dirs.c ----------------------------------------------- */


bool               dir_exists(const char *const __restrict path) __THROW _NODISCARD _NONNULL(1);
directory_entry_t *directory_entry_make(void) __THROW _NODISCARD _RETURNS_NONNULL;
directory_entry_t *directory_entry_extract(directory_t *const dir, Ulong idx) __THROW _NODISCARD _RETURNS_NONNULL _NONNULL(1);
bool               directory_entry_is_file(directory_entry_t *const entry);
bool               directory_entry_is_non_exec_file(directory_entry_t *const entry);
void               directory_entry_free(directory_entry_t *const entry) __THROW _NONNULL(1);
void               directory_data_init(directory_t *const dir) __THROW _NONNULL(1);
void               directory_data_free(directory_t *const dir) __THROW _NONNULL(1);
int                directory_get(const char *const __restrict path, directory_t *const output);
int                directory_get_recurse(const char *const __restrict path, directory_t *const output);


/* -------------- Tests -------------- */


void test_directory_t(const char *const dirpath);


/* ----------------------------------------------- text.c ----------------------------------------------- */


Ulong indentlen(const char *const string) __THROW _NODISCARD _CONST _NONNULL(1);


/* ----------------------------------------------- csyntax.c ----------------------------------------------- */


void process_syntaxfile_c(SyntaxFile *const sf) _NONNULL(1);

/* ----------------- CSyntaxMacro ----------------- */

CSyntaxMacro *csyntaxmacro_create(void);
void          csyntaxmacro_free(void *ptr);


/* ----------------------------------------------- csyntax.c ----------------------------------------------- */


Ulong wordstartindex(const char *const __restrict string, Ulong pos, bool allowunderscore) __THROW _NODISCARD _NONNULL(1);
Ulong wordendindex(const char *const __restrict string, Ulong pos, bool allowunderscore) __THROW _NODISCARD _NONNULL(1);


_END_C_LINKAGE

#endif /* _C_PROTO__H */
