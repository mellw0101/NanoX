#include "../include/prototypes.h"

/* Returns 'true' if 'c' is a cpp syntax char. */
bool isCppSyntaxChar(const char c) {
  return (c == '<' || c == '>' || c == '&' || c == '*' || c == '=' || c == '+' || c == '-' || c == '/' || c == '%' ||
          c == '!' || c == '^' || c == '|' || c == '~' || c == '{' || c == '}' || c == '[' || c == ']' || c == '(' ||
          c == ')' || c == ';' || c == ':' || c == ',' || c == '.' || c == '?' || c == '#');
}

/* Get indent in number of 'tabs', 'spaces', 'total chars', 'total tabs (based
 * on width of tab)'. */
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
  *t_char = *tabs + *spaces;
  *t_tabs = *tabs;
  if (*spaces > 0) {
    *t_tabs += *spaces / WIDTH_OF_TAB;
  }
}

/* Return the len of indent in terms of index off first non 'tab/space' char. */
Ushort indent_char_len(linestruct *line) {
  Ushort i = 0;
  for (; line->data[i]; i++) {
    if (line->data[i] != ' ' && line->data[i] != '\t') {
      break;
    }
  }
  return i;
}

/* If an area is marked then plase the 'str' at mark and current_x, thereby enclosing the marked area.
 * TODO: (enclose_marked_region) - Make the undo one action insted of two seprate once. */
void enclose_marked_region(const char *s1, const char *s2) {
  /* Sanity check, Returns is there is no mark */
  if (openfile->mark == nullptr) {
    return;
  }
  linestruct *was_current = openfile->current, *top, *bot;
  Ulong       top_x, bot_x;
  get_region(&top, &top_x, &bot, &bot_x);
  openfile->current = top;
  add_undo(REPLACE, nullptr);
  inject_in_line(&top, s1, top_x);
  /* If top and bot is equal, move mark to the right by 's1' len. */
  (top == bot) ? (bot_x += strlen(s1)) : 0;
  openfile->current = bot;
  add_undo(REPLACE, nullptr);
  inject_in_line(&bot, s2, bot_x);
  openfile->mark_x++;
  /* If top and bot is the same, move cursor one step right. */
  (top == bot) ? do_right() : void();
  openfile->current = was_current;
  set_modified();
}

/* This is a shortcut to make marked area a block comment. */
void do_block_comment(void) {
  enclose_marked_region("/* ", " */");
  refresh_needed = true;
}

/* Check if enter is requested when betweeen '{' and '}'.
 * if so properly open the brackets, place cursor in the middle and indent once.
 * Return`s 'false' if not between them, otherwise return`s 'true' */
