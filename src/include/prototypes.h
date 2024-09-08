/// @file definitions.h
#include "color.h"
#include "definitions.h"
#include "render.h"
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
extern unsigned long flags[1];

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

extern unsigned long from_x;
extern unsigned long till_x;

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

extern unsigned long wrap_at;

extern long stripe_column;
extern long tabsize;

extern regex_t quotereg;

extern int         currmenu;
extern keystruct  *sclist;
extern funcstruct *allfuncs;
extern funcstruct *exitfunc;

extern regex_t    search_regexp;
extern regmatch_t regmatches[10];

extern int           hilite_attribute;
extern int           interface_color_pair[NUMBER_OF_ELEMENTS];
extern char         *homedir;
extern char         *statedir;
extern char         *startup_problem;
extern char         *custom_nanorc;
extern char         *commandname;
extern bool          spotlighted;
extern unsigned long light_from_col;
extern unsigned long light_to_col;

extern bool last_key_was_bracket;

extern colortype *color_combo[NUMBER_OF_ELEMENTS];
extern keystruct *planted_shortcut;

extern colortype                 *last_c_color;
extern syntaxtype                *c_syntaxtype;
extern std::vector<bracket_pair>  bracket_pairs;
extern std::vector<bracket_entry> bracket_entrys;
extern std::vector<std::string>   syntax_structs;
extern std::vector<std::string>   syntax_classes;
extern std::vector<std::string>   syntax_vars;
extern std::vector<std::string>   syntax_funcs;
extern std::vector<std::string>   handled_includes;
extern std::vector<std::string>   handled_includes;

extern std::vector<function_info_t *> func_info;

extern task_queue_t          *task_queue;
extern pthread_t             *threads;
extern volatile sig_atomic_t *stop_thread_flags;
extern callback_queue_t      *callback_queue;
extern main_thread_t         *main_thread;

extern vec<char *>     includes;
extern vec<char *>     defines;
extern vec<char *>     structs;
extern vec<char *>     classes;
extern vec<char *>     funcs;
extern vec<glob_var_t> glob_vars;

typedef void (*functionptrtype)(void);

/* The two needed functions from 'browser.cpp'. */
void  browser_refresh(void);
char *browse_in(const char *inpath);
void  to_first_file(void);
void  to_last_file(void);

/* Most functions in 'chars.cpp'. */
void          utf8_init(void);
bool          using_utf8(void);
bool          is_alpha_char(const char *c);
bool          is_blank_char(const char *c);
bool          is_cntrl_char(const char *c);
bool          is_word_char(const char *c, bool allow_punct);
char          control_mbrep(const char *c, bool isdata);
int           mbtowide(wchar_t &wc, const char *c);
bool          is_doublewidth(const char *ch);
bool          is_zerowidth(const char *ch);
int           char_length(const char *const &pointer);
unsigned long mbstrlen(const char *pointer);
int           collect_char(const char *string, char *thechar);
int           advance_over(const char *string, unsigned long &column);
unsigned long step_left(const char *buf, unsigned long pos);
unsigned long step_right(const char *buf, unsigned long pos);
int           mbstrcasecmp(const char *s1, const char *s2);
int           mbstrncasecmp(const char *s1, const char *s2, unsigned long n);
char         *mbstrcasestr(const char *haystack, const char *needle);
char *revstrstr(const char *haystack, const char *needle, const char *pointer);
char *mbrevstrcasestr(const char *haystack, const char *needle,
                      const char *pointer);
char *mbstrchr(const char *string, const char *chr);
char *mbstrpbrk(const char *string, const char *accept);
char *mbrevstrpbrk(const char *head, const char *accept, const char *pointer);
bool  has_blank_char(const char *string);
bool  white_string(const char *string);
void  strip_leading_blanks_from(char *str);
void  strip_leading_chars_from(char *str, const char ch);

