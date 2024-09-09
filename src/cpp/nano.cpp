#include "../include/prototypes.h"
#include "../include/revision.h"

#include <Mlib/Debug.h>
#include <Mlib/Profile.h>
#include <Mlib/def.h>

#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#ifdef ENABLE_UTF8
#    include <langinfo.h>
#endif
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <sys/vt.h>
#include <termios.h>
#include <unistd.h>

/* Used to store the user's original mouse click interval. */
static int oldinterval = -1;
/* The original settings of the user's terminal. */
static termios original_state;
/* Containers for the original and the temporary handler for SIGINT. */
static struct sigaction oldaction, newaction;

main_thread_t *main_thread = NULL;

/* Create a new linestruct node.  Note that we do NOT set 'prevnode->next'.
 * prevnode: previous node in the linked list.
 * returns ( linestruct * ) - pointer to the new node. */
linestruct *
make_new_node(linestruct *prevnode)
{
    linestruct *newnode = (linestruct *)nmalloc(sizeof(linestruct));
    newnode->prev       = prevnode;
    newnode->next       = NULL;
    newnode->data       = NULL;
    newnode->multidata  = NULL;
    newnode->lineno     = (prevnode) ? prevnode->lineno + 1 : 1;
    newnode->has_anchor = FALSE;
    UNSET_ALL_LINE_FLAGS(newnode);
    if (prevnode != NULL)
    {
        (LINE_ISSET(prevnode, IN_BLOCK_COMMENT) ||
         LINE_ISSET(prevnode, BLOCK_COMMENT_START)) ?
            LINE_SET(newnode, IN_BLOCK_COMMENT) :
            LINE_UNSET(newnode, IN_BLOCK_COMMENT);
        (LINE_ISSET(prevnode, IN_BRACKET) ||
         LINE_ISSET(prevnode, BRACKET_START)) ?
            LINE_SET(newnode, IN_BRACKET) :
            0;
    }
    return newnode;
}

/* Splice a new node into an existing linked list of linestructs. */
void
splice_node(linestruct *afterthis, linestruct *newnode)
{
    newnode->next = afterthis->next;
    newnode->prev = afterthis;
    if (afterthis->next != NULL)
    {
        afterthis->next->prev = newnode;
    }
    afterthis->next = newnode;
    /* Update filebot when inserting a node at the end of file. */
    if (openfile && openfile->filebot == afterthis)
    {
        openfile->filebot = newnode;
    }
}

/* Free the data structures in the given node */
void
delete_node(linestruct *line)
{
    /* If the first line on the screen gets deleted, step one back. */
    if (line == openfile->edittop)
    {
        openfile->edittop = line->prev;
    }
    /* If the spill-over line for hard-wrapping is deleted... */
    if (line == openfile->spillage_line)
    {
        openfile->spillage_line = NULL;
    }
    free(line->data);
    free(line->multidata);
    free(line);
}

/* Disconnect a node from a linked list of linestructs and delete it. */
void
unlink_node(linestruct *line)
{
    if (line->prev != NULL)
    {
        line->prev->next = line->next;
    }
    if (line->next != NULL)
    {
        line->next->prev = line->prev;
    }
    /* Update filebot when removing a node at the end of file. */
    if (openfile && openfile->filebot == line)
    {
        openfile->filebot = line->prev;
    }
    delete_node(line);
}

/* Free an entire linked list of linestructs. */
void
free_lines(linestruct *src)
{
    if (src == NULL)
    {
        return;
    }
    while (src->next != NULL)
    {
        src = src->next;
        delete_node(src->prev);
    }
    delete_node(src);
}

/* Make a copy of a linestruct node. */
linestruct *
copy_node(const linestruct *src)
{
    linestruct *dst;
    dst             = (linestruct *)nmalloc(sizeof(linestruct));
    dst->data       = copy_of(src->data);
    dst->multidata  = NULL;
    dst->lineno     = src->lineno;
    dst->has_anchor = src->has_anchor;
    return dst;
}

/* Duplicate an entire linked list of linestructs. */
linestruct *
copy_buffer(const linestruct *src)
{
    linestruct *head, *item;
    head       = copy_node(src);
    head->prev = NULL;
    item       = head;
    src        = src->next;
    while (src != NULL)
    {
        item->next       = copy_node(src);
        item->next->prev = item;
        item             = item->next;
        src              = src->next;
    }
    item->next = NULL;
    return head;
}

/* Renumber the lines in a buffer, from the given line onwards. */
void
renumber_from(linestruct *line)
{
    long number;
    number = (line->prev == NULL) ? 0 : line->prev->lineno;
    while (line != NULL)
    {
        line->lineno = ++number;
        line         = line->next;
    }
}

/* Display a warning about a key disabled in view mode. */
void
print_view_warning(void)
{
    statusline(AHEM, _("Key is invalid in view mode"));
}

/* When in restricted mode, show a warning and return 'TRUE'. */
bool
in_restricted_mode(void)
{
    if ISSET (RESTRICTED)
    {
        statusline(AHEM, _("This function is disabled in restricted mode"));
        beep();
        return TRUE;
    }
    return FALSE;
}

/* Say how the user can achieve suspension (when they typed ^Z). */
void
suggest_ctrlT_ctrlZ(void)
{
    if (first_sc_for(MMAIN, do_execute) &&
        first_sc_for(MMAIN, do_execute)->keycode == 0x14 &&
        first_sc_for(MEXECUTE, do_suspend) &&
        first_sc_for(MEXECUTE, do_suspend)->keycode == 0x1A)
    {
        statusline(AHEM, _("To suspend, type ^T^Z"));
    }
}

/* Make sure the cursor is visible, then exit from curses mode, disable
 * bracketed-paste mode, and restore the original terminal settings. */
void
restore_terminal(void)
{
    curs_set(1);
    endwin();
    printf("\x1B[?2004l");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &original_state);
}

/* Exit normally: restore terminal state and report any startup errors. */
void
finish(void)
{
    /* Blank the status bar and (if applicable) the shortcut list. */
    blank_statusbar();
    blank_bottombars();
    wrefresh(footwin);
    /* Deallocate the two or three subwindows. */
    if (topwin != NULL)
    {
        delwin(topwin);
    }
    delwin(midwin);
    delwin(footwin);
    restore_terminal();
    display_rcfile_errors();
    exit(0);
}

/* Close the current buffer, freeing its memory. */
void
close_and_go(void)
{
    if (openfile->lock_filename)
    {
        delete_lockfile(openfile->lock_filename);
    }
    if (ISSET(POSITIONLOG))
    {
        update_poshistory();
    }
    /* If there is another buffer, close this one.  Otherwise just terminate. */
    if (openfile != openfile->next)
    {
        switch_to_next_buffer();
        openfile = openfile->prev;
        close_buffer();
        openfile = openfile->next;
        /* Adjust the count in the top bar. */
        titlebar(NULL);
    }
    else
    {
        if (ISSET(HISTORYLOG))
        {
            save_history();
        }
        finish();
    }
}

/* Close the current buffer if it is unmodified.  Otherwise (when not doing
 * automatic saving), ask the user whether to save it, then close it and exit,
 * or return when the user cancelled. */
void
do_exit(void)
{
    int choice;
    /* When unmodified, simply close.  Else, when doing automatic saving and the
     * file has a name, simply save.  Otherwise, ask the user. */
    if (!openfile->modified || ISSET(VIEW_MODE))
    {
        choice = NO;
    }
    else if (ISSET(SAVE_ON_EXIT) && openfile->filename[0] != '\0')
    {
        choice = YES;
    }
    else
    {
        if ISSET (SAVE_ON_EXIT)
        {
            warn_and_briefly_pause(_("No file name"));
        }
        choice = ask_user(YESORNO, _("Save modified buffer? "));
    }
    /* When not saving, or the save succeeds, close the buffer. */
    if (choice == NO || (choice == YES && write_it_out(TRUE, TRUE) > 0))
    {
        close_and_go();
    }
    else if (choice != YES)
    {
        statusbar(_("Cancelled"));
    }
}

/* Save the current buffer under the given name (or "nano.<pid>" when nameless)
 * with suffix ".save". If needed, the name is further suffixed to be unique. */
void
emergency_save(const char *filename)
{
    char *plainname, *targetname;
    if (*filename == '\0')
    {
        plainname = (char *)nmalloc(28);
        sprintf(plainname, "nano.%u", getpid());
    }
    else
    {
        plainname = copy_of(filename);
    }
    targetname = get_next_filename(plainname, ".save");
    if (*targetname == '\0')
    {
        fprintf(stderr, "\nToo meny .save files\n");
    }
    else if (write_file(targetname, NULL, SPECIAL, EMERGENCY, NONOTES))
    {
        fprintf(stderr, _("\nBuffer written to %s\n"), targetname);
    }
    free(targetname);
    free(plainname);
}

/* Die gracefully, by restoring the terminal state and, saving any buffers that
 * were modified. */
void
die(STRING_VIEW msg, ...)
{
    openfilestruct *firstone;
    static int      stabs = 0;
    va_list         ap;
    firstone = openfile;
    /* When dying for a second time, just give up. */
    if (++stabs > 1)
    {
        exit(11);
    }
    restore_terminal();
    display_rcfile_errors();
    va_start(ap, msg);
    /* Display the dying message */
    vfprintf(stderr, &msg[0], ap);
    va_end(ap);
    while (openfile)
    {
        /* If the current buffer has a lock file, remove it. */
        if (openfile->lock_filename)
        {
            delete_lockfile(openfile->lock_filename);
        }
        /* When modified, save the current buffer when not when in restricted
         * mode, as it would write a file not mentioned on the command line. */
        if (openfile->modified && !ISSET(RESTRICTED))
        {
            emergency_save(openfile->filename);
        }
        openfile = openfile->next;
        if (openfile == firstone)
        {
            break;
        }
    }
    /* Abandon the building. */
    exit(1);
}

