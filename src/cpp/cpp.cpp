#include "../include/prototypes.h"

/* Returns 'TRUE' if 'c' is a cpp syntax char. */
bool isCppSyntaxChar(const char c) {
  return (c == '<' || c == '>' || c == '&' || c == '*' || c == '=' || c == '+' || c == '-' || c == '/' || c == '%' ||
          c == '!' || c == '^' || c == '|' || c == '~' || c == '{' || c == '}' || c == '[' || c == ']' || c == '(' ||
          c == ')' || c == ';' || c == ':' || c == ',' || c == '.' || c == '?' || c == '#');
}

/* Get indent in number of 'tabs', 'spaces', 'total chars', 'total tabs (based on width of tab)'. */
void get_line_indent(linestruct *line, Ushort *tabs, Ushort *spaces, Ushort *t_char, Ushort *t_tabs) {
  Ulong i;
  *tabs = 0, *spaces = 0, *t_char = 0, *t_tabs = 0;
  if (line->data[0] != ' ' && line->data[0] != '\t') {
    return;
  }
  for (i = 0; line->data[i]; i++) {
    if (line->data[i] == ' ') {
      (*spaces)++;
    }
    else if (line->data[i] == '\t') {
      (*tabs)++;
    }
    else {
      break;
    }
  }
  *t_char = (*tabs + *spaces);
  *t_tabs = *tabs;
  if (*spaces > 0) {
    *t_tabs += (*spaces / tabsize);
  }
}

/* Return the len of indent in terms of index off first non 'tab/space' char. */
Ushort indent_char_len(linestruct *line) {
  Ushort i = 0;
  for (; line->data[i]; ++i) {
    if (line->data[i] != ' ' && line->data[i] != '\t') {
      break;
    }
  }
  return i;
}

// Check if enter is requested when betweeen '{' and '}'. if so properly
// open the brackets, place cursor in the middle and indent once.
// Return`s 'FALSE' if not between them, otherwise return`s 'TRUE'.
bool enter_with_bracket(void) {
  char        c, c_prev;
  linestruct *was_current = openfile->current, *middle, *end;
  bool        allblanks   = FALSE;
  Ulong       extra;
  if (!openfile->current->data[openfile->current_x - 1]) {
    return FALSE;
  }
  c_prev = openfile->current->data[openfile->current_x - 1];
  c      = openfile->current->data[openfile->current_x];
  if (c_prev != '{' || c != '}') {
    return FALSE;
  }
  extra = indent_length(was_current->data);
  if (extra == openfile->current_x) {
    allblanks = (indent_length(openfile->current->data) == extra);
  }
  middle       = make_new_node(openfile->current);
  middle->data = (char *)nmalloc(strlen(openfile->current->data + openfile->current_x) + extra + 1);
  /* Here we pass the first char as a ref, i.e: A ptr to the first actual
   * char. */
  strcpy(&middle->data[extra], openfile->current->data + openfile->current_x);
  if (openfile->mark == openfile->current && openfile->mark_x > openfile->current_x) {
    openfile->mark = middle;
    openfile->mark_x += extra - openfile->current_x;
  }
  strncpy(middle->data, was_current->data, extra);
  if (allblanks) {
    openfile->current_x = 0;
  }
  openfile->current->data[openfile->current_x] = '\0';
  add_undo(ENTER, NULL);
  splice_node(openfile->current, middle);
  renumber_from(middle);
  openfile->current     = middle;
  openfile->current_x   = extra;
  openfile->placewewant = xplustabs();
  openfile->totsize++;
  set_modified();
  if (ISSET(AUTOINDENT) && !allblanks) {
    openfile->totsize += extra;
  }
  update_undo(ENTER);
  /* End of 'middle' and start of 'end' */
  end       = make_new_node(openfile->current);
  end->data = (char *)nmalloc(strlen(openfile->current->data + openfile->current_x) + extra + 1);
  strcpy(&end->data[extra], openfile->current->data + openfile->current_x);
  strncpy(end->data, was_current->data, extra);
  openfile->current->data[openfile->current_x] = '\0';
  add_undo(ENTER, NULL);
  splice_node(openfile->current, end);
  renumber_from(end);
  openfile->current     = end;
  openfile->current_x   = extra;
  openfile->placewewant = xplustabs();
  openfile->totsize++;
  if (ISSET(AUTOINDENT) && !allblanks) {
    openfile->totsize += extra;
  }
  update_undo(ENTER);
  /* Place cursor at correct pos. */
  do_up();
  do_tab();
  refresh_needed = TRUE;
  focusing       = FALSE;
  return TRUE;
}

