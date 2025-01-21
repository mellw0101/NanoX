#pragma once

#include "../../../config.h"
#include "../../include/c_proto.h"

_BEGIN_C_LINKAGE

typedef struct nwindow nwindow;

nwindow *nwindow_create(short rows, short columns, short screen_row, short screen_column);

void nwindow_free(nwindow *window);

/* Return's `FALSE`, if `row` or `column` is outside of the `window` scope.  This function is `NULL-SAFE`. */
bool nwindow_move(nwindow *window, short row, short column);

void nwindow_add_nstr(nwindow *window, const char *string, Ulong len);

void nwindow_add_str(nwindow *window, const char *string);

void nwindow_add_ch(nwindow *window, const char c);

void nwindow_printf(nwindow *window, const char *format, ...) __attribute__((__format__(__printf__, 2, 3)));

/* Set the foreground and the background by encoding the data into `encoded_fg` and `encoded_bg`, 
 * by setting either to `-1`, the color can be disabled. */
void nwindow_set_rgb(nwindow *window, int encoded_fg, int encoded_bg);

/* See `nwindow_set_rgb_int()`.  This just encodes the int inline, then calls `nwindow_set_rgb_int()`. */
void nwindow_set_rgb_code(nwindow *window, Uchar r_fg, Uchar g_fg, Uchar b_fg, Uchar r_bg, Uchar g_bg, Uchar b_bg);

void nwindow_clrtoeol(nwindow *window);

void nwindow_redrawl(nwindow *window, short row);

void nwindow_redrawln(nwindow *window, short from, short howmeny);

void nwindow_scroll(nwindow *window, int howmush);

/* Macro's to do some chained things. */

#define nwindow_move_add_nstr(window, row, column, string, len) \
  (!nwindow_move((window), (row), (column)) ? (void)0 : nwindow_add_nstr((window), (string), (len)))

#define nwindow_move_add_str(window, row, column, string) \
  (!nwindow_move((window), (row), (column)) ? (void)0 : nwindow_add_str((window), (string)))

#define nwindow_move_printf(window, row, column, ...) \
  (!nwindow_move((window), (row), (column)) ? (void)0 : nwindow_printf((window), __VA_ARGS__))

#define nwindow_move_add_ch(window, row, column, ch) \
  (!nwindow_move((window), (row), (column)) ? (void)0 : nwindow_add_ch((window), (ch)))

_END_C_LINKAGE
