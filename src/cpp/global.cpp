#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <Mlib/def.h>
#include <term.h>

/* Global variables. */

/* Set to 'TRUE' by the handler whenever a SIGWINCH occurs. */
volatile sig_atomic_t the_window_resized = FALSE;
/* Whether we're running on a Linux console (a VT). */
bool on_a_vt = FALSE;
/* Whether any Sh-M-<letter> combo has been bound. */
bool shifted_metas = FALSE;
/* Whether the current keystroke is a Meta key. */
bool meta_key;
/* Whether Shift was being held together with a movement key. */
bool shift_held;
/* Whether to ignore modifier keys while running a macro or string bind. */
bool mute_modifiers = FALSE;
/* Whether text is being pasted into nano from outside. */
bool bracketed_paste = FALSE;
/* Becomes TRUE as soon as all options and files have been read. */
bool we_are_running = FALSE;
/* Whether more than one buffer is or has been open. */
bool more_than_one = FALSE;
/* Whether to show the number of lines when the minibar is used. */
bool report_size = TRUE;
/* Whether a tool has been run at the Execute-Command prompt. */
bool ran_a_tool = FALSE;
/* Whether we are in the help viewer. */
bool inhelp = FALSE;
/* When not NULL: the title of the current help text. */
char *title = NULL;
/* Did a command mangle enough of the buffer
 * that we should repaint the screen? */
bool refresh_needed = FALSE;
/* If we should refresh the suggest window. */
bool suggest_on = FALSE;
/* Whether an update of the edit window should center the cursor. */
bool focusing = TRUE;
/* Whether a 0x0A byte should be shown as a ^@ instead of a ^J. */
bool as_an_at = TRUE;
/* Whether Ctrl+C was pressed (when a keyboard interrupt is enabled). */
bool control_C_was_pressed = FALSE;
/* Messages of type HUSH should not overwrite type MILD nor ALERT. */
message_type lastmessage = VACUUM;
/* The line where the last completion was found, if any. */
linestruct *pletion_line = NULL;
/* Whether indenting/commenting should include the last line of the marked
 * region. */
bool also_the_last = FALSE;
/* The answer string used by the status-bar prompt. */
char *answer = NULL;
/* The last string we searched for. */
char *last_search = NULL;
/* Whether the last search found something. */
int didfind = 0;
/* The current browser directory when trying to do tab completion. */
char *present_path = NULL;
/* Our flags array, containing the states of all global options. */
unsigned long flags[1] = {0};

int controlleft, controlright, controlup, controldown;
int controlhome, controlend;
int controldelete, controlshiftdelete;
int shiftleft, shiftright, shiftup, shiftdown;
int shiftcontrolleft, shiftcontrolright, shiftcontrolup, shiftcontroldown;
int shiftcontrolhome, shiftcontrolend;
int altleft, altright, altup, altdown;
int althome, altend, altpageup, altpagedown;
int altinsert, altdelete;
int shiftaltleft, shiftaltright, shiftaltup, shiftaltdown;
int mousefocusin, mousefocusout;
int controlbsp;

/* The relative column where we will wrap lines. */
long fill = -COLUMNS_FROM_EOL;
/* The actual column where we will wrap lines, based on fill. */
unsigned long wrap_at = 0;
/* The top portion of the screen,
 * showing the version number of nano,
 * the name of the file,
 * and whether the buffer was modified. */
WINDOW *topwin = NULL;
/* The middle portion of the screen: the edit window, showing the
 * contents of the current buffer, the file we are editing. */
WINDOW *midwin = NULL;
/* The bottom portion of the screen, where status-bar messages,
 * the status-bar prompt, and a list of shortcuts are shown. */
WINDOW *footwin = NULL;
/* Test window for sugestions. */
WINDOW *suggestwin = NULL;
/* How many rows does the edit window take up? */
int editwinrows = 0;
/* The number of usable columns in the edit window: COLS - margin. */
int editwincols = -1;
/* The amount of space reserved at the left for line numbers. */
int margin = 0;
/* Becomes 1 when the indicator "scroll bar" must be shown. */
int sidebar = 0;
/* An array of characters that together depict the scrollbar. */
int *bardata = NULL;
/* The column at which a vertical bar will be drawn. */
long stripe_column = 0;
/* Whether to center the line with the cursor (0), push it
 * to the top of the viewport (1), or to the bottom (2). */
int cycling_aim = 0;
/* The buffer where we store cut text. */
linestruct *cutbuffer = NULL;
/* The last line in the cutbuffer. */
linestruct *cutbottom = NULL;
/* Whether to add to the cutbuffer instead of clearing it first. */
bool keep_cutbuffer = FALSE;
/* The list of all open file buffers. */
openfilestruct *openfile = NULL;
/* The first open buffer. */
openfilestruct *startfile = NULL;
/* The opening and closing brackets that bracket searches can find. */
char *matchbrackets = NULL;
/* The characters used when visibly showing tabs and spaces. */
char *whitespace = NULL;
/* The length in bytes of these characters. */
int whitelen[2];
/* The closing punctuation that can end sentences. */
char *punct = NULL;
/* The closing brackets that can follow closing punctuation and can end
 * sentences. */
char *brackets = NULL;
/* The quoting string.  The default value is set in main(). */
char *quotestr = NULL;
/* The compiled regular expression from the quoting string. */
regex_t quotereg;
/* Nonalphanumeric characters that also form words. */
char *word_chars = NULL;
/* The width of a tab in spaces.  The default is set in main(). */
long tabsize = -1;
/* The directory where we store backup files. */
char *backup_dir = NULL;
/* The path to our confining "operating" directory, when given. */
char *operating_dir = NULL;
/* The command to use for the alternate spell checker. */
char *alt_speller = NULL;
/* The global list of color syntaxes. */
syntaxtype *syntaxes = NULL;
/* The color syntax name specified on the command line. */
char *syntaxstr = NULL;
/* Whether the colors for the current syntax have been initialized. */
bool have_palette = FALSE;
/* Becomes TRUE when NO_COLOR is set in the environment. */
bool rescind_colors = FALSE;
/* Whether the multiline-coloring situation has changed. */
bool perturbed = FALSE;
/* Whether the multidata should be recalculated. */
bool recook = FALSE;
/* The currently active menu, initialized to a dummy value. */
int currmenu = MMOST;
/* The start of the shortcuts list. */
keystruct *sclist = NULL;
/* The start of the functions list. */
funcstruct *allfuncs = NULL;
/* The last function in the list. */
funcstruct *tailfunc;
/* A pointer to the special Exit/Close item. */
funcstruct *exitfunc;
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
linestruct *replacetop = NULL;
linestruct *replacebot = NULL;
linestruct *executetop = NULL;
linestruct *executebot = NULL;
/* The compiled regular expression to use in searches. */
regex_t search_regexp;
/* The match positions for parenthetical subexpressions,
 * 10 maximum, used in regular expression searches. */
regmatch_t regmatches[10];
/* The curses attribute we use to highlight something. */
int hilite_attribute = A_REVERSE;
/* The color combinations for interface elements given in the rcfile. */
colortype *color_combo[NUMBER_OF_ELEMENTS] = {NULL};
/* The processed color pairs for the interface elements. */
int interface_color_pair[NUMBER_OF_ELEMENTS] = {0};
/* The user's home directory, from $HOME or /etc/passwd. */
char *homedir = NULL;
/* The directory for nano's history files. */
char *statedir = NULL;
/* An error message (if any) about nanorc files or history files. */
char *startup_problem = NULL;
/* The argument of the --rcfile option, when given. */
char *custom_nanorc = NULL;
/* The name (of a function) between braces in a string bind. */
char *commandname = NULL;
/* The function that the above name resolves to, if any. */
keystruct *planted_shortcut = NULL;
/* Whether any text is spotlighted. */
bool spotlighted = FALSE;
/* Where the spotlighted text starts. */
unsigned long light_from_col = 0;
/* Where the spotlighted text ends. */
unsigned long light_to_col = 0;
/* To make the functions and shortcuts lists clearer. */
constexpr bool BLANKAFTER = TRUE;
constexpr bool TOGETHER   = FALSE;
/* If closing bracket char was printed then this is TRUE until another
 * key input has been prossesed. */
bool        last_key_was_bracket = FALSE;
colortype  *last_c_color         = NULL;
syntaxtype *c_syntaxtype         = NULL;
/* Vector to hold bracket bairs for closing them, (NOT YET IMPLEMENTED). */
std::vector<bracket_pair> bracket_pairs;
/* Vector for bracket entry`s. */
std::vector<bracket_entry> bracket_entrys;
/* Vector to hold struct`s that are found, we use this to higlight created
 * objects. */
std::vector<std::string> syntax_structs;
/* Vector to hold class`es that are found, we use this to higlight created
 * objects. */
std::vector<std::string> syntax_classes;
/* This vector is used to store all vars, for live syntax. */
std::vector<std::string> syntax_vars;
/* This is to store functions to avoid douplicates. */
std::vector<std::string> syntax_funcs;
/* Vector for all includes that have been handled. */
std::vector<std::string> handled_includes;