void all_brackets_pos(void) {
  linestruct *line;
  long        start_pos = -1, end_pos = -1;
  bool        is_start;
  for (line = openfile->filetop; line; line = line->next) {
    if (is_line_start_end_bracket(line, &is_start)) {
      if (is_start == TRUE) {
        start_pos = line->lineno;
      }
      else {
        if (start_pos != -1) {
          end_pos   = line->lineno;
          start_pos = -1, end_pos = -1;
        }
      }
    }
  }
}

void do_close_bracket(void) {
  // find_current_function(openfile->current);
  remove_local_vars_from(openfile->current);
  LOG_FLAG(openfile->current, IN_BRACKET);
  LOG_FLAG(openfile->current, BRACKET_START);
  LOG_FLAG(openfile->current, BRACKET_END);
  char *type, *name, *value;
  parse_variable(&openfile->current->data[indent_char_len(openfile->current)], &type, &name, &value);
  if (type) {
    nlog("type: %s\n", type);
    free(type);
  }
  if (name) {
    nlog("name: %s\n", name);
    free(name);
  }
  if (value) {
    nlog("value: %s\n", value);
    free(value);
  }
  nlog("\n");
}

void do_parse(void) {
  if (openfile->type.is_set<C_CPP>()) {
    LSP->index_file(openfile->filename);
    unix_socket_debug("got here\n");
    unix_socket_debug("include size: %u\n", LSP->index.include.size());
    unix_socket_debug("define size: %u\n", LSP->index.defines.size());
    unix_socket_debug("class size: %u\n", LSP->index.classes.size());
    unix_socket_debug("structs size: %u\n", LSP->index.structs.size());
    unix_socket_debug("typedef_structs size: %u\n", LSP->index.tdstructs.size());
    unix_socket_debug("enums size: %u\n", LSP->index.enums.size());
    unix_socket_debug("main file function size: %u\n", LSP->index.functiondefs.size());
    unix_socket_debug("main file var size: %u\n", LSP->index.vars.size());
    edit_refresh();
  }
  else if (openfile->type.is_set<BASH>()) {
    get_env_path_binaries();
  }
}

void do_test(void) {
  const char *found = nstrchr_ccpp(openfile->current->data + openfile->current_x, ';');
  if (found) {
    unix_socket_debug("found: '%s' in '%s'.\n", found, openfile->current->data);
  }
}

void do_test_window(void) {
  nlog("test win\n");
  if (suggestwin != NULL) {
    delwin(suggestwin);
  }
  suggestwin = newwin(20, 20, 0, 0);
}

int current_line_scope_end(linestruct *line) {
  int lvl      = 0;
  int cur_line = line->lineno;
  for (linestruct *l = line; l; l = l->next) {
    if (strchr(l->data, '{')) {
      lvl += 1;
    }
    if (strchr(l->data, '}')) {
      if (lvl == 0) {
        return l->lineno;
      }
      lvl -= 1;
    }
    cur_line = l->lineno;
  }
  return cur_line;
}

/* This function extracts info about a function declaration.  It retrieves all
 * the param data as well, like type, name and value.
 * TODO: 'void (*func)()' needs to be fixed. */
