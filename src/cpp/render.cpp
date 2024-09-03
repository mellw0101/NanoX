#include "../include/prototypes.h"

#include <Mlib/Profile.h>

static unsigned int  block_comment_start = (unsigned int)-1;
static unsigned int  block_comment_end   = (unsigned int)-1;
static unsigned char next_word_color     = FALSE;
static unsigned int  last_type           = 0;
static int           color_bi[3]         = {FG_YELLOW, FG_PINK, FG_BLUE};

void
render_part(int row, const char *converted, linestruct *line, unsigned long match_start,
            unsigned long match_end, unsigned long from_col, short color)
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
    paintlen = actual_x(thetext, wideness(line->data, match_end) - from_col - start_col);
    midwin_mv_add_nstr_color(row, (margin + start_col), thetext, paintlen, color);
}
#define render_text(start, end, col)     render_part(row, converted, line, start, end, from_col, col)
#define render_text_ptr(start, end, col) render_text((start - line->data), (end - line->data), col)

/* Render the text of a given line.  Note that this function only renders the text and nothing else. */
void
render_line_text(const int row, const char *str, linestruct *line, const unsigned long from_col)
{
    if (margin > 0)
    {
        midwin_color_on(LINE_NUMBER);
        if (ISSET(SOFTWRAP) && from_col != 0)
        {
            midwin_mv_printw(row, 0, "%*s", margin - 1, " ");
        }
        else
        {
            midwin_mv_printw(row, 0, "%*lu", margin - 1, line->lineno);
        }
        midwin_color_off(LINE_NUMBER);
        if (line->has_anchor == TRUE && (from_col == 0 || !ISSET(SOFTWRAP)))
        {
            if (using_utf8())
            {
                midwin_printw("\xE2\xAC\xA5");
            }
            else
            {
                midwin_printw("+");
            }
        }
        else
        {
            midwin_printw(" ");
        }
    }
    midwin_mv_add_str(row, margin, str);
    if (is_shorter || ISSET(SOFTWRAP))
    {
        midwin_clear_to_eol();
    }
    if (sidebar)
    {
        midwin_mv_add_char(row, COLS - 1, bardata[row]);
    }
}

/* Set start and end pos for comment block or if the entire line is inside of a block comment
 * set 'block_comment_start' to '0' and 'block_comment_end' to '(unsigned int)-1'. */
