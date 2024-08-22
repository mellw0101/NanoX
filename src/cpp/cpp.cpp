#include "../include/prototypes.h"

#include "../include/definitions.h"

#include <Mlib/Profile.h>
#include <Mlib/def.h>

/* Returns 'TRUE' if 'c' is a cpp syntax char. */
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

/* If an area is marked then plase the 'str' at mark and current_x, thereby enclosing the marked area.
 * TODO: (enclose_marked_region) - Make the undo one action insted of two seprate once. */
void
enclose_marked_region(const char *s1, const char *s2)
{
    /* Sanity check, Returns is there is no mark */
    if (openfile->mark == NULL)
    {
        return;
    }
    linestruct   *was_current = openfile->current, *top, *bot;
    unsigned long top_x, bot_x;
    get_region(&top, &top_x, &bot, &bot_x);
    openfile->current = top;
    add_undo(REPLACE, NULL);
    inject_in_line(&top, s1, top_x);
    /* If top and bot is equal, move mark to the right by 's1' len. */
    (top == bot) ? (bot_x += strlen(s1)) : 0;
    openfile->current = bot;
    add_undo(REPLACE, NULL);
    inject_in_line(&bot, s2, bot_x);
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
    enclose_marked_region("/* ", " */");
    refresh_needed = TRUE;
}

/* Check if enter is requested when betweeen '{' and '}'.
 * if so properly open the brackets, place cursor in the middle and indent once.
 * Return`s 'false' if not between them, otherwise return`s 'TRUE' */
bool
enter_with_bracket(void)
{
    char          c, c_prev;
    linestruct   *was_current = openfile->current, *middle, *end;
    bool          allblanks   = false;
    unsigned long extra;
    if (!openfile->current->data[openfile->current_x - 1])
    {
        return FALSE;
    }
    c_prev = openfile->current->data[openfile->current_x - 1];
    c      = openfile->current->data[openfile->current_x];
    if (c_prev != '{' || c != '}')
    {
        return FALSE;
    }
    extra = indent_length(was_current->data);
    if (extra == openfile->current_x)
    {
        allblanks = (indent_length(openfile->current->data) == extra);
    }
    middle       = make_new_node(openfile->current);
    middle->data = (char *)nmalloc(strlen(openfile->current->data + openfile->current_x) + extra + 1);
    /* Here we pass the first char as a ref, i.e: A ptr to the first actual char. */
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
    add_undo(ENTER, NULL);
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
    add_undo(ENTER, NULL);
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
    refresh_needed = TRUE;
    focusing       = FALSE;
    return TRUE;
}

void
add_bracket_pair(const unsigned long start, const unsigned long end)
{
    bracket_pair bp;
    bp.start_line = start;
    bp.end_line   = end;
    bracket_pairs.push_back(bp);
}

void
all_brackets_pos(void)
{
    linestruct *line;
    long        start_pos = -1, end_pos = -1;
    bool        is_start;
    for (line = openfile->filetop; line != NULL; line = line->next)
    {
        if (is_line_start_end_bracket(line, &is_start))
        {
            if (is_start == TRUE)
            {
                start_pos = line->lineno;
            }
            else
            {
                if (start_pos != -1)
                {
                    end_pos = line->lineno;
                    add_bracket_pair(start_pos, end_pos);
                    start_pos = -1, end_pos = -1;
                }
            }
            create_bracket_entry(line->lineno, get_line_total_tabs(line), is_start);
        }
    }
    // for (const auto &be : bracket_entrys)
    // {
    //     netlog_bracket_entry(be);
    // }
}

void
get_bracket_start_end(unsigned long lineno, unsigned long *start, unsigned long *end)
{
    *start = 0, *end = 0;
    for (const auto &[s, e] : bracket_pairs)
    {
        if (s < lineno && e > lineno)
        {
            *start = s, *end = e;
            return;
        }
        if (s > lineno && e > lineno)
        {
            return;
        }
    }
}

void
do_close_bracket(void)
{
    unsigned long start_line, end_line;
    linestruct   *line, *was_current;
    get_bracket_start_end(openfile->current->lineno, &start_line, &end_line);
    if (start_line != 0 && end_line != 0)
    {
        NETLOGGER.log("is inside bracket, start: %lu, end: %lu\n", start_line, end_line);
        line        = line_from_number(start_line + 1);
        was_current = line;
        for (; line->lineno != end_line; line = line->next)
        {
            NETLOGGER.log("%s\n", line->data);
            line->hidden = TRUE;
        }
        edit_redraw(was_current, FLOWING);
        // draw_row(openfile->current->lineno, "", openfile->current, 0);
    }
}

void
do_test_window(void)
{
    NETLOGGER.log("test win\n");
    if (test_win != NULL)
    {
        delwin(test_win);
    }
    test_win = newwin(20, 20, 0, 0);
}
