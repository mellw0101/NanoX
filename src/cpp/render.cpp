#include "../include/prototypes.h"

static Uint block_comment_start = (Uint)-1;
static Uint block_comment_end   = (Uint)-1;
static int  color_bi[3]         = {FG_VS_CODE_YELLOW, FG_VS_CODE_BRIGHT_MAGENTA, FG_VS_CODE_BRIGHT_BLUE};

static int         row       = 0;
static const char *converted = NULL;
static linestruct *line      = NULL;
static Ulong       from_col  = 0;

unordered_map<string, syntax_data_t> test_map;

void render_part(Ulong match_start, Ulong match_end, short color) {
  PROFILE_FUNCTION;
  const char *thetext = NULL;
  int paintlen = 0, start_col = 0;
  if ((match_start >= till_x)) {
    return;
  }
  if (match_start > from_x) {
    start_col = (int)(wideness(line->data, match_start) - from_col);
  }
  thetext  = (converted + actual_x(converted, start_col));
  paintlen = (int)actual_x(thetext, (wideness(line->data, match_end) - from_col - start_col));
  mv_add_nstr_color(midwin, row, (margin + start_col), thetext, paintlen, color);
}

/* Experiment. */
void render_part_raw(Ulong start_index, Ulong end_index, short color) {
  const char *start = &line->data[start_index];
  const char *end   = start;
  const char *stop  = &line->data[end_index];
  while (*end && end != stop) {
    ADV_PTR(end, (end <= stop) && (*end == ' ' || *end == '\t'));
    if (*end == '\0' || end == stop) {
      break;
    }
    start = end;
    ADV_PTR(end, (end <= stop) && (*end != ' ' && *end != '\t'));
    end += 1;
    RENDR(R, color, start, end);
  }
}

/* Render the text of a given line.  Note that this function only renders the text and nothing else. */
void render_line_text(int row, const char *str, linestruct *line, Ulong from_col) {
  PROFILE_FUNCTION;
  if (margin > 0) {
    WIN_COLOR_ON(midwin, line_number_color);
    if (ISSET(SOFTWRAP) && from_col) {
      mvwprintw(midwin, row, 0, "%*s", margin - 1, " ");
    }
    else {
      mvwprintw(midwin, row, 0, "%*lu", margin - 1, line->lineno);
    }
    WIN_COLOR_OFF(midwin, line_number_color);
    if (line->has_anchor == TRUE && (from_col == 0 || !ISSET(SOFTWRAP))) {
      if (using_utf8()) {
        wprintw(midwin, "\xE2\xAC\xA5");
      }
      else {
        wprintw(midwin, "+");
      }
    }
    else {
      wprintw(midwin, " ");
    }
  }
  mvwaddstr(midwin, row, margin, str);
  if (is_shorter || ISSET(SOFTWRAP)) {
    wclrtoeol(midwin);
  }
  /* Only draw sidebar when file is longer then editwin rows. */
  if (sidebar && openfile->filebot->lineno > editwinrows) {
    mvwaddch(midwin, row, COLS - 1, bardata[row]);
  }
}

/* Set start and end pos for comment block or if the entire line is inside of a block
 * comment set 'block_comment_start' to '0' and 'block_comment_end' to '(Uint)-1'. */
