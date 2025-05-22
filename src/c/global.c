/** @file global.c

  @author  Melwin Svensson.
  @date    2-4-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Set to 'TRUE' by the handler whenever a SIGWINCH occurs. */
volatile sig_atomic_t the_window_resized = FALSE;

/* The width of a tab in spaces.  The default is set in main(). */
long tabsize = -1;
/* The relative column where we will wrap lines. */
long fill = -COLUMNS_FROM_EOL;

/* The actual column where we will wrap lines, based on fill. */
Ulong wrap_at = 0;

/* The top portion of the screen, showing the version number of nano, the name of the file, and whether the buffer was modified. */
WINDOW *topwin = NULL;
/* The middle portion of the screen: the edit window, showing the contents of the current buffer, the file we are editing. */
WINDOW *midwin = NULL;
/* The bottom portion of the screen, where status-bar messages, the status-bar prompt, and a list of shortcuts are shown. */
WINDOW *footwin = NULL;

/* The list of all open file buffers. */
openfilestruct *openfile = NULL;
/* The first open buffer. */
openfilestruct *startfile = NULL;

/* The ui font the gui uses. */
Font *uifont = NULL;
/* The text font the gui uses. */
Font *textfont = NULL;

/* The mouse x position, this is used for the gui. */
float mouse_x = 0;
/* The mouse y position, this is used for the gui. */
float mouse_y = 0;
/* The current width of the gui. */
float gui_width = 0;
/* The current height of the gui. */
float gui_height = 0;

/* The list of all open editor's */
Editor *openeditor = NULL;
/* The first open editor. */
Editor *starteditor = NULL;

/* The start of the shortcuts list. */
keystruct *sclist = NULL;

/* The start of the functions list. */
funcstruct *allfuncs = NULL;
/* The last function in the list. */
funcstruct *tailfunc;
/* A pointer to the special Exit/Close item. */
funcstruct *exitfunc;

/* The compiled regular expression to use in searches. */
regex_t search_regexp;

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

/* These two tags are used elsewhere too, so they are global.
 * TRANSLATORS: Try to keep the next two strings at most 10 characters. */
const char *exit_tag  = N_("Exit");
const char *close_tag = N_("Close");

/* Nonalphanumeric characters that also form words. */
char *word_chars = NULL;
/* The characters used when visibly showing tabs and spaces. */
char *whitespace = NULL;
/* The path to our confining "operating" directory, when given. */
char *operating_dir = NULL;
/* The user's home directory, from $HOME or /etc/passwd. */
char *homedir = NULL;

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

/* The length in bytes of these characters. */
int whitelen[2];

/* Our flags array, containing the states of all global options. */
Ulong flags[1] = {0};

GLFWwindow *gui_window = NULL;

/* Messages of type HUSH should not overwrite type MILD nor ALERT. */
message_type lastmessage = VACUUM;

/* The processed color pairs for the interface elements. */
int interface_color_pair[NUMBER_OF_ELEMENTS] = {0};

/* Global config to store data retrieved from config file. */
configstruct *config = NULL;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* Empty functions, for the most part corresponding to toggles. */

void discard_buffer(void) {
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
const keystruct *first_sc_for(const int menu, functionptrtype function) {
  for (keystruct *sc = sclist; sc; sc = sc->next) {
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
