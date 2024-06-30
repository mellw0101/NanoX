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
extern u64 flags[1];

extern s64 fill;

extern s32 didfind;
extern s32 controlleft, controlright;
extern s32 controlup, controldown;
extern s32 controlhome, controlend;
extern s32 controldelete, controlshiftdelete;
extern s32 shiftleft, shiftright;
extern s32 shiftup, shiftdown;
extern s32 shiftcontrolleft, shiftcontrolright;
extern s32 shiftcontrolup, shiftcontroldown;
extern s32 shiftcontrolhome, shiftcontrolend;
extern s32 altleft, altright;
extern s32 altup, altdown;
extern s32 althome, altend;
extern s32 altpageup, altpagedown;
extern s32 altinsert, altdelete;
extern s32 shiftaltleft, shiftaltright;
extern s32 shiftaltup, shiftaltdown;
extern s32 mousefocusin, mousefocusout;
extern s32 editwinrows;
extern s32 editwincols;
extern s32 margin;
extern s32 sidebar;
extern s32 cycling_aim;

extern s32 *bardata;
extern s32  whitelen[2];

extern s8 *title;
extern s8 *answer;
extern s8 *last_search;
extern s8 *present_path;
extern s8 *matchbrackets;
extern s8 *whitespace;
extern s8 *punct;
extern s8 *brackets;
extern s8 *quotestr;
extern s8 *word_chars;
extern s8 *backup_dir;
extern s8 *operating_dir;
extern s8 *alt_speller;
extern s8 *syntaxstr;

extern const s8 *exit_tag;
extern const s8 *close_tag;

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

extern u64 wrap_at;

extern s64 stripe_column;
extern s64 tabsize;

extern regex_t quotereg;

extern s32         currmenu;
extern keystruct  *sclist;
extern funcstruct *allfuncs;
extern funcstruct *exitfunc;

extern regex_t    search_regexp;
extern regmatch_t regmatches[10];

extern s32  hilite_attribute;
extern s32  interface_color_pair[NUMBER_OF_ELEMENTS];
extern s8  *homedir;
extern s8  *statedir;
extern s8  *startup_problem;
extern s8  *custom_nanorc;
extern s8  *commandname;
extern bool spotlighted;
extern u64  light_from_col;
extern u64  light_to_col;

extern colortype *color_combo[NUMBER_OF_ELEMENTS];
extern keystruct *planted_shortcut;

typedef void (*functionptrtype)();

//
//  The two needed functions from 'browser.cpp'.
//
void browser_refresh();
s8  *browse_in(const s8 *inpath);
void to_first_file();
void to_last_file();

//
//  Most functions in 'chars.cpp'.
//
void utf8_init();
bool using_utf8();
bool is_alpha_char(C_s8 *c);
bool is_blank_char(C_s8 *c);
bool is_cntrl_char(C_s8 *c);
bool is_word_char(C_s8 *c, bool allow_punct);
char control_mbrep(C_s8 *c, bool isdata);
s32  mbtowide(wchar_t &wc, C_s8 *c);
bool is_doublewidth(C_s8 *ch);
bool is_zerowidth(C_s8 *ch);
s32  char_length(C_s8 *const &pointer);
u64  mbstrlen(C_s8 *pointer);
s32  collect_char(C_s8 *string, s8 *thechar);
s32  advance_over(C_s8 *string, u64 &column);
u64  step_left(C_s8 *buf, u64 pos);
u64  step_right(C_s8 *buf, u64 pos);
s32  mbstrcasecmp(C_s8 *s1, C_s8 *s2);
s32  mbstrncasecmp(C_s8 *s1, C_s8 *s2, u64 n);
s8  *mbstrcasestr(C_s8 *haystack, C_s8 *needle);
s8  *revstrstr(C_s8 *haystack, C_s8 *needle, C_s8 *pointer);
s8  *mbrevstrcasestr(C_s8 *haystack, C_s8 *needle, C_s8 *pointer);
s8  *mbstrchr(C_s8 *string, C_s8 *chr);
s8  *mbstrpbrk(C_s8 *string, C_s8 *accept);
s8  *mbrevstrpbrk(C_s8 *head, C_s8 *accept, C_s8 *pointer);
bool has_blank_char(C_s8 *string);
bool white_string(C_s8 *string);
void strip_leading_blanks_from(s8 *string);

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
void extract_segment(linestruct *top, u64 top_x, linestruct *bot, u64 bot_x);
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
void  read_file(FILE *f, s32 fd, const s8 *filename, bool undoable);
int   open_file(const char *filename, bool new_one, FILE **f);
char *get_next_filename(const s8 *name, const s8 *suffix);
void  do_insertfile();
void  do_execute();
s8   *get_full_path(const s8 *origpath);
s8   *safe_tempfile(FILE **stream);
void  init_operating_dir(void);
bool  outside_of_confinement(const char *currpath, bool allow_tabcomp);
void  init_backup_dir();
s32   copy_file(FILE *inn, FILE *out, bool close_out);
bool  write_file(const char *name, FILE *thefile, bool normal, kind_of_writing_type method, bool annotate);
bool  write_region_to_file(const char *name, FILE *stream, bool normal, kind_of_writing_type method);
s32   write_it_out(bool exiting, bool withprompt);
void  do_writeout();
void  do_savefile();
s8   *real_dir_from_tilde(const s8 *path);
int   diralphasort(const void *va, const void *vb);
s8   *input_tab(s8 *buf, u64 *place, void (*refresh_func)(), bool &listed);

