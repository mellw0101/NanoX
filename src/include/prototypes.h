/// @file definitions.h
#include "definitions.h"

/* All external variables.  See global.c for their descriptions. */

extern volatile sig_atomic_t the_window_resized;

extern WINDOW *topwin;
extern WINDOW *midwin;
extern WINDOW *footwin;

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
extern size_t flags[1];

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
extern int editwinrows;
extern int editwincols;
extern int margin;
extern int sidebar;
extern int cycling_aim;

extern int *bardata;
extern int  whitelen[2];

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

extern size_t wrap_at;

extern long stripe_column;
extern long tabsize;

extern regex_t quotereg;

extern int         currmenu;
extern keystruct  *sclist;
extern funcstruct *allfuncs;
extern funcstruct *exitfunc;

extern regex_t    search_regexp;
extern regmatch_t regmatches[10];

extern int    hilite_attribute;
extern int    interface_color_pair[NUMBER_OF_ELEMENTS];
extern char  *homedir;
extern char  *statedir;
extern char  *startup_problem;
extern char  *custom_nanorc;
extern char  *commandname;
extern bool   spotlighted;
extern size_t light_from_col;
extern size_t light_to_col;

extern colortype *color_combo[NUMBER_OF_ELEMENTS];
extern keystruct *planted_shortcut;

typedef void (*functionptrtype)();

//
//  The two needed functions from 'browser.cpp'.
//
void  browser_refresh();
char *browse_in(const char *inpath);
void  to_first_file();
void  to_last_file();

//
//  Most functions in 'chars.cpp'.
//
void   utf8_init();
bool   using_utf8();
bool   is_alpha_char(const char *c);
bool   is_blank_char(const char *c);
bool   is_cntrl_char(const char *c);
bool   is_word_char(const char *c, bool allow_punct);
char   control_mbrep(const char *c, bool isdata);
int    mbtowide(wchar_t &wc, const char *c);
bool   is_doublewidth(const char *ch);
bool   is_zerowidth(const char *ch);
int    char_length(const char *const &pointer);
size_t mbstrlen(const char *pointer);
int    collect_char(const char *string, char *thechar);
int    advance_over(const char *string, size_t &column);
size_t step_left(const char *buf, size_t pos);
size_t step_right(const char *buf, size_t pos);
int    mbstrcasecmp(const char *s1, const char *s2);
int    mbstrncasecmp(const char *s1, const char *s2, size_t n);
char  *mbstrcasestr(const char *haystack, const char *needle);
char  *revstrstr(const char *haystack, const char *needle, const char *pointer);
char  *mbrevstrcasestr(const char *haystack, const char *needle, const char *pointer);
char  *mbstrchr(const char *string, const char *chr);
char  *mbstrpbrk(const char *string, const char *accept);
char  *mbrevstrpbrk(const char *head, const char *accept, const char *pointer);
bool   has_blank_char(const char *string);
bool   white_string(const char *string);
void   strip_leading_blanks_from(char *string);

//
//  Most functions in 'color.cpp'.
//
void set_interface_colorpairs();
void prepare_palette();
void find_and_prime_applicable_syntax();
void check_the_multis(linestruct *line);
void precalc_multicolorinfo();

//
//  Most functions in 'cut.cpp'.
//
void expunge(undo_type action);
void do_delete();
void do_backspace();
void chop_previous_word();
void chop_next_word();
void extract_segment(linestruct *top, size_t top_x, linestruct *bot, size_t bot_x);
void ingraft_buffer(linestruct *topline);
void copy_from_buffer(linestruct *somebuffer);
void cut_marked_region();
void do_snip(bool marked, bool until_eof, bool append);
void cut_text();
void cut_till_eof();
void zap_text();
void copy_marked_region();
void copy_text();
void paste_text();

//
//  Most functions in 'files.cpp'.
//
void  make_new_buffer();
bool  delete_lockfile(const char *lockfilename);
bool  open_buffer(const char *filename, bool new_one);
void  set_modified();
void  prepare_for_display();
void  mention_name_and_linecount();
void  switch_to_prev_buffer();
void  switch_to_next_buffer();
void  close_buffer();
void  read_file(FILE *f, int fd, const char *filename, bool undoable);
int   open_file(const char *filename, bool new_one, FILE **f);
char *get_next_filename(const char *name, const char *suffix);
void  do_insertfile();
void  do_execute();
char *get_full_path(const char *origpath);
char *safe_tempfile(FILE **stream);
void  init_operating_dir();
bool  outside_of_confinement(const char *currpath, bool allow_tabcomp);
void  init_backup_dir();
int   copy_file(FILE *inn, FILE *out, bool close_out);
bool  write_file(const char *name, FILE *thefile, bool normal, kind_of_writing_type method, bool annotate);
bool  write_region_to_file(const char *name, FILE *stream, bool normal, kind_of_writing_type method);
int   write_it_out(bool exiting, bool withprompt);
void  do_writeout();
void  do_savefile();
char *real_dir_from_tilde(const char *path);
int   diralphasort(const void *va, const void *vb);
char *input_tab(char *buf, size_t *place, void (*refresh_func)(), bool &listed);

