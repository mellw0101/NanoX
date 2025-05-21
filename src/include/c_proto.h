#ifndef _C_PROTO__H
#define _C_PROTO__H

#include "c_defs.h"
#include "c/wchars.h"
#include "c/nfdreader.h"
#include "c/nevhandler.h"
#include "c/nfdlistener.h"


/* ---------------------------------------------------------- Extern variable's ---------------------------------------------------------- */


/* ----------------------------- global.c ----------------------------- */

extern volatile sig_atomic_t the_window_resized;

extern long tabsize;
extern long fill;

extern Ulong wrap_at;

extern WINDOW *topwin;
extern WINDOW *midwin;
extern WINDOW *footwin;

extern openfilestruct *openfile;
extern openfilestruct *startfile;

extern Font *uifont;
extern Font *textfont;

extern float mouse_x;
extern float mouse_y;

extern float gui_width;
extern float gui_height;

extern Editor *openeditor;
extern Editor *starteditor;

extern keystruct  *sclist;

extern funcstruct *allfuncs;
extern funcstruct *exitfunc;
extern funcstruct *tailfunc;

extern regex_t search_regexp;

extern const char *exit_tag;
extern const char *close_tag;

extern bool more_than_one;
extern bool inhelp;
extern bool as_an_at;
extern bool focusing;
extern bool refresh_needed;
extern bool shift_held;
extern bool on_a_vt;
extern bool is_shorter;
extern bool we_are_running;
extern bool control_C_was_pressed;
extern bool report_size;
extern bool shifted_metas;
extern bool perturbed;
extern bool recook;
extern bool meta_key;

extern char *word_chars;
extern char *whitespace;
extern char *operating_dir;
extern char *homedir;

extern int editwinrows;
extern int editwincols;
extern int margin;
extern int sidebar;
extern int currmenu;

extern Ulong from_x;
extern Ulong till_x;

extern int  whitelen[2];

extern Ulong flags[1];

extern GLFWwindow *gui_window;

extern message_type lastmessage;

extern int interface_color_pair[NUMBER_OF_ELEMENTS];

extern configstruct *config;

/* ----------------------------- color.c ----------------------------- */

extern Color color_vs_code_red;
extern Color color_white;

/* ----------------------------- winio.c ----------------------------- */

extern Ulong waiting_codes;

extern int countdown;

extern bool recording;

/* ----------------------------- General ----------------------------- */

extern ElementGrid *element_grid;
// extern Element *test_element;

extern Uint element_rect_shader;
extern Uint font_shader;

/* ----------------------------- Other ----------------------------- */

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
void        free_nulltermchararray(char **const argv);
char       *mallocstrcpy(char *dest, const char *src) __THROW _RETURNS_NONNULL _NONNULL(1, 2);
void        get_homedir(void);
linestruct *file_line_from_number(openfilestruct *const file, long number);
void        free_chararray(char **array, Ulong len);
Ulong       get_page_start(Ulong column);
Ulong       xplustabs_for(openfilestruct *const file);
Ulong       xplustabs(void) _NODISCARD;
Ulong       wideness(const char *text, Ulong maxlen) _NODISCARD _NONNULL(1);
Ulong       actual_x(const char *text, Ulong column) _NODISCARD _NONNULL(1);
Ulong       breadth(const char *text) __THROW _NODISCARD _NONNULL(1);
void        print_status(message_type type, const char *const restrict format, ...);
void        new_magicline(void);
void        remove_magicline(void);
bool        mark_is_before_cursor_for(openfilestruct *const file);
bool        mark_is_before_cursor(void) _NODISCARD;
void        get_region_for(openfilestruct *const file, linestruct **const top, Ulong *const top_x, linestruct **const bot, Ulong *const bot_x);
void        get_region(linestruct **const top, Ulong *const top_x, linestruct **const bot, Ulong *const bot_x);



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
void discard_until_in_buffer(openfilestruct *const buffer, const undostruct *const thisitem);
void discard_until(const undostruct *thisitem);


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
float            font_breadth(Font *const f, const char *const restrict string);
float            font_wideness(Font *const f, const char *const restrict string, Ulong maxlen);
void             font_add_glyph(Font *const f, vertex_buffer_t *const buf, const char *const restrict current, const char *const restrict prev, Color *const color, float *const pen_x, float *const pen_y);
void             font_vertbuf_add_mbstr(Font *const f, vertex_buffer_t *buf, const char *string, Ulong len, const char *previous, Color *const color, float *const pen_x, float *const pen_y);
void             font_upload_texture_atlas(Font *const f);


