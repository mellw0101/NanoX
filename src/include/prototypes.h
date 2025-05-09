/** @file prototypes.h */
#pragma once

#include <Mlib/Attributes.h>
#include "color.h"
#include "language_server/language_server.h"
#include "render.h"
#include "rendr/rendr.h"
#include "task_types.h"
#include "definitions.h"

/* All external variables.  See global.c for their descriptions. */

extern volatile sig_atomic_t the_window_resized;

extern WINDOW *topwin;
extern WINDOW *midwin;
extern WINDOW *footwin;
extern WINDOW *suggestwin;

extern nwindow *tui_topwin;
extern nwindow *tui_midwin;
extern nwindow *tui_footwin;

extern syntaxtype *syntaxes;

extern openfilestruct *openfile;
extern openfilestruct *startfile;

extern message_type lastmessage;

extern linestruct *search_history;
extern linestruct *replace_history;
extern linestruct *execute_history;
extern linestruct *searchtop;
extern linestruct *searchbot;
extern linestruct *replacetop;
extern linestruct *replacebot;
extern linestruct *executetop;
extern linestruct *executebot;
extern linestruct *cutbuffer;
extern linestruct *cutbottom;
extern linestruct *pletion_line;

// extern unsigned flags[4];
extern Ulong flags[1];

extern long fill;

extern int didfind;
extern int controlleft, controlright;
extern int controlup, controldown;
extern int controlhome, controlend;
extern int controldelete, controlshiftdelete;
extern int shiftleft, shiftright;
extern int shiftup, shiftdown;
extern int shiftcontrolleft, shiftcontrolright;
extern int shiftcontrolup, shiftcontroldown;
extern int shiftcontrolhome, shiftcontrolend;
extern int altleft, altright;
extern int altup, altdown;
extern int althome, altend;
extern int altpageup, altpagedown;
extern int altinsert, altdelete;
extern int shiftaltleft, shiftaltright;
extern int shiftaltup, shiftaltdown;
extern int mousefocusin, mousefocusout;
extern int controlbsp;

extern int editwinrows;
extern int editwincols;
extern int margin;
extern int sidebar;
extern int cycling_aim;

extern int *bardata;
extern int  whitelen[2];

extern Ulong from_x;
extern Ulong till_x;

extern char *title;
extern char *answer;
extern char *last_search;
extern char *present_path;
extern char *matchbrackets;
extern char *whitespace;
extern char *punct;
extern char *brackets;
extern char *quotestr;
extern char *word_chars;
extern char *backup_dir;
extern char *operating_dir;
extern char *alt_speller;
extern char *syntaxstr;

extern const char *exit_tag;
extern const char *close_tag;
extern const char *term;
extern const char *term_program;

extern bool on_a_vt;
extern bool shifted_metas;
extern bool meta_key;
extern bool shift_held;
extern bool keep_mark;
extern bool mute_modifiers;
extern bool bracketed_paste;
extern bool we_are_running;
extern bool more_than_one;
extern bool report_size;
extern bool ran_a_tool;
extern bool inhelp;
extern bool focusing;
extern bool as_an_at;
extern bool control_C_was_pressed;
extern bool also_the_last;
extern bool keep_cutbuffer;
extern bool have_palette;
extern bool rescind_colors;
extern bool perturbed;
extern bool recook;
extern bool refresh_needed;
extern bool is_shorter;

extern bool  suggest_on;
extern char  suggest_buf[1024];
extern char *suggest_str;
extern int   suggest_len;

extern Ulong wrap_at;

extern long stripe_column;
extern long tabsize;

extern regex_t quotereg;

extern int         currmenu;
extern keystruct  *sclist;
extern funcstruct *allfuncs;
extern funcstruct *exitfunc;

extern regex_t    search_regexp;
extern regmatch_t regmatches[10];

extern int   hilite_attribute;
extern int   interface_color_pair[NUMBER_OF_ELEMENTS];
extern char *homedir;
extern char *statedir;
extern char *startup_problem;
extern char *custom_nanorc;
extern char *commandname;
extern bool  spotlighted;
extern Ulong light_from_col;
extern Ulong light_to_col;

extern bool last_key_was_bracket;
extern char last_bracket_char;

extern colortype *color_combo[NUMBER_OF_ELEMENTS];
extern keystruct *planted_shortcut;

extern task_queue_t          *task_queue;
extern pthread_t             *threads;
extern volatile sig_atomic_t *stop_thread_flags;
extern callback_queue_t      *callback_queue;
extern main_thread_t         *main_thread;

extern unordered_map<string, syntax_data_t> test_map;

extern file_listener_handler_t file_listener;

extern configstruct *config;

extern HashMap *test_hashmap;

/* Some prompt decl's. */
extern char *prompt;
extern Ulong typing_x;

extern SyntaxFile *sf;

typedef void (*functionptrtype)(void);

#ifdef HAVE_GLFW
  extern vec2 pen;
  extern guielement *file_menu_element;
  /* The bottom bar. */
  extern frametimerclass frametimer;
  extern vec2 mousepos;
  extern guieditor *openeditor;
  extern guieditor *starteditor;
  extern guistruct *gui;
  extern uigridmapclass gridmap;
  /* guiprompt.cpp */
  extern Ulong gui_prompt_mark_x;
  extern bool  gui_prompt_mark;
  extern int   gui_prompt_type;
#endif

/* Asm functions. */
ASM_FUNCTION(int)  SSE_strlen(const char *str);
ASM_FUNCTION(void) asm_atomic_add(int *ptr, int value);
ASM_FUNCTION(void) asm_atomic_sub(int *ptr, int value);
ASM_FUNCTION(int)  asm_atomic_xchg(int *ptr, int value);

/* The two needed functions from 'browser.cpp'. */
void  browser_refresh(void);
void  to_first_file(void) _NOTHROW;
void  to_last_file(void) _NOTHROW;
char *browse_in(const char *inpath);

