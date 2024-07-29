#include <Mlib/Profile.h>
#include "../include/prototypes.h"

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
    while (tok != nullptr)
    {
        if (i == cap)
        {
            cap *= 2;
            words = (char **)nrealloc(words, sizeof(char *) * cap);
        }
        remove_tabs_from_word(&tok);
        words[i++] = tok;
        tok        = strtok(nullptr, " ");
    }
    words[i] = nullptr;
    return words;
}

/* Return`s a malloc`ed 'char **' of all words in a str. */
char **
words_in_str(const char *str, unsigned long *size)
{
    PROFILE_FUNCTION;
    unsigned int i     = 0;
    unsigned int cap   = 10;
    char       **words = (char **)nmalloc(sizeof(char *) * cap);
    char        *data  = strdup(str);
    char        *tok   = strtok(data, " #");
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
words_in_file(const char *path)
{
    PROFILE_FUNCTION;
    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        NETLOGGER.log("Could not open file: %s", path);
        return;
    }
    fclose(f);
}
