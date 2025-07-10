/** @file global.c

  @author  Melwin Svensson.
  @date    2-4-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* ----------------------------- sig_atomic_t ----------------------------- */

/* Set to 'TRUE' by the handler whenever a SIGWINCH occurs. */
volatile sig_atomic_t the_window_resized = FALSE;

/* ----------------------------- bool ----------------------------- */

/* Whether more than one buffer is or has been open. */
bool more_than_one = FALSE;
/* Whether we are in the help viewer. */
bool inhelp = FALSE;
/* Whether a 0x0A byte should be shown as a ^@ instead of a ^J. */
bool as_an_at = TRUE;
/* Whether an update of the edit window should center the cursor. */
bool focusing = TRUE;
/* Did a command mangle enough of the buffer that we should repaint the screen? */
bool refresh_needed = FALSE;
/* Whether Shift was being held together with a movement key. */
bool shift_held;
/* Whether we're running on a Linux console (a VT). */
bool on_a_vt = FALSE;
/* Becomes TRUE as soon as all options and files have been read. */
bool we_are_running = FALSE;
/* Whether Ctrl+C was pressed (when a keyboard interrupt is enabled). */
bool control_C_was_pressed = FALSE;
/* Whether to show the number of lines when the minibar is used. */
bool report_size = TRUE;
/* Whether any Sh-M-<letter> combo has been bound. */
bool shifted_metas = FALSE;
/* Whether the multiline-coloring situation has changed. */
bool perturbed = FALSE;
/* Whether the multidata should be recalculated. */
bool recook = FALSE;
/* Whether the current keystroke is a Meta key. */
bool meta_key;
/* Whether indenting/commenting should include the last line of the marked region. */
bool also_the_last = FALSE;
/* Whether the colors for the current syntax have been initialized. */
bool have_palette = FALSE;
/* Becomes TRUE when NO_COLOR is set in the environment. */
bool rescind_colors = FALSE;
/* Whether we're allowed to add to the last syntax.  When a file ends,
 * or when a new syntax command is seen, this bool becomes 'FALSE'. */
bool nanox_rc_opensyntax = FALSE;
/* Whether a syntax definition contains any color commands. */
bool nanox_rc_seen_color_command = FALSE;
/* Whether to keep mark when normally we wouldn't.  TODO: Add this to the `openfilestruct` structure instead. */
bool keep_mark = FALSE;
/* If we should refresh the suggest window. */
bool suggest_on = FALSE;
/* Whether any text is spotlighted. */
bool spotlighted = FALSE;
/* Whether to ignore modifier keys while running a macro or string bind. */
bool mute_modifiers = FALSE;
/* Whether text is being pasted into nano from outside. */
bool bracketed_paste = FALSE;
/* Whether to add to the cutbuffer instead of clearing it first. */
bool keep_cutbuffer = FALSE;
/* Whether a tool has been run at the Execute-Command prompt. */
bool ran_a_tool = FALSE;
/* If closing bracket char was printed then this is TRUE until another key input has been prossesed. */
bool last_key_was_bracket = FALSE;
/* Whether to ignore the nanoxrc files. */
bool ignore_rcfiles = FALSE;
/* Was the fill option used on the command line? */
bool fill_used = FALSE;

/* ----------------------------- char * ----------------------------- */

/* Nonalphanumeric characters that also form words. */
char *word_chars = NULL;
/* The characters used when visibly showing tabs and spaces. */
char *whitespace = NULL;
/* The path to our confining "operating" directory, when given. */
char *operating_dir = NULL;
/* The user's home directory, from $HOME or /etc/passwd. */
char *homedir = NULL;
/* An error message (if any) about nanorc files or history files. */
char *startup_problem = NULL;
/* The path to the rcfile we're parsing. */
char *nanox_rc_path = NULL;
/* The directory where we store backup files. */
char *backup_dir = NULL;
/* The answer string used by the status-bar prompt. */
char *answer = NULL;
/* The opening and closing brackets that bracket searches can find. */
char *matchbrackets = NULL;
/* The closing punctuation that can end sentences. */
char *punct = NULL;
/* The closing brackets that can follow closing punctuation and can end sentences. */
char *brackets = NULL;
/* The quoting string.  The default value is set in main(). */
char *quotestr = NULL;
/* The command to use for the alternate spell checker. */
char *alt_speller = NULL;
/* The argument of the --rcfile option, when given. */
char *custom_nanorc = NULL;
/* The color syntax name specified on the command line. */
char *syntaxstr = NULL;
/* The directory for nano's history files. */
char *statedir = NULL;
/* A ptr to the full suggested string. */
char *suggest_str = NULL;
/* The current browser directory when trying to do tab completion. */
char *present_path = NULL;
/* The last string we searched for. */
char *last_search = NULL;
/* When not NULL: the title of the current help text. */
char *title = NULL;
/* The name (of a function) between braces in a string bind. */
char *commandname = NULL;

