#pragma once
#include "definitions.h"

#define COLOR_LAGOON                           38
#define COLOR_PINK                             204

#define get_start_col(line, node)              wideness((line)->data, (node)->start) + margin
#define start_column(line, index)              wideness((line)->data, index) + margin
#define PTR_POS(data, ptr)                     (wideness(data, (ptr - data)) + margin)
/* Retrieve correct visual position of a ptr in a line`s data. */
#define PTR_POS_LINE(line, char_ptr)           (wideness((line)->data, (char_ptr - (line)->data)) + margin)

#define LOG_FLAG(line, flag)                   NLOG("flag: '%s' is '%s'\n", #flag, LINE_ISSET(line, flag) ? "TRUE" : "FALSE")
#define GET_BRACKET_COLOR(n)                   color_bracket_index[(n % 2)]

/* Define`s for modifying the 'midwin', i.e: The edit window. */

#define midwin_clear()                         wclear(midwin)
#define midwin_clear_to_eol()                  wclrtoeol(midwin)
/* Turn attributes on for midwin. */
#define midwin_attr_on(attributes)             wattr_on(midwin, (attr_t)attributes, NULL)
/* Turn on a base color attr for midwin. */
#define midwin_color_on(color)                 midwin_attr_on(interface_color_pair[color])
/* Turn attributes off for midwin. */
#define midwin_attr_off(attributes)            wattr_off(midwin, (attr_t)attributes, NULL)
/* Turn off a base color attr for midwin. */
#define midwin_color_off(color)                midwin_attr_off(interface_color_pair[color])
/* Move to 'row', 'col' in midwin. */
#define midwin_move(row, col)                  wmove(midwin, row, col)
/* Add one char to midwin. */
#define midwin_add_char(char)                  waddch(midwin, char)
#define midwin_mv_add_char(row, col, char)     (midwin_move(row, col) == (-1)) ?: midwin_add_char(char)
/* Add 'len' of str to midwin. */
#define midwin_add_nstr(str, len)              waddnstr(midwin, str, len)
/* Add a str to midwin. */
#define midwin_add_str(str)                    midwin_add_nstr(str, -1)
/* Move to 'row', 'col', then draw 'len' of 'str' at 'row','col' in midwin. */
#define midwin_mv_add_nstr(row, col, str, len) (midwin_move(row, col) == (-1)) ?: midwin_add_nstr(str, len)
/* Move then add str to midwin. */
#define midwin_mv_add_str(row, col, str)       (midwin_move(row, col) == (-1)) ?: midwin_add_str(str)
/* Move then add str to midwin, with attributes */
#define midwin_mv_add_str_wattr(row, col, str, attributes) \
    midwin_attr_on(attributes);                            \
    midwin_mv_add_str(row, col, str);                      \
    midwin_attr_off(attributes)
/* Simplyfied macro for using 'midwin_mv_add_str_wattr' with base colors. */
#define midwin_mv_add_str_color(row, col, str, color) \
    midwin_color_on(color);                           \
    midwin_mv_add_str(row, col, str);                 \
    midwin_color_off(color)
/* Move then add len of str to midwin, with attributes. */
#define midwin_mv_add_nstr_wattr(row, col, str, len, attributes) \
    midwin_attr_on(attributes);                                  \
    midwin_mv_add_nstr(row, col, str, len);                      \
    midwin_attr_off(attributes)
/* Simplyfied macro for using 'midwin_mv_add_nstr_wattr' with base colors. */
#define midwin_mv_add_nstr_color(row, col, str, len, color) \
    midwin_color_on(color);                                 \
    midwin_mv_add_nstr(row, col, str, len);                 \
    midwin_color_off(color)
/* Move then add char to midwin. */
#define midwin_mv_add_char_wattr(row, col, char, attributes) \
    midwin_attr_on(attributes);                              \
    midwin_mv_add_char(row, col, char);                      \
    midwin_attr_off(attributes)

#define midwin_printw(...)              wprintw(midwin, __VA_ARGS__)
#define midwin_mv_printw(row, col, ...) mvwprintw(midwin, row, col, __VA_ARGS__)

#define midwin_find_all_in_line(find, len, color)                                                          \
    needle = find;                                                                                         \
    pos    = line->data;                                                                                   \
    while ((pos = strstr(pos, needle)) != NULL)                                                            \
    {                                                                                                      \
        midwin_mv_add_nstr_wattr(row, (pos - line->data) + margin, pos, len, interface_color_pair[color]); \
        pos += len;                                                                                        \
    }

#define char_ptr_pos_in_str(char_ptr, full_str) (char_ptr - full_str)