/* Most functions in 'chars.cpp'. */
void  utf8_init(void) _NOTHROW;
bool  using_utf8(void) _NOTHROW _NODISCARD;
bool  is_language_word_char(const char *pointer, Ulong index) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_cursor_language_word_char(void) _NOTHROW _NODISCARD;
bool  is_enclose_char(const char ch) _NOTHROW _NODISCARD;
bool  is_alpha_char(const char *c) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_alnum_char(const char *const c) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_blank_char(const char *c) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_prev_blank_char(const char *pointer, Ulong index) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_prev_cursor_blank_char(void) _NOTHROW _NODISCARD;
bool  is_cursor_blank_char(void) _NOTHROW _NODISCARD;
bool  is_cntrl_char(const char *c) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_word_char(const char *c, bool allow_punct) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_cursor_word_char(bool allow_punct) _NOTHROW _NODISCARD;
bool  is_prev_word_char(const char *pointer, Ulong index, bool allow_punct = FALSE) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_prev_cursor_word_char(bool allow_punct = FALSE) _NOTHROW _NODISCARD;
bool  is_prev_char(const char *pointer, Ulong index, const char ch) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_prev_cursor_char(const char ch) _NOTHROW _NODISCARD;
bool  is_prev_char_one_of(const char *pointer, Ulong index, const char *chars) _NOTHROW _NODISCARD _NONNULL(1, 3);
bool  is_prev_cursor_char_one_of(const char *chars) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_cursor_char(const char ch) _NOTHROW _NODISCARD;
bool  is_char_one_of(const char *pointer, Ulong index, const char *chars) _NOTHROW _NODISCARD _NONNULL(1, 3);
bool  is_cursor_char_one_of(const char *chars) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_between_chars(const char *pointer, Ulong index, const char pre_ch, const char post_ch) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_cursor_between_chars(const char pre_ch, const char post_ch) _NOTHROW _NODISCARD;
char  control_mbrep(const char *c, bool isdata) _NOTHROW _NODISCARD _NONNULL(1);
int   mbtowide(wchar_t *wc, const char *c) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_doublewidth(const char *ch) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_zerowidth(const char *ch) _NOTHROW _NODISCARD _NONNULL(1);
bool  is_cursor_zerowidth(void) _NOTHROW _NODISCARD;
int   char_length(const char *const &pointer) _NOTHROW _NODISCARD;
Ulong mbstrlen(const char *pointer) _NOTHROW;
int   collect_char(const char *string, char *thechar) _NOTHROW;
int   advance_over(const char *string, Ulong &column) _NOTHROW;
Ulong step_left(const char *const buf, Ulong pos) _NOTHROW;
Ulong step_right(const char *const buf, Ulong pos) _NOTHROW;
int   mbstrcasecmp(const char *s1, const char *s2) _NOTHROW;
int   mbstrncasecmp(const char *s1, const char *s2, Ulong n) _NOTHROW;
char *mbstrcasestr(const char *haystack, const char *needle) _NOTHROW;
char *revstrstr(const char *haystack, const char *needle, const char *pointer) _NOTHROW;
char *mbrevstrcasestr(const char *haystack, const char *needle, const char *pointer) _NOTHROW;
char *mbstrchr(const char *string, const char *chr) _NOTHROW;
char *mbstrpbrk(const char *string, const char *accept) _NOTHROW;
char *mbrevstrpbrk(const char *head, const char *accept, const char *pointer) _NOTHROW;
bool  has_blank_char(const char *string) _NOTHROW;
bool  white_string(const char *string) _NOTHROW;
void  strip_leading_blanks_from(char *str) _NOTHROW;
void  strip_leading_chars_from(char *str, const char ch) _NOTHROW;

const char *_NODISCARD nstrchr_ccpp(const char *__s __no_null(), const char __c) noexcept;

/* Most functions in 'color.cpp'. */
void  set_interface_colorpairs(void);
void  set_syntax_colorpairs(syntaxtype *sntx);
void  prepare_palette(void);
void  find_and_prime_applicable_syntax(void);
void  check_the_multis(linestruct *line) _NOTHROW;
void  precalc_multicolorinfo(void) _NOTHROW;
bool  str_equal_to_rgx(const char *str, const regex_t *rgx);
short rgb_to_ncurses(Uchar value);
void  attr_idx_int_code(int idx, int *fg, int *bg);

/* Most functions in 'cut.cpp'. */
void expunge(undo_type action);
void do_delete(void);
void do_backspace(void);
void chop_previous_word(void) _NOTHROW;
void chop_next_word(void) _NOTHROW;
void extract_segment(linestruct *top, Ulong top_x, linestruct *bot, Ulong bot_x) _NOTHROW;
void ingraft_buffer(linestruct *topline) _NOTHROW;
void copy_from_buffer(linestruct *somebuffer) _NOTHROW;
void cut_marked_region(void) _NOTHROW;
void do_snip(bool marked, bool until_eof, bool append) _NOTHROW;
void cut_text(void) _NOTHROW;
void cut_till_eof(void) _NOTHROW;
void zap_text(void) _NOTHROW;
void zap_replace_text(const char *replacewith, Ulong len) _NOTHROW;
void copy_marked_region(void);
void copy_text(void);
void paste_text(void);

/* Most functions in 'files.cpp'. */
void   make_new_buffer(void) _NOTHROW;
bool   delete_lockfile(const char *lockfilename) _NOTHROW;
bool   has_valid_path(const char *filename) _NOTHROW;
bool   open_buffer(const char *filename, bool new_one);
void   open_buffer_browser(void);
void   open_new_empty_buffer(void);
void   set_modified(void) _NOTHROW;
void   prepare_for_display(void) _NOTHROW;
void   mention_name_and_linecount(void) _NOTHROW;
void   switch_to_prev_buffer(void) _NOTHROW;
void   switch_to_next_buffer(void) _NOTHROW;
void   free_one_buffer(openfilestruct *orphan, openfilestruct **open, openfilestruct **start) _NOTHROW;
void   close_buffer(void) _NOTHROW;
char  *encode_data(char *text, Ulong length) _NOTHROW;
void   read_file(FILE *f, int fd, const char *filename, bool undoable);
int    open_file(const char *filename, bool new_one, FILE **f);
char  *get_next_filename(const char *name, const char *suffix) _NOTHROW;
void   do_insertfile(void);
void   do_execute(void);
char  *get_full_path(const char *origpath) _NOTHROW;
char  *normalized_path(const char *path) _NOTHROW;
char  *abs_path(const char *path) _NOTHROW;
char  *safe_tempfile(FILE **stream);
void   init_operating_dir(void) _NOTHROW;
bool   outside_of_confinement(const char *currpath, bool allow_tabcomp) _NOTHROW;
void   init_backup_dir(void);
int    copy_file(FILE *inn, FILE *out, bool close_out);
bool   write_file(const char *name, FILE *thefile, bool normal, kind_of_writing_type method, bool annotate);
bool   write_region_to_file(const char *name, FILE *stream, bool normal, kind_of_writing_type method);
int    write_it_out(bool exiting, bool withprompt);
void   do_writeout(void);
void   do_savefile(void);
char  *real_dir_from_tilde(const char *path) _NOTHROW;
int    diralphasort(const void *va, const void *vb);
bool   is_dir(const char *const path);
char  *input_tab(char *buf, Ulong *place, void (*refresh_func)(void), bool *listed);

/* Some functions in 'global.cpp'. */
int              keycode_from_string(const char *keystring) _NOTHROW;
const keystruct *first_sc_for(const int menu, void (*function)(void)) _NOTHROW;
Ulong            shown_entries_for(int menu) _NOTHROW;
const keystruct *get_shortcut(const int keycode) _NOTHROW;
functionptrtype  func_from_key(const int keycode) _NOTHROW;
functionptrtype  interpret(const int keycode);
void             shortcut_init(void);

/* Some functions in 'help.cpp'. */
void wrap_help_text_into_buffer(void);
void do_help(void);

/* Most functions in 'history.cpp'. */
void  history_init(void) _NOTHROW;
void  reset_history_pointer_for(const linestruct *list) _NOTHROW;
void  update_history(linestruct **item, const char *text, bool avoid_duplicates) _NOTHROW;
char *get_history_completion(linestruct **h, char *s, Ulong len) _NOTHROW;
bool  have_statedir(void);
void  load_history(void);
void  save_history(void);
void  load_poshistory(void);
void  update_poshistory(void);
bool  has_old_position(const char *file, long *line, long *column);