/* Most functions in 'color.cpp'. */
void  set_interface_colorpairs(void);
void  set_syntax_colorpairs(syntaxtype *sntx);
void  prepare_palette(void);
void  find_and_prime_applicable_syntax(void);
void  check_the_multis(linestruct *line);
void  precalc_multicolorinfo(void);
bool  str_equal_to_rgx(const char *str, const regex_t *rgx);
short rgb_to_ncurses(unsigned char value);

/* Most functions in 'cut.cpp'. */
void expunge(undo_type action);
void do_delete(void);
void do_backspace(void);
void chop_previous_word(void);
void chop_next_word(void);
void extract_segment(linestruct *top, unsigned long top_x, linestruct *bot,
                     unsigned long bot_x);
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
void   make_new_buffer(void);
bool   delete_lockfile(const char *lockfilename);
bool   open_buffer(const char *filename, bool new_one);
void   set_modified(void);
void   prepare_for_display(void);
void   mention_name_and_linecount(void);
void   switch_to_prev_buffer(void);
void   switch_to_next_buffer(void);
void   close_buffer(void);
void   read_file(FILE *f, int fd, const char *filename, bool undoable);
int    open_file(const char *filename, bool new_one, FILE **f);
char  *get_next_filename(const char *name, const char *suffix);
void   do_insertfile(void);
void   do_execute(void);
char  *get_full_path(const char *origpath);
char  *safe_tempfile(FILE **stream);
void   init_operating_dir(void);
bool   outside_of_confinement(const char *currpath, bool allow_tabcomp);
void   init_backup_dir(void);
int    copy_file(FILE *inn, FILE *out, bool close_out);
bool   write_file(const char *name, FILE *thefile, bool normal,
                  kind_of_writing_type method, bool annotate);
bool   write_region_to_file(const char *name, FILE *stream, bool normal,
                            kind_of_writing_type method);
int    write_it_out(bool exiting, bool withprompt);
void   do_writeout(void);
void   do_savefile(void);
char  *real_dir_from_tilde(const char *path);
int    diralphasort(const void *va, const void *vb);
char  *input_tab(char *buf, unsigned long *place, void (*refresh_func)(void),
                 bool *listed);
bool   is_file_and_exists(const char *path);
char **retrieve_lines_from_file(const char *path, unsigned long *nlines);
char **retrieve_words_from_file(const char *path, unsigned long *nwords);
char **words_from_file(const char *path, unsigned long *nwords);
char **words_from_current_file(unsigned long *nwords);
char **dir_entrys_from(const char *path);

/* Some functions in 'global.cpp'. */
functionptrtype  func_from_key(const int keycode);
functionptrtype  interpret(const int keycode);
const keystruct *first_sc_for(const int menu, void (*function)(void));
const keystruct *get_shortcut(const int keycode);
unsigned long    shown_entries_for(int menu);
int              keycode_from_string(const char *keystring);
void             shortcut_init(void);
const char      *epithet_of_flag(const unsigned int flag);
void             add_to_handled_includes_vec(const char *path);
bool             is_in_handled_includes_vec(std::string_view path);
bool             syntax_var(std::string_view str);
bool             syntax_func(std::string_view str);
void             new_syntax_var(const char *str);
void             new_syntax_func(const char *str);

/* Some functions in 'help.cpp'. */
void wrap_help_text_into_buffer(void);
void do_help(void);