function_info_t *parse_func(const char *str) {
  int          i, pos;
  char        *copy        = copy_of(str);
  unsigned int len         = strlen(copy);
  char         prefix[256] = "", params[256] = "";
  for (i = 0; i < len && copy[i] != '('; i++);
  if (copy[i] != '(') {
    free(copy);
    return NULL;
  }
  pos = i;
  for (i = 0; i < pos; i++) {
    prefix[i] = copy[i];
  }
  prefix[i] = '\0';
  pos += 1;
  for (i = 0; copy[pos + i] && copy[pos + i] != ')'; i++) {
    params[i] = copy[pos + i];
  }
  if (copy[pos + i] != ')') {
    free(copy);
    return NULL;
  }
  params[i]                  = '\0';
  function_info_t *info      = (function_info_t *)nmalloc(sizeof(*info));
  info->full_function        = copy_of(str);
  info->name                 = NULL;
  info->return_type          = NULL;
  info->params               = NULL;
  info->number_of_params     = 0;
  info->attributes           = NULL;
  info->number_of_attributes = 0;
  /* Now the buffer contains all text before the '(' char. */
  const int slen  = strlen(prefix);
  int       words = 0;
  for (int i = slen; i > 0; --i) {
    if (prefix[i] == ' ' && i != slen - 1) {
      if (words++ == 0) {
        int was_i = i;
        /* Here we extract any ptr`s that are at the start of the name.
         */
        for (; prefix[i + 1] == '*'; i++);
        info->name = copy_of(&prefix[i + 1]);
        /* If any ptr`s were found we add them to the return string. */
        if (int diff = i - was_i; diff > 0) {
          for (; diff > 0; diff--, was_i++) {
            prefix[was_i] = '*';
          }
        }
        prefix[was_i] = '\0';
        i             = was_i;
        break;
      }
    }
  }
  info->return_type = copy_of(prefix);
  int    cap = 10, size = 0;
  char  *param_buf   = params;
  char **param_array = (char **)nmalloc(cap * sizeof(char *));
  for (i = 0, pos = 0; params[i]; i++) {
    if (params[i] == ',' || params[i + 1] == '\0') {
      (params[i + 1] == '\0') ? (i += 1) : 0;
      (cap == size) ? cap *= 2, param_array = (char **)nrealloc(param_array, cap * sizeof(char *)) : 0;
      param_array[size++] = measured_memmove_copy(param_buf, i - pos);
      (params[i + 1] == ' ') ? (i += 2) : (i += 1);
      param_buf += i - pos;
      pos = i;
    }
  }
  param_array[size]      = NULL;
  info->number_of_params = size;
  for (i = 0; i < size; i++) {
    variable_t *var   = (variable_t *)nmalloc(sizeof(*var));
    var->name         = NULL;
    var->value        = NULL;
    var->next         = NULL;
    const char *start = param_array[i];
    const char *end   = param_array[i];
    end               = strrchr(param_array[i], '*');
    if (end == NULL) {
      end = strrchr(param_array[i], ' ');
      if (end == NULL) {
        end = start;
        for (; *end; end++);
        var->type = measured_memmove_copy(param_array[i], (end - start));
        if (info->params == NULL) {
          var->prev    = NULL;
          info->params = var;
        }
        else {
          var->prev          = info->params;
          info->params->next = var;
          info->params       = info->params->next;
        }
        continue;
      }
    }
    end += 1;
    start = end;
    for (; *end; end++);
    var->name = measured_copy(param_array[i] + (start - param_array[i]), (end - start));
    var->type = measured_copy(param_array[i], (start - param_array[i]));
    if (info->params == NULL) {
      var->prev    = NULL;
      info->params = var;
    }
    else {
      var->prev          = info->params;
      info->params->next = var;
      info->params       = info->params->next;
    }
  }
  for (i = 0; i < size; i++) {
    free(param_array[i]);
  }
  free(param_array);
  free(copy);
  return info;
}

