#include "../include/prototypes.h"

/* Returns 'TRUE' when a line is a '//' comment. */
bool
is_line_comment(linestruct *line)
{
    for (unsigned int i = indent_char_len(line); line->data[i]; i++)
    {
        if (!line->data[i + 1])
        {
            break;
        }
        if (line->data[i] == '/' && line->data[i + 1] == '/')
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool
is_line_start_end_bracket(linestruct *line, bool *is_start)
{
    unsigned long i;
    for (i = 0; line->data[i]; i++)
    {
        if (line->data[i] == '{')
        {
            if (line->data[i + 1])
            {
                if (line->data[i + 1] == '}')
                {
                    return FALSE;
                }
            }
            *is_start = TRUE;
            return TRUE;
        }
        else if (line->data[i] == '}')
        {
            *is_start = FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

bool
is_line_in_bracket_pair(const unsigned long lineno)
{
    for (const auto &[start, end] : bracket_pairs)
    {
        if (start < lineno && end > lineno)
        {
            return TRUE;
        }
        if (start > lineno && end > lineno)
        {
            break;
        }
    }
    return FALSE;
}

/* Return`s 'TRUE' if the first char in a line is '\0'. */
bool
is_empty_line(linestruct *line)
{
    unsigned i = 0;
    for (; line->data[i]; i++)
        ;
    return (i == 0);
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

unsigned long
get_line_total_tabs(linestruct *line)
{
    unsigned long i, tabs = 0, spaces = 0, total = 0;
    if (line->data[0] != ' ' && line->data[0] != '\t')
    {
        return 0;
    }
    for (i = 0; line->data[i]; i++)
    {
        if (line->data[i] == ' ')
        {
            spaces++;
        }
        else if (line->data[i] == '\t')
        {
            tabs++;
        }
    }
    total = tabs;
    if (spaces)
    {
        total += spaces / WIDTH_OF_TAB;
    }
    return total;
}

/* Move a line up or down.  Will later be expanded to also move all marked lines. */
void
move_line(bool up)
{
    char *tmp_data = NULL;
    /* For now if mark is set just return early. */
    if (openfile->mark != NULL)
    {
        return;
    }
    if (up == TRUE)
    {
        if (openfile->current->prev != NULL)
        {
            tmp_data = copy_of(openfile->current->prev->data);
            free(openfile->current->prev->data);
            openfile->current->prev->data = copy_of(openfile->current->data);
            free(openfile->current->data);
            openfile->current->data = copy_of(tmp_data);
            free(tmp_data);
            openfile->current = openfile->current->prev;
        }
    }
    else
    {
        if (openfile->current->next != NULL)
        {
            tmp_data = copy_of(openfile->current->next->data);
            free(openfile->current->next->data);
            openfile->current->next->data = copy_of(openfile->current->data);
            free(openfile->current->data);
            openfile->current->data = copy_of(tmp_data);
            free(tmp_data);
            openfile->current = openfile->current->next;
        }
    }
    set_modified();
    refresh_needed = TRUE;
}

void
move_lines_up(void)
{
    if (openfile->current->lineno == 1)
    {
        return;
    }
    add_undo(MOVE_LINE_UP, NULL);
    move_line(TRUE);
}

void
move_lines_down(void)
{
    if (openfile->current->next == NULL)
    {
        return;
    }
    add_undo(MOVE_LINE_DOWN, NULL);
    move_line(FALSE);
}
