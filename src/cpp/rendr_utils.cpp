#include "../include/prototypes.h"

void get_next_word(const char **start, const char **end) {
  adv_ptr((*end), (*(*end) == ' ' || *(*end) == '\t'));
  *start = *end;
  adv_ptr((*end), (*(*end) != ' ' && *(*end) != '\t'));
}

/* Search for any match to the current suggest buf. */
void find_suggestion(void) {}

/* Clear suggest buffer and len as well as setting the
 * current suggest str to nullptr. */
void clear_suggestion(void) {
  suggest_on               = false;
  suggest_str              = nullptr;
  suggest_len              = 0;
  suggest_buf[suggest_len] = '\0';
}

/* Add last typed char to the suggest buffer. */
void add_char_to_suggest_buf(void) {
  if (openfile->current_x > 0) {
    const char *c = &openfile->current->data[openfile->current_x - 1];
    if (is_word_char(c - 1, false)) {
      suggest_len     = 0;
      const Ulong pos = word_index(true);
      for (int i = pos; i < openfile->current_x - 1; suggest_len++, i++) {
        suggest_buf[suggest_len] = openfile->current->data[i];
      }
      suggest_buf[suggest_len] = '\0';
    }
    suggest_buf[suggest_len++] = *c;
    suggest_buf[suggest_len]   = '\0';
  }
}

/* Draw current suggestion if found to the suggest window. */
void draw_suggest_win(void) {
  if (!suggest_str) {
    return;
  }
  Ulong col_len = strlen(suggest_str) + 2;
  Ulong row_len = 1;
  Ulong row_pos = (openfile->cursor_row > editwinrows - 2) ? openfile->cursor_row - row_len : openfile->cursor_row + 1;
  Ulong col_pos = xplustabs() + margin - suggest_len - 1;
  suggestwin    = newwin(row_len, col_len, row_pos, col_pos);
  mvwprintw(suggestwin, 0, 1, "%s", suggest_str);
  wrefresh(suggestwin);
  if (ISSET(SUGGEST_INLINE)) {
    RENDR(SUGGEST, suggest_str);
  }
}

/* Parse a function declaration that is over multiple lines. */
char *parse_split_decl(linestruct *line) {
  char       *data = nullptr;
  const char *p    = strchr(line->data, ')');
  if (!p) {
    return nullptr;
  }
  line            = line->prev;
  char *cur_data  = copy_of(line->data);
  char *next_data = copy_of(line->next->data);
  p               = next_data;
  for (; *p && (*p == ' ' || *p == '\t'); p++);
  char *tp = copy_of(p);
  free(next_data);
  next_data = tp;
  append_str(&cur_data, " ");
  data = alloc_str_free_substrs(cur_data, next_data);
  if (!line->prev->data[0]) {
    free(data);
    return nullptr;
  }
  char *ret_t = copy_of(line->prev->data);
  append_str(&ret_t, " ");
  char *ret = alloc_str_free_substrs(ret_t, data);
  return ret;
}

/* Return the correct line to start parsing function delc.
 * if '{' is on 'line' then we simply return 'line',
 * else we iterate until we find the first line
 * after '{' line that has text on it. */
linestruct *get_func_decl_last_line(linestruct *line) {
  const char *p = strchr(line->data, '{');
  if (p && (p == line->data || *p == line->data[indent_char_len(line)])) {
    do {
      line = line->prev;
    }
    while (!line->data[indent_char_len(line)]);
  }
  return line;
}

/* Parse function signature. */
char *parse_function_sig(linestruct *line) {
  const char *p           = nullptr;
  const char *param_start = nullptr;
  /* If the bracket is alone on a line then go to prev line. */
  line = get_func_decl_last_line(line);
  /* If the line does not contain '(', so it must be a split decl. */
  param_start = strchr(line->data, '(');
  if (!param_start) {
    return parse_split_decl(line);
  }
  p           = strchr(line->data, ' ');
  char *sig   = nullptr;
  char *ret_t = nullptr;
  char *ret   = nullptr;
  if (p && p < (param_start - 1)) {
    if (p == line->data) {
      for (; *p && (*p == ' ' || *p == '\t'); p++);
      sig = copy_of(p);
    }
    else {
      ret = copy_of(line->data);
    }
  }
  else {
    sig = copy_of(line->data);
    for (int i = 0; sig[i] && i < (param_start - line->data); i++) {
      if (sig[i] == ' ' || sig[i] == '\t') {
        alloced_remove_at(&sig, i);
      }
    }
  }
  if (!ret) {
    if (!line->prev->data[0]) {
      return nullptr;
    }
    ret_t = copy_of(line->prev->data);
    append_str(&ret_t, " ");
    ret = alloc_str_free_substrs(ret_t, sig);
  }
  return ret;
}