/* Initialize the three window portions nano uses. */
void
window_init(void)
{
    /* When resizing, first delete the existing windows. */
    if (midwin != NULL)
    {
        if (topwin != NULL)
        {
            delwin(topwin);
        }
        delwin(midwin);
        delwin(footwin);
    }
    topwin = NULL;
    /* If the terminal is very flat, don't set up a title bar */
    if (LINES < 3)
    {
        editwinrows = (ISSET(ZERO) ? LINES : 1);
        /* Set up two subwindows.  If the terminal is just one line,
         * edit window and status-bar window will cover each other. */
        midwin  = newwin(editwinrows, COLS, 0, 0);
        footwin = newwin(1, COLS, LINES - 1, 0);
    }
    else
    {
        int minimum    = (ISSET(ZERO) ? 3 : ISSET(MINIBAR) ? 4 : 5);
        int toprows    = ((ISSET(EMPTY_LINE) && LINES > minimum) ? 2 : 1);
        int bottomrows = ((ISSET(NO_HELP) || LINES < minimum) ? 1 : 3);
        if (ISSET(MINIBAR) || ISSET(ZERO))
        {
            toprows = 0;
        }
        editwinrows = LINES - toprows - bottomrows + (ISSET(ZERO) ? 1 : 0);
        /* Set up the normal three subwindows. */
        if (toprows > 0)
        {
            topwin = newwin(toprows, COLS, 0, 0);
        }
        midwin  = newwin(editwinrows, COLS, toprows, 0);
        footwin = newwin(bottomrows, COLS, LINES - bottomrows, 0);
    }
    /* In case the terminal shrunk, make sure the status line is clear. */
    wnoutrefresh(footwin);
    /* When not disabled, turn escape-sequence translation on. */
    if (!ISSET(RAW_SEQUENCES))
    {
        keypad(midwin, TRUE);
        keypad(footwin, TRUE);
    }
    /* Set up the wrapping point, accounting for screen width when negative. */
    if (COLS + fill < 0)
    {
        wrap_at = 0;
    }
    else if (fill <= 0)
    {
        wrap_at = COLS + fill;
    }
    else
    {
        wrap_at = fill;
    }
}

void
disable_mouse_support(void)
{
    mousemask(0, NULL);
    mouseinterval(oldinterval);
}

void
enable_mouse_support(void)
{
    mousemask(ALL_MOUSE_EVENTS, NULL);
    oldinterval = mouseinterval(50);
}

/* Switch mouse support on or off, as needed. */
void
mouse_init(void)
{
    if ISSET (USE_MOUSE)
    {
        enable_mouse_support();
    }
    else
    {
        disable_mouse_support();
    }
}

/* Print the usage line for the given option to the screen. */
void
print_opt(const char *const shortflag, const char *const longflag,
          const char *const description)
{
    const int firstwidth  = breadth(shortflag);
    const int secondwidth = breadth(longflag);
    printf(" %s", shortflag);
    if (firstwidth < 14)
    {
        printf("%*s", 14 - firstwidth, " ");
    }
    printf(" %s", longflag);
    if (secondwidth < 24)
    {
        printf("%*s", 24 - secondwidth, " ");
    }
    printf("%s\n", _(description));
}

/* Explain how to properly use NanoX and its command-line options. */
void
usage(void)
{
    printf(
        _("Usage: %s [OPTIONS] [[+LINE[,COLUMN]] FILE]...\n\n"), PROJECT_NAME);
    /* TRANSLATORS: The next two strings are part of the --help output.
     * It's best to keep its lines within 80 characters. */
    printf(_("To place the cursor on a specific line of a file, put the line "
             "number with\n"
             "a '+' before the filename.  The column number can be added after "
             "a comma.\n"));
    /* TRANSLATORS: The next three are column headers of the --help output. */
    printf(
        _("When a filename is '-', nano reads data from standard input.\n\n"));
    print_opt(_("Option"), _("Long option"), N_("Meaning"));
    /* TRANSLATORS: The next forty or so strings are option descriptions
     * for the --help output.  Try to keep them at most 40 characters. */
    print_opt("-A", "--smarthome", N_("Enable smart home key"));
    if (!ISSET(RESTRICTED))
    {
        print_opt("-B", "--backup", N_("Save backups of existing files"));
        print_opt(_("-C <dir>"), _("--backupdir=<dir>"),
                  N_("Directory for saving unique backup files"));
    }
    print_opt("-D", "--boldtext", N_("Use bold instead of reverse video text"));
    print_opt("-E", "--tabstospaces", N_("Convert typed tabs to spaces"));
    if (!ISSET(RESTRICTED))
    {
        print_opt("-F", "--multibuffer",
                  N_("Read a file into a new buffer by default"));
    }
    print_opt("-G", "--locking", N_("Use (vim-style) lock files"));
    if (!ISSET(RESTRICTED))
    {
        print_opt("-H", "--historylog",
                  N_("Save & reload old search/replace strings"));
    }
    print_opt("-I", "--ignorercfiles", N_("Don't look at nanorc files"));
    print_opt(_("-J <number>"), _("--guidestripe=<number>"),
              N_("Show a guiding bar at this column"));
    print_opt(
        "-K", "--rawsequences", N_("Fix numeric keypad key confusion problem"));
    print_opt("-L", "--nonewlines", N_("Don't add an automatic newline"));
    print_opt("-M", "--trimblanks", N_("Trim tail spaces when hard-wrapping"));
    print_opt(
        "-N", "--noconvert", N_("Don't convert files from DOS/Mac format"));
    print_opt(
        "-O", "--bookstyle", N_("Leading whitespace means new paragraph"));
    if (!ISSET(RESTRICTED))
    {
        print_opt(
            "-P", "--positionlog", N_("Save & restore position of the cursor"));
    }
    /* TRANSLATORS: This refers to email quoting, like the > in: > quoted text.
     */
    print_opt(_("-Q <regex>"), _("--quotestr=<regex>"),
              N_("Regular expression to match quoting"));
    if (!ISSET(RESTRICTED))
    {
        print_opt(
            "-R", "--restricted", N_("Restrict access to the filesystem"));
    }
    print_opt(
        "-S", "--softwrap", N_("Display overlong lines on multiple rows"));
    print_opt(_("-T <number>"), _("--tabsize=<number>"),
              N_("Make a tab this number of columns wide"));
    print_opt("-U", "--quickblank", N_("Wipe status bar upon next keystroke"));
    print_opt("-V", "--version", N_("Print version information and exit"));
    print_opt(
        "-W", "--wordbounds", N_("Detect word boundaries more accurately"));
    print_opt(_("-X <string>"), _("--wordchars=<string>"),
              N_("Which other characters are word parts"));
    print_opt(_("-Y <name>"), _("--syntax=<name>"),
              N_("Syntax definition to use for coloring"));
    print_opt("-Z", "--zap", N_("Let Bsp and Del erase a marked region"));
    print_opt(
        "-a", "--atblanks", N_("When soft-wrapping, do it at whitespace"));
    print_opt(
        "-b", "--breaklonglines", N_("Automatically hard-wrap overlong lines"));
    print_opt("-c", "--constantshow", N_("Constantly show cursor position"));
    print_opt(
        "-d", "--rebinddelete", N_("Fix Backspace/Delete confusion problem"));
    print_opt(
        "-e", "--emptyline", N_("Keep the line below the title bar empty"));
    print_opt(_("-f <file>"), _("--rcfile=<file>"),
              N_("Use only this file for configuring nano"));
    print_opt(
        "-g", "--showcursor", N_("Show cursor in file browser & help text"));
    print_opt("-h", "--help", N_("Show this help text and exit"));
    print_opt("-i", "--autoindent", N_("Automatically indent new lines"));
    print_opt(
        "-j", "--jumpyscrolling", N_("Scroll per half-screen, not per line"));
    print_opt("-k", "--cutfromcursor", N_("Cut from cursor to end of line"));
    print_opt(
        "-l", "--linenumbers", N_("Show line numbers in front of the text"));
    print_opt("-m", "--mouse", N_("Enable the use of the mouse"));
    print_opt("-n", "--noread", N_("Do not read the file (only write it)"));
    print_opt(_("-o <dir>"), _("--operatingdir=<dir>"),
              N_("Set operating directory"));
    print_opt("-p", "--preserve", N_("Preserve XON (^Q) and XOFF (^S) keys"));
    print_opt("-q", "--indicator", N_("Show a position+portion indicator"));
    print_opt(_("-r <number>"), _("--fill=<number>"),
              N_("Set width for hard-wrap and justify"));
    if (!ISSET(RESTRICTED))
    {
        print_opt(_("-s <program>"), _("--speller=<program>"),
                  N_("Use this alternative spell checker"));
    }
    print_opt("-t", "--saveonexit", N_("Save changes on exit, don't prompt"));
    print_opt("-u", "--unix", N_("Save a file by default in Unix format"));
    print_opt("-v", "--view", N_("View mode (read-only)"));
    print_opt("-w", "--nowrap", N_("Don't hard-wrap long lines [default]"));
    print_opt("-x", "--nohelp", N_("Don't show the two help lines"));
    print_opt("-y", "--afterends", N_("Make Ctrl+Right stop at word ends"));
    print_opt(
        "-z", "--listsyntaxes", N_("List the names of available syntaxes"));
    print_opt("-!", "--magic", N_("Also try magic to determine syntax"));
    print_opt(
        "-@", "--colonparsing", N_("Accept 'filename:linenumber' notation"));
    print_opt("-%", "--stateflags", N_("Show some states on the title bar"));
    print_opt("-_", "--minibar", N_("Show a feedback bar at the bottom"));
    print_opt("-0", "--zero", N_("Hide all bars, use whole terminal"));
    print_opt("-/", "--modernbindings", N_("Use better-known key bindings"));
    exit(0);
}