function_info_t parse_local_func(const char *str) {
  int          i, pos;
  char        *copy        = copy_of(str);
  unsigned int len         = strlen(copy);
  char         prefix[256] = "", params[256] = "";
  for (i = 0; i < len && copy[i] != '('; i++);
  if (copy[i] != '(') {
    free(copy);
    return {};
  }
  pos = i;
  for (i = 0; i < pos; i++) {
    prefix[i] = copy[i];
  }
  prefix[i] = '\0';
  pos += 1;
  for (i = 0; copy[pos + i] && copy[pos + i] != ')'; i++) {
    params[i] = copy[pos + i];
  }
  if (copy[pos + i] != ')') {
    free(copy);
    return {};
  }
  params[i] = '\0';
  function_info_t info;
  info.full_function        = copy_of(str);
  info.name                 = NULL;
  info.return_type          = NULL;
  info.params               = NULL;
  info.number_of_params     = 0;
  info.attributes           = NULL;
  info.number_of_attributes = 0;
  /* Now the buffer contains all text before the '(' char. */
  const int slen  = strlen(prefix);
  int       words = 0;
  for (int i = slen; i > 0; --i) {
    if (prefix[i] == ' ' && i != slen - 1) {
      if (words++ == 0) {
        int was_i = i;
        /* Here we extract any ptr`s that are at the start of the name.
         */
        for (; prefix[i + 1] == '*' || prefix[i + 1] == '&'; i++);
        info.name = copy_of(&prefix[i + 1]);
        /* If any ptr`s were found we add them to the return string. */
        if (int diff = i - was_i; diff > 0) {
          for (; diff > 0; diff--, was_i++) {
            prefix[was_i] = '*';
          }
        }
        prefix[was_i] = '\0';
        i             = was_i;
        break;
      }
    }
  }
  info.return_type = copy_of(prefix);
  int    cap = 10, size = 0;
  char  *param_buf   = params;
  char **param_array = (char **)nmalloc(cap * sizeof(char *));
  for (i = 0, pos = 0; params[i]; i++) {
    if (params[i] == ',' || params[i + 1] == '\0') {
      (params[i + 1] == '\0') ? (i += 1) : 0;
      (cap == size) ? cap *= 2, param_array = (char **)nrealloc(param_array, cap * sizeof(char *)) : 0;
      param_array[size++] = measured_memmove_copy(param_buf, i - pos);
      (params[i + 1] == ' ') ? (i += 2) : (i += 1);
      param_buf += i - pos;
      pos = i;
    }
  }
  param_array[size]     = NULL;
  info.number_of_params = size;
  for (i = 0; i < size; i++) {
    variable_t *var   = (variable_t *)nmalloc(sizeof(*var));
    var->name         = NULL;
    var->value        = NULL;
    var->next         = NULL;
    const char *start = param_array[i];
    const char *end   = param_array[i];
    end               = strrchr(param_array[i], '*');
    if (end == NULL) {
      end = strrchr(param_array[i], '&');
    }
    if (end == NULL) {
      end = strrchr(param_array[i], ' ');
      if (end == NULL) {
        end = start;
        for (; *end; end++);
        var->type = measured_memmove_copy(param_array[i], (end - start));
        if (info.params == NULL) {
          var->prev   = NULL;
          info.params = var;
        }
        else {
          var->prev         = info.params;
          info.params->next = var;
          info.params       = info.params->next;
        }
        continue;
      }
    }
    end += 1;
    start = end;
    for (; *end; end++);
    var->name = measured_copy(param_array[i] + (start - param_array[i]), (end - start));
    var->type = measured_copy(param_array[i], (start - param_array[i]));
    if (info.params == NULL) {
      var->prev   = NULL;
      info.params = var;
    }
    else {
      var->prev         = info.params;
      info.params->next = var;
      info.params       = info.params->next;
    }
  }
  for (i = 0; i < size; i++) {
    free(param_array[i]);
  }
  free(param_array);
  free(copy);
  return info;
}

