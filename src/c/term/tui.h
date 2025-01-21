#pragma once

#include "../../../config.h"
#include "../../include/c_defs.h"

_BEGIN_C_LINKAGE

extern int tty_fd;
extern int NLINES;
extern int NCOLS;

#include <unibilium.h>

int find_ext_str_idx(unibi_term *term, const char *name);

int find_ext_bool_idx(unibi_term *term, const char *name);

bool has_truecolor(unibi_term *term);

void set_if_empty(unibi_term *term, enum unibi_string string, const char *val);

void tui_update_size(void);

/* When visable is `TRUE`, show the cursor.  And when `FALSE`, hide it. */
void tui_curs_visible(bool visible);

int open_tty(void);

void tui_init(void);

void tui_scroll_region(int top_row, int bot_row);

void tui_set_curs_blink(bool enable);

_END_C_LINKAGE
