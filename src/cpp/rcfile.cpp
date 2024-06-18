/// @file rcfile.cpp
#include "../include/prototypes.h"


#include <cctype>
#include <cerrno>
#include <cstring>
#include <glob.h>
#include <unistd.h>

#ifndef RCFILE_NAME
#    define HOME_RC_NAME ".nanorc"
#    define RCFILE_NAME  "nanorc"
#else
#    define HOME_RC_NAME RCFILE_NAME
#endif

static const rcoption rcopts[] = {
    {             "boldtext",        BOLD_TEXT},
    {             "brackets",                0},
    {       "breaklonglines", BREAK_LONG_LINES},
    {        "casesensitive",   CASE_SENSITIVE},
    {         "constantshow",    CONSTANT_SHOW},
    {                 "fill",                0},
    {           "historylog",       HISTORYLOG},
    {          "linenumbers",     LINE_NUMBERS},
    {                "magic",        USE_MAGIC},
    {                "mouse",        USE_MOUSE},
    {          "multibuffer",      MULTIBUFFER},
    {               "nohelp",          NO_HELP},
    {           "nonewlines",      NO_NEWLINES},
    {               "nowrap",          NO_WRAP}, /* Deprecated; remove in 2027. */
    {         "operatingdir",                0},
    {          "positionlog",      POSITIONLOG},
    {             "preserve",         PRESERVE},
    {                "punct",                0},
    {             "quotestr",                0},
    {           "quickblank",      QUICK_BLANK},
    {         "rawsequences",    RAW_SEQUENCES},
    {         "rebinddelete",    REBIND_DELETE},
    {               "regexp",       USE_REGEXP},
    {           "saveonexit",     SAVE_ON_EXIT},
    {              "speller",                0},
    {            "afterends",       AFTER_ENDS},
    {"allow_insecure_backup",  INSECURE_BACKUP},
    {             "atblanks",        AT_BLANKS},
    {           "autoindent",       AUTOINDENT},
    {               "backup",      MAKE_BACKUP},
    {            "backupdir",                0},
    {            "bookstyle",        BOOKSTYLE},
    {         "colonparsing",    COLON_PARSING},
    {        "cutfromcursor",  CUT_FROM_CURSOR},
    {            "emptyline",       EMPTY_LINE},
    {          "guidestripe",                0},
    {            "indicator",        INDICATOR},
    {       "jumpyscrolling",  JUMPY_SCROLLING},
    {              "locking",          LOCKING},
    {        "matchbrackets",                0},
    {              "minibar",          MINIBAR},
    {            "noconvert",       NO_CONVERT},
    {           "showcursor",      SHOW_CURSOR},
    {            "smarthome",       SMART_HOME},
    {             "softwrap",         SOFTWRAP},
    {           "stateflags",       STATEFLAGS},
    {              "tabsize",                0},
    {         "tabstospaces",   TABS_TO_SPACES},
    {           "trimblanks",      TRIM_BLANKS},
    {                 "unix",     MAKE_IT_UNIX},
    {           "whitespace",                0},
    {           "wordbounds",      WORD_BOUNDS},
    {            "wordchars",                0},
    {                  "zap",     LET_THEM_ZAP},
    {                 "zero",             ZERO},
    {           "titlecolor",                0},
    {          "numbercolor",                0},
    {          "stripecolor",                0},
    {        "scrollercolor",                0},
    {        "selectedcolor",                0},
    {       "spotlightcolor",                0},
    {            "minicolor",                0},
    {          "promptcolor",                0},
    {          "statuscolor",                0},
    {           "errorcolor",                0},
    {             "keycolor",                0},
    {        "functioncolor",                0},
    {                   NULL,                0}
};

/* The line number of the last encountered error. */
static size_t lineno = 0;

/* The path to the rcfile we're parsing. */
static char *nanorc = NULL;

/* Whether we're allowed to add to the last syntax.  When a file ends,
 * or when a new syntax command is seen, this bool becomes FALSE. */
static bool opensyntax = FALSE;

/* The syntax that is currently being parsed. */
static syntaxtype *live_syntax;

/* Whether a syntax definition contains any color commands. */
static bool seen_color_command = FALSE;

/* The end of the color list for the current syntax. */
static colortype *lastcolor = NULL;


static linestruct *errors_head = nullptr;
static linestruct *errors_tail = nullptr;
/* Beginning and end of a list of errors in rcfiles, if any. */

/* Send the gathered error messages (if any) to the terminal. */
void
display_rcfile_errors()
{
    for (linestruct *error = errors_head; error != nullptr; error = error->next)
    {
        fprintf(stderr, "%s\n", error->data);
    }
}

#define MAXSIZE (PATH_MAX + 200)

//
/// @name @c jot_error
///
/// @brief
///  -  Store the given error message in a linked list, to be printed upon exit.
///
/// @param msg
///  -  The error message, possibly with @c printf-style format specifiers.
///
/// @param ...
///  -  The arguments to be inserted into the format specifiers.
///
/// @returns @c void
//
void
jot_error(const s8 *msg, ...)
{
    linestruct *error = make_new_node(errors_tail);
    va_list     ap;

    s8  textbuf[MAXSIZE];
    s32 length = 0;

    if (errors_head == nullptr)
    {
        errors_head = error;
    }
    else
    {
        errors_tail->next = error;
    }
    errors_tail = error;

    if (startup_problem == nullptr)
    {
        if (nanorc != nullptr)
        {
            snprintf(textbuf, MAXSIZE, _("Mistakes in '%s'"), nanorc);
            startup_problem = copy_of(textbuf);
        }
        else
        {
            startup_problem = copy_of(_("Problems with history file"));
        }
    }
    if (lineno > 0)
    {
        length = snprintf(textbuf, MAXSIZE, _("Error in %s on line %zu: "), nanorc, lineno);
    }
    va_start(ap, msg);
    length += vsnprintf(textbuf + length, MAXSIZE - length, _(msg), ap);
    va_end(ap);

    error->data = static_cast<s8 *>(nmalloc(length + 1));
    sprintf(error->data, "%s", textbuf);
}

