/// @file definitions.h
#pragma once

#include "color.h"
#include "definitions.h"
#include "language_server/language_server.h"
#include "render.h"
#include "rendr/rendr.h"
#include "task_types.h"

/* All external variables.  See global.c for their descriptions. */

extern volatile sig_atomic_t the_window_resized;

extern WINDOW *topwin;
extern WINDOW *midwin;
extern WINDOW *footwin;
extern WINDOW *suggestwin;

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

extern bool              suggest_on;
extern char              suggest_buf[1024];
extern char             *suggest_str;
extern int               suggest_len;
extern vec<const char *> types;

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
extern bool gui_enabled;

extern colortype *color_combo[NUMBER_OF_ELEMENTS];
extern keystruct *planted_shortcut;

extern bit_flag_t<8> mod_key;

extern task_queue_t          *task_queue;
extern pthread_t             *threads;
extern volatile sig_atomic_t *stop_thread_flags;
extern callback_queue_t      *callback_queue;
extern main_thread_t         *main_thread;

extern unordered_map<string, syntax_data_t> test_map;

typedef void (*functionptrtype)(void);

/* Asm functions. */
ASM_FUNCTION(int) SSE_strlen(const char *str);

/* The two needed functions from 'browser.cpp'. */
void  browser_refresh(void);
char *browse_in(const char *inpath);
void  to_first_file(void);
void  to_last_file(void);

/* Most functions in 'chars.cpp'. */
void  utf8_init(void);
bool  using_utf8(void);
bool  is_alpha_char(const char *c);
bool  is_blank_char(const char *c);
bool  is_cntrl_char(const char *c);
bool  is_word_char(const char *c, bool allow_punct);
char  control_mbrep(const char *c, bool isdata);
int   mbtowide(wchar_t &wc, const char *c);
bool  is_doublewidth(const char *ch);
bool  is_zerowidth(const char *ch);
int   char_length(const char *const &pointer);
Ulong mbstrlen(const char *pointer);
int   collect_char(const char *string, char *thechar);
int   advance_over(const char *string, Ulong &column);
Ulong step_left(const char *buf, Ulong pos);
Ulong step_right(const char *buf, Ulong pos);
int   mbstrcasecmp(const char *s1, const char *s2);
int   mbstrncasecmp(const char *s1, const char *s2, Ulong n);
char *mbstrcasestr(const char *haystack, const char *needle);
char *revstrstr(const char *haystack, const char *needle, const char *pointer);
char *mbrevstrcasestr(const char *haystack, const char *needle, const char *pointer);
char *mbstrchr(const char *string, const char *chr);
char *mbstrpbrk(const char *string, const char *accept);
char *mbrevstrpbrk(const char *head, const char *accept, const char *pointer);
bool  has_blank_char(const char *string);
bool  white_string(const char *string);
void  strip_leading_blanks_from(char *str);
void  strip_leading_chars_from(char *str, const char ch);

const char *__warn_unused nstrchr_ccpp(const char *__s __no_null(), const char __c) noexcept;

/* Most functions in 'color.cpp'. */
void  set_interface_colorpairs(void);
void  set_syntax_colorpairs(syntaxtype *sntx);
void  prepare_palette(void);
void  find_and_prime_applicable_syntax(void);
void  check_the_multis(linestruct *line);
void  precalc_multicolorinfo(void);
bool  str_equal_to_rgx(const char *str, const regex_t *rgx);
short rgb_to_ncurses(Uchar value);

/* Most functions in 'cut.cpp'. */
void expunge(undo_type action);
void do_delete(void);
void do_backspace(void);
void chop_previous_word(void);
void chop_next_word(void);
void extract_segment(linestruct *top, Ulong top_x, linestruct *bot, Ulong bot_x);
void ingraft_buffer(linestruct *topline);
void copy_from_buffer(linestruct *somebuffer);
void cut_marked_region(void);
void do_snip(bool marked, bool until_eof, bool append);
void cut_text(void);
void cut_till_eof(void);
void zap_text(void);
void copy_marked_region(void);
void copy_text(void);
void paste_text(void);

