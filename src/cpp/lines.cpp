#include "../include/prototypes.h"

/* Returns 'true' when a line is a '//' comment. */
bool is_line_comment(linestruct *line) {
  for (Uint i = indent_char_len(line); line->data[i]; i++) {
    if (!line->data[i + 1]) {
      break;
    }
    if (line->data[i] == '/' && line->data[i + 1] == '/') {
      return true;
    }
  }
  return false;
}
bool is_line_start_end_bracket(linestruct *line, bool *is_start) {
  Ulong i;
  for (i = 0; line->data[i]; i++) {
    if (line->data[i] == '{') {
      if (line->data[i + 1]) {
        if (line->data[i + 1] == '}') {
          return false;
        }
      }
      *is_start = true;
      return true;
    }
    else if (line->data[i] == '}') {
      *is_start = false;
      return true;
    }
  }
  return false;
}
/* Return`s 'true' if the first char in a line is '\0'. */
bool is_empty_line(linestruct *line) {
  Ulong i = 0;
  for (; line->data[i] && i < 1; i++);
  return (i == 0);
}
/* Inject a string into a line at an index. */
void inject_in_line(linestruct **line, const char *str, Ulong at) {
  Ulong len   = strlen((*line)->data);
  Ulong s_len = strlen(str);
  if (at > len) {
    return;
  }
  (*line)->data = (char *)nrealloc((*line)->data, len + s_len + 1);
  memmove((*line)->data + at + s_len, (*line)->data + at, len - at + 1);
  memmove((*line)->data + at, str, s_len);
}
Ulong get_line_total_tabs(linestruct *line) {
  Ulong i, tabs = 0, spaces = 0, total = 0;
  if (line->data[0] != ' ' && line->data[0] != '\t') {
    return 0;
  }
  for (i = 0; line->data[i]; i++) {
    if (line->data[i] == ' ') {
      spaces++;
    }
    else if (line->data[i] == '\t') {
      tabs++;
    }
  }
  total = tabs;
  if (spaces) {
    total += spaces / tabsize;
  }
  return total;
}
/* Move a single line up or down. */
void move_line(linestruct **line, bool up, bool refresh) {
  if (openfile->mark) {
    if (openfile->current != openfile->mark) {
      return;
    }
  }
  char *tmp_data = nullptr;
  if (up == true) {
    if ((*line)->prev != nullptr) {
      tmp_data = copy_of((*line)->prev->data);
      free((*line)->prev->data);
      (*line)->prev->data = copy_of((*line)->data);
      free((*line)->data);
      (*line)->data     = tmp_data;
      openfile->current = (*line)->prev;
    }
  }
  else {
    if ((*line)->next != nullptr) {
      tmp_data = copy_of((*line)->next->data);
      free((*line)->next->data);
      (*line)->next->data = copy_of((*line)->data);
      free((*line)->data);
      (*line)->data     = tmp_data;
      openfile->current = (*line)->next;
    }
  }
  set_modified();
  if (refresh) {
    refresh_needed = true;
  }
}
void move_lines(bool up) {
  linestruct *top, *bot, *line, *mark, *cur;
  Ulong       x_top, x_bot, bot_line, x_mark, x_cur;
  get_region(&top, &x_top, &bot, &x_bot);
  if (top == bot) {
    return;
  }
  mark   = openfile->mark;
  x_mark = openfile->mark_x;
  cur    = openfile->current;
  x_cur  = openfile->current_x;
  if (up) {
    bot_line = bot->lineno;
    if (top->prev != nullptr) {
      for (line = top->prev; line->lineno != bot_line; line = line->next) {
        move_line(&line, false, false);
      }
      mark = mark->prev;
      cur  = cur->prev;
      NETLOGGER.log("%lu\n%s\n%lu\n%s\n", x_cur, cur->data, x_mark, mark->data);
      openfile->mark      = mark;
      openfile->mark_x    = x_mark;
      openfile->current   = cur;
      openfile->current_x = x_cur;
    }
  }
}
/* Function to move line/lines up shortcut. */
void move_lines_up(void) {
  if (openfile->current->lineno == 1) {
    return;
  }
  add_undo(MOVE_LINE_UP, nullptr);
  move_line(&openfile->current, true, true);
}
/* Function to move line/lines down shortcut. */
void move_lines_down(void) {
  if (!openfile->current->next) {
    return;
  }
  add_undo(MOVE_LINE_DOWN, nullptr);
  move_line(&openfile->current, false, true);
}
/* Remove 'len' of char`s 'at' pos in line. */
void erase_in_line(linestruct *line, Ulong at, Ulong len) {
  Ulong slen = strlen(line->data);
  if (at + len > slen) {
    return;
  }
  char *data = (char *)nmalloc(slen - len + 1);
  memmove(data, line->data, at);
  memmove(data + at, line->data + at + len, slen - at - len + 1);
  free(line->data);
  line->data = data;
}
void select_line(linestruct *line, Ulong from_col, Ulong to_col) {
  const char *data     = line->data + actual_x(line->data, from_col);
  const Ulong paintlen = actual_x(line->data, to_col - from_col);
  wattron(midwin, interface_color_pair[SELECTED_TEXT]);
  mvwaddnstr(midwin, line->lineno, margin + from_col, data, paintlen);
  wattroff(midwin, interface_color_pair[SELECTED_TEXT]);
}
linestruct *find_next_bracket(bool up, linestruct *from_line) {
  if (up == true) {
    for (linestruct *line = from_line; line != nullptr; line = line->prev) {
      if (!(line->flags.is_set(IN_BRACKET))) {
        return nullptr;
      }
      else if ((line->flags.is_set(BRACKET_END))) {
        return line;
      }
      else if ((line->flags.is_set(BRACKET_START))) {
        return line;
      }
    }
  }
  return nullptr;
}
Uint total_tabs(linestruct *line) {
  Uint i = 0, spaces = 0, tabs = 0;
  for (; line->data[i]; i++) {
    if (line->data[i] == '\t') {
      tabs++;
    }
    else if (line->data[i] == ' ') {
      spaces++;
    }
  }
  tabs += (spaces / WIDTH_OF_TAB);
  return tabs;
}
