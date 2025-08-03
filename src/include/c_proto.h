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
extern bool ignore_rcfiles;
extern bool fill_used;

extern char last_bracket_char;

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

extern const char *epithet_flag[];

extern int editwinrows;
extern int editwincols;
extern int margin;
extern int sidebar;
extern int currmenu;
extern int hilite_attribute;
extern int suggest_len;
extern int cycling_aim;
extern int didfind;
extern int hardwrap;
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

extern Uint font_shader;
extern Uint rect_shader;

// extern float gui_width;
// extern float gui_height;

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

// extern GLFWwindow *gui_window;

extern message_type lastmessage;

extern configstruct *config;

extern syntaxtype *nanox_rc_live_syntax;
extern syntaxtype *syntaxes;

/* ----------------------------- files.c ----------------------------- */

/* static */ extern pid_t pid_of_command;
/* static */ extern pid_t pid_of_sender;
/* static */ extern bool should_pipe;

/* ----------------------------- winio.c ----------------------------- */

extern Ulong from_x;
extern Ulong till_x;

/* ----------------------------- winio.c ----------------------------- */

extern bool  recording;
extern int   countdown;
extern Ulong waiting_codes;

/* ----------------------------- prompt.c ----------------------------- */

extern char *prompt;
extern Ulong typing_x;

/* ----------------------------- General ----------------------------- */

extern ElementGrid *element_grid;
// extern Element *test_element;

// extern Uint element_rect_shader;

/* ----------------------------- Other ----------------------------- */

_BEGIN_C_LINKAGE

extern int unix_socket_fd;

void unix_socket_connect(const char *path);
void unix_socket_debug(const char *format, ...);

/* This callback will be called upon when a fatal error occurs.
 * That means this function should terminate the application. */
// static void (*die_cb)(const char *, ...) = NULL;
// static inline void set_c_die_callback(void (*cb)(const char *, ...)) {
//   die_cb = cb;
// }


/* ---------------------------------------------------------- xstring.c ---------------------------------------------------------- */


void *xmeml_copy(const void *data, Ulong len);

#ifndef __cplusplus
char **split_string_nano(const char *const string, const char delim, bool allow_empty, Ulong *n) _RETURNS_NONNULL _NODISCARD _NONNULL(1);
#endif


/* ----------------------------------------------- utils.c ----------------------------------------------- */


#if !defined(__cplusplus)
  // void      die(const char *format, ...);
  thread_t *get_nthreads(Ulong howmeny);
  #endif
char       *concatenate(const char *path, const char *name);
void        free_nulltermchararray(char **const argv);
void        append_chararray(char ***const array, Ulong *const len, char **const append, Ulong append_len);
char       *realloc_strncpy(char *dest, const char *const restrict src, Ulong length) __THROW _NODISCARD _RETURNS_NONNULL _NONNULL(1, 2);
char       *realloc_strcpy(char *dest, const char *const restrict src) __THROW _NODISCARD _RETURNS_NONNULL _NONNULL(1, 2);
void        get_homedir(void);
/* ----------------------------- Line from number ----------------------------- */
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
/* ----------------------------- Number of characters in ----------------------------- */
Ulong number_of_characters_in(const linestruct *const begin, const linestruct *const end) _NODISCARD _NONNULL(1, 2);
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
/* ----------------------------- Parse line column ----------------------------- */
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


/* static */ void copy_character(char **const from, char **const to);
/* static */ void squeeze(linestruct *const line, Ulong skip);

/* static */ bool fix_spello_for(CTX_ARGS, const char *const restrict word);
/* static */ bool fix_spello(const char *const restrict word);

/* static */ void concat_paragraph_for(openfilestruct *const file, linestruct *const line, Ulong count);
/* static */ void concat_paragraph(linestruct *const line, Ulong count);

/* static */ void rewrap_paragraph_for(openfilestruct *const file, int rows,
  linestruct **const line, const char *const restrict lead_str, Ulong lead_len);
/* static */ void rewrap_paragraph(linestruct **const line, const char *const restrict lead_str, Ulong lead_len);

/* static */ void justify_paragraph_for(openfilestruct *const file, int rows, linestruct **const line, Ulong count);
/* static */ void justify_paragraph(linestruct **const line, Ulong count);

/* static */ void construct_argument_list(char ***arguments, char *command, char *filename);

/* static */ bool replace_buffer_for(CTX_ARGS, const char *const restrict filename, undo_type action, const char *const restrict operation);
/* static */ bool replace_buffer(const char *const restrict filename, undo_type action, const char *const restrict operation);

/* static */ void treat_for(CTX_ARGS, char *tempfile, char *program, bool spelling);
/* static */ void treat(char *tempfile, char *program, bool spelling);

/* static */ void do_int_speller_for(CTX_ARGS, const char *const restrict tempfile);
/* static */ void do_int_speller(const char *const restrict tempfile);

/* static */ void justify_text_for(CTX_ARGS, bool whole_buffer);
/* static */ void justify_text(bool whole_buffer);

