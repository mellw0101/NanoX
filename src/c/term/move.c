/** @file move.c

  @author Melwin Svensson.
  @date   14-1-2025.

 */
#include "move.h"

#include "../../include/c_defs.h"
#include "../event/nfdwriter.h"
#include "terminfo.h"

#include <unistd.h>
#include <string.h>

void tui_clear_screen(void) {
  nfdwriter_write_stdout(terminfo->clear_screen_str, strlen(terminfo->clear_screen_str));
}

void tui_move(int row, int column) {
  char buffer[256];
  unibi_var_t vars[9] = {0};
  vars[0].i_ = row;
  vars[1].i_ = column;
  Ulong len = unibi_run(terminfo->cursor_address_str, vars, buffer, sizeof(buffer));
  if (!len) {
    return;
  }
  nfdwriter_write_stdout(buffer, len);
}

void tui_set_foreground_color(Uchar r, Uchar g, Uchar b) {
  if (terminfo->ext.set_rgb_foreground == -1) {
    return;
  }
  char buffer[256];
  unibi_var_t vars[9] = {0};
  vars[0].i_ = r;
  vars[1].i_ = g;
  vars[2].i_ = b;
  const char *str = unibi_get_ext_str(terminfo->term, terminfo->ext.set_rgb_foreground);
  Ulong len = unibi_run(str, vars, buffer, sizeof(buffer));
  if (!len) {
    return;
  }
  nfdwriter_write_stdout(buffer, len);
}

void tui_set_background_color(Uchar r, Uchar g, Uchar b) {
  if (terminfo->ext.set_rgb_background == -1) {
    return;
  }
  char buffer[256];
  unibi_var_t vars[9] = {0};
  vars[0].i_ = r;
  vars[1].i_ = g;
  vars[2].i_ = b;
  const char *str = unibi_get_ext_str(terminfo->term, terminfo->ext.set_rgb_background);
  Ulong len = unibi_run(str, vars, buffer, sizeof(buffer));
  if (!len) {
    return;
  }
  nfdwriter_write_stdout(buffer, len);
}

void tui_reset_color(void) {
  nfdwriter_write_stdout(S__LEN("\x1b[0m"));
}

void tui_enable_bracketed_pastes(void) {
  nfdwriter_write_stdout(S__LEN("\x1b[?2004h"));
}

void tui_disable_bracketed_pastes(void) {
  nfdwriter_write_stdout(S__LEN("\x1b[?2004l"));
}
