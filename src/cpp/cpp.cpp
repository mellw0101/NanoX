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

void
do_block_comment(void)
{
    PROFILE_FUNCTION;
    enclose_marked_region("/* ", " */");
    refresh_needed = true;
}
