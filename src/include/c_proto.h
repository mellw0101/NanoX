#ifndef _C_PROTO__H
#define _C_PROTO__H

#include "c_defs.h"
#include "c/wchars.h"
#include "c/nfdreader.h"
#include "c/nevhandler.h"
#include "c/nfdlistener.h"


/* ---------------------------------------------------------- Extern variable's ---------------------------------------------------------- */


/* ----------------------------- nanox.c ----------------------------- */

extern struct termios original_state;

/* ----------------------------- global.c ----------------------------- */

extern volatile sig_atomic_t the_window_resized;

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
extern bool also_the_last;
extern bool have_palette;
extern bool rescind_colors;
extern bool nanox_rc_opensyntax;
extern bool nanox_rc_seen_color_command;
extern bool keep_mark;
extern bool suggest_on;
extern bool spotlighted;
extern bool mute_modifiers;
extern bool bracketed_paste;
extern bool keep_cutbuffer;
extern bool ran_a_tool;
extern bool last_key_was_bracket;

extern char *word_chars;
extern char *whitespace;
extern char *operating_dir;
extern char *homedir;
extern char *startup_problem;
extern char *nanox_rc_path;
extern char *backup_dir;
extern char *answer;
extern char *matchbrackets;
extern char *punct;
extern char *brackets;
extern char *quotestr;
extern char *alt_speller;
extern char *custom_nanorc;
extern char *syntaxstr;
extern char *statedir;
extern char *suggest_str;
extern char *present_path;
extern char *last_search;
extern char *title;
extern char *commandname;

extern char suggest_buf[1024];

extern const char *exit_tag;
extern const char *close_tag;
extern const char *term_env_var;
extern const char *term_program_env_var;

extern int editwinrows;
extern int editwincols;
extern int margin;
extern int sidebar;
extern int currmenu;
extern int hilite_attribute;
extern int suggest_len;
extern int cycling_aim;
extern int controlleft;
extern int controlright;
extern int controlup;
extern int controldown;
extern int controlhome;
extern int controlend;
extern int controldelete;
extern int controlshiftdelete;
extern int shiftleft;
extern int shiftright;
extern int shiftup;
extern int shiftdown;
extern int shiftcontrolleft;
extern int shiftcontrolright;
extern int shiftcontrolup;
extern int shiftcontroldown;
extern int shiftcontrolhome;
extern int shiftcontrolend;
extern int altleft;
extern int altright;
extern int altup;
extern int altdown;
extern int althome;
extern int altend;
extern int altpageup;
extern int altpagedown;
extern int altinsert;
extern int altdelete;
extern int shiftaltleft;
extern int shiftaltright;
extern int shiftaltup;
extern int shiftaltdown;
extern int mousefocusin;
extern int mousefocusout;

extern int *bardata;

extern int whitelen[2];
extern int interface_color_pair[NUMBER_OF_ELEMENTS];

extern float mouse_x;
extern float mouse_y;
extern float gui_width;
extern float gui_height;

extern long tabsize;
extern long fill;
extern long stripe_column;

extern Ulong wrap_at;
extern Ulong nanox_rc_lineno;
extern Ulong light_from_col;
extern Ulong light_to_col;

extern Ulong flags[1];

extern WINDOW *topwin;
extern WINDOW *midwin;
extern WINDOW *footwin;
extern WINDOW *suggestwin;

extern linestruct *cutbuffer;
extern linestruct *cutbottom;
extern linestruct *search_history;
extern linestruct *replace_history;
extern linestruct *execute_history;
extern linestruct *searchtop;
extern linestruct *searchbot;
extern linestruct *replacetop;
extern linestruct *replacebot;
extern linestruct *executetop;
extern linestruct *executebot;
extern linestruct *pletion_line;

extern openfilestruct *openfile;
extern openfilestruct *startfile;

extern Font *uifont;
extern Font *textfont;

extern Editor *openeditor;
extern Editor *starteditor;

extern colortype *nanox_rc_lastcolor;

extern colortype *color_combo[NUMBER_OF_ELEMENTS];

extern keystruct *sclist;
extern keystruct *planted_shortcut;

extern funcstruct *allfuncs;
extern funcstruct *tailfunc;
extern funcstruct *exitfunc;

extern regex_t search_regexp;
extern regex_t quotereg;

extern regmatch_t regmatches[10];

extern GLFWwindow *gui_window;

extern message_type lastmessage;

extern configstruct *config;

extern syntaxtype *nanox_rc_live_syntax;
extern syntaxtype *syntaxes;

/* ----------------------------- winio.c ----------------------------- */

extern Ulong from_x;
extern Ulong till_x;

/* ----------------------------- winio.c ----------------------------- */

extern Ulong waiting_codes;

extern int countdown;

extern bool recording;

/* ----------------------------- prompt.c ----------------------------- */

extern char *prompt;

extern Ulong typing_x;

/* ----------------------------- browser.c ----------------------------- */

extern char **filelist;
extern Ulong list_length;
extern Ulong usable_rows;
extern int piles;
extern int gauge;
extern Ulong selected;

/* ----------------------------- help.c ----------------------------- */

extern char *end_of_help_intro;

extern const char *start_of_help_body;

