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
words_in_str(const char *str)
{
    PROFILE_FUNCTION;
    unsigned int i, cap;
    char        *data, *tok, **words;
    i     = 0;
    cap   = 10;
    words = (char **)nmalloc(sizeof(char *) * cap);
    data  = strdup(str);
    tok   = strtok(data, " ");
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

const char *
extract_include(char *str)
{
    str += 1;
    str[strlen(str) - 1] = '\0';
    return str;
}
