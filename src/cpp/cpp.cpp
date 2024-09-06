#include "../include/prototypes.h"

#include "../include/definitions.h"

#include <Mlib/Profile.h>
#include <Mlib/def.h>

/* Returns 'TRUE' if 'c' is a cpp syntax char. */
bool
isCppSyntaxChar(const char c)
{
    return (c == '<' || c == '>' || c == '&' || c == '*' || c == '=' || c == '+' || c == '-' || c == '/' ||
            c == '%' || c == '!' || c == '^' || c == '|' || c == '~' || c == '{' || c == '}' || c == '[' ||
            c == ']' || c == '(' || c == ')' || c == ';' || c == ':' || c == ',' || c == '.' || c == '?' ||
            c == '#');
}

/* Get indent in number of 'tabs', 'spaces', 'total chars', 'total tabs (based on width of tab)'. */
void
get_line_indent(linestruct *line, unsigned short *tabs, unsigned short *spaces, unsigned short *t_char,
                unsigned short *t_tabs)
{
    unsigned long i;
    *tabs = 0, *spaces = 0, *t_char = 0, *t_tabs = 0;
    if (line->data[0] != ' ' && line->data[0] != '\t')
    {
        return;
    }
    for (i = 0; line->data[i]; i++)
    {
        if (line->data[i] == ' ')
        {
            (*spaces)++;
        }
        else if (line->data[i] == '\t')
        {
            (*tabs)++;
        }
        else
        {
            break;
        }
    }
    *t_char = *tabs + *spaces;
    *t_tabs = *tabs;
    if (*spaces > 0)
    {
        *t_tabs += *spaces / WIDTH_OF_TAB;
    }
}

/* Return the len of indent in terms of index off first non 'tab/space' char. */
unsigned short
indent_char_len(linestruct *line)
{
    unsigned short i = 0;
    for (; line->data[i]; i++)
    {
        if (line->data[i] != ' ' && line->data[i] != '\t')
        {
            break;
        }
    }
    return i;
}

/* If an area is marked then plase the 'str' at mark and current_x, thereby enclosing the marked area.
 * TODO: (enclose_marked_region) - Make the undo one action insted of two seprate once. */
void
enclose_marked_region(const char *s1, const char *s2)
{
    /* Sanity check, Returns is there is no mark */
    if (openfile->mark == NULL)
    {
        return;
    }
    linestruct   *was_current = openfile->current, *top, *bot;
    unsigned long top_x, bot_x;
    get_region(&top, &top_x, &bot, &bot_x);
    openfile->current = top;
    add_undo(REPLACE, NULL);
    inject_in_line(&top, s1, top_x);
    /* If top and bot is equal, move mark to the right by 's1' len. */
    (top == bot) ? (bot_x += strlen(s1)) : 0;
    openfile->current = bot;
    add_undo(REPLACE, NULL);
    inject_in_line(&bot, s2, bot_x);
    openfile->mark_x++;
    /* If top and bot is the same, move cursor one step right. */
    (top == bot) ? do_right() : void();
    openfile->current = was_current;
    set_modified();
}

/* This is a shortcut to make marked area a block comment. */
void
do_block_comment(void)
{
    enclose_marked_region("/* ", " */");
    refresh_needed = TRUE;
}

/* Check if enter is requested when betweeen '{' and '}'.
 * if so properly open the brackets, place cursor in the middle and indent once.
 * Return`s 'false' if not between them, otherwise return`s 'TRUE' */