/* Most functions in 'history.cpp'. */
void history_init(void);
void reset_history_pointer_for(const linestruct *list);
void update_history(linestruct **item, const char *text, bool avoid_duplicates);
char *get_history_completion(linestruct **h, char *s, unsigned long len);
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
linestruct *make_new_node(linestruct *prevnode);
linestruct *copy_buffer(const linestruct *src);
void        splice_node(linestruct *afterthis, linestruct *newnode);
void        unlink_node(linestruct *line);
void        delete_node(linestruct *line);
void        free_lines(linestruct *src);
void        renumber_from(linestruct *line);
void        print_view_warning(void);
bool        in_restricted_mode(void);
void        suggest_ctrlT_ctrlZ(void);
void        finish(void);
void        close_and_go(void);
void        do_exit(void);
void        die(STRING_VIEW msg, ...);
void        window_init(void);
void        install_handler_for_Ctrl_C(void);
void        restore_handler_for_Ctrl_C(void);
void        reconnect_and_store_state(void);
void        handle_hupterm(int signal);
void        handle_crash(int signal);
void        suspend_nano(int signal);
void        do_suspend(void);
void        continue_nano(int signal);
void        block_sigwinch(bool blockit);
void        handle_sigwinch(int signal);
void        regenerate_screen(void);
void        disable_kb_interrupt(void);
void        enable_kb_interrupt(void);
void        disable_flow_control(void);
void        enable_flow_control(void);
void        terminal_init(void);
void        confirm_margin(void);
void        unbound_key(int code);
bool        changes_something(functionptrtype f);
void        inject(char *burst, unsigned long count);

/* Most functions in 'prompt.cpp'. */
unsigned long get_statusbar_page_start(unsigned long base,
                                       unsigned long column);
void          put_cursor_at_end_of_answer(void);
void          add_or_remove_pipe_symbol_from_answer(void);
int do_prompt(int menu, const char *provided, linestruct **history_list,
              void (*refresh_func)(void), const char *msg, ...);
int ask_user(bool withall, const char *question);

/* Most functions in 'rcfile.cpp'. */
short color_to_short(const char *colorname, bool &vivid, bool &thick);
char *parse_next_word(char *ptr);
void  parse_rule(char *ptr, int rex_flags);
bool  compile(const char *expression, int rex_flags, regex_t **packed);
bool  compile_with_callback(const char *expression, int rex_flags,
                            regex_t **packed, const char *from_file);
void  begin_new_syntax(char *ptr);
bool  parse_combination(char *combotext, short *fg, short *bg, int *attributes);
void  set_interface_color(const unsigned char element, char *combotext);
void  display_rcfile_errors(void);
void  jot_error(const char *msg, ...);
keystruct *strtosc(const char *input);
void       parse_one_include(char *file, syntaxtype *syntax);
short      closest_index_color(short red, short green, short blue);
void       grab_and_store(const char *kind, char *ptr, regexlisttype **storage);
bool       parse_syntax_commands(const char *keyword, char *ptr);
void       parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only);
void       do_rcfiles(void);

/* Most functions in 'search.cpp'. */
bool  regexp_init(const char *regexp);
void  tidy_up_after_search(void);
int   findnextstr(const char *needle, bool whole_word_only, int modus,
                  unsigned long *match_len, bool skipone, const linestruct *begin,
                  unsigned long begin_x);
void  do_search_forward(void);
void  do_search_backward(void);
void  do_findprevious(void);
void  do_findnext(void);
void  not_found_msg(const char *str);
void  go_looking(void);
long  do_replace_loop(const char *needle, bool whole_word_only,
                      const linestruct *real_current,
                      unsigned long    *real_current_x);
void  do_replace(void);
void  ask_for_and_do_replacements(void);
void  goto_line_posx(long line, unsigned long pos_x);
void  goto_line_and_column(long line, long column, bool retain_answer,
                           bool interactive);
void  do_gotolinecolumn(void);
void  do_find_bracket(void);
void  put_or_lift_anchor(void);
void  to_prev_anchor(void);
void  to_next_anchor(void);
char *find_global_header(const char *str);
char *find_local_header(const char *str);

/* Most functions in 'text.cpp'. */
void          do_mark(void);
void          do_tab(void);
void          do_indent(void);
void          do_unindent(void);
void          do_comment(void);
void          do_undo(void);
void          do_redo(void);
void          do_enter(void);
void          discard_until(const undostruct *thisitem);
void          add_undo(undo_type action, const char *message);
void          update_multiline_undo(long lineno, char *indentation);
void          update_undo(undo_type action);
void          do_wrap(void);
long          break_line(const char *textstart, long goal, bool snap_at_nl);
unsigned long indent_length(const char *line);
unsigned long quote_length(const char *line);
bool          begpar(const linestruct *const line, int depth);
bool          inpar(const linestruct *const line);
void          do_justify(void);
void          do_full_justify(void);
void          do_spell(void);
void          do_linter(void);
void          do_formatter(void);
void          count_lines_words_and_characters(void);
void          do_verbatim_input(void);
void          complete_a_word(void);

