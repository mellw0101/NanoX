#include "../include/prototypes.h"

/* Returns 'TRUE' when a line is a '//' comment. */
bool is_line_comment(linestruct *line) {
  for (Uint i = indent_char_len(line); line->data[i]; i++) {
    if (!line->data[i + 1]) {
      break;
    }
    if (line->data[i] == '/' && line->data[i + 1] == '/') {
      return TRUE;
    }
  }
  return FALSE;
}

bool is_line_start_end_bracket(linestruct *line, bool *is_start) {
  Ulong i;
  for (i = 0; line->data[i]; i++) {
    if (line->data[i] == '{') {
      if (line->data[i + 1]) {
        if (line->data[i + 1] == '}') {
          return FALSE;
        }
      }
      *is_start = TRUE;
      return TRUE;
    }
    else if (line->data[i] == '}') {
      *is_start = FALSE;
      return TRUE;
    }
  }
  return FALSE;
}

/* Inject a string into a line at an index. */
void inject_in_line(linestruct **line, const char *str, Ulong at) {
  Ulong len   = strlen((*line)->data);
  Ulong s_len = strlen(str);
  if (at > len) {
    return;
  }
  (*line)->data = arealloc((*line)->data, (len + s_len + 1));
  memmove(((*line)->data + at + s_len), (*line)->data + at, (len - at + 1));
  memmove(((*line)->data + at), str, s_len);
}

/* Move a single line up or down. */
void move_line(linestruct **line, bool up, bool refresh) {
  if (openfile->mark) {
    if (openfile->current != openfile->mark) {
      return;
    }
  }
  char *tmp_data = NULL;
  if (up) {
    if ((*line)->prev) {
      tmp_data = copy_of((*line)->prev->data);
      free((*line)->prev->data);
      (*line)->prev->data = copy_of((*line)->data);
      free((*line)->data);
      (*line)->data     = tmp_data;
      openfile->current = (*line)->prev;
    }
  }
  else {
    if ((*line)->next) {
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
    refresh_needed = TRUE;
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
    if (top->prev) {
      for (line = top->prev; line->lineno != bot_line; line = line->next) {
        move_line(&line, FALSE, FALSE);
      }
      mark = mark->prev;
      cur  = cur->prev;
      NLOG("%lu\n%s\n%lu\n%s\n", x_cur, cur->data, x_mark, mark->data);
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
  add_undo(MOVE_LINE_UP, NULL);
  move_line(&openfile->current, TRUE, TRUE);
}

/* Function to move line/lines down shortcut. */
void move_lines_down(void) {
  if (!openfile->current->next) {
    return;
  }
  add_undo(MOVE_LINE_DOWN, NULL);
  move_line(&openfile->current, FALSE, TRUE);
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

linestruct *find_next_bracket(bool up, linestruct *from_line) {
  if (up) {
    for (linestruct *line = from_line; line; line = line->prev) {
      if (!(line->flags.is_set(IN_BRACKET))) {
        return NULL;
      }
      else if ((line->flags.is_set(BRACKET_END))) {
        return line;
      }
      else if ((line->flags.is_set(BRACKET_START))) {
        return line;
      }
    }
  }
  return NULL;
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
  tabs += (spaces / tabsize);
  return tabs;
}

/* Returns '-1' on failure. */
int get_editwin_row(linestruct *line) {
  int row = line->lineno - openfile->edittop->lineno;
  if (row >= editwinrows) {
    return -1;
  }
  return row;
}

Uint indent_tab_len(linestruct *line) {
  Uint i = 0, spaces = 0, tabs = 0;
  for (; line->data[i]; ++i) {
    if (line->data[i] == ' ') {
      ++spaces;
    }
    else if (line->data[i] == '\t') {
      ++tabs;
    }
    else {
      break;
    }
  }
  return (tabs + (spaces / tabsize));
}