void
render_comment(int row, const char *converted, linestruct *line, unsigned long from_col)
{
    PROFILE_FUNCTION;
    const char *found_start = strstr(line->data, "/*");
    const char *found_end   = strstr(line->data, "*/");
    const char *found_slash = strstr(line->data, "//");
    /* Single line block comment. */
    if (found_start != NULL && found_end != NULL)
    {
        block_comment_start = (found_start - line->data);
        block_comment_end   = (found_end - line->data + 2);
        /* If slash comment found, adjust the start and end pos correctly. */
        if (found_slash != NULL && (found_slash - line->data) < block_comment_start)
        {
            block_comment_start = (found_slash - line->data);
            block_comment_end   = (unsigned int)-1;
        }
        LINE_SET(line, SINGLE_LINE_BLOCK_COMMENT);
        LINE_UNSET(line, BLOCK_COMMENT_START);
        LINE_UNSET(line, BLOCK_COMMENT_END);
        LINE_UNSET(line, IN_BLOCK_COMMENT);
        midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, found_end), found_end, 2, FG_GREEN);
        /* Highlight the block start if prev line is in a
         * block comment or the start of a block comment. */
        if (line->prev &&
            (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) || LINE_ISSET(line->prev, BLOCK_COMMENT_START)))
        {
            if ((found_end - line->data) > 0 && line->data[(found_end - line->data) - 1] != '/')
            {
                midwin_mv_add_nstr_color(row, (wideness(line->data, (found_start - line->data)) + margin),
                                         found_start, 2, ERROR_MESSAGE);
                /* If there is a slash comment infront the block comment. Then of cource we still color
                 * the text from the slash to the block start after we error highlight the block start. */
                if (found_slash != NULL && (found_slash - line->data) < (found_start - line->data))
                {
                    midwin_mv_add_nstr_color(
                        row, (wideness(line->data, (found_slash - line->data))) + margin, found_slash,
                        (found_start - line->data) - (found_slash - line->data), FG_GREEN);
                }
                block_comment_start += (found_start - line->data) + 2;
            }
            else if ((found_start - line->data) + 1 == (found_end - line->data))
            {
                LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
                LINE_SET(line, BLOCK_COMMENT_END);
                block_comment_start = 0;
            }
        }
        else if ((found_start - line->data) + 1 == (found_end - line->data))
        {
            found_end = strstr(found_end + 2, "*/");
            if (found_end != NULL)
            {
                block_comment_end = (found_end - line->data) + 2;
            }
            else
            {
                block_comment_end = (unsigned int)-1;
                LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
                LINE_SET(line, BLOCK_COMMENT_START);
            }
        }
        while (found_start != NULL && found_end != NULL)
        {
            /* TODO: Here we need to fix the issue of multiple block comments on a single line. */
            found_start = strstr(found_start + 2, "/*");
            found_end   = strstr(found_end + 2, "*/");
            found_slash = strstr(found_slash ? found_slash + 2 : line->data, "//");
            if (found_start != NULL && found_end != NULL)
            {
                const unsigned long match_start = (found_start - line->data);
                const unsigned long match_end   = (found_end - line->data) + 2;
                render_part(row, converted, line, match_start, match_end, from_col, FG_GREEN);
            }
            else if (found_start == NULL && found_end != NULL)
            {
                midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, found_end), found_end, 2, ERROR_MESSAGE);
            }
            else if (found_start != NULL && found_end == NULL)
            {
                const unsigned long match_start = (found_start - line->data);
                render_part(row, converted, line, match_start, till_x, from_col, FG_GREEN);
            }
        }
    }
    /* First line for a block comment. */
    else if (found_start != NULL && found_end == NULL)
    {
        block_comment_start = (found_start - line->data);
        block_comment_end   = (unsigned int)-1;
        /* Do some error checking and highlight the block start if it`s found
         * while the block above it being a start block or inside a block. */
        if (line->prev &&
            (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) || LINE_ISSET(line->prev, BLOCK_COMMENT_START)))
        {
            midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, found_start), found_start, 2, ERROR_MESSAGE);
            block_comment_start = (found_start - line->data) + 2;
        }
        /* If a slash comment is found and it is before the block start,
         * we adjust the start and end pos.  We also make sure to unset
         * 'BLOCK_COMMENT_START' for the line. */
        if (found_slash != NULL && (found_slash - line->data) < block_comment_start)
        {
            block_comment_start = (found_slash - line->data);
            block_comment_end   = (unsigned int)-1;
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
    else if (found_start == NULL && found_end == NULL)
    {
        if (line->prev &&
            (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) || LINE_ISSET(line->prev, BLOCK_COMMENT_START)) &&
            !LINE_ISSET(line->prev, SINGLE_LINE_BLOCK_COMMENT))
        {
            block_comment_start = 0;
            block_comment_end   = (unsigned int)-1;
            LINE_SET(line, IN_BLOCK_COMMENT);
            LINE_UNSET(line, BLOCK_COMMENT_START);
            LINE_UNSET(line, BLOCK_COMMENT_END);
            LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
        }
        /* If the prev line is not in a block comment or the
         * start block line we are not inside a comment block. */
        else
        {
            block_comment_start = (unsigned int)-1;
            block_comment_end   = 0;
            LINE_UNSET(line, IN_BLOCK_COMMENT);
            LINE_UNSET(line, BLOCK_COMMENT_START);
            LINE_UNSET(line, BLOCK_COMMENT_END);
            LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
            /* If slash comment is found comment out entire line after slash. */
            if (found_slash != NULL)
            {
                block_comment_start = (found_slash - line->data);
                block_comment_end   = (unsigned int)-1;
            }
        }
    }
    /* End of a block comment. */
    else if (found_start == NULL && found_end != NULL)
    {
        /* If last line is in a comment block or is the start of the block. */
        if (line->prev &&
            (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) || LINE_ISSET(line->prev, BLOCK_COMMENT_START)) &&
            !LINE_ISSET(line->prev, SINGLE_LINE_BLOCK_COMMENT))
        {
            block_comment_start = 0;
            block_comment_end   = (found_end - line->data) + 2;
            midwin_mv_add_nstr_color(
                row, (wideness(line->data, (found_end - line->data)) + margin), found_end, 2, FG_GREEN);
            LINE_SET(line, BLOCK_COMMENT_END);
            LINE_UNSET(line, IN_BLOCK_COMMENT);
            LINE_UNSET(line, BLOCK_COMMENT_START);
            LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
        }
        /* If slash if found and is before block end. */
        else if (found_slash != NULL && (found_slash - line->data) < (found_end - line->data))
        {
            block_comment_start = 0;
            block_comment_end   = (unsigned int)-1;
        }
        /* If not, error highlight the end block. */
        else
        {
            midwin_mv_add_nstr_color(
                row, (wideness(line->data, (found_end - line->data)) + margin), found_end, 2, ERROR_MESSAGE);
        }
    }
    refresh_needed = TRUE;
}

