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