extern Ulong help_location;

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
char       *concatenate(const char *path, const char *name);
void        free_nulltermchararray(char **const argv);
void        append_chararray(char ***const array, Ulong *const len, char **const append, Ulong append_len);
char       *realloc_strncpy(char *dest, const char *const restrict src, Ulong length) __THROW _NODISCARD _RETURNS_NONNULL _NONNULL(1, 2);
char       *realloc_strcpy(char *dest, const char *const restrict src) __THROW _NODISCARD _RETURNS_NONNULL _NONNULL(1, 2);
void        get_homedir(void);
linestruct *line_from_number_for(openfilestruct *const file, long number);
linestruct *line_from_number(long number);
void        free_chararray(char **array, Ulong len);
void        recode_NUL_to_LF(char *string, Ulong length);
Ulong       recode_LF_to_NUL(char *string);
Ulong       get_page_start(Ulong column, int total_cols);
Ulong       xplustabs_for(openfilestruct *const file);
Ulong       xplustabs(void) _NODISCARD;
Ulong       wideness(const char *text, Ulong maxlen) _NODISCARD _NONNULL(1);
Ulong       actual_x(const char *text, Ulong column) _NODISCARD _NONNULL(1);
Ulong       breadth(const char *text) __THROW _NODISCARD _NONNULL(1);
Ulong       number_of_characters_in(const linestruct *const begin, const linestruct *const end) _NODISCARD _NONNULL(1, 2);
/* ----------------------------- Magicline ----------------------------- */
void new_magicline_for(openfilestruct *const file) _NONNULL(1);
void new_magicline(void);
void remove_magicline_for(openfilestruct *const file);
void remove_magicline(void);
/* ----------------------------- Mark is before cursor ----------------------------- */
bool mark_is_before_cursor_for(openfilestruct *const file);
bool mark_is_before_cursor(void) _NODISCARD;
/* ----------------------------- Get region ----------------------------- */
void get_region_for(openfilestruct *const file, linestruct **const top, Ulong *const top_x, linestruct **const bot, Ulong *const bot_x);
void get_region(linestruct **const top, Ulong *const top_x, linestruct **const bot, Ulong *const bot_x);
/* ----------------------------- Get range ----------------------------- */
void  get_range_for(openfilestruct *const file, linestruct **const top, linestruct **const bot);
void  get_range(linestruct **const top, linestruct **const bot);
bool  parse_line_column(const char *string, long *const line, long *const column);
Ulong tabstop_length(const char *const restrict string, Ulong index);
char *tab_space_string_for(openfilestruct *const file, Ulong *length);
char *tab_space_string(Ulong *length);
char *construct_full_tab_string(Ulong *length);
/* ----------------------------- Set placewewant ----------------------------- */
void set_pww_for(openfilestruct *const file);
void set_pww(void);
/* ----------------------------- Set cursor to end of line ----------------------------- */
void set_cursor_to_eol_for(openfilestruct *const file);
void set_cursor_to_eol(void);
/* ----------------------------- Set mark ----------------------------- */
void set_mark_for(openfilestruct *const file, long lineno, Ulong x);
void set_mark(long lineno, Ulong x);

char *indent_plus_tab(const char *const restrict string);

bool is_separate_word(Ulong position, Ulong length, const char *const restrict text);

const char *strstrwrapper(const char *const haystack, const char *const needle, const char *const start);


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


void set_marked_region_for(openfilestruct *const file, linestruct *const top, Ulong top_x, linestruct *const bot, Ulong bot_x, bool cursor_at_head);
void  do_tab_for(openfilestruct *const file, int rows, int cols);
void  do_tab(void);
Ulong indentlen(const char *const restrict string) __THROW _NODISCARD _CONST _NONNULL(1);
Ulong quote_length(const char *const restrict line);
void  add_undo_for(openfilestruct *const file, undo_type action, const char *const restrict message);
void  add_undo(undo_type action, const char *const restrict message);
void  update_undo_for(openfilestruct *const restrict file, undo_type action);
void  update_undo(undo_type action);
void  update_multiline_undo_for(openfilestruct *const file, long lineno, const char *const restrict indentation);
void  update_multiline_undo(long lineno, const char *const restrict indentation);
long  break_line(const char *textstart, long goal, bool snap_at_nl);
void  do_wrap_for(openfilestruct *const file, int cols);
void  do_wrap(void);
void  do_mark_for(openfilestruct *const file);
void  do_mark(void);
void  discard_until_for(openfilestruct *const buffer, const undostruct *const thisitem);
void  discard_until(const undostruct *thisitem);
bool  begpar(const linestruct *const line, int depth);
bool  inpar(const linestruct *const line);
void  do_block_comment(void);

/* ----------------------------- Length of white ----------------------------- */

Ulong length_of_white_for(openfilestruct *const file, const char *text);
Ulong length_of_white(const char *text);

/* ----------------------------- Compensate leftward ----------------------------- */

void compensate_leftward_for(openfilestruct *const file, linestruct *const line, Ulong leftshift);
void compensate_leftward(linestruct *const line, Ulong leftshift);

/* ----------------------------- Unindent ----------------------------- */

void unindent_a_line_for(openfilestruct *const file, linestruct *const line, Ulong indent_len);
void unindent_a_line(linestruct *const line, Ulong indent_len);
void do_unindent_for(openfilestruct *const file, int cols);
void do_unindent(void);

/* ----------------------------- Restore undo posx and mark ----------------------------- */

void restore_undo_posx_and_mark_for(openfilestruct *const file, int rows, undostruct *const u);
void restore_undo_posx_and_mark(undostruct *const u);