/* Return`s 'TRUE' if 'sig' is not a valid variable decl. */
bool invalid_variable_sig(const char *sig) {
  if (strstr(sig, "|=") || strstr(sig, "+=") || strstr(sig, "-=") || *sig == '{' || strchr(sig, '#') ||
      strncmp(sig, "return", 6) == 0) {
    return TRUE;
  }
  const char *p_eql    = strchr(sig, '=');
  const char *p_parent = strchr(sig, '(');
  if (p_parent && p_eql) {
    if ((p_parent - sig) < (p_eql - sig)) {
      return TRUE;
    }
  }
  if (p_parent && !p_eql) {
    return TRUE;
  }
  return FALSE;
}

void parse_variable(const char *sig, char **type, char **name, char **value) {
  /* Set all refreces to 'NULL' so we can return when we want. */
  *type  = NULL;
  *name  = NULL;
  *value = NULL;
  if (invalid_variable_sig(sig)) {
    return;
  }
  const char *start = sig;
  const char *end   = start;
  const char *p     = NULL;
  adv_ptr_to_ch(end, ';');
  if (!*end) {
    return;
  }
  start = end;
  for (; start > sig && *start != '='; start--);
  if (start > sig) {
    p = start;
    p += 1;
    for (; *p && (*p == ' ' || *p == '\t'); p++);
    (value != NULL) ? *value = measured_copy(p, (end - p)) : NULL;
  }
  else {
    start = end;
  }
  start -= 1;
  for (; start > sig && (*start == ' ' || *start == '\t'); start--);
  end = start;
  for (; start > sig && *start != ' ' && *start != '\t' && *start != '*' && *start != '&'; start--);
  p = start;
  if (start > sig) {
    p += 1;
  }
  (name != NULL) ? *name = measured_copy(p, (end - p) + 1) : NULL;
  if (start == sig) {
    return;
  }
  end = p - 1;
  for (; end > sig && (*end == ' ' || *end == '\t'); end--);
  end += 1;
  (type != NULL) ? *type = measured_copy(sig, (end - sig)) : NULL;
}

void flag_all_brackets(void) {
  for (linestruct *line = openfile->filetop; line != NULL; line = line->next) {
    const char *start = strchr(line->data, '{');
    const char *end   = strrchr(line->data, '}');
    /* Start bracket line was found. */
    if (start && !end) {
      line->flags.set(BRACKET_START);
      if (line->prev && ((line->prev->flags.is_set(IN_BRACKET)) || (line->prev->flags.is_set(BRACKET_START)))) {
        line->flags.set(IN_BRACKET);
      }
    }
    /* End bracket line was found. */
    else if (!start && end) {
      line->flags.set(BRACKET_END);
      line->flags.unset(BRACKET_START);
      for (linestruct *t_line = line->prev; t_line; t_line = t_line->prev) {
        if ((t_line->flags.is_set(BRACKET_START))) {
          if (line_indent(line) == line_indent(t_line)) {
            if (t_line->prev && (t_line->prev->flags.is_set(IN_BRACKET))) {
              line->flags.set(IN_BRACKET);
            }
            else {
              line->flags.unset(IN_BRACKET);
            }
            break;
          }
        }
      }
    }
    /* Was not found. */
    else if ((start == NULL && end == NULL) || (start != NULL && end != NULL)) {
      if (line->prev && ((line->prev->flags.is_set(IN_BRACKET)) || (line->prev->flags.is_set(BRACKET_START)))) {
        line->flags.set(IN_BRACKET);
      }
      else {
        line->flags.unset(IN_BRACKET);
      }
    }
  }
}

