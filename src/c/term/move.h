#pragma once

#include "../../../config.h"
#include "../../include/c_defs.h"

_BEGIN_C_LINKAGE

void tui_clear_screen(void);
void tui_move(int row, int column);
void tui_set_foreground_color(Uchar r, Uchar g, Uchar b);
void tui_set_background_color(Uchar r, Uchar g, Uchar b);
void tui_reset_color(void);
void tui_enable_bracketed_pastes(void);
void tui_disable_bracketed_pastes(void);

_END_C_LINKAGE