/* ----------------------------- Insert empty line ----------------------------- */

void insert_empty_line_for(openfilestruct *const file, linestruct *const line, bool above, bool autoindent);
void insert_empty_line(linestruct *const line, bool above, bool autoindent);
void do_insert_empty_line_above_for(openfilestruct *const file);
void do_insert_empty_line_above(void);
void do_insert_empty_line_below_for(openfilestruct *const file);
void do_insert_empty_line_below(void);

/* ----------------------------- Cursor is between brackets ----------------------------- */

bool cursor_is_between_brackets_for(openfilestruct *const file);
bool cursor_is_between_brackets(void);

/* ----------------------------- Indent ----------------------------- */

Ulong indent_length(const char *const restrict line);
void  indent_a_line_for(openfilestruct *const file, linestruct *const line, const char *const restrict indentation) _NONNULL(1, 2, 3);
void  indent_a_line(linestruct *const line, const char *const restrict indentation) _NONNULL(1, 2);
void  do_indent_for(openfilestruct *const file, int cols);
void  do_indent(void);
void  handle_indent_action_for(openfilestruct *const file, int rows, undostruct *const u, bool undoing, bool add_indent);
void  handle_indent_action(undostruct *const u, bool undoing, bool add_indent);

/* ----------------------------- Enclose marked region ----------------------------- */

char *enclose_str_encode(const char *const restrict p1, const char *const restrict p2);
void  enclose_str_decode(const char *const restrict str, char **const p1, char **const p2);
void  enclose_marked_region_for(openfilestruct *const file, const char *const restrict p1, const char *const restrict p2);
void  enclose_marked_region(const char *const restrict p1, const char *const restrict p2);

/* ----------------------------- Auto bracket ----------------------------- */

void auto_bracket_for(openfilestruct *const file, linestruct *const line, Ulong posx);
void auto_bracket(linestruct *const line, Ulong posx);
void do_auto_bracket_for(openfilestruct *const file);
void do_auto_bracket(void);

/* ----------------------------- Comment ----------------------------- */

bool  comment_line_for(openfilestruct *const file, undo_type action, linestruct *const line, const char *const restrict comment_seq);
bool  comment_line(undo_type action, linestruct *const line, const char *const restrict comment_seq);
char *get_comment_seq_for(openfilestruct *const file);
char *get_comment_seq(void);
void  do_comment_for(openfilestruct *const file, int cols);
void  do_comment(void);
void  handle_comment_action_for(openfilestruct *const file, int rows, undostruct *const u, bool undoing, bool add_comment);
void  handle_comment_action(undostruct *const u, bool undoing, bool add_comment);
char *copy_completion(const char *restrict text);
void  do_enter_for(openfilestruct *const file);
void  do_enter(void);
void  do_undo_for(CTX_PARAMS);
void  do_undo(void);
void  do_redo_for(CTX_PARAMS);
void  do_redo(void);
bool  find_paragraph(linestruct **const first, Ulong *const count);
void  do_verbatim_input(void);


/* ---------------------------------------------------------- suggestion.c ---------------------------------------------------------- */


void do_suggestion(void);
void find_suggestion(void);
void clear_suggestion(void);
void add_char_to_suggest_buf(void);
void draw_suggest_win(void);
void accept_suggestion(void);


/* ----------------------------------------------- csyntax.c ----------------------------------------------- */


/* ----------------- CSyntaxMacro ----------------- */

CSyntaxMacro *csyntaxmacro_create(void);
void          csyntaxmacro_free(void *ptr);

/* ----------------------------- Main parsing function ----------------------------- */

void syntaxfile_parse_csyntax(SyntaxFile *const sf);


/* ----------------------------------------------- csyntax.c ----------------------------------------------- */


Ulong wordstartindex(const char *const restrict string, Ulong pos, bool allowunderscore) __THROW _NODISCARD _NONNULL(1);
Ulong wordendindex(const char *const restrict string, Ulong pos, bool allowunderscore) __THROW _NODISCARD _NONNULL(1);
bool more_than_a_blank_away(const char *const restrict string, Ulong index, bool forward, Ulong *const restrict nsteps);


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
float            font_row_top_pix(Font *const f, long row) _NODISCARD _NONNULL(1);
float            font_row_bottom_pix(Font *const f, long row) _NODISCARD _NONNULL(1);
void             gui_font_change_size(Font *const f, Uint new_size);
void             gui_font_increase_size(Font *const f);
void             gui_font_decrease_size(Font *const f);
void             gui_font_decrease_line_height(Font *const f);
void             gui_font_increase_line_height(Font *const f);
void             gui_font_rows_cols(Font *const f, float width, float height, int *const outrows, int *const outcols);
bool             gui_font_row_from_pos(Font *const f, float y_top, float y_bot, float y_pos, long *outrow);
Ulong            gui_font_index_from_pos(Font *const f, const char *const restrict string, Ulong len, float rawx, float normx);
float            font_breadth(Font *const f, const char *const restrict string);
float            font_wideness(Font *const f, const char *const restrict string, Ulong to_index);
void             font_add_glyph(Font *const f, vertex_buffer_t *const buf, const char *const restrict current, const char *const restrict prev, Uint color, float *const pen_x, float *const pen_y);
void             font_vertbuf_add_mbstr(Font *const f, vertex_buffer_t *buf, const char *string, Ulong len, const char *previous, Uint color, float *const pen_x, float *const pen_y);
void             font_upload_texture_atlas(Font *const f);


