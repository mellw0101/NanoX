#include "../include/prototypes.h"

#include <Mlib/Profile.h>

static unsigned int block_comment_start = (unsigned int)-1;
static unsigned int block_comment_end   = (unsigned int)-1;
static int          color_bi[3]         = {
    FG_VS_CODE_YELLOW, FG_VS_CODE_BRIGHT_MAGENTA, FG_VS_CODE_BRIGHT_BLUE};

vec<char *>          includes;
vec<char *>          defines;
vec<char *>          structs;
vec<char *>          classes;
vec<char *>          funcs;
vec<glob_var_t>      glob_vars;
vec<function_info_t> local_funcs;

static int           row       = 0;
static const char   *converted = NULL;
static linestruct   *line      = NULL;
static unsigned long from_col  = 0;

char              suggest_buf[1024] = "";
char             *suggest_str       = NULL;
int               suggest_len       = 0;
vec<const char *> types             = {
    "void", "char",  "int",   "unsigned", "extern",  "volatile", "static",
    "long", "short", "const", "bool",     "typedef", "class",
};

// std::unordered_map<std::string_view, syntax_data_t> color_map;
std::unordered_map<std::string, syntax_data_t> test_map;
vector<class_info_t>                           class_info_vector;
vector<var_t>                                  var_vector;

void
render_part(unsigned long match_start, unsigned long match_end, short color)
{
    const char *thetext   = NULL;
    int         paintlen  = 0;
    int         start_col = 0;
    if ((match_start >= till_x))
    {
        return;
    }
    if (match_start > from_x)
    {
        start_col = wideness(line->data, match_start) - from_col;
    }
    thetext  = converted + actual_x(converted, start_col);
    paintlen = actual_x(
        thetext, wideness(line->data, match_end) - from_col - start_col);
    midwin_mv_add_nstr_color(
        row, (margin + start_col), thetext, paintlen, color);
}

/* Experiment. */
void
render_part_raw(unsigned long start_index, unsigned long end_index, short color)
{
    const char *start = &line->data[start_index];
    const char *end   = start;
    const char *stop  = &line->data[end_index];
    while (*end && end != stop)
    {
        ADV_PTR(end, (end <= stop) && (*end == ' ' || *end == '\t'));
        if (*end == '\0' || end == stop)
        {
            break;
        }
        start = end;
        ADV_PTR(end, (end <= stop) && (*end != ' ' && *end != '\t'));
        end += 1;
        rendr(R, color, start, end);
    }
}

/* Render the text of a given line.  Note that this function only renders the
 * text and nothing else. */
void
render_line_text(const int row, const char *str, linestruct *line,
                 const unsigned long from_col)
{
    if (margin > 0)
    {
        win_color_on(midwin, LINE_NUMBER);
        if (ISSET(SOFTWRAP) && from_col != 0)
        {
            mvwprintw(midwin, row, 0, "%*s", margin - 1, " ");
        }
        else
        {
            mvwprintw(midwin, row, 0, "%*lu", margin - 1, line->lineno);
        }
        win_color_off(midwin, LINE_NUMBER);
        if (line->has_anchor == TRUE && (from_col == 0 || !ISSET(SOFTWRAP)))
        {
            if (using_utf8())
            {
                wprintw(midwin, "\xE2\xAC\xA5");
            }
            else
            {
                wprintw(midwin, "+");
            }
        }
        else
        {
            wprintw(midwin, " ");
        }
    }
    mvwaddstr(midwin, row, margin, str);
    if (is_shorter || ISSET(SOFTWRAP))
    {
        wclrtoeol(midwin);
    }
    if (sidebar)
    {
        mvwaddch(midwin, row, COLS - 1, bardata[row]);
    }
}

/* Set start and end pos for comment block or if the entire
 * line is inside of a block comment set 'block_comment_start'
 * to '0' and 'block_comment_end' to '(unsigned int)-1'. */