bool
enter_with_bracket(void)
{
    char          c, c_prev;
    linestruct   *was_current = openfile->current, *middle, *end;
    bool          allblanks   = FALSE;
    unsigned long extra;
    if (!openfile->current->data[openfile->current_x - 1])
    {
        return FALSE;
    }
    c_prev = openfile->current->data[openfile->current_x - 1];
    c      = openfile->current->data[openfile->current_x];
    if (c_prev != '{' || c != '}')
    {
        return FALSE;
    }
    extra = indent_length(was_current->data);
    if (extra == openfile->current_x)
    {
        allblanks = (indent_length(openfile->current->data) == extra);
    }
    middle       = make_new_node(openfile->current);
    middle->data = (char *)nmalloc(strlen(openfile->current->data + openfile->current_x) + extra + 1);
    /* Here we pass the first char as a ref, i.e: A ptr to the first actual char. */
    strcpy(&middle->data[extra], openfile->current->data + openfile->current_x);
    if (openfile->mark == openfile->current && openfile->mark_x > openfile->current_x)
    {
        openfile->mark = middle;
        openfile->mark_x += extra - openfile->current_x;
    }
    strncpy(middle->data, was_current->data, extra);
    if (allblanks)
    {
        openfile->current_x = 0;
    }
    openfile->current->data[openfile->current_x] = '\0';
    add_undo(ENTER, NULL);
    splice_node(openfile->current, middle);
    renumber_from(middle);
    openfile->current     = middle;
    openfile->current_x   = extra;
    openfile->placewewant = xplustabs();
    openfile->totsize++;
    set_modified();
    if (ISSET(AUTOINDENT) && !allblanks)
    {
        openfile->totsize += extra;
    }
    update_undo(ENTER);
    /* End of 'middle' and start of 'end' */
    end       = make_new_node(openfile->current);
    end->data = (char *)nmalloc(strlen(openfile->current->data + openfile->current_x) + extra + 1);
    strcpy(&end->data[extra], openfile->current->data + openfile->current_x);
    strncpy(end->data, was_current->data, extra);
    openfile->current->data[openfile->current_x] = '\0';
    add_undo(ENTER, NULL);
    splice_node(openfile->current, end);
    renumber_from(end);
    openfile->current     = end;
    openfile->current_x   = extra;
    openfile->placewewant = xplustabs();
    openfile->totsize++;
    if (ISSET(AUTOINDENT) && !allblanks)
    {
        openfile->totsize += extra;
    }
    update_undo(ENTER);
    /* Place cursor at correct pos. */
    do_up();
    do_tab();
    refresh_needed = TRUE;
    focusing       = FALSE;
    return TRUE;
}

void
add_bracket_pair(const unsigned long start, const unsigned long end)
{
    bracket_pair bp;
    bp.start_line = start;
    bp.end_line   = end;
    bracket_pairs.push_back(bp);
}

void
all_brackets_pos(void)
{
    linestruct *line;
    long        start_pos = -1, end_pos = -1;
    bool        is_start;
    for (line = openfile->filetop; line != NULL; line = line->next)
    {
        if (is_line_start_end_bracket(line, &is_start))
        {
            if (is_start == TRUE)
            {
                start_pos = line->lineno;
            }
            else
            {
                if (start_pos != -1)
                {
                    end_pos = line->lineno;
                    add_bracket_pair(start_pos, end_pos);
                    start_pos = -1, end_pos = -1;
                }
            }
            create_bracket_entry(line->lineno, get_line_total_tabs(line), is_start);
        }
    }
    // for (const auto &be : bracket_entrys)
    // {
    //     netlog_bracket_entry(be);
    // }
}

void
get_bracket_start_end(unsigned long lineno, unsigned long *start, unsigned long *end)
{
    *start = 0, *end = 0;
    for (const auto &[s, e] : bracket_pairs)
    {
        if (s < lineno && e > lineno)
        {
            *start = s, *end = e;
            return;
        }
        if (s > lineno && e > lineno)
        {
            return;
        }
    }
}

void
do_close_bracket(void)
{
    /* unsigned long start_line, end_line;
    linestruct   *line, *was_current;
    get_bracket_start_end(openfile->current->lineno, &start_line, &end_line);
    if (start_line != 0 && end_line != 0)
    {
        NETLOGGER.log("is inside bracket, start: %lu, end: %lu\n", start_line, end_line);
        line        = line_from_number(start_line + 1);
        was_current = line;
        for (; line->lineno != end_line; line = line->next)
        {
            NETLOGGER.log("%s\n", line->data);
            line->hidden = TRUE;
        }
        edit_redraw(was_current, FLOWING);
        // draw_row(openfile->current->lineno, "", openfile->current, 0);
    } */
    /* char *word = retrieve_word_from_cursor_pos(TRUE);
    if (word)
    {
        NETLOGGER.log("%s\n", word);
        sub_thread_delete_c_syntax(word);
        refresh_needed = TRUE;
    } */
    /* int         row  = 0;
    linestruct *line = openfile->edittop;
    while (row < editwinrows && line != NULL)
    {
        unsigned long from_col =
            get_page_start(wideness(line->data, (line == openfile->current) ? openfile->current_x : 0));
        char *converted = display_string(line->data, from_col, editwincols, TRUE, FALSE);
        apply_syntax_to_line(row, converted, line, from_col);
        free(converted); */
    /* if (line->data[0] != '\0')
    {
        unsigned long nwords;
        char        **words = fast_words_from_str(line->data, strlen(line->data), &nwords);
        if (nwords)
        {
            for (int i = 0; i < nwords; i++)
            {
                NETLOGGER.log("%s ", words[i]);
                free(words[i]);
            }
            NETLOGGER.log("\n");
            free(words);
        }
    } */
    /*   row++;
      line = line->next;
    } */
    // submit_search_task("/home/mellw/Downloads/74307.txt.utf-8" /* "/usr/include/stdio.h" */);
    LOG_FLAG(openfile->current, BRACKET_START);
    LOG_FLAG(openfile->current, IN_BRACKET);
    LOG_FLAG(openfile->current, BRACKET_END);
    find_current_function(openfile->current);
    NLOG("tabs: %zu\n", total_tabs(openfile->current));
}