/* ---------------------------------------------------------- gui/element_grid.c ---------------------------------------------------------- */


void     element_grid_create(int cell_size);
void     element_grid_free(void);
void     element_grid_set(Element *const e);
void     element_grid_remove(Element *const e);
Element *element_grid_get(float x, float y);
bool     element_grid_contains(float x, float y);


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
void     element_set_borders(Element *const e, float lsize, float tsize, float rsize, float bsize, Uint color /* Color *color */);
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


/* static */ char *do_lockfile(const char *const restrict filename, bool ask_the_user);
/* static */ char **filename_completion(const char *const restrict morsel, Ulong *const num_matches) ;

void  make_new_buffer_for(openfilestruct **const start, openfilestruct **const open);
void  make_new_buffer(void);
char *crop_to_fit(const char *const restrict name, Ulong room) ;
void  stat_with_alloc(const char *filename, struct stat **pstat);
void  prepare_for_display(void);
void  mention_name_and_linecount(void);
void  mention_name_and_linecount_for(openfilestruct *const file);
bool  delete_lockfile(const char *const restrict lockfilename) _NONNULL(1);
bool  write_lockfile(const char *const restrict lockfilename, const char *const restrict filename, bool modified);
bool  has_valid_path(const char *const restrict filename);
void  free_one_buffer(openfilestruct *orphan, openfilestruct **open, openfilestruct **start);
void  close_buffer(void);
void  close_buffer_for(openfilestruct *const orphan, openfilestruct **const start, openfilestruct **const open);
char *real_dir_from_tilde(const char *const restrict path) _RETURNS_NONNULL _NONNULL(1);
bool  is_dir(const char *const path) _NODISCARD _NONNULL(1);
char *get_full_path(const char *const restrict origpath);
char *check_writable_directory(const char *path);
int   diralphasort(const void *va, const void *vb);
void  set_modified_for(openfilestruct *const file);
void  set_modified(void);
char *encode_data(char *text, Ulong length);
void  init_operating_dir(void);
bool  outside_of_confinement(const char *const restrict somepath, bool tabbing);
void  init_backup_dir(void);
int   copy_file(FILE *inn, FILE *out, bool close_out);
char *safe_tempfile(FILE **stream);
void  redecorate_after_switch(void);
void  switch_to_prev_buffer(void);
void  switch_to_next_buffer(void);
char *get_next_filename(const char *const restrict name, const char *const restrict suffix);
int   open_file(const char *const restrict path, bool new_one, FILE **const f);
/* ----------------------------- Read file ----------------------------- */
void  read_file_into(openfilestruct *const file, int rows, int cols, FILE *const f, int fd, const char *const restrict filename, bool undoable);
void  read_file(FILE *f, int fd, const char *const restrict filename, bool undoable);
/* ----------------------------- Open buffer ----------------------------- */
bool open_buffer_for(openfilestruct **const start, openfilestruct **const open, int rows, int cols, const char *const restrict path, bool new_one);
bool open_buffer(const char *const restrict path, bool new_one);

char **username_completion(const char *const restrict morsel, Ulong length, Ulong *const num_matches);

char *input_tab(char *morsel, Ulong *const place, functionptrtype refresh_func, bool *const listed);


/* ---------------------------------------------------------- chars.c ---------------------------------------------------------- */


void  utf8_init(void);
bool  using_utf8(void);
bool  is_language_word_char(const char *pointer, Ulong index);
bool  is_lang_word_char(openfilestruct *const file);
bool  is_cursor_language_word_char(void);
bool  is_enclose_char(char ch);
bool  is_alpha_char(const char *const c);
bool  is_alnum_char(const char *const c);
bool  is_blank_char(const char *const c);
bool  is_prev_blank_char(const char *pointer, Ulong index);
bool  is_prev_cursor_blank_char(void);
bool  is_cursor_blank_char(void);
bool  is_cntrl_char(const char *const c);
bool  is_word_char(const char *const c, bool allow_punct);
bool  is_cursor_word_char(bool allow_punct);
bool  is_prev_word_char(const char *pointer, Ulong index, bool allow_punct);
bool  is_prev_cursor_word_char(bool allow_punct);
bool  is_prev_char(const char *pointer, Ulong index, const char ch);
bool  is_prev_cursor_char(const char ch);
bool  is_prev_char_one_of(const char *pointer, Ulong index, const char *chars);
bool  is_prev_cursor_char_one_of_for(openfilestruct *const file, const char *chars);
bool  is_prev_cursor_char_one_of(const char *chars);
bool  is_cursor_char(const char ch);
bool  is_char_one_of(const char *pointer, Ulong index, const char *chars);
bool  is_end_char_one_of(const char *const restrict ptr, const char *const restrict chars);
bool  is_cursor_char_one_of(const char *chars);
bool  is_between_chars(const char *pointer, Ulong index, const char pre_ch, const char post_ch);
bool  is_curs_between_chars_for(openfilestruct *const restrict file, char a, char b);
bool  is_curs_between_chars(char a, char b);
bool  is_between_any_char_pair(const char *const restrict ptr, Ulong index, const char **const restrict pairs, Ulong *const restrict out_index);
bool  is_curs_between_any_pair_for(openfilestruct *const restrict file, const char **const restrict pairs, Ulong *const restrict out_index);
bool  is_curs_between_any_pair(const char **const restrict pairs, Ulong *const restrict out_index);
bool  is_cursor_between_chars(const char pre_ch, const char post_ch);
char  control_mbrep(const char *const c, bool isdata);
int   mbtowide(wchar *const restrict wc, const char *const restrict c) __THROW _NODISCARD _NONNULL(1, 2);