/* Display the version number of this nano, a copyright notice, some contact
 * information, and the configuration options this nano was compiled with. */
void
version(void)
{
    printf(_(" NanoX, version %s\n"), VERSION);
    printf(" 'NanoX %s' is a Fork of 'GNU nano v8.0-44-gef1c9b9f' from git "
           "source code, converted "
           "into C++\n",
           REVISION);
    /* TRANSLATORS: The %s is the year of the latest release. */
    printf(_(" (C) %s the Free Software Foundation and various contributors\n"),
           "2024");
#ifdef DEBUG
    printf(_(" Compiled options:"));
    printf(" --enable-debug");
#endif
    printf("\n");
    exit(0);
}

/* List the names of the available syntaxes. */
void
list_syntax_names(void)
{
    int width = 0;
    printf(_("Available syntaxes:\n"));
    for (syntaxtype *sntx = syntaxes; sntx != NULL; sntx = sntx->next)
    {
        if (width > 45)
        {
            printf("\n");
            width = 0;
        }
        printf(" %s", sntx->name);
        width += wideness(sntx->name, 45 * 4);
    }
    printf("\n");
}

/* Register that Ctrl+C was pressed during some system call. */
void
make_a_note(int signal)
{
    control_C_was_pressed = TRUE;
}

/* Make ^C interrupt a system call and set a flag. */
void
install_handler_for_Ctrl_C(void)
{
    /* Enable the generation of a SIGINT when ^C is pressed. */
    enable_kb_interrupt();
    /* Set up a signal handler so that pressing ^C will set a flag. */
    newaction.sa_handler = make_a_note;
    newaction.sa_flags   = 0;
    sigaction(SIGINT, &newaction, &oldaction);
}

/* Go back to ignoring ^C. */
void
restore_handler_for_Ctrl_C(void)
{
    sigaction(SIGINT, &oldaction, NULL);
    disable_kb_interrupt();
}

/* Reconnect standard input to the tty, and store its state. */
void
reconnect_and_store_state(void)
{
    int thetty = open("/dev/tty", O_RDONLY);
    if (thetty < 0 || dup2(thetty, STDIN_FILENO) < 0)
    {
        die(_("Could not reconnect stdin to keyboard\n"));
    }
    close(thetty);
    /* If input was not cut short, store the current state of the terminal. */
    if (!control_C_was_pressed)
    {
        tcgetattr(STDIN_FILENO, &original_state);
    }
}

/* Read whatever comes from standard input into a new buffer. */
bool
scoop_stdin(void)
{
    FILE *stream;
    restore_terminal();
    /* When input comes from a terminal, show a helpful message. */
    if (isatty(STDIN_FILENO))
    {
        fprintf(stderr,
                _("Reading data from keyboard; type ^D or ^D^D to finish.\n"));
    }
    /* Open standard input. */
    stream = fopen("/dev/stdin", "rb");
    if (stream == NULL)
    {
        const char *errnoStr = strerror(errno);
        terminal_init();
        doupdate();
        statusline(ALERT, _("Failed to open stdin: %s"), errnoStr);
        return FALSE;
    }
    /* Set up a signal handler so that ^C will stop the reading. */
    install_handler_for_Ctrl_C();
    /* Read the input into a new buffer, undoably. */
    make_new_buffer();
    read_file(stream, 0, "stdin", FALSE);
    find_and_prime_applicable_syntax();
    /* Restore the original ^C handler. */
    restore_handler_for_Ctrl_C();
    if (!ISSET(VIEW_MODE) && openfile->totsize > 0)
    {
        set_modified();
    }
    return TRUE;
}

/* Register half a dozen signal handlers. */
void
signal_init(void)
{
    struct sigaction deed = {{0}};
    /* Trap SIGINT and SIGQUIT because we want them to do useful things. */
    deed.sa_handler = SIG_IGN;
    sigaction(SIGINT, &deed, NULL);
    sigaction(SIGQUIT, &deed, NULL);
    /* Trap SIGHUP and SIGTERM because we want to write the file out. */
    deed.sa_handler = handle_hupterm;
    sigaction(SIGHUP, &deed, NULL);
    sigaction(SIGTERM, &deed, NULL);
    /* Trap SIGWINCH because we want to handle window resizes. */
    deed.sa_handler = handle_sigwinch;
    sigaction(SIGWINCH, &deed, NULL);
    /* Prevent the suspend handler from getting interrupted. */
    sigfillset(&deed.sa_mask);
    deed.sa_handler = suspend_nano;
    sigaction(SIGTSTP, &deed, NULL);
    sigfillset(&deed.sa_mask);
    deed.sa_handler = continue_nano;
    sigaction(SIGCONT, &deed, NULL);
#if !defined(DEBUG)
    if (getenv("NANO_NOCATCH") == NULL)
    {
        /* Trap SIGSEGV and SIGABRT to save any changed buffers and reset
         * the terminal to a usable state.  Reset these handlers to their
         * defaults as soon as their signal fires. */
        deed.sa_handler = handle_crash;
        deed.sa_flags |= SA_RESETHAND;
        sigaction(SIGSEGV, &deed, NULL);
        sigaction(SIGABRT, &deed, NULL);
    }
#endif
}

/* Handler for SIGHUP (hangup) and SIGTERM (terminate). */
void
handle_hupterm(int signal)
{
    die(_("Received SIGHUP or SIGTERM\n"));
}

#if !defined(DEBUG)
/* Handler for SIGSEGV (segfault) and SIGABRT (abort). */
void
handle_crash(int signal)
{
    die(_("Sorry! Nano crashed! Code: %d.  Please report a bug.\n"), signal);
}
#endif

/* Handler for SIGTSTP (suspend). */
void
suspend_nano(int signal)
{
    disable_mouse_support();
    restore_terminal();
    printf("\n\n");
    /* Display our helpful message. */
    printf(_("Use \"fg\" to return to nano.\n"));
    fflush(stdout);
    /* The suspend keystroke must not elicit cursor-position display. */
    lastmessage = HUSH;
    /* Do what mutt does: send ourselves a SIGSTOP. */
    kill(0, SIGSTOP);
}

/* When permitted, put nano to sleep. */
void
do_suspend(void)
{
    if (in_restricted_mode())
    {
        return;
    }
    suspend_nano(0);
    ran_a_tool = TRUE;
}

/* Handler for SIGCONT (continue after suspend). */
void
continue_nano(int signal)
{
    if (ISSET(USE_MOUSE))
    {
        enable_mouse_support();
    }
    /* Seams wierd to me that we assume the window was resized
     * instead of checking, but it's the original code.
     * COMMENT: -> // Perhaps the user resized the window while we slept.
     * TODO: Check if the window was resized instead. */
    the_window_resized = TRUE;
    /* Insert a fake keystroke, to neutralize a key-eating issue. */
    ungetch(KEY_FRESH);
}

/* Block or unblock the SIGWINCH signal, depending on the blockit parameter. */
void
block_sigwinch(bool blockit)
{
    sigset_t winch;
    sigemptyset(&winch);
    sigaddset(&winch, SIGWINCH);
    sigprocmask(blockit ? SIG_BLOCK : SIG_UNBLOCK, &winch, NULL);
    if (the_window_resized)
    {
        regenerate_screen();
    }
}

/* Handler for SIGWINCH (window size change). */
void
handle_sigwinch(int signal)
{
    /* Let the input routine know that a SIGWINCH has occurred. */
    the_window_resized = TRUE;
}

/* Reinitialize and redraw the screen completely. */
void
regenerate_screen(void)
{
    /* Reset the trigger. */
    the_window_resized = FALSE;
    /* Leave and immediately reenter curses mode, so that ncurses notices
     * the new screen dimensions and sets LINES and COLS accordingly. */
    endwin();
    refresh();
    /* TODO : (INDECATOR) - this is for the sidebar? */
    sidebar     = (ISSET(INDICATOR) && LINES > 5 && COLS > 9) ? 1 : 0;
    bardata     = (int *)nrealloc(bardata, LINES * sizeof(int));
    editwincols = COLS - margin - sidebar;
    /* Put the terminal in the desired state again, and
     * recreate the subwindows with their (new) sizes. */
    terminal_init();
    window_init();
    /* If we have an open buffer,
     * redraw the contents of the subwindows. */
    if (openfile)
    {
        ensure_firstcolumn_is_aligned();
        draw_all_subwindows();
    }
}

/* Invert the given global flag and adjust things for its new value.
 * TODO : FIX statusline as it takes up 94.1% of the time in this function. */