/* All functions in 'utils.cpp' */
void          get_homedir(void);
char         *concatenate(const char *path, const char *name);
int           digits(long n);
bool          parseNum(std::string_view string, long &result);
bool          parse_line_column(const char *str, long *line, long *column);
void          recode_NUL_to_LF(char *string, unsigned long length);
unsigned long recode_LF_to_NUL(char *string);
void          free_chararray(char **array, unsigned long len);
bool          is_separate_word(unsigned long position, unsigned long length,
                               const char *buf);
void         *nmalloc(const unsigned long howmuch);
void         *nrealloc(void *ptr, const unsigned long howmuch);
char         *measured_copy(const char *string, unsigned long count);
char *measured_memmove_copy(const char *string, const unsigned long count);
template <unsigned long N>
char         *smart_move_copy_strltr(const char (&str)[N]);
char         *mallocstrcpy(char *dest, const char *src);
char         *copy_of(const char *string);
char         *memmove_copy_of(const char *string);
char         *free_and_assign(char *dest, char *src);
unsigned long get_page_start(unsigned long column);
unsigned long xplustabs(void);
unsigned long actual_x(const char *text, unsigned long column);
unsigned long wideness(const char *text, unsigned long maxlen);
unsigned long breadth(const char *text);
void          new_magicline(void);
void          remove_magicline(void);
bool          mark_is_before_cursor(void);
void get_region(linestruct **top, unsigned long *top_x, linestruct **bot,
                unsigned long *bot_x);
void get_range(linestruct **top, linestruct **bot);
unsigned long number_of_characters_in(const linestruct *begin,
                                      const linestruct *end);
const char   *strstrwrapper(const char *haystack, const char *needle,
                            const char *start);
const char   *tail(const char *path);
linestruct   *line_from_number(long number);
char         *alloced_pwd(void);
char         *alloc_str_free_substrs(char *str_1, char *str_2);
void          append_str(char **str, const char *appen_str);
char         *alloced_current_file_dir(void);
char         *alloced_full_current_file_dir(void);
unsigned long word_index(bool prev);

/* Most functions in 'winio.cpp'. */
void          record_macro(void);
void          run_macro(void);
void          reserve_space_for(unsigned long newsize);
unsigned long waiting_keycodes(void);
void          implant(const char *string);
int           get_input(WINDOW *win);
int           get_kbinput(WINDOW *win, bool showcursor);
char         *get_verbatim_kbinput(WINDOW *win, unsigned long *count);
int           get_mouseinput(int *mouse_y, int *mouse_x, bool allow_shortcuts);
void          blank_edit(void);
void          blank_statusbar(void);
void          wipe_statusbar(void);
void          blank_bottombars(void);
void          blank_it_when_expired(void);
void          set_blankdelay_to_one(void);
char *display_string(const char *buf, unsigned long column, unsigned long span,
                     bool isdata, bool isprompt);
void  titlebar(const char *path);
void  minibar(void);
void  statusline(message_type importance, const char *msg, ...);
void  statusbar(const char *msg);
void  warn_and_briefly_pause(const char *msg);
void  bottombars(int menu);
void  post_one_key(const char *keystroke, const char *tag, int width);
void  place_the_cursor(void);
void  draw_row(const int row, const char *converted, linestruct *line,
               const unsigned long from_col);
int   update_line(linestruct *line, unsigned long index, int offset = 0);
int   update_softwrapped_line(linestruct *line);
bool  line_needs_update(const unsigned long old_column,
                        const unsigned long new_column);