/* ----------------------------- Encode multi byte from wide ----------------------------- */
int widetomb(Uint wc, char *const restrict mb);

bool  is_doublewidth(const char *const ch);
bool  is_zerowidth(const char *ch);
bool  is_cursor_zerowidth(void);
int   char_length(const char *const pointer);
Ulong mbstrlen(const char *pointer);
int   collect_char(const char *const str, char *c);
int   advance_over(const char *const str, Ulong *column);
Ulong step_left(const char *const buf, const Ulong pos);
void  step_cursor_left(openfilestruct *const file);
Ulong step_right(const char *const buf, const Ulong pos);
void  step_cursor_right(openfilestruct *const file);
int   mbstrcasecmp(const char *s1, const char *s2);
int   mbstrncasecmp(const char *s1, const char *s2, Ulong n);
char *mbstrcasestr(const char *haystack, const char *const needle);
char *revstrstr(const char *const haystack, const char *const needle, const char *pointer) __THROW _NODISCARD _NONNULL(1, 2, 3);
char *mbrevstrcasestr(const char *const haystack, const char *const needle, const char *pointer) _NODISCARD;
char *mbstrchr(const char *string, const char *const chr);
char *mbstrpbrk(const char *str, const char *accept);
char *mbrevstrpbrk(const char *const head, const char *const accept, const char *pointer);
bool  has_blank_char(const char *str);
bool  white_string(const char *str);
void  strip_leading_blanks_from(char *const str);
void  strip_leading_chars_from(char *str, const char ch);
bool  is_char_one_of(const char *pointer, Ulong index, const char *chars);


/* ---------------------------------------------------------- winio.c ---------------------------------------------------------- */



void  record_macro(void);
void  run_macro(void);
void  reserve_space_for(Ulong newsize);
void  implant(const char *const string);
int   get_input(WINDOW *const frame);
int   convert_CSI_sequence(const int *const seq, Ulong length, int *const consumed);
int   get_kbinput(WINDOW *const frame, bool showcursor);
char *get_verbatim_kbinput(WINDOW *const frame, Ulong *const count);
int   get_mouseinput(int *const my, int *const mx, bool allow_shortcuts);
Ulong get_softwrap_breakpoint(int cols, const char *const restrict linedata, Ulong leftedge, bool *kickoff, bool *end_of_line);
Ulong get_chunk_and_edge(int cols, Ulong column, linestruct *line, Ulong *leftedge);
Ulong extra_chunks_in(int cols, linestruct *const line);
Ulong chunk_for(int cols, Ulong column, linestruct *const line);
Ulong leftedge_for(int cols, Ulong column, linestruct *const line);
int   go_back_chunks_for(openfilestruct *const file, int cols, int nrows, linestruct **const line, Ulong *const leftedge);
int   go_back_chunks(int nrows, linestruct **const line, Ulong *const leftedge);
int   go_forward_chunks_for(openfilestruct *const file, int cols, int nrows, linestruct **const line, Ulong *const leftedge);
int   go_forward_chunks(int nrows, linestruct **const line, Ulong *const leftedge);
void  ensure_firstcolumn_is_aligned_for(openfilestruct *const file, int cols);
void  ensure_firstcolumn_is_aligned(void);
char *display_string(const char *text, Ulong column, Ulong span, bool isdata, bool isprompt);
bool  line_needs_update_for(openfilestruct *const file, int cols, Ulong old_column, Ulong new_column);
bool  line_needs_update(Ulong old_column, Ulong new_column);
bool  less_than_a_screenful_for(openfilestruct *const file, int rows, int cols, Ulong was_lineno, Ulong was_leftedge);
bool  less_than_a_screenful(Ulong was_lineno, Ulong was_leftedge);
Ulong actual_last_column_for(openfilestruct *const file, int cols, Ulong leftedge, Ulong column);
Ulong actual_last_column(Ulong leftedge, Ulong column);
bool  current_is_above_screen_for(openfilestruct *const file);
bool  current_is_above_screen(void);
bool  current_is_below_screen_for(openfilestruct *const file, int total_rows, int total_cols);
bool  current_is_below_screen(void);
bool  current_is_offscreen_for(openfilestruct *const file, int rows, int cols);
bool  current_is_offscreen(void);
void  adjust_viewport_for(openfilestruct *const file, int rows, int cols, update_type manner);
void  adjust_viewport(update_type manner);
void  place_the_cursor_for(openfilestruct *const file);
void  place_the_cursor(void);
void  set_blankdelay_to_one(void);
Ulong waiting_keycodes(void);
void  edit_scroll_for(openfilestruct *const file, bool direction);
void  edit_scroll(bool direction);
void  edit_redraw_for(openfilestruct *const file, int rows, int cols, linestruct *const old_current, update_type manner);
void  edit_redraw(linestruct *const old_current, update_type manner);
void  edit_refresh(void);
void  titlebar(const char *path);
void  blank_edit(void);
void  draw_all_subwindows(void);
void  blank_statusbar(void);
void  blank_titlebar(void);
void  blank_bottombars(void);
void  blank_it_when_expired(void);
void  wipe_statusbar(void);
void  post_one_key(const char *const restrict keystroke, const char *const restrict tag, int width);
void  bottombars(int menu);
void  warn_and_briefly_pause(const char *const restrict message);
void  statusline(message_type type, const char *const restrict format, ...);
void  statusbar_all(const char *const restrict msg);
void  report_cursor_position_for(openfilestruct *const file);
void  report_cursor_position(void);