void
render_comment(void)
{
    const char *start = strstr(line->data, "/*");
    const char *end   = strstr(line->data, "*/");
    const char *slash = strstr(line->data, "//");
    /* Single line block comment. */
    if (start && end)
    {
        block_comment_start = (start - line->data);
        block_comment_end   = (end - line->data + 2);
        /* If slash comment found, adjust the start and end pos correctly. */
        if (slash != NULL && (slash - line->data) < block_comment_start)
        {
            block_comment_start = (slash - line->data);
            block_comment_end   = (unsigned int)-1;
        }
        LINE_SET(line, SINGLE_LINE_BLOCK_COMMENT);
        LINE_UNSET(line, BLOCK_COMMENT_START);
        LINE_UNSET(line, BLOCK_COMMENT_END);
        LINE_UNSET(line, IN_BLOCK_COMMENT);
        render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
        /* Highlight the block start if prev line is in a
         * block comment or the start of a block comment. */
        if (line->prev && (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) ||
                           LINE_ISSET(line->prev, BLOCK_COMMENT_START)))
        {
            if ((end - line->data) > 0 &&
                line->data[(end - line->data) - 1] != '/')
            {
                midwin_mv_add_nstr_color(
                    row, (wideness(line->data, (start - line->data)) + margin),
                    start, 2, ERROR_MESSAGE);
                /* If there is a slash comment infront the block comment. Then
                 * of cource we still color the text from the slash to the block
                 * start after we error highlight the block start. */
                if (slash != NULL &&
                    (slash - line->data) < (start - line->data))
                {
                    midwin_mv_add_nstr_color(
                        row,
                        (wideness(line->data, (slash - line->data))) + margin,
                        slash, (start - line->data) - (slash - line->data),
                        FG_GREEN);
                }
                block_comment_start += (start - line->data) + 2;
            }
            else if ((start - line->data) + 1 == (end - line->data))
            {
                LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
                LINE_SET(line, BLOCK_COMMENT_END);
                block_comment_start = 0;
            }
        }
        else if ((start - line->data) + 1 == (end - line->data))
        {
            end = strstr(end + 2, "*/");
            if (end != NULL)
            {
                block_comment_end = (end - line->data) + 2;
            }
            else
            {
                block_comment_end = (unsigned int)-1;
                LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
                LINE_SET(line, BLOCK_COMMENT_START);
            }
        }
        while (start != NULL && end != NULL)
        {
            /* TODO: Here we need to fix the issue of multiple block comments on
             * a single line. */
            start = strstr(start + 2, "/*");
            end   = strstr(end + 2, "*/");
            slash = strstr(slash ? slash + 2 : line->data, "//");
            if (start != NULL && end != NULL)
            {
                const unsigned long match_start = (start - line->data);
                const unsigned long match_end   = (end - line->data) + 2;
                render_part(match_start, match_end, FG_GREEN);
            }
            else if (start == NULL && end != NULL)
            {
                midwin_mv_add_nstr_color(
                    row, PTR_POS_LINE(line, end), end, 2, ERROR_MESSAGE);
            }
            else if (start != NULL && end == NULL)
            {
                const unsigned long match_start = (start - line->data);
                render_part(match_start, till_x, FG_GREEN);
            }
        }
    }
    /* First line for a block comment. */
    else if (start && !end)
    {
        block_comment_start = (start - line->data);
        block_comment_end   = till_x;
        render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
        /* Do some error checking and highlight the block start if it`s found
         * while the block above it being a start block or inside a block. */
        if (line->prev && (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) ||
                           LINE_ISSET(line->prev, BLOCK_COMMENT_START)))
        {
            rendr(R_LEN, ERROR_MESSAGE, start, 2);
            block_comment_start = (start - line->data) + 2;
        }
        /* If a slash comment is found and it is before the block start,
         * we adjust the start and end pos.  We also make sure to unset
         * 'BLOCK_COMMENT_START' for the line. */
        if (slash && (slash - line->data) < block_comment_start)
        {
            block_comment_start = (slash - line->data);
            block_comment_end   = till_x;
            render_part(
                block_comment_start, block_comment_end, FG_COMMENT_GREEN);
            LINE_UNSET(line, BLOCK_COMMENT_START);
        }
        else
        {
            LINE_SET(line, BLOCK_COMMENT_START);
        }
        LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
        LINE_UNSET(line, IN_BLOCK_COMMENT);
        LINE_UNSET(line, BLOCK_COMMENT_END);
    }
    /* Either inside of a block comment or not a block comment at all. */
    else if (!start && !end)
    {
        if (line->prev &&
            (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) ||
             LINE_ISSET(line->prev, BLOCK_COMMENT_START)) &&
            !LINE_ISSET(line->prev, SINGLE_LINE_BLOCK_COMMENT))
        {
            block_comment_start = 0;
            block_comment_end   = till_x;
            render_part(
                block_comment_start, block_comment_end, FG_COMMENT_GREEN);
            LINE_SET(line, IN_BLOCK_COMMENT);
            LINE_UNSET(line, BLOCK_COMMENT_START);
            LINE_UNSET(line, BLOCK_COMMENT_END);
            LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
        }
        /* If the prev line is not in a block comment or the
         * start block line we are not inside a comment block. */
        else
        {
            block_comment_start = till_x;
            block_comment_end   = 0;
            LINE_UNSET(line, IN_BLOCK_COMMENT);
            LINE_UNSET(line, BLOCK_COMMENT_START);
            LINE_UNSET(line, BLOCK_COMMENT_END);
            LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
            /* If slash comment is found comment out entire line after slash. */
            if (slash)
            {
                block_comment_start = (slash - line->data);
                block_comment_end   = till_x;
                render_part(
                    block_comment_start, block_comment_end, FG_COMMENT_GREEN);
            }
        }
    }
    /* End of a block comment. */
    else if (!start && end)
    {
        /* If last line is in a comment block or is the start of the block. */
        if (line->prev &&
            (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) ||
             LINE_ISSET(line->prev, BLOCK_COMMENT_START)) &&
            !LINE_ISSET(line->prev, SINGLE_LINE_BLOCK_COMMENT))
        {
            block_comment_start = 0;
            block_comment_end   = (end - line->data) + 2;
            render_part(
                block_comment_start, block_comment_end, FG_COMMENT_GREEN);
            LINE_SET(line, BLOCK_COMMENT_END);
            LINE_UNSET(line, IN_BLOCK_COMMENT);
            LINE_UNSET(line, BLOCK_COMMENT_START);
            LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
        }
        /* If slash if found and is before block end. */
        else if (slash != NULL && (slash - line->data) < (end - line->data))
        {
            block_comment_start = (slash - line->data);
            block_comment_end   = till_x;
            render_part(
                block_comment_start, block_comment_end, FG_COMMENT_GREEN);
        }
        /* If not, error highlight the end block. */
        else
        {
            rendr(R_LEN, ERROR_MESSAGE, end, 2);
        }
    }
    refresh_needed = TRUE;
}