/* Most functions in 'move.cpp'. */
void to_first_line(void) _NOTHROW;
void to_last_line(void) _NOTHROW;
void get_edge_and_target(Ulong *leftedge, Ulong *target_column) _NOTHROW;
void do_page_up(void) _NOTHROW;
void do_page_down(void) _NOTHROW;
void to_top_row(void) _NOTHROW;
void to_bottom_row(void) _NOTHROW;
void do_cycle(void);
void do_center(void);
void do_para_begin(linestruct **line) _NOTHROW;
void do_para_end(linestruct **line) _NOTHROW;
void to_para_begin(void);
void to_para_end(void);
void to_prev_block(void);
void to_next_block(void);
void do_prev_word(void) _NOTHROW;
bool do_next_word(bool after_ends) _NOTHROW;
void to_prev_word(void);
void to_next_word(void);
void do_home(void);
void do_end(void);
void do_up(void);
void do_down(void);
void do_scroll_up(void);
void do_scroll_down(void);
void do_left(void);
void do_right(void);

/* Most functions in 'nano.cpp'. */
linestruct *make_new_node(linestruct *prevnode) _NOTHROW;
void        splice_node(linestruct *afterthis, linestruct *newnode) _NOTHROW;
void        delete_node(linestruct *line) _NOTHROW;
void        unlink_node(linestruct *line) _NOTHROW;
void        free_lines(linestruct *src) _NOTHROW;
linestruct *copy_node(const linestruct *src) _NOTHROW _NODISCARD _NONNULL(1);
linestruct *copy_buffer(const linestruct *src) _NOTHROW _NODISCARD;
void        renumber_from(linestruct *line) _NOTHROW;
void        print_view_warning(void) _NOTHROW;
bool        in_restricted_mode(void) _NOTHROW;
void        suggest_ctrlT_ctrlZ(void) _NOTHROW;
void        finish(void) _NOTHROW _NO_RETURN;
void        close_and_go(void);
void        do_exit(void);
void        die(const char *msg, ...) _NO_RETURN _NONNULL(1);
void        window_init(void) _NOTHROW;
void        install_handler_for_Ctrl_C(void) _NOTHROW;
void        restore_handler_for_Ctrl_C(void) _NOTHROW;
void        reconnect_and_store_state(void) _NOTHROW;
void        handle_hupterm(int signal) _NO_RETURN;
void        handle_crash(int signal) _NO_RETURN;
void        suspend_nano(int signal);
void        do_suspend(void);
void        continue_nano(int signal);
void        block_sigwinch(bool blockit);
void        handle_sigwinch(int signal);
void        regenerate_screen(void);
void        disable_kb_interrupt(void) _NOTHROW;
void        enable_kb_interrupt(void) _NOTHROW;
void        disable_flow_control(void) _NOTHROW;
void        enable_flow_control(void) _NOTHROW;
void        terminal_init(void) _NOTHROW;
void        confirm_margin(void) _NOTHROW;
void        unbound_key(int code) _NOTHROW;
bool        wanted_to_move(functionptrtype f);
bool        changes_something(functionptrtype f);
void        inject(char *burst, Ulong count);

/* Most functions in 'prompt.cpp'. */
void  statusbar_discard_all_undo_redo(void) _NOTHROW;
void  do_statusbar_undo(void) _NOTHROW;
void  do_statusbar_redo(void) _NOTHROW;
void  do_statusbar_home(void) _NOTHROW;
void  do_statusbar_end(void) _NOTHROW;
void  do_statusbar_prev_word(void) _NOTHROW;
void  do_statusbar_next_word(void) _NOTHROW;
void  do_statusbar_left(void) _NOTHROW;
void  do_statusbar_right(void) _NOTHROW;
void  do_statusbar_backspace(bool with_undo) _NOTHROW;
void  do_statusbar_delete(void) _NOTHROW;
void  inject_into_answer(char *burst, Ulong count) _NOTHROW;
void  do_statusbar_chop_next_word(void) _NOTHROW;
void  do_statusbar_chop_prev_word(void) _NOTHROW;
Ulong get_statusbar_page_start(Ulong base, Ulong column) _NOTHROW;
void  put_cursor_at_end_of_answer(void) _NOTHROW;
void  add_or_remove_pipe_symbol_from_answer(void) _NOTHROW;
int   do_prompt(int menu, const char *provided, linestruct **history_list, void (*refresh_func)(void), const char *msg, ...);
int   ask_user(bool withall, const char *question);


/* ---------------------------------------------------------- rcfile.cpp ---------------------------------------------------------- */


short      color_to_short(const char *colorname, bool &vivid, bool &thick);
char      *parse_next_word(char *ptr) _NOTHROW;
void       parse_rule(char *ptr, int rex_flags);
bool       compile(const char *expression, int rex_flags, regex_t **packed);
void       begin_new_syntax(char *ptr);
bool       parse_combination(char *combotext, short *fg, short *bg, int *attributes);
void       set_interface_color(const u_char element, char *combotext);
void       display_rcfile_errors(void) _NOTHROW;
void       jot_error(const char *msg, ...) _NOTHROW;
keystruct *strtosc(const char *input);
bool       is_good_file(char *file) _NOTHROW;
void       parse_one_include(char *file, syntaxtype *syntax);
short      closest_index_color(short red, short green, short blue);
void       grab_and_store(const char *kind, char *ptr, regexlisttype **storage);
bool       parse_syntax_commands(const char *keyword, char *ptr);
void       parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only);
void       do_rcfiles(void);


/* ---------------------------------------------------------- search.cpp ---------------------------------------------------------- */


bool  regexp_init(const char *regexp);
void  tidy_up_after_search(void);
int   findnextstr(const char *needle, bool whole_word_only, int modus, Ulong *match_len, bool skipone, const linestruct *begin, Ulong begin_x);
void  do_search_forward(void);
void  do_search_backward(void);
void  do_findprevious(void);
void  do_findnext(void);
void  not_found_msg(const char *str) _NOTHROW;
void  go_looking(void);
long  do_replace_loop(const char *needle, bool whole_word_only, const linestruct *real_current, Ulong *real_current_x);
void  do_replace(void);
void  ask_for_and_do_replacements(void);
void  goto_line_posx(long line, Ulong pos_x) _NOTHROW;
void  goto_line_and_column(long line, long column, bool retain_answer, bool interactive);
void  do_gotolinecolumn(void);
bool  find_a_bracket(bool reverse, const char *bracket_pair) _NOTHROW;
void  do_find_bracket(void) _NOTHROW;
void  put_or_lift_anchor(void);
void  to_prev_anchor(void);
void  to_next_anchor(void);
char *find_global_header(const char *str);
char *find_local_header(const char *str);


/* ---------------------------------------------------------- text.cpp ---------------------------------------------------------- */