/* Inject a suggestion. */
void accept_suggestion(void) {
  if (suggest_str != nullptr) {
    inject(suggest_str + suggest_len, strlen(suggest_str) - suggest_len);
  }
  clear_suggestion();
}

void find_word(linestruct *line, const char *data, const char *word, const Ulong slen, const char **start,
               const char **end) {
  *start = strstr(data, word);
  if (*start) {
    *end = (*start) + slen;
    if (!is_word_char(&line->data[((*end) - line->data)], false) &&
        (*start == line->data || (!is_word_char(&line->data[((*start) - line->data) - 1], false) &&
                                  line->data[((*start) - line->data) - 1] != '_'))) {}
    else {
      *start = nullptr;
    }
  }
  else {
    *end = nullptr;
  }
}

void free_local_var(local_var_t *var) {
  if (var->type) {
    free(var->type);
  }
  if (var->name) {
    free(var->name);
  }
  if (var->value) {
    free(var->value);
  }
}

local_var_t parse_local_var(linestruct *line) {
  local_var_t var;
  var.type         = nullptr;
  var.name         = nullptr;
  var.value        = nullptr;
  const char *end  = nullptr;
  const char *data = nullptr;
  data             = &line->data[indent_char_len(line)];
  end              = strchr(data, ';');
  if (end) {
    char       *str     = measured_copy(&line->data[indent_char_len(line)], (end - data) + 1);
    const char *bracket = strchr(str, '{');
    if (bracket) {
      free(str);
      return var;
    }
    char *type, *name, *value;
    parse_variable(str, &type, &name, &value);
    if (type) {
      var.type = copy_of(type);
      free(type);
    }
    if (name) {
      var.name = copy_of(name);
      free(name);
    }
    if (value) {
      var.value = copy_of(value);
      free(value);
    }
    if (!(!type && !name && !value)) {
      int lvl       = 0;
      var.decl_line = line->lineno;
      for (linestruct *l = line; l; l = l->next) {
        if ((l->flags.is_set(BRACKET_START))) {
          lvl += 1;
        }
        if ((l->flags.is_set(BRACKET_END))) {
          if (lvl == 0) {
            var.scope_end = l->lineno;
            break;
          }
          lvl -= 1;
        }
      }
    }
    free(str);
  }
  return var;
}

int find_class_end_line(linestruct *from) {
  int         lvl     = 0;
  const char *b_start = nullptr;
  const char *b_end   = nullptr;
  for (linestruct *line = from; line; line = line->next) {
    b_start = line->data;
    do {
      b_start = strchr(b_start, '{');
      if (b_start) {
        if (!(line->data[(b_start - line->data) - 1] == '\'' && line->data[(b_start - line->data) + 1] == '\'')) {
          lvl += 1;
        }
        b_start += 1;
      }
    }
    while (b_start);
    b_end = line->data;
    do {
      b_end = strchr(b_end, '}');
      if (b_end) {
        if (!(line->data[(b_end - line->data) - 1] == '\'' && line->data[(b_end - line->data) + 1] == '\'')) {
          if (lvl == 0) {
            break;
          }
          lvl -= 1;
        }
        b_end += 1;
      }
    }
    while (b_end);
    if (strchr(line->data, ';') && lvl == 0) {
      return line->lineno;
    }
  }
  return -1;
}

/* Add entry to color map and remove any entry that has the same name. */
void add_rm_color_map(string str, syntax_data_t data) {
  auto it = test_map.find(str);
  if (it != test_map.end() && it->second.color == data.color && it->second.type == data.type) {
    test_map.erase(str);
  }
  test_map[str] = data;
}