/* ----------------------------- Curses ----------------------------- */

void blank_row_curses(WINDOW *const window, int row);
void statusline_curses_va(message_type type, const char *const restrict format, va_list ap);
void statusline_curses(message_type type, const char *const restrict msg, ...) _PRINTFLIKE(2, 3);
void statusbar_curses(const char *const restrict msg);
void minibar(void);
void post_one_key_curses(const char *const restrict keystroke, const char *const restrict tag, int width);
void bottombars_curses(int menu);
void place_the_cursor_curses_for(openfilestruct *const file);
void warn_and_briefly_pause_curses(const char *const restrict message);
void draw_row_marked_region_for_curses(openfilestruct *const file, int row, const char *const restrict converted, linestruct *const line, Ulong from_col);
void draw_row_marked_region_curses(int row, const char *const restrict converted, linestruct *const line, Ulong from_col);
void full_refresh(void);
void draw_row_curses_for(openfilestruct *const file, int row, const char *const restrict converted, linestruct *const line, Ulong from_col);
void draw_row_curses(int row, const char *const restrict converted, linestruct *const line, Ulong from_col);
int  update_line_curses_for(openfilestruct *const file, linestruct *const line, Ulong index);
int  update_line_curses(linestruct *const line, Ulong index);
int  update_softwrapped_line_curses_for(openfilestruct *const file, linestruct *const line);
int  update_softwrapped_line_curses(linestruct *const line);

/* ----------------------------- Spotlight curses ----------------------------- */

void spotlight_curses_for(openfilestruct *const file, Ulong from_col, Ulong to_col);
void spotlight_curses(Ulong from_col, Ulong to_col);

/* ----------------------------- Spotlight softwrapped curses ----------------------------- */

void spotlight_softwrapped_curses_for(openfilestruct *const file, Ulong from_col, Ulong to_col);
void spotlight_softwrapped_curses(Ulong from_col, Ulong to_col);


/* ---------------------------------------------------------- line.c ---------------------------------------------------------- */


bool line_in_marked_region_for(openfilestruct *const file, linestruct *const line);
bool line_in_marked_region(linestruct *const line);
char *line_last_mbchr(const linestruct *const line);
void move_line_data(linestruct *const line, bool up);
void move_lines_up_for(openfilestruct *const file);
void move_lines_up(void);
void move_lines_down_for(openfilestruct *const file);
void move_lines_down(void);


/* ---------------------------------------------------------- global.c ---------------------------------------------------------- */

void case_sens_void(void);
void regexp_void(void);
void backwards_void(void);
void get_older_item(void);
void get_newer_item(void);
void flip_replace(void);
void flip_goto(void);
void to_files(void);
void goto_dir(void);
void do_nothing(void);
void do_toggle(void);
void dos_format(void);
void mac_format(void);
void append_it(void);
void prepend_it(void);
void back_it_up(void);
void flip_execute(void);
void flip_pipe(void);
void flip_convert(void);
void flip_newbuffer(void);
void discard_buffer(void);
void do_cancel(void);
int keycode_from_string(const char *keystring);
const keystruct *first_sc_for(int menu, functionptrtype function);
Ulong shown_entries_for(int menu);
const keystruct *get_shortcut(int keycode);
/* ----------------------------- Func from key ----------------------------- */
functionptrtype func_from_key(int keycode);


/* ---------------------------------------------------------- search.c ---------------------------------------------------------- */


bool regexp_init(const char *regexp);
void tidy_up_after_search(void);
void goto_line_posx_for(openfilestruct *const file, int rows, long lineno, Ulong x);
void goto_line_posx(long lineno, Ulong x);
void not_found_msg(const char *const restrict str);


/* ---------------------------------------------------------- move.c ---------------------------------------------------------- */