// Interpret a function string given in the rc file, and return a
// shortcut record with the corresponding function filled in.
keystruct *
strtosc(const s8 *input)
{
    keystruct *s = RE_CAST(keystruct *, nmalloc(sizeof(keystruct)));

#ifndef NANO_TINY
    s->toggle = 0;
#endif

    if (!strcmp(input, "cancel"))
    {
        s->func = do_cancel;
    }
#ifdef ENABLE_HELP
    else if (!strcmp(input, "help"))
    {
        s->func = do_help;
    }
#endif
    else if (!strcmp(input, "exit"))
    {
        s->func = do_exit;
    }
    else if (!strcmp(input, "discardbuffer"))
    {
        s->func = discard_buffer;
    }
    else if (!strcmp(input, "writeout"))
    {
        s->func = do_writeout;
    }
    else if (!strcmp(input, "savefile"))
    {
        s->func = do_savefile;
    }
    else if (!strcmp(input, "insert"))
    {
        s->func = do_insertfile;
    }
    else if (!strcmp(input, "whereis"))
    {
        s->func = do_search_forward;
    }
    else if (!strcmp(input, "wherewas"))
    {
        s->func = do_search_backward;
    }
    else if (!strcmp(input, "findprevious"))
    {
        s->func = do_findprevious;
    }
    else if (!strcmp(input, "findnext"))
    {
        s->func = do_findnext;
    }
    else if (!strcmp(input, "replace"))
    {
        s->func = do_replace;
    }
    else if (!strcmp(input, "cut"))
    {
        s->func = cut_text;
    }
    else if (!strcmp(input, "copy"))
    {
        s->func = copy_text;
    }
    else if (!strcmp(input, "paste"))
    {
        s->func = paste_text;
    }
#ifndef NANO_TINY
    else if (!strcmp(input, "execute"))
    {
        s->func = do_execute;
    }
    else if (!strcmp(input, "cutrestoffile"))
    {
        s->func = cut_till_eof;
    }
    else if (!strcmp(input, "zap"))
    {
        s->func = zap_text;
    }
    else if (!strcmp(input, "mark"))
    {
        s->func = do_mark;
    }
#endif
#ifdef ENABLE_SPELLER
    else if (!strcmp(input, "tospell") || !strcmp(input, "speller"))
    {
        s->func = do_spell;
    }
#endif
#ifdef ENABLE_LINTER
    else if (!strcmp(input, "linter"))
    {
        s->func = do_linter;
    }
#endif
#ifdef ENABLE_FORMATTER
    else if (!strcmp(input, "formatter"))
    {
        s->func = do_formatter;
    }
#endif
    else if (!strcmp(input, "location"))
    {
        s->func = report_cursor_position;
    }
    else if (!strcmp(input, "gotoline"))
    {
        s->func = do_gotolinecolumn;
    }
#ifdef ENABLE_JUSTIFY
    else if (!strcmp(input, "justify"))
    {
        s->func = do_justify;
    }
    else if (!strcmp(input, "fulljustify"))
    {
        s->func = do_full_justify;
    }
    else if (!strcmp(input, "beginpara"))
    {
        s->func = to_para_begin;
    }
    else if (!strcmp(input, "endpara"))
    {
        s->func = to_para_end;
    }
#endif
#ifdef ENABLE_COMMENT
    else if (!strcmp(input, "comment"))
    {
        s->func = do_comment;
    }
#endif
#ifdef ENABLE_WORDCOMPLETION
    else if (!strcmp(input, "complete"))
    {
        s->func = complete_a_word;
    }
#endif
#ifndef NANO_TINY
    else if (!strcmp(input, "indent"))
    {
        s->func = do_indent;
    }
    else if (!strcmp(input, "unindent"))
    {
        s->func = do_unindent;
    }
    else if (!strcmp(input, "chopwordleft"))
    {
        s->func = chop_previous_word;
    }
    else if (!strcmp(input, "chopwordright"))
    {
        s->func = chop_next_word;
    }
    else if (!strcmp(input, "findbracket"))
    {
        s->func = do_find_bracket;
    }
    else if (!strcmp(input, "wordcount"))
    {
        s->func = count_lines_words_and_characters;
    }
    else if (!strcmp(input, "recordmacro"))
    {
        s->func = record_macro;
    }
    else if (!strcmp(input, "runmacro"))
    {
        s->func = run_macro;
    }
    else if (!strcmp(input, "anchor"))
    {
        s->func = put_or_lift_anchor;
    }
    else if (!strcmp(input, "prevanchor"))
    {
        s->func = to_prev_anchor;
    }
    else if (!strcmp(input, "nextanchor"))
    {
        s->func = to_next_anchor;
    }
    else if (!strcmp(input, "undo"))
    {
        s->func = do_undo;
    }
    else if (!strcmp(input, "redo"))
    {
        s->func = do_redo;
    }
    else if (!strcmp(input, "suspend"))
    {
        s->func = do_suspend;
    }
#endif
    else if (!strcmp(input, "left") || !strcmp(input, "back"))
    {
        s->func = do_left;
    }
    else if (!strcmp(input, "right") || !strcmp(input, "forward"))
    {
        s->func = do_right;
    }
    else if (!strcmp(input, "up") || !strcmp(input, "prevline"))
    {
        s->func = do_up;
    }
    else if (!strcmp(input, "down") || !strcmp(input, "nextline"))
    {
        s->func = do_down;
    }
#if !defined(NANO_TINY) || defined(ENABLE_HELP)
    else if (!strcmp(input, "scrollup"))
    {
        s->func = do_scroll_up;
    }
    else if (!strcmp(input, "scrolldown"))
    {
        s->func = do_scroll_down;
    }
#endif
    else if (!strcmp(input, "prevword"))
    {
        s->func = to_prev_word;
    }
    else if (!strcmp(input, "nextword"))
    {
        s->func = to_next_word;
    }
    else if (!strcmp(input, "home"))
    {
        s->func = do_home;
    }
    else if (!strcmp(input, "end"))
    {
        s->func = do_end;
    }
    else if (!strcmp(input, "prevblock"))
    {
        s->func = to_prev_block;
    }
    else if (!strcmp(input, "nextblock"))
    {
        s->func = to_next_block;
    }
#ifndef NANO_TINY
    else if (!strcmp(input, "toprow"))
    {
        s->func = to_top_row;
    }
    else if (!strcmp(input, "bottomrow"))
    {
        s->func = to_bottom_row;
    }
    else if (!strcmp(input, "center"))
    {
        s->func = do_center;
    }
    else if (!strcmp(input, "cycle"))
    {
        s->func = do_cycle;
    }
#endif
    else if (!strcmp(input, "pageup") || !strcmp(input, "prevpage"))
    {
        s->func = do_page_up;
    }
    else if (!strcmp(input, "pagedown") || !strcmp(input, "nextpage"))
    {
        s->func = do_page_down;
    }
    else if (!strcmp(input, "firstline"))
    {
        s->func = to_first_line;
    }
    else if (!strcmp(input, "lastline"))
    {
        s->func = to_last_line;
    }
#ifdef ENABLE_MULTIBUFFER
    else if (!strcmp(input, "prevbuf"))
    {
        s->func = switch_to_prev_buffer;
    }
    else if (!strcmp(input, "nextbuf"))
    {
        s->func = switch_to_next_buffer;
    }
#endif
    else if (!strcmp(input, "verbatim"))
    {
        s->func = do_verbatim_input;
    }
    else if (!strcmp(input, "tab"))
    {
        s->func = do_tab;
    }
    else if (!strcmp(input, "enter"))
    {
        s->func = do_enter;
    }
    else if (!strcmp(input, "delete"))
    {
        s->func = do_delete;
    }
    else if (!strcmp(input, "backspace"))
    {
        s->func = do_backspace;
    }
    else if (!strcmp(input, "refresh"))
    {
        s->func = full_refresh;
    }
    else if (!strcmp(input, "casesens"))
    {
        s->func = case_sens_void;
    }
    else if (!strcmp(input, "regexp"))
    {
        s->func = regexp_void;
    }
    else if (!strcmp(input, "backwards"))
    {
        s->func = backwards_void;
    }
    else if (!strcmp(input, "flipreplace"))
    {
        s->func = flip_replace;
    }
    else if (!strcmp(input, "flipgoto"))
    {
        s->func = flip_goto;
    }
#ifdef ENABLE_HISTORIES
    else if (!strcmp(input, "older"))
    {
        s->func = get_older_item;
    }
    else if (!strcmp(input, "newer"))
    {
        s->func = get_newer_item;
    }
#endif
#ifndef NANO_TINY
    else if (!strcmp(input, "dosformat"))
    {
        s->func = dos_format;
    }
    else if (!strcmp(input, "macformat"))
    {
        s->func = mac_format;
    }
    else if (!strcmp(input, "append"))
    {
        s->func = append_it;
    }
    else if (!strcmp(input, "prepend"))
    {
        s->func = prepend_it;
    }
    else if (!strcmp(input, "backup"))
    {
        s->func = back_it_up;
    }
    else if (!strcmp(input, "flipexecute"))
    {
        s->func = flip_execute;
    }
    else if (!strcmp(input, "flippipe"))
    {
        s->func = flip_pipe;
    }
    else if (!strcmp(input, "flipconvert"))
    {
        s->func = flip_convert;
    }
#endif
#ifdef ENABLE_MULTIBUFFER
    else if (!strcmp(input, "flipnewbuffer"))
    {
        s->func = flip_newbuffer;
    }
#endif
#ifdef ENABLE_BROWSER
    else if (!strcmp(input, "tofiles") || !strcmp(input, "browser"))
    {
        s->func = to_files;
    }
    else if (!strcmp(input, "gotodir"))
    {
        s->func = goto_dir;
    }
    else if (!strcmp(input, "firstfile"))
    {
        s->func = to_first_file;
    }
    else if (!strcmp(input, "lastfile"))
    {
        s->func = to_last_file;
    }
#endif
    else
    {
#ifndef NANO_TINY
        s->func = do_toggle;
        if (!strcmp(input, "nohelp"))
        {
            s->toggle = NO_HELP;
        }
        else if (!strcmp(input, "zero"))
        {
            s->toggle = ZERO;
        }
        else if (!strcmp(input, "constantshow"))
        {
            s->toggle = CONSTANT_SHOW;
        }
        else if (!strcmp(input, "softwrap"))
        {
            s->toggle = SOFTWRAP;
        }
#    ifdef ENABLE_LINENUMBERS
        else if (!strcmp(input, "linenumbers"))
        {
            s->toggle = LINE_NUMBERS;
        }
#    endif
        else if (!strcmp(input, "whitespacedisplay"))
        {
            s->toggle = WHITESPACE_DISPLAY;
        }
#    ifdef ENABLE_COLOR
        else if (!strcmp(input, "nosyntax"))
        {
            s->toggle = NO_SYNTAX;
        }
#    endif
        else if (!strcmp(input, "smarthome"))
        {
            s->toggle = SMART_HOME;
        }
        else if (!strcmp(input, "autoindent"))
        {
            s->toggle = AUTOINDENT;
        }
        else if (!strcmp(input, "cutfromcursor"))
        {
            s->toggle = CUT_FROM_CURSOR;
        }
#    ifdef ENABLE_WRAPPING
        else if (!strcmp(input, "breaklonglines"))
        {
            s->toggle = BREAK_LONG_LINES;
        }
#    endif
        else if (!strcmp(input, "tabstospaces"))
        {
            s->toggle = TABS_TO_SPACES;
        }
#    ifdef ENABLE_MOUSE
        else if (!strcmp(input, "mouse"))
        {
            s->toggle = USE_MOUSE;
        }
#    endif
        else
#endif /* !NANO_TINY */
        {
            free(s);
            return NULL;
        }
    }
    return s;
}