void render_comment(void) {
  const char *start = strstr(line->data, "/*");
  const char *end   = strstr(line->data, "*/");
  const char *slash = strstr(line->data, "//");
  /* Single line block comment. */
  if (start && end) {
    block_comment_start = (start - line->data);
    block_comment_end   = (end - line->data + 2);
    /* If slash comment found, adjust the start and end pos correctly. */
    if (slash && (slash - line->data) < block_comment_start) {
      block_comment_start = (slash - line->data);
      block_comment_end   = (Uint)-1;
    }
    line->flags.set<SINGLE_LINE_BLOCK_COMMENT>();
    line->flags.unset<BLOCK_COMMENT_START>();
    line->flags.unset<BLOCK_COMMENT_END>();
    line->flags.unset<IN_BLOCK_COMMENT>();
    render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
    /* Highlight the block start if prev line is in a
     * block comment or the start of a block comment. */
    if (line->prev &&
        (line->prev->flags.is_set<IN_BLOCK_COMMENT>() || line->prev->flags.is_set<BLOCK_COMMENT_START>())) {
      if ((end - line->data) > 0 && line->data[(end - line->data) - 1] != '/') {
        midwin_mv_add_nstr_color(row, (wideness(line->data, (start - line->data)) + margin), start, 2, ERROR_MESSAGE);
        /* If there is a slash comment infront the block comment. Then of cource we still color
         * the text from the slash to the block start after we error highlight the block start. */
        if (slash && (slash - line->data) < (start - line->data)) {
          midwin_mv_add_nstr_color(row, (wideness(line->data, (slash - line->data))) + margin, slash, (start - line->data) - (slash - line->data), FG_GREEN);
        }
        block_comment_start += (start - line->data) + 2;
      }
      else if ((start - line->data) + 1 == (end - line->data)) {
        line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
        line->flags.set<BLOCK_COMMENT_END>();
        block_comment_start = 0;
      }
    }
    else if ((start - line->data) + 1 == (end - line->data)) {
      end = strstr(end + 2, "*/");
      if (end) {
        block_comment_end = (end - line->data) + 2;
      }
      else {
        block_comment_end = (Uint)-1;
        line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
        line->flags.set<BLOCK_COMMENT_START>();
      }
    }
    while (start && end) {
      /** TODO: Here we need to fix the issue of multiple block comments on a single line. */
      start = strstr(start + 2, "/*");
      end   = strstr(end + 2, "*/");
      slash = strstr(slash ? slash + 2 : line->data, "//");
      if (start && end) {
        const Ulong match_start = (start - line->data);
        const Ulong match_end   = (end - line->data) + 2;
        render_part(match_start, match_end, FG_GREEN);
      }
      else if (!start && end) {
        midwin_mv_add_nstr_color(row, PTR_POS_LINE(line, end), end, 2, ERROR_MESSAGE);
      }
      else if (start && !end) {
        const Ulong match_start = (start - line->data);
        render_part(match_start, till_x, FG_GREEN);
      }
    }
  }
  /* First line for a block comment. */
  else if (start && !end) {
    block_comment_start = (start - line->data);
    block_comment_end   = till_x;
    render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
    /* Do some error checking and highlight the block start if it`s found
     * while the block above it being a start block or inside a block. */
    if (line->prev &&
        (line->prev->flags.is_set<IN_BLOCK_COMMENT>() || line->prev->flags.is_set<BLOCK_COMMENT_START>())) {
      RENDR(R_LEN, ERROR_MESSAGE, start, 2);
      block_comment_start = (start - line->data) + 2;
    }
    /* If a slash comment is found and it is before the block start,
     * we adjust the start and end pos.  We also make sure to unset
     * 'BLOCK_COMMENT_START' for the line. */
    if (slash && (slash - line->data) < block_comment_start) {
      block_comment_start = (slash - line->data);
      block_comment_end   = till_x;
      render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
      line->flags.unset<BLOCK_COMMENT_START>();
    }
    else {
      line->flags.set<BLOCK_COMMENT_START>();
    }
    line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
    line->flags.unset<IN_BLOCK_COMMENT>();
    line->flags.unset<BLOCK_COMMENT_END>();
  }
  /* Either inside of a block comment or not a block comment at all. */
  else if (!start && !end) {
    if (line->prev &&
        (line->prev->flags.is_set<IN_BLOCK_COMMENT>() || line->prev->flags.is_set<BLOCK_COMMENT_START>()) &&
        !line->prev->flags.is_set<SINGLE_LINE_BLOCK_COMMENT>()) {
      block_comment_start = 0;
      block_comment_end   = till_x;
      render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
      line->flags.set<IN_BLOCK_COMMENT>();
      line->flags.unset<BLOCK_COMMENT_START>();
      line->flags.unset<BLOCK_COMMENT_END>();
      line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
    }
    /* If the prev line is not in a block comment or the
     * start block line we are not inside a comment block. */
    else {
      block_comment_start = till_x;
      block_comment_end   = 0;
      line->flags.unset<IN_BLOCK_COMMENT>();
      line->flags.unset<BLOCK_COMMENT_START>();
      line->flags.unset<BLOCK_COMMENT_END>();
      line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
      /* If slash comment is found comment out entire line after slash. */
      if (slash) {
        block_comment_start = (slash - line->data);
        block_comment_end   = till_x;
        render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
      }
    }
  }
  /* End of a block comment. */
  else if (!start && end) {
    /* If last line is in a comment block or is the start of the block. */
    if (line->prev &&
        (line->prev->flags.is_set<IN_BLOCK_COMMENT>() || line->prev->flags.is_set<BLOCK_COMMENT_START>()) &&
        !line->prev->flags.is_set<SINGLE_LINE_BLOCK_COMMENT>()) {
      block_comment_start = 0;
      block_comment_end   = (end - line->data) + 2;
      render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
      line->flags.set<BLOCK_COMMENT_END>();
      line->flags.unset<IN_BLOCK_COMMENT>();
      line->flags.unset<BLOCK_COMMENT_START>();
      line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
    }
    /* If slash if found and is before block end. */
    else if (slash && (slash - line->data) < (end - line->data)) {
      block_comment_start = (slash - line->data);
      block_comment_end   = till_x;
      render_part(block_comment_start, block_comment_end, FG_COMMENT_GREEN);
    }
    /* If not, error highlight the end block. */
    else {
      RENDR(R_LEN, ERROR_MESSAGE, end, 2);
    }
  }
  refresh_needed = true;
}