/* ---------------------------------------------------------- gui/element_grid.c ---------------------------------------------------------- */


void     element_grid_create(int cell_size);
void     element_grid_free(void);
void     element_grid_set(Element *const e);
void     element_grid_remove(Element *const e);
Element *element_grid_get(float x, float y);
bool     element_grid_contains(float x, float y);


/* ---------------------------------------------------------- gui/color.c ---------------------------------------------------------- */


Color *color_create(float r, float g, float b, float a);
void   color_copy(Color *const dst, const Color *const src);
void   color_set_rgba(Color *const color, float r, float g, float b, float a);
void   color_set_white(Color *const color);
void   color_set_black(Color *const color);
void   color_set_default_borders(Color *const color);
void   color_set_edit_background(Color *const color);

/* ---------------------------------------------------------- gui/element.c ---------------------------------------------------------- */


Element *element_create(float x, float y, float width, float height, bool in_gridmap);
void     element_free(Element *const e);
void     element_draw(Element *const e);
void     element_move(Element *const e, float x, float y);
void     element_resize(Element *const e, float width, float height);
void     element_move_resize(Element *const e, float x, float y, float width, float height);
void     element_move_y_clamp(Element *const e, float y, float min, float max);
void     element_delete_borders(Element *const e);
bool     element_is_ancestor(Element *const e, Element *const ancestor);
void     element_set_lable(Element *const e, const char *const restrict lable, Ulong len);
void     element_set_borders(Element *const e, float lsize, float tsize, float rsize, float bsize, Color *color);
void     element_set_layer(Element *const e, Ushort layer);
void     element_set_parent(Element *const e, Element *const parent);
void     element_set_raw_data(Element *const e, void *const data);
void     element_set_sb_data(Element *const e, Scrollbar *const data);
void     element_set_menu_data(Element *const e, CMenu *const data);
void     element_set_file_data(Element *const e, openfilestruct *const data);
void     element_set_editor_data(Element *const e, Editor *const data);


/* ---------------------------------------------------------- gui/scrollbar.c ---------------------------------------------------------- */


Scrollbar *scrollbar_create(Element *const parent, void *const data, ScrollbarUpdateFunc update, ScrollbarMovingFunc moving);
void       scrollbar_move(Scrollbar *const sb, float change);
void       scrollbar_draw(Scrollbar *const sb);
void       scrollbar_refresh_needed(Scrollbar *const sb);
bool       scrollbar_element_is_base(Scrollbar *const sb, Element *const e);
bool       scrollbar_element_is_thumb(Scrollbar *const sb, Element *const e);
void       scrollbar_mouse_pos_routine(Scrollbar *const sb, Element *const e, float was_y_pos, float y_pos);
float      scrollbar_width(Scrollbar *const sb);
void       scrollbar_show(Scrollbar *const sb, bool show);
void       scrollbar_set_thumb_color(Scrollbar *const sb, bool active);


/* ---------------------------------------------------------- gui/utils.c ---------------------------------------------------------- */


void draw_rect_rgba(float x, float y, float width, float height, float r, float g, float b, float a);
void render_vertbuf(Font *const f, vertex_buffer_t *buf);
vertex_buffer_t *vertbuf_create(void);


/* ---------------------------------------------------------- gui/menu.c ---------------------------------------------------------- */


