#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <Mlib/def.h>

/* Returns 'true' if 'c' is a cpp syntax char. */
bool
isCppSyntaxChar(const char c)
{
    return (c == '<' || c == '>' || c == '&' || c == '*' || c == '=' || c == '+' || c == '-' || c == '/' || c == '%' ||
            c == '!' || c == '^' || c == '|' || c == '~' || c == '{' || c == '}' || c == '[' || c == ']' || c == '(' ||
            c == ')' || c == ';' || c == ':' || c == ',' || c == '.' || c == '?' || c == '#');
}

/* Get indent in number of 'tabs', 'spaces', 'total chars', 'total tabs (based on width of tab)'. */
void
get_line_indent(linestruct *line, unsigned short *tabs, unsigned short *spaces, unsigned short *t_char,
                unsigned short *t_tabs)
{
    unsigned long i;
    *tabs = 0, *spaces = 0, *t_char = 0, *t_tabs = 0;
    if (line->data[0] != ' ' && line->data[0] != '\t')
    {
        return;
    }
    for (i = 0; line->data[i]; i++)
    {
        if (line->data[i] == ' ')
        {
            (*spaces)++;
        }
        else if (line->data[i] == '\t')
        {
            (*tabs)++;
        }
        else
        {
            break;
        }
    }
    *t_char = *tabs + *spaces;
    *t_tabs = *tabs;
    if (*spaces > 0)
    {
        *t_tabs += *spaces / WIDTH_OF_TAB;
    }
}

/* Return the len of indent in terms of index off first non 'tab/space' char. */
unsigned short
indent_char_len(linestruct *line)
{
    unsigned short i;
    for (i = 0; line->data[i]; i++)
    {
        if (line->data[i] != ' ' && line->data[i] != '\t')
        {
            break;
        }
    }
    return i;
}

/* Inject a string into a line at an index. */
void
inject_in_line(linestruct **line, const char *str, unsigned long at)
{
    unsigned long len   = strlen((*line)->data);
    unsigned long s_len = strlen(str);
    if (at > len)
    {
        return;
    }
    (*line)->data = (char *)nrealloc((*line)->data, len + s_len + 1);
    memmove((*line)->data + at + s_len, (*line)->data + at, len - at + 1);
    memmove((*line)->data + at, str, s_len);
}

/* If an area is marked then plase the 'str' at mark and current_x, thereby enclosing the marked area.
 * TODO: (enclose_marked_region) - Make the undo one action insted of two seprate once. */
void
enclose_marked_region(const char *s1, const char *s2)
{
    linestruct   *was_current = openfile->current;
    linestruct   *top, *bot;
    unsigned long top_x, bot_x;
    /* Sanity check, Returns is there is no mark */
    if (openfile->mark == nullptr)
    {
        return;
    }
    get_region(&top, &top_x, &bot, &bot_x);
    openfile->current = top;
    add_undo(REPLACE, nullptr);
    inject_in_line(&top, s1, top_x);
    /* If top and bot is equal, move mark to the right by 's1' len. */
    (top == bot) ? (bot_x += strlen(s1)) : 0;
    openfile->current = bot;
    add_undo(REPLACE, nullptr);
    inject_in_line(&bot, s2, bot_x);
    update_undo(REPLACE);
    openfile->mark_x++;
    /* If top and bot is the same, move cursor one step right. */
    (top == bot) ? do_right() : void();
    openfile->current = was_current;
    set_modified();
}

/* This is a shortcut to make marked area a block comment. */
void
do_block_comment(void)
{
    PROFILE_FUNCTION;
    enclose_marked_region("/* ", " */");
    refresh_needed = true;
}

/* Check if enter is requested when betweeen '{' and '}'.
 * if so properly open the brackets, place cursor in the middle and indent once.
 * Return`s 'false' if not between them, otherwise return`s 'true' */