/* Color brackets based on indent. */
void render_bracket(void) {
  PROFILE_FUNCTION;
  const char *found = line->data;
  do {
    found = strstr_array(found, (const char *[]){ "{", "}", "[", "]", "(", ")" }, 6, NULL);
    if (found) {
      RENDR(R_LEN, color_bi[((*found == '{' || *found == '}') ? line_indent(line) : line_indent(line) + 1) % 3], found, 1 );
      ++found;
    }
  } while (found && *found);
}

void render_parents(void) {
  const char *start = strchr(line->data, '(');
  const char *end   = strchr(line->data, ')');
  while (1) {
    if (start) {
      rendr_ch_str_ptr(start, color_bi[(wideness(line->data, indent_char_len(line)) % 3)]);
      start = strchr(start + 1, '(');
    }
    if (end) {
      rendr_ch_str_ptr(end, color_bi[(wideness(line->data, indent_char_len(line)) % 3)]);
      end = strchr(end + 1, ')');
    }
    if (!start && !end) {
      break;
    }
  }
}

/* This function highlights string literals.  Error handeling is needed. */
void render_string_literals(void) {
  const char *start = line->data;
  const char *end   = NULL;
  while ((start = strchr(start, '"'))) {
    end = strchr(start + 1, '"');
    if (end) {
      render_part((start - line->data), (end - line->data) + 1, FG_YELLOW);
    }
    else {
      return;
    }
    start = end + 1;
  }
}

/* Function to handle char strings inside other strings or just in general. */
void render_char_strings(void) {
  const char *start = line->data, *end = line->data;
  while (start) {
    for (; *end && *end != '\''; end++);
    if (*end != '\'') {
      return;
    }
    start = end;
    end++;
    for (; *end && *end != '\''; end++);
    if (*end != '\'') {
      return;
    }
    end++;
    const Ulong match_start = (start - line->data);
    const Ulong match_end   = (end - line->data);
    if (match_start >= block_comment_start && match_end <= block_comment_end) {
      return;
    }
    render_part(match_start, match_end, FG_MAGENTA);
    start = end;
  }
}