#define NUMBER_OF_MENUS 16
s8 *menunames[NUMBER_OF_MENUS] = {_("main"),    _("search"),      _("replace"),  _("replacewith"),
                                  _("yesno"),   _("gotoline"),    _("writeout"), _("insert"),
                                  _("execute"), _("help"),        _("spell"),    _("linter"),
                                  _("browser"), _("whereisfile"), _("gotodir"),  _("all")};

s32 menusymbols[NUMBER_OF_MENUS] = {
    MMAIN,    MWHEREIS, MREPLACE, MREPLACEWITH, MYESNO,   MGOTOLINE,    MWRITEFILE, MINSERTFILE,
    MEXECUTE, MHELP,    MSPELL,   MLINTER,      MBROWSER, MWHEREISFILE, MGOTODIR,   MMOST | MBROWSER | MHELP | MYESNO};

/* Return the symbol that corresponds to the given menu name. */
int
name_to_menu(const char *name)
{
    int index = -1;

    while (++index < NUMBER_OF_MENUS)
    {
        if (strcmp(name, menunames[index]) == 0)
        {
            return menusymbols[index];
        }
    }

    return 0;
}

/* Return the name that corresponds to the given menu symbol. */
char *
menu_to_name(int menu)
{
    int index = -1;

    while (++index < NUMBER_OF_MENUS)
    {
        if (menusymbols[index] == menu)
        {
            return menunames[index];
        }
    }

    return _("boooo");
}

// Parse the next word from the string, null-terminate it, and return
// a pointer to the first character after the null terminator.  The
// returned pointer will point to '\0' if we hit the end of the line.
char *
parse_next_word(char *ptr)
{
    while (!isblank((unsigned char)*ptr) && *ptr != '\0')
    {
        ptr++;
    }

    if (*ptr == '\0')
    {
        return ptr;
    }

    /* Null-terminate and advance ptr. */
    *ptr++ = '\0';

    while (isblank((unsigned char)*ptr))
    {
        ptr++;
    }

    return ptr;
}

/* Parse an argument, with optional quotes, after a keyword that takes
 * one.  If the next word starts with a ", we say that it ends with the
 * last " of the line.  Otherwise, we interpret it as usual, so that the
 * arguments can contain "'s too. */
s8 *
parse_argument(s8 *ptr)
{
    const s8 *ptr_save   = ptr;
    s8       *last_quote = nullptr;

    if (*ptr != '"')
    {
        return parse_next_word(ptr);
    }

    while (*ptr != '\0')
    {
        if (*++ptr == '"')
        {
            last_quote = ptr;
        }
    }

    if (last_quote == nullptr)
    {
        jot_error(N_("Argument '%s' has an unterminated \""), ptr_save);
        return nullptr;
    }

    *last_quote = '\0';
    ptr         = last_quote + 1;

    while (isblank(static_cast<u8>(*ptr)))
    {
        ptr++;
    }

    return ptr;
}

#ifdef ENABLE_COLOR
/* Advance over one regular expression in the line starting at ptr,
 * null-terminate it, and return a pointer to the succeeding text. */
char *
parse_next_regex(char *ptr)
{
    char *starting_point = ptr;

    if (*(ptr - 1) != '"')
    {
        jot_error(N_("Regex strings must begin and end with a \" character"));
        return NULL;
    }

    /* Continue until the end of the line, or until a double quote followed
     * by end-of-line or a blank. */
    while (*ptr != '\0' && (*ptr != '"' || (ptr[1] != '\0' && !isblank((unsigned char)ptr[1]))))
    {
        ptr++;
    }

    if (*ptr == '\0')
    {
        jot_error(N_("Regex strings must begin and end with a \" character"));
        return NULL;
    }

    if (ptr == starting_point)
    {
        jot_error(N_("Empty regex string"));
        return NULL;
    }

    /* Null-terminate the regex and skip until the next non-blank. */
    *ptr++ = '\0';

    while (isblank((unsigned char)*ptr))
    {
        ptr++;
    }

    return ptr;
}

/* Compile the given regular expression and store the result in packed (when
 * this pointer is not NULL).  Return TRUE when the expression is valid. */
bool
compile(const char *expression, int rex_flags, regex_t **packed)
{
    regex_t *compiled = RE_CAST(regex_t *, nmalloc(sizeof(regex_t)));
    int      outcome  = regcomp(compiled, expression, rex_flags);

    if (outcome != 0)
    {
        size_t length  = regerror(outcome, compiled, NULL, 0);
        char  *message = RE_CAST(char *, nmalloc(length));

        regerror(outcome, compiled, message, length);
        jot_error(N_("Bad regex \"%s\": %s"), expression, message);
        free(message);

        regfree(compiled);
        free(compiled);
    }
    else
    {
        *packed = compiled;
    }

    return (outcome == 0);
}

/* Parse the next syntax name and its possible extension regexes from the
 * line at ptr, and add it to the global linked list of color syntaxes. */