int   go_back_chunks(int nrows, linestruct **line, unsigned long *leftedge);
int   go_forward_chunks(int nrows, linestruct **line, unsigned long *leftedge);
bool  less_than_a_screenful(unsigned long was_lineno,
                            unsigned long was_leftedge);
void  edit_scroll(bool direction);
unsigned long get_softwrap_breakpoint(const char   *linedata,
                                      unsigned long leftedge, bool *kickoff,
                                      bool *end_of_line);
unsigned long get_chunk_and_edge(unsigned long column, linestruct *line,
                                 unsigned long *leftedge);
unsigned long chunk_for(unsigned long column, linestruct *line);
unsigned long leftedge_for(unsigned long column, linestruct *line);
unsigned long extra_chunks_in(linestruct *line);
void          ensure_firstcolumn_is_aligned(void);
unsigned long actual_last_column(unsigned long leftedge, unsigned long column);
void          edit_redraw(linestruct *old_current, update_type manner);
void          edit_refresh(void);
void          adjust_viewport(update_type manner);
void          full_refresh(void);
void          draw_all_subwindows(void);
void          report_cursor_position(void);
void          spotlight(unsigned long from_col, unsigned long to_col);
void spotlight_softwrapped(unsigned long from_col, unsigned long to_col);
void do_credits(void);

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
bool           isCppSyntaxChar(const char c);
void           get_line_indent(linestruct *line, unsigned short *tabs,
                               unsigned short *spaces, unsigned short *t_char,
                               unsigned short *t_tabs) __nonnull((1, 2, 3, 4, 5));
unsigned short indent_char_len(linestruct *line);
void           enclose_marked_region(const char *s1, const char *s2);
void           do_block_comment(void);
bool           enter_with_bracket(void);
void add_bracket_pair(const unsigned long start, const unsigned long end);
void all_brackets_pos(void);
void do_close_bracket(void);
void do_test_window(void);
function_info_t *parse_function(const char *str);
function_info_t *parse_func(const char *str);
function_info_t  parse_func_local_mem(const char *str);
void parse_variable(const char *sig, char **type, char **name, char **value);
void flag_all_brackets(void);
void find_current_function(linestruct *l);
void free_function_info(function_info_t *info, const int index = -1);

/* 'syntax.cpp' */
void   syntax_check_file(openfilestruct *file);
bool   parse_color_opts(const char *color_fg, const char *color_bg, short *fg,
                        short *bg, int *attr);
void   add_syntax_color(const char *color_fg, const char *color_bg,
                        const char *rgxstr, colortype **c,
                        const char *from_file = NULL);
void   add_start_end_syntax(const char *color_fg, const char *color_bg,
                            const char *start, const char *end, colortype **c);
bool   check_func_syntax(char ***words, unsigned long *i);
void   check_syntax(const char *path);
void   check_include_file_syntax(const char *path);
int    add_syntax(const unsigned short *type, char *word);
void   handle_include(char *str);
void   handle_define(char *str);
void   do_cpp_syntax(void);
void   check_for_syntax_words(linestruct *line);
void   update_c_syntaxtype(void);
void   add_syntax_word(const char *color_fg, const char *color_bg,
                       const char *word, const char *from_file = NULL);
void   set_last_c_colortype(void);
void   add_syntax_struct(const char *name);
void   add_syntax_class(const char *name);
bool   is_syntax_struct(std::string_view str);
bool   is_syntax_class(std::string_view str);
bool   define_exists(const char *str);
void   handle_struct_syntax(char **word);
void   openfile_syntax_c(void);
void   find_block_comments(int before, int end);
char **find_functions_in_file(char *path);
char **find_variabels_in_file(char *path);

/* 'netlog.cpp' */
void netlog_syntaxtype(syntaxtype *s);
void netlog_colortype(colortype *c);
void netlog_bracket_entry(const bracket_entry &be);
void netlog_func_info(function_info_t *info);