/* ----------------------------- char [] ----------------------------- */

/* The buffer holding the current completion search string.  Note that this is for the `tui`. */
char  suggest_buf[1024] = "";

/* ----------------------------- const char * ----------------------------- */

/* These two tags are used elsewhere too, so they are global.
 * TRANSLATORS: Try to keep the next two strings at most 10 characters. */
const char *exit_tag  = N_("Exit");
const char *close_tag = N_("Close");
/* Holds the `TERM` environment variable, when it exists.  Otherwise `NULL`. */
const char *term_env_var = NULL;
/* Holds the `TERM_PROGRAM` environment variable, when it exists.  Otherwise `NULL`. */
const char *term_program_env_var = NULL;

/* ----------------------------- int ----------------------------- */

/* How many rows does the edit window take up? */
int editwinrows = 0;
/* The number of usable columns in the edit window: COLS - margin. */
int editwincols = -1;
/* The amount of space reserved at the left for line numbers. */
int margin = 0;
/* Becomes 1 when the indicator "scroll bar" must be shown. */
int sidebar = 0;
/* The currently active menu, initialized to a dummy value. */
int currmenu = MMOST;
/* The curses attribute we use to highlight something. */
int hilite_attribute = A_REVERSE;
/* The current length of the string used to search for completions. */
int suggest_len = 0;
/* Whether to center the line with the cursor (0), push it to the top of the viewport (1), or to the bottom (2). */
int cycling_aim = 0;
/* Whether the last search found something. */
int didfind = 0;
/* Becomes 0 when --nowrap and 1 when --breaklonglines is used. */
int hardwrap = -2;
/* Extended ncurses key code for `Ctrl+Left`. */
int controlleft;
/* Extended ncurses key code for `Ctrl+Right`. */
int controlright;
/* Extended ncurses key code for `Ctrl+Up`. */
int controlup;
/* Extended ncurses key code for `Ctrl+Down`. */
int controldown;
/* Extended ncurses key code for `Ctrl+Home`. */
int controlhome;
/* Extended ncurses key code for `Ctrl+End`. */
int controlend;
/* Extended ncurses key code for `Ctrl+Delete`. */
int controldelete;
/* Extended ncurses key code for `Ctrl+Shift+Delete`. */
int controlshiftdelete;
/* Extended ncurses key code for `Shift+Left`. */
int shiftleft;
/* Extended ncurses key code for `Shift+Right`. */
int shiftright;
/* Extended ncurses key code for `Shift+Up`. */
int shiftup;
/* Extended ncurses key code for `Shift+Down`. */
int shiftdown;
/* Extended ncurses key code for `Shift+Ctrl+Left`. */
int shiftcontrolleft;
/* Extended ncurses key code for `Shift+Ctrl+Right`. */
int shiftcontrolright;
/* Extended ncurses key code for `Shift+Ctrl+Up`. */
int shiftcontrolup;
/* Extended ncurses key code for `Shift+Ctrl+Down`. */
int shiftcontroldown;
/* Extended ncurses key code for `Shift+Ctrl+Home`. */
int shiftcontrolhome;
/* Extended ncurses key code for `Shift+Ctrl+End`. */
int shiftcontrolend;
/* Extended ncurses key code for `Alt+Left`. */
int altleft;
/* Extended ncurses key code for `Alt+Right`. */
int altright;
/* Extended ncurses key code for `Alt+Up`. */
int altup;
/* Extended ncurses key code for `Alt+Down`. */
int altdown;
/* Extended ncurses key code for `Alt+Home`. */
int althome;
/* Extended ncurses key code for `Alt+End`. */
int altend;
/* Extended ncurses key code for `Alt+PageUp`. */
int altpageup;
/* Extended ncurses key code for `Alt+PageDown`. */
int altpagedown;
/* Extended ncurses key code for `Alt+Insert`. */
int altinsert;
/* Extended ncurses key code for `Alt+Delete`. */
int altdelete;
/* Extended ncurses key code for `Shift+Alt+Left`. */
int shiftaltleft;
/* Extended ncurses key code for `Shift+Alt+Right`. */
int shiftaltright;
/* Extended ncurses key code for `Shift+Alt+Up`. */
int shiftaltup;
/* Extended ncurses key code for `Shift+Alt+Down`. */
int shiftaltdown;
/* Extended ncurses key code for mouse focus in. */
int mousefocusin;
/* Extended ncurses key code for mouse focus out. */
int mousefocusout;

