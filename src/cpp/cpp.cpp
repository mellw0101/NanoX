#include "../include/prototypes.h"

#include "../include/definitions.h"

#include <Mlib/Profile.h>
#include <Mlib/def.h>

/* Returns 'TRUE' if 'c' is a cpp syntax char. */
bool
isCppSyntaxChar(const char c)
{
    return (c == '<' || c == '>' || c == '&' || c == '*' || c == '=' ||
            c == '+' || c == '-' || c == '/' || c == '%' || c == '!' ||
            c == '^' || c == '|' || c == '~' || c == '{' || c == '}' ||
            c == '[' || c == ']' || c == '(' || c == ')' || c == ';' ||
            c == ':' || c == ',' || c == '.' || c == '?' || c == '#');
}

/* Get indent in number of 'tabs', 'spaces', 'total chars', 'total tabs (based
 * on width of tab)'. */
void
get_line_indent(linestruct *line, unsigned short *tabs, unsigned short *spaces,
                unsigned short *t_char, unsigned short *t_tabs)
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

/* If an area is marked then plase the 'str' at mark and current_x, thereby
 * enclosing the marked area.
 * TODO: (enclose_marked_region) - Make the undo one action insted of two
 * seprate once. */
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
    middle->data = (char *)nmalloc(
        strlen(openfile->current->data + openfile->current_x) + extra + 1);
    /* Here we pass the first char as a ref, i.e: A ptr to the first actual
     * char. */
    strcpy(&middle->data[extra], openfile->current->data + openfile->current_x);
    if (openfile->mark == openfile->current &&
        openfile->mark_x > openfile->current_x)
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
    end->data = (char *)nmalloc(
        strlen(openfile->current->data + openfile->current_x) + extra + 1);
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
                    end_pos   = line->lineno;
                    start_pos = -1, end_pos = -1;
                }
            }
            create_bracket_entry(
                line->lineno, get_line_total_tabs(line), is_start);
        }
    }
}

void
do_close_bracket(void)
{
    find_current_function(openfile->current);
}

void
do_test_window(void)
{
    nlog("test win\n");
    if (suggestwin != NULL)
    {
        delwin(suggestwin);
    }
    suggestwin = newwin(20, 20, 0, 0);
}

/* This function extracts info about a function declaration.  It retrieves all
 * the param data as well, like type, name and value.
 * TODO: 'void (*func)()' needs to be fixed. */