/* Most functions in 'files.cpp'. */
void        make_new_buffer(void);
bool        delete_lockfile(const char *lockfilename);
bool        has_valid_path(const char *filename);
bool        open_buffer(const char *filename, bool new_one);
void        open_buffer_browser(void);
void        open_new_empty_buffer(void);
void        set_modified(void);
void        prepare_for_display(void);
void        mention_name_and_linecount(void);
void        switch_to_prev_buffer(void);
void        switch_to_next_buffer(void);
void        close_buffer(void);
char       *encode_data(char *text, Ulong length);
void        read_file(FILE *f, int fd, const char *filename, bool undoable);
int         open_file(const char *filename, bool new_one, FILE **f);
char       *get_next_filename(const char *name, const char *suffix);
void        do_insertfile(void);
void        do_execute(void);
char       *get_full_path(const char *origpath);
char       *safe_tempfile(FILE **stream);
void        init_operating_dir(void);
bool        outside_of_confinement(const char *currpath, bool allow_tabcomp);
void        init_backup_dir(void);
int         copy_file(FILE *inn, FILE *out, bool close_out);
bool        write_file(const char *name, FILE *thefile, bool normal, kind_of_writing_type method, bool annotate);
bool        write_region_to_file(const char *name, FILE *stream, bool normal, kind_of_writing_type method);
int         write_it_out(bool exiting, bool withprompt);
void        do_writeout(void);
void        do_savefile(void);
char       *real_dir_from_tilde(const char *path);
int         diralphasort(const void *va, const void *vb);
bool        is_dir(const char *const path);
char       *input_tab(char *buf, Ulong *place, void (*refresh_func)(void), bool *listed);
bool        is_file_and_exists(const char *path);
char      **retrieve_lines_from_file(const char *path, Ulong *nlines);
char      **retrieve_words_from_file(const char *path, Ulong *nwords);
char      **words_from_file(const char *path, Ulong *nwords);
char      **words_from_current_file(Ulong *nwords);
char      **dir_entrys_from(const char *path);
linestruct *retrieve_file_as_lines(const string &path);
string      openfile_absolute_path(void);

/* Some functions in 'global.cpp'. */
functionptrtype  func_from_key(const int keycode);
functionptrtype  interpret(const int keycode);
const keystruct *first_sc_for(const int menu, void (*function)(void));
const keystruct *get_shortcut(const int keycode);
Ulong            shown_entries_for(int menu);
int              keycode_from_string(const char *keystring);
void             shortcut_init(void);
const char      *epithet_of_flag(const Uint flag);
void             add_to_handled_includes_vec(const char *path);
bool             is_in_handled_includes_vec(std::string_view path);
bool             syntax_var(std::string_view str);
void             new_syntax_var(const char *str);
void             new_syntax_func(const char *str);

/* Some functions in 'help.cpp'. */
void wrap_help_text_into_buffer(void);
void do_help(void);

/* Most functions in 'history.cpp'. */
void  history_init(void);
void  reset_history_pointer_for(const linestruct *list);
void  update_history(linestruct **item, const char *text, bool avoid_duplicates);
char *get_history_completion(linestruct **h, char *s, Ulong len);
bool  have_statedir(void);
void  load_history(void);
void  save_history(void);
void  load_poshistory(void);
void  update_poshistory(void);
bool  has_old_position(const char *file, long *line, long *column);