const char *term = NULL;

/* Empty functions, for the most part corresponding to toggles. */
void
case_sens_void(void)
{
    ;
}

void
regexp_void(void)
{
    ;
}

void
backwards_void(void)
{
    ;
}

void
get_older_item(void)
{
    ;
}

void
get_newer_item(void)
{
    ;
}

void
flip_replace(void)
{
    ;
}

void
flip_goto(void)
{
    ;
}

void
to_files(void)
{
    ;
}

void
goto_dir(void)
{
    ;
}

void
do_nothing(void)
{
    ;
}

void
do_toggle(void)
{
	;
}

void
dos_format(void)
{
    ;
}

void
mac_format(void)
{
    ;
}

void
append_it(void)
{
    ;
}

void
prepend_it(void)
{
    ;
}

void
back_it_up(void)
{
    ;
}

void
flip_execute(void)
{
    ;
}

void
flip_pipe(void)
{
    ;
}

void
flip_convert(void)
{
    ;
}

void
flip_newbuffer(void)
{
    ;
}

void
discard_buffer(void)
{
    ;
}

void
do_cancel(void)
{
    ;
}

/* Add a function to the linked list of functions. */
void
add_to_funcs(functionptrtype function, const int menus, const char *tag,
             const char *phrase, bool blank_after)
{
    funcstruct *f = (funcstruct *)nmalloc(sizeof(funcstruct));
    (allfuncs == NULL) ? allfuncs = f : tailfunc->next = f;
    tailfunc       = f;
    f->next        = NULL;
    f->func        = function;
    f->menus       = menus;
    f->tag         = tag;
    f->phrase      = phrase;
    f->blank_after = blank_after;
}

/* Parse the given keystring and return the corresponding keycode,
 * or return -1 when the string is invalid. */