/* Color brackets based on indent. */
void
render_bracket(int row, linestruct *line)
{
    PROFILE_FUNCTION;
    const char *start = strchr(line->data, '{');
    const char *end   = strrchr(line->data, '}');
    /* Bracket start and end on the same line. */
    if (start != NULL && end != NULL)
    {
        while (start != NULL)
        {
            midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, start), "{", 1, FG_YELLOW);
            if (line->data[(start - line->data) + 1] == '\0')
            {
                start = NULL;
                continue;
            }
            start = strchr(start + 1, '{');
        }
        while (end != NULL)
        {
            midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, end), "}", 1, FG_YELLOW);
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
        if (line->prev && (LINE_ISSET(line->prev, IN_BRACKET) || LINE_ISSET(line->prev, BRACKET_START)))
        {
            LINE_SET(line, IN_BRACKET);
        }
    }
    /* Start bracket line was found. */
    else if (start != NULL && end == NULL)
    {
        LINE_SET(line, BRACKET_START);
        midwin_mv_add_nstr_color(
            row, PTR_POS_LINE(line, start), "{", 1, color_bi[(indent_char_len(line) % 3)]);
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
        midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, end), "}", 1, color_bi[(indent_char_len(line) % 3)]);
        for (linestruct *t_line = line->prev; t_line; t_line = t_line->prev)
        {
            if (LINE_ISSET(t_line, BRACKET_START))
            {
                unsigned short spaces, tabs, t_char, t_tabs;
                get_line_indent(line, &tabs, &spaces, &t_char, &t_tabs);
                const unsigned short line_t_tabs = t_tabs;
                get_line_indent(t_line, &tabs, &spaces, &t_char, &t_tabs);
                const unsigned short t_line_t_tabs = t_tabs;
                if (line_t_tabs == t_line_t_tabs)
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
    else if (start == NULL && end == NULL)
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

void
render_parents(int row, const char *converted, linestruct *line, unsigned long from_col)
{
    const char *start = strchr(line->data, '(');
    const char *end   = strchr(line->data, ')');
    while (1)
    {
        if (start != NULL)
        {
            midwin_mv_add_nstr_color(
                row, PTR_POS_LINE(line, start), "(", 1, color_bi[(indent_char_len(line) % 3)]);
            start = strchr(start + 1, '(');
        }
        if (end != NULL)
        {
            midwin_mv_add_nstr_color(
                row, PTR_POS_LINE(line, end), ")", 1, color_bi[(indent_char_len(line) % 3)]);
            end = strchr(end + 1, ')');
        }
        if (start == NULL && end == NULL)
        {
            break;
        }
    }
}

/* This function highlights string literals.  Error handeling is needed. */
void
render_string_literals(int row, const char *converted, linestruct *line, unsigned long from_col)
{
    PROFILE_FUNCTION;
    const char *start = line->data;
    const char *end   = line->data;
    while (start != NULL)
    {
        for (; *end && *end != '"'; end++)
            ;
        if (*end != '"')
        {
            return;
        }
        start                           = end;
        const unsigned long match_start = (start - line->data);
        end += 1;
        for (; *end && *end != '"'; end++)
            ;
        if (*end != '"')
        {
            render_part(row, converted, line, match_start, till_x, from_col, ERROR_MESSAGE);
            return;
        }
        end += 1;
        const unsigned long match_end = (end - line->data);
        if ((match_start >= till_x) || (match_start >= block_comment_start && match_end <= block_comment_end))
        {
            return;
        }
        render_part(row, converted, line, match_start, match_end, from_col, FG_YELLOW);
        start = end;
    }
}

/* Function to handle char strings inside other strings or just in general. */
void
render_char_strings(int row, const char *converted, linestruct *line, unsigned long from_col)
{
    PROFILE_FUNCTION;
    const char *start = line->data, *end = line->data;
    while (start != NULL)
    {
        for (; *end && *end != '\''; end++)
            ;
        if (*end != '\'')
        {
            return;
        }
        start = end;
        end++;
        for (; *end && *end != '\''; end++)
            ;
        if (*end != '\'')
        {
            return;
        }
        end++;
        const unsigned long match_start = (start - line->data);
        const unsigned long match_end   = (end - line->data);
        if (match_start >= block_comment_start && match_end <= block_comment_end)
        {
            return;
        }
        render_part(row, converted, line, match_start, match_end, from_col, FG_MAGENTA);
        start = end;
    }
}

void
render_preprossesor(int row, const char *converted, linestruct *line, unsigned long from_col)
{
    PROFILE_FUNCTION;
    char          *current_word = NULL;
    const char    *start        = NULL;
    const char    *end          = NULL;
    const char    *found        = strchr(line->data, '#');
    preprossesor_t ltype        = NONE;
    if (found != NULL)
    {
        render_str_len(found, "#", 1, FG_MAGENTA);
        start = found + 1;
        end   = found + 1;
        while (1)
        {
            for (; end < (line->data + till_x) && (*end == ' ' || *end == '\t'); end++)
                ;
            if (end == (line->data + till_x))
            {
                return;
            }
            start = end;
            for (; end < (line->data + till_x) && (*end != ' ') && (*end != '\t'); end++)
                ;
            current_word = measured_copy(start, (end - start));
            switch (ltype)
            {
                case NONE :
                {
                    break;
                }
                case DEFINE :
                {
                    end = start;
                    for (; end <= (line->data + till_x) && (*end != '(') && (*end != ' ') && (*end != '\t');
                         end++)
                        ;
                    render_str_ptr(start, end, FG_BLUE);
                    if (*end == '(')
                    {
                        while (end != (line->data + till_x))
                        {
                            end += 1;
                            start = end;
                            for (; end <= (line->data + till_x) && (*end != ')') && (*end != ',') &&
                                   (*end != ' ') && (*end != '\t');
                                 end++)
                                ;
                            if (*end == ')' || *end == ',')
                            {
                                render_str_ptr(start, end, FG_BLUE);
                                free(current_word);
                                current_word      = measured_copy(start, (end - start));
                                const char *param = strstr(end, current_word);
                                if (param != NULL)
                                {
                                    while (param != NULL)
                                    {
                                        if ((param[(end - start)] == ' ' || param[(end - start)] == ')' ||
                                             param[(end - start)] == ',') &&
                                            (line->data[(param - line->data) - 1] == ',' ||
                                             line->data[(param - line->data) - 1] == ' ' ||
                                             line->data[(param - line->data) - 1] == '('))
                                        {
                                            render_str_len(param, param, (end - start), FG_BLUE);
                                        }
                                        param = strstr(param + (end - start), current_word);
                                    }
                                }
                                if (*end == ',' && end[1] == ' ')
                                {
                                    end += 1;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    /* TODO: Continue here when parsing define`s are complete.
                    if (*end == '\t' || *end == ' ')
                    {
                        start = end + 1;
                        if (*start == '\\')
                        {}
                    } */
                    free(current_word);
                    return;
                }
                case IF :
                {
                    if (strncmp(current_word, "defined", 7) == 0)
                    {
                        free(current_word);
                        current_word =
                            measured_copy(line->data + (start - line->data), (till_x - (start - line->data)));
                        const char *defined = strstr(start, "defined");
                        while (defined != NULL)
                        {
                            start = defined;
                            defined += 7;
                            render_text_ptr(start, defined, FG_MAGENTA);
                            defined = strstr(defined, "defined");
                        }
                        free(current_word);
                        return;
                    }
                    break;
                }
                case INCLUDE :
                {
                    end = start;
                    for (; end < (line->data + till_x) && (*end != '>') && (*end != '"'); end++)
                        ;
                    if (end != (line->data + till_x) && (*start == '<' || *start == '"'))
                    {
                        end += 1;
                        render_text_ptr(start, end, FG_YELLOW);
                    }
                    else
                    {
                        if (end != (line->data + till_x))
                        {
                            end += 1;
                        }
                        render_text_ptr(start, end, ERROR_MESSAGE);
                    }
                    return;
                }
                case IFDEF :
                {
                    render_str_ptr(start, end, FG_BLUE);
                    break;
                }
            }
            if (strcmp(current_word, "define") == 0)
            {
                render_str_ptr(start, end, FG_MAGENTA);
                ltype = DEFINE;
            }
            else if (strcmp(current_word, "if") == 0)
            {
                render_text_ptr(start, end, FG_MAGENTA);
                ltype = IF;
            }
            else if (strcmp(current_word, "endif") == 0)
            {
                render_text_ptr(start, end, FG_MAGENTA);
            }
            else if (strcmp(current_word, "ifndef") == 0)
            {
                render_text_ptr(start, end, FG_MAGENTA);
            }
            else if (strcmp(current_word, "pragma") == 0)
            {
                render_text_ptr(start, end, FG_MAGENTA);
            }
            else if (strcmp(current_word, "ifdef") == 0)
            {
                render_str_ptr(start, end, FG_MAGENTA);
                ltype = IFDEF;
            }
            else if (strcmp(current_word, "else") == 0)
            {
                render_str_ptr(start, end, FG_MAGENTA);
            }
            else if (strcmp(current_word, "include") == 0)
            {
                render_str_ptr(start, end, FG_MAGENTA);
                ltype = INCLUDE;
            }
            else
            {
                free(current_word);
                break;
            }
            free(current_word);
        }
    }
}

/* Main function that applies syntax to a line in real time. */
void
apply_syntax_to_line(const int row, const char *converted, linestruct *line, unsigned long from_col)
{
    PROFILE_FUNCTION;
    render_preprossesor(row, converted, line, from_col);
    render_comment(row, converted, line, from_col);
    render_bracket(row, line);
    render_parents(row, converted, line, from_col);
    if (line->data[0] == '\0')
    {
        return;
    }
    line_word_t *head = line_word_list(line->data, till_x);
    while (head != NULL)
    {
        line_word_t *node = head;
        head              = node->next;
        if (node->start >= block_comment_start && node->end <= block_comment_end)
        {
            midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_GREEN);
            free_node(node);
            continue;
        }
        if (node->str[1] && *node->str == '(')
        {
            node->start++;
            char *p   = node->str;
            node->str = measured_copy(node->str + 1, --node->len);
            free(p);
        }
        (node->str[node->len - 1] != ';') ?: node->str[--node->len] = '\0';
        while (node->str[node->len - 1] == ')')
        {
            node->str[--node->len] = '\0';
        }
        unsigned long at;
        if (char_is_in_word(node->str, '(', &at))
        {
            if (node->str[at + 1] != '\0')
            {
                line_word_t *new_word = (line_word_t *)malloc(sizeof(*new_word));
                new_word->start       = node->start + at;
                new_word->end         = node->end;
                new_word->len         = node->len - at;
                new_word->str         = measured_copy(node->str + at, new_word->len);
                new_word->next        = node->next;
                head                  = new_word;
                node->str[at]         = '\0';
            }
        }
        if (next_word_color != FALSE)
        {
            for (unsigned int i = 0; node->str[i]; i++)
            {
                if (is_word_char(&node->str[i], FALSE) == FALSE && node->str[i] != '_')
                {
                    node->str[i] = '\0';
                    node->len    = i;
                }
            }
            midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, next_word_color);
            next_word_color = FALSE;
            if (last_type != FALSE)
            {
                if (last_type & CS_STRUCT)
                {
                    if (!is_syntax_struct(node->str))
                    {
                        add_syntax_struct(node->str);
                    }
                }
                else if (last_type & CS_CLASS)
                {
                    if (!is_syntax_class(node->str))
                    {
                        add_syntax_class(node->str);
                    }
                }
                last_type = FALSE;
            }
            free_node(node);
            continue;
        }
        const unsigned int type = retrieve_c_syntax_type(node->str);
        if (type != 0)
        {
            if (type & CS_VOID || type & CS_SHORT || type & CS_INT || type & CS_CHAR || type & CS_LONG ||
                type & CS_STATIC || type & CS_UNSIGNED || type & CS_BOOL || type & CS_CONST ||
                type & CS_NULL || type & CS_TRUE || type & CS_FALSE || type & CS_TYPEDEF || type & CS_SIZEOF)
            {
                midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_BLUE);
            }
            else if (type & CS_STRUCT || type & CS_ENUM || type & CS_CLASS || type & CS_NAMESPACE)
            {
                midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_BLUE);
                next_word_color = FG_GREEN;
                if (type & CS_STRUCT || type & CS_CLASS)
                {
                    last_type = type;
                }
            }
            else if (type & CS_SIZE_T || type & CS_SSIZE_T)
            {
                midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_GREEN);
            }
            else if (type & CS_IF || type & CS_CASE || type & CS_SWITCH || type & CS_ELSE || type & CS_FOR ||
                     type & CS_WHILE || type & CS_RETURN || type & CS_BREAK || type & CS_DO)
            {
                midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_MAGENTA);
            }
            free_node(node);
            continue;
        }
        for (unsigned int i = 0; node->str[i]; i++)
        {
            if (is_word_char(&node->str[i], FALSE) == FALSE && node->str[i] != '_')
            {
                node->str[i] = '\0';
                node->len    = i;
            }
        }
        if (syntax_func(node->str))
        {
            midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_YELLOW);
        }
        else if (is_syntax_struct(node->str))
        {
            midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_GREEN);
            // next_word_color = FG_LAGOON;
        }
        else if (is_syntax_class(node->str))
        {
            midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_GREEN);
            // next_word_color = FG_LAGOON;
        }
        if (func_info != NULL)
        {
            for (int i = 0; func_info[i]; i++)
            {
                if (strcmp(node->str, func_info[i]->name) == 0)
                {
                    midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_YELLOW);
                }
            }
        }
        free_node(node);
    }
    render_string_literals(row, converted, line, from_col);
    render_char_strings(row, converted, line, from_col);
}