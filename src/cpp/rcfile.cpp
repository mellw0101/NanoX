/// @file rcfile.cpp
#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <glob.h>
#include <unistd.h>

using namespace Mlib;

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
//
//  The line number of the last encountered error.
//
static size_t lineno = 0;
//
//  The path to the rcfile we're parsing. */
//
static s8 *nanorc = nullptr;
//
//  Whether we're allowed to add to the last syntax.  When a file ends,
//  or when a new syntax command is seen, this bool becomes 'false'.
//
static bool opensyntax = FALSE;
//
//  The syntax that is currently being parsed.
//
static syntaxtype *live_syntax;
//
//  Whether a syntax definition contains any color commands.
//
static bool seen_color_command = FALSE;
//
//  The end of the color list for the current syntax.
//
static colortype *lastcolor = nullptr;
//
//  Beginning and end of a list of errors in rcfiles, if any.
//
static linestruct *errors_head = nullptr;
static linestruct *errors_tail = nullptr;

//
//  Send the gathered error messages (if any) to the terminal.
//
void
display_rcfile_errors()
{
    for (linestruct *error = errors_head; error != nullptr; error = error->next)
    {
        std::fprintf(stderr, "%s\n", error->data);
    }
}

static constexpr u16 MAXSIZE = (PATH_MAX + 200);
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
    PROFILE_FUNCTION;

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
            std::snprintf(textbuf, MAXSIZE, _("Mistakes in '%s'"), nanorc);
            startup_problem = copy_of(textbuf);
        }
        else
        {
            startup_problem = copy_of(_("Problems with history file"));
        }
    }
    if (lineno > 0)
    {
        length = std::snprintf(textbuf, MAXSIZE, _("Error in %s on line %zu: "), nanorc, lineno);
    }
    va_start(ap, msg);
    length += std::vsnprintf(textbuf + length, MAXSIZE - length, _(msg), ap);
    va_end(ap);

    error->data = static_cast<s8 *>(nmalloc(length + 1));
    std::sprintf(error->data, "%s", textbuf);
}

CONSTEXPR_MAP<std::string_view, functionptrtype, 104> keyMap = {
    {{"cancel", do_cancel},
     {"help", do_help},
     {"exit", do_exit},
     {"discardbuffer", discard_buffer},
     {"writeout", do_writeout},
     {"savefile", do_savefile},
     {"insert", do_insertfile},
     {"whereis", do_search_forward},
     {"wherewas", do_search_backward},
     {"findprevious", do_findprevious},
     {"findnext", do_findnext},
     {"replace", do_replace},
     {"cut", cut_text},
     {"copy", copy_text},
     {"paste", paste_text},
     {"execute", do_execute},
     {"cutrestoffile", cut_till_eof},
     {"zap", zap_text},
     {"mark", do_mark},
     {"tospell", do_spell},
     {"speller", do_spell},
     {"linter", do_linter},
     {"formatter", do_formatter},
     {"location", report_cursor_position},
     {"gotoline", do_gotolinecolumn},
     {"justify", do_justify},
     {"fulljustify", do_full_justify},
     {"beginpara", to_para_begin},
     {"endpara", to_para_end},
     {"comment", do_comment},
     {"complete", complete_a_word},
     {"indent", do_indent},
     {"unindent", do_unindent},
     {"chopwordleft", chop_previous_word},
     {"chopwordright", chop_next_word},
     {"findbracket", do_find_bracket},
     {"wordcount", count_lines_words_and_characters},
     {"recordmacro", record_macro},
     {"runmacro", run_macro},
     {"anchor", put_or_lift_anchor},
     {"prevanchor", to_prev_anchor},
     {"nextanchor", to_next_anchor},
     {"undo", do_undo},
     {"redo", do_redo},
     {"suspend", do_suspend},
     {"left", do_left},
     {"back", do_left},
     {"right", do_right},
     {"forward", do_right},
     {"up", do_up},
     {"prevline", do_up},
     {"down", do_down},
     {"nextline", do_down},
     {"scrollup", do_scroll_up},
     {"scrolldown", do_scroll_down},
     {"prevword", to_prev_word},
     {"nextword", to_next_word},
     {"home", do_home},
     {"end", do_end},
     {"prevblock", to_prev_block},
     {"nextblock", to_next_block},
     {"pageup", do_page_up},
     {"prevpage", do_page_up},
     {"pagedown", do_page_down},
     {"nextpage", do_page_down},
     {"firstline", to_first_line},
     {"lastline", to_last_line},
     {"toprow", to_top_row},
     {"bottomrow", to_bottom_row},
     {"center", do_center},
     {"cycle", do_cycle},
     {"dosformat", dos_format},
     {"macformat", mac_format},
     {"append", append_it},
     {"prepend", prepend_it},
     {"backup", back_it_up},
     {"flipexecute", flip_execute},
     {"flippipe", flip_pipe},
     {"flipconvert", flip_convert},
     {"flipnewbuffer", flip_newbuffer},
     {"flipgoto", flip_goto},
     {"flipreplace", flip_replace},
     {"flippipe", flip_pipe},
     {"flipconvert", flip_convert},
     {"flipnewbuffer", flip_newbuffer},
     {"verbatim", do_verbatim_input},
     {"tab", do_tab},
     {"enter", do_enter},
     {"delete", do_delete},
     {"backspace", do_backspace},
     {"refresh", full_refresh},
     {"casesens", case_sens_void},
     {"regexp", regexp_void},
     {"backwards", backwards_void},
     {"flipreplace", flip_replace},
     {"flipgoto", flip_goto},
     {"flippipe", flip_pipe},
     {"flipconvert", flip_convert},
     {"flipnewbuffer", flip_newbuffer},
     {"tofiles", to_files},
     {"browser", to_files},
     {"gotodir", goto_dir},
     {"firstfile", to_first_file},
     {"lastfile", to_last_file}}
};
constexpr auto
retriveScFromStr(std::string_view str)
{
    for (const auto &[key, value] : keyMap)
    {
        if (key == str)
        {
            return value;
        }
    }
    return (functionptrtype) nullptr;
}