void  do_mark(void) _NOTHROW;
void  do_tab(void);
void  do_indent(void) _NOTHROW;
void  do_unindent(void) _NOTHROW;
void  do_comment(void) _NOTHROW;
void  enclose_marked_region(const char *s1, const char *s2) _NOTHROW;
void  insert_empty_line(linestruct *line, bool above, bool autoindent) _NOTHROW;
void  do_insert_empty_line_above(void) _NOTHROW;
void  do_insert_empty_line_below(void) _NOTHROW;
void  do_undo(void);
void  do_redo(void);
void  do_enter(void);
void  discard_until_in_buffer(openfilestruct *const buffer, const undostruct *const thisitem);
void  discard_until(const undostruct *thisitem) _NOTHROW;
void  add_undo(undo_type action, const char *message) _NOTHROW;
void  update_multiline_undo(long lineno, char *indentation) _NOTHROW;
void  update_undo(undo_type action) _NOTHROW;
void  do_wrap(void);
long  break_line(const char *textstart, long goal, bool snap_at_nl) _NOTHROW;
Ulong indent_length(const char *line) _NOTHROW;
Ulong quote_length(const char *line) _NOTHROW;
bool  begpar(const linestruct *const line, int depth) _NOTHROW;
bool  inpar(const linestruct *const line) _NOTHROW;
void  do_justify(void);
void  do_full_justify(void);
void  do_spell(void);
void  do_linter(void);
void  do_formatter(void);
void  count_lines_words_and_characters(void);
void  do_verbatim_input(void);
char *copy_completion(char *text);
void  complete_a_word(void);
char *lower_case_word(const char *str);


/* ---------------------------------------------------------- utils.cpp ---------------------------------------------------------- */


void        get_homedir(void) _NOTHROW;
char      **get_env_paths(Ulong *npaths) _NOTHROW _NODISCARD _NONNULL(1);
char       *concatenate(const char *path, const char *name) _NOTHROW _NODISCARD _RETURNS_NONNULL _NONNULL(1, 2);
char       *concatenate_path(const char *prefix, const char *suffix) _NOTHROW _NODISCARD _RETURNS_NONNULL _NONNULL(1, 2);
const char *concat_path(const char *s1, const char *s2) _NOTHROW _NONNULL(1, 2);
int         digits(long n) _NOTHROW _NODISCARD;
bool        parse_line_column(const char *str, long *line, long *column) _NOTHROW _NONNULL(1, 2, 3);
void        recode_NUL_to_LF(char *string, Ulong length) _NOTHROW _NONNULL(1);
Ulong       recode_LF_to_NUL(char *string) _NOTHROW _NONNULL(1);
void        free_chararray(char **array, Ulong len) _NOTHROW;
void        append_chararray(char ***array, Ulong *len, char **append, Ulong append_len) _NOTHROW _NONNULL(1, 2, 3);
bool        is_separate_word(Ulong position, Ulong length, const char *buf) _NOTHROW;
void       *nmalloc(const Ulong howmuch) _NOTHROW _RETURNS_NONNULL;
void       *nrealloc(void *ptr, const Ulong howmuch) _NOTHROW _RETURNS_NONNULL _NONNULL(1);
#define     arealloc(ptr, howmuch) (__TYPE(ptr))xrealloc(ptr, howmuch)
char       *mallocstrcpy(char *dest, const char *src) _NOTHROW;
char       *free_and_assign(char *dest, char *src) _NOTHROW;
Ulong       get_page_start(Ulong column) _NOTHROW;
Ulong       xplustabs(void) _NOTHROW _NODISCARD;
Ulong       actual_x(const char *text, Ulong column) _NOTHROW _NODISCARD _NONNULL(1);
Ulong       wideness(const char *text, Ulong maxlen) _NOTHROW _NODISCARD _NONNULL(1);
Ulong       breadth(const char *text) _NOTHROW _NODISCARD _NONNULL(1);
void        new_magicline(void) _NOTHROW;
void        remove_magicline(void) _NOTHROW;
bool        mark_is_before_cursor(void) _NOTHROW;
void        get_region(linestruct **top, Ulong *top_x, linestruct **bot, Ulong *bot_x) _NOTHROW;
void        get_range(linestruct **top, linestruct **bot) _NOTHROW;
linestruct *line_from_number(long number) _NOTHROW;
Ulong       number_of_characters_in(const linestruct *begin, const linestruct *end) _NOTHROW _NONNULL(1, 2);
const char *strstrwrapper(const char *haystack, const char *needle, const char *start) _NOTHROW _NONNULL(1);
char       *alloced_pwd(void);
char       *alloc_str_free_substrs(char *str_1, char *str_2) _NOTHROW _NONNULL(1, 2);
void        append_str(char **str, const char *appen_str);
char       *alloced_current_file_dir(void);
char       *alloced_full_current_file_dir(void);
void        alloced_remove_at(char **str, int at);
const char *word_strstr(const char *data, const char *needle);
char      **retrieve_exec_output(const char *cmd, Uint *n_lines);
const char *word_strstr_array(const char *str, const char **substrs, Uint count, Uint *index);
const char *strstr_array(const char *str, const char **substrs, Uint count, Uint *index);
const char *string_strstr_array(const char *str, const vector<string> &substrs, Uint *index);
void        set_mark(long lineno, Ulong pos_x) _NOTHROW;
int         qsort_strlen(const void *a, const void *b);