void
toggle_this(const int flag)
{
    bool enabled = !ISSET(flag);
    TOGGLE(flag);
    focusing = FALSE;
    switch (flag)
    {
        case ZERO :
        {
            window_init();
            draw_all_subwindows();
            return;
        }
        case NO_HELP :
        {
            if (LINES < (ISSET(ZERO) ? 3 : ISSET(MINIBAR) ? 4 : 5))
            {
                statusline(AHEM, _("Too tiny"));
                TOGGLE(flag);
                return;
            }
            window_init();
            draw_all_subwindows();
            break;
        }
        case CONSTANT_SHOW :
        {
            if (LINES == 1)
            {
                statusline(AHEM, _("Too tiny"));
                TOGGLE(flag);
            }
            else if ISSET (ZERO)
            {
                SET(CONSTANT_SHOW);
                toggle_this(ZERO);
            }
            else if (!ISSET(MINIBAR))
            {
                wipe_statusbar();
            }
            return;
        }
        case SOFTWRAP :
        {
            if (!ISSET(SOFTWRAP))
            {
                openfile->firstcolumn = 0;
            }
            refresh_needed = TRUE;
            break;
        }
        case WHITESPACE_DISPLAY :
        {
            titlebar(NULL);
            refresh_needed = TRUE;
            break;
        }
        case NO_SYNTAX :
        {
            precalc_multicolorinfo();
            refresh_needed = TRUE;
            break;
        }
        case TABS_TO_SPACES :
        {
            if (openfile->syntax && openfile->syntax->tabstring)
            {
                statusline(AHEM, _("Current syntax determines Tab"));
                TOGGLE(flag);
                return;
            }
            break;
        }
        case USE_MOUSE :
        {
            mouse_init();
            break;
        }
    }
    if (flag == AUTOINDENT || flag == BREAK_LONG_LINES || flag == SOFTWRAP)
    {
        if (ISSET(MINIBAR) && !ISSET(ZERO) && ISSET(STATEFLAGS))
        {
            return;
        }
        if ISSET (STATEFLAGS)
        {
            titlebar(NULL);
        }
    }
    if (flag == NO_HELP || flag == LINE_NUMBERS || flag == WHITESPACE_DISPLAY)
    {
        if (ISSET(MINIBAR) || ISSET(ZERO) || LINES == 1)
        {
            return;
        }
    }
    if (flag == NO_HELP || flag == NO_SYNTAX)
    {
        enabled = !enabled;
    }
    statusline(REMARK, "%s %s", _(epithet_of_flag(flag)),
               enabled ? _("enabled") : _("disabled"));
}

/* Disable extended input and output processing in our terminal settings. */
void
disable_extended_io(void)
{
    termios settings = {0};
    tcgetattr(0, &settings);
    settings.c_lflag &= ~IEXTEN;
    settings.c_oflag &= ~OPOST;
    tcsetattr(0, TCSANOW, &settings);
}

/* Stop ^C from generating a SIGINT. */
void
disable_kb_interrupt(void)
{
    termios settings = {0};
    tcgetattr(0, &settings);
    settings.c_lflag &= ~ISIG;
    tcsetattr(0, TCSANOW, &settings);
}

/* Make ^C generate a SIGINT. */
void
enable_kb_interrupt(void)
{
    termios settings = {0};
    tcgetattr(0, &settings);
    settings.c_lflag |= ISIG;
    tcsetattr(0, TCSANOW, &settings);
}

/* Disable the terminal's XON/XOFF flow-control characters. */
void
disable_flow_control(void)
{
    termios settings;
    tcgetattr(0, &settings);
    settings.c_iflag &= ~IXON;
    tcsetattr(0, TCSANOW, &settings);
}

/* Enable the terminal's XON/XOFF flow-control characters. */
void
enable_flow_control(void)
{
    termios settings;
    tcgetattr(0, &settings);
    settings.c_iflag |= IXON;
    tcsetattr(0, TCSANOW, &settings);
}

/* Set up the terminal state.  Put the terminal in raw mode
 * (read one character at a time, disable the special control keys, and disable
 * the flow control characters), disable translation of carriage return (^M)
 * into newline (^J), so that we can tell the difference between the Enter key
 * and Ctrl-J, and disable echoing of characters as they're typed. Finally,
 * disable extended input and output processing, and, if we're not in preserve
 * mode, reenable interpretation of the flow control characters. */
void
terminal_init(void)
{
    raw();
    nonl();
    noecho();
    disable_extended_io();
    if (ISSET(PRESERVE))
    {
        enable_flow_control();
    }
    disable_kb_interrupt();
    /* Tell the terminal to enable bracketed pastes. */
    printf("\x1B[?2004h");
    fflush(stdout);
}

/* Ask ncurses for a keycode, or assign a default one. */
int
get_keycode(const char *const keyname, const int standard)
{
    const char *keyvalue = tigetstr(keyname);
    if (keyvalue != 0 && keyvalue != (char *)-1 && key_defined(keyvalue))
    {
        return key_defined(keyvalue);
    }
#ifdef DEBUG
    if (!ISSET(RAW_SEQUENCES))
    {
        fprintf(stderr, "Using fallback keycode for %s\n", keyname);
    }
#endif
    return standard;
}

/* Ensure that the margin can accommodate the buffer's highest line number. */
void
confirm_margin(void)
{
    int needed_margin = digits(openfile->filebot->lineno) + 1;
    /* When not requested or space is too tight, suppress line numbers. */
    if (!ISSET(LINE_NUMBERS) || needed_margin > COLS - 4)
    {
        needed_margin = 0;
    }
    if (needed_margin != margin)
    {
        bool keep_focus = (margin > 0) && focusing;
        margin          = needed_margin;
        editwincols     = COLS - margin - sidebar;
        /* Ensure a proper starting column for the first screen row. */
        ensure_firstcolumn_is_aligned();
        focusing = keep_focus;
        /* The margin has changed -- schedule a full refresh. */
        refresh_needed = TRUE;
    }
}

/* Say that an unbound key was struck, and if possible which one. */
void
unbound_key(int code)
{
    if (code == FOREIGN_SEQUENCE)
    {
        /* TRANSLATORS: This refers to a sequence of escape codes
         *              (from the keyboard) that nano does not recognize. */
        statusline(AHEM, _("Unknown sequence"));
    }
    else if (code == NO_SUCH_FUNCTION)
    {
        statusline(AHEM, _("Unknown function: %s"), commandname);
    }
    else if (code == MISSING_BRACE)
    {
        statusline(AHEM, _("Missing }"));
    }
    else if (code > KEY_F0 && code < KEY_F0 + 25)
    {
        /* TRANSLATORS : This refers to an unbound function key. */
        statusline(AHEM, _("Unbound key: F%i"), code - KEY_F0);
    }
    else if (code > 0x7F)
    {
        statusline(AHEM, _("Unbound key"));
    }
    else if (meta_key)
    {
        if (code < 0x20)
        {
            statusline(AHEM, _("Unbindable key: M-^%c"), code + 0x40);
        }
        else if (shifted_metas && 'A' <= code && code <= 'Z')
        {
            statusline(AHEM, _("Unbound key: %s%c"), "Sh-M-", code);
        }
        else
        {
            statusline(AHEM, _("Unbound key: %s%c"), "M-", toupper(code));
        }
    }
    else if (code == ESC_CODE)
    {
        statusline(AHEM, _("Unbindable key: ^["));
    }
    else if (code < 0x20)
    {
        statusline(AHEM, _("Unbound key: %s%c"), "^", code + 0x40);
    }
    else
    {
        statusline(AHEM, _("Unbound key: %s%c"), "", code);
    }
    set_blankdelay_to_one();
}

/* Handle a mouse click on the edit window or the shortcut list. */
int
do_mouse(void)
{
    int click_row;
    int click_col;
    int retval = get_mouseinput(&click_row, &click_col, TRUE);
    /* If the click is wrong or already handled, we're done. */
    if (retval != 0)
    {
        return retval;
    }
    /* If the click was in the edit window, put the cursor in that spot. */
    if (wmouse_trafo(midwin, &click_row, &click_col, FALSE))
    {
        linestruct   *was_current = openfile->current;
        long          row_count   = click_row - openfile->cursor_row;
        unsigned long leftedge;
        unsigned long was_x = openfile->current_x;
        if ISSET (SOFTWRAP)
        {
            leftedge = leftedge_for(xplustabs(), openfile->current);
        }
        else
        {
            leftedge = get_page_start(xplustabs());
        }
        /* Move current up or down to the row that was clicked on. */
        if (row_count < 0)
        {
            go_back_chunks(-row_count, &openfile->current, &leftedge);
        }
        else
        {
            go_forward_chunks(row_count, &openfile->current, &leftedge);
        }
        openfile->current_x = actual_x(
            openfile->current->data, actual_last_column(leftedge, click_col));
        /* Clicking there where the cursor is toggles the mark. */
        if (row_count == 0 && openfile->current_x == was_x)
        {
            do_mark();
            if (ISSET(STATEFLAGS))
            {
                titlebar(NULL);
            }
        }
        else
        {
            /* The cursor moved; clean the cutbuffer on the next cut. */
            keep_cutbuffer = FALSE;
        }
        edit_redraw(was_current, CENTERING);
    }
    /* No more handling is needed. */
    return 2;
}

/* Return 'TRUE' when the given function is a cursor-moving command. */
bool
wanted_to_move(functionptrtype f)
{
    return (f == do_left || f == do_right || f == do_up || f == do_down ||
            f == do_home || f == do_end || f == to_prev_word ||
            f == to_next_word || f == to_para_begin || f == to_para_end ||
            f == to_prev_block || f == to_next_block || f == do_page_up ||
            f == do_page_down || f == to_first_line || f == to_last_line);
}

/* Return 'TRUE' when the given function makes a change -- no good for view
 * mode. */
bool
changes_something(functionptrtype f)
{
    return (f == do_savefile || f == do_writeout || f == do_enter ||
            f == do_tab || f == do_delete || f == do_backspace ||
            f == cut_text || f == paste_text || f == chop_previous_word ||
            f == chop_next_word || f == zap_text || f == cut_till_eof ||
            f == do_execute || f == do_indent || f == do_unindent ||
            f == do_justify || f == do_full_justify || f == do_comment ||
            f == do_spell || f == do_formatter || f == complete_a_word ||
            f == do_replace || f == do_verbatim_input);
}

