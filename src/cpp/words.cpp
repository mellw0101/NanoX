#include <string.h>
#include "../include/definitions.h"
#include "../include/prototypes.h"

#include <Mlib/Io.h>
#include <Mlib/Profile.h>

/* Remove all tabs from a word passed by refrence. */
void
remove_tabs_from_word(char **word)
{
    unsigned int i;
    for (i = 0; (*word)[i]; i++)
    {
        if ((*word)[i] != '\t')
        {
            break;
        }
    }
    *word += i;
    for (i = 0; (*word)[i]; i++)
    {
        if ((*word)[i] == '[')
        {
            (*word)[i] = '\0';
        }
    }
}

/* Creates a malloc`ed 'char **' containing all words in a line. */
char **
words_in_line(linestruct *line)
{
    unsigned int cap = 10, i = 0;
    char       **words = (char **)nmalloc(sizeof(char *) * cap);
    char        *data  = strdup(line->data);
    char        *tok   = strtok(data, " (");
    while (tok != NULL)
    {
        if (i == cap)
        {
            cap *= 2;
            words = (char **)nrealloc(words, sizeof(char *) * cap);
        }
        remove_tabs_from_word(&tok);
        words[i++] = tok;
        tok        = strtok(NULL, " ");
    }
    words[i] = NULL;
    return words;
}

/* Return`s a malloc`ed 'char **' of all words in a str. */
char **
words_in_str(const char *str, unsigned long *size)
{
    PROFILE_FUNCTION;
    if (str == NULL)
    {
        return NULL;
    }
    unsigned int i     = 0;
    unsigned int cap   = 10;
    char       **words = (char **)nmalloc(sizeof(char *) * cap);
    char        *data  = strdup(str);
    char        *tok   = strtok(data, " #\t");
    while (tok != NULL)
    {
        if (i == cap)
        {
            cap *= 2;
            words = (char **)nrealloc(words, sizeof(char *) * cap);
        }
        remove_tabs_from_word(&tok);
        words[i++] = tok;
        tok        = strtok(NULL, " \t");
    }
    words[i] = NULL;
    (size != NULL) ? *size = 1 : 0;
    return words;
}

/* Remove first and last char from 'str' */
const char *
extract_include(char *str)
{
    str += 1;
    str[strlen(str) - 1] = '\0';
    return str;
}

char **
append_arry(char **dst, unsigned long size_dst, char **src, unsigned long size_src)
{
    char        **result = (char **)nmalloc(sizeof(char *) * (size_dst + size_src));
    unsigned long i;
    for (i = 0; i < size_dst; i++)
    {
        result[i] = strdup(dst[i]);
    }
    for (i = 0; i < size_src; i++)
    {
        result[size_dst + i] = strdup(src[i]);
    }
    return result;
}

void
add_word_to_arry(const char *word, char ***words, unsigned long *cword, unsigned long *cap)
{
    if (*cword == *cap)
    {
        *cap *= 2;
        *words = (char **)realloc(*words, sizeof(char *) * (*cap));
    }
    char *nword = (char *)malloc(strlen(word) + 1);
    strcpy(nword, word);
    *(words[*(cword++)]) = nword;
}

void
words_in_file(const char *path)
{
    // unsigned long cap   = 10;
    // char       ***lines = (char ***)nmalloc(sizeof(char **) * cap);
}

/* Returns the text after '.' in 'openfile->filename'. */
char *
get_file_extention(void)
{
    unsigned int i;
    char        *fname = strdup(openfile->filename);
    for (i = 0; fname[i]; i++)
    {
        if (fname[i] == '.')
        {
            fname += i + 1;
            break;
        }
    }
    return fname;
}

bool
is_word_func(char *word, unsigned long *at)
{
    unsigned int i;
    for (i = 0; word[i]; i++)
    {
        if (word[i] == '(')
        {
            word[i] = '\0';
            *at     = i;
            return TRUE;
        }
    }
    return FALSE;
}

void
remove_leading_ptrs(char **word)
{
    while (*(*word) == '*')
    {
        *word += 1;
    }
}

void
remove_leading_parent(char **word)
{
    while (*(*word) == '(')
    {
        *word += 1;
    }
}

/* Return`s "\<('word')\>". */
const char *
rgx_word(const char *word)
{
    static char buf[1024];
    sprintf(buf, "%s%s%s", "\\<(", word, ")\\>");
    return buf;
}

/* Concat path from 2 string`s.  if end of 's1' is not '/', it is added. */
const char *
concat_path(const char *s1, const char *s2)
{
    static char   buf[PATH_MAX];
    unsigned long len_s1 = strlen(s1);
    if (s1[len_s1 - 1] == '/')
    {
        snprintf(buf, sizeof(buf), "%s%s", s1, s2);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s%s%s", s1, "/", s2);
    }
    return buf;
}

/* Check either behind or infront of 'cursor_x' if there are more than a single ' ' char.
 * If so, return 'TRUE' and asign the number of spaces to 'nspaces'. */
bool
word_more_then_one_space_away(bool forward, unsigned long *nspaces)
{
    unsigned long i = openfile->current_x, spaces = 0;
    if (!forward)
    {
        i--;
        for (; i != (unsigned long)-1 && openfile->current->data[i] == ' '; i--, spaces++)
            ;
    }
    else
    {
        for (; openfile->current->data[i] && openfile->current->data[i] == ' '; i++, spaces++)
            ;
    }
    (spaces > 1) ? *nspaces = spaces : 0;
    return (spaces > 1);
}