//
//  Interpret a function string given in the rc file, and return a
//  shortcut record with the corresponding function filled in.
//
//  TODO : Use a map instead of strcmp
//
keystruct *
strtosc(const s8 *input)
{
    PROFILE_FUNCTION;

    keystruct *s = static_cast<keystruct *>(nmalloc(sizeof(keystruct)));

    s->toggle = 0;

    const auto it = retriveScFromStr(input);
    if (it != nullptr)
    {
        s->func = it;
    }
    else
    {
        s->func           = do_toggle;
        const auto toggle = retriveToggleOptionFromStr(input);
        if (toggle)
        {
            s->toggle = toggle;
        }
        else
        {
            std::free(s);
            return nullptr;
        }
    }
    return s;
}

//
//  Parse the next word from the string, null-terminate it, and return
//  a pointer to the first character after the null terminator.  The
//  returned pointer will point to '\0' if we hit the end of the line.
//
s8 *
parse_next_word(s8 *ptr)
{
    while (!std::isblank((unsigned char)*ptr) && *ptr != '\0')
    {
        ptr++;
    }

    if (*ptr == '\0')
    {
        return ptr;
    }

    //
    //  Null-terminate and advance ptr.
    //
    *ptr++ = '\0';

    while (std::isblank((unsigned char)*ptr))
    {
        ptr++;
    }

    return ptr;
}

//
//  Parse an argument, with optional quotes, after a keyword that takes
//  one.  If the next word starts with a ", we say that it ends with the
//  last " of the line.  Otherwise, we interpret it as usual, so that the
//  arguments can contain "'s too.
//
s8 *
parse_argument(s8 *ptr)
{
    PROFILE_FUNCTION;

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

    while (std::isblank(static_cast<u8>(*ptr)))
    {
        ptr++;
    }

    return ptr;
}