//
//  Some functions in 'global.cpp'.
//
functionptrtype  func_from_key(const int keycode);
functionptrtype  interpret(const int keycode);
const keystruct *first_sc_for(int menu, void (*function)(void));
const keystruct *get_shortcut(const int keycode);

size_t      shown_entries_for(int menu);
int         keycode_from_string(const char *keystring);
void        shortcut_init();
const char *epithet_of_flag(const unsigned int flag);

//
//  Some functions in 'help.cpp'.
//
void wrap_help_text_into_buffer();
void do_help();

//
//  Most functions in 'history.cpp'.
//
void  history_init();
void  reset_history_pointer_for(const linestruct *list);
void  update_history(linestruct **item, const char *text, bool avoid_duplicates);
char *get_history_completion(linestruct **h, char *s, size_t len);
bool  have_statedir();
void  load_history();
void  save_history();
void  load_poshistory();
void  update_poshistory();
bool  has_old_position(const char *file, long *line, long *column);

//
//  Most functions in 'move.cpp'.
//
void to_first_line();
void to_last_line();
void do_page_up();
void do_page_down();
void to_top_row();
void to_bottom_row();
void do_cycle();
void do_center();
void do_para_begin(linestruct **line);
void do_para_end(linestruct **line);
void to_para_begin();
void to_para_end();
void to_prev_block();
void to_next_block();
void do_prev_word();
bool do_next_word(bool after_ends);
void to_prev_word();
void to_next_word();
void do_home();
void do_end();
void do_up();
void do_down();
void do_scroll_up();
void do_scroll_down();
void do_left();
void do_right();

//
//  Most functions in 'nano.cpp'.
//
linestruct *make_new_node(linestruct *prevnode);
linestruct *copy_buffer(const linestruct *src);
//
void splice_node(linestruct *afterthis, linestruct *newnode);
void unlink_node(linestruct *line);
void delete_node(linestruct *line);
void free_lines(linestruct *src);
void renumber_from(linestruct *line);
void print_view_warning();
bool in_restricted_mode();
void suggest_ctrlT_ctrlZ();
void finish();
void close_and_go();
void do_exit();
void die(STRING_VIEW msg, ...);
void window_init();
void install_handler_for_Ctrl_C();
void restore_handler_for_Ctrl_C();
void reconnect_and_store_state();
void handle_hupterm(int signal);
void handle_crash(int signal);
void suspend_nano(int signal);
void do_suspend();
void continue_nano(int signal);
void block_sigwinch(bool blockit);
void handle_sigwinch(int signal);
void regenerate_screen();
void disable_kb_interrupt();
void enable_kb_interrupt();
void disable_flow_control();
void enable_flow_control();
void terminal_init();
void confirm_margin();
void unbound_key(int code);
bool changes_something(CFuncPtr f);
void inject(char *burst, size_t count);

//
//  Most functions in 'prompt.cpp'.
//
size_t get_statusbar_page_start(size_t base, size_t column);
void   put_cursor_at_end_of_answer();
void   add_or_remove_pipe_symbol_from_answer();
int do_prompt(int menu, const char *provided, linestruct **history_list, void (*refresh_func)(), const char *msg, ...);
int ask_user(bool withall, const char *question);

//
//  Most functions in 'rcfile.cpp'.
//
void       set_interface_color(int element, char *combotext);
void       display_rcfile_errors();
void       jot_error(const char *msg, ...);
keystruct *strtosc(const char *input);
void       parse_one_include(char *file, syntaxtype *syntax);
void       grab_and_store(const char *kind, char *ptr, regexlisttype **storage);
bool       parse_syntax_commands(const char *keyword, char *ptr);
void       parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only);
void       do_rcfiles();

//
//  Most functions in 'search.cpp'.
//
bool regexp_init(const char *regexp);
void tidy_up_after_search();
int  findnextstr(const char *needle, bool whole_word_only, int modus, size_t *match_len, bool skipone,
                 const linestruct *begin, size_t begin_x);
void do_search_forward();
void do_search_backward();
void do_findprevious();
void do_findnext();
void not_found_msg(const char *str);
void go_looking();
long do_replace_loop(const char *needle, bool whole_word_only, const linestruct *real_current, size_t *real_current_x);
void do_replace();
void ask_for_and_do_replacements();
void goto_line_posx(long line, size_t pos_x);
void goto_line_and_column(long line, long column, bool retain_answer, bool interactive);
void do_gotolinecolumn();
void do_find_bracket();
void put_or_lift_anchor();
void to_prev_anchor();
void to_next_anchor();