void
do_test_window(void)
{
    NETLOGGER.log("test win\n");
    if (test_win != NULL)
    {
        delwin(test_win);
    }
    test_win = newwin(20, 20, 0, 0);
}

function_info_t *
parse_function(const char *str)
{
    PROFILE_FUNCTION;
    function_info_t *info      = (function_info_t *)nmalloc(sizeof(*info));
    info->full_function        = copy_of(str);
    info->name                 = NULL;
    info->return_type          = NULL;
    info->params               = NULL;
    info->number_of_params     = 0;
    info->attributes           = NULL;
    info->number_of_attributes = 0;
    char *copy                 = copy_of(str);
    char *rest                 = copy;
    char  buf[1024]            = "";
    char *pos                  = NULL;
    char *token                = strtok_r(rest, " ", &rest);
    /* We want to find the first mention of '(' char indecating that we have gone thrue the prefix. */
    while (token != NULL)
    {
        if ((pos = strstr(token, "(")) == NULL)
        {
            strcat(buf, token);
            strcat(buf, " ");
        }
        else
        {
            if (*token == '(')
            {
                break;
            }
            else
            {
                int  i = 0;
                char name[256];
                for (; token[i] && token[i] != '('; (name[i] = token[i]), i++)
                    ;
                name[i] = '\0';
                for (int i = 0;; i++)
                {
                    if (buf[i] == '\0')
                    {
                        int index = 0;
                        while (name[index] != '\0')
                        {
                            buf[i++] = name[index++];
                        }
                        buf[i] = '\0';
                        break;
                    }
                }
                break;
            }
        }
        token = strtok_r(rest, " ", &rest);
    }
    /* Quick fix for now, this is so the loop bellow can always find return type. */
    if (buf[0] != ' ')
    {
        char tbuf[256] = "  ";
        strcat(tbuf, buf);
        buf[0] = '\0';
        strcat(buf, tbuf);
    }
    /* Now the buffer contains all text before '(' char. */
    const int slen  = strlen(buf);
    int       words = 0;
    for (int i = slen; i > 0; --i)
    {
        if (buf[i] == ' ' && i != slen - 1)
        {
            if (words++ == 0)
            {
                int was_i = i;
                /* Here we extract any ptr`s that are at the start of the name. */
                for (; buf[i + 1] == '*'; i++)
                    ;
                info->name = copy_of(&buf[i + 1]);
                /* If any ptr`s were found we add them to the return string. */
                if (int diff = i - was_i; diff != 0)
                {
                    for (; diff > 0; diff--, was_i++)
                    {
                        buf[was_i] = '*';
                    }
                }
                buf[was_i] = '\0';
            }
            else
            {
                info->return_type = copy_of(&buf[i + 1]);
            }
        }
    }
    /* The params. */
    if (token != NULL)
    {
        int i = 0;
        for (; token[i] && token[i - 1] != '('; i++)
            ;
        if (token[i] != '\0')
        {
            token += i;
        }
        buf[0] = '\0';
        strcat(buf, token);
        strcat(buf, " ");
        for (i = 0; rest[i] && rest[i] != ')'; i++)
            ;
        rest[i] = '\0';
        strcat(buf, rest);
        rest += i + 1;
        unsigned long number_of_params;
        char        **params = delim_str(buf, ",", &number_of_params);
        for (i = 0; i < number_of_params; i++)
        {
            if (*params[i] == ' ')
            {
                char *param = copy_of(params[i] + 1);
                free(params[i]);
                params[i] = param;
            }
        }
        /* info->params           = params; */
        info->number_of_params = number_of_params;
        /* Add attributes later.  Thay are in the rest ptr.
        NLOG("attributes: %s\n", rest); */
    }
    free(copy);
    return info;
}

/* This function extracts info about a function declaration.  It retrieves all the
 * param data as well, like type, name and value. */