void rendr_define(Uint index) {
  const char *start = NULL, *end = NULL, *param = NULL;
  char       *word = NULL;
  start            = &line->data[index];
  if (!*start) {
    RENDR(E, "<-(Macro name missing)");
    return;
  }
  end = start;
  ADV_PTR(end, (*end != '(' && *end != ' ' && *end != '\t'));
  if (end == start) {
    return;
  }
  RENDR(R, FG_VS_CODE_BLUE, start, end);
  string define_name(start, (end - start));
  if (test_map.find(define_name) == test_map.end()) {
    test_map[define_name] = {FG_VS_CODE_BLUE};
  }
  vector<string> params {};
  /* Handle macro parameter list.  If there is one. */
  if (*end == '(') {
    while (*end) {
      end += 1;
      /* Advance to the start of the word. */
      ADV_PTR(end, (*end == ' ' || *end == '\t'));
      if (!*end) {
        break;
      }
      start = end;
      /* Now find the end of the word. */
      ADV_PTR(end, (*end != ')' && *end != ',' && *end != ' ' && *end != '\t'));
      if (end == start) {
        break;
      }
      RENDR(R, FG_VS_CODE_BRIGHT_CYAN, start, end);
      word ? free(word) : void();
      word = measured_copy(start, (end - start));
      params.push_back(string(word));
      param = strstr(end, word);
      if (param) {
        while (param) {
          const char *ps = &line->data[(param - line->data) - 1];
          const char *pe = &param[(end - start)];
          if ((*pe == ' ' || *pe == ')' || *pe == ',') && (*ps == ' ' || *ps == '(' || *ps == ',')) {
            RENDR(R_LEN, FG_VS_CODE_BLUE, param, (end - start));
          }
          param = strstr(param + (end - start), word);
        }
      }
      if (*end == ')') {
        end += 1;
        break;
      }
      else if (*end == ' ') {
        ADV_PTR(end, (*end == ' ' || *end == '\t'));
        if (*end == ')') {
          break;
        }
        else if (*end != ',') {
          start = end;
          ADV_PTR(end, (*end != ')' && *end != ',' && *end != ' ' && *end != '\t'));
          RENDR(R, ERROR_MESSAGE, start, end);
          RENDR(E, "<-(Expected comma)");
          if (*end == ')') {
            break;
          }
        }
      }
      if (*end != ',') {
        end += 1;
      }
    }
    if (*end == ')') {
      end += 1;
    }
  }
  word ? free(word) : (void)0;
  ADV_PTR(end, (*end == ' ' || *end == '\t'));
  if (!*end) {
    return;
  }
  start = end;
  ADV_PTR(end, (*end != '(' && *end != ' ' && *end != '\t'));
  if (end == start) {
    return;
  }
  if (*start == '\\') {
    int    end_lineno;
    string full_decl = parse_full_define(line, &start, &end_lineno);
    if (!line->next) {
      remove_from_color_map(line->next, FG_VS_CODE_BLUE, DEFINE_PARAM_SYNTAX);
      for (int i = 0; i < params.size(); ++i) {
        const auto &it = test_map.find(params[i]);
        if (it == test_map.end()) {
          test_map[params[i]] = {
            FG_VS_CODE_BLUE,
            (int)line->next->lineno,
            end_lineno,
            DEFINE_PARAM_SYNTAX,
          };
        }
      }
    }
  }
  else if (*end == '(') {
    RENDR(R, FG_VS_CODE_BRIGHT_YELLOW, start, end);
  }
  else if (!(*start >= '0' && *start <= '9')) {
    RENDR(R, FG_VS_CODE_BLUE, start, end);
  }
}

void rendr_include(Uint index) {
  const char *start = &line->data[index];
  const char *end   = start;
  if (*start == '"') {
    end += 1;
  }
  ADV_PTR(end, (*end != '>' && *end != '"'));
  if (*end) {
    if (*start == '<' && *end == '>') {
      ++end;
      RENDR(C_PTR, FG_YELLOW, start, end);
    }
    else if (*start == '"' && *end == '"') {
      ++end;
      render_part((start - line->data), (end - line->data), FG_YELLOW);
    }
    else {
      ++end;
      RENDR(C_PTR, ERROR_MESSAGE, start, end);
    }
  }
  else {
    RENDR(C_PTR, ERROR_MESSAGE, start, end);
  }
  start = end;
  while (*end) {
    ADV_PTR(end, (*end == ' ' || *end == '\t'));
    if (!*end) {
      break;
    }
    start = end;
    ADV_PTR(end, (*end != ' ' && *end != '\t'));
    RENDR(R, ERROR_MESSAGE, start, end);
  }
}