void
begin_new_syntax(char *ptr)
{
    char *nameptr = ptr;

    /* Check that the syntax name is not empty. */
    if (*ptr == '\0' || (*ptr == '"' && (*(ptr + 1) == '\0' || *(ptr + 1) == '"')))
    {
        jot_error(N_("Missing syntax name"));
        return;
    }

    ptr = parse_next_word(ptr);

    /* Check that quotes around the name are either paired or absent. */
    if ((*nameptr == '\x22') ^ (nameptr[strlen(nameptr) - 1] == '\x22'))
    {
        jot_error(N_("Unpaired quote in syntax name"));
        return;
    }

    /* If the name is quoted, strip the quotes. */
    if (*nameptr == '\x22')
    {
        nameptr++;
        nameptr[strlen(nameptr) - 1] = '\0';
    }

    /* Redefining the "none" syntax is not allowed. */
    if (strcmp(nameptr, "none") == 0)
    {
        jot_error(N_("The \"none\" syntax is reserved"));
        return;
    }

    /* Initialize a new syntax struct. */
    live_syntax                = RE_CAST(syntaxtype *, nmalloc(sizeof(syntaxtype)));
    live_syntax->name          = copy_of(nameptr);
    live_syntax->filename      = copy_of(nanorc);
    live_syntax->lineno        = lineno;
    live_syntax->augmentations = NULL;
    live_syntax->extensions    = NULL;
    live_syntax->headers       = NULL;
    live_syntax->magics        = NULL;
    live_syntax->linter        = NULL;
    live_syntax->formatter     = NULL;
    live_syntax->tabstring     = NULL;
#    ifdef ENABLE_COMMENT
    live_syntax->comment = copy_of(GENERAL_COMMENT_CHARACTER);
#    endif
    live_syntax->color      = NULL;
    live_syntax->multiscore = 0;

    /* Hook the new syntax in at the top of the list. */
    live_syntax->next = syntaxes;
    syntaxes          = live_syntax;

    opensyntax         = TRUE;
    seen_color_command = FALSE;

    /* The default syntax should have no associated extensions. */
    if (strcmp(live_syntax->name, "default") == 0 && *ptr != '\0')
    {
        jot_error(N_("The \"default\" syntax does not accept extensions"));
        return;
    }

    /* If there seem to be extension regexes, pick them up. */
    if (*ptr != '\0')
    {
        grab_and_store("extension", ptr, &live_syntax->extensions);
    }
}
#endif /* ENABLE_COLOR */

/* Verify that a syntax definition contains at least one color command. */
void
check_for_nonempty_syntax()
{
    if (opensyntax && !seen_color_command)
    {
        size_t current_lineno = lineno;

        lineno = live_syntax->lineno;
        jot_error(N_("Syntax \"%s\" has no color commands"), live_syntax->name);
        lineno = current_lineno;
    }

    opensyntax = false;
}

/* Return TRUE when the given function is present in almost all menus. */
bool
is_universal(void (*func)())
{
    return (func == do_left || func == do_right || func == do_home || func == do_end ||
#ifndef NANO_TINY
            func == to_prev_word || func == to_next_word ||
#endif
            func == do_delete || func == do_backspace || func == cut_text || func == paste_text || func == do_tab ||
            func == do_enter || func == do_verbatim_input);
}

/* Bind or unbind a key combo, to or from a function. */
void
parse_binding(char *ptr, bool dobind)
{
    char      *keyptr = NULL, *keycopy = NULL, *funcptr = NULL, *menuptr = NULL;
    int        keycode, menu, mask = 0;
    keystruct *newsc = NULL;

    check_for_nonempty_syntax();

    if (*ptr == '\0')
    {
        jot_error(N_("Missing key name"));
        return;
    }

    keyptr  = ptr;
    ptr     = parse_next_word(ptr);
    keycopy = copy_of(keyptr);

    /* Uppercase either the second or the first character of the key name. */
    if (keycopy[0] == '^')
    {
        keycopy[1] = toupper((unsigned char)keycopy[1]);
    }
    else
    {
        keycopy[0] = toupper((unsigned char)keycopy[0]);
    }

    /* Verify that the key name is not too short, to allow the next call. */
    if (keycopy[1] == '\0' || (keycopy[0] == 'M' && keycopy[2] == '\0'))
    {
        jot_error(N_("Key name %s is invalid"), keycopy);
        goto free_things;
    }

    keycode = keycode_from_string(keycopy);

    if (keycode < 0)
    {
        jot_error(N_("Key name %s is invalid"), keycopy);
        goto free_things;
    }

    if (dobind)
    {
        funcptr = ptr;
        ptr     = parse_argument(ptr);

        if (funcptr[0] == '\0')
        {
            jot_error(N_("Must specify a function to bind the key to"));
            goto free_things;
        }
        else if (ptr == NULL)
        {
            goto free_things;
        }
    }

    menuptr = ptr;
    ptr     = parse_next_word(ptr);

    if (menuptr[0] == '\0')
    {
        /* TRANSLATORS: Do not translate the word "all". */
        jot_error(N_("Must specify a menu (or \"all\") "
                     "in which to bind/unbind the key"));
        goto free_things;
    }

    menu = name_to_menu(menuptr);
    if (menu == 0)
    {
        jot_error(N_("Unknown menu: %s"), menuptr);
        goto free_things;
    }

    if (dobind)
    {
        /* If the thing to bind starts with a double quote, it is a string,
         * otherwise it is the name of a function. */
        if (*funcptr == '"')
        {
            newsc            = RE_CAST(keystruct *, nmalloc(sizeof(keystruct)));
            newsc->func      = (functionptrtype)implant;
            newsc->expansion = copy_of(funcptr + 1);
#ifndef NANO_TINY
            newsc->toggle = 0;
#endif
        }
        else
        {
            newsc = strtosc(funcptr);
        }

        if (newsc == NULL)
        {
            jot_error(N_("Unknown function: %s"), funcptr);
            goto free_things;
        }
    }

    /* Wipe the given shortcut from the given menu. */
    for (keystruct *s = sclist; s != NULL; s = s->next)
    {
        if ((s->menus & menu) && s->keycode == keycode)
        {
            s->menus &= ~menu;
        }
    }

    /* When unbinding, we are done now. */
    if (!dobind)
    {
        goto free_things;
    }

    /* Limit the given menu to those where the function exists;
     * first handle five special cases, then the general case. */
    if (is_universal(newsc->func))
    {
        menu &= MMOST | MBROWSER;
    }
#ifndef NANO_TINY
    else if (newsc->func == do_toggle && newsc->toggle == NO_HELP)
    {
        menu &= (MMOST | MBROWSER | MYESNO) & ~MFINDINHELP;
    }
    else if (newsc->func == do_toggle)
    {
        menu &= MMAIN;
    }
#endif
    else if (newsc->func == full_refresh)
    {
        menu &= MMOST | MBROWSER | MHELP | MYESNO;
    }
    else if (newsc->func == (functionptrtype)implant)
    {
        menu &= MMOST | MBROWSER | MHELP;
    }
    else
    {
        /* Tally up the menus where the function exists. */
        for (funcstruct *f = allfuncs; f != NULL; f = f->next)
        {
            if (f->func == newsc->func)
            {
                mask |= f->menus;
            }
        }

        menu &= mask;
    }

    if (!menu)
    {
        if (!ISSET(RESTRICTED) && !ISSET(VIEW_MODE))
        {
            jot_error(N_("Function '%s' does not exist in menu '%s'"), funcptr, menuptr);
        }
        goto free_things;
    }

    newsc->menus   = menu;
    newsc->keystr  = keycopy;
    newsc->keycode = keycode;

    /* Disallow rebinding <Esc> (^[). */
    if (newsc->keycode == ESC_CODE)
    {
        jot_error(N_("Keystroke %s may not be rebound"), keycopy);
    free_things:
        free(keycopy);
        free(newsc);
        return;
    }

#ifndef NANO_TINY
    /* If this is a toggle, find and copy its sequence number. */
    if (newsc->func == do_toggle)
    {
        for (keystruct *s = sclist; s != NULL; s = s->next)
        {
            if (s->func == do_toggle && s->toggle == newsc->toggle)
            {
                newsc->ordinal = s->ordinal;
            }
        }
    }
    else
    {
        newsc->ordinal = 0;
    }
#endif
    /* Add the new shortcut at the start of the list. */
    newsc->next = sclist;
    sclist      = newsc;
}

// Verify that the given file exists, is not a folder nor a device.
bool
is_good_file(s8 *file)
{
    struct stat rcinfo;

    // First check that the file exists and is readable.
    if (access(file, R_OK) != 0)
    {
        return false;
    }

    // If the thing exists, it may be neither a directory nor a device.
    if (stat(file, &rcinfo) != -1 && (S_ISDIR(rcinfo.st_mode) || S_ISCHR(rcinfo.st_mode) || S_ISBLK(rcinfo.st_mode)))
    {
        jot_error(S_ISDIR(rcinfo.st_mode) ? N_("\"%s\" is a directory") : N_("\"%s\" is a device file"), file);
        return false;
    }
    else
    {
        return true;
    }
}

