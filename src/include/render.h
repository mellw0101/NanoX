#pragma once
#include "definitions.h"

#define rgb_8bit_in_xterm_color(r, g, b) \
    xterm_byte_scale(r) * 4 - 1, xterm_byte_scale(g) * 4 - 1, xterm_byte_scale(b) * 4 - 1

#define get_start_col(line, node)            wideness((line)->data, (node)->start) + margin
#define start_column(line, index)            wideness((line)->data, index) + margin
#define PTR_POS(data, ptr)                   (wideness(data, (ptr - data)) + margin)
/* Retrieve correct visual position of a ptr in a line`s data. */
#define PTR_POS_LINE(line, char_ptr)         (wideness((line)->data, (char_ptr - (line)->data)) + margin)

#define LOG_FLAG(line, flag)                 NLOG("flag: '%s' is '%s'\n", #flag, line->flags.is_set(flag) ? "TRUE" : "FALSE")
#define GET_BRACKET_COLOR(n)                 color_bracket_index[(n % 2)]

#define line_indent(line)                    wideness(line->data, indent_char_len(line))

#define win_attr_on(win, attr)               wattr_on(win, (attr_t)attr, NULL)
#define win_attr_off(win, attr)              wattr_off(win, (attr_t)attr, NULL)
#define win_color_on(win, color)             win_attr_on(win, interface_color_pair[color])
#define win_color_off(win, color)            win_attr_off(win, interface_color_pair[color])
#define mv_add_nstr(win, row, col, str, len) (wmove(win, row, col) == (-1)) ?: waddnstr(win, str, len)
#define mv_add_nstr_color(win, row, col, str, len, color) \
    win_color_on(win, color);                             \
    mv_add_nstr(win, row, col, str, len);                 \
    win_color_off(win, color)

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
#define mvwaddchwattr(win, row, col, char, attributes) \
    wattron(win, attributes);                          \
    mvwaddch(win, row, col, char);                     \
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

#define render_str_len(start, str, len, c)      midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, start), str, len, c)

#define rendr_len(start, len, color)            midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, start), start, len, color)

/* Should only be used in 'render.cpp', this is to simplify.  See 'render.cpp'.
 * this uses a ptr called start that should point to the first char.  And one
 * called end, witch should point to the last char.  */
#define rendr_se_ptr(color)                     midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, start), start, (end - start), color);

#define rendr_ptr_ptr(start, end, color) \
    midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, start), start, (end - start), color);

#define rendr_ch_ptr(color)            midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, ch), ch, 1, color);
#define rendr_ch_str_ptr(start, color) midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, start), start, 1, color)

#define render_node_str(node, color) \
    midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, color);

#define IS_PTR(x) _Generic((x), char *: 1, const char *: 1, int *: 1, void *: 1, default: 0)

/* New framework. */
#define PP_RSEQ_N()                                                                                                   \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36,   \
        35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, \
        7, 6, 5, 4, 3, 2, 1, 0
#define PP_NARG(...)  PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, \
                 _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42,  \
                 _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62,  \
                 _63, N, ...)                                                                                         \
    N
#define PP_CAT_I(a, b)             a##b
#define PP_CAT(a, b)               PP_CAT_I(a, b)

/* Raw ptr rendering. */
#define R_3(color, start, end)     midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, start), start, (end - start), color)
/* Raw ptr rendering with len. */
#define R_LEN_3(color, ptr, len)   midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, ptr), ptr, len, color)
/* Render one char based on char_ptr. */
#define R_CHAR_2(color, char_ptr)  midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, char_ptr), char_ptr, 1, color)
/* Index based rendering. */
#define C_3(color, start, end)     render_part(start, end, color)
/* Index based rendering derived from ptr pos in line. */
#define C_PTR_3(color, start, end) render_part((start - line->data), (end - line->data), color)
/* Print a warning at end of line. */
#define W_1(str)                   midwin_mv_add_nstr_color(row, wideness(line->data, till_x) + margin + 1, str, str##_sllen, FG_YELLOW)
/* Print a error at end of line. */
#define E_1(str) \
    midwin_mv_add_nstr_color(row, wideness(line->data, till_x) + margin + 1, str, str##_sllen, FG_VS_CODE_RED)
#define SUGGEST_1(str)        \
    midwin_mv_add_nstr_color( \
        openfile->cursor_row, xplustabs() + margin, str + suggest_len, strlen(str) - suggest_len, FG_SUGGEST_GRAY)

/* Main rendering caller. */
#define rendr(opt, ...) PP_CAT(opt##_, PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#define RENDR(opt, ...) PP_CAT(opt##_, PP_NARG(__VA_ARGS__))(__VA_ARGS__)