function_info_t *
parse_func(const char *str)
{
    int          i, pos;
    char        *copy        = copy_of(str);
    unsigned int len         = strlen(copy);
    char         prefix[256] = "", params[256] = "";
    for (i = 0; i < len && copy[i] != '('; i++);
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
                /* Here we extract any ptr`s that are at the start of the name.
                 */
                for (; prefix[i + 1] == '*'; i++);
                info->name = copy_of(&prefix[i + 1]);
                /* If any ptr`s were found we add them to the return string. */
                if (int diff = i - was_i; diff > 0)
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
    info->return_type = copy_of(prefix);
    int    cap = 10, size = 0;
    char  *param_buf   = params;
    char **param_array = (char **)nmalloc(cap * sizeof(char *));
    for (i = 0, pos = 0; params[i]; i++)
    {
        if (params[i] == ',' || params[i + 1] == '\0')
        {
            (params[i + 1] == '\0') ? (i += 1) : 0;
            (cap == size) ? cap *= 2, param_array = (char **)nrealloc(
                                          param_array, cap * sizeof(char *)) :
                            0;
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
                for (; *end; end++);
                var->type =
                    measured_memmove_copy(param_array[i], (end - start));
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
        for (; *end; end++);
        var->name = measured_memmove_copy(
            param_array[i] + (start - param_array[i]), (end - start));
        var->type =
            measured_memmove_copy(param_array[i], (start - param_array[i]));
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
parse_variable(const char *sig, char **type, char **name, char **value)
{
    char       *copy  = copy_of(sig);
    const char *start = copy;
    const char *end   = start;
    const char *p     = NULL;
    for (; *end && (*end != ';'); end++);
    if (!*end)
    {
        *type  = NULL;
        *name  = NULL;
        *value = NULL;
        free(copy);
        return;
    }
    start = end;
    for (; start > copy && *start != '='; start--);
    if (start > copy)
    {
        p = start;
        p += 1;
        for (; *p && (*p == ' ' || *p == '\t'); p++);
        (value != NULL) ? *value = measured_memmove_copy(p, (end - p)) : NULL;
    }
    else
    {
        *value = NULL;
        start  = end;
    }
    start -= 1;
    for (; start > copy && (*start == ' ' || *start == '\t'); start--);
    end = start;
    for (; start > copy && *start != ' ' && *start != '\t' && *start != '*' &&
           *start != '&';
         start--);
    p = start;
    if (start > copy)
    {
        p += 1;
    }
    (name != NULL) ? *name = measured_memmove_copy(p, (end - p) + 1) : NULL;
    if (!(start > copy))
    {
        *type = NULL;
        free(copy);
        return;
    }
    end = p - 1;
    for (; end > copy && (*end == ' ' || *end == '\t'); end--);
    end += 1;
    for (; start > copy; start--);
    (type != NULL) ? *type = measured_memmove_copy(start, (end - start)) : NULL;
    free(copy);
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
            line->set(BRACKET_END);
            line->unset(BRACKET_START);
            for (linestruct *t_line = line->prev; t_line; t_line = t_line->prev)
            {
                if (t_line->is_set(BRACKET_START))
                {
                    if (line_indent(line) == line_indent(t_line))
                    {
                        if (t_line->prev && t_line->prev->is_set(IN_BRACKET))
                        {
                            line->set(IN_BRACKET);
                        }
                        else
                        {
                            line->unset(IN_BRACKET);
                            find_current_function(line->prev);
                        }
                        break;
                    }
                }
            }
        }
        /* Was not found. */
        else if ((start == NULL && end == NULL) ||
                 (start != NULL && end != NULL))
        {
            if (line->prev && (line->prev->is_set(IN_BRACKET) ||
                               line->prev->is_set(BRACKET_START)))
            {
                line->set(IN_BRACKET);
            }
            else
            {
                line->unset(IN_BRACKET);
            }
        }
    }
}

/* Set comment flags for all lines in current file. */
void
flag_all_block_comments(void)
{
    PROFILE_FUNCTION;
    for (linestruct *line = openfile->filetop; line != NULL; line = line->next)
    {
        const char *found_start = strstr(line->data, "/*");
        const char *found_end   = strstr(line->data, "*/");
        const char *found_slash = strstr(line->data, "//");
        /* First line for a block comment. */
        if (found_start != NULL && found_end == NULL)
        {
            /* If a slash comment is found and it is before the block start,
             * we adjust the start and end pos.  We also make sure to unset
             * 'BLOCK_COMMENT_START' for the line. */
            if (found_slash != NULL && found_slash < found_start)
            {
                line->unset(BLOCK_COMMENT_START);
            }
            else
            {
                line->set(BLOCK_COMMENT_START);
            }
            line->unset(SINGLE_LINE_BLOCK_COMMENT);
            line->unset(IN_BLOCK_COMMENT);
            line->unset(BLOCK_COMMENT_END);
        }
        /* Either inside of a block comment or not a block comment at all. */
        else if (found_start == NULL && found_end == NULL)
        {
            if (line->prev &&
                (line->prev->is_set(IN_BLOCK_COMMENT) ||
                 line->prev->is_set(BLOCK_COMMENT_START)) &&
                !line->prev->is_set(SINGLE_LINE_BLOCK_COMMENT))
            {
                line->set(IN_BLOCK_COMMENT);
                line->unset(BLOCK_COMMENT_START);
                line->unset(BLOCK_COMMENT_END);
                line->unset(SINGLE_LINE_BLOCK_COMMENT);
            }
            /* If the prev line is not in a block comment or the
             * start block line we are not inside a comment block. */
            else
            {
                line->unset(IN_BLOCK_COMMENT);
                line->unset(BLOCK_COMMENT_START);
                line->unset(BLOCK_COMMENT_END);
                line->unset(SINGLE_LINE_BLOCK_COMMENT);
            }
        }
        /* End of a block comment. */
        else if (found_start == NULL && found_end != NULL)
        {
            /* If last line is in a comment block or is the start of the block.
             */
            if (line->prev &&
                (line->prev->is_set(IN_BLOCK_COMMENT) ||
                 line->prev->is_set(BLOCK_COMMENT_START)) &&
                !line->prev->is_set(SINGLE_LINE_BLOCK_COMMENT))
            {
                line->set(BLOCK_COMMENT_END);
                line->unset(IN_BLOCK_COMMENT);
                line->unset(BLOCK_COMMENT_START);
                line->unset(SINGLE_LINE_BLOCK_COMMENT);
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
        if (l->lineno + 2 >= func_info[i]->start_bracket &&
            l->lineno <= func_info[i]->end_braket)
        {
            free_function_info(func_info[i], i);
            break;
        }
    }
    /* If not we add it. */
    for (linestruct *line = l; line; line = line->prev)
    {
        if (line->is_set(BRACKET_START) && !line->is_set(IN_BRACKET))
        {
            if (line->prev == NULL)
            {
                return;
            }
            char *func_sig = parse_function_sig(line);
            if (func_sig == NULL)
            {
                return;
            }
            line                  = line->prev;
            function_info_t *info = parse_func(func_sig);
            if (info)
            {
                info->start_bracket = line->next->lineno;
                for (linestruct *l = line->next; l != NULL; l = l->next)
                {
                    if (l->is_set(BRACKET_END) && !l->is_set(IN_BRACKET))
                    {
                        info->end_braket = l->lineno;
                        break;
                    }
                }
                for (int i = 0; i < func_info.size(); i++)
                {
                    if (func_info[i]->name != NULL)
                    {
                        if (strcmp(func_info[i]->name, info->name) == 0)
                        {
                            free_function_info(func_info[i], i);
                            break;
                        }
                    }
                }
                color_map[info->name] = FG_VS_CODE_BRIGHT_YELLOW;
                func_info.push_back(info);
                refresh_needed = TRUE;
            }
            free(func_sig);
            break;
        }
        if (!line->is_set(IN_BRACKET))
        {
            break;
        }
    }
}