/* Read in all waiting input bytes and paste them into the buffer in one go. */
void
suck_up_input_and_paste_it(void)
{
    linestruct   *was_cutbuffer = cutbuffer;
    linestruct   *line          = make_new_node(NULL);
    unsigned long index         = 0;
    line->data                  = copy_of("");
    cutbuffer                   = line;
    while (bracketed_paste)
    {
        int input = get_kbinput(midwin, BLIND);
        if (input == '\r' || input == '\n')
        {
            line->next = make_new_node(line);
            line       = line->next;
            line->data = copy_of("");
            index      = 0;
        }
        else if ((0x20 <= input && input <= 0xFF && input != DEL_CODE) ||
                 input == '\t')
        {
            line->data          = (char *)nrealloc(line->data, index + 2);
            line->data[index++] = (char)input;
            line->data[index]   = '\0';
        }
        else if (input != BRACKETED_PASTE_MARKER)
        {
            beep();
        }
    }
    if (ISSET(VIEW_MODE))
    {
        print_view_warning();
    }
    else
    {
        paste_text();
    }
    free_lines(cutbuffer);
    cutbuffer = was_cutbuffer;
}

/* Insert the given short burst of bytes into the edit buffer. */
void
inject(char *burst, unsigned long count)
{
    linestruct   *thisline     = openfile->current;
    unsigned long datalen      = strlen(thisline->data);
    unsigned long original_row = 0;
    unsigned long old_amount   = 0;
    if (ISSET(SOFTWRAP))
    {
        if (openfile->cursor_row == editwinrows - 1)
        {
            original_row = chunk_for(xplustabs(), thisline);
        }
        old_amount = extra_chunks_in(thisline);
    }
    /* Encode an embedded NUL byte as 0x0A. */
    for (unsigned long index = 0; index < count; index++)
    {
        if (burst[index] == '\0')
        {
            burst[index] = '\n';
        }
    }
    /* Only add a new undo item when the current item is not an ADD or when
     * the current typing is not contiguous with the previous typing. */
    if (openfile->last_action != ADD ||
        openfile->current_undo->tail_lineno != thisline->lineno ||
        openfile->current_undo->tail_x != openfile->current_x)
    {
        add_undo(ADD, NULL);
    }
    /* Make room for the new bytes and copy them into the line. */
    thisline->data = (char *)nrealloc(thisline->data, datalen + count + 1);
    memmove(thisline->data + openfile->current_x + count,
            thisline->data + openfile->current_x,
            datalen - openfile->current_x + 1);
    strncpy(thisline->data + openfile->current_x, burst, count);
    /* When the cursor is on the top row and not on the first chunk
     * of a line, adding text there might change the preceding chunk
     * and thus require an adjustment of firstcolumn. */
    if (thisline == openfile->edittop && openfile->firstcolumn > 0)
    {
        ensure_firstcolumn_is_aligned();
        refresh_needed = TRUE;
    }
    /* When the mark is to the right of the cursor, compensate its position. */
    if (thisline == openfile->mark && openfile->current_x < openfile->mark_x)
    {
        openfile->mark_x += count;
    }
    openfile->current_x += count;
    openfile->totsize += mbstrlen(burst);
    set_modified();
    /* If text was added to the magic line, create a new magic line. */
    if (thisline == openfile->filebot && !ISSET(NO_NEWLINES))
    {
        new_magicline();
        if (margin || (openfile->syntax && openfile->syntax->multiscore))
        {
            if (margin)
            {
                if (openfile->cursor_row < editwinrows - 1)
                {
                    update_line(thisline->next, 0);
                }
            }
        }
    }
    update_undo(ADD);
    if (ISSET(BREAK_LONG_LINES))
    {
        do_wrap();
    }
    openfile->placewewant = xplustabs();
    /* When softwrapping and the number of chunks in the current line changed,
     * or we were on the last row of the edit window and moved to a new chunk,
     * we need a full refresh. */
    if (ISSET(SOFTWRAP) &&
        (extra_chunks_in(openfile->current) != old_amount ||
         (openfile->cursor_row == editwinrows - 1 &&
          chunk_for(openfile->placewewant, openfile->current) > original_row)))
    {
        refresh_needed = TRUE;
        focusing       = FALSE;
    }
    if (!refresh_needed)
    {
        check_the_multis(openfile->current);
    }
    if (!refresh_needed)
    {
        update_line(openfile->current, openfile->current_x);
    }
}

/* Read in a keystroke, and execute its command or insert it into the buffer. */
void
process_a_keystroke(void)
{
    /* The keystroke we read in, this can be a char or a shortcut */
    int input;
    /* The buffer to hold the actual chars. */
    static char *puddle = NULL;
    /* The size of the buffer, doubles when needed. */
    static unsigned long capacity = 12;
    /* The current length of the buffer. */
    static unsigned long depth                 = 0;
    linestruct          *was_mark              = openfile->mark;
    static bool          give_a_hint           = TRUE;
    bool                 was_open_bracket_char = FALSE;
    const keystruct     *shortcut;
    functionptrtype      function;
    /* Read in a keystroke, and show the cursor while waiting. */
    input = get_kbinput(midwin, VISIBLE);
    /* NETLOGGER.log("input: %i.\n", input); */
    lastmessage = VACUUM;
    /* When the input is a window resize, do nothing. */
    if (input == KEY_WINCH)
    {
        return;
    }
    /* When the input is a mouse click, handle it. */
    if (input == KEY_MOUSE)
    {
        /* If the user clicked on a shortcut,
         * read in the key code that it was converted into.
         * Else the click has been handled or was invalid. */
        if (do_mouse() == 1)
        {
            input = get_kbinput(midwin, BLIND);
        }
        else
        {
            return;
        }
    }
    /* Check for a shortcut in the main list. */
    shortcut = get_shortcut(input);
    function = (shortcut ? shortcut->func : NULL);
    /* If not a command, discard anything that is not a normal character byte.
     */
    if (!function)
    {
        /* When the input is a function key,
         * execute the function it is bound to. */
        if (input < 0x20 || input > 0xFF || meta_key)
        {
            unbound_key(input);
        }
        else if (ISSET(VIEW_MODE))
        {
            print_view_warning();
        }
        else
        {
            /* When the input buffer (plus room for terminating NUL) is full,
             * extend it. Otherwise, if it does not exist yet, create it. */
            if (depth + 1 == capacity)
            {
                capacity = 2 * capacity;
                puddle   = (char *)nrealloc(puddle, capacity);
            }
            else if (!puddle)
            {
                puddle = (char *)nmalloc(capacity);
            }
            /* If region is marked, and 'input' is an enclose char, then we
             * enclose the marked region with that char. */
            if (openfile->mark)
            {
                if (input == '"' || input == '\'' || input == '(' ||
                    input == '{' || input == '[')
                {
                    const char *s1, *s2;
                    input == '"'                           ? s1 = "\"",
                                   s2 = s1 : input == '\'' ? s1 = "'",
                                   s2 = s1 : input == '('  ? s1 = "(",
                                   s2 = ")" : input == '{' ? s1 = "{",
                                   s2 = "}" : input == '[' ? s1 = "[",
                                   s2                           = "]" : 0;
                    enclose_marked_region(s1, s2);
                    refresh_needed = TRUE;
                    return;
                }
            }
            if (openfile->mark && openfile->softmark)
            {
                openfile->mark = NULL;
                refresh_needed = TRUE;
            }
            puddle[depth++] = (char)input;
            /* Check for a bracketed input start.  Meaning a char that has a
             * corresponding closing bracket. */
            if (input == '(' || input == '[' || input == '{' || input == '<' ||
                input == '\'' || input == '"')
            {
                if (input == '<')
                {
                    (openfile->current->data[0] == '#') ?
                        puddle[depth++] = '>' :
                        0;
                }
                else
                {
                    puddle[depth++] = input == '('  ? ')' :
                                      input == '['  ? ']' :
                                      input == '"'  ? '"' :
                                      input == '\'' ? '\'' :
                                                      '}';
                }
                /* Set a flag to remember that an open bracket
                 * character was inserted into the input buffer. */
                was_open_bracket_char = TRUE;
            }
        }
    }
    /* If there are gathered bytes and we have a command or no other key codes
     * are waiting, it's time to insert these bytes into the edit buffer. */
    if (depth > 0 && (function || waiting_keycodes() == 0))
    {
        puddle[depth] = '\0';
        inject(puddle, depth);
        if (was_open_bracket_char)
        {
            if (puddle[depth - 1] != '<')
            {
                do_left();
                /* Set this flag so we can delete both of the
                 * brackeded char if Bsp is pressed directly after. */
                last_key_was_bracket = TRUE;
            }
        }
        else
        {
            last_key_was_bracket = FALSE;
            if (ISSET(SUGGEST))
            {
                suggest_on = TRUE;
            }
        }
        depth = 0;
    }
    if (function != do_cycle)
    {
        cycling_aim = 0;
    }
    if (!function)
    {
        pletion_line   = NULL;
        keep_cutbuffer = FALSE;
        return;
    }
    if (ISSET(VIEW_MODE) && changes_something(function))
    {
        print_view_warning();
        return;
    }
    if (input == '\b' && give_a_hint && openfile->current_x == 0 &&
        openfile->current == openfile->filetop && !ISSET(NO_HELP))
    {
        statusbar(_("^W = Ctrl+W    M-W = Alt+W"));
        give_a_hint = FALSE;
    }
    else if (meta_key)
    {
        give_a_hint = FALSE;
    }
    /* When not cutting or copying text, drop the cutbuffer the next time. */
    if (function != cut_text && function != copy_text)
    {
        if (function != zap_text)
        {
            keep_cutbuffer = FALSE;
        }
    }
    if (function != complete_a_word)
    {
        pletion_line = NULL;
    }
    if (function == (functionptrtype)implant)
    {
        implant(shortcut->expansion);
        return;
    }
    if (function == do_toggle)
    {
        toggle_this(shortcut->toggle);
        if (shortcut->toggle == CUT_FROM_CURSOR)
        {
            keep_cutbuffer = FALSE;
        }
        return;
    }
    linestruct   *was_current = openfile->current;
    unsigned long was_x       = openfile->current_x;
    /* If Shifted movement occurs, set the mark. */
    if (shift_held && !openfile->mark)
    {
        openfile->mark     = openfile->current;
        openfile->mark_x   = openfile->current_x;
        openfile->softmark = TRUE;
    }
    /* Execute the function of the shortcut. */
    function();
    /* When the marked region changes without Shift being held, discard a soft
     * mark. And when the set of lines changes, reset the "last line too" flag.
     */
    if (openfile->mark && openfile->softmark && !shift_held &&
        (openfile->current != was_current || openfile->current_x != was_x ||
         wanted_to_move(function)))
    {
        openfile->mark = NULL;
        refresh_needed = TRUE;
    }
    else if (openfile->current != was_current)
    {
        also_the_last = FALSE;
    }
    if (bracketed_paste)
    {
        suck_up_input_and_paste_it();
    }
    if (ISSET(STATEFLAGS) && openfile->mark != was_mark)
    {
        titlebar(NULL);
    }
    !was_open_bracket_char ? (last_key_was_bracket = FALSE) : 0;
}

