#pragma once
#include "definitions.h"

#define midwin_clear()                         wclear(midwin)
#define midwin_clear_to_eol()                  wclrtoeol(midwin)
/* Turn attributes on for midwin. */
#define midwin_attr_on(attributes)             wattr_on(midwin, (attr_t)attributes, NULL)
/* Turn attributes off for midwin. */
#define midwin_attr_off(attributes)            wattr_off(midwin, (attr_t)attributes, NULL)
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
/* Move then add len of str to midwin, with attributes. */
#define midwin_mv_add_nstr_wattr(row, col, str, len, attributes) \
    midwin_attr_on(attributes);                                  \
    midwin_mv_add_nstr(row, col, str, len);                      \
    midwin_attr_off(attributes)
/* Move then add char to midwin. */
#define midwin_mv_add_char_wattr(row, col, char, attributes) \
    midwin_attr_on(attributes);                              \
    midwin_mv_add_char(row, col, char);                      \
    midwin_attr_off(attributes);

#define midwin_printw(...)              wprintw(midwin, __VA_ARGS__)
#define midwin_mv_printw(row, col, ...) mvwprintw(midwin, row, col, __VA_ARGS__)