int
keycode_from_string(const char *keystring)
{
    if (keystring[0] == '^')
    {
        if (keystring[2] == '\0')
        {
            if (keystring[1] == '/' || keystring[1] == '-')
            {
                return 31;
            }
            if (keystring[1] <= '_')
            {
                return keystring[1] - 64;
            }
            if (keystring[1] == '`')
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else if (constexpr_strcasecmp(keystring, "^Space") == 0)
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else if (keystring[0] == 'M')
    {
        if (keystring[1] == '-' && keystring[3] == '\0')
        {
            return constexpr_tolower((unsigned char)keystring[2]);
        }
        if (constexpr_strcasecmp(keystring, "M-Space") == 0)
        {
            return (int)' ';
        }
        else
        {
            return -1;
        }
    }
    else if (constexpr_strncasecmp(keystring, "Sh-M-", 5) == 0 &&
             'a' <= (keystring[5] | 0x20) && (keystring[5] | 0x20) <= 'z' &&
             keystring[6] == '\0')
    {
        shifted_metas = TRUE;
        return (keystring[5] & 0x5F);
    }
    else if (keystring[0] == 'F')
    {
        int fn = atoi(&keystring[1]);
        if (fn < 1 || fn > 24)
        {
            return -1;
        }
        return KEY_F0 + fn;
    }
    else if (constexpr_strcasecmp(keystring, "Ins") == 0)
    {
        return KEY_IC;
    }
    else if (constexpr_strcasecmp(keystring, "Del") == 0)
    {
        return KEY_DC;
    }
    else
    {
        return -1;
    }
}

void
show_curses_version(void)
{
    statusline(INFO, "ncurses-%i.%i, patch %li", NCURSES_VERSION_MAJOR,
               NCURSES_VERSION_MINOR, NCURSES_VERSION_PATCH);
}

/* Add a key combo to the linked list of shortcuts. */
void
add_to_sclist(const int menus, const char *scstring, const int keycode,
              functionptrtype function, const int toggle)
{
    static keystruct *tailsc;
    static int        counter = 0;
    keystruct        *sc      = (keystruct *)nmalloc(sizeof(keystruct));
    /* Start the list, or tack on the next item. */
    (sclist == NULL) ? sclist = sc : tailsc->next = sc;
    sc->next = NULL;
    /* Fill in the data. */
    sc->menus  = menus;
    sc->func   = function;
    sc->toggle = toggle;
    /* When not the same toggle as the previous one, increment the ID. */
    if (toggle)
    {
        sc->ordinal = (tailsc->toggle == toggle) ? counter : ++counter;
    }
    sc->keystr  = scstring;
    sc->keycode = (keycode ? keycode : keycode_from_string(scstring));
    tailsc      = sc;
}

/* Return the first shortcut in the list of shortcuts that,
 * matches the given function in the given menu. */
const keystruct *
first_sc_for(const int menu, functionptrtype function)
{
    for (keystruct *sc = sclist; sc != NULL; sc = sc->next)
    {
        if ((sc->menus & menu) && sc->func == function && sc->keystr[0])
        {
            return sc;
        }
    }
    return NULL;
}

/* Return the number of entries that can be shown in the given menu. */
unsigned long
shown_entries_for(const int menu)
{
    funcstruct   *item    = allfuncs;
    unsigned long maximum = ((COLS + 40) / 20) * 2;
    unsigned long count   = 0;
    while (count < maximum && item != NULL)
    {
        if (item->menus & menu)
        {
            count++;
        }
        item = item->next;
    }
    /* When --saveonexit is not used, widen the grid of the WriteOut menu. */
    if (menu == MWRITEFILE && item == NULL &&
        first_sc_for(menu, discard_buffer) == NULL)
    {
        count--;
    }
    return count;
}

/* Return the first shortcut in the current menu that matches the given input.
 * TODO : (get_shortcut) - Figure out how this works. */
const keystruct *
get_shortcut(const int keycode)
{
    /* Plain characters and upper control codes cannot be shortcuts. */
    if (!meta_key && 0x20 <= keycode && keycode <= 0xFF)
    {
        return NULL;
    }
    /* Lower control codes with Meta cannot be shortcuts either. */
    if (meta_key && keycode < 0x20)
    {
        return NULL;
    }
    /* During a paste at a prompt, ignore all command keycodes. */
    if (bracketed_paste && keycode != BRACKETED_PASTE_MARKER)
    {
        return NULL;
    }
    if (keycode == PLANTED_A_COMMAND)
    {
        return planted_shortcut;
    }
    for (const keystruct *sc = sclist; sc != NULL; sc = sc->next)
    {
        if ((sc->menus & currmenu) && keycode == sc->keycode)
        {
            return sc;
        }
    }
    return NULL;
}

/* Return a pointer to the function that is bound to the given key. */
functionptrtype
func_from_key(const int keycode)
{
    const keystruct *sc = get_shortcut(keycode);
    return (sc) ? sc->func : NULL;
}

/* Return the function that is bound to the given key in the file browser or
 * the help viewer.  Accept also certain plain characters, for compatibility
 * with Pico or to mimic 'less' and similar text viewers. */
functionptrtype
interpret(const int keycode)
{
    if (!meta_key)
    {
        if (keycode == 'N')
        {
            return do_findprevious;
        }
        if (keycode == 'n')
        {
            return do_findnext;
        }
        switch (constexpr_tolower(keycode))
        {
            case 'b' :
            case '-' :
            {
                return do_page_up;
            }
            case ' ' :
            {
                return do_page_down;
            }
            case 'w' :
            case '/' :
            {
                return do_search_forward;
            }
            case 'g' :
            {
                return goto_dir;
            }
            case '?' :
            {
                return do_help;
            }
            case 's' :
            {
                return do_enter;
            }
            case 'e' :
            case 'q' :
            case 'x' :
            {
                return do_exit;
            }
        }
    }
    return func_from_key(keycode);
}

/* These two tags are used elsewhere too, so they are global.
 * TRANSLATORS: Try to keep the next two strings at most 10 characters. */
const char *exit_tag  = N_("Exit");
const char *close_tag = N_("Close");

/* Initialize the list of functions and the list of shortcuts.
 * This is the place where all the functions and shortcuts are defined.
 * TODO: (shortcut_init) Currently, this function is a mess. It needs to be
 * cleaned up, and the keybindings need to be changed to a more resonable
 * format.
 * TODO 2: FIX ^Bsp */
void
shortcut_init(void)
{
    /* TRANSLATORS: The next long series of strings are shortcut descriptions;
     *              they are best kept shorter than 56 characters, but may be
     * longer. */
    const char *cancel_gist = N_("Cancel the current function");
    const char *help_gist   = N_("Display this help text");
    const char *exit_gist   = N_("Close the current buffer / Exit from nano");
    const char *writeout_gist =
        N_("Write the current buffer (or the marked region) to disk");
    const char *readfile_gist =
        N_("Insert another file into current buffer (or into new buffer)");
    const char *whereis_gist =
        N_("Search forward for a string or a regular expression");
    const char *wherewas_gist =
        N_("Search backward for a string or a regular expression");
    const char *cut_gist =
        N_("Cut current line (or marked region) and store it in cutbuffer");
    const char *copy_gist =
        N_("Copy current line (or marked region) and store it in cutbuffer");
    const char *paste_gist =
        N_("Paste the contents of cutbuffer at current cursor position");
    const char *cursorpos_gist = N_("Display the position of the cursor");
    const char *spell_gist     = N_("Invoke the spell checker, if available");
    const char *replace_gist   = N_("Replace a string or a regular expression");
    const char *gotoline_gist  = N_("Go to line and column number");
    const char *bracket_gist   = N_("Go to the matching bracket");
    const char *mark_gist = N_("Mark text starting from the cursor position");
    const char *zap_gist = N_("Throw away the current line (or marked region)");
    const char *indent_gist = N_("Indent the current line (or marked lines)");
    const char *unindent_gist =
        N_("Unindent the current line (or marked lines)");
    const char *undo_gist      = N_("Undo the last operation");
    const char *redo_gist      = N_("Redo the last undone operation");
    const char *back_gist      = N_("Go back one character");
    const char *forward_gist   = N_("Go forward one character");
    const char *prevword_gist  = N_("Go back one word");
    const char *nextword_gist  = N_("Go forward one word");
    const char *prevline_gist  = N_("Go to previous line");
    const char *nextline_gist  = N_("Go to next line");
    const char *home_gist      = N_("Go to beginning of current line");
    const char *end_gist       = N_("Go to end of current line");
    const char *prevblock_gist = N_("Go to previous block of text");
    const char *nextblock_gist = N_("Go to next block of text");
    const char *parabegin_gist =
        N_("Go to beginning of paragraph; then of previous paragraph");
    const char *paraend_gist =
        N_("Go just beyond end of paragraph; then of next paragraph");
    const char *toprow_gist    = N_("Go to first row in the viewport");
    const char *bottomrow_gist = N_("Go to last row in the viewport");
    const char *center_gist    = N_("Center the line where the cursor is");
    const char *cycle_gist =
        N_("Push the cursor line to the center, then top, then bottom");
    const char *prevpage_gist  = N_("Go one screenful up");
    const char *nextpage_gist  = N_("Go one screenful down");
    const char *firstline_gist = N_("Go to the first line of the file");
    const char *lastline_gist  = N_("Go to the last line of the file");
    const char *scrollup_gist =
        N_("Scroll up one line without moving the cursor textually");
    const char *scrolldown_gist =
        N_("Scroll down one line without moving the cursor textually");
    const char *prevfile_gist = N_("Switch to the previous file buffer");
    const char *nextfile_gist = N_("Switch to the next file buffer");
    const char *verbatim_gist = N_("Insert the next keystroke verbatim");
    const char *tab_gist =
        N_("Insert a tab at the cursor position (or indent marked lines)");
    const char *enter_gist  = N_("Insert a newline at the cursor position");
    const char *delete_gist = N_("Delete the character under the cursor");
    const char *backspace_gist =
        N_("Delete the character to the left of the cursor");
    const char *chopwordleft_gist =
        N_("Delete backward from cursor to word start");
    const char *chopwordright_gist =
        N_("Delete forward from cursor to next word start");
    const char *cuttilleof_gist =
        N_("Cut from the cursor position to the end of the file");
    const char *justify_gist     = N_("Justify the current paragraph");
    const char *fulljustify_gist = N_("Justify the entire file");
    const char *wordcount_gist =
        N_("Count the number of lines, words, and characters");
    const char *suspend_gist = N_("Suspend the editor (return to the shell)");
    const char *refresh_gist = N_("Refresh (redraw) the current screen");
    const char *completion_gist = N_("Try and complete the current word");
    const char *comment_gist =
        N_("Comment/uncomment the current line (or marked lines)");
    const char *savefile_gist    = N_("Save file without prompting");
    const char *findprev_gist    = N_("Search next occurrence backward");
    const char *findnext_gist    = N_("Search next occurrence forward");
    const char *recordmacro_gist = N_("Start/stop recording a macro");
    const char *runmacro_gist    = N_("Run the last recorded macro");
    const char *anchor_gist =
        N_("Place or remove an anchor at the current line");
    const char *prevanchor_gist = N_("Jump backward to the nearest anchor");
    const char *nextanchor_gist = N_("Jump forward to the nearest anchor");
    const char *case_gist    = N_("Toggle the case sensitivity of the search");
    const char *reverse_gist = N_("Reverse the direction of the search");
    const char *regexp_gist  = N_("Toggle the use of regular expressions");
    const char *older_gist   = N_("Recall the previous search/replace string");
    const char *newer_gist   = N_("Recall the next search/replace string");
    const char *dos_gist     = N_("Toggle the use of DOS format");
    const char *mac_gist     = N_("Toggle the use of Mac format");
    const char *append_gist  = N_("Toggle appending");
    const char *prepend_gist = N_("Toggle prepending");
    const char *backup_gist  = N_("Toggle backing up of the original file");
    const char *execute_gist = N_("Execute a function or an external command");
    const char *pipe_gist =
        N_("Pipe the current buffer (or marked region) to the command");
    const char *older_command_gist = N_("Recall the previous command");
    const char *newer_command_gist = N_("Recall the next command");
    const char *convert_gist       = N_("Do not convert from DOS/Mac format");
    const char *newbuffer_gist     = N_("Toggle the use of a new buffer");
    const char *discardbuffer_gist = N_("Close buffer without saving it");
    const char *tofiles_gist       = N_("Go to file browser");
    const char *exitbrowser_gist   = N_("Exit from the file browser");
    const char *firstfile_gist     = N_("Go to the first file in the list");
    const char *lastfile_gist      = N_("Go to the last file in the list");
    const char *backfile_gist      = N_("Go to the previous file in the list");
    const char *forwardfile_gist   = N_("Go to the next file in the list");
    const char *browserlefthand_gist  = N_("Go to lefthand column");
    const char *browserrighthand_gist = N_("Go to righthand column");
    const char *browsertoprow_gist    = N_("Go to first row in this column");
    const char *browserbottomrow_gist = N_("Go to last row in this column");
    const char *browserwhereis_gist   = N_("Search forward for a string");
    const char *browserwherewas_gist  = N_("Search backward for a string");
    const char *browserrefresh_gist   = N_("Refresh the file list");
    const char *gotodir_gist          = N_("Go to directory");
    const char *lint_gist             = N_("Invoke the linter, if available");
    const char *prevlint_gist         = N_("Go to previous linter msg");
    const char *nextlint_gist         = N_("Go to next linter msg");
    const char *formatter_gist =
        N_("Invoke a program to format/arrange/manipulate the buffer");
    /* If Backspace is not ^H, then ^H can be used for Help. */
    char *bsp_string = tgetstr("kb", NULL);
    char *help_key =
        (bsp_string && *bsp_string != 0x08) ? (char *)"^H" : (char *)"^N";
#define WHENHELP(description) description
    /* Start populating the different menus with functions. */
    /* TRANSLATORS: Try to keep the next thirteen strings at most 10 characters.
     */
    add_to_funcs(do_help, (MMOST | MBROWSER) & ~MFINDINHELP, N_("Help"),
                 WHENHELP(help_gist), TOGETHER);
    add_to_funcs(do_cancel, ((MMOST & ~MMAIN) | MYESNO), N_("Cancel"),
                 WHENHELP(cancel_gist), BLANKAFTER);
    /* Remember the entry for Exit, to be able to replace it with Close. */
    add_to_funcs(do_exit, MMAIN, exit_tag, WHENHELP(exit_gist), TOGETHER);
    exitfunc = tailfunc;
    add_to_funcs(
        do_exit, MBROWSER, close_tag, WHENHELP(exitbrowser_gist), TOGETHER);
    add_to_funcs(
        do_writeout, MMAIN, N_("Write Out"), WHENHELP(writeout_gist), TOGETHER);
    /* In restricted mode, replace Insert with Justify, when possible;
     * otherwise, show Insert anyway, to keep the help items paired. */
    if (!ISSET(RESTRICTED))
    {
        add_to_funcs(do_insertfile, MMAIN, N_("Read File"),
                     WHENHELP(readfile_gist), BLANKAFTER);
    }
    else
    {
        add_to_funcs(do_justify, MMAIN, N_("Justify"), WHENHELP(justify_gist),
                     BLANKAFTER);
    }
    /* The description ("x") and blank_after (0) are irrelevant,
     * because the help viewer does not have a help text. */
    add_to_funcs(full_refresh, MHELP, N_("Refresh"), "x", 0);
    add_to_funcs(do_exit, MHELP, close_tag, "x", 0);
    add_to_funcs(do_search_forward, MMAIN | MHELP, N_("Where Is"),
                 WHENHELP(whereis_gist), TOGETHER);
    add_to_funcs(
        do_replace, MMAIN, N_("Replace"), WHENHELP(replace_gist), TOGETHER);
    add_to_funcs(cut_text, MMAIN, N_("Cut"), WHENHELP(cut_gist), TOGETHER);
    add_to_funcs(
        paste_text, MMAIN, N_("Paste"), WHENHELP(paste_gist), BLANKAFTER);
    if (!ISSET(RESTRICTED))
    {
        add_to_funcs(
            do_execute, MMAIN, N_("Execute"), WHENHELP(execute_gist), TOGETHER);
        add_to_funcs(do_justify, MMAIN, N_("Justify"), WHENHELP(justify_gist),
                     BLANKAFTER);
    }
    /* TRANSLATORS: This refers to the position of the cursor. */
    add_to_funcs(report_cursor_position, MMAIN, N_("Location"),
                 WHENHELP(cursorpos_gist), TOGETHER);
    /* Conditionally placing this one here or further on, to keep the
     * help items nicely paired in most conditions. */
    add_to_funcs(do_gotolinecolumn, MMAIN, N_("Go To Line"),
                 WHENHELP(gotoline_gist), BLANKAFTER);
    /* TRANSLATORS : Try to keep the next ten strings at most 12 characters. */
    add_to_funcs(do_undo, MMAIN, N_("Undo"), WHENHELP(undo_gist), TOGETHER);
    add_to_funcs(do_redo, MMAIN, N_("Redo"), WHENHELP(redo_gist), BLANKAFTER);
    add_to_funcs(do_mark, MMAIN, N_("Set Mark"), WHENHELP(mark_gist), TOGETHER);
    add_to_funcs(copy_text, MMAIN, N_("Copy"), WHENHELP(copy_gist), BLANKAFTER);
    add_to_funcs(case_sens_void, MWHEREIS | MREPLACE, N_("Case Sens"),
                 WHENHELP(case_gist), TOGETHER);
    add_to_funcs(regexp_void, MWHEREIS | MREPLACE, N_("Reg.exp."),
                 WHENHELP(regexp_gist), TOGETHER);
    add_to_funcs(backwards_void, MWHEREIS | MREPLACE, N_("Backwards"),
                 WHENHELP(reverse_gist), BLANKAFTER);
    add_to_funcs(flip_replace, MWHEREIS, N_("Replace"), WHENHELP(replace_gist),
                 BLANKAFTER);
    add_to_funcs(flip_replace, MREPLACE, N_("No Replace"),
                 WHENHELP(whereis_gist), BLANKAFTER);
    add_to_funcs(get_older_item,
                 MWHEREIS | MREPLACE | MREPLACEWITH | MWHEREISFILE, N_("Older"),
                 WHENHELP(older_gist), TOGETHER);
    add_to_funcs(get_newer_item,
                 MWHEREIS | MREPLACE | MREPLACEWITH | MWHEREISFILE, N_("Newer"),
                 WHENHELP(newer_gist), BLANKAFTER);
    add_to_funcs(get_older_item, MEXECUTE, N_("Older"),
                 WHENHELP(older_command_gist), TOGETHER);
    add_to_funcs(get_newer_item, MEXECUTE, N_("Newer"),
                 WHENHELP(newer_command_gist), BLANKAFTER);
    /* TRANSLATORS : Try to keep the next four strings at most 10 characters. */
    add_to_funcs(
        goto_dir, MBROWSER, N_("Go To Dir"), WHENHELP(gotodir_gist), TOGETHER);
    add_to_funcs(full_refresh, MBROWSER, N_("Refresh"),
                 WHENHELP(browserrefresh_gist), BLANKAFTER);
    add_to_funcs(do_search_forward, MBROWSER, N_("Where Is"),
                 WHENHELP(browserwhereis_gist), TOGETHER);
    add_to_funcs(do_search_backward, MBROWSER, N_("Where Was"),
                 WHENHELP(browserwherewas_gist), TOGETHER);
    add_to_funcs(do_findprevious, MBROWSER, N_("Previous"),
                 WHENHELP(findprev_gist), TOGETHER);
    add_to_funcs(
        do_findnext, MBROWSER, N_("Next"), WHENHELP(findnext_gist), BLANKAFTER);
    add_to_funcs(do_find_bracket, MMAIN, N_("To Bracket"),
                 WHENHELP(bracket_gist), BLANKAFTER);
    /* TRANSLATORS: This starts a backward search. */
    add_to_funcs(do_search_backward, MMAIN | MHELP, N_("Where Was"),
                 WHENHELP(wherewas_gist), TOGETHER);
    /* TRANSLATORS: This refers to searching the preceding occurrence. */
    add_to_funcs(do_findprevious, MMAIN | MHELP, N_("Previous"),
                 WHENHELP(findprev_gist), TOGETHER);
    add_to_funcs(do_findnext, MMAIN | MHELP, N_("Next"),
                 WHENHELP(findnext_gist), BLANKAFTER);
    /* TRANSLATORS: This means move the cursor one character back. */
    add_to_funcs(do_left, MMAIN, N_("Back"), WHENHELP(back_gist), TOGETHER);
    add_to_funcs(
        do_right, MMAIN, N_("Forward"), WHENHELP(forward_gist), TOGETHER);
    add_to_funcs(
        do_left, MBROWSER, N_("Back"), WHENHELP(backfile_gist), TOGETHER);
    add_to_funcs(do_right, MBROWSER, N_("Forward"), WHENHELP(forwardfile_gist),
                 TOGETHER);
    /* TRANSLATORS: Try to keep the next ten strings at most 12 characters. */
    add_to_funcs(to_prev_word, MMAIN, N_("Prev Word"), WHENHELP(prevword_gist),
                 TOGETHER);
    add_to_funcs(to_next_word, MMAIN, N_("Next Word"), WHENHELP(nextword_gist),
                 TOGETHER);
    add_to_funcs(do_home, MMAIN, N_("Home"), WHENHELP(home_gist), TOGETHER);
    add_to_funcs(do_end, MMAIN, N_("End"), WHENHELP(end_gist), BLANKAFTER);
    add_to_funcs(do_up, MMAIN | MBROWSER | MHELP, N_("Prev Line"),
                 WHENHELP(prevline_gist), TOGETHER);
    add_to_funcs(do_down, MMAIN | MBROWSER | MHELP, N_("Next Line"),
                 WHENHELP(nextline_gist), TOGETHER);
    add_to_funcs(do_scroll_up, MMAIN, N_("Scroll Up"), WHENHELP(scrollup_gist),
                 TOGETHER);
    add_to_funcs(do_scroll_down, MMAIN, N_("Scroll Down"),
                 WHENHELP(scrolldown_gist), BLANKAFTER);
    add_to_funcs(to_prev_block, MMAIN, N_("Prev Block"),
                 WHENHELP(prevblock_gist), TOGETHER);
    add_to_funcs(to_next_block, MMAIN, N_("Next Block"),
                 WHENHELP(nextblock_gist), TOGETHER);
    /* TRANSLATORS: Try to keep these two strings at most 16 characters. */
    add_to_funcs(to_para_begin, MMAIN | MGOTOLINE, N_("Begin of Paragr."),
                 WHENHELP(parabegin_gist), TOGETHER);
    add_to_funcs(to_para_end, MMAIN | MGOTOLINE, N_("End of Paragraph"),
                 WHENHELP(paraend_gist), BLANKAFTER);
    add_to_funcs(
        to_top_row, MMAIN, N_("Top Row"), WHENHELP(toprow_gist), TOGETHER);
    add_to_funcs(to_bottom_row, MMAIN, N_("Bottom Row"),
                 WHENHELP(bottomrow_gist), BLANKAFTER);
    /* TRANSLATORS: Try to keep the next six strings at most 12 characters. */
    add_to_funcs(do_page_up, MMAIN | MHELP, N_("Prev Page"),
                 WHENHELP(prevpage_gist), TOGETHER);
    add_to_funcs(do_page_down, MMAIN | MHELP, N_("Next Page"),
                 WHENHELP(nextpage_gist), TOGETHER);
    add_to_funcs(to_first_line, MMAIN | MHELP | MGOTOLINE, N_("First Line"),
                 WHENHELP(firstline_gist), TOGETHER);
    add_to_funcs(to_last_line, MMAIN | MHELP | MGOTOLINE, N_("Last Line"),
                 WHENHELP(lastline_gist), BLANKAFTER);
    add_to_funcs(switch_to_prev_buffer, MMAIN, N_("Prev File"),
                 WHENHELP(prevfile_gist), TOGETHER);
    add_to_funcs(switch_to_next_buffer, MMAIN, N_("Next File"),
                 WHENHELP(nextfile_gist), BLANKAFTER);
    /* TRANSLATORS: The next four strings are names of keyboard keys. */
    add_to_funcs(do_tab, MMAIN, N_("Tab"), WHENHELP(tab_gist), TOGETHER);
    add_to_funcs(
        do_enter, MMAIN, N_("Enter"), WHENHELP(enter_gist), BLANKAFTER);
    add_to_funcs(do_backspace, MMAIN, N_("Backspace"), WHENHELP(backspace_gist),
                 TOGETHER);
    add_to_funcs(
        do_delete, MMAIN, N_("Delete"), WHENHELP(delete_gist), BLANKAFTER);
    /* TRANSLATORS: The next two strings refer to deleting words. */
    add_to_funcs(chop_previous_word, MMAIN, N_("Chop Left"),
                 WHENHELP(chopwordleft_gist), TOGETHER);
    add_to_funcs(chop_next_word, MMAIN, N_("Chop Right"),
                 WHENHELP(chopwordright_gist), TOGETHER);
    add_to_funcs(cut_till_eof, MMAIN, N_("Cut Till End"),
                 WHENHELP(cuttilleof_gist), BLANKAFTER);
    add_to_funcs(do_full_justify, MMAIN, N_("Full Justify"),
                 WHENHELP(fulljustify_gist), TOGETHER);
    add_to_funcs(count_lines_words_and_characters, MMAIN, N_("Word Count"),
                 WHENHELP(wordcount_gist), TOGETHER);
    add_to_funcs(copy_text, MMAIN, N_("Copy"), WHENHELP(copy_gist), BLANKAFTER);
    add_to_funcs(do_verbatim_input, MMAIN, N_("Verbatim"),
                 WHENHELP(verbatim_gist), BLANKAFTER);
    add_to_funcs(
        do_indent, MMAIN, N_("Indent"), WHENHELP(indent_gist), TOGETHER);
    add_to_funcs(do_unindent, MMAIN, N_("Unindent"), WHENHELP(unindent_gist),
                 BLANKAFTER);
    add_to_funcs(do_comment, MMAIN, N_("Comment Lines"), WHENHELP(comment_gist),
                 TOGETHER);
    add_to_funcs(complete_a_word, MMAIN, N_("Complete"),
                 WHENHELP(completion_gist), BLANKAFTER);
    add_to_funcs(record_macro, MMAIN, N_("Record"), WHENHELP(recordmacro_gist),
                 TOGETHER);
    add_to_funcs(
        run_macro, MMAIN, N_("Run Macro"), WHENHELP(runmacro_gist), BLANKAFTER);
    /* TRANSLATORS: This refers to deleting a line or marked region. */
    add_to_funcs(zap_text, MMAIN, N_("Zap"), WHENHELP(zap_gist), BLANKAFTER);
    add_to_funcs(put_or_lift_anchor, MMAIN, N_("Anchor"), WHENHELP(anchor_gist),
                 TOGETHER);
    add_to_funcs(to_prev_anchor, MMAIN, N_("Up to anchor"),
                 WHENHELP(prevanchor_gist), TOGETHER);
    add_to_funcs(to_next_anchor, MMAIN, N_("Down to anchor"),
                 WHENHELP(nextanchor_gist), BLANKAFTER);
    add_to_funcs(
        do_spell, MMAIN, N_("Spell Check"), WHENHELP(spell_gist), TOGETHER);
    add_to_funcs(do_linter, MMAIN, N_("Linter"), WHENHELP(lint_gist), TOGETHER);
    add_to_funcs(do_formatter, MMAIN, N_("Formatter"), WHENHELP(formatter_gist),
                 BLANKAFTER);
    /* Although not allowed in restricted mode, keep execution rebindable. */
    if (ISSET(RESTRICTED))
    {
        add_to_funcs(
            do_execute, MMAIN, N_("Execute"), WHENHELP(execute_gist), TOGETHER);
    }
    add_to_funcs(
        do_suspend, MMAIN, N_("Suspend"), WHENHELP(suspend_gist), TOGETHER);
    add_to_funcs(
        full_refresh, MMAIN, N_("Refresh"), WHENHELP(refresh_gist), BLANKAFTER);
    add_to_funcs(
        do_center, MMAIN, N_("Center"), WHENHELP(center_gist), TOGETHER);
    add_to_funcs(
        do_cycle, MMAIN, N_("Cycle"), WHENHELP(cycle_gist), BLANKAFTER);
    add_to_funcs(
        do_savefile, MMAIN, N_("Save"), WHENHELP(savefile_gist), BLANKAFTER);
    /* Include the new-buffer toggle only when it can actually be used. */
    if (!ISSET(RESTRICTED) && !ISSET(VIEW_MODE))
    {
        add_to_funcs(flip_newbuffer, MINSERTFILE | MEXECUTE, N_("New Buffer"),
                     WHENHELP(newbuffer_gist), TOGETHER);
    }
    add_to_funcs(
        flip_pipe, MEXECUTE, N_("Pipe Text"), WHENHELP(pipe_gist), BLANKAFTER);
    /* TRANSLATORS: Try to keep the next four strings at most 12 characters. */
    add_to_funcs(
        do_spell, MEXECUTE, N_("Spell Check"), WHENHELP(spell_gist), TOGETHER);
    add_to_funcs(
        do_linter, MEXECUTE, N_("Linter"), WHENHELP(lint_gist), BLANKAFTER);
    add_to_funcs(do_full_justify, MEXECUTE, N_("Full Justify"),
                 WHENHELP(fulljustify_gist), TOGETHER);
    add_to_funcs(do_formatter, MEXECUTE, N_("Formatter"),
                 WHENHELP(formatter_gist), BLANKAFTER);
    add_to_funcs(flip_goto, MWHEREIS, N_("Go To Line"), WHENHELP(gotoline_gist),
                 BLANKAFTER);
    add_to_funcs(flip_goto, MGOTOLINE, N_("Go To Text"), WHENHELP(whereis_gist),
                 BLANKAFTER);
    add_to_funcs(
        dos_format, MWRITEFILE, N_("DOS Format"), WHENHELP(dos_gist), TOGETHER);
    add_to_funcs(
        mac_format, MWRITEFILE, N_("Mac Format"), WHENHELP(mac_gist), TOGETHER);
    /* If we're using restricted mode, the Append, Prepend, and Backup toggles
     * are disabled.  The first and second are not useful as they only allow
     * reduplicating the current file, and the third is not allowed as it
     * would write to a file not specified on the command line. */
    if (!ISSET(RESTRICTED))
    {
        add_to_funcs(append_it, MWRITEFILE, N_("Append"), WHENHELP(append_gist),
                     TOGETHER);
        add_to_funcs(prepend_it, MWRITEFILE, N_("Prepend"),
                     WHENHELP(prepend_gist), TOGETHER);
        add_to_funcs(back_it_up, MWRITEFILE, N_("Backup File"),
                     WHENHELP(backup_gist), BLANKAFTER);
    }
    add_to_funcs(flip_convert, MINSERTFILE, N_("No Conversion"),
                 WHENHELP(convert_gist), BLANKAFTER);
    /* Command execution is only available when not in restricted mode. */
    if (!ISSET(RESTRICTED) && !ISSET(VIEW_MODE))
    {
        add_to_funcs(flip_execute, MINSERTFILE, N_("Execute Command"),
                     WHENHELP(execute_gist), BLANKAFTER);
    }
    add_to_funcs(cut_till_eof, MEXECUTE, N_("Cut Till End"),
                 WHENHELP(cuttilleof_gist), BLANKAFTER);
    add_to_funcs(do_suspend, MEXECUTE, N_("Suspend"), WHENHELP(suspend_gist),
                 BLANKAFTER);
    /* The file browser is only available when not in restricted mode. */
    if (!ISSET(RESTRICTED))
    {
        /* TODO: This invokes the file browser. */
        add_to_funcs(to_files, MWRITEFILE | MINSERTFILE, N_("Browse"),
                     WHENHELP(tofiles_gist), BLANKAFTER);
    }
    add_to_funcs(do_page_up, MBROWSER, N_("Prev Page"), WHENHELP(prevpage_gist),
                 TOGETHER);
    add_to_funcs(do_page_down, MBROWSER, N_("Next Page"),
                 WHENHELP(nextpage_gist), TOGETHER);
    add_to_funcs(to_first_file, MBROWSER | MWHEREISFILE, N_("First File"),
                 WHENHELP(firstfile_gist), TOGETHER);
    add_to_funcs(to_last_file, MBROWSER | MWHEREISFILE, N_("Last File"),
                 WHENHELP(lastfile_gist), BLANKAFTER);
    add_to_funcs(to_prev_word, MBROWSER, N_("Left Column"),
                 WHENHELP(browserlefthand_gist), TOGETHER);
    add_to_funcs(to_next_word, MBROWSER, N_("Right Column"),
                 WHENHELP(browserrighthand_gist), TOGETHER);
    add_to_funcs(to_prev_block, MBROWSER, N_("Top Row"),
                 WHENHELP(browsertoprow_gist), TOGETHER);
    add_to_funcs(to_next_block, MBROWSER, N_("Bottom Row"),
                 WHENHELP(browserbottomrow_gist), BLANKAFTER);
    add_to_funcs(discard_buffer, MWRITEFILE, N_("Discard buffer"),
                 WHENHELP(discardbuffer_gist), BLANKAFTER);
    /* TRANSLATORS: The next two strings may be up to 37 characters each. */
    add_to_funcs(do_page_up, MLINTER, N_("Previous Linter message"),
                 WHENHELP(prevlint_gist), TOGETHER);
    add_to_funcs(do_page_down, MLINTER, N_("Next Linter message"),
                 WHENHELP(nextlint_gist), TOGETHER);
#define SLASH_OR_DASH (on_a_vt) ? "^-" : "^/"
    /* Link key combos to functions in certain menus. */
    add_to_sclist(MMOST | MBROWSER, "^M", '\r', do_enter, 0);
    add_to_sclist(MMOST | MBROWSER, "Enter", KEY_ENTER, do_enter, 0);
    add_to_sclist(MMOST, "^I", '\t', do_tab, 0);
    /* add_to_sclist(MMOST, "^I", 0, do_test_window, 0); */
    add_to_sclist(MMOST, "Tab", '\t', do_tab, 0);
    add_to_sclist(MMAIN | MBROWSER | MHELP, "^B", 0, do_search_backward, 0);
    add_to_sclist(MMAIN | MBROWSER | MHELP, "^F", 0, do_search_forward, 0);
    if (ISSET(MODERN_BINDINGS))
    {
        add_to_sclist(
            (MMOST | MBROWSER) & ~MFINDINHELP, help_key, 0, do_help, 0);
        add_to_sclist(MHELP, help_key, 0, do_exit, 0);
        add_to_sclist(MMAIN | MBROWSER | MHELP, "^Q", 0, do_exit, 0);
        add_to_sclist(MMAIN, "^S", 0, do_savefile, 0);
        add_to_sclist(MMAIN, "^W", 0, do_writeout, 0);
        add_to_sclist(MMAIN, "^O", 0, do_insertfile, 0);
        add_to_sclist(MMAIN | MBROWSER | MHELP, "^D", 0, do_findprevious, 0);
        add_to_sclist(MMAIN | MBROWSER | MHELP, "^G", 0, do_findnext, 0);
        add_to_sclist(MMAIN, "^R", 0, do_replace, 0);
        add_to_sclist(MMAIN, "^T", 0, do_gotolinecolumn, 0);
        /* add_to_sclist(MMAIN, "^P", 0, report_cursor_position, 0); */
        add_to_sclist(MMAIN, "^P", 0, do_close_bracket, 0);
        add_to_sclist(MMAIN, "^Z", 0, do_undo, 0);
        add_to_sclist(MMAIN, "^Y", 0, do_redo, 0);
        /* add_to_sclist(MMAIN, "^A", 0, do_mark, 0); */
        add_to_sclist(MMAIN, "^A", 0, do_block_comment, 0);
        add_to_sclist(MMAIN, "^X", 0, cut_text, 0);
        add_to_sclist(MMAIN, "^C", 0, copy_text, 0);
        add_to_sclist(MMAIN, "^V", 0, paste_text, 0);
    }
    else
    {
        add_to_sclist((MMOST | MBROWSER) & ~MFINDINHELP, "^G", 0, do_help, 0);
        add_to_sclist(MMAIN | MBROWSER | MHELP, "^X", 0, do_exit, 0);
        if (!ISSET(PRESERVE))
        {
            add_to_sclist(MMAIN, "^S", 0, do_savefile, 0);
        }
        add_to_sclist(MMAIN, "^O", 0, do_writeout, 0);
        add_to_sclist(MMAIN, "^R", 0, do_insertfile, 0);
        if (!ISSET(PRESERVE))
        {
            add_to_sclist(
                MMAIN | MBROWSER | MHELP, "^Q", 0, do_search_backward, 0);
        }
        add_to_sclist(MMAIN | MBROWSER | MHELP, "^W", 0, do_search_forward, 0);
        add_to_sclist(MMOST, "^A", 0, do_home, 0);
        add_to_sclist(MMOST, "^E", 0, do_end, 0);
        add_to_sclist(MMAIN | MBROWSER | MHELP, "^P", 0, do_up, 0);
        add_to_sclist(MMAIN | MBROWSER | MHELP, "^N", 0, do_down, 0);
        add_to_sclist(
            MMAIN | MBROWSER | MHELP | MLINTER, "^Y", 0, do_page_up, 0);
        add_to_sclist(
            MMAIN | MBROWSER | MHELP | MLINTER, "^V", 0, do_page_down, 0);
        add_to_sclist(MMAIN, "^C", 0, report_cursor_position, 0);
        add_to_sclist(MMOST, "^H", '\b', do_backspace, 0);
        add_to_sclist(MMOST, "^D", 0, do_delete, 0);
    }
    add_to_sclist(MMOST, "Bsp", KEY_BACKSPACE, do_backspace, 0);
    add_to_sclist(MMOST, "Sh-Del", SHIFT_DELETE, do_backspace, 0);
    add_to_sclist(MMOST, "Del", KEY_DC, do_delete, 0);
    add_to_sclist(MMAIN, "Ins", KEY_IC, do_insertfile, 0);
    add_to_sclist(MMAIN, "^\\", 0, do_replace, 0);
    add_to_sclist(MMAIN, "M-R", 0, do_replace, 0);
    add_to_sclist(MMOST, "^K", 0, cut_text, 0);
    add_to_sclist(MMOST, "M-6", 0, copy_text, 0);
    add_to_sclist(MMOST, "M-^", 0, copy_text, 0);
    add_to_sclist(MMOST, "^U", 0, paste_text, 0);
    add_to_sclist(
        MMAIN, ISSET(MODERN_BINDINGS) ? "^E" : "^T", 0, do_execute, 0);
    if (!ISSET(PRESERVE))
    {
        add_to_sclist(MEXECUTE, "^S", 0, do_spell, 0);
    }
    add_to_sclist(MEXECUTE, "^T", 0, do_spell, 0);
    add_to_sclist(MMAIN, "^J", '\n', do_justify, 0);
    add_to_sclist(MEXECUTE, "^Y", 0, do_linter, 0);
    add_to_sclist(MEXECUTE, "^O", 0, do_formatter, 0);
    add_to_sclist(MMAIN, "M-3", 0, do_gotolinecolumn, 0);
    add_to_sclist(MMAIN, "M-G", 0, do_gotolinecolumn, 0);
    /* add_to_sclist(MMAIN, "^_", 0, do_gotolinecolumn, 0); */
    add_to_sclist(
        MMAIN | MBROWSER | MHELP | MLINTER, "PgUp", KEY_PPAGE, do_page_up, 0);
    add_to_sclist(
        MMAIN | MBROWSER | MHELP | MLINTER, "PgDn", KEY_NPAGE, do_page_down, 0);
    add_to_sclist(MBROWSER | MHELP, "Bsp", KEY_BACKSPACE, do_page_up, 0);
    add_to_sclist(MBROWSER | MHELP, "Sh-Del", SHIFT_DELETE, do_page_up, 0);
    add_to_sclist(MBROWSER | MHELP, "Space", 0x20, do_page_down, 0);
    add_to_sclist(MMAIN | MHELP, "M-\\", 0, to_first_line, 0);
    add_to_sclist(MMAIN | MHELP, "^Home", CONTROL_HOME, to_first_line, 0);
    add_to_sclist(MMAIN | MHELP, "M-/", 0, to_last_line, 0);
    add_to_sclist(MMAIN | MHELP, "^End", CONTROL_END, to_last_line, 0);
    add_to_sclist(MMAIN | MBROWSER | MHELP, "M-B", 0, do_findprevious, 0);
    add_to_sclist(MMAIN | MBROWSER | MHELP, "M-F", 0, do_findnext, 0);
    add_to_sclist(MMAIN | MBROWSER | MHELP, "M-W", 0, do_findnext, 0);
    add_to_sclist(MMAIN | MBROWSER | MHELP, "M-Q", 0, do_findprevious, 0);
    add_to_sclist(MMAIN, "M-]", 0, do_find_bracket, 0);
    add_to_sclist(MMAIN, "M-A", 0, do_mark, 0);
    add_to_sclist(MMAIN, "^6", 0, do_mark, 0);
    /* add_to_sclist(MMAIN, "^^", 0, do_mark, 0); */
    add_to_sclist(MMAIN, "M-}", 0, do_indent, 0);
    add_to_sclist(MMAIN, "M-{", 0, do_unindent, 0);
    add_to_sclist(MMAIN, "Sh-Tab", SHIFT_TAB, do_unindent, 0);
    add_to_sclist(MMAIN, "M-:", 0, record_macro, 0);
    add_to_sclist(MMAIN, "M-;", 0, run_macro, 0);
    add_to_sclist(MMAIN, "M-U", 0, do_undo, 0);
    add_to_sclist(MMAIN, "M-E", 0, do_redo, 0);
    add_to_sclist(MMAIN, "M-Bsp", CONTROL_SHIFT_DELETE, chop_previous_word, 0);
    add_to_sclist(
        MMAIN, "Sh-^Del", CONTROL_SHIFT_DELETE, chop_previous_word, 0);
    add_to_sclist(MMAIN, "^Bsp", CONTROL_BSP, chop_previous_word, 0);
    add_to_sclist(MMAIN, "^Del", CONTROL_DELETE, chop_next_word, 0);
    add_to_sclist(MMAIN, "M-Del", ALT_DELETE, zap_text, 0);
    add_to_sclist(MMAIN, "M-Ins", ALT_INSERT, put_or_lift_anchor, 0);
    add_to_sclist(MMAIN, "M-Home", ALT_HOME, to_top_row, 0);
    add_to_sclist(MMAIN, "M-End", ALT_END, to_bottom_row, 0);
    add_to_sclist(MMAIN, "M-PgUp", ALT_PAGEUP, to_prev_anchor, 0);
    add_to_sclist(MMAIN, "M-PgDn", ALT_PAGEDOWN, to_next_anchor, 0);
    add_to_sclist(MMAIN, "M-\"", 0, put_or_lift_anchor, 0);
    add_to_sclist(MMAIN, "M-'", 0, to_next_anchor, 0);
    add_to_sclist(MMAIN, "^]", 0, complete_a_word, 0);
    add_to_sclist(MMAIN, (on_a_vt) ? "^-" : "^/", 0, do_comment, 0);
    add_to_sclist(MMAIN, "^_", 0, do_comment, 0);
    add_to_sclist(MMAIN, "Sh-^/", 0, do_block_comment, 0);
    add_to_sclist(MMOST & ~MMAIN, "^B", 0, do_left, 0);
    add_to_sclist(MMOST & ~MMAIN, "^F", 0, do_right, 0);
    if (using_utf8())
    {
        add_to_sclist(
            MMOST | MBROWSER | MHELP, "\xE2\x97\x82", KEY_LEFT, do_left, 0);
        add_to_sclist(
            MMOST | MBROWSER | MHELP, "\xE2\x96\xb8", KEY_RIGHT, do_right, 0);
        add_to_sclist(MSOME, "^\xE2\x97\x82", CONTROL_LEFT, to_prev_word, 0);
        add_to_sclist(MSOME, "^\xE2\x96\xb8", CONTROL_RIGHT, to_next_word, 0);
        if (!on_a_vt)
        {
            add_to_sclist(
                MMAIN, "M-\xE2\x97\x82", ALT_LEFT, switch_to_prev_buffer, 0);
            add_to_sclist(
                MMAIN, "M-\xE2\x96\xb8", ALT_RIGHT, switch_to_next_buffer, 0);
        }
    }
    else
    {
        add_to_sclist(MMOST | MBROWSER | MHELP, "Left", KEY_LEFT, do_left, 0);
        add_to_sclist(
            MMOST | MBROWSER | MHELP, "Right", KEY_RIGHT, do_right, 0);
        add_to_sclist(MSOME, "^Left", CONTROL_LEFT, to_prev_word, 0);
        add_to_sclist(MSOME, "^Right", CONTROL_RIGHT, to_next_word, 0);
        if (!on_a_vt)
        {
            add_to_sclist(MMAIN, "M-Left", ALT_LEFT, switch_to_prev_buffer, 0);
            add_to_sclist(
                MMAIN, "M-Right", ALT_RIGHT, switch_to_next_buffer, 0);
        }
    }
    add_to_sclist(MMOST, "M-Space", 0, to_prev_word, 0);
    add_to_sclist(MMOST, "^Space", 0, to_next_word, 0);
    add_to_sclist(MMOST, "Home", KEY_HOME, do_home, 0);
    add_to_sclist(MMOST, "End", KEY_END, do_end, 0);
    if (using_utf8())
    {
        add_to_sclist(
            MMAIN | MBROWSER | MHELP, "\xE2\x96\xb4", KEY_UP, do_up, 0);
        add_to_sclist(
            MMAIN | MBROWSER | MHELP, "\xE2\x96\xbe", KEY_DOWN, do_down, 0);
        add_to_sclist(MMAIN | MBROWSER | MLINTER, "^\xE2\x96\xb4", CONTROL_UP,
                      to_prev_block, 0);
        add_to_sclist(MMAIN | MBROWSER | MLINTER, "^\xE2\x96\xbe", CONTROL_DOWN,
                      to_next_block, 0);
    }
    else
    {
        add_to_sclist(MMAIN | MBROWSER | MHELP, "Up", KEY_UP, do_up, 0);
        add_to_sclist(MMAIN | MBROWSER | MHELP, "Down", KEY_DOWN, do_down, 0);
        add_to_sclist(
            MMAIN | MBROWSER | MLINTER, "^Up", CONTROL_UP, to_prev_block, 0);
        add_to_sclist(MMAIN | MBROWSER | MLINTER, "^Down", CONTROL_DOWN,
                      to_next_block, 0);
    }
    add_to_sclist(MMAIN, "M-7", 0, to_prev_block, 0);
    add_to_sclist(MMAIN, "M-8", 0, to_next_block, 0);
    add_to_sclist(MMAIN, "M-(", 0, to_para_begin, 0);
    add_to_sclist(MMAIN, "M-9", 0, to_para_begin, 0);
    add_to_sclist(MMAIN, "M-)", 0, to_para_end, 0);
    add_to_sclist(MMAIN, "M-0", 0, to_para_end, 0);
    if (using_utf8())
    {
        add_to_sclist(
            MMAIN | MHELP, "M-\xE2\x96\xb4", ALT_UP, move_lines_up, 0);
        add_to_sclist(
            MMAIN | MHELP, "M-\xE2\x96\xbe", ALT_DOWN, move_lines_down, 0);
    }
    else
    {
        add_to_sclist(MMAIN | MHELP, "M-Up", ALT_UP, do_scroll_up, 0);
        add_to_sclist(MMAIN | MHELP, "M-Down", ALT_DOWN, do_scroll_down, 0);
    }
    add_to_sclist(MMAIN | MHELP, "M--", 0, do_scroll_up, 0);
    add_to_sclist(MMAIN | MHELP, "M-_", 0, do_scroll_up, 0);
    add_to_sclist(MMAIN | MHELP, "M-+", 0, do_scroll_down, 0);
    add_to_sclist(MMAIN | MHELP, "M-=", 0, do_scroll_down, 0);
    add_to_sclist(MMAIN, "M-,", 0, switch_to_prev_buffer, 0);
    add_to_sclist(MMAIN, "M-<", 0, switch_to_prev_buffer, 0);
    add_to_sclist(MMAIN, "M-.", 0, switch_to_next_buffer, 0);
    add_to_sclist(MMAIN, "M->", 0, switch_to_next_buffer, 0);
    add_to_sclist(MMOST, "M-V", 0, do_verbatim_input, 0);
    add_to_sclist(MMAIN, "M-T", 0, cut_till_eof, 0);
    add_to_sclist(MEXECUTE, "^V", 0, cut_till_eof, 0);
    add_to_sclist(MEXECUTE, "^Z", 0, do_suspend, 0);
    add_to_sclist(MMAIN, "^Z", 0, suggest_ctrlT_ctrlZ, 0);
    add_to_sclist(MMAIN, "M-D", 0, count_lines_words_and_characters, 0);
    add_to_sclist(MMAIN, "M-H", 0, do_help, 0);
    add_to_sclist(MMAIN, "M-J", 0, do_full_justify, 0);
    add_to_sclist(MEXECUTE, "^J", 0, do_full_justify, 0);
    add_to_sclist(MMAIN, "^L", 0, do_cycle, 0);
    add_to_sclist(MMOST | MBROWSER | MHELP | MYESNO, "^L", 0, full_refresh, 0);
    /* Group of "Appearance" toggles. */
    add_to_sclist(MMAIN, "M-Z", 0, do_toggle, ZERO);
    add_to_sclist((MMOST | MBROWSER | MYESNO) & ~MFINDINHELP, "M-X", 0,
                  do_toggle, NO_HELP);
    add_to_sclist(MMAIN, "M-C", 0, do_toggle, CONSTANT_SHOW);
    add_to_sclist(MMAIN, "M-S", 0, do_toggle, SOFTWRAP);
    add_to_sclist(MMAIN, "M-$", 0, do_toggle, SOFTWRAP);
    /* Legacy keystroke. */
    add_to_sclist(MMAIN, "M-N", 0, do_toggle, LINE_NUMBERS);
    /* Legacy keystroke. */
    add_to_sclist(MMAIN, "M-#", 0, do_toggle, LINE_NUMBERS);
    add_to_sclist(MMAIN, "M-P", 0, do_toggle, WHITESPACE_DISPLAY);
    add_to_sclist(MMAIN, "M-Y", 0, do_toggle, NO_SYNTAX);
    /* Group of 'Behavior' toggles. */
    add_to_sclist(MMAIN, "M-H", 0, do_toggle, SMART_HOME);
    add_to_sclist(MMAIN, "M-I", 0, do_toggle, AUTOINDENT);
    add_to_sclist(MMAIN, "M-K", 0, do_toggle, CUT_FROM_CURSOR);
    add_to_sclist(MMAIN, "M-L", 0, do_toggle, BREAK_LONG_LINES);
    add_to_sclist(MMAIN, "M-O", 0, do_toggle, TABS_TO_SPACES);
    add_to_sclist(MMAIN, "M-M", 0, do_toggle, USE_MOUSE);
    add_to_sclist(((MMOST & ~MMAIN) | MYESNO), "^C", 0, do_cancel, 0);
    add_to_sclist(MWHEREIS | MREPLACE, "M-C", 0, case_sens_void, 0);
    add_to_sclist(MWHEREIS | MREPLACE, "M-R", 0, regexp_void, 0);
    add_to_sclist(MWHEREIS | MREPLACE, "M-B", 0, backwards_void, 0);
    add_to_sclist(MWHEREIS | MREPLACE, "^R", 0, flip_replace, 0);
    add_to_sclist(MWHEREIS | MGOTOLINE, "^T", 0, flip_goto, 0);
    add_to_sclist(MWHEREIS | MGOTOLINE, SLASH_OR_DASH, 0, flip_goto, 0);
    add_to_sclist(MWHEREIS | MREPLACE | MREPLACEWITH | MWHEREISFILE |
                      MFINDINHELP | MEXECUTE,
                  "^P", 0, get_older_item, 0);
    add_to_sclist(MWHEREIS | MREPLACE | MREPLACEWITH | MWHEREISFILE |
                      MFINDINHELP | MEXECUTE,
                  "^N", 0, get_newer_item, 0);
    if (using_utf8())
    {
        add_to_sclist(MWHEREIS | MREPLACE | MREPLACEWITH | MWHEREISFILE |
                          MFINDINHELP | MEXECUTE,
                      "\xE2\x96\xb4", KEY_UP, get_older_item, 0);
        add_to_sclist(MWHEREIS | MREPLACE | MREPLACEWITH | MWHEREISFILE |
                          MFINDINHELP | MEXECUTE,
                      "\xE2\x96\xbe", KEY_DOWN, get_newer_item, 0);
    }
    else
    {
        add_to_sclist(MWHEREIS | MREPLACE | MREPLACEWITH | MWHEREISFILE |
                          MFINDINHELP | MEXECUTE,
                      "Up", KEY_UP, get_older_item, 0);
        add_to_sclist(MWHEREIS | MREPLACE | MREPLACEWITH | MWHEREISFILE |
                          MFINDINHELP | MEXECUTE,
                      "Down", KEY_DOWN, get_newer_item, 0);
    }
    add_to_sclist(MGOTOLINE, "^W", 0, to_para_begin, 0);
    add_to_sclist(MGOTOLINE, "^O", 0, to_para_end, 0);
    /* Some people are used to having these keystrokes in the Search menu. */
    add_to_sclist(
        MGOTOLINE | MWHEREIS | MFINDINHELP, "^Y", 0, to_first_line, 0);
    add_to_sclist(MGOTOLINE | MWHEREIS | MFINDINHELP, "^V", 0, to_last_line, 0);
    add_to_sclist(MWHEREISFILE, "^Y", 0, to_first_file, 0);
    add_to_sclist(MWHEREISFILE, "^V", 0, to_last_file, 0);
    add_to_sclist(MBROWSER | MWHEREISFILE, "M-\\", 0, to_first_file, 0);
    add_to_sclist(MBROWSER | MWHEREISFILE, "M-/", 0, to_last_file, 0);
    add_to_sclist(MBROWSER, "Home", KEY_HOME, to_first_file, 0);
    add_to_sclist(MBROWSER, "End", KEY_END, to_last_file, 0);
    add_to_sclist(MBROWSER, "^Home", CONTROL_HOME, to_first_file, 0);
    add_to_sclist(MBROWSER, "^End", CONTROL_END, to_last_file, 0);
    add_to_sclist(MBROWSER, SLASH_OR_DASH, 0, goto_dir, 0);
    add_to_sclist(MBROWSER, "M-G", 0, goto_dir, 0);
    add_to_sclist(MBROWSER, "^_", 0, goto_dir, 0);
    if (ISSET(SAVE_ON_EXIT) && !ISSET(PRESERVE))
    {
        add_to_sclist(MWRITEFILE, "^Q", 0, discard_buffer, 0);
    }
    add_to_sclist(MWRITEFILE, "M-D", 0, dos_format, 0);
    add_to_sclist(MWRITEFILE, "M-M", 0, mac_format, 0);
    /* Only when not in restricted mode, allow Appending, Prepending,
     * making backups, and executing a command. */
    if (!ISSET(RESTRICTED) && !ISSET(VIEW_MODE))
    {
        add_to_sclist(MWRITEFILE, "M-A", 0, append_it, 0);
        add_to_sclist(MWRITEFILE, "M-P", 0, prepend_it, 0);
        add_to_sclist(MWRITEFILE, "M-B", 0, back_it_up, 0);
        add_to_sclist(MINSERTFILE | MEXECUTE, "^X", 0, flip_execute, 0);
    }
    add_to_sclist(MINSERTFILE, "M-N", 0, flip_convert, 0);
    if (!ISSET(RESTRICTED) && !ISSET(VIEW_MODE))
    {
        add_to_sclist(MINSERTFILE | MEXECUTE, "M-F", 0, flip_newbuffer, 0);
        add_to_sclist(MEXECUTE, "M-\\", 0, flip_pipe, 0);
    }
    add_to_sclist(MBROWSER | MHELP, "^C", 0, do_exit, 0);
    /* Only when not in restricted mode, allow entering the file browser. */
    if (!ISSET(RESTRICTED))
    {
        add_to_sclist(MWRITEFILE | MINSERTFILE, "^T", 0, to_files, 0);
    }
    /* Allow exiting the file browser with the same key as used for entry. */
    add_to_sclist(MBROWSER, "^T", 0, do_exit, 0);
    /* Allow exiting the help viewer with the same keys as used for entry. */
    add_to_sclist(MHELP, "^G", 0, do_exit, 0);
    add_to_sclist(MHELP, "F1", KEY_F(1), do_exit, 0);
    add_to_sclist(MHELP, "Home", KEY_HOME, to_first_line, 0);
    add_to_sclist(MHELP, "End", KEY_END, to_last_line, 0);
    add_to_sclist(MLINTER, "^X", 0, do_cancel, 0);
    add_to_sclist(MMOST & ~MFINDINHELP, "F1", KEY_F(1), do_help, 0);
    add_to_sclist(MMAIN | MBROWSER | MHELP, "F2", KEY_F(2), do_exit, 0);
    add_to_sclist(MMAIN, "F3", KEY_F(3), do_writeout, 0);
    add_to_sclist(MMAIN, "F4", KEY_F(4), do_justify, 0);
    add_to_sclist(MMAIN, "F5", KEY_F(5), do_insertfile, 0);
    add_to_sclist(
        MMAIN | MBROWSER | MHELP, "F6", KEY_F(6), do_search_forward, 0);
    add_to_sclist(
        MMAIN | MBROWSER | MHELP | MLINTER, "F7", KEY_F(7), do_page_up, 0);
    add_to_sclist(
        MMAIN | MBROWSER | MHELP | MLINTER, "F8", KEY_F(8), do_page_down, 0);
    add_to_sclist(MMOST, "F9", KEY_F(9), cut_text, 0);
    add_to_sclist(MMOST, "F10", KEY_F(10), paste_text, 0);
    add_to_sclist(MMAIN, "F11", KEY_F(11), report_cursor_position, 0);
    add_to_sclist(MMAIN, "F12", KEY_F(12), do_spell, 0);
    add_to_sclist(MMAIN, "M-&", 0, show_curses_version, 0);
    add_to_sclist((MMOST & ~MMAIN) | MYESNO, "", KEY_CANCEL, do_cancel, 0);
    add_to_sclist(MMAIN, "", KEY_SIC, do_insertfile, 0);
    /* Catch and ignore bracketed paste marker keys. */
    add_to_sclist(MMOST | MBROWSER | MHELP | MYESNO, "", BRACKETED_PASTE_MARKER,
                  do_nothing, 0);
}

/* Return the textual description that corresponds to the given flag. */
const char *
epithet_of_flag(const unsigned int flag)
{
    return &epithetOfFlagMap[flag].value[0];
}

/* Add 'path' to 'handles_include' vector. */
void
add_to_handled_includes_vec(const char *path)
{
    handled_includes.push_back(path);
}

/* Return`s 'TRUE' if 'path' is found in 'handles_includes' vector. */
bool
is_in_handled_includes_vec(std::string_view path)
{
    for (const auto &p : handled_includes)
    {
        if (p == path)
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool
syntax_var(std::string_view str)
{
    for (const auto &var : syntax_vars)
    {
        if (str == var)
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool
syntax_func(std::string_view str)
{
    for (int i = 0; i < funcs.get_size(); i++)
    {
        if (strcmp(&str[0], funcs[i]) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void
new_syntax_var(const char *str)
{
    syntax_vars.push_back(str);
}

void
new_syntax_func(const char *str)
{
    syntax_funcs.push_back(str);
}