/* ----------------------------- Line indent plus tab ----------------------------- */
char *line_indent_plus_tab(const char *const restrict data, Ulong *const len);
/* ----------------------------- Set marked region ----------------------------- */
void set_marked_region_for(openfilestruct *const file, linestruct *const top, Ulong top_x, linestruct *const bot, Ulong bot_x, bool cursor_at_head);
/* ----------------------------- Do tab ----------------------------- */
void do_tab_for(CTX_ARGS);
void do_tab(void);
/* ----------------------------- Indentlen ----------------------------- */
Ulong indentlen(const char *const restrict string) __THROW _NODISCARD _CONST _NONNULL(1);
/* ----------------------------- Quote length ----------------------------- */
Ulong quote_length(const char *const restrict line);
/* ----------------------------- Add undo ----------------------------- */
void add_undo_for(openfilestruct *const file, undo_type action, const char *const restrict message);
void add_undo(undo_type action, const char *const restrict message);
/* ----------------------------- Update undo ----------------------------- */
void update_undo_for(openfilestruct *const restrict file, undo_type action);
void update_undo(undo_type action);
/* ----------------------------- Update multiline undo ----------------------------- */
void update_multiline_undo_for(openfilestruct *const file, long lineno, const char *const restrict indentation);
void update_multiline_undo(long lineno, const char *const restrict indentation);
/* ----------------------------- Break line ----------------------------- */
long break_line(const char *textstart, long goal, bool snap_at_nl);
/* ----------------------------- Do wrap ----------------------------- */
void do_wrap_for(openfilestruct *const file, int cols);
void do_wrap(void);
/* ----------------------------- Do mark ----------------------------- */
void do_mark_for(openfilestruct *const file);
void do_mark(void);
/* ----------------------------- Discard until ----------------------------- */
void discard_until_for(openfilestruct *const buffer, const undostruct *const thisitem);
void discard_until(const undostruct *thisitem);
/* ----------------------------- Begpar ----------------------------- */
bool begpar(const linestruct *const line, int depth);
/* ----------------------------- Inpar ----------------------------- */
bool inpar(const linestruct *const line);
/* ----------------------------- Do block comment ----------------------------- */
void do_block_comment_for(openfilestruct *const file);
void do_block_comment(void);
/* ----------------------------- Length of white ----------------------------- */
Ulong length_of_white_for(openfilestruct *const file, const char *text);
Ulong length_of_white(const char *text);
/* ----------------------------- Compensate leftward ----------------------------- */
void compensate_leftward_for(openfilestruct *const file, linestruct *const line, Ulong leftshift);
void compensate_leftward(linestruct *const line, Ulong leftshift);
/* ----------------------------- Unindent a line ----------------------------- */
void unindent_a_line_for(openfilestruct *const file, linestruct *const line, Ulong indent_len);
void unindent_a_line(linestruct *const line, Ulong indent_len);
/* ----------------------------- Do unindent ----------------------------- */
void do_unindent_for(openfilestruct *const file, int cols);
void do_unindent(void);
/* ----------------------------- Restore undo posx and mark ----------------------------- */
void restore_undo_posx_and_mark_for(openfilestruct *const file, int rows, undostruct *const u);
void restore_undo_posx_and_mark(undostruct *const u);
/* ----------------------------- Insert empty line ----------------------------- */
void insert_empty_line_for(openfilestruct *const file, linestruct *const line, bool above, bool autoindent);
void insert_empty_line(linestruct *const line, bool above, bool autoindent);
/* ----------------------------- Do insert empty line above ----------------------------- */
void do_insert_empty_line_above_for(openfilestruct *const file);
void do_insert_empty_line_above(void);
/* ----------------------------- Do insert empty line below ----------------------------- */
void do_insert_empty_line_below_for(openfilestruct *const file);
void do_insert_empty_line_below(void);
/* ----------------------------- Cursor is between brackets ----------------------------- */
bool cursor_is_between_brackets_for(openfilestruct *const file);
bool cursor_is_between_brackets(void);
/* ----------------------------- Indent length ----------------------------- */
Ulong indent_length(const char *const restrict line);
/* ----------------------------- Indent a line ----------------------------- */
void indent_a_line_for(openfilestruct *const file, linestruct *const line, const char *const restrict indentation) _NONNULL(1, 2, 3);
void indent_a_line(linestruct *const line, const char *const restrict indentation) _NONNULL(1, 2);
/* ----------------------------- Do indent ----------------------------- */
void do_indent_for(openfilestruct *const file, int cols);
void do_indent(void);
/* ----------------------------- Handle indent action ----------------------------- */
void handle_indent_action_for(openfilestruct *const file, int rows, undostruct *const u, bool undoing, bool add_indent);
void handle_indent_action(undostruct *const u, bool undoing, bool add_indent);
/* ----------------------------- Enclose str encode ----------------------------- */
char *enclose_str_encode(const char *const restrict p1, const char *const restrict p2);
/* ----------------------------- Enclose str decode ----------------------------- */
void enclose_str_decode(const char *const restrict str, char **const p1, char **const p2);
/* ----------------------------- Enclose marked region ----------------------------- */
void enclose_marked_region_for(openfilestruct *const file, const char *const restrict p1, const char *const restrict p2);
void enclose_marked_region(const char *const restrict p1, const char *const restrict p2);
/* ----------------------------- Auto bracket ----------------------------- */
void auto_bracket_for(openfilestruct *const file, linestruct *const line, Ulong posx);
void auto_bracket(linestruct *const line, Ulong posx);
/* ----------------------------- Do auto bracket for ----------------------------- */
void do_auto_bracket_for(openfilestruct *const file);
void do_auto_bracket(void);
/* ----------------------------- Comment line ----------------------------- */
bool comment_line_for(openfilestruct *const file, undo_type action, linestruct *const line, const char *const restrict comment_seq);
bool comment_line(undo_type action, linestruct *const line, const char *const restrict comment_seq);
/* ----------------------------- Get comment seq ----------------------------- */
char *get_comment_seq_for(openfilestruct *const file);
char *get_comment_seq(void);
/* ----------------------------- Do comment ----------------------------- */
void do_comment_for(openfilestruct *const file, int cols);
void do_comment(void);
/* ----------------------------- Handle comment action ----------------------------- */
void handle_comment_action_for(openfilestruct *const file, int rows, undostruct *const u, bool undoing, bool add_comment);
void handle_comment_action(undostruct *const u, bool undoing, bool add_comment);
/* ----------------------------- Copy completion ----------------------------- */
char *copy_completion(const char *restrict text);
/* ----------------------------- Do enter ----------------------------- */
void do_enter_for(openfilestruct *const file);
void do_enter(void);
/* ----------------------------- Do undo ----------------------------- */
void do_undo_for(CTX_ARGS);
void do_undo(void);
/* ----------------------------- Do redo ----------------------------- */
void do_redo_for(CTX_ARGS);
void do_redo(void);
/* ----------------------------- Count lines words and characters ----------------------------- */
void count_lines_words_and_characters_for(openfilestruct *const file);
void count_lines_words_and_characters(void);
/* ----------------------------- Do formatter ----------------------------- */
void do_formatter_for(CTX_ARGS, char *formatter);
void do_formatter(void);
/* ----------------------------- Do spell ----------------------------- */
void do_spell_for(CTX_ARGS);
void do_spell(void);
/* ----------------------------- Do justify ----------------------------- */
void do_justify_for(CTX_ARGS);
void do_justify(void);
/* ----------------------------- Do full justify ----------------------------- */
void do_full_justify_for(CTX_ARGS);
void do_full_justify(void);
/* ----------------------------- Do linter ----------------------------- */
void do_linter_for(CTX_ARGS_REF_OF, char *const linter);
void do_linter(void);
/* ----------------------------- Complete a word ----------------------------- */
void complete_a_word_for(CTX_ARGS_REF_OF);
void complete_a_word(void);
/* ----------------------------- Find paragraph ----------------------------- */
bool find_paragraph(linestruct **const first, Ulong *const count);
/* ----------------------------- Do verbatim input ----------------------------- */
void do_verbatim_input(void);
/* ----------------------------- Get previous char ----------------------------- */
char *get_previous_char(linestruct *line, Ulong xpos, linestruct **const outline, Ulong *const outxpos);
/* ----------------------------- Get next char ----------------------------- */
char *get_next_char(linestruct *line, Ulong xpos, linestruct **const outline, Ulong *const outxpos);
/* ----------------------------- Is previous char one of ----------------------------- */
bool is_previous_char_one_of(linestruct *const line, Ulong xpos,
  const char *const restrict matches, linestruct **const outline, Ulong *const outxpos);
/* ----------------------------- Is next char one of ----------------------------- */
bool is_next_char_one_of(linestruct *const line, Ulong xpos,
  const char *const restrict matches, linestruct **const outline, Ulong *const outxpos);
/* ----------------------------- Get previous char match ----------------------------- */
char *get_previous_char_match(linestruct *line, Ulong xpos, const char *const restrict matches,
  bool allow_literals, linestruct **const outline, Ulong *const outxpos);
/* ----------------------------- Tab helper ----------------------------- */
bool tab_helper(openfilestruct *const file);
/* ----------------------------- Lower case word ----------------------------- */
char *lower_case_word(const char *const restrict word);
/* ----------------------------- Mark whole file ----------------------------- */
void mark_whole_file_for(openfilestruct *const file);
void mark_whole_file(void);


/* ---------------------------------------------------------- arg.c ---------------------------------------------------------- */


/* ----------------------------- Proccess cli arguments ----------------------------- */
void proccess_cli_arguments(int *const argc, char **argv);


/* ---------------------------------------------------------- suggestion.c ---------------------------------------------------------- */


void do_suggestion(void);
void find_suggestion(void);
void clear_suggestion(void);
void add_char_to_suggest_buf(void);
void draw_suggest_win(void);
void accept_suggestion(void);
/* ----------------------------- Suggestmenu create ----------------------------- */
void suggestmenu_create(void);
/* ----------------------------- Suggestmenu free ----------------------------- */
void suggestmenu_free(void);
/* ----------------------------- Suggestmenu clear ----------------------------- */
void suggestmenu_clear(void);
/* ----------------------------- Suggestmenu load str ----------------------------- */
void suggestmenu_load_str(void);
/* ----------------------------- Suggestmenu find ----------------------------- */
void suggestmenu_find(void);
/* ----------------------------- Suggestmenu run ----------------------------- */
void suggestmenu_run(void);
/* ----------------------------- Suggestmenu draw ----------------------------- */
void suggestmenu_draw(void);
/* ----------------------------- Suggestmenu ----------------------------- */
Menu *suggestmenu(void);


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
/* ----------------------------- Prev word get ----------------------------- */
char *prev_word_get(const char *const restrict data, Ulong index, Ulong *const outlen, bool allow_underscore);


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