bool
enter_with_bracket(void)
{
    char          c, c_prev;
    linestruct   *was_current = openfile->current, *middle, *end;
    bool          allblanks   = false;
    unsigned long extra;
    if (!openfile->current->data[openfile->current_x - 1])
    {
        return false;
    }
    c_prev = openfile->current->data[openfile->current_x - 1];
    c      = openfile->current->data[openfile->current_x];
    if (c_prev != '{' || c != '}')
    {
        return false;
    }
    extra = indent_length(was_current->data);
    if (extra == openfile->current_x)
    {
        allblanks = (indent_length(openfile->current->data) == extra);
    }
    middle       = make_new_node(openfile->current);
    middle->data = (char *)nmalloc(strlen(openfile->current->data + openfile->current_x) + extra + 1);
    strcpy(&middle->data[extra], openfile->current->data + openfile->current_x);
    if (openfile->mark == openfile->current && openfile->mark_x > openfile->current_x)
    {
        openfile->mark = middle;
        openfile->mark_x += extra - openfile->current_x;
    }
    strncpy(middle->data, was_current->data, extra);
    if (allblanks)
    {
        openfile->current_x = 0;
    }
    openfile->current->data[openfile->current_x] = '\0';
    add_undo(ENTER, nullptr);
    splice_node(openfile->current, middle);
    renumber_from(middle);
    openfile->current     = middle;
    openfile->current_x   = extra;
    openfile->placewewant = xplustabs();
    openfile->totsize++;
    set_modified();
    if (ISSET(AUTOINDENT) && !allblanks)
    {
        openfile->totsize += extra;
    }
    update_undo(ENTER);
    /* End of 'middle' and start of 'end' */
    end       = make_new_node(openfile->current);
    end->data = (char *)nmalloc(strlen(openfile->current->data + openfile->current_x) + extra + 1);
    strcpy(&end->data[extra], openfile->current->data + openfile->current_x);
    strncpy(end->data, was_current->data, extra);
    openfile->current->data[openfile->current_x] = '\0';
    add_undo(ENTER, nullptr);
    splice_node(openfile->current, end);
    renumber_from(end);
    openfile->current     = end;
    openfile->current_x   = extra;
    openfile->placewewant = xplustabs();
    openfile->totsize++;
    if (ISSET(AUTOINDENT) && !allblanks)
    {
        openfile->totsize += extra;
    }
    update_undo(ENTER);
    /* Place cursor at correct pos. */
    do_up();
    do_tab();
    refresh_needed = true;
    focusing       = false;
    return true;
}

bool
is_empty_line(linestruct *line)
{
    unsigned i = 0;
    for (; line->data[i]; i++)
        ;
    return (i == 0);
}

/* Add a word and the color of that word to the syntax list. */
void
add_syntax_word(const char *color, const char *word)
{
    PROFILE_FUNCTION;
    syntaxtype *s;
    colortype  *c;
    /* Find 'c' syntax. */
    for (s = syntaxes; s != nullptr && strcmp(s->name, "c"); s = s->next)
        ;
    /* Return if syntax 'c' was not found */
    if (s == nullptr)
    {
        return;
    }
    /* Find end of colortype list in syntax 'c' */
    for (c = s->color; c->next != nullptr; c = c->next)
        ;
    /* Return if 'c' == nullptr */
    if (c == nullptr)
    {
        return;
    }
    /* Add, then update the syntax. */
    add_syntax_color(color, word, c);
    set_syntax_colorpairs(s);
}

/* Add some basic cpp syntax. */
void
do_cpp_syntax(void)
{
    syntaxtype *syntax;
    for (syntax = syntaxes; syntax != nullptr; syntax = syntax->next)
    {
        if (strcmp(syntax->name, "c") == 0 && syntax->filename == nullptr)
        {
            break;
        }
    }
    if (syntax != nullptr)
    {
        netlog_syntaxtype(syntax);
        colortype *c;
        for (c = syntax->color; c->next != nullptr; c = c->next)
            ;
        if (c != nullptr)
        {
            add_syntax_color("gray", ";", c);
            c = c->next;
            add_syntax_color("brightred", "\\<[A-Z_][0-9A-Z_]*\\>", c);
            c = c->next;
            add_syntax_color("blue", "\\<(nullptr)\\>", c);
            c = c->next;
            add_syntax_color("brightmagenta", "^[[:blank:]]*[A-Z_a-z][0-9A-Z_a-z]*:[[:blank:]]*$", c);
            c = c->next;
            add_syntax_color("normal", ":[[:blank:]]*$", c);
            c = c->next;
            /* This makes word after green while typing. */
            add_syntax_color("brightgreen", "(namespace|enum|struct|class)[[:blank:]]+[A-Za-z_][A-Za-z_0-9]*", c);
            c = c->next;
            /* Types and related keywords. */
            add_syntax_color("brightblue",
                             "\\<(auto|bool|char|const|double|enum|extern|float|inline|int|long|restrict|short|signed|"
                             "sizeof|static|struct|typedef|union|unsigned|void)\\>",
                             c);
            c = c->next;
            add_syntax_color("brightgreen", "\\<([[:lower:]][[:lower:]_]*|(u_?)?int(8|16|32|64))_t\\>", c);
            c = c->next;
            add_syntax_color(
                "green",
                "\\<(_(Alignas|Alignof|Atomic|Bool|Complex|Generic|Imaginary|Noreturn|Static_assert|Thread_local))\\>",
                c);
            c = c->next;
            add_syntax_color("brightblue",
                             "\\<(class|explicit|friend|mutable|namespace|override|private|protected|public|register|"
                             "template|this|typename|virtual|volatile|false|true)\\>",
                             c);
            c = c->next;
            add_syntax_color("brightgreen", "\\<(std|string|vector)\\>", c);
            c = c->next;
            /* Flow control. */
            add_syntax_color("brightyellow", "\\<(if|else|for|while|do|switch|case|default)\\>", c);
            c = c->next;
            add_syntax_color("brightyellow", "\\<(try|throw|catch|operator|new|delete)\\>", c);
            c = c->next;
            add_syntax_color("brightmagenta", "\\<(using|break|continue|goto|return)\\>", c);
            c = c->next;
            add_syntax_color("brightmagenta", "'([^'\\]|\\([\"'\abfnrtv]|x[[:xdigit:]]{1,2}|[0-3]?[0-7]{1,2}))'", c);
            c = c->next;
            add_syntax_color("cyan",
                             "__attribute__[[:blank:]]*\\(\\([^)]*\\)\\)|__(aligned|asm|builtin|hidden|inline|packed|"
                             "restrict|section|typeof|weak)__",
                             c);
            c = c->next;
            // add_syntax_color("brightyellow", "\"([^\"]|\\\")*\"|#[[:blank:]]*include[[:blank:]]*<[^>]+>", c);
            // c = c->next;
            add_syntax_color("green", "//[^\"]*$|(^|[[:blank:]])//.*", c);
            c = c->next;
            // add_syntax_color("brightwhite,yellow", "\\<(FIXME|TODO|XXX)\\>", c);
            // c = c->next;
            // add_syntax_color(",green", "[[:space:]]+$", c);
            set_syntax_colorpairs(syntax);
        }
    }
}