//
//  Most functions in 'text.cpp'.
//
void   do_mark();
void   do_tab();
void   do_indent();
void   do_unindent();
void   do_comment();
void   do_undo();
void   do_redo();
void   do_enter();
void   discard_until(const undostruct *thisitem);
void   add_undo(undo_type action, const char *message);
void   update_multiline_undo(long lineno, char *indentation);
void   update_undo(undo_type action);
void   do_wrap();
long   break_line(const char *textstart, long goal, bool snap_at_nl);
size_t indent_length(const char *line);
size_t quote_length(const char *line);
bool   begpar(const linestruct *const line, int depth);
bool   inpar(const linestruct *const line);
void   do_justify();
void   do_full_justify();
void   do_spell();
void   do_linter();
void   do_formatter();
void   count_lines_words_and_characters();
void   do_verbatim_input();
void   complete_a_word();

//
//  All functions in utils.cpp
//
void   get_homedir();
char  *concatenate(const char *path, const char *name);
int    digits(long n);
bool   parseNum(std::string_view string, int64_t &result);
bool   parse_line_column(const char *str, long *line, long *column);
void   recode_NUL_to_LF(char *string, size_t length);
size_t recode_LF_to_NUL(char *string);
void   free_chararray(char **array, size_t len);
bool   is_separate_word(size_t position, size_t length, const char *buf);
void  *nmalloc(const size_t howmuch);
void  *nrealloc(void *ptr, const size_t howmuch);
char  *measured_copy(const char *string, size_t count);
char  *mallocstrcpy(char *dest, const char *src);
char  *copy_of(const char *string);
char  *free_and_assign(char *dest, char *src);
size_t get_page_start(size_t column);
size_t xplustabs();
size_t actual_x(const char *text, size_t column);
size_t wideness(const char *text, size_t maxlen);
size_t breadth(const char *text);
void   new_magicline();
void   remove_magicline();
bool   mark_is_before_cursor();
void   get_region(linestruct **top, unsigned long *top_x, linestruct **bot, unsigned long *bot_x);
void   get_range(linestruct **top, linestruct **bot);
size_t number_of_characters_in(const linestruct *begin, const linestruct *end);

const char *strstrwrapper(const char *haystack, const char *needle, const char *start);
const char *tail(const char *path);
linestruct *line_from_number(long number);

//
//  Most functions in 'winio.cpp'.
//
void   record_macro();
void   run_macro();
void   reserve_space_for(size_t newsize);
size_t waiting_keycodes();
void   implant(const char *string);
int    get_input(WINDOW *win);
int    get_kbinput(WINDOW *win, bool showcursor);
char  *get_verbatim_kbinput(WINDOW *win, size_t *count);
int    get_mouseinput(int &mouse_y, int &mouse_x, bool allow_shortcuts);
void   blank_edit();
void   blank_statusbar();
void   wipe_statusbar();
void   blank_bottombars();
void   blank_it_when_expired(void);
void   set_blankdelay_to_one(void);
char  *display_string(const char *buf, size_t column, size_t span, bool isdata, bool isprompt);
void   titlebar(const char *path);
void   minibar();
void   statusline(message_type importance, const char *msg, ...);
void   statusbar(const char *msg);
void   warn_and_briefly_pause(const char *msg);
void   bottombars(int menu);
void   post_one_key(const char *keystroke, const char *tag, int width);
void   place_the_cursor(void);
int    update_line(linestruct *line, size_t index);
int    update_softwrapped_line(linestruct *line);
bool   line_needs_update(const size_t old_column, const size_t new_column);
int    go_back_chunks(int nrows, linestruct **line, size_t *leftedge);
int    go_forward_chunks(int nrows, linestruct *&line, size_t &leftedge);
bool   less_than_a_screenful(size_t was_lineno, size_t was_leftedge);
void   edit_scroll(bool direction);
size_t get_softwrap_breakpoint(const char *linedata, size_t leftedge, bool &kickoff, bool &end_of_line);
size_t get_chunk_and_edge(size_t column, linestruct *line, size_t *leftedge);
size_t chunk_for(size_t column, linestruct *line);
size_t leftedge_for(size_t column, linestruct *line);
size_t extra_chunks_in(linestruct *line);
void   ensure_firstcolumn_is_aligned(void);
size_t actual_last_column(size_t leftedge, size_t column);
void   edit_redraw(linestruct *old_current, update_type manner);
void   edit_refresh(void);
void   adjust_viewport(update_type manner);
void   full_refresh(void);
void   draw_all_subwindows(void);
void   report_cursor_position(void);
void   spotlight(size_t from_col, size_t to_col);
void   spotlight_softwrapped(size_t from_col, size_t to_col);
void   do_credits(void);

//
//  These are just name definitions.
//
void case_sens_void();
void regexp_void();
void backwards_void();
void get_older_item();
void get_newer_item();
void flip_replace();
void flip_goto();
void to_files();
void goto_dir();
void do_nothing();
void do_toggle();
void dos_format();
void mac_format();
void append_it();
void prepend_it();
void back_it_up();
void flip_execute();
void flip_pipe();
void flip_convert();
void flip_newbuffer();
void discard_buffer();
void do_cancel();

//
//  All functions in 'cpp.cpp'.
//
bool isCppSyntaxChar(const char c);

#include <Mlib/def.h>