/* Most functions in 'move.cpp'. */
void to_first_line(void);
void to_last_line(void);
void do_page_up(void);
void do_page_down(void);
void to_top_row(void);
void to_bottom_row(void);
void do_cycle(void);
void do_center(void);
void do_para_begin(linestruct **line);
void do_para_end(linestruct **line);
void to_para_begin(void);
void to_para_end(void);
void to_prev_block(void);
void to_next_block(void);
void do_prev_word(void);
bool do_next_word(bool after_ends);
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
linestruct      *make_new_node(linestruct *prevnode);
linestruct      *copy_buffer(const linestruct *src);
void             splice_node(linestruct *afterthis, linestruct *newnode);
void             unlink_node(linestruct *line);
void             delete_node(linestruct *line);
void             free_lines(linestruct *src);
void             renumber_from(linestruct *line);
void             print_view_warning(void);
bool             in_restricted_mode(void);
void             suggest_ctrlT_ctrlZ(void);
void             finish(void);
void             close_and_go(void);
void             do_exit(void);
void __no_return die(const char *msg, ...);
void             window_init(void);
void             install_handler_for_Ctrl_C(void);
void             restore_handler_for_Ctrl_C(void);
void             reconnect_and_store_state(void);
void             handle_hupterm(int signal);
void             handle_crash(int signal);
void             suspend_nano(int signal);
void             do_suspend(void);
void             continue_nano(int signal);
void             block_sigwinch(bool blockit);
void             handle_sigwinch(int signal);
void             regenerate_screen(void);
void             disable_kb_interrupt(void);
void             enable_kb_interrupt(void);
void             disable_flow_control(void);
void             enable_flow_control(void);
void             terminal_init(void);
void             confirm_margin(void);
void             unbound_key(int code);
bool             changes_something(functionptrtype f);
void             inject(char *burst, Ulong count);

/* Most functions in 'prompt.cpp'. */
Ulong get_statusbar_page_start(Ulong base, Ulong column);
void  put_cursor_at_end_of_answer(void);
void  add_or_remove_pipe_symbol_from_answer(void);
int   do_prompt(int menu, const char *provided, linestruct **history_list, void (*refresh_func)(void), const char *msg,
                ...);
int   ask_user(bool withall, const char *question);

/* Most functions in 'rcfile.cpp'. */
short      color_to_short(const char *colorname, bool &vivid, bool &thick);
char      *parse_next_word(char *ptr);
void       parse_rule(char *ptr, int rex_flags);
bool       compile(const char *expression, int rex_flags, regex_t **packed);
void       begin_new_syntax(char *ptr);
bool       parse_combination(char *combotext, short *fg, short *bg, int *attributes);
void       set_interface_color(const u_char element, char *combotext);
void       display_rcfile_errors(void);
void       jot_error(const char *msg, ...);
keystruct *strtosc(const char *input);
bool       is_good_file(char *file);
void       parse_one_include(char *file, syntaxtype *syntax);
short      closest_index_color(short red, short green, short blue);
void       grab_and_store(const char *kind, char *ptr, regexlisttype **storage);
bool       parse_syntax_commands(const char *keyword, char *ptr);
void       parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only);
void       do_rcfiles(void);

/* Most functions in 'search.cpp'. */
bool  regexp_init(const char *regexp);
void  tidy_up_after_search(void);
int   findnextstr(const char *needle, bool whole_word_only, int modus, Ulong *match_len, bool skipone,
                  const linestruct *begin, Ulong begin_x);
void  do_search_forward(void);
void  do_search_backward(void);
void  do_findprevious(void);
void  do_findnext(void);
void  not_found_msg(const char *str);
void  go_looking(void);
long  do_replace_loop(const char *needle, bool whole_word_only, const linestruct *real_current, Ulong *real_current_x);
void  do_replace(void);
void  ask_for_and_do_replacements(void);
void  goto_line_posx(long line, Ulong pos_x);
void  goto_line_and_column(long line, long column, bool retain_answer, bool interactive);
void  do_gotolinecolumn(void);
bool  find_a_bracket(bool reverse, const char *bracket_pair);
void  do_find_bracket(void);
void  put_or_lift_anchor(void);
void  to_prev_anchor(void);
void  to_next_anchor(void);
char *find_global_header(const char *str);
char *find_local_header(const char *str);

