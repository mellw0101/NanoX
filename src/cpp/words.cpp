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
    (size != NULL) ? *size = i : 0;
    return words;
}

char **
delim_str(const char *str, const char *delim, unsigned long *size)
{
    PROFILE_FUNCTION;
    if (str == NULL)
    {
        return NULL;
    }
    unsigned int bsize = 0, cap = 10;
    char       **words = (char **)nmalloc(sizeof(char *) * cap);
    char        *tok   = strtok((char *)str, delim);
    while (tok != NULL)
    {
        if (bsize == cap)
        {
            cap *= 2;
            words = (char **)nrealloc(words, sizeof(char *) * cap);
        }
        words[bsize++] = copy_of(tok);
        tok            = strtok(NULL, delim);
    }
    words[bsize] = NULL;
    (size != NULL) ? *size = bsize : 0;
    return words;
}

char **
split_into_words(const char *str, const unsigned int len,
                 unsigned int *word_count)
{
    const unsigned int max_words = (len / 2) + 1;
    char             **words     = (char **)nmalloc(sizeof(char *) * max_words);
    unsigned int       count     = 0;
    const char        *start = str, *end = str;
    while (end < (str + len))
    {
        for (; end < (str + len) && *end == ' '; end++);
        if (end == str + len)
        {
            break;
        }
        start = end;
        for (; end < (str + len) && *end != ' '; end++);
        const unsigned int word_len = end - start;
        words[count++]              = measured_copy(start, word_len);
    }
    *word_count = count;
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
append_arry(char **dst, unsigned long size_dst, char **src,
            unsigned long size_src)
{
    char **result = (char **)nmalloc(sizeof(char *) * (size_dst + size_src));
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
add_word_to_arry(const char *word, char ***words, unsigned long *cword,
                 unsigned long *cap)
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

/* Remove`s all leading 'c' char`s from 'word'.  Note that 'word' shall be
 * passed by refrence. */
void
remove_leading_char_type(char **word, const char c)
{
    while (*(*word) == c)
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

/* Assigns the number of steps of char 'ch' to the prev/next word to 'nchars'.
 * Return`s 'TRUE' when word is more then 2 steps of 'ch' away. */
bool
word_more_than_one_char_away(bool forward, unsigned long *nchars, const char ch)
{
    unsigned long i = openfile->current_x, chars = 0;
    if (!forward)
    {
        i--;
        for (; i != (unsigned long)-1 && openfile->current->data[i] == ch;
             i--, chars++);
    }
    else
    {
        for (; openfile->current->data[i] && openfile->current->data[i] == ch;
             i++, chars++);
    }
    (chars > 1) ? *nchars = chars : 0;
    return (chars > 1);
}

bool
word_more_than_one_white_away(bool forward, unsigned long *nsteps)
{
    unsigned long i = openfile->current_x, chars = 0;
    if (!forward)
    {
        i--;
        for (; i != (unsigned long)-1 && (openfile->current->data[i] == ' ' ||
                                          openfile->current->data[i] == '\t');
             i--, chars++);
    }
    else
    {
        for (;
             openfile->current->data[i] && (openfile->current->data[i] == ' ' ||
                                            openfile->current->data[i] == '\t');
             i++, chars++);
    }
    (chars > 1) ? *nsteps = chars : 0;
    return (chars > 1);
}

/* Check either behind or infront of 'current_x' if there are more than a single
 * ' ' char. If so, return 'TRUE' and asign the number of spaces to 'nspaces'.
 */
bool
word_more_than_one_space_away(bool forward, unsigned long *nspaces)
{
    return word_more_than_one_char_away(forward, nspaces, ' ');
}

/* Check either behind or infront of 'current_x' if there are more than a single
 * '\t' char. And if so, return 'TRUE' and assign the number of tabs to 'ntabs'.
 */
bool
word_more_than_one_tab_away(bool forward, unsigned long *ntabs)
{
    return word_more_than_one_char_away(forward, ntabs, '\t');
}

bool
prev_word_is_comment_start(unsigned long *nsteps)
{
    unsigned long i = openfile->current_x - 1, steps = 0;
    for (; i != (unsigned long)-1 && openfile->current->data[i] == ' ';
         i--, steps++);
    if (openfile->current->data[i - 1] &&
        (openfile->current->data[i - 1] == '/' &&
         openfile->current->data[i] == '/'))
    {
        *nsteps = steps + 2;
        return TRUE;
    }
    return FALSE;
}

/* Return`s 'TRUE' when 'ch' is found in 'word', and 'FALSE' otherwise. */
bool
char_is_in_word(const char *word, const char ch, unsigned long *at)
{
    *at = (unsigned long)-1;
    for (unsigned long i = 0; word[i] != '\0' && *at == (unsigned long)-1;
         (word[i] == ch) ? *at = i : 0, i++);
    return (*at != (unsigned long)-1);
}

char *
retrieve_word_from_cursor_pos(bool forward)
{
    const unsigned long slen = strlen(openfile->current->data);
    unsigned long       i;
    for (i = openfile->current_x; i < slen; i++)
    {
        if (!is_word_char(openfile->current->data + i, FALSE))
        {
            if (openfile->current->data[i] != '_')
            {
                break;
            }
        }
    }
    if (i == openfile->current_x)
    {
        return NULL;
    }
    return measured_copy(
        &openfile->current->data[openfile->current_x], i - openfile->current_x);
}

char **
fast_words_from_str(const char *str, unsigned long slen, unsigned long *nwords)
{
    PROFILE_FUNCTION;
    unsigned long size = 0, cap = 10;
    char        **words = (char **)nmalloc(sizeof(char *) * cap);
    (str[slen] == '\n') ? slen-- : 0;
    const char *start = str, *end = str;
    while (end < (str + slen))
    {
        for (; end < (str + slen) && (*end == ' ' || *end == '\t'); end++);
        if (end == (str + slen))
        {
            break;
        }
        start = end;
        for (; end < (str + slen) && *end != ' '; end++);
        const unsigned int word_len = end - start;
        (size == cap) ? cap *= 2,
            words     = (char **)nrealloc(words, sizeof(char *) * cap) : 0;
        words[size++] = measured_copy(start, word_len);
    }
    words[size] = NULL;
    *nwords     = size;
    return words;
}

line_word_t *
line_word_list(const char *str, unsigned long slen)
{
    line_word_t *head = NULL, *tail = NULL;
    (str[slen] == '\n') ? slen-- : 0;
    const char *start = str, *end = str;
    while (end < (str + slen))
    {
        for (; end < (str + slen) && (*end == ' ' || *end == '\t'); end++);
        if (end == (str + slen))
        {
            break;
        }
        while (!is_word_char(end, FALSE))
        {
            end += 1;
        }
        start = end;
        for (; (end < (str + slen)) && (*end != ' ') && (*end != '\t') &&
               (is_word_char(end, FALSE) || *end == '_');
             end++);
        const unsigned int word_len = end - start;
        line_word_t       *word     = (line_word_t *)malloc(sizeof(*word));
        word->str                   = measured_copy(start, word_len);
        word->start                 = start - str;
        word->len                   = word_len;
        word->end                   = word->start + word_len;
        word->next                  = NULL;
        if (!tail)
        {
            head = word;
            tail = word;
        }
        else
        {
            tail->next = word;
            tail       = tail->next;
        }
    }
    return head;
}

line_word_t *
line_word_list_is_word_char(const char *str, unsigned long slen)
{
    PROFILE_FUNCTION;
    line_word_t *head = NULL, *tail = NULL;
    (str[slen] == '\n') ? slen-- : 0;
    const char *start = str, *end = str;
    while (end < (str + slen))
    {
        for (; end < (str + slen) && (*end == ' ' || *end == '\t'); end++);
        if (end == (str + slen))
        {
            break;
        }
        start = end;
        for (; (end < (str + slen)) && (*end != ' ') && (*end != '\t'); end++);
        const unsigned int word_len = end - start;
        line_word_t       *word     = (line_word_t *)malloc(sizeof(*word));
        word->str                   = measured_copy(start, word_len);
        word->start                 = start - str;
        word->len                   = (end - start);
        word->end                   = word->start + (end - start);
        word->next                  = NULL;
        if (tail == NULL)
        {
            head = word;
            tail = word;
        }
        else
        {
            tail->next = word;
            tail       = tail->next;
        }
    }
    return head;
}

line_word_t *
make_line_word(char *str, unsigned short start, unsigned short len,
               unsigned short end)
{
    line_word_t *word = (line_word_t *)nmalloc(sizeof(*word));
    word->str         = str;
    word->start       = start;
    word->len         = len;
    word->end         = end;
    word->next        = NULL;
    return word;
}

unsigned int
last_strchr(const char *str, const char ch, unsigned int maxlen)
{
    unsigned int i = 0, last_seen = 0;
    for (; str[i] && (i < maxlen); i++)
    {
        if (str[i] == ch)
        {
            last_seen = i;
        }
    }
    return last_seen;
}

char *
memmove_concat(const char *s1, const char *s2)
{
    unsigned long len1 = strlen(s1);
    unsigned long len2 = strlen(s2);
    char         *data = (char *)nmalloc(len1 + len2 + 1);
    memmove(data, s1, len1);
    memmove(data + len1, s2, len2);
    data[len1 + len2] = '\0';
    return data;
}

const char *
substr(const char *str, unsigned long end_index)
{
    static char buf[PATH_MAX];
    for (uint i = 0; i < end_index; buf[i] = str[i], i++);
    return buf;
}