/* ----------------------------- int * ----------------------------- */

/* An array of characters that together depict the scrollbar. */
int *bardata = NULL;

/* ----------------------------- int [] ----------------------------- */

/* The length in bytes of these characters. */
int whitelen[2];
/* The processed color pairs for the interface elements. */
int interface_color_pair[NUMBER_OF_ELEMENTS] = {0};

/* ----------------------------- float ----------------------------- */

/* The mouse x position, this is used for the gui. */
float mouse_x = 0;
/* The mouse y position, this is used for the gui. */
float mouse_y = 0;
/* The current width of the gui. */
float gui_width = 0;
/* The current height of the gui. */
float gui_height = 0;

/* ----------------------------- long ----------------------------- */

/* The width of a tab in spaces.  The default is set in main(). */
long tabsize = -1;
/* The relative column where we will wrap lines. */
long fill = -COLUMNS_FROM_EOL;
/* The column at which a vertical bar will be drawn. */
long stripe_column = 0;

/* ----------------------------- Ulong ----------------------------- */

/* The actual column where we will wrap lines, based on fill. */
Ulong wrap_at = 0;
/* The line number of the last encountered error when parsing an rc file. */
Ulong nanox_rc_lineno = 0;
/* Where the spotlighted text starts. */
Ulong light_from_col = 0;
/* Where the spotlighted text ends. */
Ulong light_to_col = 0;

/* ----------------------------- Ulong [] ----------------------------- */

/* Our flags array, containing the states of all global options. */
Ulong flags[1] = {0};

/* ----------------------------- WINDOW * ----------------------------- */

/* The top portion of the screen, showing the version number of nano, the name of the file, and whether the buffer was modified. */
WINDOW *topwin = NULL;
/* The middle portion of the screen: the edit window, showing the contents of the current buffer, the file we are editing. */
WINDOW *midwin = NULL;
/* The bottom portion of the screen, where status-bar messages, the status-bar prompt, and a list of shortcuts are shown. */
WINDOW *footwin = NULL;
/* Test window for sugestions. */
WINDOW *suggestwin = NULL;

/* ----------------------------- linestruct * ----------------------------- */

/* The buffer where we store cut text. */
linestruct *cutbuffer = NULL;
/* The last line in the cutbuffer. */
linestruct *cutbottom = NULL;
/* The current item in the list of strings that were searched for. */
linestruct *search_history = NULL;
/* The current item in the list of replace strings. */
linestruct *replace_history = NULL;
/* The current item in the list of commands that were run with ^T. */
linestruct *execute_history = NULL;
/* The oldest item in the list of search strings. */
linestruct *searchtop = NULL;
/* The empty item at the end of the list of search strings. */
linestruct *searchbot  = NULL;
/* The oldest item in the list of replace strings. */
linestruct *replacetop = NULL;
/* The empty item at the end of the list of replace strings. */
linestruct *replacebot = NULL;
/* The oldest item in the list of execution strings. */
linestruct *executetop = NULL;
/* The empty item at the end of the list of execution strings. */
linestruct *executebot = NULL;
/* The line where the last completion was found, if any. */
linestruct *pletion_line = NULL;