/* Partially parse the syntaxes in the given file, or (when syntax
 * is not NULL) fully parse one specific syntax from the file. */
void
parse_one_include(char *file, syntaxtype *syntax)
{
    char          *was_nanorc = nanorc;
    size_t         was_lineno = lineno;
    augmentstruct *extra;
    FILE          *rcstream;

    /* Don't open directories, character files, or block files. */
    if (access(file, R_OK) == 0 && !is_good_file(file))
    {
        return;
    }

    rcstream = fopen(file, "rb");

    if (rcstream == NULL)
    {
        jot_error(N_("Error reading %s: %s"), file, strerror(errno));
        return;
    }

    /* Use the name and line number position of the included syntax file
     * while parsing it, so we can know where any errors in it are. */
    nanorc = file;
    lineno = 0;

    /* If this is the first pass, parse only the prologue. */
    if (syntax == NULL)
    {
        parse_rcfile(rcstream, TRUE, TRUE);
        nanorc = was_nanorc;
        lineno = was_lineno;
        return;
    }

    live_syntax = syntax;
    lastcolor   = NULL;

    /* Fully parse the given syntax (as it is about to be used). */
    parse_rcfile(rcstream, TRUE, FALSE);

    extra = syntax->augmentations;

    /* Apply any stored extendsyntax commands. */
    while (extra != NULL)
    {
        char *keyword = extra->data;
        char *therest = parse_next_word(extra->data);

        nanorc = extra->filename;
        lineno = extra->lineno;

        if (!parse_syntax_commands(keyword, therest))
        {
            jot_error(N_("Command \"%s\" not understood"), keyword);
        }

        extra = extra->next;
    }

    free(syntax->filename);
    syntax->filename = NULL;

    nanorc = was_nanorc;
    lineno = was_lineno;
}

/* Expand globs in the passed name, and parse the resultant files. */
void
parse_includes(char *ptr)
{
    char  *pattern, *expanded;
    glob_t files;
    int    result;

    check_for_nonempty_syntax();

    pattern = ptr;
    if (*pattern == '"')
    {
        pattern++;
    }
    ptr = parse_argument(ptr);

    if (strlen(pattern) > PATH_MAX)
    {
        jot_error(N_("Path is too long"));
        return;
    }

    /* Expand a tilde first, then try to match the globbing pattern. */
    expanded = real_dir_from_tilde(pattern);
    result   = glob(expanded, GLOB_ERR | GLOB_NOCHECK, NULL, &files);

    /* If there are matches, process each of them.  Otherwise, only
     * report an error if it's something other than zero matches. */
    if (result == 0)
    {
        for (size_t i = 0; i < files.gl_pathc; ++i)
        {
            parse_one_include(files.gl_pathv[i], NULL);
        }
    }
    else if (result != GLOB_NOMATCH)
    {
        jot_error(N_("Error expanding %s: %s"), pattern, strerror(errno));
    }

    globfree(&files);
    free(expanded);
}

//
/// @name
///  - @c closest_index_color
///
/// @brief
///  -  Return the index of the color that is closest to the given RGB levels,
///     assuming that the terminal uses the 6x6x6 color cube of xterm-256color.
///     When red == green == blue, return an index in the xterm gray scale.
///
/// @param red ( short )
///  -  The red color level.
///
/// @param green ( short )
///  -  The green color level.
///
/// @param blue ( short )
///  -  The blue color level.
///
/// @returns ( short )
///  -  The index of the color that is closest to the given RGB levels.
//
s16
closest_index_color(s16 red, s16 green, s16 blue)
{
    /* Translation table, from 16 intended color levels to 6 available levels. */
    static const short level[] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5};

    /* Translation table, from 14 intended gray levels to 24 available levels. */
    static const short gray[] = {1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 15, 18, 21, 23};

    if (COLORS != 256)
    {
        return THE_DEFAULT;
    }
    else if (red == green && green == blue && 0 < red && red < 0xF)
    {
        return 232 + gray[red - 1];
    }
    else
    {
        return (36 * level[red] + 6 * level[green] + level[blue] + 16);
    }
}

#define COLORCOUNT 34

const s8 hues[COLORCOUNT][8] = {"red",   "green",  "blue",  "yellow", "cyan",    "magenta", "white", "black",  "normal",
                                "pink",  "purple", "mauve", "lagoon", "mint",    "lime",    "peach", "orange", "latte",
                                "rosy",  "beet",   "plum",  "sea",    "sky",     "slate",   "teal",  "sage",   "brown",
                                "ocher", "sand",   "tawny", "brick",  "crimson", "grey",    "gray"};

s16 indices[COLORCOUNT] = {COLOR_RED,
                           COLOR_GREEN,
                           COLOR_BLUE,
                           COLOR_YELLOW,
                           COLOR_CYAN,
                           COLOR_MAGENTA,
                           COLOR_WHITE,
                           COLOR_BLACK,
                           THE_DEFAULT,
                           204,
                           163,
                           134,
                           38,
                           48,
                           148,
                           215,
                           208,
                           137,
                           175,
                           127,
                           98,
                           32,
                           111,
                           66,
                           35,
                           107,
                           100,
                           142,
                           186,
                           136,
                           166,
                           161,
                           COLOR_BLACK + 8,
                           COLOR_BLACK + 8};

/* Return the short value corresponding to the given color name, and set
 * vivid to TRUE for a lighter color, and thick for a heavier typeface. */
short
color_to_short(const s8 *colorname, bool *vivid, bool *thick)
{
    if (strncmp(colorname, "bright", 6) == 0 && colorname[6] != '\0')
    {
        /* Prefix "bright" is deprecated; remove in 2027. */
        *vivid = TRUE;
        *thick = TRUE;
        colorname += 6;
    }
    else if (strncmp(colorname, "light", 5) == 0 && colorname[5] != '\0')
    {
        *vivid = TRUE;
        *thick = FALSE;
        colorname += 5;
    }
    else
    {
        *vivid = FALSE;
        *thick = FALSE;
    }

    if (colorname[0] == '#' && strlen(colorname) == 4)
    {
        u16 r, g, b;

        if (*vivid)
        {
            jot_error(N_("Color '%s' takes no prefix"), colorname);
            return BAD_COLOR;
        }

        if (sscanf(colorname, "#%1hX%1hX%1hX", &r, &g, &b) == 3)
        {
            return closest_index_color(r, g, b);
        }
    }

    for (int index = 0; index < COLORCOUNT; index++)
    {
        if (strcmp(colorname, hues[index]) == 0)
        {
            if (index > 7 && *vivid)
            {
                jot_error(N_("Color '%s' takes no prefix"), colorname);
                return BAD_COLOR;
            }
            else if (index > 8 && COLORS < 255)
            {
                return THE_DEFAULT;
            }
            else
            {
                return indices[index];
            }
        }
    }

    jot_error(N_("Color \"%s\" not understood"), colorname);
    return BAD_COLOR;
}

/* Parse the color name (or pair of color names) in the given string.
 * Return FALSE when any color name is invalid; otherwise return TRUE. */
bool
parse_combination(char *combotext, short *fg, short *bg, int *attributes)
{
    bool  vivid, thick;
    char *comma;

    *attributes = A_NORMAL;

    if (strncmp(combotext, "bold", 4) == 0)
    {
        *attributes |= A_BOLD;
        if (combotext[4] != ',')
        {
            jot_error(N_("An attribute requires a subsequent comma"));
            return FALSE;
        }
        combotext += 5;
    }

    if (strncmp(combotext, "italic", 6) == 0)
    {
#ifdef A_ITALIC
        *attributes |= A_ITALIC;
#endif
        if (combotext[6] != ',')
        {
            jot_error(N_("An attribute requires a subsequent comma"));
            return FALSE;
        }
        combotext += 7;
    }

    comma = strchr(combotext, ',');

    if (comma)
    {
        *comma = '\0';
    }

    if (!comma || comma > combotext)
    {
        *fg = color_to_short(combotext, &vivid, &thick);
        if (*fg == BAD_COLOR)
        {
            return FALSE;
        }
        if (vivid && !thick && COLORS > 8)
        {
            *fg += 8;
        }
        else if (vivid)
        {
            *attributes |= A_BOLD;
        }
    }
    else
    {
        *fg = THE_DEFAULT;
    }

    if (comma)
    {
        *bg = color_to_short(comma + 1, &vivid, &thick);
        if (*bg == BAD_COLOR)
        {
            return FALSE;
        }
        if (vivid && COLORS > 8)
        {
            *bg += 8;
        }
    }
    else
    {
        *bg = THE_DEFAULT;
    }

    return TRUE;
}

