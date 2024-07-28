#include "../include/prototypes.h"

#include <Mlib/Profile.h>

const char *
rgx_word(const char *word)
{
    static char buf[1024];
    sprintf(buf, "%s%s%s", "\\<(", word, ")\\>");
    return buf;
}

void
syntax_check_file(openfilestruct *file)
{
    PROFILE_FUNCTION;
    linestruct *line;
    for (line = file->filetop; line != nullptr; line = line->next)
    {
        check_for_syntax_words(line);
    }
}

bool
is_word_func(char *word)
{
    unsigned int i;
    for (i = 0; word[i]; i++)
    {
        if (word[i] == '(')
        {
            word[i] = '\0';
            return true;
        }
    }
    return false;
}

bool
parse_color_opts(const char *color, short *fg, short *bg, int *attr)
{
    bool  vivid, thick;
    char *comma;
    *attr = A_NORMAL;
    if (strncmp(color, "bold", 4) == 0)
    {
        *attr |= A_BOLD;
        if (color[4] != ',')
        {
            NETLOGGER.log("%s: Attribute 'bold' requires a subsequent comma.\n", __func__);
            jot_error("%s: Attribute 'bold' requires a subsequent comma.\n", __func__);
            return false;
        }
        color += 5;
    }
    if (strncmp(color, "italic", 6) == 0)
    {
        *attr |= A_ITALIC;
        if (color[6] != ',')
        {
            NETLOGGER.log("%s: Attribute 'italic' requires a subsequent comma.\n", __func__);
            jot_error("%s: Attribute 'italic' requires a subsequent comma.\n", __func__);
        }
        color += 7;
    }
    comma = strrchr((char *)color, ',');
    if (comma != nullptr)
    {
        *comma = '\0';
    }
    if (comma == nullptr || comma > color)
    {
        *fg = color_to_short(color, vivid, thick);
        if (*fg == BAD_COLOR)
        {
            return false;
        }
        if (vivid && !thick && COLORS > 8)
        {
            fg += 8;
        }
        else if (vivid)
        {
            *attr |= A_BOLD;
        }
    }
    else
    {
        *fg = THE_DEFAULT;
    }
    if (comma)
    {
        *bg = color_to_short(comma + 1, vivid, thick);
        if (*bg == BAD_COLOR)
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
        *bg = THE_DEFAULT;
    }
    return true;
}

/* Add syntax regex to end of colortype list 'c'. */
void
add_syntax_color(const char *color, const char *rgxstr, colortype *c)
{
    if (c == nullptr)
    {
        return;
    }
    regex_t   *rgx = nullptr;
    colortype *nc  = nullptr;
    short      fg, bg;
    int        attr;
    if (!parse_color_opts(color, &fg, &bg, &attr))
    {
        return;
    }
    if (!compile(rgxstr, NANO_REG_EXTENDED, &rgx))
    {
        return;
    }
    nc             = (colortype *)nmalloc(sizeof(colortype));
    nc->start      = rgx;
    nc->end        = nullptr;
    nc->fg         = fg;
    nc->bg         = bg;
    nc->attributes = attr;
    nc->pairnum    = c->pairnum + 1;
    nc->next       = nullptr;
    c->next        = nc;
}