/* Most functions in 'winio.cpp'. */
void  record_macro(void) _NOTHROW;
void  run_macro(void) _NOTHROW;
void  reserve_space_for(Ulong newsize) _NOTHROW;
Ulong waiting_keycodes(void) _NOTHROW;
void  implant(const char *string) _NOTHROW;
int   get_input(WINDOW *win);
int   convert_CSI_sequence(const int *seq, Ulong length, int *consumed) _NOTHROW;
int   get_kbinput(WINDOW *win, bool showcursor);
char *get_verbatim_kbinput(WINDOW *win, Ulong *count) _NOTHROW;
int   get_mouseinput(int *mouse_y, int *mouse_x, bool allow_shortcuts) _NOTHROW;
void  blank_edit(void) _NOTHROW;
void  blank_statusbar(void) _NOTHROW;
void  wipe_statusbar(void) _NOTHROW;
void  blank_bottombars(void) _NOTHROW;
void  blank_it_when_expired(void) _NOTHROW;
void  set_blankdelay_to_one(void) _NOTHROW;
char *display_string(const char *buf, Ulong column, Ulong span, bool isdata, bool isprompt) _NOTHROW;
void  titlebar(const char *path) _NOTHROW;
void  minibar(void) _NOTHROW;
void  statusline(message_type importance, const char *msg, ...) _NOTHROW;
void  statusbar(const char *msg) _NOTHROW;
void  warn_and_briefly_pause(const char *msg) _NOTHROW _NONNULL(1);
void  bottombars(int menu) _NOTHROW;
void  post_one_key(const char *keystroke, const char *tag, int width) _NOTHROW;
void  place_the_cursor(void) _NOTHROW;
void  draw_row(const int row, const char *converted, linestruct *line, const Ulong from_col);
int   update_line(linestruct *line, Ulong index, int offset = 0);
int   update_softwrapped_line(linestruct *line);
bool  line_needs_update(const Ulong old_column, const Ulong new_column) _NOTHROW;
int   go_back_chunks(int nrows, linestruct **line, Ulong *leftedge) _NOTHROW;
int   go_forward_chunks(int nrows, linestruct **line, Ulong *leftedge) _NOTHROW;
bool  less_than_a_screenful(Ulong was_lineno, Ulong was_leftedge) _NOTHROW _NODISCARD;
void  edit_scroll(bool direction);
Ulong get_softwrap_breakpoint(const char *linedata, Ulong leftedge, bool *kickoff, bool *end_of_line) _NOTHROW;
Ulong get_chunk_and_edge(Ulong column, linestruct *line, Ulong *leftedge) _NOTHROW _NONNULL(2);
Ulong chunk_for(Ulong column, linestruct *line) _NOTHROW;
Ulong leftedge_for(Ulong column, linestruct *line) _NOTHROW;
Ulong extra_chunks_in(linestruct *line) _NOTHROW _NODISCARD _NONNULL(1);
void  ensure_firstcolumn_is_aligned(void) _NOTHROW;
Ulong actual_last_column(Ulong leftedge, Ulong column) _NOTHROW;
bool  current_is_offscreen(void) _NOTHROW;
void  edit_redraw(linestruct *old_current, update_type manner);
void  edit_refresh(void);
void  adjust_viewport(update_type manner) _NOTHROW;
void  full_refresh(void) _NOTHROW;
void  draw_all_subwindows(void);
void  report_cursor_position(void) _NOTHROW;
void  spotlight(Ulong from_col, Ulong to_col) _NOTHROW;
void  spotlight_softwrapped(Ulong from_col, Ulong to_col) _NOTHROW;
void  do_credits(void);

/* These are just name definitions. */
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

/* All functions in 'cpp.cpp'. */
bool   isCppSyntaxChar(const char c);
void   get_line_indent(linestruct *line, Ushort *tabs, Ushort *spaces, Ushort *t_char, Ushort *t_tabs) _NONNULL(1, 2, 3, 4, 5);
Ushort indent_char_len(linestruct *line);
void   do_block_comment(void) _NOTHROW;
bool   enter_with_bracket(void);
void   all_brackets_pos(void);
void   do_close_bracket(void);
void   do_parse(void);
void   do_test(void);
void   do_test_window(void);
int    current_line_scope_end(linestruct *line);
function_info_t *parse_func(const char *str);
function_info_t  parse_local_func(const char *str);
bool   invalid_variable_sig(const char *sig);
void   parse_variable(const char *sig, char **type, char **name, char **value);
void   flag_all_brackets(void);
void   flag_all_block_comments(linestruct *from);
void   remove_local_vars_from(linestruct *line);
void   remove_from_color_map(linestruct *line, int color, int type);

/* 'syntax.cpp' */
void   syntax_check_file(openfilestruct *file);
bool   syntax_map_exists(file_type type, const char *const restrict key, vec4 *const color);

/* 'netlog.cpp' */
void netlog_syntaxtype(syntaxtype *s);
void netlog_colortype(colortype *c);
void netlog_func_info(function_info_t *info);
void debug_define(const DefineEntry &de);
void netlog_openfiles(void);

/* 'words.cpp' */
char       **split_into_words(const char *str, const u_int len, u_int *word_count);
bool         word_more_than_one_white_away(const char *string, Ulong index, bool forward, Ulong *nsteps) _NOTHROW _NODISCARD _NONNULL(1, 4);
bool         cursor_word_more_than_one_white_away(bool forward, Ulong *nsteps) _NOTHROW;
bool         char_is_in_word(const char *word, const char ch, Ulong *at);
char        *retrieve_word_from_cursor_pos(bool forward);
line_word_t *line_word_list(const char *str, Ulong slen);
line_word_t *get_line_words(const char *string, Ulong slen);
Uint         last_strchr(const char *str, const char ch, Uint maxlen);
char        *memmove_concat(const char *s1, const char *s2);
const char  *substr(const char *str, Ulong end_index);
Ulong        get_prev_word_start_index(const char *line, Ulong cursor_x, bool allow_underscore = FALSE) _NOTHROW _NODISCARD _NONNULL(1);
Ulong        get_prev_cursor_word_start_index(bool allow_underscore = FALSE) _NOTHROW _NODISCARD;
char        *get_prev_word(const char *cursorline, const Ulong cursor_x, Ulong *wordlen) _NOTHROW _NODISCARD _NONNULL(1, 3);
char        *get_prev_cursor_word(Ulong *wordlen) _NOTHROW _NODISCARD _NONNULL(1);
Ulong        get_current_word_end_index(const char *line, Ulong from_index, bool allow_underscore = FALSE) _NOTHROW _NODISCARD _NONNULL(1);
Ulong        get_current_cursor_word_end_index(bool allow_underscore = FALSE) _NOTHROW;


/* ---------------------------------------------------------- lines.cpp ---------------------------------------------------------- */


bool  is_line_comment(linestruct *line);
bool  is_line_start_end_bracket(linestruct *line, bool *is_start);
void  inject_in_line(linestruct *line, const char *str, Ulong at);
void  move_line(linestruct *line, bool up) _NOTHROW;
void  move_lines_up(void) _NOTHROW;
void  move_lines_down(void) _NOTHROW;
void  erase_in_line(linestruct *line, Ulong at, Ulong len);
Uint  total_tabs(linestruct *line);
int   get_editwin_row(linestruct *line);
bool  line_in_marked_region(linestruct *line) _NOTHROW _NODISCARD _NONNULL(1);


/* ---------------------------------------------------------- threadpool.cpp ---------------------------------------------------------- */


void  lock_pthread_mutex(pthread_mutex_t *mutex, bool lock) _NOTHROW;
void  pause_all_sub_threads(bool pause) _NOTHROW;
void  init_queue_task(void) _NOTHROW;
int   task_queue_count(void) _NOTHROW;
void  shutdown_queue(void) _NOTHROW;
void  submit_task(task_functionptr_t function, void *arg, void **result, callback_functionptr_t callback) _NOTHROW;
void  stop_thread(Uchar thread_id) _NOTHROW;
Uchar thread_id_from_pthread(pthread_t *thread) _NOTHROW;


/* ---------------------------------------------------------- event.cpp ---------------------------------------------------------- */


bool is_main_thread(void) _NOTHROW;
void send_SIGUSR1_to_main_thread(void) _NOTHROW;
void send_signal_to_main_thread(callback_functionptr_t func, void *arg) _NOTHROW;
void init_event_handler(void) _NOTHROW;
void enqueue_callback(callback_functionptr_t callback, void *result) _NOTHROW;
void prosses_callback_queue(void) _NOTHROW;
void cleanup_event_handler(void) _NOTHROW;


/* ---------------------------------------------------------- tasks.cpp ---------------------------------------------------------- */


void sub_thread_find_syntax(const char *path);
void get_line_list_task(const char *path);


/* ---------------------------------------------------------- signal.cpp ---------------------------------------------------------- */