/* ----------------------------- Font create ----------------------------- */
Font *font_create(void);
/* ----------------------------- Font free ----------------------------- */
void font_free(Font *const f);
/* ----------------------------- Font load ----------------------------- */
void font_load(Font *const f, const char *const restrict path, Uint size, Uint atlas_size);
/* ----------------------------- Font get font ----------------------------- */
texture_font_t  *font_get_font(Font *const f);
/* ----------------------------- Font get atlas ----------------------------- */
texture_atlas_t *font_get_atlas(Font *const f);
/* ----------------------------- Font get glyph ----------------------------- */
texture_glyph_t *font_get_glyph(Font *const f, const char *const restrict codepoint);
/* ----------------------------- Font get size ----------------------------- */
Uint font_get_size(Font *const f);
/* ----------------------------- Font get atlas size ----------------------------- */
Uint font_get_atlas_size(Font *const f);
/* ----------------------------- Font get line height ----------------------------- */
long font_get_line_height(Font *const f);
/* ----------------------------- Font is mono ----------------------------- */
bool font_is_mono(Font *const f);
/* ----------------------------- Font height ----------------------------- */
float font_height(Font *const f);
/* ----------------------------- Font row baseline ----------------------------- */
float font_row_baseline(Font *const f, long row);
/* ----------------------------- Font row top bot ----------------------------- */
void font_row_top_bot(Font *const f, long row, float *const top, float *const bot);
/* ----------------------------- Font row top pix ----------------------------- */
float font_row_top_pix(Font *const f, long row) _NODISCARD _NONNULL(1);
/* ----------------------------- Font row bottom pix ----------------------------- */
float font_row_bottom_pix(Font *const f, long row) _NODISCARD _NONNULL(1);
/* ----------------------------- Font change size ----------------------------- */
void font_change_size(Font *const f, Uint new_size);
/* ----------------------------- Font increase size ----------------------------- */
void font_increase_size(Font *const f);
/* ----------------------------- Font decrease size ----------------------------- */
void font_decrease_size(Font *const f);
/* ----------------------------- Font decrease line height ----------------------------- */
void font_decrease_line_height(Font *const f);
/* ----------------------------- Font increase line height ----------------------------- */
void font_increase_line_height(Font *const f);
/* ----------------------------- Font rows cols ----------------------------- */
void font_rows_cols(Font *const f, float width, float height, int *const outrows, int *const outcols);
/* ----------------------------- Font row from pos ----------------------------- */
bool font_row_from_pos(Font *const f, float y_top, float y_bot, float y_pos, long *outrow);
/* ----------------------------- Font index from pos ----------------------------- */
Ulong font_index_from_pos(Font *const f, const char *const restrict string, Ulong len, float rawx, float normx);
/* ----------------------------- Font breadth ----------------------------- */
float font_breadth(Font *const f, const char *const restrict string);
/* ----------------------------- Font wideness ----------------------------- */
float font_wideness(Font *const f, const char *const restrict string, Ulong to_index);
/* ----------------------------- Font add glyph ----------------------------- */
void font_add_glyph(Font *const f, vertex_buffer_t *const buf, const char *const restrict current, const char *const restrict prev, Uint color, float *const pen_x, float *const pen_y);
/* ----------------------------- Font vertbuf add mbstr ----------------------------- */
void font_vertbuf_add_mbstr(Font *const f, vertex_buffer_t *buf, const char *string, Ulong len, const char *previous, Uint color, float *const pen_x, float *const pen_y);
/* ----------------------------- Font upload texture atlas ----------------------------- */
void font_upload_texture_atlas(Font *const f);
/* ----------------------------- Font add cursor ----------------------------- */
void font_add_cursor(Font *const f, vertex_buffer_t *const buf, long row, Uint color, float x, float rowzero_y);


/* ---------------------------------------------------------- gui/element_grid.c ---------------------------------------------------------- */


/* ----------------------------- Element grid create ----------------------------- */
void element_grid_create(int cell_size);
/* ----------------------------- Element grid free ----------------------------- */
void element_grid_free(void);
/* ----------------------------- Element grid set ----------------------------- */
void element_grid_set(Element *const e);
/* ----------------------------- Element grid remove ----------------------------- */
void element_grid_remove(Element *const e);
/* ----------------------------- Element grid get ----------------------------- */
Element *element_grid_get(float x, float y);
/* ----------------------------- Element grid contains ----------------------------- */
bool element_grid_contains(float x, float y);


/* ---------------------------------------------------------- gui/element.c ---------------------------------------------------------- */


/* ----------------------------- Element create ----------------------------- */
Element *element_create(float x, float y, float width, float height, bool in_gridmap);
/* ----------------------------- Element free ----------------------------- */
void element_free(Element *const e);
/* ----------------------------- Element draw ----------------------------- */
void element_draw(Element *const e);
/* ----------------------------- Element move ----------------------------- */
void element_move(Element *const e, float x, float y);
/* ----------------------------- Element resize ----------------------------- */
void element_resize(Element *const e, float width, float height);
/* ----------------------------- Element move resize ----------------------------- */
void element_move_resize(Element *const e, float x, float y, float width, float height);
/* ----------------------------- Element move y clamp ----------------------------- */
void element_move_y_clamp(Element *const e, float y, float min, float max);
/* ----------------------------- Element delete borders ----------------------------- */
void element_delete_borders(Element *const e);
/* ----------------------------- Element set color ----------------------------- */
void element_set_color(Element *const e, Uint color);
/* ----------------------------- Element is ancestor ----------------------------- */
bool element_is_ancestor(Element *const e, Element *const ancestor);
/* ----------------------------- Element set lable ----------------------------- */
void element_set_lable(Element *const e, const char *const restrict lable, Ulong len);
/* ----------------------------- Element set borders ----------------------------- */
void element_set_borders(Element *const e, float lsize, float tsize, float rsize, float bsize, Uint color);
/* ----------------------------- Element set layers ----------------------------- */
void element_set_layer(Element *const e, Ushort layer);
/* ----------------------------- Element set parent ----------------------------- */
void element_set_parent(Element *const e, Element *const parent);
/* ----------------------------- Element set raw data ----------------------------- */
void element_set_raw_data(Element *const e, void *const data);
/* ----------------------------- Element set sb data ----------------------------- */
void element_set_sb_data(Element *const e, Scrollbar *const data);
/* ----------------------------- Element set menu data ----------------------------- */
void element_set_menu_data(Element *const e, Menu *const data);
/* ----------------------------- Element set file data ----------------------------- */
void element_set_file_data(Element *const e, openfilestruct *const data);
/* ----------------------------- Element set editor data ----------------------------- */
void element_set_editor_data(Element *const e, Editor *const data);
/* ----------------------------- Element set rounded apex  ----------------------------- */
void element_set_rounded_apex_fraction(Element *const e, float t);
/* ----------------------------- Element set data callback ----------------------------- */
void element_set_data_callback(Element *const e, void *const ptr);
/* ----------------------------- Element set extra routine rect ----------------------------- */
void element_set_extra_routine_rect(Element *const e, ElementExtraCallback callback);


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


// void draw_rect_rgba(float x, float y, float width, float height, float r, float g, float b, float a);
void render_vertbuf(Font *const f, vertex_buffer_t *buf);
vertex_buffer_t *vertbuf_create(void);


/* ---------------------------------------------------------- gui/menu.c ---------------------------------------------------------- */


/* ----------------------------- Menu create ----------------------------- */
Menu *menu_create(Element *const parent, Font *const font, void *data, MenuPositionFunc position_routine, MenuAcceptFunc accept_routine);
/* ----------------------------- Menuu create submenu ----------------------------- */
Menu *menu_create_submenu(Menu *const parent, const char *const restrict lable, void *data, MenuAcceptFunc accept_routine);
/* ----------------------------- Menu free ----------------------------- */
void menu_free(Menu *const menu);
/* ----------------------------- Menu get active ----------------------------- */
Menu *menu_get_active(void);
/* ----------------------------- Menu draw ----------------------------- */
void menu_draw(Menu *const menu);
/* ----------------------------- Menu push back ----------------------------- */
void menu_push_back(Menu *const menu, const char *const restrict string);
/* ----------------------------- Menu refresh pos ----------------------------- */
void menu_refresh_pos(Menu *const menu);
/* ----------------------------- Menu refresh text ----------------------------- */
void menu_refresh_text(Menu *const menu);
/* ----------------------------- Menu refresh scrollbar ----------------------------- */
void menu_refresh_scrollbar(Menu *const menu);
/* ----------------------------- Menu show ----------------------------- */
void menu_show(Menu *const menu, bool show);
/* ----------------------------- Menu selected up ----------------------------- */
void menu_selected_up(Menu *const menu);
/* ----------------------------- Menu selected down ----------------------------- */
void menu_selected_down(Menu *const menu);
/* ----------------------------- Menu submenu exit ----------------------------- */
void menu_submenu_exit(Menu *const menu);
/* ----------------------------- Menu submenu enter ----------------------------- */
void menu_submenu_enter(Menu *const menu);
/* ----------------------------- Menu action accept ----------------------------- */
void menu_action_accept(Menu *const menu);
/* ----------------------------- Menu action hover ----------------------------- */
void menu_action_hover(Menu *const menu, float x_pos, float y_pos);
/* ----------------------------- Menu scroll action ----------------------------- */
void menu_action_scroll(Menu *const menu, bool direction, float x_pos, float y_pos);
/* ----------------------------- Menu click action ----------------------------- */
void menu_action_click(Menu *const menu, float x_pos, float y_pos);
/* ----------------------------- Menu clear entries ----------------------------- */
void menu_clear_entries(Menu *const menu);
/* ----------------------------- Menu set static width ----------------------------- */
void menu_set_static_width(Menu *const menu, float width);
/* ----------------------------- Menu behavior tab accept ----------------------------- */
void menu_behavior_tab_accept(Menu *const menu, bool accept_on_tab);
/* ----------------------------- Menu behavior arrow depth navigation ----------------------------- */
void menu_behavior_arrow_depth_navigation(Menu *const menu, bool enable_arrow_depth_navigation);
/* ----------------------------- Menu set lable offset ----------------------------- */
void menu_set_lable_offset(Menu *const menu, Ushort pixels);
/* ----------------------------- Menu owns element ----------------------------- */
bool menu_owns_element(Menu *const menu, Element *const e);
/* ----------------------------- Menu element is main ----------------------------- */
bool menu_element_is_main(Menu *const menu, Element *const e);
/* ----------------------------- Menu allows accept on tab ----------------------------- */
bool menu_allows_accept_on_tab(Menu *const menu);
/* ----------------------------- Menu allows arrow depth navigation ----------------------------- */
bool menu_allows_arrow_depth_navigation(Menu *const menu);
/* ----------------------------- Menu is ancestor ----------------------------- */
bool menu_is_ancestor(Menu *const menu, Menu *const ancestor);
/* ----------------------------- Menu is shown ----------------------------- */
bool menu_is_shown(Menu *const menu);
/* ----------------------------- Menu get font ----------------------------- */
Font *menu_get_font(Menu *const menu);
/* ----------------------------- Menu len ----------------------------- */
int menu_len(Menu *const menu);
/* ----------------------------- Menu qsort cb strlen ----------------------------- */
int menu_qsort_cb_strlen(const void *a, const void *b);
/* ----------------------------- Menu qsort ----------------------------- */
void menu_qsort(Menu *const menu, CmpFuncPtr cmp_func);


