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
            nlog("full_func: %s\n", i->full_function);
            nlog("return_type: %s\n", i->return_type);
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
        /*
        if (suggest_len < 1)
        {
            clear_suggestion();
            const unsigned long pos = word_index(TRUE);
            for (int i = pos; i < openfile->current_x; suggest_len++, i++)
            {
                suggest_buf[suggest_len] = openfile->current->data[i];
            }
            suggest_buf[suggest_len] = '\0';
        }
        else
        {
        } */
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
    unsigned long col_pos = openfile->current_x + margin - suggest_len;
    suggestwin            = newwin(row_len, col_len, row_pos, col_pos);
    mvwprintw(suggestwin, 0, 1, "%s", suggest_str);
    wrefresh(suggestwin);
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
    }
    param_start = strchr(line->data, '(');
    p           = strchr(line->data, ' ');
    char *sig   = NULL;
    char *ret_t = NULL;
    char *ret   = NULL;
    if (p && p < param_start)
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
