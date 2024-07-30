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
add_syntax_word(const char *color_fg, const char *color_bg, const char *word)
{
    PROFILE_FUNCTION;
    if (last_c_color == NULL)
    {
        last_c_color = get_last_c_colortype();
    }
    /* Add, then update the syntax. */
    add_syntax_color(color_fg, color_bg, word, &last_c_color);
}

/* Add some basic cpp syntax. */
void
do_cpp_syntax(void)
{
    add_syntax_word("gray", NULL, ";");
    add_syntax_word("brightred", NULL, "\\<[A-Z_][0-9A-Z_]*\\>");
    add_syntax_word("blue", NULL, "\\<(nullptr)\\>");
    add_syntax_word("brightmagenta", NULL, "^[[:blank:]]*[A-Z_a-z][0-9A-Z_a-z]*:[[:blank:]]*$");
    add_syntax_word("normal", NULL, ":[[:blank:]]*$");
    /* This makes word after green, while typing. */
    add_syntax_word("brightgreen", NULL, "(namespace|enum|struct|class)[[:blank:]]+[A-Za-z_][A-Za-z_0-9]*");
    /* Types and related keywords. */
    add_syntax_word("brightblue", NULL,
                    "\\<(auto|bool|char|const|double|enum|extern|float|inline|int|long|restrict|short|signed|"
                    "sizeof|static|struct|typedef|union|unsigned|void)\\>");
    add_syntax_word("brightgreen", NULL, "\\<([[:lower:]][[:lower:]_]*|(u_?)?int(8|16|32|64))_t\\>");
    add_syntax_word(
        "green", NULL,
        "\\<(_(Alignas|Alignof|Atomic|Bool|Complex|Generic|Imaginary|Noreturn|Static_assert|Thread_local))\\>");
    add_syntax_word("brightblue", NULL,
                    "\\<(class|explicit|friend|mutable|namespace|override|private|protected|public|register|"
                    "template|this|typename|virtual|volatile|false|true)\\>");
    add_syntax_word("brightgreen", NULL, "\\<(std|string|vector)\\>");
    /* Flow control. */
    add_syntax_word("brightyellow", NULL, "\\<(if|else|for|while|do|switch|case|default)\\>");
    add_syntax_word("brightyellow", NULL, "\\<(try|throw|catch|operator|new|delete)\\>");
    add_syntax_word("brightmagenta", NULL, "\\<(using|break|continue|goto|return)\\>");
    add_syntax_word("brightmagenta", NULL, "'([^'\\]|\\([\"'\abfnrtv]|x[[:xdigit:]]{1,2}|[0-3]?[0-7]{1,2}))'");
    add_syntax_word("cyan", NULL,
                    "__attribute__[[:blank:]]*\\(\\([^)]*\\)\\)|__(aligned|asm|builtin|hidden|inline|packed|"
                    "restrict|section|typeof|weak)__");
    add_syntax_word("green", NULL, "//[^\"]*$|(^|[[:blank:]])//.*");
    add_start_end_syntax("green", NULL, "/\\*", "\\*/", &last_c_color);
    add_syntax_word("brightwhite", "yellow", "\\<(FIXME|TODO|XXX)\\>");
    add_syntax_word(NULL, "green", "[[:space:]]+$");
    update_c_syntaxtype();
}

static unsigned short last_type = 0;

/* Check a line for syntax words, also index files that are included and add functions as well. */
void
check_for_syntax_words(linestruct *line)
{
    unsigned i;
    char   **words;
    if (is_empty_line(line))
    {
        return;
    }
    words = words_in_str(line->data);
    if (*words == NULL)
    {
        free(words);
        return;
    }
    for (i = 0; words[i] != nullptr; i++)
    {
        const unsigned short type = retrieve_c_syntax_type(words[i]);
        if (last_type != 0)
        {
            if (last_type & CS_VOID || last_type & CS_INT || last_type & CS_CHAR || last_type & CS_LONG ||
                last_type & CS_BOOL)
            {
                unsigned int j;
                for (j = 0; (words[i])[j]; j++)
                {
                    if ((words[i])[j] == '(')
                    {
                        (words[i])[j] = '\0';
                    }
                }
                add_syntax_word("yellow", NULL, rgx_word(words[i]));
                last_type = 0;
                continue;
            }
        }
        if (words[i + 1] == NULL)
        {
            last_type = type;
            break;
        }
        if (!type)
        {
            continue;
        }
        else if (type & CS_STRUCT || type & CS_CLASS || type & CS_ENUM)
        {
            add_syntax_word("brightgreen", NULL, rgx_word(words[++i]));
        }
        else if (type & CS_LONG || type & CS_VOID || type & CS_INT || type & CS_CHAR || type & CS_BOOL)
        {
            if (check_func_syntax(&words, &i))
            {
                continue;
            }
            if (add_syntax(&type, words[i]) == NEXT_WORD_ALSO)
            {
                while (true)
                {
                    if (words[++i] == NULL)
                    {
                        break;
                    }
                    if (add_syntax(&type, words[i]) != NEXT_WORD_ALSO)
                    {
                        break;
                    }
                }
                continue;
            }
        }
        else if (type & CS_INCLUDE)
        {
            handle_include(words[++i]);
        }
    }
    free(words);
}

colortype *
get_last_c_colortype(void)
{
    if (c_syntaxtype == NULL)
    {
        c_syntaxtype = get_c_syntaxtype();
        if (c_syntaxtype == NULL)
        {
            return NULL;
        }
    }
    colortype *c = NULL;
    /* Find end of colortype list in syntax 'c' */
    for (c = c_syntaxtype->color; c->next != NULL; c = c->next)
        ;
    /* Return if 'c' == nullptr */
    if (c == NULL)
    {
        return NULL;
    }
    return c;
}

syntaxtype *
get_c_syntaxtype(void)
{
    syntaxtype *s = NULL;
    for (s = syntaxes; s != NULL && strcmp(s->name, "c"); s = s->next)
        ;
    return s;
}

void
update_c_syntaxtype(void)
{
    if (c_syntaxtype == NULL)
    {
        c_syntaxtype = get_c_syntaxtype();
        if (c_syntaxtype == NULL)
        {
            return;
        }
    }
    set_syntax_colorpairs(c_syntaxtype);
}
