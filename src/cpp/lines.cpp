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
is_line_start_end_bracket(linestruct *line, bool *start)
{
    unsigned long i;
    for (i = 0; line->data[i]; i++)
    {
        if (line->data[i] == '{')
        {
            *start = TRUE;
            return TRUE;
        }
        else if (line->data[i] == '}')
        {
            *start = FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

void
add_bracket_pair(const unsigned long start, const unsigned long end)
{
    bracket_pair bp;
    bp.start_line = start;
    bp.end_line   = end;
    bracket_pairs.push_back(bp);
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

void
all_brackets_pos(void)
{
    linestruct *line;
    long        start_pos = -1, end_pos = -1;
    bool        start;
    for (line = openfile->filetop; line != NULL; line = line->next)
    {
        if (is_line_start_end_bracket(line, &start))
        {
            if (start == TRUE)
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
        }
    }
    for (line = openfile->filetop; line != NULL; line = line->next)
    {
        if (is_line_in_bracket_pair(line->lineno))
        {
            NETLOGGER.log("line: %lu\n", line->lineno);
        }
    }
}