/* ---------------------------------------------------------- gui/frame.c ---------------------------------------------------------- */


/* ----------------------------- Frame start ----------------------------- */
void frame_start(void);
/* ----------------------------- Frame end ----------------------------- */
void frame_end(void);
/* ----------------------------- Frame get rate ----------------------------- */
int frame_get_rate(void);
/* ----------------------------- Frame set rate ----------------------------- */
void frame_set_rate(int x);
/* ----------------------------- Frame get time ms ----------------------------- */
double frame_get_time_ms(void);
/* ----------------------------- Frame get time ns ----------------------------- */
Llong frame_get_time_ns(void);
/* ----------------------------- Frame should poll ----------------------------- */
bool frame_should_poll(void);
/* ----------------------------- Frame set poll ----------------------------- */
void frame_set_poll(void);
/* ----------------------------- Frame elapsed ----------------------------- */
Ulong frame_elapsed(void);
/* ----------------------------- Frame should report ----------------------------- */
void frame_should_report(bool print_times);
/* ----------------------------- Frame elapsed time ----------------------------- */
Llong frame_elapsed_time(void);


/* ---------------------------------------------------------- gui/monitor.c ---------------------------------------------------------- */


int monitor_count(void);
int monitor_return_first_monitor_rate(void);
/* ----------------------------- Monitor get array ----------------------------- */
SDL_DisplayID *monitor_get_all(int *const count);
/* ----------------------------- Monitor get mode ----------------------------- */
const SDL_DisplayMode *monitor_get_mode(SDL_DisplayID monitor);
/* ----------------------------- Monitor refresh rate array ----------------------------- */
int *monitor_refresh_rate_array(int *const count);
/* ----------------------------- Monitor closest refresh rate ----------------------------- */
int monitor_closest_refresh_rate(int rate);
/* ----------------------------- Monitor fastest refresh rate ----------------------------- */
int monitor_fastest_refresh_rate(void);
/* ----------------------------- Monitor fastest refresh rate from array ----------------------------- */
int monitor_fastest_refresh_rate_from_array(int *const rates, int count);
/* ----------------------------- Monitor current ----------------------------- */
// GLFWmonitor *monitor_current(void);
/* ----------------------------- Monitor mode ----------------------------- */
// const SDL_DisplayMode *monitor_mode(void);
/* ----------------------------- Monitor refresh rate ----------------------------- */
int monitor_refresh_rate(void);


/* ---------------------------------------------------------- gui/promptmenu.c ---------------------------------------------------------- */


/* ----------------------------- Promptmenu create ----------------------------- */
void promptmenu_create(void);
/* ----------------------------- Promptmenu free ----------------------------- */
void promptmenu_free(void);
/* ----------------------------- Promptmenu draw ----------------------------- */
void promptmenu_draw(void);
/* ----------------------------- Promptmenu open ----------------------------- */
void promptmenu_open(void);
/* ----------------------------- Promptmenu close ----------------------------- */
void promptmenu_close(void);
/* ----------------------------- Promptmenu active ----------------------------- */
bool promptmenu_active(void);
/* ----------------------------- Promptmenu yn mode ----------------------------- */
bool promptmenu_yn_mode(void);
/* ----------------------------- Promptmenu refresh text ----------------------------- */
void promptmenu_refresh_text(void);
/* ----------------------------- Promptmenu completions search ----------------------------- */
void promptmenu_completions_search(void);
/* ----------------------------- Promptmenu enter action ----------------------------- */
void promptmenu_action_enter(void);
/* ----------------------------- Promptmenu tab action ----------------------------- */
void promptmenu_action_tab(void);
/* ----------------------------- Promptmenu action yes ----------------------------- */
void promptmenu_routine_yes(void);
/* ----------------------------- Promptmenu action no ----------------------------- */
void promptmenu_routine_no(void);
/* ----------------------------- Promptmenu ask ----------------------------- */
void promptmenu_ask(PromptMenuType type);


/* ---------------------------------------------------------- files.c ---------------------------------------------------------- */


/* static */ char *do_lockfile(const char *const restrict filename, bool ask_the_user);
/* static */ char **filename_completion(const char *const restrict morsel, Ulong *const num_matches) ;
/* static */ bool make_backup_of_for(openfilestruct *const file, char *realname);
/* static */ bool make_backup_of(char *realname);

/* static */ void cancel_the_command(int _UNUSED signal);
/* static */ void send_data(const linestruct *head, int fd);

/* static */ void execute_command_for(CTX_ARGS_REF_OF, const char *const restrict command);
/* static */ void execute_command(const char *const restrict command);

/* static */ void insert_a_file_or_for(FULL_CTX_ARGS, bool execute);
/* static */ void insert_a_file_or(bool execute);