void init_main_thread(void) _NOTHROW;
void cleanup_main_thread(void) _NOTHROW;
void setup_signal_handler_on_sub_thread(void (*handler)(int)) _NOTHROW;
void block_pthread_sig(int sig, bool block) _NOTHROW;


/* ---------------------------------------------------------- render.cpp ---------------------------------------------------------- */


void render_line_text(int row, const char *str, linestruct *line, Ulong from_col) _NOTHROW;
void apply_syntax_to_line(const int row, const char *converted, linestruct *line, Ulong from_col);


/* ---------------------------------------------------------- render_utils.cpp ---------------------------------------------------------- */


void        get_next_word(const char **start, const char **end);
char       *parse_function_sig(linestruct *line);
void        find_word(linestruct *line, const char *data, const char *word, const Ulong slen, const char **start, const char **end);
int         preprossesor_data_from_key(const char *key);
void        free_local_var(local_var_t *var);
local_var_t parse_local_var(linestruct *line);
int         find_class_end_line(linestruct *from);
void        add_rm_color_map(string str, syntax_data_t data);


/* ---------------------------------------------------------- suggestion.cpp ---------------------------------------------------------- */


void do_suggestion(void);
void find_suggestion(void);
void clear_suggestion(void);
void add_char_to_suggest_buf(void);
void draw_suggest_win(void);
void accept_suggestion(void);

/* ----------------------------- Gui suggestmenu ----------------------------- */

void gui_suggestmenu_create(void);
void gui_suggestmenu_free(void);
void gui_suggestmenu_clear(void);
void gui_suggestmenu_load_str(void);
void gui_suggestmenu_find(void);
void gui_suggestmenu_run(void);
// void gui_suggestmenu_resize(void);
// void gui_suggestmenu_draw_selected(void);
// void gui_suggestmenu_draw_text(void);
// void gui_suggestmenu_selected_up(void);
// void gui_suggestmenu_selected_down(void);
// bool gui_suggestmenu_accept(void);
void gui_suggestmenu_hover_action(float y_pos);
void gui_suggestmenu_scroll_action(bool direction, float y_pos);
void gui_suggestmenu_click_action(float y_pos);


/* ---------------------------------------------------------- parse.cpp ---------------------------------------------------------- */


void parse_class_data(linestruct *from);
void parse_var_type(const char *data);
void line_variable(linestruct *line, vector<var_t> &var_vector);
void func_decl(linestruct *line);


/* ---------------------------------------------------------- brackets.cpp ---------------------------------------------------------- */


bool  find_matching_bracket(linestruct *start_line, Ulong start_index, linestruct **to_line, Ulong *to_index);
bool  find_end_bracket(linestruct *from, Ulong index, linestruct **end, Ulong *end_index);
char *fetch_bracket_body(linestruct *from, Ulong index);