/* ----------------------------- openfilestruct * ----------------------------- */

/* The list of all open file buffers. */
openfilestruct *openfile = NULL;
/* The first open buffer. */
openfilestruct *startfile = NULL;

/* ----------------------------- Font * ----------------------------- */

/* The ui font the gui uses. */
Font *uifont = NULL;
/* The text font the gui uses. */
Font *textfont = NULL;

/* ----------------------------- Editor * ----------------------------- */

/* The list of all open editor's */
Editor *openeditor = NULL;
/* The first open editor. */
Editor *starteditor = NULL;

/* ----------------------------- colortype * ----------------------------- */

/* The end of the color list for the current syntax. */
colortype *nanox_rc_lastcolor = NULL;

/* ----------------------------- colortype *[] ----------------------------- */

/* The color combinations for interface elements given in the rcfile. */
colortype *color_combo[NUMBER_OF_ELEMENTS] = {NULL};

/* ----------------------------- keystruct * ----------------------------- */

/* The start of the shortcuts list. */
keystruct *sclist = NULL;
/* The function that the above name resolves to, if any. */
keystruct *planted_shortcut = NULL;

/* ----------------------------- funcstruct * ----------------------------- */

/* The start of the functions list. */
funcstruct *allfuncs = NULL;
/* The last function in the list. */
funcstruct *tailfunc = NULL;
/* A pointer to the special Exit/Close item. */
funcstruct *exitfunc = NULL;

/* ----------------------------- regex_t ----------------------------- */

/* The compiled regular expression to use in searches. */
regex_t search_regexp;
/* The compiled regular expression from the quoting string. */
regex_t quotereg;

/* ----------------------------- regmatch_t [] ----------------------------- */

/* The match positions for parenthetical subexpressions, 10 maximum, used in regular expression searches. */
regmatch_t regmatches[10];

/* ----------------------------- GLFWwindow * ----------------------------- */

GLFWwindow *gui_window = NULL;

/* ----------------------------- message_type ----------------------------- */

/* Messages of type HUSH should not overwrite type MILD nor ALERT. */
message_type lastmessage = VACUUM;

/* ----------------------------- configstruct * ----------------------------- */

/* Global config to store data retrieved from config file. */
configstruct *config = NULL;

/* ----------------------------- syntaxtype * ----------------------------- */

/* The syntax that is currently being parsed. */
syntaxtype *nanox_rc_live_syntax = NULL;
/* The global list of color syntaxes. */
syntaxtype *syntaxes = NULL;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* Empty functions, for the most part corresponding to toggles. */

void case_sens_void(void) {
  ;
}

void regexp_void(void) {
  ;
}

void backwards_void(void) {
  ;
}

void get_older_item(void) {
  ;
}

void get_newer_item(void) {
  ;
}

void flip_replace(void) {
  ;
}

void flip_goto(void) {
  ;
}

void to_files(void) {
  ;
}

void goto_dir(void) {
  ;
}

void do_nothing(void) {
  ;
}

void do_toggle(void) {
  ;
}

void dos_format(void) {
  ;
}

void mac_format(void) {
  ;
}

void append_it(void) {
  ;
}

void prepend_it(void) {
  ;
}

void back_it_up(void) {
  ;
}

void flip_execute(void) {
  ;
}

void flip_pipe(void) {
  ;
}

void flip_convert(void) {
  ;
}

void flip_newbuffer(void) {
  ;
}

void discard_buffer(void) {
  ;
}

void do_cancel(void) {
  ;
}

