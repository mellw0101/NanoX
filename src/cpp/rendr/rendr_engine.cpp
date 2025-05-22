#include "../../include/prototypes.h"

/* Set '_instance' to 'nullptr' to init it. */
RendrEngine *RendrEngine::_instance = nullptr;

/* Destroy RendrEngine instance. */
void RendrEngine::_destroy(void) noexcept {
  delete _instance;
}

/* Render a single line and also matching brackets. */
void RendrEngine::_render_bracket(int row, linestruct *line) {
  const char *st = strchr(line->data, '{');
  if (st) {
    midwin_mv_add_nstr_color(row, ((st - line->data) + margin), "{", 1, FG_VS_CODE_YELLOW);
    unix_socket_debug("Found st: %s\n", line->data);
    Ulong       end_idx;
    linestruct *end_line;
    if (find_end_bracket(line, (st - line->data), &end_line, &end_idx)) {
      unix_socket_debug("Found end: %s\n", end_line->data);
      int end_row = get_editwin_row(end_line);
      if (end_row != -1) {
        unix_socket_debug("Found end row: %d\n", end_row);
        midwin_mv_add_nstr_color(end_row, (end_idx + margin), "}", 1, FG_VS_CODE_YELLOW);
      }
    }
  }
}

void RendrEngine::_brackets(void) {
  int row = 0;
  linestruct *line = openfile->edittop;
  while (row < editwinrows && line) {
    const char *st = strchr(line->data, '{');
    if (st) {
      midwin_mv_add_nstr_color(row, ((st - line->data) + margin), "{", 1, FG_VS_CODE_YELLOW);
      Ulong end_idx;
      linestruct *end_line;
      if (find_end_bracket(line, (st - line->data), &end_line, &end_idx)) {
        int end_row = get_editwin_row(end_line);
        if (end_row != -1) {
          midwin_mv_add_nstr_color(end_row, (end_idx + margin), "}", 1, FG_VS_CODE_YELLOW);
        }
        row = end_row;
        line = end_line;
      }
    }
    line = line->next;
    ++row;
  }
}

RendrEngine *RendrEngine::instance(void) noexcept {
  if (!_instance) {
    _instance = new (std::nothrow) RendrEngine();
    if (!_instance) {
      logE("Failed to alloc 'RendrEngine'.");
      exit(1);
    }
    atexit(_destroy);
  }
  return _instance;
}

void RendrEngine::whole_editwin(void) {
  PROFILE_FUNCTION;
  int         row  = 0;
  linestruct *line = openfile->edittop;
  while (row < editwinrows && line) {
    char *converted = display_string(line->data, 0, editwincols, true, false);
    Ulong from_col = get_page_start(wideness(line->data, (line == openfile->current) ? openfile->current_x : 0), editwincols);
    render_line_text(row, converted, line, from_col);
    free(converted);
    line = line->next;
    ++row;
  }
  _brackets();
}