CMenu *menu_create(Element *const parent, Font *const font, void *data, MenuPositionFunc position_routine, MenuAcceptFunc accept_routine);
CMenu *menu_create_submenu(CMenu *const parent, const char *const restrict lable, void *data, MenuAcceptFunc accept_routine);
void   menu_free(CMenu *const menu);
CMenu *menu_get_active(void);
void   menu_draw(CMenu *const menu);
void   menu_push_back(CMenu *const menu, const char *const restrict string);
void   menu_pos_refresh_needed(CMenu *const menu);
void   menu_text_refresh_needed(CMenu *const menu);
void   menu_scrollbar_refresh_needed(CMenu *const menu);
void   menu_show(CMenu *const menu, bool show);
void   menu_selected_up(CMenu *const menu);
void   menu_selected_down(CMenu *const menu);
void   menu_exit_submenu(CMenu *const menu);
void   menu_enter_submenu(CMenu *const menu);
void   menu_accept_action(CMenu *const menu);
void   menu_hover_action(CMenu *const menu, float x_pos, float y_pos);
void   menu_scroll_action(CMenu *const menu, bool direction, float x_pos, float y_pos);
void   menu_click_action(CMenu *const menu, float x_pos, float y_pos);
void   menu_clear_entries(CMenu *const menu);
void   menu_set_static_width(CMenu *const menu, float width);
void   menu_set_tab_accept_behavior(CMenu *const menu, bool accept_on_tab);
void   menu_set_arrow_depth_navigation(CMenu *const menu, bool enable_arrow_depth_navigation);
bool   menu_owns_element(CMenu *const menu, Element *const e);
bool   menu_element_is_main(CMenu *const menu, Element *const e);
bool   menu_should_accept_on_tab(CMenu *const menu);
bool   menu_allows_arrow_navigation(CMenu *const menu);
bool   menu_is_ancestor(CMenu *const menu, CMenu *const ancestor);
bool   menu_is_shown(CMenu *const menu);
Font  *menu_get_font(CMenu *const menu);
int    menu_len(CMenu *const menu);
int    menu_entry_qsort_strlen_cb(const void *a, const void *b);
void   menu_qsort(CMenu *const menu, CmpFuncPtr cmp_func);


/* ---------------------------------------------------------- files.c ---------------------------------------------------------- */


bool  delete_lockfile(const char *const restrict lockfilename) _NONNULL(1);
bool  write_lockfile(const char *const restrict lockfilename, const char *const restrict filename, bool modified);
bool  has_valid_path(const char *const restrict filename);
void  make_new_buffer(void);
void  free_one_buffer(openfilestruct *orphan, openfilestruct **open, openfilestruct **start);
void  close_buffer(void);
char *real_dir_from_tilde(const char *const restrict path) _RETURNS_NONNULL _NONNULL(1);
bool  is_dir(const char *const path) _NODISCARD _NONNULL(1);
char *get_full_path(const char *const restrict origpath);
char *check_writable_directory(const char *path);
int   diralphasort(const void *va, const void *vb);
void  set_modified_for(openfilestruct *const file);
void  set_modified(void);
bool open_buffer(const char *filename, bool new_one);


/* ---------------------------------------------------------- chars.c ---------------------------------------------------------- */


void utf8_init(void);
bool using_utf8(void);
bool is_language_word_char(const char *pointer, Ulong index);
bool is_cursor_language_word_char(void);
bool is_enclose_char(const char ch);
bool is_alpha_char(const char *const c);
bool is_alnum_char(const char *const c);
bool is_blank_char(const char *const c);
bool is_prev_blank_char(const char *pointer, Ulong index);
bool is_prev_cursor_blank_char(void);
bool is_cursor_blank_char(void);
bool is_cntrl_char(const char *const c);
bool is_word_char(const char *const c, bool allow_punct);
bool is_cursor_word_char(bool allow_punct);
bool is_prev_word_char(const char *pointer, Ulong index, bool allow_punct);
bool is_prev_cursor_word_char(bool allow_punct);
bool is_prev_char(const char *pointer, Ulong index, const char ch);
bool is_prev_cursor_char(const char ch);
bool is_prev_char_one_of(const char *pointer, Ulong index, const char *chars);
bool is_prev_cursor_char_one_of(const char *chars);
bool is_cursor_char(const char ch);
bool is_char_one_of(const char *pointer, Ulong index, const char *chars);
bool is_cursor_char_one_of(const char *chars);
bool is_between_chars(const char *pointer, Ulong index, const char pre_ch, const char post_ch);
bool is_cursor_between_chars(const char pre_ch, const char post_ch);
char control_mbrep(const char *const c, bool isdata);
int  mbtowide(wchar_t *wc, const char *const c);
bool is_doublewidth(const char *const ch);
bool is_zerowidth(const char *ch);
bool is_cursor_zerowidth(void);
int  char_length(const char *const pointer);
Ulong mbstrlen(const char *pointer);
int  collect_char(const char *const str, char *c);
int advance_over(const char *const str, Ulong *column);
Ulong step_left(const char *const buf, const Ulong pos);
Ulong step_right(const char *const buf, const Ulong pos);
int   mbstrcasecmp(const char *s1, const char *s2);
int  mbstrncasecmp(const char *s1, const char *s2, Ulong n);
char *mbstrcasestr(const char *haystack, const char *const needle);
char *revstrstr(const char *const haystack, const char *const needle, const char *pointer);
char *mbrevstrcasestr(const char *const haystack, const char *const needle, const char *pointer);
char *mbstrchr(const char *string, const char *const chr);
char *mbstrpbrk(const char *str, const char *accept);
char *mbrevstrpbrk(const char *const head, const char *const accept, const char *pointer);
bool has_blank_char(const char *str);
bool white_string(const char *str);
void strip_leading_blanks_from(char *str);
void strip_leading_chars_from(char *str, const char ch);
bool is_char_one_of(const char *pointer, Ulong index, const char *chars);


