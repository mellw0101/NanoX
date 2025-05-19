/** @file global.c

  @author  Melwin Svensson.
  @date    2-4-2025.

 */
#include "../include/c_proto.h"


/* The width of a tab in spaces.  The default is set in main(). */
long tabsize = -1;

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

/* The start of the functions list. */
funcstruct *allfuncs = NULL;
/* The last function in the list. */
funcstruct *tailfunc;
/* A pointer to the special Exit/Close item. */
funcstruct *exitfunc;

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

/* These two tags are used elsewhere too, so they are global.
 * TRANSLATORS: Try to keep the next two strings at most 10 characters. */
const char *exit_tag  = N_("Exit");
const char *close_tag = N_("Close");

/* Nonalphanumeric characters that also form words. */
char *word_chars = NULL;
/* The characters used when visibly showing tabs and spaces. */
char *whitespace = NULL;

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