void to_first_line_for(openfilestruct *const file);
void to_first_line(void);
void to_last_line_for(openfilestruct *const file, int rows);
void to_last_line(void);
void get_edge_and_target_for(openfilestruct *const file, int cols, Ulong *const leftedge, Ulong *const target_column);
void get_edge_and_target(Ulong *const leftedge, Ulong *target_column);
void do_page_up_for(openfilestruct *const file, int rows, int cols);
void do_page_up(void);
void do_page_down_for(openfilestruct *const file, int total_rows, int total_cols);
void do_page_down(void);
void to_top_row_for(openfilestruct *const file, int total_cols);
void to_top_row(void);
void to_bottom_row_for(openfilestruct *const file, int total_rows, int total_cols);
void to_bottom_row(void);
void do_cycle_for(openfilestruct *const file, int rows, int cols);
void do_cycle(void);
void do_center(void);
void do_para_begin(linestruct **const line);
void do_para_end(linestruct **const line);
void to_para_begin_for(openfilestruct *const file, int rows, int cols);
void to_para_begin(void);
void to_para_end_for(openfilestruct *const file, int rows, int cols);
void to_para_end(void);
void to_prev_block_for(openfilestruct *const file, int rows, int cols);
void to_prev_block(void);
void to_next_block_for(openfilestruct *const file, int rows, int cols);
void to_next_block(void);
void do_up_for(CTX_PARAMS);
void do_up(void);
void do_down_for(CTX_PARAMS);
void do_down(void);
void do_left_for(openfilestruct *const file, int rows, int cols);
void do_left(void);
void do_right_for(CTX_PARAMS);
void do_right(void);
void do_prev_word_for(openfilestruct *const file, bool allow_punct);
void do_prev_word(void);
void to_prev_word_for(CTX_PARAMS, bool allow_punct);
void to_prev_word(void);
bool do_next_word_for(openfilestruct *const file, bool after_ends, bool allow_punct);
bool do_next_word(bool after_ends);
void to_next_word_for(CTX_PARAMS, bool after_ends, bool allow_punct);
void to_next_word(void);
void do_home_for(CTX_PARAMS);
void do_home(void);
void do_end_for(CTX_PARAMS);
void do_end(void);
void do_scroll_up_for(CTX_PARAMS);
void do_scroll_up(void);
void do_scroll_down_for(CTX_PARAMS);
void do_scroll_down(void);


/* ---------------------------------------------------------- rcfile.c ---------------------------------------------------------- */


void  display_rcfile_errors(void);
void  jot_error(const char *const restrict format, ...);
void  check_for_nonempty_syntax(void);
void  set_interface_color(int element, char *combotext);
char *parse_next_word(char *ptr);
char *parse_argument(char *ptr);
bool  compile(const char *const restrict expression, int rex_flags, regex_t **const packed);
void  begin_new_syntax(char *ptr);
short closest_index_color(short red, short green, short blue);
short color_to_short(const char *colorname, bool *vivid, bool *thick);
// Uint  syntax_opt_type_from_str(const char *const restrict key);
bool  parse_combination(char *combotext, short *fg, short *bg, int *attributes);
void  grab_and_store(const char *const restrict kind, char *ptr, regexlisttype **const storage);
bool  parse_syntax_commands(const char *keyword, char *ptr);
void  parse_rule(char *ptr, int rex_flags);
bool  is_good_file(const char *const restrict file);
void  parse_one_include(char *file, syntaxtype *syntax);
void  parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only);
void  do_rcfiles(void);

void check_vitals_mapped(void);
void parse_binding(char *ptr, bool dobind);


/* ---------------------------------------------------------- color.c ---------------------------------------------------------- */


void prepare_palette_for(openfilestruct *const file);
void prepare_palette(void);
void check_the_multis_for(openfilestruct *const file, linestruct *const line);
void check_the_multis(linestruct *const line);
void set_interface_colorpairs(void);
void set_syntax_colorpairs(syntaxtype *sntx);
void find_and_prime_applicable_syntax_for(openfilestruct *const file);
void find_and_prime_applicable_syntax(void);
void precalc_multicolorinfo_for(openfilestruct *const file);
void precalc_multicolorinfo(void);


/* ---------------------------------------------------------- prompt.c ---------------------------------------------------------- */


/* static */ int do_statusbar_mouse(void);

void  lop_the_answer(void);
void  copy_the_answer(void);
void  paste_into_answer(void);
void  absorb_character(int input, functionptrtype function);
void  statusbar_discard_all_undo_redo(void);
void  do_statusbar_undo(void);
void  do_statusbar_redo(void);
void  do_statusbar_home(void);
void  do_statusbar_end(void);
void  do_statusbar_prev_word(void);
void  do_statusbar_next_word(void);
void  do_statusbar_left(void);
void  do_statusbar_right(void);
void  do_statusbar_backspace(bool with_undo);
void  do_statusbar_delete(void);
void  inject_into_answer(char *burst, Ulong count);
void  do_statusbar_chop_next_word(void);
void  do_statusbar_chop_prev_word(void);
Ulong get_statusbar_page_start(Ulong base, Ulong column);
void  put_cursor_at_end_of_answer(void);
void  add_or_remove_pipe_symbol_from_answer(void);
void  draw_the_promptbar(void);
int   ask_user(bool withall, const char *const restrict question);

int do_prompt(
  int menu, const char *const provided, linestruct **const histlist,
  functionptrtype refresh_func, const char *const format, ...);


/* ---------------------------------------------------------- history.c ---------------------------------------------------------- */


void  history_init(void);
void  reset_history_pointer_for(const linestruct *const item);
void  update_history(linestruct **const item, const char *text, bool avoid_duplicates);
char *get_history_completion(linestruct **const here, char *string, Ulong len);
bool  have_statedir(void);
void  load_history(void);
void  save_history(void);
void  load_poshistory(void);
void  update_poshistory_for(openfilestruct *const file);
void  update_poshistory(void);
bool  has_old_position(const char *const restrict file, long *const line, long *const column);


/* ---------------------------------------------------------- browser.c ---------------------------------------------------------- */


/* static */ void search_filename(bool forward);

void  read_the_list(const char *path, DIR *dir);
void  reselect(const char *const name);
void  browser_refresh(void);
void  findfile(const char *needle, bool forwards);
void  to_first_file(void);
void  to_last_file(void);
void  research_filename(bool forwards);
char *strip_last_component(const char *const restrict path);


