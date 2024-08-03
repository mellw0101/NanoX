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