/* Most functions in 'text.cpp'. */
void  do_mark(void);
void  do_tab(void);
void  do_indent(void);
void  do_unindent(void);
void  do_comment(void);
void  do_undo(void);
void  do_redo(void);
void  do_enter(void);
void  discard_until(const undostruct *thisitem);
void  add_undo(undo_type action, const char *message);
void  update_multiline_undo(long lineno, char *indentation);
void  update_undo(undo_type action);
void  do_wrap(void);
long  break_line(const char *textstart, long goal, bool snap_at_nl);
Ulong indent_length(const char *line);
Ulong quote_length(const char *line);
bool  begpar(const linestruct *const line, int depth);
bool  inpar(const linestruct *const line);
void  do_justify(void);
void  do_full_justify(void);
void  do_spell(void);
void  do_linter(void);
void  do_formatter(void);
void  count_lines_words_and_characters(void);
void  do_verbatim_input(void);
void  complete_a_word(void);
char *lower_case_word(const char *str);

/* All functions in 'utils.cpp' */
void        get_homedir(void);
const char *tail(const char *path);
const char *ext(const char *path);
char       *concatenate(const char *path, const char *name);
int         digits(long n);
bool        parse_num(const char *string, long *result);
bool        parse_line_column(const char *str, long *line, long *column);
void        recode_NUL_to_LF(char *string, Ulong length);
Ulong       recode_LF_to_NUL(char *string);
void        free_chararray(char **array, Ulong len);
bool        is_separate_word(Ulong position, Ulong length, const char *buf);
void       *nmalloc(Ulong howmuch);
void       *nrealloc(void *ptr, Ulong howmuch);
#define     arealloc(ptr, howmuch) (decltype(ptr))nrealloc(ptr, howmuch)
char       *measured_copy(const char *string, Ulong count);
char       *measured_memmove_copy(const char *string, const Ulong count);
char       *mallocstrcpy(char *dest, const char *src);
char       *copy_of(const char *string);
char       *memmove_copy_of(const char *string);
char       *free_and_assign(char *dest, char *src);
Ulong       get_page_start(Ulong column);
Ulong       xplustabs(void);
Ulong       actual_x(const char *text, Ulong column);
Ulong       wideness(const char *text, Ulong maxlen);
Ulong       breadth(const char *text);
void        new_magicline(void);
void        remove_magicline(void);
bool        mark_is_before_cursor(void);
void        get_region(linestruct **top, Ulong *top_x, linestruct **bot, Ulong *bot_x);
void        get_range(linestruct **top, linestruct **bot);
Ulong       number_of_characters_in(const linestruct *begin, const linestruct *end);
const char *strstrwrapper(const char *haystack, const char *needle, const char *start);
linestruct *line_from_number(long number);
char       *alloced_pwd(void);
char       *alloc_str_free_substrs(char *str_1, char *str_2);
void        append_str(char **str, const char *appen_str);
char       *alloced_current_file_dir(void);
char       *alloced_full_current_file_dir(void);
Ulong       word_index(bool prev);
void        alloced_remove_at(char **str, int at);
const char *word_strstr(const char *data, const char *needle);
string      file_extention_str(void);
string      current_file_dir(void);
char      **retrieve_exec_output(const char *cmd, Uint *n_lines);
const char *word_strstr_array(const char *str, const char **substrs, Uint count, Uint *index);
const char *strstr_array(const char *str, const char **substrs, Uint count, Uint *index);
const char *string_strstr_array(const char *str, const vector<string> &substrs, Uint *index);

template <typename T>
T *anrealloc(T *ptr, Ulong howmush) {
  T *data = (T *)realloc(ptr, howmush);
  if (!data) {
    die(_("Failed to reallocate ptr."));
  }
  return data;
}

