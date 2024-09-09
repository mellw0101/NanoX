#include "../include/prototypes.h"

void
get_next_word(const char **start, const char **end)
{
    adv_ptr((*end), (*(*end) == ' ' || *(*end) == '\t'));
    *start = *end;
    adv_ptr((*end), (*(*end) != ' ' && *(*end) != '\t'));
}

/* Search for any match to the current suggest buf. */
void
find_suggestion(void)
{
    PROFILE_FUNCTION;
    for (const auto &i : func_info)
    {
        if (strncmp(i->full_function + strlen(i->return_type) + 1, suggest_buf,
                    suggest_len) == 0)
        {
            suggest_str = i->full_function + strlen(i->return_type) + 1;
            return;
        }
        if (openfile->current->lineno >= i->start_bracket &&
            openfile->current->lineno <= i->end_braket)
        {
            for (variable_t *var = i->params; var != NULL; var = var->prev)
            {
                if (var->name == NULL)
                {
                    continue;
                }
                if (strncmp(var->name, suggest_buf, suggest_len) == 0)
                {
                    suggest_str = var->name;
                    return;
                }
            }
        }
    }
    for (const auto &t : types)
    {
        if (strncmp(t, suggest_buf, suggest_len) == 0)
        {
            suggest_str = (char *)t;
            return;
        }
    }
    for (const auto &d : defines)
    {
        if (strncmp(d, suggest_buf, strlen(suggest_buf)) == 0)
        {
            suggest_str = d;
            nlog("define: %s\n", d);
            return;
        }
    }
    for (const auto &f : funcs)
    {
        if (strncmp(f, suggest_buf, suggest_len) == 0)
        {
            suggest_str = f;
            nlog("func: %s\n", f);
            return;
        }
    }
    for (const auto &s : structs)
    {
        if (strncmp(s, suggest_buf, suggest_len) == 0)
        {
            suggest_str = s;
            nlog("struct: %s\n", s);
            return;
        }
    }
    for (const auto &c : classes)
    {
        if (strncmp(c, suggest_buf, suggest_len) == 0)
        {
            suggest_str = c;
            nlog("class: %s\n", c);
            return;
        }
    }
}

/* Clear suggest buffer and len as well as setting the
 * current suggest str to NULL. */
void
clear_suggestion(void)
{
    suggest_on               = FALSE;
    suggest_str              = NULL;
    suggest_len              = 0;
    suggest_buf[suggest_len] = '\0';
}

/* Add last typed char to the suggest buffer. */
void
add_char_to_suggest_buf(void)
{
    if (openfile->current_x > 0)
    {
        const char *c = &openfile->current->data[openfile->current_x - 1];
        if (is_word_char(c - 1, FALSE))
        {
            suggest_len             = 0;
            const unsigned long pos = word_index(TRUE);
            for (int i = pos; i < openfile->current_x - 1; suggest_len++, i++)
            {
                suggest_buf[suggest_len] = openfile->current->data[i];
            }
            suggest_buf[suggest_len] = '\0';
        }
        suggest_buf[suggest_len++] = *c;
        suggest_buf[suggest_len]   = '\0';
    }
}

/* Draw current suggestion if found to the suggest window. */
void
draw_suggest_win(void)
{
    if (!suggest_str)
    {
        return;
    }
    unsigned long col_len = strlen(suggest_str) + 2;
    unsigned long row_len = 1;
    unsigned long row_pos = (openfile->cursor_row > editwinrows - 2) ?
                                openfile->cursor_row - row_len :
                                openfile->cursor_row + 1;
    unsigned long col_pos = xplustabs() + margin - suggest_len - 1;
    suggestwin            = newwin(row_len, col_len, row_pos, col_pos);
    mvwprintw(suggestwin, 0, 1, "%s", suggest_str);
    wrefresh(suggestwin);
    if (ISSET(SUGGEST_INLINE))
    {
        rendr(SUGGEST, suggest_str);
    }
}

/* Parse function signature. */
char *
parse_function_sig(linestruct *line)
{
    const char *p           = strchr(line->data, '{');
    const char *param_start = NULL;
    if (p && (p == line->data || *p == line->data[indent_char_len(line)]))
    {
        line = line->prev;
        if (line->data[0] == ' ' || line->data[0] == '\t')
        {

            return NULL;
        }
    }
    param_start = strchr(line->data, '(');
    p           = strchr(line->data, ' ');
    char *sig   = NULL;
    char *ret_t = NULL;
    char *ret   = NULL;
    if (p && p < (param_start - 1))
    {
        if (p == line->data)
        {
            for (; *p && (*p == ' ' || *p == '\t'); p++);
            sig = copy_of(p);
        }
        else
        {
            ret = copy_of(line->data);
            p   = strchr(ret, ' ');
        }
    }
    else
    {
        sig = copy_of(line->data);
        for (int i = 0; sig[i] && i < (param_start - line->data); i++)
        {
            if (sig[i] == ' ' || sig[i] == '\t')
            {
                alloced_remove_at(&sig, i);
            }
        }
    }
    if (!ret)
    {
        if (!line->prev->data[0])
        {

            return NULL;
        }
        ret_t = copy_of(line->prev->data);
        append_str(&ret_t, " ");
        ret = alloc_str_free_substrs(ret_t, sig);
    }
    return ret;
}

/* Inject a suggestion. */
void
accept_suggestion(void)
{
    if (suggest_str != NULL)
    {
        inject(suggest_str + suggest_len, strlen(suggest_str) - suggest_len);
    }
    clear_suggestion();
}

void
find_word(linestruct *line, const char *data, const char *word,
          const unsigned long slen, const char **start, const char **end)
{
    *start = strstr(data, word);
    if (*start)
    {
        *end = (*start) + slen;
        if (!is_word_char(&line->data[((*end) - line->data)], FALSE) &&
            (*start == line->data ||
             !is_word_char(&line->data[((*start) - line->data) - 1], FALSE)))
        {}
        else
        {
            *start = NULL;
            *end   = NULL;
        }
    }
}

// Use precomputed hashes as keys
const std::unordered_map<unsigned int, int> &
get_preprossesor_map(void)
{
    static const std::unordered_map<unsigned int, int> hash_map = {
        { hash_string("define"), 1},
        {     hash_string("if"), 2},
        {  hash_string("endif"), 3},
        { hash_string("ifndef"), 4},
        { hash_string("pragma"), 5},
        {  hash_string("ifdef"), 6},
        {   hash_string("else"), 7},
        {hash_string("include"), 8},
        {  hash_string("undef"), 9},
    };
    return hash_map;
}

// Lookup using compile-time hashed string
int
preprossesor_data_from_key(const char *key)
{
    PROFILE_FUNCTION;
    unsigned int hashedKey = hash_string(key);
    const auto  &hashMap   = get_preprossesor_map();
    auto         it        = hashMap.find(hashedKey);
    if (it != hashMap.end())
    {
        return it->second;
    }
    return 0;
}

function_info_t *
func_from_lineno(int lineno)
{
    for (const auto &f : func_info)
    {
        if (lineno >= f->start_bracket && lineno <= f->end_braket)
        {
            return f;
        }
    }
    return NULL;
}