/* Color brackets based on indent.  TODO: This needs to be fix. */
void
render_bracket(void)
{
    const char *start = strchr(line->data, '{');
    const char *end   = strrchr(line->data, '}');
    /* Bracket start and end on the same line. */
    if (start && end)
    {
        while (start)
        {
            rendr(R_CHAR, FG_VS_CODE_YELLOW, start);
            if (line->data[(start - line->data) + 1] == '\0')
            {
                start = NULL;
                continue;
            }
            start = strchr(start + 1, '{');
        }
        while (end)
        {
            rendr(R_CHAR, FG_VS_CODE_YELLOW, end);
            unsigned int last_pos =
                last_strchr(line->data, '}', (end - line->data));
            if (last_pos < till_x && last_pos != 0)
            {
                end = &line->data[last_pos];
            }
            else
            {
                end = NULL;
            }
        }
        if (line->prev && (LINE_ISSET(line->prev, IN_BRACKET) ||
                           LINE_ISSET(line->prev, BRACKET_START)))
        {
            LINE_SET(line, IN_BRACKET);
        }
    }
    /* Start bracket line was found. */
    else if (start && !end)
    {
        LINE_SET(line, BRACKET_START);
        rendr(R_CHAR, color_bi[(line_indent(line) % 3)], start);
        if (line->prev && (LINE_ISSET(line->prev, IN_BRACKET) ||
                           LINE_ISSET(line->prev, BRACKET_START)))
        {
            LINE_SET(line, IN_BRACKET);
        }
        /* TODO: Find better trigger to recheck current func. */
        else
        {
            find_current_function(line->next);
        }
    }
    /* End bracket line was found. */
    else if (!start && end)
    {
        LINE_SET(line, BRACKET_END);
        LINE_UNSET(line, BRACKET_START);
        rendr(R_CHAR, color_bi[(line_indent(line) % 3)], end);
        for (linestruct *t_line = line->prev; t_line; t_line = t_line->prev)
        {
            if (LINE_ISSET(t_line, BRACKET_START))
            {
                if (line_indent(t_line) == line_indent(line))
                {
                    if (t_line->prev && LINE_ISSET(t_line->prev, IN_BRACKET))
                    {
                        LINE_SET(line, IN_BRACKET);
                    }
                    else
                    {
                        LINE_UNSET(line, IN_BRACKET);
                    }
                    break;
                }
            }
        }
    }
    /* Was not found. */
    else if (!start && !end)
    {
        if (line->prev && (LINE_ISSET(line->prev, IN_BRACKET) ||
                           LINE_ISSET(line->prev, BRACKET_START)))
        {
            LINE_SET(line, IN_BRACKET);
        }
        else
        {
            LINE_UNSET(line, IN_BRACKET);
        }
    }
}

void
render_parents(void)
{
    const char *start = strchr(line->data, '(');
    const char *end   = strchr(line->data, ')');
    while (1)
    {
        if (start)
        {
            rendr_ch_str_ptr(
                start,
                color_bi[(wideness(line->data, indent_char_len(line)) % 3)]);
            start = strchr(start + 1, '(');
        }
        if (end)
        {
            rendr_ch_str_ptr(
                end,
                color_bi[(wideness(line->data, indent_char_len(line)) % 3)]);
            end = strchr(end + 1, ')');
        }
        if (!start && !end)
        {
            break;
        }
    }
}