/* Render if preprossesor statements. */
void rendr_if_preprosses(Uint index) {
  const char *start   = &line->data[index];
  const char *defined = strstr(start, "defined");
  while (defined) {
    start = defined;
    defined += 7;
    if (*defined && *defined == ' ') {
      RENDR(R, FG_VS_CODE_BLUE, start, defined);
    }
    else {
      RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, defined);
    }
    const char *parent_start = defined;
    const char *parent_end   = defined;
    while (*parent_start && *parent_end) {
      ADV_PTR(parent_start, *parent_start != '(' && *parent_start != '&' && *parent_start != '|');
      ADV_PTR(parent_end, *parent_end != ')' && *parent_end != '&' && *parent_end != '|');
      if ((*parent_start == '&') || (*parent_start == '|') || (*parent_end == '&') || (*parent_end == '|')) {
        break;
      }
      if (*parent_start && *parent_end) {
        parent_start += 1;
        const char *p = parent_start;
        ADV_PTR(p, p != parent_end && *p != ' ' && *p != '\t');
        if (parent_start == parent_end) {
          render_part((parent_start - line->data) - 1, (parent_end - line->data) + 1, ERROR_MESSAGE);
        }
        else if (p == parent_end) {
          RENDR(R, FG_VS_CODE_BLUE, parent_start, parent_end);
        }
        else {
          RENDR(C_PTR, ERROR_MESSAGE, parent_start, parent_end);
        }
        parent_start += 1;
        parent_end += 1;
        p                     = parent_end - 1;
        const char *error_end = NULL;
        while (*p && *p != '&' && *p != '|') {
          p += 1;
          ADV_PTR(p, (*p != '&') && (*p != '|') && (*p != '(') && (*p != ')'));
          if (*p == '(' || *p == ')') {
            error_end = p;
          }
        }
        if (error_end) {
          error_end += 1;
          RENDR(C_PTR, ERROR_MESSAGE, start, error_end);
        }
        break;
      }
      else if ((!*parent_start && *parent_end) || (*parent_start && !*parent_end)) {
        if (*parent_start) {
          RENDR(R_CHAR, ERROR_MESSAGE, parent_start);
        }
        else {
          RENDR(R_CHAR, ERROR_MESSAGE, parent_end);
        }
        break;
      }
    }
    defined = strstr(defined, "defined");
  }
}

/* This 'render' sub-system is responsible for handeling all pre-prossesor syntax. */
void render_preprossesor(void) {
  char       *current_word = NULL;
  const char *start        = strchr(line->data, '#');
  const char *end          = NULL;
  if (start) {
    RENDR(R_CHAR, FG_VS_CODE_BRIGHT_MAGENTA, start);
    ++start;
    if (!*start) {
      return;
    }
    end = start;
    ADV_PTR(end, (*end == ' ' || *end == '\t'));
    if (!*end) {
      return;
    }
    start = end;
    ADV_PTR(end, (*end != ' ' && *end != '\t'));
    if (end == start) {
      return;
    }
    current_word   = measured_copy(start, (end - start));
    const int type = hash_string(current_word);
    switch (type) {
      case define_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        ADV_PTR(end, (*end == ' ' || *end == '\t'));
        if (!*end) {
          break;
        }
        rendr_define((end - line->data));
        break;
      }
      case if_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        ADV_PTR(end, (*end == ' ' || *end == '\t'));
        rendr_if_preprosses((end - line->data));
        break;
      }
      case endif_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        break;
      }
      case ifndef_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        get_next_word(&start, &end);
        RENDR(R, FG_VS_CODE_BLUE, start, end);
        break;
      }
      case pragma_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        get_next_word(&start, &end);
        RENDR(R, FG_LAGOON, start, end);
        break;
      }
      case ifdef_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        get_next_word(&start, &end);
        RENDR(R, FG_VS_CODE_BLUE, start, end);
        break;
      }
      case else_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        break;
      }
      case include_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        ADV_PTR(end, (*end == ' ' || *end == '\t'));
        rendr_include((end - line->data));
        break;
      }
      case "undef"_uint_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        get_next_word(&start, &end);
        RENDR(R, FG_VS_CODE_BLUE, start, end);
        break;
      }
      /* case "error"_uint_hash : {
        RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
        ADV_PTR(end, (*end == ' ' || *end == '\t'));
        if (!*end || *end != '"') {
          break;
        }
        start = end;
        end += 1;
        ADV_PTR(end, (*end != '"'));
        if (*end) {
          end += 1;
        }
        render_part((start - line->data), (end - line->data), FG_YELLOW);
        break;
      } */
    }
    free(current_word);
  }
}