function_info_t *
parse_func(const char *str)
{
    int   i, pos;
    char *copy        = copy_of(str);
    char  prefix[256] = "", params[256] = "";
    for (i = 0; copy[i] && copy[i] != '('; i++)
        ;
    if (copy[i] != '(')
    {
        free(copy);
        return NULL;
    }
    pos = i;
    for (i = 0; i < pos; i++)
    {
        prefix[i] = copy[i];
    }
    prefix[i] = '\0';
    pos += 1;
    for (i = 0; copy[pos + i] && copy[pos + i] != ')'; i++)
    {
        params[i] = copy[pos + i];
    }
    if (copy[pos + i] != ')')
    {
        free(copy);
        return NULL;
    }
    params[i]                  = '\0';
    function_info_t *info      = (function_info_t *)nmalloc(sizeof(*info));
    info->full_function        = copy_of(str);
    info->name                 = NULL;
    info->return_type          = NULL;
    info->params               = NULL;
    info->number_of_params     = 0;
    info->attributes           = NULL;
    info->number_of_attributes = 0;
    /* Now the buffer contains all text before the '(' char. */
    const int slen  = strlen(prefix);
    int       words = 0;
    for (int i = slen; i > 0; --i)
    {
        if (prefix[i] == ' ' && i != slen - 1)
        {
            if (words++ == 0)
            {
                int was_i = i;
                /* Here we extract any ptr`s that are at the start of the name. */
                for (; prefix[i + 1] == '*'; i++)
                    ;
                info->name = copy_of(&prefix[i + 1]);
                /* If any ptr`s were found we add them to the return string. */
                if (int diff = i - was_i; diff != 0)
                {
                    for (; diff > 0; diff--, was_i++)
                    {
                        prefix[was_i] = '*';
                    }
                }
                prefix[was_i] = '\0';
                i             = was_i;
                break;
            }
        }
    }
    info->return_type = measured_memmove_copy(prefix, i);
    int    cap = 10, size = 0;
    char  *param_buf   = params;
    char **param_array = (char **)nmalloc(cap * sizeof(char *));
    for (i = 0, pos = 0; params[i]; i++)
    {
        if (params[i] == ',' || params[i + 1] == '\0')
        {
            (params[i + 1] == '\0') ? (i += 1) : 0;
            (cap == size) ? cap *= 2, param_array = (char **)nrealloc(param_array, cap * sizeof(char *)) : 0;
            param_array[size++] = measured_memmove_copy(param_buf, i - pos);
            (params[i + 1] == ' ') ? (i += 2) : (i += 1);
            param_buf += i - pos;
            pos = i;
        }
    }
    param_array[size]      = NULL;
    info->number_of_params = size;
    for (i = 0; i < size; i++)
    {
        variable_t *var   = (variable_t *)nmalloc(sizeof(*var));
        var->name         = NULL;
        var->value        = NULL;
        var->next         = NULL;
        const char *start = param_array[i];
        const char *end   = param_array[i];
        end               = strrchr(param_array[i], '*');
        if (end == NULL)
        {
            end = strrchr(param_array[i], ' ');
            if (end == NULL)
            {
                end = start;
                for (; *end; end++)
                    ;
                var->type = measured_memmove_copy(param_array[i], (end - start));
                if (info->params == NULL)
                {
                    var->prev    = NULL;
                    info->params = var;
                }
                else
                {
                    var->prev          = info->params;
                    info->params->next = var;
                    info->params       = info->params->next;
                }
                continue;
            }
        }
        end += 1;
        start = end;
        for (; *end; end++)
            ;
        var->name = measured_memmove_copy(param_array[i] + (start - param_array[i]), (end - start));
        var->type = measured_memmove_copy(param_array[i], (start - param_array[i]));
        if (info->params == NULL)
        {
            var->prev    = NULL;
            info->params = var;
        }
        else
        {
            var->prev          = info->params;
            info->params->next = var;
            info->params       = info->params->next;
        }
    }
    for (i = 0; i < size; i++)
    {
        free(param_array[i]);
    }
    free(param_array);
    free(copy);
    return info;
}

void
free_function_info(function_info_t *info, const int index)
{
    variable_t *var = info->params;
    while (var != NULL)
    {
        info->params = info->params->prev;
        (var->name != NULL) ? free(var->name) : void();
        (var->type != NULL) ? free(var->type) : void();
        (var->value != NULL) ? free(var->value) : void();
        free(var);
        var = info->params;
    }
    (info->params != NULL) ? free(info->params) : void();
    (info->full_function != NULL) ? free(info->full_function) : void();
    (info->name != NULL) ? free(info->name) : void();
    (info->return_type != NULL) ? free(info->return_type) : void();
    free(info);
    if (index > -1)
    {
        func_info.erase(func_info.begin() + index);
    }
}