/* This function highlights string literals.  Error handeling is needed. */
void
render_string_literals(void)
{
    const char *start = line->data;
    const char *end   = line->data;
    // const char *slash      = NULL;
    // const char *slash_end  = NULL;
    // const char *format     = NULL;
    // const char *format_end = NULL;
    while (*start)
    {
        ADV_PTR_BY_CH(end, '"');
        if (!*end)
        {
            return;
        }
        if (*end != '"')
        {
            return;
        }
        start                           = end;
        const unsigned long match_start = (start - line->data);
        end += 1;
        ADV_PTR_BY_CH(end, '"');
        if (!*end)
        {
            return;
        }
        if (*end != '"')
        {
            if (match_start <= block_comment_start &&
                match_start >= block_comment_end)
            {
                render_part(match_start, till_x, ERROR_MESSAGE);
            }
            return;
        }
        end += 1;
        const unsigned long match_end = (end - line->data);
        if ((match_start >= till_x) || (match_start >= block_comment_start &&
                                        match_end <= block_comment_end))
        {
            return;
        }
        rendr(C, FG_YELLOW, match_start, match_end);
        /* slash = start + 1;
        while (*slash && *slash != '"')
        {
            ADV_PTR(slash, (*slash != '"') && (*slash != '\\'))
            slash_end = slash + 1;
            if (*slash_end != '\0')
            {
                if (*slash_end == 'n' || *slash_end == 'r' || *slash_end == 'e')
                {
                    slash_end += 1;
                    rendr(R, FG_MAGENTA, slash, slash_end);
                }
                else if (*slash_end == 'x')
                {
                    slash_end += 1;
                    if (*slash_end == '1')
                    {
                        slash_end += 1;
                        if (*slash_end == 'B')
                        {
                            slash_end += 1;
                            rendr(R, FG_MAGENTA, slash, slash_end);
                        }
                    }
                }
            }
            slash = slash_end;
        } */
        /* format = start + 1;
        while (*format && *format != '"')
        {
            ADV_PTR(format, (*format != '"') && (*format != '%'))
            format_end = format + 1;
            if (*format_end != '\0')
            {
                if (*format_end >= '0' && *format_end <= '9')
                {
                    adv_ptr(
                        format_end, (*format_end >= '0' && *format_end <= '9'));
                }
                else if (*format_end == '*')
                {
                    format_end += 1;
                }
                if (*format_end == 'l' || *format_end == 'z' ||
                    *format_end == 'h')
                {
                    format_end += 1;
                }
                if (*format_end == 's' || *format_end == 'S' ||
                    *format_end == 'u' || *format_end == 'U' ||
                    *format_end == 'x' || *format_end == 's' ||
                    *format_end == 'i' || *format_end == 'e' ||
                    *format_end == 'd')
                {
                    format_end += 1;
                    rendr(R, FG_VS_CODE_BRIGHT_CYAN, format, format_end);
                }
            }
            format = format_end;
        } */
        start = end + 1;
    }
}

/* Function to handle char strings inside other strings or just in general. */
void
render_char_strings(void)
{
    const char *start = line->data, *end = line->data;
    while (start != NULL)
    {
        for (; *end && *end != '\''; end++);
        if (*end != '\'')
        {
            return;
        }
        start = end;
        end++;
        for (; *end && *end != '\''; end++);
        if (*end != '\'')
        {
            return;
        }
        end++;
        const unsigned long match_start = (start - line->data);
        const unsigned long match_end   = (end - line->data);
        if (match_start >= block_comment_start &&
            match_end <= block_comment_end)
        {
            return;
        }
        render_part(match_start, match_end, FG_MAGENTA);
        start = end;
    }
}

void
rendr_define(unsigned int index)
{
    const char *start = NULL, *end = NULL, *param = NULL;
    char       *word = NULL;
    start            = &line->data[index];
    if (*start == '\0')
    {
        RENDR(E, "<-(Macro name missing)");
        return;
    }
    end = start;
    ADV_PTR(end, (*end != '(' && *end != ' ' && *end != '\t'));
    RENDR(R, FG_VS_CODE_BLUE, start, end);
    string define_name(start, (end - start));
    if (test_map.find(define_name) == test_map.end())
    {
        test_map[define_name] = {FG_VS_CODE_BLUE};
    }
    /* Handle macro parameter list.  If there is one. */
    if (*end == '(')
    {
        while (*end)
        {
            end += 1;
            /* Advance to the start of the word. */
            ADV_PTR(end, (*end == ' ' || *end == '\t'));
            start = end;
            /* Now find the end of the word. */
            ADV_PTR(end, (*end != ')' && *end != ',' && *end != ' ' &&
                          *end != '\t'));
            RENDR(R, FG_VS_CODE_BRIGHT_CYAN, start, end);
            (word != NULL) ? free(word) : void();
            word  = measured_copy(start, (end - start));
            param = strstr(end, word);
            if (param != NULL)
            {
                while (param != NULL)
                {
                    const char *ps = &line->data[(param - line->data) - 1];
                    const char *pe = &param[(end - start)];
                    if ((*pe == ' ' || *pe == ')' || *pe == ',') &&
                        (*ps == ' ' || *ps == '(' || *ps == ','))
                    {
                        RENDR(R_LEN, FG_VS_CODE_BLUE, param, (end - start));
                    }
                    param = strstr(param + (end - start), word);
                }
            }
            if (*end == ')')
            {
                end += 1;
                break;
            }
            else if (*end == ' ')
            {
                adv_ptr(end, (*end == ' ' || *end == '\t'));
                if (*end == ')')
                {
                    break;
                }
                else if (*end != ',')
                {
                    start = end;
                    adv_ptr(end, (*end != ')' && *end != ',' && *end != ' ' &&
                                  *end != '\t'));
                    rendr(R, ERROR_MESSAGE, start, end);
                    rendr(E, "<-(Expected comma)");
                    if (*end == ')')
                    {
                        break;
                    }
                }
            }
            if (*end != ',')
            {
                end += 1;
            }
        }
    }
    (word != NULL) ? free(word) : void();
    ADV_PTR(end, (*end == ' ' || *end == '\t'));
    start = end;
    ADV_PTR(end, (*end != '(' && *end != ' ' && *end != '\t'));
    if (*end == '(')
    {
        RENDR(R, FG_VS_CODE_BRIGHT_YELLOW, start, end);
    }
    else if (!(*start >= '0' && *start <= '9'))
    {
        RENDR(R, FG_VS_CODE_BLUE, start, end);
    }
}