//
//  Some functions in 'global.cpp'.
//
functionptrtype  func_from_key(const s32 keycode);
functionptrtype  interpret(const s32 keycode);
const keystruct *first_sc_for(s32 menu, void (*function)(void));
const keystruct *get_shortcut(const s32 keycode);

u64   shown_entries_for(s32 menu);
s32   keycode_from_string(C_s8 *keystring);
void  shortcut_init();
C_s8 *epithet_of_flag(C_u32 flag);

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
void  update_history(linestruct **item, const s8 *text, bool avoid_duplicates);
char *get_history_completion(linestruct **h, char *s, u64 len);
bool  have_statedir();
void  load_history();
void  save_history();
void  load_poshistory();
void  update_poshistory();
bool  has_old_position(const s8 *file, s64 *line, s64 *column);

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
void handle_hupterm(s32 signal);
void handle_crash(s32 signal);
void suspend_nano(s32 signal);
void do_suspend();
void continue_nano(s32 signal);
void block_sigwinch(bool blockit);
void handle_sigwinch(s32 signal);
void regenerate_screen();
void disable_kb_interrupt();
void enable_kb_interrupt();
void disable_flow_control();
void enable_flow_control();
void terminal_init();
void confirm_margin();
void unbound_key(s32 code);
bool changes_something(CFuncPtr f);
void inject(s8 *burst, u64 count);

//
//  Most functions in 'prompt.cpp'.
//
u64  get_statusbar_page_start(u64 base, u64 column);
void put_cursor_at_end_of_answer();
void add_or_remove_pipe_symbol_from_answer();
s32  do_prompt(s32 menu, const s8 *provided, linestruct **history_list, void (*refresh_func)(), const s8 *msg, ...);
s32  ask_user(bool withall, const s8 *question);

//
//  Most functions in 'rcfile.cpp'.
//
void       set_interface_color(s32 element, s8 *combotext);
void       display_rcfile_errors();
void       jot_error(const s8 *msg, ...);
keystruct *strtosc(C_s8 *input);
void       parse_one_include(s8 *file, syntaxtype *syntax);
void       grab_and_store(const s8 *kind, s8 *ptr, regexlisttype **storage);
bool       parse_syntax_commands(C_s8 *keyword, s8 *ptr);
void       parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only);
void       do_rcfiles();

//
//  Most functions in 'search.cpp'.
//
bool regexp_init(const s8 *regexp);
void tidy_up_after_search();
s32  findnextstr(const s8 *needle, bool whole_word_only, s32 modus, u64 *match_len, bool skipone,
                 const linestruct *begin, u64 begin_x);
void do_search_forward();
void do_search_backward();
void do_findprevious();
void do_findnext();
void not_found_msg(const s8 *str);
void go_looking();
s64  do_replace_loop(const s8 *needle, bool whole_word_only, const linestruct *real_current, u64 *real_current_x);
void do_replace();
void ask_for_and_do_replacements();
void goto_line_posx(s64 line, u64 pos_x);
void goto_line_and_column(s64 line, s64 column, bool retain_answer, bool interactive);
void do_gotolinecolumn();
void do_find_bracket();
void put_or_lift_anchor();
void to_prev_anchor();
void to_next_anchor();