#ifdef HAVE_GLFW
  /* ---------------------------------------------------------- gui.cpp ---------------------------------------------------------- */
  

  void log_error_gui(const char *format, ...);
  void init_gui(void);
  void glfw_loop(void);
  bool gui_quit(void);
  
  
  /* ---------------------------------------------------------- gui/guiutils.cpp ---------------------------------------------------------- */
  
  
  void  upload_texture_atlas(texture_atlas_t *atlas);
  float glyph_width(const char *current, const char *prev, texture_font_t *font);
  float string_pixel_offset(const char *string, const char *previous_char, Ulong index, texture_font_t *font);
  float pixel_breadth(texture_font_t *font, const char *text);
  long  index_from_mouse_x(const char *string, Uint len, texture_font_t *font, float start_x);
  long  index_from_mouse_x(const char *string, texture_font_t *font, float start_x);
  linestruct *line_from_mouse_y(texture_font_t *font, float y);
  linestruct *line_and_index_from_mousepos(texture_font_t *const font, Ulong *const index) _RETURNS_NONNULL _NONNULL(1, 2);
  float get_line_number_pixel_offset(linestruct *line, texture_font_t *font);
  float line_pixel_x_pos(linestruct *line, Ulong index, texture_font_t *font);
  float cursor_pixel_x_pos(texture_font_t *font);
  float line_y_pixel_offset(linestruct *line, texture_font_t *font);
  float cursor_pixel_y_pos(texture_font_t *font);
  void  add_glyph(const char *current, const char *previous, vertex_buffer_t *buffer, texture_font_t *font, vec4 color, vec2 *pen);
  void  vertex_buffer_add_string(vertex_buffer_t *buffer, const char *string, Ulong slen, const char *previous, texture_font_t *font, vec4 color, vec2 *pen);
  void  add_openfile_cursor(texture_font_t *font, vertex_buffer_t *buffer, vec4 color);
  void  add_cursor(texture_font_t *font, vertex_buffer_t *buf, vec4 color, vec2 at);
  void  update_projection_uniform(Uint shader);
  void vertex_buffer_add_element_lable(guielement *element, texture_font_t *font, vertex_buffer_t *buffer);
  void vertex_buffer_add_element_lable_offset(guielement *element, texture_font_t *font, vertex_buffer_t *buf, vec2 offset);
  bool is_ancestor(guielement *e, guielement *ancestor);
  vec4 color_idx_to_vec4(int index) _NOTHROW;
  linestruct *gui_line_from_number(guieditor *editor, long number);
  long get_lineno_from_scrollbar_position(guieditor *editor, float ypos);
  
  
  /* ---------------------------------------------------------- gui/guicallback.cpp ---------------------------------------------------------- */
  
  
  void window_resize_callback(GLFWwindow *window, int newwidth, int newheight);
  void window_maximize_callback(GLFWwindow *window, int maximized);
  void framebuffer_resize_callback(GLFWwindow *window, int width, int height);
  void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
  void char_callback(GLFWwindow *window, Uint ch);
  void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
  void mouse_pos_callback(GLFWwindow *window, double x, double y);
  void window_enter_callback(GLFWwindow *window, int entered);
  void scroll_callback(GLFWwindow *window, double x, double y);
  
  
  /* ---------------------------------------------------------- gui/guiwinio.cpp ---------------------------------------------------------- */
  
  
  void draw_rect(vec2 pos, vec2 size, vec4 color);
  void render_vertex_buffer(Uint shader, vertex_buffer_t *buf);
  void show_statusmsg(message_type type, float seconds, const char *format, ...);
  void show_toggle_statusmsg(int flag);
  void draw_editor(guieditor *editor);
  void draw_topbar(void);
  void draw_suggestmenu(void);
  void draw_botbar(void);
  void draw_statusbar(void);
  void do_fullscreen(GLFWwindow *window);
  int  glfw_get_framerate(void);
  
  
  /* ---------------------------------------------------------- gui/guiprompt.cpp ---------------------------------------------------------- */
  
  
  void gui_promptmode_enter(void);
  void gui_promptmode_leave(void);
  void gui_ask_user(const char *question, guiprompt_type type);
  long prompt_index_from_mouse(bool allow_outside);
  void gui_promptmenu_create(void);
  void gui_promptmenu_free(void);
  void gui_promptmenu_resize(void);
  void gui_promptmenu_draw_text(void);
  void gui_promptmenu_draw_selected(void);
  void gui_promptmenu_selected_up(void);
  void gui_promptmenu_selected_down(void);
  void gui_promptmenu_enter_action(void);
  void gui_promptmenu_completions_search(void);
  void gui_promptmenu_hover_action(float y_pos);
  void gui_promptmenu_scroll_action(bool direction, float y_pos);
  void gui_promptmenu_click_action(float y_pos);
  
  /* ----------------------------- Open file ----------------------------- */
  
  void gui_promptmenu_open_file(void);

  /* ----------------------------- Set font ----------------------------- */

  void gui_promptmenu_set_font(void);

  
  /* ---------------------------------------------------------- gui/guifiles.cpp ---------------------------------------------------------- */
  
  
  void gui_switch_to_prev_buffer(void);
  void gui_switch_to_next_buffer(void);
  bool gui_delete_lockfile(const char *lockfile);
  void gui_close_buffer(void);
  bool gui_close_and_go(void);
  
  
  /* ---------------------------------------------------------- gui/guielement.cpp ---------------------------------------------------------- */
  
  
  guielement *gui_element_create(vec2 pos, vec2 size, vec2 endoff, vec4 color, bool in_gridmap = TRUE) _NOTHROW;
  guielement *gui_element_create(bool in_gridmap = TRUE) _NOTHROW;
  guielement *gui_element_create(guielement *const parent, bool in_gridmap = TRUE) _NOTHROW;
  void        gui_element_free(guielement *const element);
  void        gui_element_set_lable(guielement *const element, const char *string) _NOTHROW;
  void        gui_element_delete_children(guielement *const element);
  guielement *gui_element_from_mousepos(void);
  void        gui_element_resize(guielement *const e, vec2 size);
  void        gui_element_move(guielement *const e, vec2 pos);
  void        gui_element_move_y_clamp(guielement *const e, float ypos, float min, float max);
  void        gui_element_move_resize(guielement *const e, vec2 pos, vec2 size);
  void        gui_element_delete_borders(guielement *const e);
  void        gui_element_set_borders(guielement *const e, vec4 size, vec4 color);
  void        gui_element_draw(guielement *const e);
  void        gui_element_set_raw_data(guielement *const element, void *const data) _NOTHROW;
  void        gui_element_set_file_data(guielement *const element, openfilestruct *const file) _NOTHROW;
  void        gui_element_set_editor_data(guielement *const element, guieditor *const editor) _NOTHROW;
  void        gui_element_set_sb_data(guielement *const e, GuiScrollbar *const sb) _NOTHROW;
  void        gui_element_set_menu_data(guielement *const e, Menu *const menu) _NOTHROW;
  bool        gui_element_has_raw_data(guielement *const e) _NOTHROW;
  bool        gui_element_has_file_data(guielement *const element) _NOTHROW;
  bool        gui_element_has_editor_data(guielement *const element) _NOTHROW;
  bool        gui_element_has_sb_data(guielement *const e) _NOTHROW;
  bool        gui_element_has_menu_data(guielement *const e) _NOTHROW;
  void        gui_element_set_flag_recurse(guielement *const e, bool set, Uint flag);
  void        gui_element_call_cb(guielement *const e, guielement_callback_type type);

  
  /* ---------------------------------------------------------- gui/guieditor.cpp ---------------------------------------------------------- */
  
  
  void       gui_editor_refresh_topbar(guieditor *const editor);
  void       gui_editor_update_active_topbar(guieditor *editor);
  void       make_new_editor(bool new_buffer);
  void       gui_editor_free(guieditor *const editor);
  void       gui_editor_close(void);
  void       gui_editor_hide(guieditor *const editor, bool hide);
  void       gui_editor_switch_to_prev(void);
  void       gui_editor_switch_to_next(void);
  void       gui_editor_switch_openfile_to_prev(void);
  void       gui_editor_switch_openfile_to_next(void);
  void       gui_editor_set_open(guieditor *const editor);
  guieditor *gui_editor_from_element(guielement *e);
  guieditor *gui_editor_from_file(openfilestruct *file);
  void       gui_editor_calculate_rows(guieditor *const editor);
  void       gui_editor_resize(guieditor *const editor);
  void       gui_editor_redecorate(guieditor *const editor);
  void       gui_editor_open_new_empty_buffer(void);
  void       gui_editor_close_open_buffer(void);
  void       gui_editor_open_buffer(const char *const restrict path);
  void       gui_editor_update_all(void);

  
  /* ---------------------------------------------------------- gui/guigrid.cpp ---------------------------------------------------------- */
  

  guigridsection *make_new_gridsection(void);
  void gridsection_resize(guigridsection *section, Ulong newsize);
  void gridsection_add_editor(guigridsection *section, guieditor *editor);
  guigrid *make_new_grid(void);
#endif


/* ---------------------------------------------------------- cfg.cpp ---------------------------------------------------------- */


bool lookup_coloropt(const char *color, int len, int *color_opt) _NOTHROW;
void init_cfg(void);
void cleanup_cfg(void) _NOTHROW;

/* bash_lsp.cpp */
void get_env_path_binaries(void);

/* nstring.cpp */
Ulong  inject_in(char **dst, Ulong dstlen, const char *src, Ulong srclen, Ulong at, bool realloc = TRUE) _NOTHROW _NONNULL(1, 3);
Ulong  inject_in(char **dst, const char *src, Ulong srclen, Ulong at, bool realloc = TRUE) _NOTHROW _NONNULL(1, 2);
Ulong  inject_in(char **dst, const char *src, Ulong at, bool realloc = TRUE) _NOTHROW _NONNULL(1, 2);
void   inject_in_cursor(const char *src, Ulong srclen, bool advance_x) _NOTHROW _NONNULL(1);
Ulong  erase_in(char **str, Ulong slen, Ulong at, Ulong eraselen, bool do_realloc) _NOTHROW _NONNULL(1);
Ulong  erase_in(char **str, Ulong at, Ulong eraselen, bool do_realloc = TRUE) _NOTHROW _NONNULL(1);
Ulong  append_to(char **dst, Ulong dstlen, const char *src, Ulong srclen) _NOTHROW _NONNULL(1, 3);
Ulong  append_to(char **dst, const char *src, Ulong srclen) _NOTHROW _NONNULL(1, 2);
Ulong  append_to(char **dst, const char *src) _NOTHROW _NONNULL(1, 2);
char **split_string_nano(const char *string, const char delim, Ulong *n) _NOTHROW _NODISCARD _NONNULL(1, 3);


/* ---------------------------------------------------------- gui/font/loading.cpp ---------------------------------------------------------- */


/* ----------------------------- Gui font ----------------------------- */