void render_control_statements(Ulong index) {
  switch (line->data[index]) {
    case 'e' : /* else */ {
      Ulong       else_indent = line_indent(line);
      Ulong       indent;
      const char *if_found = NULL;
      int         i        = 0;
      for (linestruct *l = line->prev; l && (i < 500); l = l->prev, ++i) {
        indent = line_indent(l);
        if (indent <= else_indent) {
          if_found = strstr(l->data, "if");
          if (if_found) {
            break;
          }
        }
      }
      if (!if_found || indent != else_indent) {
        RENDR(E, "<- Misleading indentation");
        /* Add BG_YELLOW. */
        RENDR(R, ERROR_MESSAGE, &line->data[index], &line->data[index + 4]);
      }
      break;
    }
  }
}

void rendr_classes(void) {
  PROFILE_FUNCTION;
  const char *found = word_strstr(line->data, "class");
  if (found) {
    remove_from_color_map(line, FG_VS_CODE_GREEN, CLASS_SYNTAX);
    const char *start = NULL;
    const char *end   = NULL;
    start             = found;
    start += "class"_sllen;
    ADV_PTR(start, (*start == ' ' || *start == '\t'));
    /* If there is nothing after the word class print error msg. */
    if (!*start) {
      RENDR(E, "<-(Expected class name)");
      return;
    }
    // int end_line = find_class_end_line(line);
    end = start;
    ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '{'));
    /* If the char after class name is not '{' or null terminator. */
    if (*end != '{' && *end != '\0') {
      /* We then iter to past all spaces and tabs. */
      ADV_PTR(end, (*end == ' ' || *end == '\t'));
      /* And if there is something other then '{'
       * or null-terminator.  We print error. */
      if (*end != '\0' && *end != '{') {
        RENDR(E, "<-(Expected one word as class name)");
        return;
      }
    }
    end = start;
    ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '{'));
    if (start == end) {
      return;
    }
    string name(start, (end - start));
    add_rm_color_map(name, {FG_VS_CODE_GREEN, (int)line->lineno, 100000, CLASS_SYNTAX});
    class_info_t class_info;
    class_info.name = name;
    /* const char *func_found = NULL;
    for (linestruct *cl = line; cl->lineno < end_line; cl = cl->next)
    {
        func_found = strchr(cl->data, '(');
        if (func_found)
        {
            end = func_found;
            dcr_ptr(end, cl->data, (*end != ' ' && *end != '\t'));
            adv_ptr_to_next_word(end);
            string method(end, (func_found - end));
            if (method != class_info.name)
            {
                remove_from_color_map(
                    line, FG_VS_CODE_BRIGHT_YELLOW, CLASS_METHOD_SYNTAX);
                // if (test_map.find(method) == test_map.end())
                // {
                //     test_map[method] = {
                //         FG_VS_CODE_BRIGHT_YELLOW,
                //         (int)cl->lineno,
                //         end_line,
                //         CLASS_METHOD_SYNTAX,
                //     };
                // }
                add_rm_color_map(
                    method, {FG_VS_CODE_BRIGHT_YELLOW, (int)cl->lineno,
                             end_line, CLASS_METHOD_SYNTAX});

                class_info.methods.push_back(method);
            }
        }
    } */
  }
}

void rendr_structs(int index) {
  remove_from_color_map(line, FG_VS_CODE_GREEN, STRUCT_SYNTAX);
  const char *start = &line->data[index];
  const char *end   = &line->data[index];
  ADV_TO_NEXT_WORD(end);
  if (!*end) {
    return;
  }
  start = end;
  ADV_PAST_WORD(end);
  if (end == start) {
    return;
  }
  string name(start, (end - start));
  if (test_map.find(name) == test_map.end()) {
    test_map[name] = {
      FG_VS_CODE_GREEN,
      (int)line->lineno,
      100000,
      STRUCT_SYNTAX,
    };
  }
}