//
//  Most functions in 'text.cpp'.
//
void do_mark();
void do_tab();
void do_indent();
void do_unindent();
void do_comment();
void do_undo();
void do_redo();
void do_enter();
void discard_until(const undostruct *thisitem);
void add_undo(undo_type action, const s8 *message);
void update_multiline_undo(s64 lineno, s8 *indentation);
void update_undo(undo_type action);
void do_wrap();
s64  break_line(const s8 *textstart, s64 goal, bool snap_at_nl);
u64  indent_length(const s8 *line);
u64  quote_length(const s8 *line);
bool begpar(const linestruct *const line, s32 depth);
bool inpar(const linestruct *const line);
void do_justify();
void do_full_justify();
void do_spell();
void do_linter();
void do_formatter();
void count_lines_words_and_characters();
void do_verbatim_input();
void complete_a_word();

//
//  All functions in utils.cpp
//
void  get_homedir();
s8   *concatenate(const s8 *path, const s8 *name);
int   digits(s64 n);
bool  parseNum(std::string_view string, int64_t &result);
bool  parse_line_column(const s8 *str, s64 *line, s64 *column);
void  recode_NUL_to_LF(s8 *string, u64 length);
u64   recode_LF_to_NUL(s8 *string);
void  free_chararray(char **array, u64 len);
bool  is_separate_word(u64 position, u64 length, const s8 *buf);
void *nmalloc(const u64 howmuch);
void *nrealloc(void *ptr, const u64 howmuch);
s8   *measured_copy(const s8 *string, u64 count);
s8   *mallocstrcpy(s8 *dest, const s8 *src);
s8   *copy_of(const s8 *string);
s8   *free_and_assign(s8 *dest, s8 *src);
u64   get_page_start(u64 column);
u64   xplustabs();
u64   actual_x(const s8 *text, u64 column);
u64   wideness(const s8 *text, u64 maxlen);
u64   breadth(const s8 *text);
void  new_magicline();
void  remove_magicline();
bool  mark_is_before_cursor();
void  get_region(linestruct *&top, u64 &top_x, linestruct *&bot, u64 &bot_x);
void  get_range(linestruct *&top, linestruct *&bot);
u64   number_of_characters_in(const linestruct *begin, const linestruct *end);

const s8   *strstrwrapper(const s8 *haystack, const s8 *needle, const s8 *start);
const s8   *tail(const s8 *path);
linestruct *line_from_number(s64 number);

//
//  Most functions in 'winio.cpp'.
//
void record_macro();
void run_macro();
void reserve_space_for(size_t newsize);
u64  waiting_keycodes();
void implant(const s8 *string);
int  get_input(WINDOW *win);
int  get_kbinput(WINDOW *win, bool showcursor);
s8  *get_verbatim_kbinput(WINDOW *win, size_t *count);
s32  get_mouseinput(s32 &mouse_y, s32 &mouse_x, bool allow_shortcuts);
void blank_edit();
void blank_statusbar();
void wipe_statusbar();
void blank_bottombars();
void blank_it_when_expired(void);
void set_blankdelay_to_one(void);
s8  *display_string(const s8 *buf, u64 column, u64 span, bool isdata, bool isprompt);
void titlebar(const s8 *path);
void minibar();
void statusline(message_type importance, const s8 *msg, ...);
void statusbar(const s8 *msg);
void warn_and_briefly_pause(const s8 *msg);
void bottombars(s32 menu);
void post_one_key(const s8 *keystroke, const s8 *tag, int width);
void place_the_cursor(void);
int  update_line(linestruct *line, size_t index);
int  update_softwrapped_line(linestruct *line);
bool line_needs_update(const size_t old_column, const size_t new_column);
int  go_back_chunks(int nrows, linestruct **line, size_t *leftedge);
s32  go_forward_chunks(s32 nrows, linestruct *&line, u64 &leftedge);
bool less_than_a_screenful(size_t was_lineno, size_t was_leftedge);
void edit_scroll(bool direction);
u64  get_softwrap_breakpoint(const s8 *linedata, size_t leftedge, bool &kickoff, bool &end_of_line);
u64  get_chunk_and_edge(size_t column, linestruct *line, size_t *leftedge);
u64  chunk_for(size_t column, linestruct *line);
u64  leftedge_for(size_t column, linestruct *line);
u64  extra_chunks_in(linestruct *line);
void ensure_firstcolumn_is_aligned(void);
u64  actual_last_column(size_t leftedge, size_t column);
void edit_redraw(linestruct *old_current, update_type manner);
void edit_refresh(void);
void adjust_viewport(update_type manner);
void full_refresh(void);
void draw_all_subwindows(void);
void report_cursor_position(void);
void spotlight(size_t from_col, size_t to_col);
void spotlight_softwrapped(size_t from_col, size_t to_col);
void do_credits(void);

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
bool isCppSyntaxChar(const s8 c);

#include <Mlib/def.h>
