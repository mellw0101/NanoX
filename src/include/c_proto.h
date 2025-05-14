#ifndef _C_PROTO__H
#define _C_PROTO__H

#include "c_defs.h"
#include "c/nfdreader.h"
#include "c/nevhandler.h"
#include "c/nfdlistener.h"


/* ---------------------------------------------------------- Extern variable's ---------------------------------------------------------- */


extern long tabsize;


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


/* ---------------------------------------------------------- xstring.c ---------------------------------------------------------- */


void *xmeml_copy(const void *data, Ulong len);

#ifndef __cplusplus
char **split_string_nano(const char *const string, const char delim, bool allow_empty, Ulong *n) _RETURNS_NONNULL _NODISCARD _NONNULL(1);
#endif


/* ----------------------------------------------- utils.c ----------------------------------------------- */


#if !defined(__cplusplus)
  void      die(const char *format, ...);
  thread_t *get_nthreads(Ulong howmeny);
#endif
void free_nulltermchararray(char **const argv);


/* ----------------------------------------------- syntax/synx.c ----------------------------------------------- */


/* ------------ SyntaxFilePos ------------ */

SyntaxFilePos *syntaxfilepos_create(int row, int column) __THROW _NODISCARD _RETURNS_NONNULL;
void           syntaxfilepos_free(SyntaxFilePos *const pos) __THROW _NONNULL(1);
void           syntaxfilepos_set(SyntaxFilePos *const pos, int row, int column) __THROW _NONNULL(1);

/* ------------ SyntaxFileLine ------------ */

SyntaxFileLine *syntaxfileline_create(SyntaxFileLine *const prev) __THROW _NODISCARD _RETURNS_NONNULL;
void            syntaxfileline_free(SyntaxFileLine *const line) __THROW _NONNULL(1);
void            syntaxfileline_unlink(SyntaxFileLine *const line) __THROW _NONNULL(1);
void            syntaxfileline_free_lines(SyntaxFileLine *head) __THROW;
void            syntaxfileline_from_str(const char *const __restrict string, SyntaxFileLine **head, SyntaxFileLine **tail) __THROW _NONNULL(1, 2, 3);

/* ------------ SyntaxObject ------------ */

SyntaxObject *syntaxobject_create(void);
void          syntaxobject_free(SyntaxObject *const obj);
void          syntaxobject_unlink(SyntaxObject *const obj);
void          syntaxobject_free_objects(void *ptr);
void          syntaxobject_setfile(SyntaxObject *const obj, const char *const restrict file);
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


/* ----------------------------------------------- fd.c ----------------------------------------------- */


int  opennetfd(const char *address, Ushort port);
int  unixfd(void);


/* ----------------------------------------------- socket.c ----------------------------------------------- */


extern int serverfd;
extern int globclientfd;


void nanox_fork_socket(void);
int  nanox_socketrun(void);
int  nanox_socket_client(void);


/* ----------------------------------------------- text.c ----------------------------------------------- */


Ulong indentlen(const char *const restrict string) __THROW _NODISCARD _CONST _NONNULL(1);


/* ----------------------------------------------- csyntax.c ----------------------------------------------- */


/* ----------------- CSyntaxMacro ----------------- */

CSyntaxMacro *csyntaxmacro_create(void);
void          csyntaxmacro_free(void *ptr);

/* ----------------------------- Main parsing function ----------------------------- */

void syntaxfile_parse_csyntax(SyntaxFile *const sf);


/* ----------------------------------------------- csyntax.c ----------------------------------------------- */


Ulong wordstartindex(const char *const restrict string, Ulong pos, bool allowunderscore) __THROW _NODISCARD _NONNULL(1);
Ulong wordendindex(const char *const restrict string, Ulong pos, bool allowunderscore) __THROW _NODISCARD _NONNULL(1);


/* ----------------------------------------------- bracket.c ----------------------------------------------- */


void findblockcommentmatch(SyntaxFileLine *const startline, Ulong startidx, SyntaxFileLine **const endline, Ulong *endidx);
void skip_blkcomment(SyntaxFileLine **const outline, const char **const outdata);
void findbracketmatch(SyntaxFileLine **const outline, const char **const outdata);
void findnextchar(SyntaxFileLine **const outline, const char **const outdata);


/* ---------------------------------------------------------- gui/strpx.c ---------------------------------------------------------- */


float strpx_glyphlen(const char *const restrict current, const char *const restrict previous, texture_font_t *const font);
float *strpx_array(const char *const restrict str, Ulong len, float normx, Ulong *const outlen, texture_font_t *const font);
Ulong strpx_array_index(float *const array, Ulong len, float rawx);
Ulong strpx_str_index(texture_font_t *const font, const char *const restrict string, Ulong len, float rawx, float normx);


/* ---------------------------------------------------------- gui/cursor.c ---------------------------------------------------------- */


void set_cursor_type(GLFWwindow *const window, int type) _NONNULL(1);


/* ---------------------------------------------------------- gui/font.c ---------------------------------------------------------- */


Font            *gui_font_create(void);
void             gui_font_free(Font *const f);
void             gui_font_load(Font *const f, const char *const restrict path, Uint size, Uint atlas_size);
texture_font_t  *gui_font_get_font(Font *const f);
texture_atlas_t *gui_font_get_atlas(Font *const f);
texture_glyph_t *gui_font_get_glyph(Font *const f, const char *const restrict codepoint);
Uint             gui_font_get_size(Font *const f);
Uint             gui_font_get_atlas_size(Font *const f);
long             gui_font_get_line_height(Font *const f);
bool             gui_font_is_mono(Font *const f);
float            gui_font_height(Font *const f);
float            gui_font_row_baseline(Font *const f, long row);
void             gui_font_row_top_bot(Font *const f, long row, float *const top, float *const bot);
void             gui_font_change_size(Font *const f, Uint new_size);
void             gui_font_increase_size(Font *const f);
void             gui_font_decrease_size(Font *const f);
void             gui_font_decrease_line_height(Font *const f);
void             gui_font_increase_line_height(Font *const f);
void             gui_font_rows_cols(Font *const f, float width, float height, int *const outrows, int *const outcols);
bool             gui_font_row_from_pos(Font *const f, float y_top, float y_bot, float y_pos, long *outrow);
Ulong            gui_font_index_from_pos(Font *const f, const char *const restrict string, Ulong len, float rawx, float normx);


_END_C_LINKAGE


#endif /* _C_PROTO__H */