/* ----------------------------- Make new buffer ----------------------------- */
void make_new_buffer_for(openfilestruct **const start, openfilestruct **const open);
void make_new_buffer(void);
/* ----------------------------- Crop to fit ----------------------------- */
char *crop_to_fit(const char *const restrict name, Ulong room);
/* ----------------------------- Stat with alloc ----------------------------- */
void stat_with_alloc(const char *filename, struct stat **pstat);
/* ----------------------------- Prepare for display ----------------------------- */
void prepare_for_display_for(openfilestruct *const file);
void prepare_for_display(void);
/* ----------------------------- Mention name and linecount ----------------------------- */
void mention_name_and_linecount_for(openfilestruct *const file);
void mention_name_and_linecount(void);
/* ----------------------------- Delete lockfile ----------------------------- */
bool delete_lockfile(const char *const restrict lockfilename) _NONNULL(1);
/* ----------------------------- Write lockfile ----------------------------- */
bool write_lockfile(const char *const restrict lockfilename, const char *const restrict filename, bool modified);
/* ----------------------------- Has valid path ----------------------------- */
bool has_valid_path(const char *const restrict filename);
void  free_one_buffer(openfilestruct *orphan, openfilestruct **open, openfilestruct **start);
/* ----------------------------- Close buffer ----------------------------- */
void close_buffer_for(openfilestruct *const orphan, openfilestruct **const start, openfilestruct **const open);
void close_buffer(void);
/* ----------------------------- Real dir from tilde ----------------------------- */
char *real_dir_from_tilde(const char *const restrict path) _RETURNS_NONNULL _NONNULL(1);
/* ----------------------------- Is dir ----------------------------- */
bool is_dir(const char *const path) _NODISCARD _NONNULL(1);
/* ----------------------------- Get full path ----------------------------- */
char *get_full_path(const char *const restrict origpath);
/* ----------------------------- Check writable directory ----------------------------- */
char *check_writable_directory(const char *path);
/* ----------------------------- Diralphasort ----------------------------- */
int diralphasort(const void *va, const void *vb);
/* ----------------------------- Set modified ----------------------------- */
void set_modified_for(openfilestruct *const file);
void set_modified(void);
/* ----------------------------- Encode data ----------------------------- */
char *encode_data(char *text, Ulong length);
/* ----------------------------- Init operating dir ----------------------------- */
void init_operating_dir(void);
/* ----------------------------- Outside of confinement ----------------------------- */
bool outside_of_confinement(const char *const restrict somepath, bool tabbing);
/* ----------------------------- Init backup dir ----------------------------- */
void init_backup_dir(void);
/* ----------------------------- Copy file ----------------------------- */
int copy_file(FILE *inn, FILE *out, bool close_out);
/* ----------------------------- Safe tempfile ----------------------------- */
char *safe_tempfile_for(openfilestruct *const file, FILE **const stream);
char *safe_tempfile(FILE **const stream);
/* ----------------------------- Redecorate after switch ----------------------------- */
void redecorate_after_switch_for(openfilestruct *const file, int cols);
void redecorate_after_switch(void);
/* ----------------------------- Switch to prev buffer ----------------------------- */
void switch_to_prev_buffer_for(openfilestruct **const open, int cols);
void switch_to_prev_buffer(void);
/* ----------------------------- Switch to next buffer ----------------------------- */
void switch_to_next_buffer_for(openfilestruct **const open, int cols);
void switch_to_next_buffer(void);
/* ----------------------------- Get next filename ----------------------------- */
char *get_next_filename(const char *const restrict name, const char *const restrict suffix);
/* ----------------------------- Open file ----------------------------- */
int open_file(const char *const restrict path, bool new_one, FILE **const f);
/* ----------------------------- Read file ----------------------------- */
void read_file_into(CTX_ARGS, FILE *const f, int fd, const char *const restrict filename, bool undoable);
void read_file(FILE *f, int fd, const char *const restrict filename, bool undoable);
/* ----------------------------- Open buffer ----------------------------- */
bool open_buffer_for(openfilestruct **const start, openfilestruct **const open, int rows, int cols, const char *const restrict path, bool new_one);
bool open_buffer(const char *const restrict path, bool new_one);
/* ----------------------------- Open buffer browser ----------------------------- */
void open_buffer_browser_for(FULL_CTX_ARGS);
void open_buffer_browser(void);
/* ----------------------------- Open new empty buffer ----------------------------- */
void open_new_empty_buffer_for(openfilestruct **const start, openfilestruct **const open);
void open_new_empty_buffer(void);
/* ----------------------------- Username completion ----------------------------- */
char **username_completion(const char *const restrict morsel, Ulong length, Ulong *const num_matches);
/* ----------------------------- Input tab ----------------------------- */
char *input_tab(char *morsel, Ulong *const place, functionptrtype refresh_func, bool *const listed);
/* ----------------------------- Write file ----------------------------- */
bool write_file_for(openfilestruct *const file, const char *const restrict name,
  FILE *thefile, bool normal, kind_of_writing_type method, bool annotate);
bool write_file(const char *const restrict name, FILE *thefile, bool normal, kind_of_writing_type method, bool annotate);
/* ----------------------------- Write region to file ----------------------------- */
bool write_region_to_file_for(openfilestruct *const file, const char *const restrict name, FILE *stream, bool normal, kind_of_writing_type method);
bool write_region_to_file(const char *const restrict name, FILE *stream, bool normal, kind_of_writing_type method);
/* ----------------------------- Write it out ----------------------------- */
int write_it_out_for(openfilestruct *const file, bool exiting, bool withprompt);
int write_it_out(bool exiting, bool withprompt);
/* ----------------------------- Do writeout ----------------------------- */
void do_writeout_for(openfilestruct **const start, openfilestruct **const open, int cols);
void do_writeout(void);
/* ----------------------------- Do savefile ----------------------------- */
void do_savefile_for(openfilestruct **const start, openfilestruct **const open, int cols);
void do_savefile(void);
/* ----------------------------- Do insertfile ----------------------------- */
void do_insertfile_for(FULL_CTX_ARGS);
void do_insertfile(void);
/* ----------------------------- Do execute ----------------------------- */
void do_execute_for(FULL_CTX_ARGS);
void do_execute(void);
/* ----------------------------- Norm path ----------------------------- */
char *norm_path(const char *const restrict path);
void norm_path_test_a_path(const char *const restrict path);
void norm_path_test(void) _NO_RETURN;
/* ----------------------------- Abs path ----------------------------- */
char *abs_path(const char *const restrict path);


/* ---------------------------------------------------------- chars.c ---------------------------------------------------------- */


/* ----------------------------- Utf8 init ----------------------------- */
void utf8_init(void);
/* ----------------------------- Using utf8 ----------------------------- */
bool using_utf8(void);
bool is_language_word_char(const char *pointer, Ulong index);
bool is_lang_word_char(openfilestruct *const file);
bool is_cursor_language_word_char(void);
bool is_enclose_char(char ch);
/* ----------------------------- Is alpha char ----------------------------- */
bool is_alpha_char(const char *const restrict c);
/* ----------------------------- Is alnum char ----------------------------- */
bool is_alnum_char(const char *const restrict c) _NODISCARD;
/* ----------------------------- Is blank char ----------------------------- */
bool is_blank_char(const char *const restrict c) _NODISCARD;
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
bool is_prev_cursor_char_one_of_for(openfilestruct *const file, const char *chars);
bool is_prev_cursor_char_one_of(const char *chars);
bool is_cursor_char(const char ch);
bool is_char_one_of(const char *pointer, Ulong index, const char *chars);
bool is_end_char_one_of(const char *const restrict ptr, const char *const restrict chars);
bool is_cursor_char_one_of(const char *chars);
bool is_between_chars(const char *pointer, Ulong index, const char pre_ch, const char post_ch);
bool is_curs_between_chars_for(openfilestruct *const restrict file, char a, char b);
bool is_curs_between_chars(char a, char b);
bool is_between_any_char_pair(const char *const restrict ptr, Ulong index, const char **const restrict pairs, Ulong *const restrict out_index);
bool is_curs_between_any_pair_for(openfilestruct *const restrict file, const char **const restrict pairs, Ulong *const restrict out_index);
bool is_curs_between_any_pair(const char **const restrict pairs, Ulong *const restrict out_index);
bool is_cursor_between_chars(const char pre_ch, const char post_ch);
char control_mbrep(const char *const c, bool isdata);
/* ----------------------------- Mbtowide ----------------------------- */
int mbtowide(wchar *const restrict wc, const char *const restrict c) __THROW _NODISCARD _NONNULL(1, 2);
/* ----------------------------- Widetomb ----------------------------- */
int widetomb(Uint wc, char *const restrict mb);
/* ----------------------------- Is doublewidth ----------------------------- */
bool is_doublewidth(const char *const ch);
/* ----------------------------- Is zerowidth ----------------------------- */
bool is_zerowidth(const char *ch);
bool is_cursor_zerowidth(void);
/* ----------------------------- Char length ----------------------------- */
int char_length(const char *const pointer);
/* ----------------------------- Mbstrlen ----------------------------- */
Ulong mbstrlen(const char *pointer);
/* ----------------------------- Collect char ----------------------------- */
int collect_char(const char *const str, char *c);
/* ----------------------------- Advance over ----------------------------- */
int advance_over(const char *const str, Ulong *column);
/* ----------------------------- Step left ----------------------------- */
Ulong step_left(const char *const buf, const Ulong pos);
/* ----------------------------- Step cursor left ----------------------------- */
void step_cursor_left(openfilestruct *const file);
/* ----------------------------- Step right ----------------------------- */
Ulong step_right(const char *const buf, const Ulong pos);
/* ----------------------------- Step cursor right ----------------------------- */
void step_cursor_right(openfilestruct *const file);
/* ----------------------------- Mbstrcasecmp ----------------------------- */
int mbstrcasecmp(const char *s1, const char *s2);
/* ----------------------------- Mbstrncasecmp ----------------------------- */
int mbstrncasecmp(const char *s1, const char *s2, Ulong n);
/* ----------------------------- Mbstrcasestr ----------------------------- */
char *mbstrcasestr(const char *haystack, const char *const needle);
/* ----------------------------- Revstrstr ----------------------------- */
char *revstrstr(const char *const haystack, const char *const needle, const char *pointer) __THROW _NODISCARD _NONNULL(1, 2, 3);
/* ----------------------------- Mbrevstrcasestr ----------------------------- */
char *mbrevstrcasestr(const char *const haystack, const char *const needle, const char *pointer) _NODISCARD;
/* ----------------------------- Mbstrchr ----------------------------- */
char *mbstrchr(const char *string, const char *const chr);
/* ----------------------------- Mbstrpbrk ----------------------------- */
char *mbstrpbrk(const char *str, const char *accept);
/* ----------------------------- Mbrevstrpbrk ----------------------------- */
char *mbrevstrpbrk(const char *const head, const char *const accept, const char *pointer);
/* ----------------------------- Has blank char ----------------------------- */
bool has_blank_char(const char *restrict str);
/* ----------------------------- White string ----------------------------- */
bool white_string(const char *restrict str);
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
bool  less_than_a_screenful_for(CTX_ARGS, Ulong was_lineno, Ulong was_leftedge);
bool  less_than_a_screenful(Ulong was_lineno, Ulong was_leftedge);
Ulong actual_last_column_for(openfilestruct *const file, int cols, Ulong leftedge, Ulong column);
Ulong actual_last_column(Ulong leftedge, Ulong column);
bool  current_is_above_screen_for(openfilestruct *const file);
bool  current_is_above_screen(void);
bool  current_is_below_screen_for(openfilestruct *const file, int total_rows, int total_cols);
bool  current_is_below_screen(void);
bool  current_is_offscreen_for(CTX_ARGS);
bool  current_is_offscreen(void);
void  adjust_viewport_for(CTX_ARGS, update_type manner);
void  adjust_viewport(update_type manner);
void  place_the_cursor_for(openfilestruct *const file);
void  place_the_cursor(void);
void  set_blankdelay_to_one(void);
Ulong waiting_keycodes(void);
void  edit_scroll_for(openfilestruct *const file, bool direction);
void  edit_scroll(bool direction);
void  edit_redraw_for(CTX_ARGS, linestruct *const old_current, update_type manner);
void  edit_redraw(linestruct *const old_current, update_type manner);
void  edit_refresh_for(CTX_ARGS);
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
void  statusline(message_type type, const char *const restrict format, ...) _PRINTFLIKE(2, 3);
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
void do_credits(void);
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
/* ----------------------------- Interpret ----------------------------- */
functionptrtype interpret(int keycode);


