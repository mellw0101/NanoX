#include "../../../include/c_proto.h"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


_UNUSED
static void curses_render_line_text(int row, const char *const restrict data, linestruct *line, Ulong from_col) {
  ASSERT(data);
  ASSERT(line);
  /* If line-numbers are enabled. */
  if (margin > 0) {
    wattron(midwin, interface_color_pair[coloridx_linenumber /* config->linenumber.color */]);
    if (ISSET(SOFTWRAP) && from_col) {
      mvwprintw(midwin, row, 0, "%*s", (margin - 1), " ");
    }
    else {
      mvwprintw(midwin, row, 0, "%*lu", (margin - 1), line->lineno);
    }
    wattroff(midwin, interface_color_pair[coloridx_linenumber /* config->linenumber.color */]);
    if (line->has_anchor && (!from_col || !ISSET(SOFTWRAP))) {
      if (using_utf8()) {
        wprintw(midwin, "\xE2\xAC\xA5");
      }
      else {
        wprintw(midwin, "+");
      } 
    }
    else {
      if (verticalbar /* config->linenumber.verticalbar || config->linenumber.fullverticalbar */) {
        wattron(midwin, interface_color_pair[coloridx_linenumber_bar /* config->linenumber.barcolor */]);
        waddch(midwin, ACS_VLINE);
        wattroff(midwin, interface_color_pair[coloridx_linenumber_bar /* config->linenumber.barcolor */]);
      }
      else {
        wprintw(midwin, " ");
      }
    }
  }
  mvwaddnstr(midwin, row, margin, data, strlen(data));
  if (is_shorter || ISSET(SOFTWRAP)) {
    wclrtoeol(midwin);
  }
  /* Only draw the sidebar when more then a screenful of rows exists. */
  if (sidebar && TUI_OF->filebot->lineno > editwinrows) {
    mvwaddch(midwin, row, (COLS - 1), bardata[row]);
  }
}
