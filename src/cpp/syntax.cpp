#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <fcntl.h>

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
    if (openfile->filetop->next == nullptr)
    {
        return;
    }
    const char *fext = get_file_extention(openfile->filename);
    if (strcmp(fext, "cpp") == 0 || strcmp(fext, "c") == 0)
    {
        linestruct *line;
        for (line = file->filetop; line != NULL; line = line->next)
        {
            check_for_syntax_words(line);
        }
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
            return TRUE;
        }
    }
    return FALSE;
}

bool
parse_color_opts(const char *color_fg, const char *color_bg, short *fg, short *bg, int *attr)
{
    bool vivid, thick;
    *attr = A_NORMAL;
    if (color_fg != NULL)
    {
        if (strncmp(color_fg, "bold", 4) == 0)
        {
            *attr |= A_BOLD;
            color_fg += 5;
        }
        if (strncmp(color_fg, "italic", 6) == 0)
        {
            *attr |= A_ITALIC;
            color_fg += 7;
        }
        *fg = color_to_short(color_fg, vivid, thick);
        if (*fg == BAD_COLOR)
        {
            return FALSE;
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
    if (color_bg != NULL)
    {
        if (strncmp(color_bg, "bold", 4) == 0)
        {
            *attr |= A_BOLD;
            color_bg += 5;
        }
        if (strncmp(color_bg, "italic", 6) == 0)
        {
            *attr |= A_ITALIC;
            color_bg += 7;
        }
        *bg = color_to_short(color_bg, vivid, thick);
        if (*bg == BAD_COLOR)
        {
            return FALSE;
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
    return TRUE;
}

/* Add syntax regex to end of colortype list 'c'. */
void
add_syntax_color(const char *color_fg, const char *color_bg, const char *rgxstr, colortype **c)
{
    if (c == nullptr)
    {
        return;
    }
    short      fg, bg;
    int        attr;
    regex_t   *start_rgx = NULL;
    colortype *nc        = NULL;
    if (!parse_color_opts(color_fg, color_bg, &fg, &bg, &attr))
    {
        return;
    }
    if (!compile(rgxstr, NANO_REG_EXTENDED, &start_rgx))
    {
        return;
    }
    nc             = (colortype *)nmalloc(sizeof(colortype));
    nc->start      = start_rgx;
    nc->end        = NULL;
    nc->fg         = fg;
    nc->bg         = bg;
    nc->attributes = attr;
    nc->pairnum    = (*c)->pairnum + 1;
    nc->next       = NULL;
    (*c)->next     = nc;
    (*c)           = (*c)->next;
}

void
add_start_end_syntax(const char *color_fg, const char *color_bg, const char *start, const char *end, colortype **c)
{
    short      fg, bg;
    int        attr;
    regex_t   *start_rgx = NULL, *end_rgx = NULL;
    colortype *nc;
    if (c == NULL)
    {
        return;
    }
    if (!parse_color_opts(color_fg, color_bg, &fg, &bg, &attr))
    {
        return;
    }
    if (!compile(start, NANO_REG_EXTENDED, &start_rgx))
    {
        return;
    }
    if (!compile(end, NANO_REG_EXTENDED, &end_rgx))
    {
        regfree(start_rgx);
        free(start_rgx);
        return;
    }
    nc             = (colortype *)nmalloc(sizeof(colortype));
    nc->start      = start_rgx;
    nc->end        = end_rgx;
    nc->fg         = fg;
    nc->bg         = bg;
    nc->attributes = attr;
    nc->pairnum    = (*c)->pairnum + 1;
    nc->next       = NULL;
    (*c)->next     = nc;
    (*c)           = (*c)->next;
}

bool
check_func_syntax(char ***words, unsigned int *i)
{
    if ((*words)[++(*i)] == NULL)
    {
        return false;
    }
    /* Remove all if any preceding '*' char`s. */
    while (*((*words)[*i]) == '*')
    {
        (*words)[*i] += 1;
    }
    if (is_word_func((*words)[*i]))
    {
        add_syntax_word("yellow", NULL, rgx_word((*words)[*i]));
        return true;
    }
    if ((*words)[(*i) + 1] != NULL)
    {
        if (*((*words)[(*i) + 1]) == '(')
        {
            add_syntax_word("yellow", NULL, rgx_word((*words)[*i]));
            return true;
        }
    }
    return false;
}

/* Check a file for syntax and add relevent syntax. */
void
check_syntax(const char *path)
{
    PROFILE_FUNCTION;
    char         *buf = nullptr, **words;
    unsigned int  i;
    unsigned long size, len;
    FILE         *f = fopen(path, "rb");
    if (f == NULL)
    {
        return;
    }
    while ((len = getline(&buf, &size, f)) != EOF)
    {
        if (buf[len - 1] == '\n')
        {
            buf[--len] = '\0';
        }
        words = words_in_str(buf);
        if (*words == NULL)
        {
            continue;
        }
        for (i = 0; words[i] != NULL; i++)
        {
            const unsigned short type = retrieve_c_syntax_type(words[i]);
            if (words[i + 1] == NULL)
            {
                break;
            }
            if (!type)
            {
                continue;
            }
            if (type & CS_STRUCT || type & CS_ENUM || type & CS_CLASS)
            {
                add_syntax_word("brightgreen", NULL, rgx_word(words[++i]));
            }
            else if (type & CS_CHAR || type & CS_VOID || type & CS_INT || type & CS_LONG || type & CS_BOOL ||
                     type & CS_SIZE_T || type & CS_SSIZE_T)
            {
                if (check_func_syntax(&words, &i))
                {
                    continue;
                }
            }
            else if (type & CS_DEFINE)
            {
                handle_define(words[++i]);
            }
        }
        free(words);
    }
    fclose(f);
}

int
add_syntax(const unsigned short *type, char *word)
{
    if (*type & CS_STRUCT || *type & CS_ENUM || *type & CS_CLASS)
    {
        add_syntax_word("brightgreen", NULL, rgx_word(word));
    }
    else if (*type & CS_INT || *type & CS_VOID || *type & CS_LONG || *type & CS_CHAR || *type & CS_BOOL ||
             *type & CS_SIZE_T || *type & CS_SSIZE_T)
    {
        /* Remove all if any '*' char`s */
        while (word[0] == '*')
        {
            word += 1;
        }
        if (word[0] == '\0')
        {
            return NEXT_WORD_ALSO;
        }
        unsigned int i = 0;
        for (i = 0; word[i]; i++)
        {
            if (word[i] == ',')
            {
                word[i] = '\0';
                add_syntax_word("lagoon", NULL, rgx_word(word));
                return NEXT_WORD_ALSO;
            }
            if (word[i] == ';' || word[i] == ')')
            {
                word[i] = '\0';
                break;
            }
        }
        add_syntax_word("lagoon", NULL, rgx_word(word));
    }
    return 0;
}

void
handle_include(char *str)
{
    static char        buf[PATH_MAX];
    static const char *pwd          = NULL;
    char              *current_file = NULL;
    unsigned long      i, pos;
    if (str == NULL)
    {
        return;
    }
    if (*str == '<')
    {
        sprintf(buf, "%s%s", "/usr/include/", extract_include(str));
        if (is_file_and_exists(buf))
        {
            check_syntax(buf);
            return;
        }
        str += 1;
        if (*str == 'c')
        {
            str += 1;
        }
        memset(buf, '\0', sizeof(buf));
        sprintf(buf, "%s%s", "/usr/include/c++/v1/", str);
        if (is_file_and_exists(str))
        {
            check_syntax(buf);
            return;
        }
        memset(buf, '\0', sizeof(buf));
        sprintf(buf, "%s%s%s", "/usr/include/c++/v1/", str, ".h");
        if (is_file_and_exists(buf))
        {
            check_syntax(buf);
            return;
        }
    }
    else if (*str == '"')
    {
        pwd = getenv("PWD");
        if (pwd == NULL)
        {
            return;
        }
        current_file = strdup(openfile->filename);
        for (i = 0, pos = 0; current_file[i]; i++)
        {
            if (current_file[i] == '/')
            {
                pos = i;
            }
        }
        if (pos != 0)
        {
            current_file[pos + 1] = '\0';
        }
        sprintf(buf, "%s%s%s%s", pwd, "/", (current_file != NULL) ? current_file : "", extract_include(str));
        if (is_file_and_exists(buf))
        {
            check_syntax(buf);
        }
        free(current_file);
    }
}

/* Add a '#define' to syntax. */
void
handle_define(char *str)
{
    unsigned int i;
    if (*str == '\\')
    {
        return;
    }
    for (i = 0; str[i]; i++)
    {
        if (str[i] == '(')
        {
            str[i] = '\0';
            break;
        }
    }
    add_syntax_word("lagoon", NULL, rgx_word(str));
}
