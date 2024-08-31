#include "../include/prototypes.h"

#include <Mlib/Profile.h>

static bool          in_block_comment    = FALSE;
static unsigned int  block_comment_start = -1;
static unsigned int  block_comment_end   = -1;
static unsigned char next_word_color     = FALSE;
static unsigned int  last_type           = 0;

/* Render the text of a given line.  Note that this function only renders the text and nothing else. */
void
render_line_text(const int row, const char *str, linestruct *line, const unsigned long from_col)
{
    if (margin > 0)
    {
        midwin_attr_on(interface_color_pair[LINE_NUMBERS]);
        if (ISSET(SOFTWRAP) && from_col != 0)
        {
            midwin_mv_printw(row, 0, "%*s", margin - 1, " ");
        }
        else
        {
            midwin_mv_printw(row, 0, "%*lu", margin - 1, line->lineno);
        }
        midwin_attr_off(interface_color_pair[LINE_NUMBERS]);
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

bool
find_block_start(linestruct *from_line)
{
    PROFILE_FUNCTION;
    linestruct *line = from_line;
    for (int i = 0; line->prev != NULL; line = line->prev, i++)
    {
        /* if (LINE_ISSET(line, IN_BLOCK_COMMENT))
        {
            continue;
        }
        else if (LINE_ISSET(line, BLOCK_COMMENT_START))
        {
            // NLOG("Found block start: %s\n", line->data);
            return TRUE;
        }
        else  */
        if (LINE_ISSET(line, BLOCK_COMMENT_END))
        {
            NLOG("Found block end: %s\n", line->data);
            return TRUE;
        }
        if (i > 30)
        {
            break;
        }
    }
    return FALSE;
}

/* Set start and end pos for comment block or if the entire line is inside of a block comment
 * set 'block_comment_start' to '0' and 'block_comment_end' to '(unsigned int)-1'. */
void
comment_block(int row, linestruct *line)
{
    PROFILE_FUNCTION;
    int         len         = 2;
    int         start_col   = 0;
    const char *found_start = strstr(line->data, "/*");
    const char *found_end   = strstr(line->data, "*/");
    if (found_start != NULL && found_end != NULL)
    {
        block_comment_start = (found_start - line->data);
        block_comment_end   = (block_comment_start + (found_end - line->data + len));
        /* start_col for the end of the block comment. */
        start_col = wideness(line->data, (found_end - line->data));
        /* Then we color just the end block. */
        midwin_mv_add_nstr_color(row, (start_col + margin), found_end, len, FG_GREEN);
        in_block_comment = FALSE;
        LINE_SET(line, SINGLE_LINE_BLOCK_COMMENT);
        LINE_UNSET(line, BLOCK_COMMENT_START);
        LINE_UNSET(line, BLOCK_COMMENT_END);
        LINE_UNSET(line, IN_BLOCK_COMMENT);
        refresh_needed = TRUE;
        return;
    }
    else if (found_start != NULL && found_end == NULL)
    {
        block_comment_start = (found_start - line->data);
        block_comment_end   = (unsigned int)-1;
        LINE_SET(line, BLOCK_COMMENT_START);
        LINE_UNSET(line, SINGLE_LINE_BLOCK_COMMENT);
        LINE_UNSET(line, IN_BLOCK_COMMENT);
        LINE_UNSET(line, BLOCK_COMMENT_END);
        in_block_comment = TRUE;
        return;
    }
    else if (in_block_comment && found_start == NULL && found_end == NULL)
    {
        if (line->prev &&
            (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) || LINE_ISSET(line->prev, BLOCK_COMMENT_START)) &&
            !LINE_ISSET(line->prev, SINGLE_LINE_BLOCK_COMMENT))
        {
            block_comment_start = 0;
            block_comment_end   = (unsigned int)-1;
            LINE_SET(line, IN_BLOCK_COMMENT);
        }
        return;
    }
    else if (found_start == NULL && found_end != NULL)
    {
        block_comment_start = 0;
        block_comment_end   = (found_end - line->data);
        start_col           = wideness(line->data, block_comment_end);
        midwin_mv_add_nstr_color(row, start_col + margin, found_end, len, FG_GREEN);
        if (line->prev &&
            (LINE_ISSET(line->prev, IN_BLOCK_COMMENT) || LINE_ISSET(line->prev, BLOCK_COMMENT_START)) &&
            !LINE_ISSET(line->prev, SINGLE_LINE_BLOCK_COMMENT))
        {
            LINE_SET(line, BLOCK_COMMENT_END);
        }
        in_block_comment = FALSE;
        return;
    }
    /* block_comment_start = (unsigned int)-1;
    block_comment_end   = 0; */
}

void
apply_syntax_to_line(const int row, const char *converted, linestruct *line, unsigned long from_col)
{
    PROFILE_FUNCTION;
    if (line->data[0] == '\0')
    {
        return;
    }
    comment_block(row, line);
    if (LINE_ISSET(line, IN_BLOCK_COMMENT))
    {
        block_comment_start = 0;
        block_comment_end   = (unsigned int)-1;
        if (line->prev != NULL &&
            (!LINE_ISSET(line->prev, IN_BLOCK_COMMENT) && !LINE_ISSET(line->prev, BLOCK_COMMENT_START)))
        {
            block_comment_start = (unsigned int)-1;
            block_comment_end   = 0;
            LINE_UNSET(line, IN_BLOCK_COMMENT);
        }
    }
    else if (!LINE_ISSET(line, SINGLE_LINE_BLOCK_COMMENT) && !LINE_ISSET(line, BLOCK_COMMENT_START) &&
             !LINE_ISSET(line, BLOCK_COMMENT_END))
    {
        block_comment_start = (unsigned int)-1;
        block_comment_end   = 0;
    }
    const char *pos = strstr(line->data, "//");
    if (pos)
    {
        block_comment_start = (pos - line->data);
        block_comment_end   = (unsigned int)-1;
    }
    line_word_t *head = line_word_list(line->data, till_x);
    while (head != NULL)
    {
        int          start_col = 0;
        line_word_t *node      = head;
        head                   = node->next;
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
        if (strcmp("#include", node->str) == 0)
        {
            midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_MAGENTA);
            free_node(node);
            continue;
        }
        if (strncmp("<", node->str, 1) == 0 && line->data[0] == '#')
        {
            midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_YELLOW);
            free_node(node);
            continue;
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
                     type & CS_WHILE || type & CS_RETURN)
            {
                midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_MAGENTA);
            }
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
            start_col = wideness(line->data, node->start);
            midwin_mv_add_nstr_color(row, (start_col + margin), node->str, node->len, FG_GREEN);
            next_word_color = FG_LAGOON;
        }
        else if (is_syntax_class(node->str))
        {
            start_col = wideness(line->data, node->start);
            midwin_mv_add_nstr_color(row, (start_col + margin), node->str, node->len, FG_GREEN);
            next_word_color = FG_LAGOON;
        }
        free_node(node);
    }
}
