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
        if (strncmp(d, suggest_buf, suggest_len) == 0)
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

/* Parse a function declaration that is over multiple lines. */
char *
parse_split_decl(linestruct *line)
{
    char       *data = NULL;
    const char *p    = strchr(line->data, ')');
    if (!p)
    {
        return NULL;
    }
    line            = line->prev;
    char *cur_data  = copy_of(line->data);
    char *next_data = copy_of(line->next->data);
    p               = next_data;
    for (; *p && (*p == ' ' || *p == '\t'); p++);
    char *tp = copy_of(p);
    free(next_data);
    next_data = tp;
    append_str(&cur_data, " ");
    data = alloc_str_free_substrs(cur_data, next_data);
    if (!line->prev->data[0])
    {
        free(data);
        return NULL;
    }
    char *ret_t = copy_of(line->prev->data);
    append_str(&ret_t, " ");
    char *ret = alloc_str_free_substrs(ret_t, data);
    return ret;
}

/* Return the correct line to start parsing function delc.
 * if '{' is on 'line' then we simply return 'line',
 * else we iterate until we find the first line
 * after '{' line that has text on it. */
linestruct *
get_func_decl_last_line(linestruct *line)
{
    const char *p = strchr(line->data, '{');
    if (p && (p == line->data || *p == line->data[indent_char_len(line)]))
    {
        do {
            line = line->prev;
        }
        while (!line->data[indent_char_len(line)]);
    }
    return line;
}

/* Parse function signature. */
char *
parse_function_sig(linestruct *line)
{
    const char *p           = NULL;
    const char *param_start = NULL;
    /* If the bracket is alone on a line then go to prev line. */
    line = get_func_decl_last_line(line);
    /* If the line does not contain '(', so it must be a split decl. */
    param_start = strchr(line->data, '(');
    if (!param_start)
    {
        return parse_split_decl(line);
    }
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
             (!is_word_char(&line->data[((*start) - line->data) - 1], FALSE) &&
              line->data[((*start) - line->data) - 1] != '_')))
        {}
        else
        {
            *start = NULL;
        }
    }
    else
    {
        *end = NULL;
    }
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

bool
func_info_exists(const char *sig)
{
    for (const auto &f : func_info)
    {
        if (strcmp(f->full_function, sig) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}