bool enter_with_bracket(void) {
  char        c, c_prev;
  linestruct *was_current = openfile->current, *middle, *end;
  bool        allblanks   = false;
  Ulong       extra;
  if (!openfile->current->data[openfile->current_x - 1]) {
    return false;
  }
  c_prev = openfile->current->data[openfile->current_x - 1];
  c      = openfile->current->data[openfile->current_x];
  if (c_prev != '{' || c != '}') {
    return false;
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
  add_undo(ENTER, nullptr);
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
  add_undo(ENTER, nullptr);
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
  refresh_needed = true;
  focusing       = false;
  return true;
}

void all_brackets_pos(void) {
  linestruct *line;
  long        start_pos = -1, end_pos = -1;
  bool        is_start;
  for (line = openfile->filetop; line != nullptr; line = line->next) {
    if (is_line_start_end_bracket(line, &is_start)) {
      if (is_start == true) {
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
  check_line_for_vars(openfile->current);
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
  LSP->index_file(openfile->filename);
  unix_socket_debug("got here\n");
  unix_socket_debug("include size: %u\n", LSP->index.include.size());
  unix_socket_debug("define size: %u\n", LSP->index.defines.size());
  unix_socket_debug("class size: %u\n", LSP->index.classes.size());
  unix_socket_debug("var size: %u\n", LSP->index.variabels.size());
  unix_socket_debug("rawstructs size: %u\n", LSP->index.rawstruct.size());
  unix_socket_debug("rawtypedefs size: %u\n", LSP->index.rawtypedef.size());
  unix_socket_debug("rawenums size: %u\n", LSP->index.rawenum.size());
  /* if (!openfile->filename[0]) {
     return;
   }
   char *abs_path = get_full_path(openfile->filename);
   if (abs_path) {
     IndexFile idfile;
     idfile.file = abs_path;
     LSP->check(&idfile);
     unix_socket_debug("got here.\n");
     unix_socket_debug("define size: %u\n", LSP->index.defines.size());
     unix_socket_debug("class size: %u\n", LSP->index.classes.size());
     unix_socket_debug("var size: %u\n", LSP->index.variabels.size());
     unix_socket_debug("rawstructs size: %u\n", LSP->index.rawstruct.size());
     unix_socket_debug("rawtypedefs size: %u\n", LSP->index.rawtypedef.size());
     unix_socket_debug("rawenums size: %u\n", LSP->index.rawenum.size());
   }
   if (test_map.find("int") != test_map.end()) {
     unix_socket_debug("int still in map.\n");
   } */
  /* linestruct *end;
  Ulong index;
  if (find_end_bracket(openfile->current, openfile->current_x, &end, &index)) {
    unix_socket_debug("found: %lu:%lu\n", end->lineno, index);
  } */
}

void do_test(void) {
  const char *found = nstrchr_ccpp(openfile->current->data + openfile->current_x, ';');
  if (found) {
    unix_socket_debug("found: '%s' in '%s'.\n", found, openfile->current->data);
  }
}

void do_test_window(void) {
  nlog("test win\n");
  if (suggestwin != nullptr) {
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
    return nullptr;
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
    return nullptr;
  }
  params[i]                  = '\0';
  function_info_t *info      = (function_info_t *)nmalloc(sizeof(*info));
  info->full_function        = copy_of(str);
  info->name                 = nullptr;
  info->return_type          = nullptr;
  info->params               = nullptr;
  info->number_of_params     = 0;
  info->attributes           = nullptr;
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
  param_array[size]      = nullptr;
  info->number_of_params = size;
  for (i = 0; i < size; i++) {
    variable_t *var   = (variable_t *)nmalloc(sizeof(*var));
    var->name         = nullptr;
    var->value        = nullptr;
    var->next         = nullptr;
    const char *start = param_array[i];
    const char *end   = param_array[i];
    end               = strrchr(param_array[i], '*');
    if (end == nullptr) {
      end = strrchr(param_array[i], ' ');
      if (end == nullptr) {
        end = start;
        for (; *end; end++);
        var->type = measured_memmove_copy(param_array[i], (end - start));
        if (info->params == nullptr) {
          var->prev    = nullptr;
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
    if (info->params == nullptr) {
      var->prev    = nullptr;
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
  info.name                 = nullptr;
  info.return_type          = nullptr;
  info.params               = nullptr;
  info.number_of_params     = 0;
  info.attributes           = nullptr;
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
  param_array[size]     = nullptr;
  info.number_of_params = size;
  for (i = 0; i < size; i++) {
    variable_t *var   = (variable_t *)nmalloc(sizeof(*var));
    var->name         = nullptr;
    var->value        = nullptr;
    var->next         = nullptr;
    const char *start = param_array[i];
    const char *end   = param_array[i];
    end               = strrchr(param_array[i], '*');
    if (end == nullptr) {
      end = strrchr(param_array[i], '&');
    }
    if (end == nullptr) {
      end = strrchr(param_array[i], ' ');
      if (end == nullptr) {
        end = start;
        for (; *end; end++);
        var->type = measured_memmove_copy(param_array[i], (end - start));
        if (info.params == nullptr) {
          var->prev   = nullptr;
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
    if (info.params == nullptr) {
      var->prev   = nullptr;
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

/* Return`s 'true' if 'sig' is not a valid variable decl. */
bool invalid_variable_sig(const char *sig) {
  if (strstr(sig, "|=") || strstr(sig, "+=") || strstr(sig, "-=") || *sig == '{' || strchr(sig, '#') ||
      strncmp(sig, "return", 6) == 0) {
    return true;
  }
  const char *p_eql    = strchr(sig, '=');
  const char *p_parent = strchr(sig, '(');
  if (p_parent && p_eql) {
    if ((p_parent - sig) < (p_eql - sig)) {
      return true;
    }
  }
  if (p_parent && !p_eql) {
    return true;
  }
  return false;
}

void parse_variable(const char *sig, char **type, char **name, char **value) {
  /* Set all refreces to 'nullptr' so we can return when we want. */
  *type  = nullptr;
  *name  = nullptr;
  *value = nullptr;
  if (invalid_variable_sig(sig)) {
    return;
  }
  const char *start = sig;
  const char *end   = start;
  const char *p     = nullptr;
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
    (value != nullptr) ? *value = measured_copy(p, (end - p)) : nullptr;
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
  (name != nullptr) ? *name = measured_copy(p, (end - p) + 1) : nullptr;
  if (start == sig) {
    return;
  }
  end = p - 1;
  for (; end > sig && (*end == ' ' || *end == '\t'); end--);
  end += 1;
  (type != nullptr) ? *type = measured_copy(sig, (end - sig)) : nullptr;
}

void flag_all_brackets(void) {
  for (linestruct *line = openfile->filetop; line != nullptr; line = line->next) {
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
              /* Once we find the end bracket we
               * parse the current function. */
              find_current_function(line->prev);
            }
            break;
          }
        }
      }
    }
    /* Was not found. */
    else if ((start == nullptr && end == nullptr) || (start != nullptr && end != nullptr)) {
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
  for (linestruct *line = from; line != nullptr; line = line->next) {
    const char *found_start = strstr(line->data, "/*");
    const char *found_end   = strstr(line->data, "*/");
    const char *found_slash = strstr(line->data, "//");
    /* First line for a block comment. */
    if (found_start != nullptr && found_end == nullptr) {
      /* If a slash comment is found and it is before the block start,
       * we adjust the start and end pos.  We also make sure to unset
       * 'BLOCK_COMMENT_START' for the line. */
      if (found_slash != nullptr && found_slash < found_start) {
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
    else if (found_start == nullptr && found_end == nullptr) {
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
    else if (found_start == nullptr && found_end != nullptr) {
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

/* TODO: Fix updating function. */
void find_current_function(linestruct *l) {
  PROFILE_FUNCTION;
  for (linestruct *line = l; line; line = line->prev) {
    if ((line->flags.is_set(BRACKET_START)) && !(line->flags.is_set(IN_BRACKET))) {
      if (line->prev == nullptr) {
        return;
      }
      char *func_sig = parse_function_sig(line);
      if (func_sig == nullptr) {
        return;
      }
      line                       = line->prev;
      function_info_t local_func = parse_local_func(func_sig);
      local_func.start_bracket   = line->next->lineno;
      for (linestruct *lend = line->next; lend != nullptr; lend = lend->next) {
        if ((lend->flags.is_set(IN_BRACKET)) && !(lend->flags.is_set(IN_BRACKET))) {
          local_func.end_braket = lend->lineno;
          break;
        }
      }
      bool found = false;
      for (int i = 0; i < local_funcs.get_size(); i++) {
        if (l->lineno + 2 >= local_funcs[i].start_bracket && l->lineno <= local_funcs[i].end_braket) {
          local_funcs[i] = local_func;
          found          = true;
          break;
        }
      }
      if (!found) {
        add_rm_color_map(local_func.name, {FG_VS_CODE_BRIGHT_YELLOW});
        local_funcs.push_back(local_func);
      }
      refresh_needed = true;
      free(func_sig);
      break;
    }
    if (!(line->flags.is_set(IN_BRACKET))) {
      break;
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
    bool done = false;
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
            done = true;
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
  bool done = false;
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
      done = true;
    }
  }
}

void check_line_for_vars(linestruct *line) {
  auto it = var_vector.begin();
  while (it != var_vector.end()) {
    if (it->decl_line == line->lineno) {
      it = var_vector.erase(it);
    }
    else {
      it++;
    }
  }
  remove_from_color_map(line, FG_VS_CODE_BRIGHT_CYAN, LOCAL_VAR_SYNTAX);
  vector<var_t> vars;
  line_variable(line, vars);
  for (const auto &[type, name, value, delc_line, scope_end, file] : vars) {
    const auto &it = test_map.find(name);
    if (it != test_map.end() &&
        (it->second.color == FG_VS_CODE_BLUE || it->second.color == FG_VS_CODE_BRIGHT_MAGENTA)) {
      continue;
    }
    test_map[name] = {
      FG_VS_CODE_BRIGHT_CYAN,
      delc_line,
      scope_end,
      LOCAL_VAR_SYNTAX,
    };
    var_vector.push_back({
      type,
      name,
      value,
      delc_line,
      scope_end,
    });
    /* NLOG("\n"
         "           Type: %s\n"
         "           Name: %s\n"
         "          Value: %s\n"
         "      Decl line: %i\n"
         "      Scope_end: %i\n"
         "var_vector size: %i\n"
         "\n",
         type.c_str(), name.c_str(), value.c_str(), delc_line, scope_end,
         var_vector.size()); */
  }
  // const char *data = &line->data[indent_char_len(line)];
  // if (invalid_variable_sig(data))
  // {
  //     return;
  // }
  // const char *p          = data;
  // char       *decl_start = nullptr;
  // adv_ptr(p, (*p != ',') && (*p != ';') && (*p != '=') && (*p != '{'));
  // if (!*p || *p == ';' || *p == '=')
  // {
  //     char *tmp_data = nullptr;
  //     if (*p == '=' && p[1] == '\0')
  //     {
  //         if (line->next)
  //         {
  //             char *first_part = copy_of(data);
  //             char *second_part =
  //                 copy_of(&line->next->data[indent_char_len(line->next)]);
  //             tmp_data = alloc_str_free_substrs(first_part, second_part);
  //             data     = tmp_data;
  //             // nlog("full sig: %s\n", tmp_data);
  //         }
  //     }
  //     char *type, *name, *value;
  //     parse_variable(data, &type, &name, &value);
  //     if (type && name)
  //     {
  //         if (strchr(type, '(') ||
  //             strncmp(type, "return", "return"_sllen) == 0 ||
  //             strchr(type, '[') || strstr(type, "/*") ||
  //             strncmp(name, "operator", "operator"_sllen) == 0)
  //         {
  //             // nlog("Wrong type: %s\n", type);
  //             // nlog("Wrong name: %s\n", name);
  //             NULL_safe_free(type);
  //             NULL_safe_free(name);
  //             NULL_safe_free(value);
  //             NULL_safe_free(tmp_data);
  //             return;
  //         }
  //         // nlog("type: %s\n", type);
  //         // nlog("name: %s\n", name);
  //         const char *array_start = strchr(name, '[');
  //         const char *array_end   = strchr(name, ']');
  //         char       *var         = nullptr;
  //         if (array_start && array_end)
  //         {
  //             var = measured_copy(name, (array_start - name));
  //         }
  //         else
  //         {
  //             var = copy_of(name);
  //         }
  //         const auto &it = color_map.find(var);
  //         if (it != color_map.end() &&
  //             (it->second.color == FG_VS_CODE_BLUE ||
  //              it->second.color == FG_VS_CODE_BRIGHT_MAGENTA))
  //         {
  //             free(var);
  //             return;
  //         }
  //         color_map[var] = {
  //             FG_VS_CODE_BRIGHT_CYAN,
  //             (int)line->lineno,
  //             current_line_scope_end(line),
  //             LOCAL_VAR_SYNTAX,
  //         };
  //     }
  //     NULL_safe_free(type);
  //     NULL_safe_free(name);
  //     NULL_safe_free(value);
  //     NULL_safe_free(tmp_data);
  //     return;
  // }
  // else if (*p == ',')
  // {
  //     char *first_part = measured_copy(data, (p - data));
  //     /* nlog("first part: %s\n", first_part);
  //     nlog("line: %s\n", data); */
  //     for (; p > first_part && (*p == ' ' || *p == '\t'); p--);
  //     if (p == first_part)
  //     {
  //         return;
  //     }
  //     p = strrchr(first_part, ' ');
  //     if (!p)
  //     {
  //         return;
  //     }
  //     p += 1;
  //     char *base_type = measured_copy(first_part, (p - first_part));
  //     if (strchr(base_type, '=') || strstr(base_type, "/*") ||
  //         strchr(base_type, '(') ||
  //         strncmp(base_type, "return", "return"_sllen) == 0)
  //     {
  //         free(first_part);
  //         free(base_type);
  //         return;
  //     }
  //     // nlog("base_type: %s\n", base_type);
  //     decl_start = copy_of(data + strlen(base_type));
  //     // nlog("decl_start: %s\n", decl_start);
  //     if (decl_start)
  //     {
  //         vector<string> names;
  //         const char    *start = decl_start;
  //         const char    *end   = nullptr;
  //         while (*start)
  //         {
  //             adv_ptr_to_next_word(start);
  //             adv_ptr(start, (*start == '*' || *start == '&'));
  //             end = start;
  //             adv_ptr(end, *end != ',' && *end != ';');
  //             if (!*end)
  //             {
  //                 return;
  //             }
  //             string name(start, (end - start));
  //             p = &name[0];
  //             names.push_back(p);
  //             start = end + 1;
  //         }
  //         for (const auto &str : names)
  //         {
  //             char       *name = copy_of(&str[0]);
  //             const auto &it   = color_map.find(name);
  //             if (it != color_map.end() &&
  //                 (it->second.color == FG_VS_CODE_BLUE ||
  //                  it->second.color == FG_VS_CODE_BRIGHT_MAGENTA))
  //             {
  //                 free(name);
  //                 continue;
  //             }
  //             color_map[name] = {
  //                 .color     = FG_VS_CODE_BRIGHT_CYAN,
  //                 .from_line = (int)line->lineno,
  //                 .to_line   = current_line_scope_end(line),
  //                 .type      = LOCAL_VAR_SYNTAX,
  //             };
  //         }
  //     }
  // }
}