/* ---------------------------------------------------------- winio.c ---------------------------------------------------------- */


bool  get_has_more(void);
Ulong get_softwrap_breakpoint(const char *linedata, Ulong leftedge, bool *kickoff, bool *end_of_line);
Ulong get_chunk_and_edge(Ulong column, linestruct *line, Ulong *leftedge);
Ulong extra_chunks_in(linestruct *line);
Ulong chunk_for(Ulong column, linestruct *line);
Ulong leftedge_for(Ulong column, linestruct *line);
int   go_back_chunks_for(openfilestruct *const file, int nrows, linestruct **const line, Ulong *const leftedge);
int   go_back_chunks(int nrows, linestruct **const line, Ulong *const leftedge);
int   go_forward_chunks_for(openfilestruct *const file, int nrows, linestruct **const line, Ulong *const leftedge);
int   go_forward_chunks(int nrows, linestruct **const line, Ulong *const leftedge);
void  ensure_firstcolumn_is_aligned_for(openfilestruct *const file);
void  ensure_firstcolumn_is_aligned(void);
char *display_string(const char *text, Ulong column, Ulong span, bool isdata, bool isprompt);
bool  line_needs_update_for(openfilestruct *const file, Ulong old_column, Ulong new_column);
bool  line_needs_update(Ulong old_column, Ulong new_column);
bool  less_than_a_screenful_for(openfilestruct *const file, Ulong was_lineno, Ulong was_leftedge);
bool  less_than_a_screenful(Ulong was_lineno, Ulong was_leftedge);
Ulong actual_last_column_for(openfilestruct *const file, Ulong leftedge, Ulong column);
Ulong actual_last_column(Ulong leftedge, Ulong column);
bool current_is_above_screen_for(openfilestruct *const file);
bool current_is_above_screen(void);
bool current_is_below_screen_for(openfilestruct *const file);
bool current_is_below_screen(void);
bool current_is_offscreen_for(openfilestruct *const file);
bool current_is_offscreen(void);
void adjust_viewport_for(openfilestruct *const file, update_type manner);
void adjust_viewport(update_type manner);

/* ----------------------------- Curses ----------------------------- */

void blank_row_curses(WINDOW *const window, int row);
void blank_titlebar_curses(void);
void blank_statusbar_curses(void);
void blank_bottombars_curses(void);
void statusline_curses_va(message_type type, const char *const restrict format, va_list ap);
void statusline_curses(message_type type, const char *const restrict msg, ...) _PRINTFLIKE(2, 3);
void titlebar_curses(const char *path);
void minibar_curses(void);
void post_one_key_curses(const char *const restrict keystroke, const char *const restrict tag, int width);
void bottombars_curses(int menu);
void place_the_cursor_for_curses(openfilestruct *const file);
void place_the_cursor_curses(void);
void warn_and_briefly_pause_curses(const char *const restrict message);
void draw_row_marked_region_for_curses(openfilestruct *const file, int row, const char *const restrict converted, linestruct *const line, Ulong from_col);
void draw_row_marked_region_curses(int row, const char *const restrict converted, linestruct *const line, Ulong from_col);


/* ---------------------------------------------------------- line.c ---------------------------------------------------------- */