/* Parse the given keystring and return the corresponding keycode, or return -1 when the string is invalid. */
int keycode_from_string(const char *keystring) {
  int fn;
  if (keystring[0] == '^') {
    if (keystring[2] == '\0') {
      if (keystring[1] == '/' || keystring[1] == '-') {
        return 31;
      }
      if (keystring[1] <= '_') {
        return keystring[1] - 64;
      }
      if (keystring[1] == '`') {
        return 0;
      }
      else {
        return -1;
      }
    }
    else if (strcasecmp(keystring, "^Space") == 0) {
      return 0;
    }
    else {
      return -1;
    }
  }
  else if (keystring[0] == 'M') {
    if (keystring[1] == '-' && keystring[3] == '\0') {
      return tolower((Uchar)keystring[2]);
    }
    if (strcasecmp(keystring, "M-Space") == 0) {
      return (int)' ';
    }
    else {
      return -1;
    }
  }
  else if (strncasecmp(keystring, "Sh-M-", 5) == 0 && 'a' <= (keystring[5] | 0x20) && (keystring[5] | 0x20) <= 'z' && keystring[6] == '\0') {
    shifted_metas = TRUE;
    return (keystring[5] & 0x5F);
  }
  else if (keystring[0] == 'F') {
    fn = atoi(&keystring[1]);
    if (fn < 1 || fn > 24) {
      return -1;
    }
    return (KEY_F0 + fn);
  }
  else if (strcasecmp(keystring, "Ins") == 0) {
    return KEY_IC;
  }
  else if (strcasecmp(keystring, "Del") == 0) {
    return KEY_DC;
  }
  else {
    return -1;
  }
}

/* Return the first shortcut in the list of shortcuts that, matches the given function in the given menu. */
const keystruct *first_sc_for(int menu, functionptrtype function) {
  DLIST_FOR_NEXT(sclist, sc) {
    if ((sc->menus & menu) && sc->func == function && sc->keystr[0]) {
      return sc;
    }
  }
  return NULL;
}

/* Return the number of entries that can be shown in the given menu. */
Ulong shown_entries_for(int menu) {
  funcstruct *item = allfuncs;
  Ulong maximum = (((COLS + 40) / 20) * 2);
  Ulong count = 0;
  while (count < maximum && item) {
    PREFETCH(item->next);
    if (item->menus & menu) {
      ++count;
    }
    item = item->next;
  }
  /* When --saveonexit is not used, widen the grid of the WriteOut menu. */
  if (menu == MWRITEFILE && !item && !first_sc_for(menu, discard_buffer)) {
    --count;
  }
  return count;
}

/* Return the first shortcut in the current menu that matches the given input. */
const keystruct *get_shortcut(int keycode) {
  /* When in gui mode, always return `NULL`. */
  if (ISSET(USING_GUI)) {
    return NULL;
  }
  /* Plain characters and upper control codes cannot be shortcuts. */
  if (!meta_key && 0x20 <= keycode && keycode <= 0xFF) {
    return NULL;
  }
  /* Lower control codes with Meta cannot be shortcuts either. */
  if (meta_key && keycode < 0x20) {
    return NULL;
  }
  /* During a paste at a prompt, ignore all command keycodes. */
  if (bracketed_paste && keycode != BRACKETED_PASTE_MARKER) {
    return NULL;
  }
  if (keycode == PLANTED_A_COMMAND) {
    return planted_shortcut;
  }
  DLIST_FOR_NEXT(sclist, sc) {
    PREFETCH(sc->next);
    if ((sc->menus & currmenu) && keycode == sc->keycode) {
      return sc;
    }
  }
  return NULL;
}

/* ----------------------------- Func from key ----------------------------- */

/* Returns a pointer to the function that is bound to the given key. */
functionptrtype func_from_key(int keycode) {
  const keystruct *sc = get_shortcut(keycode);
  return (sc ? sc->func : NULL);
}

/* ----------------------------- Interpret ----------------------------- */

/* Returns the function that is bound to the given key in the file browser
 * or the help viewer.  Accept also certain plain characters, for
 * compatibility with Pico or to mimic `less` and similar text viewers. */
functionptrtype interpret(int keycode) {
  if (!meta_key) {
    if (keycode == 'N') {
      return do_findprevious;
    }
    else if (keycode == 'n') {
      return do_findnext;
    }
    switch (tolower(keycode)) {
      case 'b':
      case '-': {
        return do_page_up;
      }
      case ' ': {
        return do_page_down;
      }
      case 'w':
      case '/': {
        return do_search_forward;
      }
      case 'g': {
        return goto_dir;
      }
      case '?': {
        return do_help;
      }
      case 's': {
        return do_enter;
      }
      case 'e':
      case 'q':
      case 'x': {
        return do_exit;
      }
    }
  }
  return func_from_key(keycode);
}