/* ---------------------------------------------------------- help.c ---------------------------------------------------------- */


void wrap_help_text_into_buffer(void);
void do_help(void);


/* ---------------------------------------------------------- cut.c ---------------------------------------------------------- */


void expunge_for(openfilestruct *const file, int cols, undo_type action);
void expunge(undo_type action);
void extract_segment_for(openfilestruct *const file, int rows, int cols, linestruct *const top, Ulong top_x, linestruct *const bot, Ulong bot_x);
void extract_segment(linestruct *const top, Ulong top_x, linestruct *const bot, Ulong bot_x);
void cut_marked_region_for(openfilestruct *const file, int rows, int cols);
void cut_marked_region(void);
void do_snip_for(openfilestruct *const file, int rows, int cols, bool marked, bool until_eof, bool append);
void do_snip(bool marked, bool until_eof, bool append);
void cut_text_for(openfilestruct *const file, int rows, int cols);
void cut_text(void);
void cut_till_eof_for(openfilestruct *const file, int rows, int cols);
void cut_till_eof(void);
void zap_text_for(openfilestruct *const file, int rows, int cols);
void zap_text(void);
void do_delete_for(openfilestruct *const file, int rows, int cols);
void do_delete(void);
void ingraft_buffer_into(openfilestruct *const file, linestruct *top, linestruct *bot);
void ingraft_buffer(linestruct *topline);
void do_backspace_for(openfilestruct *const file, int rows, int cols);
void do_backspace(void);
void copy_from_buffer_for(openfilestruct *const file, int rows, linestruct *const head);
void copy_from_buffer(linestruct *const head);
void copy_marked_region_for(openfilestruct *const file);
void copy_marked_region(void);
void copy_text_for(openfilestruct *const file, int rows, int cols);
void copy_text(void);
void paste_text_for(CTX_PARAMS);
void paste_text(void);
void zap_replace_text_for(CTX_PARAMS, const char *const restrict replace_with, Ulong len);
void zap_replace_text(const char *const restrict replace_with, Ulong len);
void chop_previous_word_for(CTX_PARAMS);
void chop_previous_word(void);
void chop_next_word_for(CTX_PARAMS);
void chop_next_word(void);


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
void editor_set_rows_cols(Editor *const editor, float width, float heighr);
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
void statusline_gui_timed(message_type type, float seconds, const char *format, ...);
void statusline_gui_va(message_type type, const char *const restrict format, va_list ap);
void statusline_gui(message_type type, const char *format, ...);
void statusbar_gui(const char *const restrict msg);
void statusbar_draw(float fps);


/* ---------------------------------------------------------- nanox.c ---------------------------------------------------------- */


/* static */ void mouse_init(void);

linestruct *make_new_node(linestruct *prevnode);

/* ----------------------------- Splice node ----------------------------- */

void splice_node_for(openfilestruct *const file, linestruct *const after, linestruct *const node);
void splice_node(linestruct *const after, linestruct *const node);

/* ----------------------------- Delete node ----------------------------- */

void delete_node_for(openfilestruct *const file, linestruct *const node);
void delete_node(linestruct *const line);

/* ----------------------------- Unlink node ----------------------------- */

void unlink_node_for(openfilestruct *const file, linestruct *const node);
void unlink_node(linestruct *const node);

/* ----------------------------- Free lines ----------------------------- */

void free_lines_for(openfilestruct *const file, linestruct *src);
void free_lines(linestruct *const head);

linestruct *copy_node(const linestruct *const src) _NODISCARD _RETURNS_NONNULL _NONNULL(1);
void        copy_buffer_top_bot(const linestruct *src, linestruct **const top, linestruct **const bot);
linestruct *copy_buffer(const linestruct *src);
void        renumber_from(linestruct *line);
void        print_view_warning(void);
bool        in_restricted_mode(void);
void        disable_flow_control(void);
void        enable_flow_control(void);
void        disable_extended_io(void);
void        confirm_margin(void);
void        disable_kb_interrupt(void);
void        enable_kb_interrupt(void);
void        install_handler_for_Ctrl_C(void);
void        restore_handler_for_Ctrl_C(void);
void        terminal_init(void);
void        window_init(void);
void        regenerate_screen(void);
void        block_sigwinch(bool blockit);
void        handle_sigwinch(int signal);
void        suspend_nano(int _UNUSED signal);
void        continue_nano(int _UNUSED signal);
void        do_suspend(void);
void        reconnect_and_store_state(void);
void        handle_hupterm(int _UNUSED signal);
void        handle_crash(int _UNUSED signal);

/* ----------------------------- Inject ----------------------------- */

void inject_into_buffer(openfilestruct *const file, int rows, int cols, char *burst, Ulong count);
void inject(char *burst, Ulong count);


/* ---------------------------------------------------------- Defined in C++ ---------------------------------------------------------- */


// void inject(char *burst, Ulong count);
void render_line_text(int row, const char *str, linestruct *line, Ulong from_col) __THROW;
void apply_syntax_to_line(const int row, const char *converted, linestruct *line, Ulong from_col);
keystruct *strtosc(const char *input);
void finish(void) __THROW _NO_RETURN;
void syntax_check_file(openfilestruct *file);

bool wanted_to_move(functionptrtype f);
bool changes_something(functionptrtype f);

_END_C_LINKAGE


#endif /* _C_PROTO__H */