/* ---------------------------------------------------------- search.c ---------------------------------------------------------- */


/* ----------------------------- Regular expression init ----------------------------- */
bool regexp_init(const char *regexp);
/* ----------------------------- Tidy up after search ----------------------------- */
void tidy_up_after_search_for(openfilestruct *const file);
void tidy_up_after_search(void);
/* ----------------------------- Goto line posx ----------------------------- */
void goto_line_posx_for(openfilestruct *const file, int rows, long lineno, Ulong x);
void goto_line_posx(long lineno, Ulong x);
/* ----------------------------- Not found msg ----------------------------- */
void not_found_msg(const char *const restrict str);
/* ----------------------------- Find next str ----------------------------- */
int findnextstr_for(openfilestruct *const file, const char *const restrict needle, bool whole_word_only,
  int modus, Ulong *const match_len, bool skipone, const linestruct *const begin, Ulong begin_x);
int findnextstr(const char *const restrict needle, bool whole_word_only, int modus,
  Ulong *const match_len, bool skipone, const linestruct *const begin, Ulong begin_x);
/* ----------------------------- Go looking ----------------------------- */
void go_looking_for(CTX_ARGS);
void go_looking(void);
/* ----------------------------- Do findprevious ----------------------------- */
void do_findprevious_for(CTX_ARGS);
void do_findprevious(void);
/* ----------------------------- Do findnext ----------------------------- */
void do_findnext_for(CTX_ARGS);
void do_findnext(void);
/* ----------------------------- Do replace loop ----------------------------- */
long do_replace_loop_for(CTX_ARGS, const char *const restrict needle,
  bool whole_word_only, const linestruct *const real_current, Ulong *const real_current_x);
long do_replace_loop(const char *const restrict needle,
  bool whole_word_only, const linestruct *const real_current, Ulong *const real_current_x);
/* ----------------------------- Ask for and do replacement ----------------------------- */  
void ask_for_and_do_replacements_for(CTX_ARGS);
void ask_for_and_do_replacements(void);
/* ----------------------------- Goto line and column ----------------------------- */
void goto_line_and_column_for(CTX_ARGS, long line, long column, bool retain_answer, bool interactive);
void goto_line_and_column(long line, long column, bool retain_answer, bool interactive);
/* ----------------------------- Do gotolinecolumn ----------------------------- */
void do_gotolinecolumn_for(CTX_ARGS);
void do_gotolinecolumn(void);
/* ----------------------------- Do search forward ----------------------------- */
void do_search_forward_for(CTX_ARGS);
void do_search_forward(void);
/* ----------------------------- Do search backward ----------------------------- */
void do_search_backward_for(CTX_ARGS);
void do_search_backward(void);
/* ----------------------------- Do replace ----------------------------- */
void do_replace_for(CTX_ARGS);
void do_replace(void);
/* ----------------------------- Put or lift anchor ----------------------------- */
void put_or_lift_anchor_for(openfilestruct *const file);
void put_or_lift_anchor(void);
/* ----------------------------- To prev anchor ----------------------------- */
void to_prev_anchor_for(CTX_ARGS);
void to_prev_anchor(void);
/* ----------------------------- To next anchor ----------------------------- */
void to_next_anchor_for(CTX_ARGS);
void to_next_anchor(void);


/* ---------------------------------------------------------- move.c ---------------------------------------------------------- */


void to_first_line_for(openfilestruct *const file);
void to_first_line(void);
void to_last_line_for(openfilestruct *const file, int rows);
void to_last_line(void);
void get_edge_and_target_for(openfilestruct *const file, int cols, Ulong *const leftedge, Ulong *const target_column);
void get_edge_and_target(Ulong *const leftedge, Ulong *target_column);
void do_page_up_for(CTX_ARGS);
void do_page_up(void);
void do_page_down_for(openfilestruct *const file, int total_rows, int total_cols);
void do_page_down(void);
void to_top_row_for(openfilestruct *const file, int total_cols);
void to_top_row(void);
void to_bottom_row_for(openfilestruct *const file, int total_rows, int total_cols);
void to_bottom_row(void);
void do_cycle_for(CTX_ARGS);
void do_cycle(void);
void do_center(void);
void do_para_begin(linestruct **const line);
void do_para_end(linestruct **const line);
void to_para_begin_for(CTX_ARGS);
void to_para_begin(void);
void to_para_end_for(CTX_ARGS);
void to_para_end(void);
void to_prev_block_for(CTX_ARGS);
void to_prev_block(void);
void to_next_block_for(CTX_ARGS);
void to_next_block(void);
void do_up_for(CTX_ARGS);
void do_up(void);
void do_down_for(CTX_ARGS);
void do_down(void);
void do_left_for(CTX_ARGS);
void do_left(void);
void do_right_for(CTX_ARGS);
void do_right(void);
void do_prev_word_for(openfilestruct *const file, bool allow_punct);
void do_prev_word(void);
void to_prev_word_for(CTX_ARGS, bool allow_punct);
void to_prev_word(void);
bool do_next_word_for(openfilestruct *const file, bool after_ends, bool allow_punct);
bool do_next_word(bool after_ends);
void to_next_word_for(CTX_ARGS, bool after_ends, bool allow_punct);
void to_next_word(void);
void do_home_for(CTX_ARGS);
void do_home(void);
void do_end_for(CTX_ARGS);
void do_end(void);
void do_scroll_up_for(CTX_ARGS);
void do_scroll_up(void);
void do_scroll_down_for(CTX_ARGS);
void do_scroll_down(void);


/* ---------------------------------------------------------- rcfile.c ---------------------------------------------------------- */


/* static */ bool is_universal(void (*f)(void));