void render_function(void) {
  PROFILE_FUNCTION;
  /* index_data *id = Lsp::instance().get_file_index_data(openfile->filename, false);
  if (!id) {
    return;
  }
  for (const auto &f : id->main.functions) {
    if (line->lineno >= f->start.line && line->lineno <= f->end.line) {
      for (const auto &p : f->params) {
        const char *data  = line->data;
        const char *start = NULL;
        const char *end   = NULL;
        do {
          find_word(line, data, p.name.c_str(), p.name.length(), &start, &end);
          if (start) {
            if ((line->lineno == p.e.line) && ((start - line->data) >= p.e.column - 1)) {
              break;
            }
            RENDR(R, FG_VS_CODE_BRIGHT_CYAN, start, end);
          }
          data = end;
        }
        while (data);
      }
      for (const auto &v : f->body.vars) {
        const char *data  = line->data;
        const char *start = NULL;
        const char *end   = NULL;
        do {
          find_word(line, data, v.name.c_str(), v.name.length(), &start, &end);
          if (start) {
            if ((line->lineno == v.e.line) && ((start - line->data) >= v.e.column - 1)) {
              break;
            }
            RENDR(R, FG_VS_CODE_BRIGHT_CYAN, start, end);
          }
          data = end;
        }
        while (data);
      }
    }
    else if (line->lineno < f->start.line) {
      break;
    }
  } */
}