/* Parse the color specification that starts at ptr, and then the one or more
 * regexes that follow it.  For each valid regex (or start=/end= regex pair),
 * add a rule to the current syntax. */
void
parse_rule(char *ptr, int rex_flags)
{
    char *names, *regexstring;
    short fg, bg;
    int   attributes;

    if (*ptr == '\0')
    {
        jot_error(N_("Missing color name"));
        return;
    }

    names = ptr;
    ptr   = parse_next_word(ptr);

    if (!parse_combination(names, &fg, &bg, &attributes))
    {
        return;
    }

    if (*ptr == '\0')
    {
        jot_error(N_("Missing regex string after '%s' command"), "color");
        return;
    }

    while (*ptr != '\0')
    {
        regex_t *start_rgx = NULL, *end_rgx = NULL;
        /* Intermediate storage for compiled regular expressions. */
        colortype *newcolor = NULL;
        /* Container for compiled regex (pair) and the color it paints. */
        bool expectend = FALSE;
        /* Whether it is a start=/end= regex pair. */

        if (strncmp(ptr, "start=", 6) == 0)
        {
            ptr += 6;
            expectend = TRUE;
        }

        regexstring = ++ptr;
        ptr         = parse_next_regex(ptr);

        /* When there is no regex, or it is invalid, skip this line. */
        if (ptr == NULL || !compile(regexstring, rex_flags, &start_rgx))
        {
            return;
        }

        if (expectend)
        {
            if (strncmp(ptr, "end=", 4) != 0)
            {
                jot_error(N_("\"start=\" requires a corresponding \"end=\""));
                regfree(start_rgx);
                free(start_rgx);
                return;
            }

            regexstring = ptr + 5;
            ptr         = parse_next_regex(ptr + 5);

            /* When there is no valid end= regex, abandon the rule. */
            if (ptr == NULL || !compile(regexstring, rex_flags, &end_rgx))
            {
                regfree(start_rgx);
                free(start_rgx);
                return;
            }
        }

        /* Allocate a rule, fill in the data, and link it into the list. */
        newcolor = RE_CAST(colortype *, nmalloc(sizeof(colortype)));

        newcolor->start = start_rgx;
        newcolor->end   = end_rgx;

        newcolor->fg         = fg;
        newcolor->bg         = bg;
        newcolor->attributes = attributes;

        if (lastcolor == NULL)
        {
            live_syntax->color = newcolor;
        }
        else
        {
            lastcolor->next = newcolor;
        }

        newcolor->next = NULL;
        lastcolor      = newcolor;

        /* For a multiline rule, give it a number and increase the count. */
        if (expectend)
        {
            newcolor->id = live_syntax->multiscore;
            live_syntax->multiscore++;
        }
    }
}

/* Set the colors for the given interface element to the given combination. */
void
set_interface_color(s32 element, s8 *combotext)
{
    colortype *trio = static_cast<colortype *>(nmalloc(sizeof(colortype)));

    if (parse_combination(combotext, &trio->fg, &trio->bg, &trio->attributes))
    {
        free(color_combo[element]);
        color_combo[element] = trio;
    }
    else
    {
        free(trio);
    }
}

/* Read regex strings enclosed in double quotes from the line pointed at
 * by ptr, and store them quoteless in the passed storage place. */
void
grab_and_store(const s8 *kind, s8 *ptr, regexlisttype **storage)
{
    regexlisttype *lastthing, *newthing;
    const s8      *regexstring;

    if (!opensyntax)
    {
        jot_error(N_("A '%s' command requires a preceding 'syntax' command"), kind);
        return;
    }

    /* The default syntax doesn't take any file matching stuff. */
    if (strcmp(live_syntax->name, "default") == 0 && *ptr != '\0')
    {
        jot_error(N_("The \"default\" syntax does not accept '%s' regexes"), kind);
        return;
    }

    if (*ptr == '\0')
    {
        jot_error(N_("Missing regex string after '%s' command"), kind);
        return;
    }

    lastthing = *storage;

    /* If there was an earlier command, go to the last of those regexes. */
    while (lastthing != nullptr && lastthing->next != nullptr)
    {
        lastthing = lastthing->next;
    }

    /* Now gather any valid regexes and add them to the linked list. */
    while (*ptr != '\0')
    {
        regex_t *packed_rgx = nullptr;

        regexstring = ++ptr;
        ptr         = parse_next_regex(ptr);

        if (ptr == nullptr)
        {
            return;
        }

        /* If the regex string is malformed, skip it. */
        if (!compile(regexstring, NANO_REG_EXTENDED | REG_NOSUB, &packed_rgx))
        {
            continue;
        }

        /* Copy the regex into a struct, and hook this in at the end. */
        newthing          = RE_CAST(regexlisttype *, nmalloc(sizeof(regexlisttype)));
        newthing->one_rgx = packed_rgx;
        newthing->next    = nullptr;

        if (lastthing == nullptr)
        {
            *storage = newthing;
        }
        else
        {
            lastthing->next = newthing;
        }

        lastthing = newthing;
    }
}

/* Gather and store the string after a comment/linter command. */
void
pick_up_name(const char *kind, char *ptr, char **storage)
{
    if (*ptr == '\0')
    {
        jot_error(N_("Missing argument after '%s'"), kind);
        return;
    }

    /* If the argument starts with a quote, find the terminating quote. */
    if (*ptr == '"')
    {
        char *look = ptr + strlen(ptr);

        while (*look != '"')
        {
            if (--look == ptr)
            {
                jot_error(N_("Argument of '%s' lacks closing \""), kind);
                return;
            }
        }

        *look = '\0';
        ptr++;
    }

    *storage = mallocstrcpy(*storage, ptr);
}