/* ----------------------------- Display rcfile errors ----------------------------- */
void display_rcfile_errors(void);
/* ----------------------------- Jot error ----------------------------- */
void jot_error(const char *const restrict format, ...) _PRINTFLIKE(1, 2);
/* ----------------------------- Check for nonempty syntax ----------------------------- */
void check_for_nonempty_syntax(void);
/* ----------------------------- Set interface color ----------------------------- */
void set_interface_color(int element, char *combotext);
/* ----------------------------- Parse argument ----------------------------- */
char *parse_argument(char *ptr);
/* ----------------------------- Parse next word ----------------------------- */
char *parse_next_word(char *ptr);
/* ----------------------------- Compile ----------------------------- */
bool compile(const char *const restrict expression, int rex_flags, regex_t **const packed);
/* ----------------------------- Begin new syntax ----------------------------- */
void begin_new_syntax(char *ptr);
/* ----------------------------- Closest index color ----------------------------- */
short closest_index_color(short red, short green, short blue);
/* ----------------------------- Color to short ----------------------------- */
short color_to_short(const char *colorname, bool *vivid, bool *thick);
/* ----------------------------- Parse combination ----------------------------- */
bool parse_combination(char *combotext, short *fg, short *bg, int *attributes);
/* ----------------------------- Grab and store ----------------------------- */
void grab_and_store(const char *const restrict kind, char *ptr, regexlisttype **const storage);
/* ----------------------------- Parse syntax commands ----------------------------- */
bool parse_syntax_commands(const char *keyword, char *ptr);
/* ----------------------------- Parse rule ----------------------------- */
void parse_rule(char *ptr, int rex_flags);
/* ----------------------------- Is good file ----------------------------- */
bool is_good_file(const char *const restrict file);
/* ----------------------------- Check vitals mapped ----------------------------- */
void check_vitals_mapped(void);
/* ----------------------------- Strtosc ----------------------------- */
keystruct *strtosc(const char *const restrict input);
/* ----------------------------- Parse one include ----------------------------- */
void parse_one_include(char *file, syntaxtype *syntax);
/* ----------------------------- Parse binding ----------------------------- */
void parse_binding(char *ptr, bool dobind);
/* ----------------------------- Parse rcfile ----------------------------- */
void parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only);
/* ----------------------------- Do rcfile ----------------------------- */
void do_rcfiles(void);

// void parse_binding(char *ptr, bool dobind);


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

/* ----------------------------- Lop the answer ----------------------------- */
void lop_the_answer(void);
/* ----------------------------- Copy the answer ----------------------------- */
void copy_the_answer(void);
/* ----------------------------- Paste into answer ----------------------------- */
void paste_into_answer(void);
/* ----------------------------- Absorb character ----------------------------- */
void absorb_character(int input, functionptrtype function);
/* ----------------------------- Statusbar discard all undo redo ----------------------------- */
void statusbar_discard_all_undo_redo(void);
/* ----------------------------- Do statusbar undo ----------------------------- */
void do_statusbar_undo(void);
/* ----------------------------- Do statusbar redo ----------------------------- */
void do_statusbar_redo(void);
/* ----------------------------- Do statusbar home ----------------------------- */
void do_statusbar_home(void);
/* ----------------------------- Do statusbar end ----------------------------- */
void do_statusbar_end(void);
/* ----------------------------- Do statusbar prev word ----------------------------- */
void do_statusbar_prev_word(void);
/* ----------------------------- Do statusbar next word ----------------------------- */
void do_statusbar_next_word(void);
/* ----------------------------- Do statusbar left ----------------------------- */
void do_statusbar_left(void);
/* ----------------------------- Do statusbar right ----------------------------- */
void do_statusbar_right(void);
/* ----------------------------- Do statusbar backspace ----------------------------- */
void do_statusbar_backspace(bool with_undo);
/* ----------------------------- Do statusbar delete ----------------------------- */
void do_statusbar_delete(void);
/* ----------------------------- Inject into answer ----------------------------- */
void inject_into_answer(char *burst, Ulong count);
/* ----------------------------- Do statusbar chop next word ----------------------------- */
void do_statusbar_chop_next_word(void);
/* ----------------------------- Do statusbar chop prev word ----------------------------- */
void do_statusbar_chop_prev_word(void);
/* ----------------------------- Do statusbar replace ----------------------------- */
void do_statusbar_replace(const char *const restrict data);
/* ----------------------------- Get statusbar page start ----------------------------- */
Ulong get_statusbar_page_start(Ulong base, Ulong column);
/* ----------------------------- Put cursor at the end of answer ----------------------------- */
void put_cursor_at_end_of_answer(void);
/* ----------------------------- Add or remove pipe symbol from answer ----------------------------- */
void add_or_remove_pipe_symbol_from_answer(void);
/* ----------------------------- Draw the promptbar ----------------------------- */
void draw_the_promptbar(void);
/* ----------------------------- Ask user ----------------------------- */
int ask_user(bool withall, const char *const restrict question);
/* ----------------------------- Do prompt ----------------------------- */
int do_prompt(int menu, const char *const provided, linestruct **const histlist,
  functionptrtype refresh_func, const char *const format, ...) _PRINTFLIKE(5, 6);


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


/* ----------------------------- Browser refresh ----------------------------- */
void browser_refresh(void);
/* ----------------------------- To first file ----------------------------- */
void to_first_file(void);
/* ----------------------------- To last file ----------------------------- */
void to_last_file(void);
/* ----------------------------- Browse in ----------------------------- */
char *browse_in(const char *const restrict inpath);


/* ---------------------------------------------------------- help.c ---------------------------------------------------------- */


/* ----------------------------- Wrap help text into buffer ----------------------------- */
void wrap_help_text_into_buffer_for(openfilestruct **const start, openfilestruct **const open, int rows, int cols);
void wrap_help_text_into_buffer(void);
/* ----------------------------- Do help ----------------------------- */
void do_help_for(FULL_CTX_ARGS);
void do_help(void);


/* ---------------------------------------------------------- cut.c ---------------------------------------------------------- */


void expunge_for(openfilestruct *const file, int cols, undo_type action);
void expunge(undo_type action);
void extract_segment_for(CTX_ARGS, linestruct *const top, Ulong top_x, linestruct *const bot, Ulong bot_x);
void extract_segment(linestruct *const top, Ulong top_x, linestruct *const bot, Ulong bot_x);
void cut_marked_region_for(CTX_ARGS);
void cut_marked_region(void);
void do_snip_for(CTX_ARGS, bool marked, bool until_eof, bool append);
void do_snip(bool marked, bool until_eof, bool append);
void cut_text_for(CTX_ARGS);
void cut_text(void);
void cut_till_eof_for(CTX_ARGS);
void cut_till_eof(void);
void zap_text_for(CTX_ARGS);
void zap_text(void);
void do_delete_for(CTX_ARGS);
void do_delete(void);
void ingraft_buffer_into(openfilestruct *const file, linestruct *top, linestruct *bot);
void ingraft_buffer(linestruct *topline);
void do_backspace_for(CTX_ARGS);
void do_backspace(void);
void copy_from_buffer_for(openfilestruct *const file, int rows, linestruct *const head);
void copy_from_buffer(linestruct *const head);
void copy_marked_region_for(openfilestruct *const file);
void copy_marked_region(void);
void copy_text_for(CTX_ARGS);
void copy_text(void);
void paste_text_for(CTX_ARGS);
void paste_text(void);
void zap_replace_text_for(CTX_ARGS, const char *const restrict replace_with, Ulong len);
void zap_replace_text(const char *const restrict replace_with, Ulong len);
void chop_previous_word_for(CTX_ARGS);
void chop_previous_word(void);
void chop_next_word_for(CTX_ARGS);
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
void editor_set_rows_cols(Editor *const editor, float width, float height);
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


void statusbar_init(void);
void statusbar_free(void);
void statusline_gui_timed(message_type type, double seconds, const char *format, ...);
void statusline_gui_va(message_type type, const char *const restrict format, va_list ap);
void statusline_gui(message_type type, const char *format, ...) _PRINTFLIKE(2, 3);
void statusbar_gui(const char *const restrict msg);
void statusbar_count_frame(void);
void statusbar_draw(void);


/* ---------------------------------------------------------- gui/mouse.c ---------------------------------------------------------- */


/* ----------------------------- Mouse gui init ----------------------------- */
void mouse_gui_init(void);
/* ----------------------------- Mouse gui free ----------------------------- */
void mouse_gui_free(void);
/* ----------------------------- Update mouse state ----------------------------- */
void mouse_gui_update_state(bool press, int button);
/* ----------------------------- Update mouse pos ----------------------------- */
void mouse_gui_update_pos(float x, float y);
/* ----------------------------- Mouse gui get x ----------------------------- */
float mouse_gui_get_x(void);
/* ----------------------------- Mouse gui get y ----------------------------- */
float mouse_gui_get_y(void);
/* ----------------------------- Mouse gui get pos ----------------------------- */
void mouse_gui_get_pos(float *const x, float *const y);
float mouse_gui_get_last_x(void);
float mouse_gui_get_last_y(void);
/* ----------------------------- Is mouse flag set ----------------------------- */
bool mouse_gui_is_flag_set(Uint flag);
void mouse_gui_clear_flags(void);
/* ----------------------------- Mouse gui button down ----------------------------- */
void mouse_gui_button_down(Uchar button, Ushort mod, float x, float y);
/* ----------------------------- Mouse gui button up ----------------------------- */
void mouse_gui_button_up(Uchar button, Ushort _UNUSED mod, float x, float y);
/* ----------------------------- Mouse gui position ----------------------------- */
void mouse_gui_position(float x, float y);
/* ----------------------------- Mouse gui left ----------------------------- */
void mouse_gui_left(void);
/* ----------------------------- Mouse gui scroll ----------------------------- */
void mouse_gui_scroll(float mx, float my, int _UNUSED ix, int iy, SDL_MouseWheelDirection type);


