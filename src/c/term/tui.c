#include "tui.h"

#include "terminfo.h"
#include "../event/nfdwriter.h"
#include "../event/nevhandler.h"

#include <fcntl.h>
#include <unibilium.h>
#include <sys/ioctl.h>

int tty_fd = -1;
int NLINES = 0;
int NCOLS  = 0;

int find_ext_str_idx(unibi_term *term, const char *name) {
  Ulong max_ext = unibi_count_ext_str(term);
  for (Ulong i = 0; i < max_ext; ++i) {
    const char *ext_name = unibi_get_ext_str_name(term, i);
    if (ext_name && strcmp(ext_name, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

int find_ext_bool_idx(unibi_term *term, const char *name) {
  Ulong max_ext = unibi_count_ext_bool(term);
  for (Ulong i = 0; i < max_ext; ++i) {
    const char *ext_name = unibi_get_ext_bool_name(term, i);
    if (ext_name && strcmp(ext_name, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

bool has_truecolor(unibi_term *term) {
  Ulong bool_ext_max = unibi_count_ext_bool(term);
  for (Ulong i = 0; i < bool_ext_max; ++i) {
    const char *bool_ext_name = unibi_get_ext_bool_name(term, i);
    if (bool_ext_name && (strcmp(bool_ext_name, "Tc") == 0 || strcmp(bool_ext_name, "RGB") == 0)) {
      return TRUE;
    }
  }
  bool setrgbb = FALSE;
  bool setrgbf = FALSE;
  Ulong str_ext_max = unibi_count_ext_str(term);
  for (Ulong i = 0; i < str_ext_max; ++i) {
    const char *str_ext_name = unibi_get_ext_str_name(term, i);
    if (str_ext_name) {
      if (!setrgbf && strcmp(str_ext_name, "setrgbf") == 0) {
        setrgbf = TRUE;
      }
      else if (!setrgbb && strcmp(str_ext_name, "setrgbb") == 0) {
        setrgbb = TRUE;
      }
    }
  }
  return (setrgbf && setrgbb);
}

void set_if_empty(unibi_term *term, enum unibi_string string, const char *val) {
  if (!unibi_get_str(term, string)) {
    unibi_set_str(term, string, val);
  }
}

void tui_update_size(void) {
  struct winsize size;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1) {
    NLINES = -1;
    NCOLS  = -1;
    return;
  }
  NLINES = size.ws_row;
  NCOLS  = size.ws_col;
}

void tui_curs_visible(bool visible) {
  if ((visible && terminfo->cursor_visible_str)) {
    nfdwriter_write_stdout(terminfo->cursor_visible_str, strlen(terminfo->cursor_visible_str));
  }
  else if (!visible && terminfo->cursor_invisible_str) {
    nfdwriter_write_stdout(terminfo->cursor_invisible_str, strlen(terminfo->cursor_invisible_str));
  }
}

int open_tty(void) {
  return open("/dev/tty", (O_RDWR | O_NOCTTY));
}

void tui_init(void) {
  tui_handler = nevhandler_create();
  /* Init the standard out writer. */
  nfdwriter_stdout = nfdwriter_create(STDOUT_FILENO);
  /* Init the terminfo we need. */
  terminfo_init();
}

void tui_scroll_region(int top_row, int bot_row) {
  nfdwriter_printf(nfdwriter_stdout, "\233%d;%dr", top_row, bot_row);
}

void tui_set_curs_blink(bool enable) {
  nfdwriter_write_stdout(S__LEN(enable ? "\x1b[5m" : "\x1b[25m"));
}