/* Most functions in 'winio.cpp'. */
void  record_macro(void);
void  run_macro(void);
void  reserve_space_for(Ulong newsize);
Ulong waiting_keycodes(void);
void  implant(const char *string);
int   get_input(WINDOW *win);
int   get_kbinput(WINDOW *win, bool showcursor);
char *get_verbatim_kbinput(WINDOW *win, Ulong *count);
int   get_mouseinput(int *mouse_y, int *mouse_x, bool allow_shortcuts);
void  blank_edit(void);
void  blank_statusbar(void);
void  wipe_statusbar(void);
void  blank_bottombars(void);
void  blank_it_when_expired(void);
void  set_blankdelay_to_one(void);
char *display_string(const char *buf, Ulong column, Ulong span, bool isdata, bool isprompt);
void  titlebar(const char *path);
void  minibar(void);
void  statusline(message_type importance, const char *msg, ...);
void  statusbar(const char *msg);
void  warn_and_briefly_pause(const char *msg);
void  bottombars(int menu);
void  post_one_key(const char *keystroke, const char *tag, int width);
void  place_the_cursor(void);
void  draw_row(const int row, const char *converted, linestruct *line, const Ulong from_col);
int   update_line(linestruct *line, Ulong index, int offset = 0);
int   update_softwrapped_line(linestruct *line);
bool  line_needs_update(const Ulong old_column, const Ulong new_column);
int   go_back_chunks(int nrows, linestruct **line, Ulong *leftedge);
int   go_forward_chunks(int nrows, linestruct **line, Ulong *leftedge);
bool  less_than_a_screenful(Ulong was_lineno, Ulong was_leftedge);
void  edit_scroll(bool direction);
Ulong get_softwrap_breakpoint(const char *linedata, Ulong leftedge, bool *kickoff, bool *end_of_line);
Ulong get_chunk_and_edge(Ulong column, linestruct *line, Ulong *leftedge);
Ulong chunk_for(Ulong column, linestruct *line);
Ulong leftedge_for(Ulong column, linestruct *line);
Ulong extra_chunks_in(linestruct *line);
void  ensure_firstcolumn_is_aligned(void);
Ulong actual_last_column(Ulong leftedge, Ulong column);
void  edit_redraw(linestruct *old_current, update_type manner);
void  edit_refresh(void);
void  adjust_viewport(update_type manner);
void  full_refresh(void);
void  draw_all_subwindows(void);
void  report_cursor_position(void);
void  spotlight(Ulong from_col, Ulong to_col);
void  spotlight_softwrapped(Ulong from_col, Ulong to_col);
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
void   get_line_indent(linestruct *line, Ushort *tabs, Ushort *spaces, Ushort *t_char, Ushort *t_tabs) __nonnull((1, 2, 3, 4, 5));
Ushort indent_char_len(linestruct *line);
void   enclose_marked_region(const char *s1, const char *s2);
void   do_block_comment(void);
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
string get_word_after(const char *data, const char *word);
void   syntax_check_file(openfilestruct *file);
bool   parse_color_opts(const char *color_fg, const char *color_bg, short *fg, short *bg, int *attr);
void   do_syntax(void);
void   handle_struct_syntax(char **word);
void   find_block_comments(int before, int end);
char **find_functions_in_file(char *path);
char **find_variabels_in_file(char *path);

/* 'netlog.cpp' */
void netlog_syntaxtype(syntaxtype *s);
void netlog_colortype(colortype *c);
void netlog_func_info(function_info_t *info);
void debug_define(const DefineEntry &de);

/* 'words.cpp' */
void         remove_tabs_from_word(char **word);
char       **words_in_line(linestruct *line);
char       **words_in_str(const char *str, Ulong *size = NULL);
char       **delim_str(const char *str, const char *delim, Ulong *size);
char       **split_into_words(const char *str, const u_int len, u_int *word_count);
const char  *extract_include(char *str);
char        *get_file_extention(void);
const char  *rgx_word(const char *word);
bool         is_word_func(char *word, Ulong *at);
void         remove_leading_char_type(char **word, const char c);
const char  *concat_path(const char *s1, const char *s2);
bool         word_more_than_one_char_away(bool forward, Ulong *nchars, const char ch);
bool         word_more_than_one_white_away(bool forward, Ulong *nsteps);
bool         word_more_than_one_space_away(bool forward, Ulong *nspaces);
bool         word_more_than_one_tab_away(bool forward, Ulong *ntabs);
bool         prev_word_is_comment_start(Ulong *nsteps);
bool         char_is_in_word(const char *word, const char ch, Ulong *at);
char        *retrieve_word_from_cursor_pos(bool forward);
char       **fast_words_from_str(const char *str, Ulong slen, Ulong *nwords);
line_word_t *line_word_list(const char *str, Ulong slen);
line_word_t *line_word_list_is_word_char(const char *str, Ulong slen);
line_word_t *make_line_word(char *str, u_short start, u_short len, u_short end);
Uint         last_strchr(const char *str, const char ch, Uint maxlen);
char        *memmove_concat(const char *s1, const char *s2);
const char  *substr(const char *str, Ulong end_index);