void
flag_all_brackets(void)
{
    PROFILE_FUNCTION;
    for (linestruct *line = openfile->filetop; line != NULL; line = line->next)
    {
        const char *start = strchr(line->data, '{');
        const char *end   = strrchr(line->data, '}');
        /* Bracket start and end on the same line. */
        /* if (start != NULL && end != NULL)
        {
            while (start != NULL)
            {
                if (line->data[(start - line->data) + 1] == '\0')
                {
                    start = NULL;
                    continue;
                }
                start = strchr(start + 1, '{');
            }
            while (end != NULL)
            {
                unsigned int last_pos = last_strchr(line->data, '}', (end - line->data));
                if (last_pos < till_x && last_pos != 0)
                {
                    end = &line->data[last_pos];
                }
                else
                {
                    end = NULL;
                }
            }
        } */
        /* Start bracket line was found. */
        if (start != NULL && end == NULL)
        {
            LINE_SET(line, BRACKET_START);
            if (line->prev && (LINE_ISSET(line->prev, IN_BRACKET)))
            {
                LINE_SET(line, IN_BRACKET);
            }
        }
        /* End bracket line was found. */
        else if (start == NULL && end != NULL)
        {
            LINE_SET(line, BRACKET_END);
            LINE_UNSET(line, BRACKET_START);
            for (linestruct *t_line = line->prev; t_line; t_line = t_line->prev)
            {
                if (LINE_ISSET(t_line, BRACKET_START))
                {
                    if (indent_char_len(line) == indent_char_len(t_line))
                    {
                        if (t_line->prev && LINE_ISSET(t_line->prev, IN_BRACKET))
                        {

                            LINE_SET(line, IN_BRACKET);
                        }
                        else
                        {
                            LINE_UNSET(line, IN_BRACKET);
                            find_current_function(line->prev);
                        }
                        break;
                    }
                }
            }
        }
        /* Was not found. */
        else if ((start == NULL && end == NULL) || (start != NULL && end != NULL))
        {
            if (line->prev && (LINE_ISSET(line->prev, IN_BRACKET) || LINE_ISSET(line->prev, BRACKET_START)))
            {
                LINE_SET(line, IN_BRACKET);
            }
            else
            {
                LINE_UNSET(line, IN_BRACKET);
            }
        }
    }
}

void
find_current_function(linestruct *l)
{
    PROFILE_FUNCTION;
    for (int i = 0; i < func_info.size(); i++)
    {
        if (l->lineno + 2 >= func_info[i]->start_bracket && l->lineno <= func_info[i]->end_braket)
        {
            free_function_info(func_info[i], i);
            break;
        }
    }
    char        buf[4096];
    const char *function_str = NULL;
    /* If not we add it. */
    for (linestruct *line = l; line; line = line->prev)
    {
        if (LINE_ISSET(line, BRACKET_START) && !LINE_ISSET(line, IN_BRACKET))
        {
            if (line->prev == NULL)
            {
                return;
            }
            line         = line->prev;
            function_str = strchr(line->data, '(');
            if (function_str != NULL)
            {
                memset(buf, 0, sizeof(buf));
                unsigned long len       = 0;
                const char   *same_line = strchr(line->data, ' ');
                if (same_line != NULL && (same_line - line->data) < (function_str - line->data))
                {
                    len = strlen(line->data);
                    memcpy(buf, line->data, len + 1);
                    buf[len] = '\0';
                }
                else
                {
                    if (line->prev->data[0] != '\0')
                    {
                        len = strlen(line->prev->data);
                        memcpy(buf, line->prev->data, len);
                        memcpy(buf + len, " ", 1);
                    }
                    len += 1;
                    unsigned long len_2 = strlen(line->data);
                    memcpy(buf + len, line->data, len_2);
                    buf[len + len_2] = '\0';
                }
                function_info_t *info = parse_func(buf);
                if (info)
                {
                    info->start_bracket = line->next->lineno;
                    for (linestruct *l = line->next; l != NULL; l = l->next)
                    {
                        if (LINE_ISSET(l, BRACKET_END) && !LINE_ISSET(l, IN_BRACKET))
                        {
                            info->end_braket = l->lineno;
                            break;
                        }
                    }
                    bool found = FALSE;
                    for (const auto &i : func_info)
                    {
                        if (i->name != NULL)
                        {
                            if (strcmp(i->name, info->name) == 0)
                            {
                                found = TRUE;
                                break;
                            }
                        }
                    }
                    if (found == FALSE)
                    {
                        func_info.push_back(info);
                        refresh_needed = TRUE;
                    }
                    else
                    {
                        free_function_info(info);
                    }
                }
            }
            break;
        }
        if (!LINE_ISSET(line, IN_BRACKET))
        {
            break;
        }
    }
}