bool line_in_marked_region_for(openfilestruct *const file, linestruct *const line);
bool line_in_marked_region(linestruct *const line);


/* ---------------------------------------------------------- global.c ---------------------------------------------------------- */


void discard_buffer(void);
int keycode_from_string(const char *keystring);
const keystruct *first_sc_for(const int menu, functionptrtype function);
Ulong shown_entries_for(int menu);


/* ---------------------------------------------------------- search.c ---------------------------------------------------------- */


bool regexp_init(const char *regexp);
void tidy_up_after_search(void);


/* ---------------------------------------------------------- move.c ---------------------------------------------------------- */


void to_first_line_for(openfilestruct *const file);
void to_first_line(void);
void to_last_line_for(openfilestruct *const file);
void to_last_line(void);


/* ---------------------------------------------------------- gui/editor/topbar.c ---------------------------------------------------------- */


EditorTb *etb_create(Editor *const editor);
void etb_free(EditorTb *const etb);
void etb_draw(EditorTb *const etb);
void etb_active_refresh_needed(EditorTb *const etb);
void etb_text_refresh_needed(EditorTb *const etb);
void etb_entries_refresh_needed(EditorTb *const etb);
void etb_show_context_menu(EditorTb *const etb, Element *const from_element, bool show);
bool etb_element_is_main(EditorTb *const etb, Element *const e);
bool etb_owns_element(EditorTb *const etb, Element *const e);


/* ---------------------------------------------------------- gui/editor/editor.c ---------------------------------------------------------- */


void editor_create(bool new_buffer);
void editor_free(Editor *const editor);
void editor_confirm_margin(Editor *const editor);
void editor_set_rows_cols(Editor *const editor);
Editor *editor_from_file(openfilestruct *const file);
void editor_hide(Editor *const editor, bool hide);
void editor_close(Editor *const editor);
void editor_resize(Editor *const editor);
void editor_redecorate(Editor *const editor);
void editor_switch_to_prev(void);
void editor_switch_to_next(void);
void editor_switch_openfile_to_prev(void);
void editor_switch_openfile_to_next(void);
void editor_set_open(Editor *const editor);
void editor_check_should_close(void);
void editor_close_open_buffer(void);
void editor_open_new_empty_buffer(void);
void editor_update_all(void);
Ulong editor_get_page_start(Editor *const editor, const Ulong column);
float editor_cursor_x_pos(Editor *const editor, linestruct *const line, Ulong index);
linestruct *editor_get_text_line(Editor *const editor, float y_pos);
Ulong editor_get_text_index(Editor *const editor, linestruct *const line, float x_pos);
void editor_get_text_line_index(Editor *const editor, float x_pos, float y_pos, linestruct **const outline, Ulong *const outindex);
void editor_open_buffer(const char *const restrict path);
void editor_close_a_open_buffer(openfilestruct *const file);


/* ---------------------------------------------------------- gui/statusbar.c ---------------------------------------------------------- */


void statusbar_init(Element *const parent);
void statusbar_free(void);
void statusbar_timed_msg(message_type type, float seconds, const char *format, ...);
void statusbar_msg_va(message_type type, const char *const restrict format, va_list ap);
void statusbar_msg(message_type type, const char *format, ...);
void statusbar_draw(float fps);


/* ---------------------------------------------------------- nanox.c ---------------------------------------------------------- */


linestruct *make_new_node(linestruct *prevnode);
void        splice_node(linestruct *afterthis, linestruct *newnode);
void        delete_node(linestruct *line);
void        unlink_node(linestruct *line);
void        free_lines(linestruct *src);
linestruct *copy_node(const linestruct *src) _NODISCARD _RETURNS_NONNULL _NONNULL(1);
linestruct *copy_buffer(const linestruct *src);
void        renumber_from(linestruct *line);
void        print_view_warning(void);
bool        in_restricted_mode(void);
void        confirm_margin_for(openfilestruct *const file, int *const out_margin);
void        confirm_margin(void);
void        disable_kb_interrupt(void);
void        enable_kb_interrupt(void);
void        install_handler_for_Ctrl_C(void);
void        restore_handler_for_Ctrl_C(void);

/* ----------------------------- Curses ----------------------------- */

void window_init_curses(void);


_END_C_LINKAGE


#endif /* _C_PROTO__H */