/* ---------------------------------------------------------- gui/shader.c ---------------------------------------------------------- */


/* ----------------------------- Shader compile ----------------------------- */
void shader_compile(void);
/* ----------------------------- Shader free ----------------------------- */
void shader_free(void);
/* ----------------------------- Shader rect vertex load ----------------------------- */
void shader_rect_vertex_load(RectVertex *buf, float x, float y, float w, float h, Uint color);
/* ----------------------------- Shader rect vertex load array ----------------------------- */
void shader_rect_vertex_load_array(RectVertex *buf, float *const array, Uint color);
/* ----------------------------- Shader set projection ----------------------------- */
void shader_set_projection(float left, float right, float top, float bot, float znear, float zfar);
/* ----------------------------- Shader upload projection ----------------------------- */
void shader_upload_projection(void);
/* ----------------------------- Shader get location font tex ----------------------------- */
int shader_get_location_font_tex(void);
/* ----------------------------- Shader get location font projection ----------------------------- */
int shader_get_location_font_projection(void);
/* ----------------------------- Shader get location rect projection ----------------------------- */
int shader_get_location_rect_projection(void);


/* ----------------------------------------------------------  ---------------------------------------------------------- */


/* ----------------------------- Gl window resize needed ----------------------------- */
bool gl_window_resize_needed(void);
/* ----------------------------- Gl window init ----------------------------- */
void gl_window_init(void);
/* ----------------------------- Gl window free ----------------------------- */
void gl_window_free(void);
/* ----------------------------- Gl window ----------------------------- */
GLFWwindow *gl_window(void);
/* ----------------------------- Gl window root ----------------------------- */
Element *gl_window_root(void);
/* ----------------------------- Gl window width ----------------------------- */
int gl_window_width(void);
/* ----------------------------- Gl window height ----------------------------- */
int gl_window_height(void);
/* ----------------------------- Gl window add root child ----------------------------- */
void gl_window_add_root_child(Element *const e);
void gl_window_borderless_fullscreen(void);
void gl_window_poll_events(void);
void gl_window_swap(void);
bool gl_window_running(void);
bool gl_window_quit(void);
/* ----------------------------- Gl window should quit ----------------------------- */
void gl_window_should_quit(void);


/* ---------------------------------------------------------- gui/keyboard.c ---------------------------------------------------------- */


/* ----------------------------- Kb key pressed ----------------------------- */
void kb_key_pressed(Uint key, Uint scan, Ushort mod, bool repeat);
/* ----------------------------- Kb char input ----------------------------- */
void kb_char_input(const char *const restrict data, Ushort mod);
/* ----------------------------- Kb prompt key pressed ----------------------------- */
void kb_prompt_key_pressed(Uint key, Uint _UNUSED scan, Ushort mod, bool _UNUSED repeat);
/* ----------------------------- Kb prompt char input ----------------------------- */
void kb_prompt_char_input(const char *const restrict data, Ushort mod);


/* ---------------------------------------------------------- nanox.c ---------------------------------------------------------- */


/* static */ int get_keycode(const char *const restrict keyname, int standard);

/* static */ void mouse_init(void);

/* static */ void emergency_save_for(openfilestruct *const file, const char *const restrict name);
/* static */ void emergency_save(const char *const restrict name);

/* static */ void print_opt(const char *const restrict sflag, const char *const restrict lflag, const char *const restrict description);

/* static */ void signal_init(void);

/* static */ void suck_up_input_and_paste_it(void);

/* ----------------------------- Make new node ----------------------------- */
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
/* ----------------------------- Copy node ----------------------------- */
linestruct *copy_node(const linestruct *const src) _NODISCARD _RETURNS_NONNULL _NONNULL(1);
/* ----------------------------- Copy buffer top bot ----------------------------- */
void copy_buffer_top_bot(const linestruct *src, linestruct **const top, linestruct **const bot);
/* ----------------------------- Copy buffer ----------------------------- */
linestruct *copy_buffer(const linestruct *src);
/* ----------------------------- Renumber from ----------------------------- */
void renumber_from(linestruct *line);
/* ----------------------------- Print view warning ----------------------------- */
void print_view_warning(void);
/* ----------------------------- In restricted mode ----------------------------- */
bool in_restricted_mode(void);
/* ----------------------------- Disable flow control ----------------------------- */
void disable_flow_control(void);
/* ----------------------------- Enable flow control ----------------------------- */
void enable_flow_control(void);
/* ----------------------------- Disable extended io ----------------------------- */
void disable_extended_io(void);
/* ----------------------------- Confirm margin ----------------------------- */
void confirm_margin_for(openfilestruct *const file, int *const cols);
void confirm_margin(void);
/* ----------------------------- Disable kb interrupt ----------------------------- */
void disable_kb_interrupt(void);
/* ----------------------------- Enable kb interrupt ----------------------------- */
void enable_kb_interrupt(void);
/* ----------------------------- Install handler for Ctrl C ----------------------------- */
void install_handler_for_Ctrl_C(void);
/* ----------------------------- Restore handler for Ctrl C ----------------------------- */
void restore_handler_for_Ctrl_C(void);
/* ----------------------------- Terminal init ----------------------------- */
void terminal_init(void);
/* ----------------------------- Window init ----------------------------- */
void window_init(void);
/* ----------------------------- Regenerate screen ----------------------------- */
void regenerate_screen(void);
/* ----------------------------- Block sigwinch ----------------------------- */
void block_sigwinch(bool blockit);
/* ----------------------------- Handle sigwinch ----------------------------- */
void handle_sigwinch(int signal);
/* ----------------------------- Suspend nano ----------------------------- */
void suspend_nano(int _UNUSED signal);
/* ----------------------------- Continue nano ----------------------------- */
void continue_nano(int _UNUSED signal);
/* ----------------------------- Do suspend ----------------------------- */
void do_suspend(void);
/* ----------------------------- Reconnect and store state ----------------------------- */
void reconnect_and_store_state(void);
/* ----------------------------- Handle hupterm ----------------------------- */
void handle_hupterm(int _UNUSED signal) _NO_RETURN;
/* ----------------------------- Handle crash ----------------------------- */
void handle_crash(int _UNUSED_IN_DEBUG signal) _NO_RETURN;
/* ----------------------------- Inject ----------------------------- */
void inject_into_buffer(CTX_ARGS, char *burst, Ulong count);
void inject(char *burst, Ulong count);
/* ----------------------------- Unbound key ----------------------------- */
void unbound_key(int code);
/* ----------------------------- Close and go ----------------------------- */
void close_and_go_for(openfilestruct **const start, openfilestruct **const open, int cols);
void close_and_go(void);
/* ----------------------------- Die ----------------------------- */
void die(const char *const restrict format, ...) _NO_RETURN;
/* ----------------------------- Version ----------------------------- */
void version(void) _NO_RETURN;
/* ----------------------------- Usage ----------------------------- */
void usage(void) _NO_RETURN;
/* ----------------------------- Name to menu ----------------------------- */
Uint name_to_menu(const char *const restrict name);
/* ----------------------------- Menu to name ----------------------------- */
const char *menu_to_name(Uint menu);


/* ---------------------------------------------------------- Defined in C++ ---------------------------------------------------------- */


void render_line_text(int row, const char *str, linestruct *line, Ulong from_col) __THROW;
void apply_syntax_to_line(const int row, const char *converted, linestruct *line, Ulong from_col);
// keystruct *strtosc(const char *input);
void finish(void) __THROW _NO_RETURN;
void syntax_check_file(openfilestruct *file);

bool wanted_to_move(functionptrtype f);
bool changes_something(functionptrtype f);
void do_exit(void);


_END_C_LINKAGE


#endif /* _C_PROTO__H */
