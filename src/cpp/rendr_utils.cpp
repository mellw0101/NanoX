#include "../include/prototypes.h"

void
get_next_word(const char **start, const char **end)
{
    adv_ptr((*end), (*(*end) == ' ' || *(*end) == '\t'));
    *start = *end;
    adv_ptr((*end), (*(*end) != ' ' && *(*end) != '\t'));
}

void
find_suggestion(void)
{
    PROFILE_FUNCTION;
    for (const auto &t : types)
    {
        if (strncmp(t, suggest_buf, suggest_len) == 0)
        {
            suggest_str = (char *)t;
            return;
        }
    }
    for (const auto &i : func_info)
    {
        if (strncmp(i->full_function + strlen(i->return_type) + 1, suggest_buf, suggest_len) == 0)
        {
            suggest_str = i->full_function + strlen(i->return_type) + 1;
            nlog("full_func: %s\n", i->full_function);
            nlog("return_type: %s\n", i->return_type);
            return;
        }
        if (openfile->current->lineno >= i->start_bracket && openfile->current->lineno <= i->end_braket)
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

void
clear_suggestion(void)
{
    suggest_on               = FALSE;
    suggest_str              = NULL;
    suggest_len              = 0;
    suggest_buf[suggest_len] = '\0';
}

void
add_char_to_suggest_buf(void)
{
    suggest_buf[suggest_len++] = openfile->current->data[openfile->current_x - 1];
    suggest_buf[suggest_len]   = '\0';
}