GuiFont         *gui_font_create(void);
void             gui_font_free(GuiFont *const f);
void             gui_font_load(GuiFont *const f, const char *const restrict path, Uint size, Uint atlas_size);
texture_font_t  *gui_font_get_font(GuiFont *const f);
texture_atlas_t *gui_font_get_atlas(GuiFont *const f);
texture_glyph_t *gui_font_get_glyph(GuiFont *const f, const char *const restrict codepoint);
Uint             gui_font_get_size(GuiFont *const f);
long             gui_font_get_line_height(GuiFont *const f);
bool             gui_font_is_mono(GuiFont *const f);
float            gui_font_height(GuiFont *const f);
float            gui_font_row_baseline(GuiFont *const f, long row);
void             gui_font_row_top_bot(GuiFont *const f, long row, float *const top, float *const bot);
void             gui_font_change_size(GuiFont *const f, Uint new_size);
void             gui_font_increase_size(GuiFont *const f);
void             gui_font_decrease_size(GuiFont *const f);
void             gui_font_decrease_line_height(GuiFont *const f);
void             gui_font_increase_line_height(GuiFont *const f);
bool             gui_font_row_from_pos(GuiFont *const f, float y_top, float y_bot, float y_pos, long *outrow);

/* ----------------------------- General ----------------------------- */

void free_atlas(texture_atlas_t *atlas);
// void free_gui_font(bool uifont);
// void set_gui_font(const char *const restrict path, Uint size);
// void set_gui_uifont(const char *const restrict path, Uint size);
// void set_all_gui_fonts(const char *const restrict path, Uint size, Uint uisize);
// void change_gui_font_size(Uint size);
void list_available_fonts(void);


/* ---------------------------------------------------------- gui/font/utils.cpp ---------------------------------------------------------- */


// float row_baseline_pixel(long lineno, texture_font_t *const font);
// void row_top_bot_pixel(long lineno, texture_font_t *const font, float *const top, float *const bot);
void line_add_cursor(long lineno, GuiFont *const font, vertex_buffer_t *const buf, vec4 color, float xpos, float yoffset);
vertex_buffer_t *make_new_font_buffer(void) _NODISCARD;


/* ---------------------------------------------------------- gui/rendering/utils.cpp ---------------------------------------------------------- */


float pixnbreadth_prev(const char *const restrict string, long len, const char *const restrict prev_char);
float pixnbreadth(const char *const restrict string, long len);
float pixbreadth(texture_font_t *const font, const char *const restrict string);
float pixbreadth(GuiFont *const font, const char *const restrict string);


/* ---------------------------------------------------------- gui/cursor/cursor.cpp ---------------------------------------------------------- */


linestruct *line_from_cursor_pos(guieditor *const editor) _RETURNS_NONNULL;
float *pixpositions(const char *const restrict string, float normx, Ulong *outlen, texture_font_t *const font);
Ulong closest_index(float *array, Ulong len, float rawx, texture_font_t *const font);
Ulong index_from_pix_xpos(const char *const restrict string, float rawx, float normx, texture_font_t *const font);


/* ---------------------------------------------------------- gui/scollbar.cpp ---------------------------------------------------------- */


void          calculate_scrollbar(float total_pixel_length, Uint start_entry, Uint total_entries, Uint visable_entries, Uint current_entry, float *height, float *relative_y_position);
long          index_from_scrollbar_pos(float total_pixel_length, Uint startidx, Uint endidx, Uint visable_idxno, float ypos) __THROW _NODISCARD;
GuiScrollbar *gui_scrollbar_create(guielement *const parent, void *const data, GuiScrollbarUpdateFunc update_routine, GuiScrollbarMoveFunc moving_routine) __THROW _RETURNS_NONNULL _NODISCARD _NONNULL(1, 2, 3);
void          gui_scrollbar_move(GuiScrollbar *const sb, float change) _NONNULL(1);
void          gui_scrollbar_draw(GuiScrollbar *const sb) _NONNULL(1);
void          gui_scrollbar_refresh_needed(GuiScrollbar *const sb) __THROW _NONNULL(1);
bool          gui_scrollbar_element_is_base(GuiScrollbar *const sb, guielement *const e) __THROW _NODISCARD _NONNULL(1, 2);
bool          gui_scrollbar_element_is_thumb(GuiScrollbar *const sb, guielement *const e) __THROW _NODISCARD _NONNULL(1, 2);
float         gui_scrollbar_width(GuiScrollbar *const sb);
void          gui_scrollbar_show(GuiScrollbar *const sb, bool show);


/* ---------------------------------------------------------- gui/menu.cpp ---------------------------------------------------------- */


Menu    *gui_menu_create(guielement *const parent, GuiFont *const font, void *data, MenuPosFunc position_routine, MenuAcceptFunc accept_routine);
Menu    *gui_menu_create_submenu(Menu *const parent, const char *const restrict lable, void *data, MenuAcceptFunc accept_routine);
void     gui_menu_free(Menu *const menu);
void     gui_menu_draw(Menu *const menu);
void     gui_menu_push_back(Menu *const menu, const char *const restrict string);
void     gui_menu_pos_refresh_needed(Menu *const menu);
void     gui_menu_text_refresh_needed(Menu *const menu);
void     gui_menu_scrollbar_refresh_needed(Menu *const menu);
void     gui_menu_show(Menu *const menu, bool show);
void     gui_menu_selected_up(Menu *const menu);
void     gui_menu_selected_down(Menu *const menu);
void     gui_menu_exit_submenu(Menu *const menu);
void     gui_menu_enter_submenu(Menu *const menu);
void     gui_menu_accept_action(Menu *const menu);
void     gui_menu_hover_action(Menu *const menu, float x_pos, float y_pos);
void     gui_menu_scroll_action(Menu *const menu, bool direction, float x_pos, float y_pos);
void     gui_menu_click_action(Menu *const menu, float x_pos, float y_pos);
void     gui_menu_clear_entries(Menu *const menu);
void     gui_menu_set_static_width(Menu *const menu, float width);
void     gui_menu_set_tab_accept_behavior(Menu *const menu, bool accept_on_tab);
void     gui_menu_set_arrow_depth_navigation(Menu *const menu, bool enable_arrow_depth_navigation);
bool     gui_menu_owns_element(Menu *const menu, guielement *const e);
bool     gui_menu_element_is_main(Menu *const menu, guielement *const e);
bool     gui_menu_should_accept_on_tab(Menu *const menu);
bool     gui_menu_allows_arrow_navigation(Menu *const menu);
bool     gui_menu_is_ancestor(Menu *const menu, Menu *const ancestor);
bool     gui_menu_is_shown(Menu *const menu);
GuiFont *gui_menu_get_font(Menu *const menu);
int      gui_menu_len(Menu *const menu);

/* ----------------------------- Menu qsort callback's ----------------------------- */

int gui_menu_entry_qsort_strlen_cb(const void *a, const void *b);

/* ----------------------------- Menu qsort call ----------------------------- */

void gui_menu_qsort(Menu *const menu, CmpFuncPtr cmp_func);


/* ---------------------------------------------------------- gui/context_menu.cpp ---------------------------------------------------------- */


ContextMenu *context_menu_create(void);
void context_menu_free(ContextMenu *const cxm);
void context_menu_draw(ContextMenu *const cxm);
void context_menu_show(ContextMenu *const cxm, bool show);


#include <Mlib/def.h>
#include "c_proto.h"