void
rendr_include(unsigned int index)
{
    const char *start = &line->data[index];
    const char *end   = start;
    if (*start == '"')
    {
        end += 1;
    }
    adv_ptr(end, (*end != '>' && *end != '"'));
    if (*end != '\0')
    {
        if (*start == '<' && *end == '>')
        {
            end += 1;
            rendr(C_PTR, FG_YELLOW, start, end);
            /* TODO: enable later once local file rendering includes all types.
             */
            /*
            char       *file      = measured_copy(start + 1, (end - start) - 2);
            const char *full_path = concat_path("/usr/include/", file);
            for (int i = 0; i < includes.get_size(); i++)
            {
                if (strcmp(includes[i], file) == 0)
                {
                    free(file);
                    return;
                }
            }
            includes.push_back(file);
            get_line_list_task(full_path);
            if (is_file_and_exists(full_path))
            {
                includes.push_back(file);
                check_include_file_syntax(full_path);
            }
            else if (file != NULL)
            {
                free(file);
            } */
        }
        else if (*start == '"' && *end == '"')
        {
            end += 1;
            render_part((start - line->data), (end - line->data), FG_YELLOW);
            /*

            char *path = measured_memmove_copy(start + 1, (end - start) - 2);
            char *pwd  = alloced_full_current_file_dir();
            char *full_path = alloc_str_free_substrs(pwd, path);
            for (const auto &i : includes)
            {
                if (strcmp(full_path, i) == 0)
                {
                    free(full_path);
                    return;
                }
            }
            includes.push_back(full_path);
            get_line_list_task(full_path);
            if (is_file_and_exists(full_path))
            {
                includes.push_back(full_path);
                find_functions_task(full_path);
                find_glob_vars_task(full_path);
            } */
            /*
            { */
            /* char **func_vec = find_functions_in_file(full_path);
            if (func_vec != NULL)
            {
                for (int i = 0; func_vec[i]; i++)
                {
                    function_info_t *info = parse_func(func_vec[i]);
                    char            *name = copy_of(info->name);
                    free_function_info(info);
                    color_map[name] = FG_VS_CODE_BRIGHT_YELLOW;
                }
            }
            free(func_vec); */
            /* char **var_vec = find_variabels_in_file(full_path);
            if (var_vec)
            {
                for (int i = 0; var_vec[i]; i++)
                {
                    // nlog("%s\n", var_vec[i]);
                    glob_var_t gvar;
                    parse_variable(
                        var_vec[i], &gvar.type, &gvar.name, &gvar.value);
                    if (gvar.name)
                    {
                        color_map[gvar.name] = FG_VS_CODE_BRIGHT_CYAN;
                        glob_vars.push_back(gvar);
                    }
                }
                free(var_vec);
            }
            includes.push_back(full_path);
            return;
            }
            free(full_path); */
        }
        else
        {
            end += 1;
            rendr(C_PTR, ERROR_MESSAGE, start, end);
        }
    }
    else
    {
        rendr(C_PTR, ERROR_MESSAGE, start, end);
    }
    start = end;
    while (*end)
    {
        adv_ptr(end, (*end == ' ' || *end == '\t'));
        if (*end == '\0')
        {
            break;
        }
        start = end;
        adv_ptr(end, (*end != ' ' && *end != '\t'));
        rendr(R, ERROR_MESSAGE, start, end);
    }
}

