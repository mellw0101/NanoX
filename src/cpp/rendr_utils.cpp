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
    for (auto &it : var_vector)
    {
        if (openfile->current->lineno >= it.decl_line && openfile->current->lineno <= it.scope_end)
        {
            if (strncmp(it.name.c_str(), suggest_buf, suggest_len) == 0)
            {
                suggest_str = &it.name[0];
                return;
            }
        }
    }
    for (auto &ci : class_info_vector)
    {
        if (strncmp(ci.name.c_str(), suggest_buf, suggest_len) == 0)
        {
            suggest_str = &ci.name[0];
            return;
        }
    }
    for (const auto &i : local_funcs)
    {
        if (strncmp(i.full_function + strlen(i.return_type) + 1, suggest_buf, suggest_len) == 0)
        {
            suggest_str = i.full_function + strlen(i.return_type) + 1;
            return;
        }
        if (openfile->current->lineno >= i.start_bracket && openfile->current->lineno <= i.end_braket)
        {
            for (variable_t *var = i.params; var != NULL; var = var->prev)
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
    unsigned long row_pos =
        (openfile->cursor_row > editwinrows - 2) ? openfile->cursor_row - row_len : openfile->cursor_row + 1;
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
    for (; *p && (*p == ' ' || *p == '\t'); p++)
        ;
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
        do
        {
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
            for (; *p && (*p == ' ' || *p == '\t'); p++)
                ;
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
find_word(linestruct *line, const char *data, const char *word, const unsigned long slen, const char **start,
          const char **end)
{
    *start = strstr(data, word);
    if (*start)
    {
        *end = (*start) + slen;
        if (!is_word_char(&line->data[((*end) - line->data)], FALSE) &&
            (*start == line->data || (!is_word_char(&line->data[((*start) - line->data) - 1], FALSE) &&
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

void
free_local_var(local_var_t *var)
{
    if (var->type)
    {
        free(var->type);
    }
    if (var->name)
    {
        free(var->name);
    }
    if (var->value)
    {
        free(var->value);
    }
}

local_var_t
parse_local_var(linestruct *line)
{
    local_var_t var;
    var.type         = NULL;
    var.name         = NULL;
    var.value        = NULL;
    const char *end  = NULL;
    const char *data = NULL;
    data             = &line->data[indent_char_len(line)];
    end              = strchr(data, ';');
    if (end)
    {
        char       *str     = measured_copy(&line->data[indent_char_len(line)], (end - data) + 1);
        const char *bracket = strchr(str, '{');
        if (bracket)
        {
            free(str);
            return var;
        }
        char *type, *name, *value;
        parse_variable(str, &type, &name, &value);
        if (type)
        {
            var.type = copy_of(type);
            free(type);
        }
        if (name)
        {
            var.name = copy_of(name);
            free(name);
        }
        if (value)
        {
            var.value = copy_of(value);
            free(value);
        }
        if (!(!type && !name && !value))
        {
            int lvl       = 0;
            var.decl_line = line->lineno;
            for (linestruct *l = line; l; l = l->next)
            {
                if ((l->flags.is_set(BRACKET_START)))
                {
                    lvl += 1;
                }
                if ((l->flags.is_set(BRACKET_END)))
                {
                    if (lvl == 0)
                    {
                        var.scope_end = l->lineno;
                        break;
                    }
                    lvl -= 1;
                }
            }
        }
        free(str);
    }
    return var;
}

int
find_class_end_line(linestruct *from)
{
    int         lvl     = 0;
    const char *b_start = NULL;
    const char *b_end   = NULL;
    for (linestruct *line = from; line; line = line->next)
    {
        b_start = line->data;
        do
        {
            b_start = strchr(b_start, '{');
            if (b_start)
            {
                if (!(line->data[(b_start - line->data) - 1] == '\'' && line->data[(b_start - line->data) + 1] == '\''))
                {
                    lvl += 1;
                }
                b_start += 1;
            }
        }
        while (b_start);
        b_end = line->data;
        do
        {
            b_end = strchr(b_end, '}');
            if (b_end)
            {
                if (!(line->data[(b_end - line->data) - 1] == '\'' && line->data[(b_end - line->data) + 1] == '\''))
                {
                    if (lvl == 0)
                    {
                        break;
                    }
                    lvl -= 1;
                }
                b_end += 1;
            }
        }
        while (b_end);
        if (strchr(line->data, ';') && lvl == 0)
        {
            return line->lineno;
        }
    }
    return -1;
}

/* Add entry to color map and remove any entry that has the same name. */
void
add_rm_color_map(string str, syntax_data_t data)
{
    auto it = test_map.find(str);
    if (it != test_map.end() && it->second.color == data.color && it->second.type == data.type)
    {
        test_map.erase(str);
    }
    test_map[str] = data;
}