//
//  Advance over one regular expression in the line starting at ptr,
//  null-terminate it, and return a pointer to the succeeding text.
//
s8 *
parse_next_regex(s8 *ptr)
{
    PROFILE_FUNCTION;

    s8 *starting_point = ptr;

    if (*(ptr - 1) != '"')
    {
        jot_error(N_("Regex strings must begin and end with a \" character"));
        return nullptr;
    }

    //
    //  Continue until the end of the line, or until a double quote followed
    //  by end-of-line or a blank.
    //
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

//
//  Compile the given regular expression and store the result in packed (when
//  this pointer is not NULL).  Return TRUE when the expression is valid.
//
bool
compile(const s8 *expression, s32 rex_flags, regex_t *&packed)
{
    PROFILE_FUNCTION;

    regex_t *compiled = static_cast<regex_t *>(nmalloc(sizeof(regex_t)));
    s32      outcome  = regcomp(compiled, expression, rex_flags);

    if (outcome != 0)
    {
        u64 length  = regerror(outcome, compiled, nullptr, 0);
        s8 *message = static_cast<s8 *>(nmalloc(length));

        regerror(outcome, compiled, message, length);
        jot_error(N_("Bad regex \"%s\": %s"), expression, message);
        std::free(message);

        regfree(compiled);
        std::free(compiled);
    }
    else
    {
        packed = compiled;
    }

    return (outcome == 0);
}

//
//  Parse the next syntax name and its possible extension regexes from the
//  line at ptr, and add it to the global linked list of color syntaxes.
//
void
begin_new_syntax(s8 *ptr)
{
    PROFILE_FUNCTION;

    s8 *nameptr = ptr;

    //
    //  Check that the syntax name is not empty.
    //
    if (*ptr == '\0' || (*ptr == '"' && (*(ptr + 1) == '\0' || *(ptr + 1) == '"')))
    {
        jot_error(N_("Missing syntax name"));
        return;
    }

    ptr = parse_next_word(ptr);

    //
    //  Check that quotes around the name are either paired or absent.
    //
    if ((*nameptr == '\x22') ^ (nameptr[std::strlen(nameptr) - 1] == '\x22'))
    {
        jot_error(N_("Unpaired quote in syntax name"));
        return;
    }

    //
    //  If the name is quoted, strip the quotes.
    //
    if (*nameptr == '\x22')
    {
        nameptr++;
        nameptr[strlen(nameptr) - 1] = '\0';
    }

    //
    //  Redefining the "none" syntax is not allowed.
    //
    if (strcmp(nameptr, "none") == 0)
    {
        jot_error(N_("The \"none\" syntax is reserved"));
        return;
    }

    //
    //  Initialize a new syntax struct.
    //
    live_syntax                = RE_CAST(syntaxtype *, nmalloc(sizeof(syntaxtype)));
    live_syntax->name          = copy_of(nameptr);
    live_syntax->filename      = copy_of(nanorc);
    live_syntax->lineno        = lineno;
    live_syntax->augmentations = nullptr;
    live_syntax->extensions    = nullptr;
    live_syntax->headers       = nullptr;
    live_syntax->magics        = nullptr;
    live_syntax->linter        = nullptr;
    live_syntax->formatter     = nullptr;
    live_syntax->tabstring     = nullptr;
    live_syntax->comment       = copy_of(GENERAL_COMMENT_CHARACTER);
    live_syntax->color         = nullptr;
    live_syntax->multiscore    = 0;
    //
    //  Hook the new syntax in at the top of the list.
    //
    live_syntax->next = syntaxes;
    syntaxes          = live_syntax;

    opensyntax         = true;
    seen_color_command = false;

    //
    //  The default syntax should have no associated extensions.
    //
    if (std::strcmp(live_syntax->name, "default") == 0 && *ptr != '\0')
    {
        jot_error(N_("The \"default\" syntax does not accept extensions"));
        return;
    }

    //
    //  If there seem to be extension regexes, pick them up.
    //
    if (*ptr != '\0')
    {
        grab_and_store("extension", ptr, &live_syntax->extensions);
    }
}

//
//  Verify that a syntax definition contains at least one color command.
//
void
check_for_nonempty_syntax()
{
    if (opensyntax && !seen_color_command)
    {
        u64 current_lineno = lineno;

        lineno = live_syntax->lineno;
        jot_error(N_("Syntax \"%s\" has no color commands"), live_syntax->name);
        lineno = current_lineno;
    }
    opensyntax = false;
}

//
//  Return TRUE when the given function is present in almost all menus.
//
bool
is_universal(void (*func)())
{
    PROFILE_FUNCTION;

    return (func == do_left || func == do_right || func == do_home || func == do_end || func == to_prev_word ||
            func == to_next_word || func == do_delete || func == do_backspace || func == cut_text ||
            func == paste_text || func == do_tab || func == do_enter || func == do_verbatim_input);
}

/* Bind or unbind a key combo, to or from a function. */
void
parse_binding(char *ptr, bool dobind)
{
    PROFILE_FUNCTION;

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

    menu = nameToMenu(menuptr);
    if (!menu)
    {
        jot_error(N_("Unknown menu: %s"), menuptr);
        goto free_things;
    }

    if (dobind)
    {
        //
        //  If the thing to bind starts with a double quote, it is a string,
        //  otherwise it is the name of a function.
        //
        if (*funcptr == '"')
        {
            newsc            = static_cast<keystruct *>(nmalloc(sizeof(keystruct)));
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

    //
    //  Limit the given menu to those where the function exists;
    //  first handle five special cases, then the general case.
    //
    if (is_universal(newsc->func))
    {
        menu &= MMOST | MBROWSER;
    }
    else if (newsc->func == do_toggle && newsc->toggle == NO_HELP)
    {
        menu &= (MMOST | MBROWSER | MYESNO) & ~MFINDINHELP;
    }
    else if (newsc->func == do_toggle)
    {
        menu &= MMAIN;
    }
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
        //
        //  Tally up the menus where the function exists.
        //
        for (funcstruct *f = allfuncs; f != nullptr; f = f->next)
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
    PROFILE_FUNCTION;

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
    PROFILE_FUNCTION;

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

//
//  Expand globs in the passed name, and parse the resultant files.
//
void
parse_includes(s8 *ptr)
{
    PROFILE_FUNCTION;

    s8 *pattern;
    s8 *expanded;

    glob_t files;
    s32    result;

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

    //
    //  Expand a tilde first,
    //  then try to match the globbing pattern.
    //
    expanded = real_dir_from_tilde(pattern);
    result   = glob(expanded, GLOB_ERR | GLOB_NOCHECK, nullptr, &files);

    //
    //  If there are matches,
    //  process each of them.
    //  Otherwise,
    //  only report an error if it's something other than zero matches.
    //
    if (result == 0)
    {
        for (u64 i = 0; i < files.gl_pathc; ++i)
        {
            parse_one_include(files.gl_pathv[i], nullptr);
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
    PROFILE_FUNCTION;

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

static constexpr u8 COLORCOUNT = 34;

CONSTEXPR_MAP<STRING_VIEW, s16, COLORCOUNT> huesIndiecesMap = {
    {{"red", COLOR_RED},
     {"green", COLOR_GREEN},
     {"blue", COLOR_BLUE},
     {"yellow", COLOR_YELLOW},
     {"cyan", COLOR_CYAN},
     {"magenta", COLOR_MAGENTA},
     {"white", COLOR_WHITE},
     {"black", COLOR_BLACK},
     {"normal", THE_DEFAULT},
     {"pink", 204},
     {"purple", 163},
     {"mauve", 134},
     {"lagoon", 38},
     {"mint", 48},
     {"lime", 148},
     {"peach", 215},
     {"orange", 208},
     {"latte", 137},
     {"rosy", 175},
     {"beet", 127},
     {"plum", 98},
     {"sea", 32},
     {"sky", 111},
     {"slate", 66},
     {"teal", 35},
     {"sage", 107},
     {"brown", 100},
     {"ocher", 142},
     {"sand", 186},
     {"tawny", 136},
     {"brick", 166},
     {"crimson", 161},
     {"grey", COLOR_BLACK + 8},
     {"gray", COLOR_BLACK + 8}}
};

//
//  Return the short value corresponding to the given color name, and set
//  vivid to TRUE for a lighter color, and thick for a heavier typeface.
//
//  TODO : Use references instead of pointers
//
short
color_to_short(const s8 *colorname, bool &vivid, bool &thick)
{
    if (Mlib::Constexpr::strncmp(colorname, "bright", 6) == 0 && colorname[6] != '\0')
    {
        vivid = true;
        thick = true;
        colorname += 6;
    }
    else if (Mlib::Constexpr::strncmp(colorname, "light", 5) == 0 && colorname[5] != '\0')
    {
        vivid = true;
        thick = false;
        colorname += 5;
    }
    else
    {
        vivid = false;
        thick = false;
    }

    if (colorname[0] == '#' && Mlib::Constexpr::strlen(colorname) == 4)
    {
        u16 r, g, b;

        if (vivid)
        {
            jot_error(N_("Color '%s' takes no prefix"), colorname);
            return BAD_COLOR;
        }

        if (std::sscanf(colorname, "#%1hX%1hX%1hX", &r, &g, &b) == 3)
        {
            return closest_index_color(r, g, b);
        }
    }

    for (s32 index = 0; index < COLORCOUNT; index++)
    {
        if (Mlib::Constexpr::strcmp(colorname, &huesIndiecesMap[index].key[0]))
        {
            if (index > 7 && vivid)
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
                return huesIndiecesMap[index].value;
            }
        }
    }

    jot_error(N_("Color \"%s\" not understood"), colorname);
    return BAD_COLOR;
}

//
//  Parse the color name (or pair of color names) in the given string.
//  Return FALSE when any color name is invalid; otherwise return TRUE.
//
//  TODO : Use references instead of pointers
//
bool
parse_combination(s8 *combotext, s16 &fg, s16 &bg, s32 &attributes)
{
    PROFILE_FUNCTION;

    bool vivid, thick;
    s8  *comma;

    attributes = A_NORMAL;

    if (std::strncmp(combotext, "bold", 4) == 0)
    {
        attributes |= A_BOLD;
        if (combotext[4] != ',')
        {
            jot_error(N_("An attribute requires a subsequent comma"));
            return FALSE;
        }
        combotext += 5;
    }

    if (std::strncmp(combotext, "italic", 6) == 0)
    {
#ifdef A_ITALIC
        attributes |= A_ITALIC;
#endif
        if (combotext[6] != ',')
        {
            jot_error(N_("An attribute requires a subsequent comma"));
            return FALSE;
        }
        combotext += 7;
    }

    comma = std::strchr(combotext, ',');

    if (comma)
    {
        *comma = '\0';
    }

    if (!comma || comma > combotext)
    {
        fg = color_to_short(combotext, vivid, thick);
        if (fg == BAD_COLOR)
        {
            return false;
        }
        if (vivid && !thick && COLORS > 8)
        {
            fg += 8;
        }
        else if (vivid)
        {
            attributes |= A_BOLD;
        }
    }
    else
    {
        fg = THE_DEFAULT;
    }

    if (comma)
    {
        bg = color_to_short(comma + 1, vivid, thick);
        if (bg == BAD_COLOR)
        {
            return false;
        }
        if (vivid && COLORS > 8)
        {
            bg += 8;
        }
    }
    else
    {
        bg = THE_DEFAULT;
    }

    return true;
}

//
//  Parse the color specification that starts at ptr, and then the one or more
//  regexes that follow it.  For each valid regex (or start=/end= regex pair),
//  add a rule to the current syntax.
//
void
parse_rule(char *ptr, int rex_flags)
{
    PROFILE_FUNCTION;

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

    if (!parse_combination(names, fg, bg, attributes))
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
        if (ptr == NULL || !compile(regexstring, rex_flags, start_rgx))
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
            if (ptr == NULL || !compile(regexstring, rex_flags, end_rgx))
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

    if (parse_combination(combotext, trio->fg, trio->bg, trio->attributes))
    {
        free(color_combo[element]);
        color_combo[element] = trio;
    }
    else
    {
        free(trio);
    }
}

//
//  Read regex strings enclosed in double quotes from the line pointed at
//  by ptr, and store them quoteless in the passed storage place.
//
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

    //
    //  The default syntax doesn't take any file matching stuff.
    //
    if (std::strcmp(live_syntax->name, "default") == 0 && *ptr != '\0')
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

    //
    //  If there was an earlier command, go to the last of those regexes.
    //
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
        if (!compile(regexstring, NANO_REG_EXTENDED | REG_NOSUB, packed_rgx))
        {
            continue;
        }

        //
        //  Copy the regex into a struct, and hook this in at the end.
        //
        newthing          = static_cast<regexlisttype *>(nmalloc(sizeof(regexlisttype)));
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

//
//  Gather and store the string after a comment/linter command.
//
void
pick_up_name(const s8 *kind, s8 *ptr, s8 **storage)
{
    PROFILE_FUNCTION;

    if (*ptr == '\0')
    {
        jot_error(N_("Missing argument after '%s'"), kind);
        return;
    }
    //
    //  If the argument starts with a quote, find the terminating quote.
    //
    if (*ptr == '"')
    {
        s8 *look = ptr + std::strlen(ptr);

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

//
//  Parse the syntax command in the given string,
//  and set the syntax options accordingly.
//
bool
parse_syntax_commands(s8 *keyword, s8 *ptr)
{
    PROFILE_FUNCTION;

    const u32 syntax_opt = retriveSyntaxOptionFromStr(keyword);
    if (syntax_opt == false)
    {
        return false;
    }
    if (syntax_opt & SYNTAX_OPT_COLOR)
    {
        parse_rule(ptr, NANO_REG_EXTENDED);
    }
    else if (syntax_opt & SYNTAX_OPT_ICOLOR)
    {
        parse_rule(ptr, NANO_REG_EXTENDED | REG_ICASE);
    }
    else if (syntax_opt & SYNTAX_OPT_COMMENT)
    {
        pick_up_name("comment", ptr, &live_syntax->comment);
    }
    else if (syntax_opt & SYNTAX_OPT_TABGIVES)
    {
        pick_up_name("tabgives", ptr, &live_syntax->tabstring);
    }
    else if (syntax_opt & SYNTAX_OPT_LINTER)
    {
        pick_up_name("linter", ptr, &live_syntax->linter);
        strip_leading_blanks_from(live_syntax->linter);
    }
    else if (syntax_opt & SYNTAX_OPT_FORMATTER)
    {
        pick_up_name("formatter", ptr, &live_syntax->formatter);
        strip_leading_blanks_from(live_syntax->formatter);
    }

    return true;
}

static constexpr s8 VITALS = 4;
//
//  Verify that the user has not unmapped every shortcut for a
//  function that we consider 'vital' (such as 'do_exit').
//
static void
check_vitals_mapped()
{
    PROFILE_FUNCTION;

    void (*vitals[VITALS])() = {do_exit, do_exit, do_exit, do_cancel};
    s32 inmenus[VITALS]      = {MMAIN, MBROWSER, MHELP, MYESNO};

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
                              f->tag, menuToName(inmenus[v]));
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
    PROFILE_FUNCTION;

    s8 *buffer = nullptr;
    u64 size   = 0;
    s64 length = 0;

    //
    // TODO : This is the main loop for rcfile parsing. FIX IT
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
        if (!just_syntax && std::strcmp(keyword, "extendsyntax") == 0)
        {
            augmentstruct *newitem, *extra;
            s8            *syntaxname = ptr;
            syntaxtype    *sntx;

            check_for_nonempty_syntax();

            ptr = parse_next_word(ptr);

            for (sntx = syntaxes; sntx != NULL; sntx = sntx->next)
            {
                if (strcmp(sntx->name, syntaxname) == 0)
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

            //
            //  File-matching commands need to be processed immediately;
            //  other commands are stored for possible later processing.
            //
            if (strcmp(keyword, "header") == 0 || strcmp(keyword, "magic") == 0)
            {
                free(argument);
                live_syntax = sntx;
                opensyntax  = TRUE;
                drop_open   = TRUE;
            }
            else
            {
                newitem = static_cast<augmentstruct *>(nmalloc(sizeof(augmentstruct)));

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

        //
        //  Find the just parsed option name among the existing names.
        //
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

        //
        //  When in a UTF-8 locale, ignore arguments with invalid sequences.
        //
        if (using_utf8() && mbstowcs(nullptr, argument, 0) == (u64)-1)
        {
            jot_error(N_("Argument is not a valid multibyte string"));
            continue;
        }

        const s32 colorOption = retriveColorOptionFromStr(option);
        (colorOption != u32_MAX) ? set_interface_color(colorOption, argument) : void();

        const u32 configOption = retriveConfigOptionFromStr(option);
        if (!configOption)
        {
            ;
        }
        else if (configOption & OPERATINGDIR)
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
            if (parseNum(argument, tabsize) == 0 || tabsize <= 0)
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

//
//  Read and interpret one of the two nanorc files.
//
void
parse_one_nanorc()
{
    FILE *rcstream = fopen(nanorc, "rb");

    //
    //  If opening the file succeeded, parse it.
    //  Otherwise, only complain if the file actually exists.
    //
    if (rcstream != nullptr)
    {
        parse_rcfile(rcstream, false, true);
    }
    else if (errno != ENOENT)
    {
        jot_error(N_("Error reading %s: %s"), nanorc, ERRNO_C_STR);
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