/* Render if preprossesor statements. */
void
rendr_if_preprosses(unsigned int index)
{
    const char *start   = &line->data[index];
    const char *defined = strstr(start, "defined");
    while (defined != NULL)
    {
        start = defined;
        defined += 7;
        if (*defined && *defined == ' ')
        {
            rendr(R, FG_VS_CODE_BLUE, start, defined);
        }
        else
        {
            rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, defined);
        }
        const char *parent_start = defined;
        const char *parent_end   = defined;
        while (*parent_start && *parent_end)
        {
            ADV_PTR(parent_start, *parent_start != '(' &&
                                      *parent_start != '&' &&
                                      *parent_start != '|');
            ADV_PTR(parent_end, *parent_end != ')' && *parent_end != '&' &&
                                    *parent_end != '|');
            if ((*parent_start == '&') || (*parent_start == '|') ||
                (*parent_end == '&') || (*parent_end == '|'))
            {
                break;
            }
            if (*parent_start != '\0' && *parent_end != '\0')
            {
                parent_start += 1;
                const char *p = parent_start;
                ADV_PTR(p, p != parent_end && *p != ' ' && *p != '\t');
                if (parent_start == parent_end)
                {
                    render_part((parent_start - line->data) - 1,
                                (parent_end - line->data) + 1, ERROR_MESSAGE);
                }
                else if (p == parent_end)
                {
                    rendr(R, FG_VS_CODE_BLUE, parent_start, parent_end);
                }
                else
                {
                    rendr(C_PTR, ERROR_MESSAGE, parent_start, parent_end);
                }
                parent_start += 1;
                parent_end += 1;
                p                     = parent_end - 1;
                const char *error_end = NULL;
                while (*p && *p != '&' && *p != '|')
                {
                    p += 1;
                    adv_ptr(p, (*p != '&') && (*p != '|') && (*p != '(') &&
                                   (*p != ')'));
                    if (*p == '(' || *p == ')')
                    {
                        error_end = p;
                    }
                }
                if (error_end != NULL)
                {
                    error_end += 1;
                    rendr(C_PTR, ERROR_MESSAGE, start, error_end);
                }
                break;
            }
            else if ((*parent_start == '\0' && *parent_end != '\0') ||
                     (*parent_start != '\0' && *parent_end == '\0'))
            {
                if (*parent_start != '\0')
                {
                    rendr(R_CHAR, ERROR_MESSAGE, parent_start);
                }
                else
                {
                    if (line->data[index] != '(')
                    {
                        rendr(R_CHAR, ERROR_MESSAGE, parent_end);
                    }
                }
                break;
            }
        }
        defined = strstr(defined, "defined");
    }
}

/* This 'render' sub-system is responsible for handeling all pre-prossesor
 * syntax.  TODO: Create a structured way to parse, and then create a
 * system to include error handeling in real-time. */
void
render_preprossesor(void)
{
    char       *current_word = NULL;
    const char *start        = strchr(line->data, '#');
    const char *end          = NULL;
    if (start != NULL)
    {
        rendr(R_CHAR, FG_VS_CODE_BRIGHT_MAGENTA, start);
        start += 1;
        if (*start == '\0')
        {
            return;
        }
        end = start;
        adv_ptr(end, (*end == ' ' || *end == '\t'));
        if (*end == '\0')
        {
            return;
        }
        start = end;
        adv_ptr(end, (*end != ' ' && *end != '\t'));
        current_word   = measured_copy(start, (end - start));
        const int type = hash_string(current_word);
        switch (type)
        {
            case define_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                adv_ptr(end, (*end == ' ' || *end == '\t'));
                rendr_define((end - line->data));
                break;
            }
            case if_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                adv_ptr(end, (*end == ' ' || *end == '\t'));
                rendr_if_preprosses((end - line->data));
                break;
            }
            case endif_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                break;
            }
            case ifndef_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                get_next_word(&start, &end);
                rendr(R, FG_VS_CODE_BLUE, start, end);
                break;
            }
            case pragma_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                get_next_word(&start, &end);
                rendr(R, FG_LAGOON, start, end);
                break;
            }
            case ifdef_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                get_next_word(&start, &end);
                rendr(R, FG_VS_CODE_BLUE, start, end);
                break;
            }
            case else_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                break;
            }
            case include_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                adv_ptr(end, (*end == ' ' || *end == '\t'));
                rendr_include((end - line->data));
                break;
            }
            case "undef"_uint_hash :
            {
                rendr(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                get_next_word(&start, &end);
                rendr(R, FG_VS_CODE_BLUE, start, end);
                break;
            }
            case "error"_uint_hash :
            {
                RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
                ADV_PTR(end, (*end == ' ' || *end == '\t'));
                if (!*end || *end != '"')
                {
                    break;
                }
                start = end;
                end += 1;
                ADV_PTR(end, (*end != '"'));
                if (*end)
                {
                    end += 1;
                }
                render_part(
                    (start - line->data), (end - line->data), FG_YELLOW);
                break;
            }
        }
        free(current_word);
    }
}