int
main(int argc, char **argv)
{
    init_queue_task();
    init_event_handler();
    Mlib::Profile::setupReportGeneration("/home/mellw/.NanoX.profile");
    LOUT.setOutputFile("/home/mellw/.NanoX.log");
    LOUT_logI("Starting NanoX");
    term                  = getenv("TERM");
    const char *netlogger = getenv("NETLOGGER");
    if (netlogger != NULL)
    {
        NETLOGGER.enable();
        NETLOGGER.init(netlogger, 8080);
    }
    NETLOGGER.send_to_server("Starting NanoX.\n");
    atexit(
        []
        {
            LOUT.destroy();
            std::vector<std::string> gprof_report =
                GLOBALPROFILER->retrveFormatedStrVecStats();
            for (const std::string &str : gprof_report)
            {
                NETLOGGER << str << NETLOG_ENDL;
            }
            NETLOGGER.send_to_server("\nExiting NanoX.\n");
            NETLOGGER.destroy();
        });
    int stdin_flags;
    /* Whether to ignore the nanorc files. */
    bool ignore_rcfiles = FALSE;
    /* Was the fill option used on the command line? */
    bool fill_used = FALSE;
    /* Becomes 0 when --nowrap and 1 when --breaklonglines is used. */
    int hardwrap = -2;
    /* Whether the quoting regex was compiled successfully. */
    int     quoterc;
    vt_stat dummy;
    /* Check whether we're running on a Linux console. */
    on_a_vt = !ioctl(STDOUT_FILENO, VT_GETSTATE, &dummy);
    /* Back up the terminal settings so that they can be restored. */
    tcgetattr(STDIN_FILENO, &original_state);
    /* Get the state of standard input and ensure it uses blocking mode. */
    stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (stdin_flags != -1)
    {
        fcntl(STDIN_FILENO, F_SETFL, stdin_flags & ~O_NONBLOCK);
    }
    /* If setting the locale is successful and it uses UTF-8, we will
     * need to use the multibyte functions for text processing. */
    if (setlocale(LC_ALL, "") &&
        constexpr_strcmp(nl_langinfo(CODESET), "UTF-8") == 0)
    {
        utf8_init();
    }
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    /* Set some default flags. */
    SET(NO_WRAP);
    SET(LET_THEM_ZAP);
    SET(MODERN_BINDINGS);
    SET(AUTOINDENT);
    SET(MINIBAR);
    SET(CONSTANT_SHOW);
    SET(STATEFLAGS);
    SET(NO_HELP);
    SET(INDICATOR);
    SET(AFTER_ENDS);
    SET(SUGGEST);
    SET(SUGGEST_INLINE);
    /* This is my new system for live syntax, and it`s fucking fast. */
    SET(EXPERIMENTAL_FAST_LIVE_SYNTAX);
    /* If the executable's name starts with 'r', activate restricted mode. */
    if (*(tail(argv[0])) == 'r')
    {
        SET(RESTRICTED);
    }
    /* Check for cmd flags. */
    for (int i = 1; i < argc; ++i)
    {
        const unsigned int flag = retriveFlagFromStr(argv[i]);
        flag ? SET(flag) : 0;

        const unsigned int cliCmd = retriveCliOptionFromStr(argv[i]);
        cliCmd &CLI_OPT_VERSION ? version() : void();
        cliCmd &CLI_OPT_HELP ? usage() : void();
        cliCmd &CLI_OPT_IGNORERCFILE ? ignore_rcfiles = TRUE : 0;
        cliCmd &CLI_OPT_BACKUPDIR ?
            (i++ < argc) ? backup_dir = mallocstrcpy(backup_dir, argv[i]) : 0 :
            0;
        cliCmd &CLI_OPT_WORDCHARS ?
            (i++ < argc) ? word_chars = mallocstrcpy(word_chars, argv[i]) : 0 :
            0;
        cliCmd &CLI_OPT_SYNTAX ?
            (i++ < argc) ? syntaxstr = mallocstrcpy(syntaxstr, argv[i]) : 0 :
            0;
        cliCmd &CLI_OPT_RCFILE ?
            (i++ < argc) ?
            custom_nanorc = mallocstrcpy(custom_nanorc, argv[i]) :
            0 :
            0;
        cliCmd &CLI_OPT_BREAKLONGLINES ? hardwrap = 1 : 0;
        cliCmd &CLI_OPT_SPELLER ?
            (i++ < argc) ? alt_speller = mallocstrcpy(alt_speller, argv[i]) :
                           0 :
            0;
        cliCmd &CLI_OPT_SYNTAX ?
            (i++ < argc) ? syntaxstr = mallocstrcpy(syntaxstr, argv[i]) : 0 :
            0;

        if (cliCmd & CLI_OPT_OPERATINGDIR)
        {
            if (++i < argc)
            {
                operating_dir = mallocstrcpy(operating_dir, argv[i]);
            }
        }
        if (cliCmd & CLI_OPT_LISTSYNTAX)
        {
            if (!ignore_rcfiles)
            {
                do_rcfiles();
            }
            if (syntaxes)
            {
                list_syntax_names();
            }
            exit(0);
        }
        if (cliCmd & CLI_OPT_FILL)
        {
            if (++i < argc)
            {
                if (!parseNum(argv[i], fill) || fill <= 0)
                {
                    fprintf(stderr, _("Requested fill size \"%s\" is invalid"),
                            optarg);
                    fprintf(stderr, "\n");
                    exit(1);
                }
                fill_used = TRUE;
            }
        }
        if (cliCmd & CLI_OPT_TABSIZE)
        {
            if (++i < argc)
            {
                if (!parseNum(argv[i], tabsize) || tabsize <= 0)
                {
                    fprintf(stderr, _("Requested tab size \"%s\" is invalid"),
                            argv[i]);
                    fprintf(stderr, "\n");
                    exit(1);
                }
                continue;
            }
        }
        if (cliCmd & CLI_OPT_GUIDESTRIPE)
        {
            if (++i < argc)
            {
                if (!parseNum(argv[i], stripe_column) || stripe_column <= 0)
                {
                    fprintf(
                        stderr, _("Guide column \"%s\" is invalid"), optarg);
                    fprintf(stderr, "\n");
                    exit(1);
                }
            }
        }
        /* if (cliCmd & CLI_OPT_GUI)
        {
            init_window();
        } */
    }
    /* Curses needs TERM; if it is unset, try falling back to a VT220. */
    if (getenv("TERM") == NULL)
    {
        putenv((char *)"TERM=vt220");
        setenv("TERM", "vt220", 1);
    }
    /* Enter into curses mode.  Abort if this fails. */
    if (initscr() == NULL)
    {
        exit(1);
    }
    /* If the terminal can do colors, tell ncurses to switch them on. */
    if (has_colors())
    {
        start_color();
    }
    /* When requested, suppress the default spotlight and error colors. */
    rescind_colors = (getenv("NO_COLOR") != NULL);
    /* Set up the function and shortcut lists.  This needs to be done
     * before reading the rcfile, to be able to rebind/unbind keys. */
    shortcut_init();
    if (!ignore_rcfiles)
    {
        /* Back up the command-line options that take an argument. */
        long          fill_cmdline          = fill;
        unsigned long stripeclm_cmdline     = stripe_column;
        long          tabsize_cmdline       = tabsize;
        char         *backup_dir_cmdline    = backup_dir;
        char         *word_chars_cmdline    = word_chars;
        char         *operating_dir_cmdline = operating_dir;
        char         *quotestr_cmdline      = quotestr;
        char         *alt_speller_cmdline   = alt_speller;
        /* Back up the command-line flags. */
        unsigned long flags_cmdline[sizeof(flags) / sizeof(flags[0])];
        memcpy(flags_cmdline, flags, sizeof(flags_cmdline));
        /* Clear the string options, to not overwrite the specified ones. */
        backup_dir    = NULL;
        word_chars    = NULL;
        operating_dir = NULL;
        quotestr      = NULL;
        alt_speller   = NULL;
        /* Now process the system's and the user's nanorc file, if any. */
        do_rcfiles();
        /* If the backed-up command-line options have a value, restore them. */
        if (fill_used)
        {
            fill = fill_cmdline;
        }
        if (backup_dir_cmdline != NULL)
        {
            free(backup_dir);
            backup_dir = backup_dir_cmdline;
        }
        if (word_chars_cmdline != NULL)
        {
            free(word_chars);
            word_chars = word_chars_cmdline;
        }
        if (stripeclm_cmdline > 0)
        {
            stripe_column = stripeclm_cmdline;
        }
        if (tabsize_cmdline != -1)
        {
            tabsize = tabsize_cmdline;
        }
        if (operating_dir_cmdline != NULL || ISSET(RESTRICTED))
        {
            free(operating_dir);
            operating_dir = operating_dir_cmdline;
        }
        if (quotestr_cmdline != NULL)
        {
            free(quotestr);
            quotestr = quotestr_cmdline;
        }
        if (alt_speller_cmdline != NULL)
        {
            free(alt_speller);
            alt_speller = alt_speller_cmdline;
        }
        strip_leading_blanks_from(alt_speller);
        /* If an rcfile undid the default setting, copy it to the new flag. */
        if (!ISSET(NO_WRAP))
        {
            SET(BREAK_LONG_LINES);
        }
        /* Simply OR the boolean flags from rcfile and command line. */
        for (unsigned long i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
        {
            flags[i] |= flags_cmdline[i];
        }
    }
    if (hardwrap == 0)
    {
        UNSET(BREAK_LONG_LINES);
    }
    else if (hardwrap == 1)
    {
        SET(BREAK_LONG_LINES);
    }
    /* If the user wants bold instead of reverse video for hilited text... */
    if (ISSET(BOLD_TEXT))
    {
        hilite_attribute = A_BOLD;
    }
    /* When in restricted mode, disable backups and history files, since they
     * would allow writing to files not specified on the command line. */
    if (ISSET(RESTRICTED))
    {
        UNSET(MAKE_BACKUP);
        UNSET(HISTORYLOG);
        UNSET(POSITIONLOG);
    }
    /* When getting untranslated escape sequences, the mouse cannot be used. */
    if (ISSET(RAW_SEQUENCES))
    {
        UNSET(USE_MOUSE);
    }
    /* When --modernbindings is used, ^Q and ^S need to be functional. */
    if (ISSET(MODERN_BINDINGS))
    {
        UNSET(PRESERVE);
    }
    /* When suppressing title bar or minibar, suppress also the help lines. */
    if (ISSET(ZERO))
    {
        SET(NO_HELP);
    }
    /* Initialize the pointers for the Search/Replace/Execute histories. */
    history_init();
    /* If we need history files, verify that we have a directory for them,
     * and when not, cancel the options. */
    if ((ISSET(HISTORYLOG) || ISSET(POSITIONLOG)) && !have_statedir())
    {
        UNSET(HISTORYLOG);
        UNSET(POSITIONLOG);
    }
    /* If the user wants history persistence, read the relevant files. */
    if (ISSET(HISTORYLOG))
    {
        load_history();
    }
    if (ISSET(POSITIONLOG))
    {
        load_poshistory();
    }
    /* If a backup directory was specified and we're not in restricted mode,
     * verify it is an existing folder, so backup files can be saved there. */
    if (backup_dir != NULL && !ISSET(RESTRICTED))
    {
        init_backup_dir();
    }
    /* Set up the operating directory.  This entails chdir()ing there,
     * so that file reads and writes will be based there. */
    if (operating_dir != NULL)
    {
        init_operating_dir();
    }
    /* Set the default value for things that weren't specified. */
    (punct == NULL) ? punct = copy_of("!.?") : 0;
    (brackets == NULL) ? brackets = copy_of("\"')>]}") : 0;
    (quotestr == NULL) ? quotestr = copy_of("^([ \t]*([!#%:;>|}]|/{2}))+") : 0;
    /* Compile the quoting regex, and exit when it's invalid. */
    quoterc = regcomp(&quotereg, quotestr, NANO_REG_EXTENDED);
    if (quoterc != 0)
    {
        unsigned long size    = regerror(quoterc, &quotereg, NULL, 0);
        char         *message = (char *)nmalloc(size);
        regerror(quoterc, &quotereg, message, size);
        die(_("Bad quoting regex \"%s\": %s\n"), quotestr, message);
    }
    else
    {
        free(quotestr);
    }
    /* If we don't have an alternative spell checker after reading the
     * command line and/or rcfile(s), check $SPELL for one, as Pico
     * does (unless we're using restricted mode, in which case spell
     * checking is disabled, since it would allow reading from or
     * writing to files not specified on the command line). */
    if (alt_speller == NULL && !ISSET(RESTRICTED))
    {
        const char *spellenv = getenv("SPELL");
        if (spellenv != NULL)
        {
            alt_speller = copy_of(spellenv);
        }
    }
    /* If matchbrackets wasn't specified, set its default value. */
    if (matchbrackets == NULL)
    {
        matchbrackets = copy_of("(<[{)>]}");
    }
    /* If the whitespace option wasn't specified, set its default value. */
    if (whitespace == NULL)
    {
        if (using_utf8())
        {
            /* A tab is shown as a Right-Pointing Double Angle Quotation Mark
             * (U+00BB), and a space as a Middle Dot (U+00B7). */
            whitespace  = copy_of("\xC2\xBB\xC2\xB7");
            whitelen[0] = 2;
            whitelen[1] = 2;
        }
        else
        {
            whitespace  = copy_of(">.");
            whitelen[0] = 1;
            whitelen[1] = 1;
        }
    }
    /* Initialize the search string. */
    last_search = copy_of("");
    UNSET(BACKWARDS_SEARCH);
    /* If tabsize wasn't specified, set its default value. */
    if (tabsize == -1)
    {
        tabsize = WIDTH_OF_TAB;
    }
    /* On capable terminals, use colors, otherwise just reverse or bold. */
    if (has_colors())
    {
        set_interface_colorpairs();
    }
    else
    {
        interface_color_pair[TITLE_BAR]     = hilite_attribute;
        interface_color_pair[LINE_NUMBER]   = hilite_attribute;
        interface_color_pair[GUIDE_STRIPE]  = A_REVERSE;
        interface_color_pair[SCROLL_BAR]    = A_NORMAL;
        interface_color_pair[SELECTED_TEXT] = hilite_attribute;
        interface_color_pair[SPOTLIGHTED]   = A_REVERSE;
        interface_color_pair[MINI_INFOBAR]  = hilite_attribute;
        interface_color_pair[PROMPT_BAR]    = hilite_attribute;
        interface_color_pair[STATUS_BAR]    = hilite_attribute;
        interface_color_pair[ERROR_MESSAGE] = hilite_attribute;
        interface_color_pair[KEY_COMBO]     = hilite_attribute;
        interface_color_pair[FUNCTION_TAG]  = A_NORMAL;
    }
    /* Set up the terminal state. */
    terminal_init();
    /* Create the three subwindows, based on the current screen dimensions. */
    window_init();
    curs_set(0);
    sidebar     = (ISSET(INDICATOR) && LINES > 5 && COLS > 9) ? 1 : 0;
    bardata     = (int *)nrealloc(bardata, LINES * sizeof(int));
    editwincols = COLS - sidebar;
    /* Set up the signal handlers. */
    signal_init();
    /* Initialize mouse support. */
    mouse_init();
    /* Ask ncurses for the key codes for most modified editing keys. */
    controlleft        = get_keycode("kLFT5", CONTROL_LEFT);
    controlright       = get_keycode("kRIT5", CONTROL_RIGHT);
    controlup          = get_keycode("kUP5", CONTROL_UP);
    controldown        = get_keycode("kDN5", CONTROL_DOWN);
    controlhome        = get_keycode("kHOM5", CONTROL_HOME);
    controlend         = get_keycode("kEND5", CONTROL_END);
    controldelete      = get_keycode("kDC5", CONTROL_DELETE);
    controlshiftdelete = get_keycode("kDC6", CONTROL_SHIFT_DELETE);
    shiftup            = get_keycode("kUP", SHIFT_UP);
    shiftdown          = get_keycode("kDN", SHIFT_DOWN);
    shiftcontrolleft   = get_keycode("kLFT6", SHIFT_CONTROL_LEFT);
    shiftcontrolright  = get_keycode("kRIT6", SHIFT_CONTROL_RIGHT);
    shiftcontrolup     = get_keycode("kUP6", SHIFT_CONTROL_UP);
    shiftcontroldown   = get_keycode("kDN6", SHIFT_CONTROL_DOWN);
    shiftcontrolhome   = get_keycode("kHOM6", SHIFT_CONTROL_HOME);
    shiftcontrolend    = get_keycode("kEND6", SHIFT_CONTROL_END);
    altleft            = get_keycode("kLFT3", ALT_LEFT);
    altright           = get_keycode("kRIT3", ALT_RIGHT);
    altup              = get_keycode("kUP3", ALT_UP);
    altdown            = get_keycode("kDN3", ALT_DOWN);
    althome            = get_keycode("kHOM3", ALT_HOME);
    altend             = get_keycode("kEND3", ALT_END);
    altpageup          = get_keycode("kPRV3", ALT_PAGEUP);
    altpagedown        = get_keycode("kNXT3", ALT_PAGEDOWN);
    altinsert          = get_keycode("kIC3", ALT_INSERT);
    altdelete          = get_keycode("kDC3", ALT_DELETE);
    shiftaltleft       = get_keycode("kLFT4", SHIFT_ALT_LEFT);
    shiftaltright      = get_keycode("kRIT4", SHIFT_ALT_RIGHT);
    shiftaltup         = get_keycode("kUP4", SHIFT_ALT_UP);
    shiftaltdown       = get_keycode("kDN4", SHIFT_ALT_DOWN);
    mousefocusin       = get_keycode("kxIN", FOCUS_IN);
    mousefocusout      = get_keycode("kxOUT", FOCUS_OUT);
    /* Disable the type-ahead checking that ncurses normally does. */
    typeahead(-1);
#ifdef HAVE_SET_ESCDELAY
    /* Tell ncurses to pass the Esc key quickly. */
    set_escdelay(50);
#endif
    /* Read the files mentioned on the command line into new buffers. */
    while (optind < argc && (!openfile || TRUE))
    {
        long  givenline = 0, givencol = 0;
        char *searchstring = NULL;
        /* If there's a +LINE[,COLUMN] argument here, eat it up. */
        if (optind < argc - 1 && argv[optind][0] == '+')
        {
            int n = 1;
            while (isalpha((char)(argv[optind][n])))
            {
                switch (argv[optind][n++])
                {
                    case 'c' :
                    {
                        SET(CASE_SENSITIVE);
                        break;
                    }
                    case 'C' :
                    {
                        UNSET(CASE_SENSITIVE);
                        break;
                    }
                    case 'r' :
                    {
                        SET(USE_REGEXP);
                        break;
                    }
                    case 'R' :
                    {
                        UNSET(USE_REGEXP);
                        break;
                    }
                    default :
                    {
                        statusline(ALERT, _("Invalid search modifier '%c'"),
                                   argv[optind][n - 1]);
                        break;
                    }
                }
            }
            if (argv[optind][n] == '/' || argv[optind][n] == '?')
            {
                if (argv[optind][n + 1])
                {
                    searchstring = copy_of(&argv[optind][n + 1]);
                    if (argv[optind][n] == '?')
                    {
                        SET(BACKWARDS_SEARCH);
                    }
                }
                else
                {
                    statusline(ALERT, _("Empty search string"));
                }
                optind++;
            }
            else
            {
                /* When there is nothing after the "+", understand it as
                 * go-to-EOF, otherwise parse and store the given number(s). */
                if (argv[optind++][1] == '\0')
                {
                    givenline = -1;
                }
                else if (!parse_line_column(
                             &argv[optind - 1][1], &givenline, &givencol))
                {
                    statusline(ALERT, _("Invalid line or column number"));
                }
            }
        }
        /* If the filename is a dash, read from standard input; otherwise,
         * open the file; skip positioning the cursor if either failed. */
        if (strcmp(argv[optind], "-") == 0)
        {
            optind++;
            if (!scoop_stdin())
            {
                continue;
            }
        }
        else
        {
            /* Consume any flags in cmd line. */
            const unsigned int flag = retriveFlagFromStr(argv[optind]);
            if (flag)
            {
                optind++;
                continue;
            }
            /* Consume any options in cmd line. */
            const unsigned int cliCmd = retriveCliOptionFromStr(argv[optind]);
            if (cliCmd)
            {
                optind += 2;
                continue;
            }
            char       *filename = argv[optind++];
            struct stat fileinfo;
            /* If the filename contains a colon and this file does not exist,
             * then check if the filename ends with digits preceded by a colon
             * (possibly preceded by more digits and a colon).  If there is or
             * are such trailing numbers, chop the colons plus numbers off.
             * The number is later used to place the cursor on that line. */
            if (ISSET(COLON_PARSING) && !givenline &&
                constexpr_strchr(filename, ':') && !givencol &&
                stat(filename, &fileinfo) < 0)
            {
                char *coda = filename + strlen(filename);
            maybe_two:
                while (--coda > filename + 1 && ('0' <= *coda && *coda <= '9'))
                {
                    ;
                }
                if (*coda == ':' && ('0' <= *(coda + 1) && *(coda + 1) <= '9'))
                {
                    *coda = '\0';
                    if (stat(filename, &fileinfo) < 0)
                    {
                        *coda = ':';
                        /* If this was the first colon, look for a second one.
                         */
                        if (!constexpr_strchr(coda + 1, ':'))
                        {
                            goto maybe_two;
                        }
                    }
                    else if (!parse_line_column(
                                 coda + 1, &givenline, &givencol))
                    {
                        die(_("Invalid number\n"));
                    }
                }
            }
            if (!open_buffer(filename, TRUE))
            {
                continue;
            }
        }
        /* If a position was given on the command line, go there. */
        if (givenline != 0 || givencol != 0)
        {
            goto_line_and_column(givenline, givencol, FALSE, FALSE);
        }
        else if (searchstring != NULL)
        {
            if (ISSET(USE_REGEXP))
            {
                regexp_init(searchstring);
            }
            if (!findnextstr(searchstring, FALSE, JUSTFIND, NULL,
                             ISSET(BACKWARDS_SEARCH), openfile->filetop, 0))
            {
                not_found_msg(searchstring);
            }
            else if (lastmessage <= REMARK)
            {
                wipe_statusbar();
            }
            openfile->placewewant = xplustabs();
            adjust_viewport(CENTERING);
            if (ISSET(USE_REGEXP))
            {
                tidy_up_after_search();
            }
            free(last_search);
            last_search  = searchstring;
            searchstring = NULL;
        }
        else if (ISSET(POSITIONLOG) && openfile->filename[0] != '\0')
        {
            long savedline, savedcol;
            /* If edited before, restore the last cursor position. */
            if (has_old_position(argv[optind - 1], &savedline, &savedcol))
            {
                goto_line_and_column(savedline, savedcol, FALSE, FALSE);
            }
        }
    }
    /* After handling the files on the command line, allow inserting files. */
    UNSET(NOREAD_MODE);
    /* Nano is a hands-on editor -- it needs a keyboard. */
    if (!isatty(STDIN_FILENO))
    {
        die(_("Standard input is not a terminal\n"));
    }
    /* If no filenames were given, or all of them were invalid things like
     * directories, then open a blank buffer and allow editing.  Otherwise,
     * switch from the last opened file to the next, that is: the first. */
    if (openfile == NULL)
    {
        open_buffer("", TRUE);
        UNSET(VIEW_MODE);
    }
    else
    {
        openfile = openfile->next;
        (more_than_one) ? mention_name_and_linecount() : void();
        ISSET(VIEW_MODE) ? SET(MULTIBUFFER) : 0;
    }
    if (optind < argc)
    {
        die(_("Can open just one file\n"));
    }
    prepare_for_display();
    if (startup_problem != NULL)
    {
        statusline(ALERT, "%s", startup_problem);
    }
    /* THIS: -> #define NOTREBOUND first_sc_for(MMAIN, do_help) &&
     * first_sc_for(MMAIN, do_help)->keycode == 0x07 Is form nano c source code
     * We will not use this for now as it is not needed */
    if (*openfile->filename == '\0' && openfile->totsize == 0 &&
        openfile->next == openfile && !ISSET(NO_HELP) &&
        (first_sc_for(MMAIN, do_help) &&
         first_sc_for(MMAIN, do_help)->keycode == 0x07))
    {
        statusbar(_("Welcome to NanoX.  For help, type Ctrl+G."));
    }
    do_cpp_syntax();
    /* Set the margin to an impossible value to force re-evaluation. */
    margin         = 12345;
    we_are_running = TRUE;
    LOUT_logI("Reached main loop.");
    /* TODO: This is the main loop of the editor. */
    while (TRUE)
    {
        prosses_callback_queue();
        confirm_margin();
        if (on_a_vt && waiting_keycodes() == 0)
        {
            mute_modifiers = FALSE;
        }
        if (currmenu != MMAIN)
        {
            bottombars(MMAIN);
        }
        if (ISSET(MINIBAR) && !ISSET(ZERO) && LINES > 1 && lastmessage < REMARK)
        {
            minibar();
            if (suggest_on)
            {
                edit_refresh();
                rendr_suggestion();
            }
        }
        else
        {
            /* Update the displayed current cursor position only when there
             * is no message and no keys are waiting in the input buffer. */
            if (ISSET(CONSTANT_SHOW) && lastmessage == VACUUM && LINES > 1 &&
                !ISSET(ZERO) && waiting_keycodes() == 0)
            {
                report_cursor_position();
            }
        }
        as_an_at = TRUE;
        if ((refresh_needed && LINES > 1) ||
            (LINES == 1 && lastmessage <= HUSH))
        {
            edit_refresh();
        }
        else
        {
            place_the_cursor();
        }
        /* In barless mode, either redraw a relevant status message,
         * or overwrite a minor, redundant one. */
        if (ISSET(ZERO) && lastmessage > HUSH)
        {
            if (openfile->cursor_row == editwinrows - 1 && LINES > 1)
            {
                edit_scroll(FORWARD);
                wnoutrefresh(midwin);
            }
            wredrawln(footwin, 0, 1);
            wnoutrefresh(footwin);
            place_the_cursor();
        }
        else if (ISSET(ZERO) && lastmessage > VACUUM)
        {
            wredrawln(midwin, editwinrows - 1, 1);
        }
        errno    = 0;
        focusing = TRUE;
        /* Forget any earlier cursor position at the prompt. */
        put_cursor_at_end_of_answer();
        /* Read in and interpret a single keystroke. */
        process_a_keystroke();
    }
    cleanup_event_handler();
    shutdown_queue();
    cleanup_rendr();
    return 0;
}
