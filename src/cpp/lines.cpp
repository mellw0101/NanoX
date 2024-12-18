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
void inject_in_line(linestruct *line, const char *str, Ulong at) {
  Ulong len   = strlen(line->data);
  Ulong s_len = strlen(str);
  if (at > len) {
    return;
  }
  line->data = arealloc(line->data, (len + s_len + 1));
  memmove((line->data + at + s_len), line->data + at, (len - at + 1));
  memmove((line->data + at), str, s_len);
}

/* Move a single line up or down. */
void move_line(linestruct *line, bool up) {
  char *tmp_data = NULL;
  if (up && line->prev) {
    tmp_data         = line->prev->data;
    line->prev->data = line->data;
    line->data       = tmp_data;
  }
  else if (!up && line->next) {
    tmp_data         = line->next->data;
    line->next->data = line->data;
    line->data       = tmp_data;
  }
  else {
    return;
  }
  set_modified();
  refresh_needed = TRUE;
}

/* Function to move line/lines up shortcut. */
void move_lines_up(void) {
  /* Multi line move. */
  if (openfile->mark && openfile->mark != openfile->current) {
    bool mark_top = mark_is_before_cursor();
    linestruct *top = (mark_top ? openfile->mark : openfile->current);
    linestruct *bot = (mark_top ? openfile->current : openfile->mark);
    if (top->lineno == 1) {
      return;
    }
    add_undo(MOVE_LINE_UP, NULL);
    Ulong end_line = bot->lineno;
    for (linestruct *line = top->prev; line->lineno != end_line; line = line->next) {
      move_line(line, FALSE);
    }
    openfile->current = openfile->current->prev;
    openfile->mark    = openfile->mark->prev;
    keep_mark = TRUE;
  }
  /* Single line move. */
  else {
    /* We cannot move the first line up. */
    if (openfile->current->lineno == 1) {
      return;
    }
    add_undo(MOVE_LINE_UP, NULL);
    move_line(openfile->current, TRUE);
    openfile->current = openfile->current->prev;
    if (openfile->mark) {
      openfile->mark = openfile->current;
      keep_mark = TRUE;
    }
  }
}

/* Function to move line/lines down shortcut. */
void move_lines_down(void) {
  /* Multi line move. */
  if (openfile->mark && openfile->mark != openfile->current) {
    bool mark_top = mark_is_before_cursor();
    linestruct *top = (mark_top ? openfile->mark : openfile->current);
    linestruct *bot = (mark_top ? openfile->current : openfile->mark);
    if (!bot->next) {
      return;
    }
    add_undo(MOVE_LINE_DOWN, NULL);
    Ulong end_line = top->lineno;
    for (linestruct *line = bot->next; line->lineno != end_line; line = line->prev) {
      move_line(line, TRUE);
    }
    openfile->current = openfile->current->next;
    openfile->mark    = openfile->mark->next;
    keep_mark = TRUE;
  }
  /* Single line move. */
  else {
    /* We cannot move the last line down. */
    if (!openfile->current->next) {
      return;
    }
    add_undo(MOVE_LINE_DOWN, NULL);
    move_line(openfile->current, FALSE);
    openfile->current = openfile->current->next;
    if (openfile->mark) {
      openfile->mark = openfile->current;
      keep_mark = TRUE;
    }
  }
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
  int row = (line->lineno - openfile->edittop->lineno);
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