/* Handels most syntax local to functions such as handeling
 * local variabels and params. */
void
render_function_params(void)
{
    const char *start = NULL;
    const char *end   = NULL;
    const char *data  = NULL;
    for (const auto &info : local_funcs)
    {
        if (line->lineno + 2 >= info.start_bracket &&
            line->lineno <= info.end_braket)
        {
            for (variable_t *var = info.params; var != NULL; var = var->prev)
            {
                if (var->name != NULL)
                {
                    const unsigned int word_len = strlen(var->name);
                    data                        = line->data;
                    do {
                        find_word(
                            line, data, var->name, word_len, &start, &end);
                        if (start)
                        {
                            rendr(R, FG_VS_CODE_BRIGHT_CYAN, start, end);
                        }
                        data = end;
                    }
                    while (data);
                }
            }
        }
    }
}

void
render_control_statements(unsigned long index)
{
    switch (line->data[index])
    {
        case 'e' : /* else */
        {
            unsigned long else_indent = line_indent(line);
            unsigned long indent;
            const char   *if_found = NULL;
            int           i        = 0;
            for (linestruct *l = line->prev; l != NULL && (i < 500);
                 l             = l->prev, i++)
            {
                indent = line_indent(l);
                if (indent <= else_indent)
                {
                    if_found = strstr(l->data, "if");
                    if (if_found != NULL)
                    {
                        break;
                    }
                }
            }
            if (if_found == NULL || indent != else_indent)
            {
                rendr(E, "<- Misleading indentation");
                /* Add BG_YELLOW. */
                rendr(R, ERROR_MESSAGE, &line->data[index],
                      &line->data[index + 4]);
            }
            break;
        }
    }
}

void
rendr_glob_vars(void)
{
    if (LINE_ISSET(line, IN_BLOCK_COMMENT) ||
        LINE_ISSET(line, SINGLE_LINE_BLOCK_COMMENT) ||
        LINE_ISSET(line, BLOCK_COMMENT_START) ||
        LINE_ISSET(line, BLOCK_COMMENT_END))
    {
        return;
    }
    const char *data    = NULL;
    const char *start   = NULL;
    bool        in_func = FALSE;
    for (const auto &f : local_funcs)
    {
        if (line->lineno >= f.start_bracket && line->lineno <= f.end_braket)
        {
            in_func = TRUE;
            break;
        }
    }
    if (!in_func)
    {
        data  = line->data;
        start = strchr(data, ';');
        if (start && line->data[indent_char_len(line)] != '}')
        {
            char *str = measured_copy(line->data, (start - line->data) + 1);
            glob_var_t ngvar;
            parse_variable(str, &ngvar.type, &ngvar.name, &ngvar.value);
            if (ngvar.name)
            {
                bool found = FALSE;
                for (const auto &gv : glob_vars)
                {
                    if (strcmp(gv.name, ngvar.name) == 0)
                    {
                        found = TRUE;
                        break;
                    }
                }
                if (found == FALSE)
                {
                    const auto &it = test_map.find(ngvar.name);
                    if (it == test_map.end())
                    {
                        test_map[ngvar.name].color = FG_VS_CODE_BRIGHT_CYAN;
                        glob_vars.push_back(ngvar);
                        nlog("ngvar str: %s\n", str);
                    }
                }
            }
            free(str);
        }
    }
}

void
rendr_classes(void)
{
    const char *found = word_strstr(line->data, "class");
    if (found)
    {
        remove_from_color_map(line, FG_VS_CODE_GREEN, CLASS_SYNTAX);
        const char *start = NULL;
        const char *end   = NULL;
        start             = found;
        start += "class"_sllen;
        ADV_PTR(start, (*start == ' ' || *start == '\t'));
        /* If there is nothing after the word class print error msg. */
        if (*start == '\0')
        {
            RENDR(E, "<-(Expected class name)");
            return;
        }
        int end_line = find_class_end_line(line);
        end          = start;
        ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '{'));
        /* If the char after class name is not '{' or null terminator. */
        if (*end != '{' && *end != '\0')
        {
            /* We then iter to past all spaces and tabs. */
            ADV_PTR(end, (*end == ' ' || *end == '\t'));
            /* And if there is something other then '{'
             * or null-terminator.  We print error. */
            if (*end != '\0' && *end != '{')
            {
                rendr(E, "<-(Expected one word as class name)");
                return;
            }
        }
        end = start;
        ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '{'));
        class_info_t class_info;
        string       name(start, (end - start));
        class_info.name           = name;
        test_map[class_info.name] = {
            FG_VS_CODE_GREEN,
            (int)line->lineno,
            100000,
            CLASS_SYNTAX,
        };
        const char *func_found = NULL;
        for (linestruct *cl = line; cl->lineno < end_line; cl = cl->next)
        {
            func_found = strchr(cl->data, '(');
            if (func_found)
            {
                end = func_found;
                dcr_ptr(end, cl->data, (*end != ' ' && *end != '\t'));
                adv_ptr_to_next_word(end);
                string method(end, (func_found - end));
                if (method != class_info.name)
                {
                    remove_from_color_map(
                        line, FG_VS_CODE_BRIGHT_YELLOW, CLASS_METHOD_SYNTAX);
                    test_map[method] = {
                        FG_VS_CODE_BRIGHT_YELLOW,
                        (int)cl->lineno,
                        end_line,
                        CLASS_METHOD_SYNTAX,
                    };
                    class_info.methods.push_back(method);
                }
            }
        }
        class_info_vector.push_back(class_info);
    }
}