/* Set comment flags for all lines in current file. */
void flag_all_block_comments(linestruct *from) {
  for (linestruct *line = from; line != NULL; line = line->next) {
    const char *found_start = strstr(line->data, "/*");
    const char *found_end   = strstr(line->data, "*/");
    const char *found_slash = strstr(line->data, "//");
    /* First line for a block comment. */
    if (found_start != NULL && found_end == NULL) {
      /* If a slash comment is found and it is before the block start,
       * we adjust the start and end pos.  We also make sure to unset
       * 'BLOCK_COMMENT_START' for the line. */
      if (found_slash != NULL && found_slash < found_start) {
        line->flags.unset(BLOCK_COMMENT_START);
      }
      else {
        line->flags.set(BLOCK_COMMENT_START);
      }
      line->flags.unset(SINGLE_LINE_BLOCK_COMMENT);
      line->flags.unset(IN_BLOCK_COMMENT);
      line->flags.unset(BLOCK_COMMENT_END);
    }
    /* Either inside of a block comment or not a block comment at all. */
    else if (found_start == NULL && found_end == NULL) {
      if (line->prev &&
          ((line->prev->flags.is_set(IN_BLOCK_COMMENT)) || (line->prev->flags.is_set(BLOCK_COMMENT_START))) &&
          !(line->prev->flags.is_set(SINGLE_LINE_BLOCK_COMMENT))) {
        line->flags.set(IN_BLOCK_COMMENT);
        line->flags.unset(BLOCK_COMMENT_START);
        line->flags.unset(BLOCK_COMMENT_END);
        line->flags.unset(SINGLE_LINE_BLOCK_COMMENT);
      }
      /* If the prev line is not in a block comment or the
       * start block line we are not inside a comment block. */
      else {
        line->flags.unset(IN_BLOCK_COMMENT);
        line->flags.unset(BLOCK_COMMENT_START);
        line->flags.unset(BLOCK_COMMENT_END);
        line->flags.unset(SINGLE_LINE_BLOCK_COMMENT);
      }
    }
    /* End of a block comment. */
    else if (found_start == NULL && found_end != NULL) {
      /* If last line is in a comment block or is the start of the block.
       */
      if (line->prev &&
          ((line->prev->flags.is_set(IN_BLOCK_COMMENT)) || (line->prev->flags.is_set(BLOCK_COMMENT_START))) &&
          !(line->prev->flags.is_set(SINGLE_LINE_BLOCK_COMMENT))) {
        line->flags.set(BLOCK_COMMENT_END);
        line->flags.unset(IN_BLOCK_COMMENT);
        line->flags.unset(BLOCK_COMMENT_START);
        line->flags.unset(SINGLE_LINE_BLOCK_COMMENT);
      }
    }
  }
}

/* void
remove_local_vars_from(linestruct *line)
{
    if (!(line->flags.is_set(IN_BRACKET)))
    {
        return;
    }
    bool done = FALSE;
    while (!done)
    {
        auto it = test_map.begin();
        for (; it != test_map.end(); ++it)
        {
            if (it->second.from_line == -1)
            {
                continue;
            }
            if (it->second.from_line == line->lineno && it->second.color == FG_VS_CODE_BRIGHT_CYAN &&
                it->second.type == LOCAL_VAR_SYNTAX)
            {
                test_map.erase(it);
                break;
            }
        }
        if (it == test_map.end())
        {
            done = TRUE;
        }
    }
} */

void remove_local_vars_from(linestruct *line) {
  if (!(line->flags.is_set(IN_BRACKET))) {
    return;
  }
  for (auto it = test_map.begin(); it != test_map.end();) {
    if (it->second.from_line == -1) {
      ++it;
      continue;
    }
    if (it->second.from_line == line->lineno && it->second.color == FG_VS_CODE_BRIGHT_CYAN &&
        it->second.type == LOCAL_VAR_SYNTAX) {
      it = test_map.erase(it);
    }
    else {
      ++it;
    }
  }
}

void remove_from_color_map(linestruct *line, int color, int type) {
  bool done = FALSE;
  while (!done) {
    auto it = test_map.begin();
    for (; it != test_map.end(); it++) {
      const auto &[c, fline, tline, t] = it->second;
      if (fline == -1) {
        continue;
      }
      else if (c == color && fline == line->lineno && t == type) {
        test_map.erase(it);
        break;
      }
    }
    if (it == test_map.end()) {
      done = TRUE;
    }
  }
}