/* 'words.cpp' */
void        remove_tabs_from_word(char **word);
char      **words_in_line(linestruct *line);
char      **words_in_str(const char *str, unsigned long *size = NULL);
char      **delim_str(const char *str, const char *delim, unsigned long *size);
char      **split_into_words(const char *str, const unsigned int len,
                             unsigned int *word_count);
const char *extract_include(char *str);
char       *get_file_extention(void);
const char *rgx_word(const char *word);
bool        is_word_func(char *word, unsigned long *at);
void        remove_leading_char_type(char **word, const char c);
const char *concat_path(const char *s1, const char *s2);
bool        word_more_than_one_char_away(bool forward, unsigned long *nchars,
                                         const char ch);
bool        word_more_than_one_white_away(bool forward, unsigned long *nsteps);
bool        word_more_than_one_space_away(bool forward, unsigned long *nspaces);
bool        word_more_than_one_tab_away(bool forward, unsigned long *ntabs);
bool        prev_word_is_comment_start(unsigned long *nsteps);
bool        char_is_in_word(const char *word, const char ch, unsigned long *at);
char       *retrieve_word_from_cursor_pos(bool forward);
char      **fast_words_from_str(const char *str, unsigned long slen,
                                unsigned long *nwords);
line_word_t *line_word_list(const char *str, unsigned long slen);
line_word_t *make_line_word(char *str, unsigned short start, unsigned short len,
                            unsigned short end);
unsigned int last_strchr(const char *str, const char ch, unsigned int maxlen);
char        *memmove_concat(const char *s1, const char *s2);
const char  *substr(const char *str, unsigned long end_index);

/* 'lines.cpp' */
bool is_line_comment(linestruct *line);
bool is_line_start_end_bracket(linestruct *line, bool *is_start);
bool is_line_in_bracket_pair(const unsigned long lineno);
bool is_empty_line(linestruct *line);
void inject_in_line(linestruct **line, const char *str, unsigned long at);
unsigned long get_line_total_tabs(linestruct *line);
void          move_line(linestruct **line, bool up, bool refresh);
void          move_lines_up(void);
void          move_lines_down(void);
void erase_in_line(linestruct *line, unsigned long at, unsigned long len);
void select_line(linestruct *line, unsigned long from_col,
                 unsigned long to_col);
unsigned int total_tabs(linestruct *line);

/* 'brackets.cpp' */
void create_bracket_entry(unsigned long indent, unsigned long lineno,
                          bool is_start);

/* 'threadpool.cpp' */
void          lock_pthread_mutex(pthread_mutex_t *mutex, bool lock);
void          pause_all_sub_threads(bool pause);
void          init_queue_task(void);
int           task_queue_count(void);
void          shutdown_queue(void);
void          submit_task(task_functionptr_t function, void *arg, void **result,
                          callback_functionptr_t callback);
void          stop_thread(unsigned char thread_id);
unsigned char thread_id_from_pthread(pthread_t *thread);

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
void sub_thread_delete_c_syntax(char *word);
// void sub_thread_compile_add_rgx(const char *color_fg, const char *color_bg,
// const char *rgxstr,
//                                 colortype **last_c);
void sub_thread_find_syntax(const char *path);
void sub_thread_parse_funcs(const char *path);

/* 'signal.cpp' */
void init_main_thread(void);
void cleanup_main_thread(void);
void setup_signal_handler_on_sub_thread(void (*handler)(int));
void block_pthread_sig(int sig, bool block);

/* 'render.cpp' */
void render_line_text(const int row, const char *str, linestruct *line,
                      const unsigned long from_col);
void apply_syntax_to_line(const int row, const char *converted,
                          linestruct *line, unsigned long from_col);
void rendr_suggestion();
void cleanup_rendr(void);

/* 'render_utils.cpp' */
void  get_next_word(const char **start, const char **end);
void  clear_suggestion(void);
void  find_suggestion(void);
void  add_char_to_suggest_buf(void);
void  draw_suggest_win(void);
char *parse_function_sig(linestruct *line);
void  accept_suggestion(void);

/* 'gui.cpp' */
// void init_window(void);

#include <Mlib/def.h>