void
rendr_structs(int index)
{
    remove_from_color_map(line, FG_VS_CODE_GREEN, STRUCT_SYNTAX);
    const char *start = &line->data[index];
    const char *end   = &line->data[index];
    ADV_TO_NEXT_WORD(end);
    if (!*end)
    {
        return;
    }
    start = end;
    ADV_PAST_WORD(end);
    string name(start, (end - start));
    test_map[name] = {
        FG_VS_CODE_GREEN,
        (int)line->lineno,
        100000,
        STRUCT_SYNTAX,
    };
    NLOG("struct found: %s\n", name.c_str());
}

/* Main function that applies syntax to a line in real time. */
void
apply_syntax_to_line(const int row, const char *converted, linestruct *line,
                     unsigned long from_col)
{
    PROFILE_FUNCTION;
    ::row       = row;
    ::converted = converted;
    ::line      = line;
    ::from_col  = from_col;
    render_function_params();
    render_comment();
    if (line->data[indent_char_len(line)] == '#')
    {

        render_preprossesor();
        return;
    }
    if (line->data[0] == '\0' ||
        (block_comment_start == 0 && block_comment_end == till_x))
    {
        return;
    }
    render_bracket();
    render_parents();
    remove_local_vars_from(line);
    check_line_for_vars(line);
    line_word_t *head = line_word_list(line->data, till_x);
    while (head != NULL)
    {
        line_word_t *node = head;
        head              = node->next;
        if (node->start >= block_comment_start &&
            node->end <= block_comment_end)
        {
            /* midwin_mv_add_nstr_color(row, get_start_col(line, node),
               node->str, node->len, FG_COMMENT_GREEN); */
            free_node(node);
            continue;
        }
        const auto &it = test_map.find(node->str);
        if (it != test_map.end())
        {
            if (it->second.from_line != -1)
            {
                if (line->lineno >= it->second.from_line &&
                    line->lineno <= it->second.to_line)

                {
                    if (line->lineno == it->second.to_line)
                    {
                        const char *bracket = strchr(line->data, '}');
                        if (bracket && node->start > (bracket - line->data))
                        {
                            free_node(node);
                            continue;
                        }
                    }
                    midwin_mv_add_nstr_color(row, get_start_col(line, node),
                                             node->str, node->len,
                                             it->second.color);
                }
            }
            else
            {
                midwin_mv_add_nstr_color(row, get_start_col(line, node),
                                         node->str, node->len,
                                         it->second.color);
                if (it->second.color == FG_VS_CODE_BRIGHT_MAGENTA)
                {
                    render_control_statements(node->start);
                }
            }
            if (it->second.type == IS_WORD_STRUCT)
            {
                rendr_structs(node->end);
            }
            else if (it->second.type == IS_WORD_CLASS)
            {
                rendr_classes();
            }
        }
        free_node(node);
    }
    if (line->data[indent_char_len(line)] == '#')
    {
        render_preprossesor();
        return;
    }
    // render_string_literals();
    render_char_strings();
    if (LINE_ISSET(line, DONT_PREPROSSES_LINE))
    {
        render_part(0, till_x, FG_SUGGEST_GRAY);
        return;
    }
}

void
rendr_suggestion(void)
{
    PROFILE_FUNCTION;
    suggest_str = NULL;
    if (suggestwin != NULL)
    {
        delwin(suggestwin);
    }
    if ((openfile->current_x == 0 ||
         (!is_word_char(
              &openfile->current->data[openfile->current_x - 1], FALSE) &&
          openfile->current->data[openfile->current_x - 1] != '_')) &&
        openfile->current->data[openfile->current_x - 1] != '.')
    {
        clear_suggestion();
        return;
    }
    if (suggest_len < 0)
    {
        clear_suggestion();
    }
    add_char_to_suggest_buf();
    find_suggestion();
    draw_suggest_win();
}

/* Cleans up all thing related to rendering. */
void
cleanup_rendr(void)
{
    for (int i = 0; i < includes.get_size(); i++)
    {
        free(includes[i]);
    }
    for (int i = 0; i < defines.get_size(); i++)
    {
        free(defines[i]);
    }
}