/* Main function that applies syntax to a line in real time. */
void apply_syntax_to_line(const int row, const char *converted, linestruct *line, Ulong from_col) {
  PROFILE_FUNCTION;
  ::row       = row;
  ::converted = converted;
  ::line      = line;
  ::from_col  = from_col;
  if (openfile->type.is_set<C_CPP>()) {
    render_bracket();
    render_comment();
    if (!line->data[0] || (block_comment_start == 0 && block_comment_end == till_x)) {
      return;
    }
    line_word_t *head = line_word_list(line->data, till_x);
    while (head) {
      line_word_t *node = head;
      head              = node->next;
      if (node->start >= block_comment_start && node->end <= block_comment_end) {
        free_node(node);
        continue;
      }
      const auto &it = test_map.find(node->str);
      if (it != test_map.end()) {
        if (it->second.from_line != -1) {
          if (line->lineno >= it->second.from_line && line->lineno <= it->second.to_line) {
            if (line->lineno == it->second.to_line) {
              const char *bracket = strchr(line->data, '}');
              if (bracket && node->start > (bracket - line->data)) {
                free_node(node);
                continue;
              }
            }
            midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, it->second.color);
          }
        }
        else {
          midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, it->second.color);
          if (it->second.color == FG_VS_CODE_BRIGHT_MAGENTA) {
            render_control_statements(node->start);
          }
        }
      }
      const auto &is_var = LSP->index.vars.find(node->str);
      if (is_var != LSP->index.vars.end()) {
        for (const auto &v : is_var->second) {
          if (strcmp(tail(v.file), tail(openfile->filename)) == 0) {
            if (line->lineno >= v.decl_st && line->lineno <= v.decl_end) {
              midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_VS_CODE_BRIGHT_CYAN);
            }
          }
        }
      }
      const auto &macro = LSP->index.defines.find(node->str);
      if (macro != LSP->index.defines.end()) {
        midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_VS_CODE_BLUE);
        free_node(node);
        continue;
      }
      const auto &is_enum = LSP->index.enums.find(node->str);
      if (is_enum != LSP->index.enums.end()) {
        midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_VS_CODE_GREEN);
        free_node(node);
        continue;
      }
      const auto &tdsc = LSP->index.tdstructs.find(node->str);
      if (tdsc != LSP->index.tdstructs.end()) {
        midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_VS_CODE_GREEN);
        free_node(node);
        continue;
      }
      const auto &is_struct = LSP->index.structs.find(node->str);
      if (is_struct != LSP->index.structs.end()) {
        midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_VS_CODE_GREEN);
        free_node(node);
        continue;
      }
      const auto &is_fd = LSP->index.functiondefs.find(node->str);
      if (is_fd != LSP->index.functiondefs.end()) {
        midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, FG_VS_CODE_BRIGHT_YELLOW);
        free_node(node);
        continue;
      }
      free_node(node);
    }
    if (line->data[indent_char_len(line)] == '#') {
      render_preprossesor();
      return;
    }
    render_string_literals();
  }
  /* TODO: Fix bug where if '0.' is at end of line then we crash when trying to modify that
   * line, it`s weird tough as it only craches if this it the first action and not otherwise. */
  else if (openfile->type.is_set<ASM>()) {
    if (!line->data[0]) {
      return;
    }
    const char *comment = strpbrk(line->data, ";#");
    /* If comment is found then color from comment to end of line. */
    if (comment) {
      render_part((comment - line->data), till_x, FG_COMMENT_GREEN);
    }
    line_word_t *head = line_word_list(line->data, till_x);
    while (head) {
      line_word_t *node = head;
      head              = node->next;
      /* If there is a comment on the line skip all other words. */
      if (comment && node->start > (comment - line->data)) {
        free_node(node);
        continue;
      }
      char       *word = lower_case_word(node->str);
      const auto &it   = test_map.find(word);
      free(word);
      if (it != test_map.end()) {
        midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, it->second.color);
      }
      free_node(node);
    }
  }
  else if (openfile->type.is_set<BASH>()) {
    /* Return early when line is empty. */
    if (!line->data[0]) {
      return;
    }
    /* Look for comments. */
    const char *comment = strchr(line->data, '#');
    if (comment) {
      if (comment == line->data || line->data[(comment - line->data) - 1] != '$') {
        render_part((comment - line->data), till_x, FG_COMMENT_GREEN);
      }
      else {
        comment = NULL;
      }
    }
    line_word_t *head = line_word_list(line->data, till_x);
    while (head) {
      line_word_t *node = head;
      head = node->next;
      /* If current pos is after comment then continue. */
      if (comment && node->start > (comment - line->data)) {
        free_node(node);
        continue;
      }
      if (test_map.count(node->str)) {
        mv_add_nstr_color(midwin, row, get_start_col(line, node), node->str, node->len, test_map[node->str].color);
      }
      else if (LSP->index.bash_data.variable.count(node->str)) {
        if (line->lineno == LSP->index.bash_data.variable[node->str].lineno) {
          mv_add_nstr_color(midwin, row, get_start_col(line, node), node->str, node->len, FG_VS_CODE_BRIGHT_CYAN);
        }
        else if (node->start > 0 && line->data[node->start - 1] == '$') {
          mv_add_nstr_color(midwin, row, (get_start_col(line, node) - 1), &line->data[node->start - 1], (node->len + 1), FG_VS_CODE_BRIGHT_CYAN);
        }
      }
      free_node(node);
    }
  }
  else if (openfile->type.is_set<GLSL>()) {
    if (!line->data[0]) {
      return;
    }
    if (line->data[indent_char_len(line)] == '#') {
      render_preprossesor();
      return;
    }
    render_comment();
    /* Retrieve all words in the current line. */
    line_word_t *head = line_word_list(line->data, till_x);
    while (head) {
      /* Assign head to node, and assign head to the next word. */
      line_word_t *node = head;
      head              = node->next;
      /* Search the syntax map for the word. */
      const auto &it = test_map.find(node->str);
      if (it != test_map.end()) {
        /* If found use the map element to fetch the color. */
        mv_add_nstr_color(midwin, row, get_start_col(line, node), it->first.c_str(), it->first.length(), it->second.color);
        free_node(node);
        continue;
      }
      /* Function defenitions. */
      const auto &func_decl = LSP->index.functiondefs.find(node->str);
      if (func_decl != LSP->index.functiondefs.end()) {
        const auto &[name, data] = *func_decl;
        /* If found use the map element to fetch the color. */
        mv_add_nstr_color(midwin, row, get_start_col(line, node), name.c_str(), name.length(), FG_VS_CODE_BRIGHT_YELLOW);
        free_node(node);
        continue;
      }
      /* Vars. */
      const auto &var = LSP->index.vars.find(node->str);
      if (var != LSP->index.vars.end()) {
        const auto &[name, vector] = *var;
        for (const auto &v : vector) {
          if (line->lineno >= v.decl_st && line->lineno <= v.decl_end) {
            mv_add_nstr_color(midwin, row, get_start_col(line, node), name.c_str(), name.length(), FG_VS_CODE_BRIGHT_CYAN);
          }
        }
      }
      free_node(node);
    }
  }
  else if (openfile->type.is_set<SYSTEMD_SERVICE>()) {
    /* Return early on empty line. */
    if (!line->data[0]) {
      return;
    }
    line_word_t *head = line_word_list(line->data, till_x);
    while (head) {
      line_word_t *node = head;
      head = node->next;
      if (test_map.count(node->str)) {
        mv_add_nstr_color(midwin, row, get_start_col(line, node), node->str, node->len, test_map[node->str].color);
      }
      free_node(node);
    }
  }
}