/* Handle the six syntax-only commands. */
bool
parse_syntax_commands(char *keyword, char *ptr)
{
    if (strcmp(keyword, "color") == 0)
    {
        parse_rule(ptr, NANO_REG_EXTENDED);
    }
    else if (strcmp(keyword, "icolor") == 0)
    {
        parse_rule(ptr, NANO_REG_EXTENDED | REG_ICASE);
    }
    else if (strcmp(keyword, "comment") == 0)
    {
#ifdef ENABLE_COMMENT
        pick_up_name("comment", ptr, &live_syntax->comment);
#endif
    }
    else if (strcmp(keyword, "tabgives") == 0)
    {
        pick_up_name("tabgives", ptr, &live_syntax->tabstring);
    }
    else if (strcmp(keyword, "linter") == 0)
    {
        pick_up_name("linter", ptr, &live_syntax->linter);
        strip_leading_blanks_from(live_syntax->linter);
    }
    else if (strcmp(keyword, "formatter") == 0)
    {
        pick_up_name("formatter", ptr, &live_syntax->formatter);
        strip_leading_blanks_from(live_syntax->formatter);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

//
/// Verify that the user has not unmapped every shortcut for a
/// function that we consider 'vital' (such as "Exit").
//
static void
check_vitals_mapped()
{
    static constexpr s8 VITALS   = 4;
    void (*vitals[VITALS])(void) = {do_exit, do_exit, do_exit, do_cancel};
    s32 inmenus[VITALS]          = {MMAIN, MBROWSER, MHELP, MYESNO};

    for (s32 v = 0; v < VITALS; v++)
    {
        for (funcstruct *f = allfuncs; f != nullptr; f = f->next)
        {
            if (f->func == vitals[v] && (f->menus & inmenus[v]))
            {
                if (first_sc_for(inmenus[v], f->func) == nullptr)
                {
                    jot_error(N_("No key is bound to function '%s' in menu '%s'. "
                                 " Exiting.\n"),
                              f->tag, menu_to_name(inmenus[v]));
                    die(_("If needed, use nano with the -I option "
                          "to adjust your nanorc settings.\n"));
                }
                else
                {
                    break;
                }
            }
        }
    }
}

//
/// @name
///  -  @c checkForColorOptions
///
/// @brief
///  -  Check for the color options in the rcfile.
///
/// @param keyword ( const std::string & )
///  -  The keyword to check for.
///
/// @returns ( s32 )
///  -  The color option.
//
static s32
checkForColorOptions(const std::string &keyword)
{
    static const std::unordered_map<std::string, const s32> colorOptionsMap = {
        {    "titlecolor",     TITLE_BAR},
        {   "numbercolor",   LINE_NUMBER},
        {   "stripecolor",  GUIDE_STRIPE},
        { "scrollercolor",    SCROLL_BAR},
        { "selectedcolor", SELECTED_TEXT},
        {"spotlightcolor",   SPOTLIGHTED},
        {     "minicolor",  MINI_INFOBAR},
        {   "promptcolor",    PROMPT_BAR},
        {   "statuscolor",    STATUS_BAR},
        {    "errorcolor", ERROR_MESSAGE},
        {      "keycolor",     KEY_COMBO},
        { "functioncolor",  FUNCTION_TAG}
    };
    const auto it = colorOptionsMap.find(keyword);
    return it != colorOptionsMap.end() ? it->second : INT32_MAX;
}

static u32
checkForConfigOptions(const std::string &keyWord)
{
    static const std::unordered_map<std::string, const u32> map = {
        { "operatingdir",     OPERATINGDIR},
        {         "fill",             FILL},
        {"matchbrackets",    MATCHBRACKETS},
        {   "whitespace",       WHITESPACE},
        {        "punct",            PUNCT},
        {     "brackets",         BRACKETS},
        {     "quotestr",         QUOTESTR},
        {      "speller",          SPELLER},
        {    "backupdir",        BACKUPDIR},
        {    "wordchars",        WORDCHARS},
        {  "guidestripe",      GUIDESTRIPE},
        {      "tabsize", CONF_OPT_TABSIZE}
    };
    const auto it = map.find(keyWord);
    return it != map.end() ? it->second : static_cast<u32>(0);
}

//
/// @name
///  -  @c parse_rcfile
///
/// @brief
///  -  Parse the rcfile, once it has been opened successfully at rcstream,
///     and close it afterwards.  If just_syntax is TRUE, allow the file to
///     to contain only color syntax commands.
///
/// @param rcstream ( FILE * )
///  -  The file stream to read from.
///
/// @param just_syntax ( bool )
///  -  Whether to parse only the syntax commands.
///
/// @param intros_only ( bool )
///  -  Whether to parse only the syntax prologue.
///
/// @returns ( void )
//
/// TODO : This function is too long and needs to be refactored.
///        It is also convoluted and hard to follow.
//
void
parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only)
{
    s8 *buffer = nullptr;
    u64 size   = 0;
    s64 length = 0;

    //
    /// TODO : This is the main loop for rcfile parsing. FIX IT
    //
    while ((length = getline(&buffer, &size, rcstream)) > 0)
    {
        s8 *ptr, *keyword, *option, *argument;

        bool drop_open = false;
        s32  set       = 0;
        u64  i;

        lineno++;

        /* If doing a full parse, skip to after the 'syntax' command. */
        if (just_syntax && !intros_only && lineno <= live_syntax->lineno)
        {
            continue;
        }

        /* Strip the terminating newline and possibly a carriage return. */
        if (buffer[length - 1] == '\n')
        {
            buffer[--length] = '\0';
        }
        if (length > 0 && buffer[length - 1] == '\r')
        {
            buffer[--length] = '\0';
        }

        ptr = buffer;
        while (isblank(static_cast<u8>(*ptr)))
        {
            ptr++;
        }

        /* If the line is empty or a comment, skip to next line. */
        if (*ptr == '\0' || *ptr == '#')
        {
            continue;
        }

        /* Otherwise, skip to the next space. */
        keyword = ptr;
        ptr     = parse_next_word(ptr);

        /* Handle extending first... */
        if (!just_syntax && strcmp(keyword, "extendsyntax") == 0)
        {
            augmentstruct *newitem, *extra;
            s8            *syntaxname = ptr;
            syntaxtype    *sntx;

            check_for_nonempty_syntax();

            ptr = parse_next_word(ptr);

            for (sntx = syntaxes; sntx != NULL; sntx = sntx->next)
            {
                if (!strcmp(sntx->name, syntaxname))
                {
                    break;
                }
            }

            if (sntx == nullptr)
            {
                jot_error(N_("Could not find syntax \"%s\" to extend"), syntaxname);
                continue;
            }

            keyword  = ptr;
            argument = copy_of(ptr);
            ptr      = parse_next_word(ptr);

            /* File-matching commands need to be processed immediately;
             * other commands are stored for possible later processing. */
            if (strcmp(keyword, "header") == 0 || strcmp(keyword, "magic") == 0)
            {
                free(argument);
                live_syntax = sntx;
                opensyntax  = TRUE;
                drop_open   = TRUE;
            }
            else
            {
                newitem = RE_CAST(augmentstruct *, nmalloc(sizeof(augmentstruct)));
                ;

                newitem->filename = copy_of(nanorc);
                newitem->lineno   = lineno;
                newitem->data     = argument;
                newitem->next     = NULL;

                if (sntx->augmentations != NULL)
                {
                    extra = sntx->augmentations;
                    while (extra->next != NULL)
                    {
                        extra = extra->next;
                    }
                    extra->next = newitem;
                }
                else
                {
                    sntx->augmentations = newitem;
                }

                continue;
            }
        }

        /* Try to parse the keyword. */
        if (strcmp(keyword, "syntax") == 0)
        {
            if (intros_only)
            {
                check_for_nonempty_syntax();
                begin_new_syntax(ptr);
            }
            else
            {
                break;
            }
        }
        else if (strcmp(keyword, "header") == 0)
        {
            if (intros_only)
            {
                grab_and_store("header", ptr, &live_syntax->headers);
            }
        }
        else if (strcmp(keyword, "magic") == 0)
        {
#ifdef HAVE_LIBMAGIC
            if (intros_only)
            {
                grab_and_store("magic", ptr, &live_syntax->magics);
            }
#endif
        }
        else if (just_syntax && (strcmp(keyword, "set") == 0 || strcmp(keyword, "unset") == 0 ||
                                 strcmp(keyword, "bind") == 0 || strcmp(keyword, "unbind") == 0 ||
                                 strcmp(keyword, "include") == 0 || strcmp(keyword, "extendsyntax") == 0))
        {
            if (intros_only)
            {
                jot_error(N_("Command \"%s\" not allowed in included file"), keyword);
            }
            else
            {
                break;
            }
        }
        else if (intros_only && (strcmp(keyword, "color") == 0 || strcmp(keyword, "icolor") == 0 ||
                                 strcmp(keyword, "comment") == 0 || strcmp(keyword, "tabgives") == 0 ||
                                 strcmp(keyword, "linter") == 0 || strcmp(keyword, "formatter") == 0))
        {
            if (!opensyntax)
            {
                jot_error(N_("A '%s' command requires a preceding "
                             "'syntax' command"),
                          keyword);
            }
            if (strstr("icolor", keyword))
            {
                seen_color_command = true;
            }
            continue;
        }
        else if (parse_syntax_commands(keyword, ptr))
        {
            ;
        }
        else if (strcmp(keyword, "include") == 0)
        {
            parse_includes(ptr);
        }
        else
        {
            if (strcmp(keyword, "set") == 0)
            {
                set = 1;
            }
            else if (strcmp(keyword, "unset") == 0)
            {
                set = -1;
            }
            else if (strcmp(keyword, "bind") == 0)
            {
                parse_binding(ptr, true);
            }
            else if (strcmp(keyword, "unbind") == 0)
            {
                parse_binding(ptr, false);
            }
            else if (intros_only)
            {
                jot_error(N_("Command \"%s\" not understood"), keyword);
            }
        }

        if (drop_open)
        {
            opensyntax = false;
        }

        if (set == 0)
        {
            continue;
        }

        check_for_nonempty_syntax();

        if (*ptr == '\0')
        {
            jot_error(N_("Missing option"));
            continue;
        }

        option = ptr;
        ptr    = parse_next_word(ptr);

        /* Find the just parsed option name among the existing names. */
        for (i = 0; rcopts[i].name != nullptr; i++)
        {
            if (strcmp(option, rcopts[i].name) == 0)
            {
                break;
            }
        }

        if (rcopts[i].name == nullptr)
        {
            jot_error(N_("Unknown option: %s"), option);
            continue;
        }

        /* If the option has a flag, set it or unset it, as requested. */
        if (rcopts[i].flag)
        {
            if (set == 1)
            {
                SET(rcopts[i].flag);
            }
            else
            {
                UNSET(rcopts[i].flag);
            }
            continue;
        }

        /* An option that takes an argument cannot be unset. */
        if (set == -1)
        {
            jot_error(N_("Cannot unset option \"%s\""), option);
            continue;
        }

        if (*ptr == '\0')
        {
            jot_error(N_("Option \"%s\" requires an argument"), option);
            continue;
        }

        argument = ptr;
        if (*argument == '"')
        {
            argument++;
        }
        ptr = parse_argument(ptr);

        // When in a UTF-8 locale, ignore arguments with invalid sequences.
        if (using_utf8() && mbstowcs(nullptr, argument, 0) == (u64)-1)
        {
            jot_error(N_("Argument is not a valid multibyte string"));
            continue;
        }

        //
        /// Check for color options.
        /// This uses a unordered map to check for
        /// color options and return the corresponding
        /// index as apposed to the previous implementation
        /// used by GNU for the original Nano that
        /// used a series of if else statements.
        /// This is a more efficient way to check for
        /// color options.
        ///
        /// @see checkForColorOptions
        //
        const s32 colorOption = checkForColorOptions(option);
        (colorOption == INT32_MAX) ? void() : set_interface_color(colorOption, argument);

        //
        /// Check for configuration options.
        /// This uses a unordered map and a
        /// enum with bit mask values like (1 << 0)
        /// for the first enum, (1 << 1) for the second and so on.
        /// This way we can check for configuration options using bitwise operations.
        /// This is a far more efficient way to check for configuration options
        /// then using strcmp for each option.
        ///
        /// @see checkForConfigOptions
        //
        const u32 configOption = checkForConfigOptions(option);
        if (configOption & OPERATINGDIR)
        {
            operating_dir = mallocstrcpy(operating_dir, argument);
        }
        else if (configOption & FILL)
        {
            if (!parseNum(argument, fill))
            {
                jot_error(N_("Requested fill size \"%s\" is invalid"), argument);
                fill = -COLUMNS_FROM_EOL;
            }
        }
        else if (configOption & MATCHBRACKETS)
        {
            if (has_blank_char(argument))
            {
                jot_error(N_("Non-blank characters required"));
            }
            else if (mbstrlen(argument) % 2 != 0)
            {
                jot_error(N_("Even number of characters required"));
            }
            else
            {
                matchbrackets = mallocstrcpy(matchbrackets, argument);
            }
        }
        else if (configOption & WHITESPACE)
        {
            if (mbstrlen(argument) != 2 || breadth(argument) != 2)
            {
                jot_error(N_("Two single-column characters required"));
            }
            else
            {
                whitespace  = mallocstrcpy(whitespace, argument);
                whitelen[0] = char_length(whitespace);
                whitelen[1] = char_length(whitespace + whitelen[0]);
            }
        }
        else if (configOption & PUNCT)
        {
            if (has_blank_char(argument))
            {
                jot_error(N_("Non-blank characters required"));
            }
            else
            {
                punct = mallocstrcpy(punct, argument);
            }
        }
        else if (configOption & BRACKETS)
        {
            if (has_blank_char(argument))
            {
                jot_error(N_("Non-blank characters required"));
            }
            else
            {
                brackets = mallocstrcpy(brackets, argument);
            }
        }
        else if (configOption & QUOTESTR)
        {
            quotestr = mallocstrcpy(quotestr, argument);
        }
        else if (configOption & SPELLER)
        {
            alt_speller = mallocstrcpy(alt_speller, argument);
        }
        else if (configOption & BACKUPDIR)
        {
            backup_dir = mallocstrcpy(backup_dir, argument);
        }
        else if (configOption & WORDCHARS)
        {
            word_chars = mallocstrcpy(word_chars, argument);
        }
        else if (configOption & GUIDESTRIPE)
        {
            if (!parseNum(argument, stripe_column) || stripe_column <= 0)
            {
                jot_error(N_("Guide column \"%s\" is invalid"), argument);
                stripe_column = 0;
            }
        }
        else if (configOption & CONF_OPT_TABSIZE)
        {
            if (!parseNum(argument, tabsize) || tabsize <= 0)
            {
                jot_error(N_("Requested tab size \"%s\" is invalid"), argument);
                tabsize = -1;
            }
        }
    }

    if (intros_only)
    {
        check_for_nonempty_syntax();
    }

    fclose(rcstream);
    free(buffer);
    lineno = 0;

    return;
}

// Read and interpret one of the two nanorc files.
void
parse_one_nanorc()
{
    FILE *rcstream = fopen(nanorc, "rb");

    // If opening the file succeeded, parse it.  Otherwise, only
    // complain if the file actually exists.
    if (rcstream != nullptr)
    {
        parse_rcfile(rcstream, FALSE, TRUE);
    }
    else if (errno != ENOENT)
    {
        jot_error(N_("Error reading %s: %s"), nanorc, strerror(errno));
    }
}

bool
have_nanorc(const s8 *path, const s8 *name)
{
    if (path == nullptr)
    {
        return false;
    }

    free(nanorc);
    nanorc = concatenate(path, name);

    return is_good_file(nanorc);
}

//
/// @name
///  - @c do_rcfiles
///
/// @brief
///  -  Process the nanorc file that was specified on the command line (if any),
///  -  and otherwise the system-wide rcfile followed by the user's rcfile.
///
/// @returns ( void )
//
void
do_rcfiles()
{
    if (custom_nanorc)
    {
        nanorc = get_full_path(custom_nanorc);
        if (nanorc == nullptr || access(nanorc, F_OK) != 0)
        {
            die(_("Specified rcfile does not exist\n"));
        }
    }
    else
    {
        nanorc = mallocstrcpy(nanorc, SYSCONFDIR "/nanorc");
    }

    if (is_good_file(nanorc))
    {
        parse_one_nanorc();
    }

    if (custom_nanorc == nullptr)
    {
        const s8 *xdgconfdir = getenv("XDG_CONFIG_HOME");

        get_homedir();

        /* Now try to find a nanorc file in the user's home directory or in the
         * XDG configuration directories, and process the first one found. */
        if (have_nanorc(homedir, "/" HOME_RC_NAME) || have_nanorc(xdgconfdir, "/nano/" RCFILE_NAME) ||
            have_nanorc(homedir, "/.config/nano/" RCFILE_NAME))
        {
            parse_one_nanorc();
        }
        else if (homedir == nullptr && xdgconfdir == nullptr)
        {
            jot_error(N_("I can't find my home directory!  Wah!"));
        }
    }

    check_vitals_mapped();
    free(nanorc);
    nanorc = nullptr;
}
