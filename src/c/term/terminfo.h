#pragma once

#include "../../../config.h"
#include "../../include/c_defs.h"

_BEGIN_C_LINKAGE

#include <unibilium.h>

typedef struct {
  unibi_term *term;
  const char *term_env;
  int max_colors_num;
  const char *clear_screen_str;
  const char *clr_eol_str;
  const char *clr_eos_str;
  const char *cursor_left_str;
  const char *cursor_right_str;
  const char *cursor_up_str;
  const char *cursor_down_str;
  const char *cursor_address_str;
  const char *initialize_color_str;
  const char *initialize_pair_str;
  const char *set_color_pair_str;
  const char *cursor_invisible_str;
  const char *cursor_visible_str;
  const char *scroll_forward_str;
  const char *scroll_reverse_str;
  const char *bell_str;
  struct {
    int set_rgb_background;
    int set_rgb_foreground;
    bool truecolor;
  } ext;
  char backspace_key;
  bool on_a_vt;
} terminfo_t;

extern terminfo_t *terminfo;

void try_get_env_terminfo(void);

void terminfo_init(void);

_END_C_LINKAGE