/* Check a line for syntax words, also index files that are included and add functions as well. */
void
check_for_syntax_words(linestruct *line)
{
    PROFILE_FUNCTION;
    unsigned i;
    char   **words;
    if (is_empty_line(line))
    {
        return;
    }
    words = words_in_line(line);
    if (*words == nullptr)
    {
        free(words);
        return;
    }
    for (i = 0; words[i] != nullptr; i++)
    {
        if (words[i + 1] == nullptr)
        {
            break;
        }
        const unsigned short type = retrieve_c_syntax_type(words[i]);
        if (!type)
        {
            continue;
        }
        else if (type & CS_STRUCT)
        {
            add_syntax_word("brightgreen", rgx_word(words[++i]));
        }
        else if (type & CS_ENUM)
        {
            add_syntax_word("brightgreen", rgx_word(words[++i]));
        }
        else if (type & CS_INT)
        {
            if (check_func_syntax(&words, &i))
            {
                continue;
            }
            add_syntax(&type, words[i]);
        }
        else if (type & CS_VOID)
        {
            if (check_func_syntax(&words, &i))
            {
                continue;
            }
            add_syntax(&type, words[i]);
        }
        else if (type & CS_LONG)
        {
            if (check_func_syntax(&words, &i))
            {
                continue;
            }
            add_syntax(&type, words[i]);
        }
        else if (type & CS_CHAR)
        {
            if (check_func_syntax(&words, &i))
            {
                continue;
            }
            add_syntax(&type, words[i]);
        }
        else if (type & CS_INCLUDE)
        {
            if (*(words[++i]) == '<')
            {
                static char buf[PATH_MAX];
                sprintf(buf, "%s%s", "/usr/include/", extract_include(words[i]));
                if (does_include_file_exist(buf))
                {
                    check_syntax(buf);
                    continue;
                }
                words[i] += 1;
                if (*(words[i]) == 'c')
                {
                    words[i] += 1;
                }
                memset(buf, '\0', sizeof(buf));
                sprintf(buf, "%s%s%s", "/usr/include/c++/v1/", words[i], ".h");
                if (does_include_file_exist(buf))
                {
                    check_syntax(buf);
                    continue;
                }
            }
        }
        else if (type & CS_CLASS)
        {
            add_syntax_word("brightgreen", words[++i]);
        }
        else if (type & CS_BOOL)
        {
            if (check_func_syntax(&words, &i))
            {
                continue;
            }
            add_syntax(&type, words[i]);
        }
    }
    free(words);
}

bool
does_include_file_exist(const char *path)
{
    struct stat st;
    if (access(path, R_OK) != 0)
    {
        return false;
    }
    if (stat(path, &st) != -1 && (S_ISDIR(st.st_mode) || S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)))
    {
        return false;
    }
    return true;
}
