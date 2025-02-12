#include "../../include/c_proto.h"
#include "terminfo.h"
#include "tui.h"

#include <stdlib.h>
#include <sys/ioctl.h>
#include <term.h>
#include <sgtty.h>
#include <linux/vt.h>
#include <unibilium.h>

terminfo_t *terminfo = NULL;

void try_get_env_terminfo(void) {
  terminfo->term = unibi_from_env();
  if (!terminfo->term) {
    die("%s: Failed to get terminfo from env", __func__);
  }
}

static const char *get_bsp_key(void) {
  static char buffer[2] = {0};
  struct termios t = {0};
  if (tcgetattr(STDIN_FILENO, &t) == -1) {
    die("%s: Failed to get termios attr.\n", __func__);
  }
  buffer[0] = (char)t.c_cc[VERASE];
  buffer[1] = NUL;
  return buffer;
}

void terminfo_init(void) {
#define UNIBI_STR(name) terminfo->name##_str = unibi_get_str(terminfo->term, unibi_##name)
#define UNIBI_NUM(name) terminfo->name##_num = unibi_get_num(terminfo->term, unibi_##name)
  terminfo = xmalloc(sizeof(*terminfo));
  try_get_env_terminfo();
  if (terminfo->term) {
    terminfo->term_env = getenv("TERM");
    terminfo->max_colors_num = unibi_get_num(terminfo->term, unibi_max_colors);
    if (!terminfo->max_colors_num) {
      terminfo->max_colors_num = 16;
    }
    UNIBI_STR(clear_screen);
    UNIBI_STR(clr_eol);
    UNIBI_STR(clr_eos);
    UNIBI_STR(cursor_address);
    UNIBI_STR(initialize_color);
    UNIBI_STR(initialize_pair);
    UNIBI_STR(set_color_pair);
    UNIBI_STR(cursor_invisible);
    UNIBI_STR(cursor_visible);
    UNIBI_STR(scroll_forward);
    UNIBI_STR(scroll_reverse);
    UNIBI_STR(bell);
    terminfo->ext.truecolor = has_truecolor(terminfo->term);
    terminfo->ext.set_rgb_foreground = find_ext_str_idx(terminfo->term, "setrgbf");
    if (terminfo->ext.set_rgb_foreground == -1) {
      terminfo->ext.set_rgb_foreground = (int)unibi_add_ext_str(terminfo->term, "setrgbf", "\x1b[38:2:%p1%d:%p2%d:%p3%dm");
    }
    terminfo->ext.set_rgb_background = find_ext_str_idx(terminfo->term, "setrgbb");
    if (terminfo->ext.set_rgb_background == -1) {
      terminfo->ext.set_rgb_background = (int)unibi_add_ext_str(terminfo->term, "setrgbf", "\x1b[48:2:%p1%d:%p2%d:%p3%dm");
    }
    /* Get some keycodes. */
    if (strcmp(terminfo->term_env, "xterm") == 0) {
      terminfo->backspace_key = 0x08;
    }
    else {
      terminfo->backspace_key = *get_bsp_key();
    }
    struct vt_stat dummy;
    terminfo->on_a_vt = !ioctl(STDOUT_FILENO, VT_GETSTATE, &dummy);
  }
}