/* 'lines.cpp' */
bool  is_line_comment(linestruct *line);
bool  is_line_start_end_bracket(linestruct *line, bool *is_start);
bool  is_line_in_bracket_pair(const Ulong lineno);
bool  is_empty_line(linestruct *line);
void  inject_in_line(linestruct **line, const char *str, Ulong at);
Ulong get_line_total_tabs(linestruct *line);
void  move_line(linestruct **line, bool up, bool refresh);
void  move_lines_up(void);
void  move_lines_down(void);
void  erase_in_line(linestruct *line, Ulong at, Ulong len);
void  select_line(linestruct *line, Ulong from_col, Ulong to_col);
Uint  total_tabs(linestruct *line);
int   get_editwin_row(linestruct *line);

/* 'threadpool.cpp' */
void  lock_pthread_mutex(pthread_mutex_t *mutex, bool lock);
void  pause_all_sub_threads(bool pause);
void  init_queue_task(void);
int   task_queue_count(void);
void  shutdown_queue(void);
void  submit_task(task_functionptr_t function, void *arg, void **result, callback_functionptr_t callback);
void  stop_thread(u_char thread_id);
Uchar thread_id_from_pthread(pthread_t *thread);

/* 'event.cpp' */
bool is_main_thread(void);
void send_SIGUSR1_to_main_thread(void);
void send_signal_to_main_thread(callback_functionptr_t func, void *arg);
void init_event_handler(void);
void enqueue_callback(callback_functionptr_t callback, void *result);
void prosses_callback_queue(void);
void cleanup_event_handler(void);

/* 'tasks.cpp' */
void submit_search_task(const char *path);
void submit_find_in_dir(const char *file, const char *in_dir);
void sub_thread_find_syntax(const char *path);
void sub_thread_parse_funcs(const char *path);
void find_functions_task(const char *path);
void find_glob_vars_task(const char *path);
void get_line_list_task(const char *path);

/* 'signal.cpp' */
void init_main_thread(void);
void cleanup_main_thread(void);
void setup_signal_handler_on_sub_thread(void (*handler)(int));
void block_pthread_sig(int sig, bool block);

/* 'render.cpp' */
void render_line_text(int row, const char *str, linestruct *line, Ulong from_col);
void apply_syntax_to_line(const int row, const char *converted, linestruct *line, Ulong from_col);
void rendr_suggestion(void);

/* 'render_utils.cpp' */
void        get_next_word(const char **start, const char **end);
void        clear_suggestion(void);
void        find_suggestion(void);
void        add_char_to_suggest_buf(void);
void        draw_suggest_win(void);
char       *parse_function_sig(linestruct *line);
void        accept_suggestion(void);
void        find_word(linestruct *line, const char *data, const char *word, const Ulong slen, const char **start,
                      const char **end);
int         preprossesor_data_from_key(const char *key);
void        free_local_var(local_var_t *var);
local_var_t parse_local_var(linestruct *line);
int         find_class_end_line(linestruct *from);
void        add_rm_color_map(string str, syntax_data_t data);

/* 'parse.cpp' */
void parse_class_data(linestruct *from);
void parse_var_type(const char *data);
void line_variable(linestruct *line, vector<var_t> &var_vector);
void func_decl(linestruct *line);

/* 'backets.cpp' */
bool  find_matching_bracket(linestruct *start_line, Ulong start_index, linestruct **to_line, Ulong *to_index);
bool  find_end_bracket(linestruct *from, Ulong index, linestruct **end, Ulong *end_index);
char *fetch_bracket_body(linestruct *from, Ulong index);

/* 'gui.cpp' */
// void init_window(void);
// int run_gui(void) noexcept;

/* 'cfg/cfg_file.cpp'. */
void init_cfg_file(void);

#include <Mlib/def.h>
#include "c_proto.h"
