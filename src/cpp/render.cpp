#include "../include/prototypes.h"

static Uint block_comment_start = (Uint)-1;
static Uint block_comment_end   = (Uint)-1;
static int  color_bi[3]         = {FG_VS_CODE_YELLOW, FG_VS_CODE_BRIGHT_MAGENTA, FG_VS_CODE_BRIGHT_BLUE};

vec<char *>          includes;
vec<char *>          defines;
vec<char *>          structs;
vec<char *>          classes;
vec<char *>          funcs;
vec<glob_var_t>      glob_vars;
vec<function_info_t> local_funcs;

static int         row       = 0;
static const char *converted = nullptr;
static linestruct *line      = nullptr;
static Ulong       from_col  = 0;

char              suggest_buf[1024] = "";
char             *suggest_str       = nullptr;
int               suggest_len       = 0;
vec<const char *> types             = {
  "void", "char",  "int",   "unsigned", "extern",  "volatile", "static",
  "long", "short", "const", "bool",     "typedef", "class",
};

unordered_map<string, syntax_data_t> test_map;
vector<class_info_t>                 class_info_vector;
vector<var_t>                        var_vector;

void render_part(Ulong match_start, Ulong match_end, short color) {
  PROFILE_FUNCTION;
  const char *thetext   = nullptr;
  int         paintlen  = 0;
  int         start_col = 0;
  if ((match_start >= till_x)) {
    return;
  }
  if (match_start > from_x) {
    start_col = (int)(wideness(line->data, (int)match_start) - from_col);
  }
  thetext  = converted + actual_x(converted, start_col);
  paintlen = (int)actual_x(thetext, wideness(line->data, match_end) - from_col - start_col);
  midwin_mv_add_nstr_color(row, (margin + start_col), thetext, paintlen, color);
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

/* Render the text of a given line.  Note that this function only renders the
 * text and nothing else. */
void render_line_text(const int row, const char *str, linestruct *line, const Ulong from_col) {
  if (margin > 0) {
    WIN_COLOR_ON(midwin, LINE_NUMBER);
    if (ISSET(SOFTWRAP) && from_col != 0) {
      mvwprintw(midwin, row, 0, "%*s", margin - 1, " ");
    }
    else {
      mvwprintw(midwin, row, 0, "%*lu", margin - 1, line->lineno);
    }
    WIN_COLOR_OFF(midwin, LINE_NUMBER);
    if (line->has_anchor == true && (from_col == 0 || !ISSET(SOFTWRAP))) {
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
  if (sidebar) {
    mvwaddch(midwin, row, COLS - 1, bardata[row]);
  }
}

/* Set start and end pos for comment block or if the entire
 * line is inside of a block comment set 'block_comment_start'
 * to '0' and 'block_comment_end' to '(Uint)-1'. */
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
        /* If there is a slash comment infront the block comment. Then
         * of cource we still color the text from the slash to the block
         * start after we error highlight the block start. */
        if (slash && (slash - line->data) < (start - line->data)) {
          midwin_mv_add_nstr_color(row, (wideness(line->data, (slash - line->data))) + margin, slash,
                                   (start - line->data) - (slash - line->data), FG_GREEN);
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

/* Color brackets based on indent.  TODO: This needs to be fix. */
void render_bracket(void) {
  const char *start = strchr(line->data, '{');
  const char *end   = strrchr(line->data, '}');
  /* Bracket start and end on the same line. */
  if (start && end) {
    while (start) {
      RENDR(R_CHAR, FG_VS_CODE_YELLOW, start);
      if (line->data[(start - line->data) + 1] == '\0') {
        start = nullptr;
        continue;
      }
      start = strchr(start + 1, '{');
    }
    while (end) {
      RENDR(R_CHAR, FG_VS_CODE_YELLOW, end);
      Uint last_pos = last_strchr(line->data, '}', (end - line->data));
      if (last_pos < till_x && last_pos != 0) {
        end = &line->data[last_pos];
      }
      else {
        end = nullptr;
      }
    }
    if (line->prev && (line->prev->flags.is_set<IN_BRACKET>() || line->prev->flags.is_set<BRACKET_START>())) {
      line->flags.set<IN_BRACKET>();
    }
  }
  /* Start bracket line was found. */
  else if (start && !end) {
    line->flags.set<BRACKET_START>();
    RENDR(R_CHAR, color_bi[(line_indent(line) % 3)], start);
    if (line->prev && (line->prev->flags.is_set<IN_BRACKET>() || line->prev->flags.is_set<BRACKET_START>())) {
      line->flags.set<IN_BRACKET>();
    }
  }
  /* End bracket line was found. */
  else if (!start && end) {
    line->flags.set<IN_BRACKET>();
    line->flags.unset<BRACKET_START>();
    RENDR(R_CHAR, color_bi[(line_indent(line) % 3)], end);
    for (linestruct *t_line = line->prev; t_line != nullptr; t_line = t_line->prev) {
      if (t_line->flags.is_set<BRACKET_START>()) {
        if (line_indent(t_line) == line_indent(line)) {
          if (t_line->prev && t_line->prev->flags.is_set(IN_BRACKET)) {
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
  else if (!start && !end) {
    if (line->prev && (line->prev->flags.is_set<IN_BRACKET>() || line->prev->flags.is_set<BRACKET_START>())) {
      line->flags.set<IN_BRACKET>();
    }
    else {
      line->flags.unset<IN_BRACKET>();
    }
  }
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
  const char *end   = nullptr;
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
  while (start != nullptr) {
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
  const char *start = nullptr, *end = nullptr, *param = nullptr;
  char       *word = nullptr;
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
      (word != nullptr) ? free(word) : void();
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
  if (*end != '\0') {
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
    if (*end == '\0') {
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
  while (defined != nullptr) {
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
      if (*parent_start != '\0' && *parent_end != '\0') {
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
        const char *error_end = nullptr;
        while (*p && *p != '&' && *p != '|') {
          p += 1;
          ADV_PTR(p, (*p != '&') && (*p != '|') && (*p != '(') && (*p != ')'));
          if (*p == '(' || *p == ')') {
            error_end = p;
          }
        }
        if (error_end != nullptr) {
          error_end += 1;
          RENDR(C_PTR, ERROR_MESSAGE, start, error_end);
        }
        break;
      }
      else if ((*parent_start == '\0' && *parent_end != '\0') || (*parent_start != '\0' && *parent_end == '\0')) {
        if (*parent_start != '\0') {
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

/* This 'render' sub-system is responsible for handeling all pre-prossesor
 * syntax.  TODO: Create a structured way to parse, and then create a
 * system to include error handeling in real-time. */
void render_preprossesor(void) {
  char       *current_word = nullptr;
  const char *start        = strchr(line->data, '#');
  const char *end          = nullptr;
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
        /* case "error"_uint_hash :
        {
            RENDR(R, FG_VS_CODE_BRIGHT_MAGENTA, start, end);
            ADV_PTR(end, (*end == ' ' || *end == '\t'));
            if (!*end || *end != '"')
            {
                break;
            }
            start = end;
            end += 1;
            ADV_PTR(end, (*end != '"'));
            if (*end)
            {
                end += 1;
            }
            render_part(
                (start - line->data), (end - line->data), FG_YELLOW);
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
      const char *if_found = nullptr;
      int         i        = 0;
      for (linestruct *l = line->prev; l != nullptr && (i < 500); l = l->prev, i++) {
        indent = line_indent(l);
        if (indent <= else_indent) {
          if_found = strstr(l->data, "if");
          if (if_found) {
            break;
          }
        }
      }
      if (if_found == nullptr || indent != else_indent) {
        RENDR(E, "<- Misleading indentation");
        /* Add BG_YELLOW. */
        RENDR(R, ERROR_MESSAGE, &line->data[index], &line->data[index + 4]);
      }
      break;
    }
  }
}

void rendr_glob_vars(void) {
  if (line->flags.is_set(IN_BLOCK_COMMENT) || line->flags.is_set(SINGLE_LINE_BLOCK_COMMENT) ||
      line->flags.is_set(BLOCK_COMMENT_START) || line->flags.is_set(BLOCK_COMMENT_END)) {
    return;
  }
  const char *data    = nullptr;
  const char *start   = nullptr;
  bool        in_func = false;
  for (const auto &f : local_funcs) {
    if (line->lineno >= f.start_bracket && line->lineno <= f.end_braket) {
      in_func = true;
      break;
    }
  }
  if (!in_func) {
    data  = line->data;
    start = strchr(data, ';');
    if (start && line->data[indent_char_len(line)] != '}') {
      char      *str = measured_copy(line->data, (start - line->data) + 1);
      glob_var_t ngvar;
      parse_variable(str, &ngvar.type, &ngvar.name, &ngvar.value);
      if (ngvar.name) {
        bool found = false;
        for (const auto &gv : glob_vars) {
          if (strcmp(gv.name, ngvar.name) == 0) {
            found = true;
            break;
          }
        }
        if (found == false) {
          const auto &it = test_map.find(ngvar.name);
          if (it == test_map.end()) {
            test_map[ngvar.name].color = FG_VS_CODE_BRIGHT_CYAN;
            glob_vars.push_back(ngvar);
            nlog("ngvar str: %s\n", str);
          }
        }
      }
      free(str);
    }
  }
}

void rendr_classes(void) {
  PROFILE_FUNCTION;
  const char *found = word_strstr(line->data, "class");
  if (found) {
    remove_from_color_map(line, FG_VS_CODE_GREEN, CLASS_SYNTAX);
    const char *start = nullptr;
    const char *end   = nullptr;
    start             = found;
    start += "class"_sllen;
    ADV_PTR(start, (*start == ' ' || *start == '\t'));
    /* If there is nothing after the word class print error msg. */
    if (*start == '\0') {
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
    /* const char *func_found = nullptr;
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
    class_info_vector.push_back(class_info);
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
        const char *start = nullptr;
        const char *end   = nullptr;
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
        const char *start = nullptr;
        const char *end   = nullptr;
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
      free_node(node);
    }
    if (line->data[indent_char_len(line)] == '#') {
      render_preprossesor();
      return;
    }
    render_string_literals();
    // render_char_strings();
    /* if (line->flags.is_set(DONT_PREPROSSES_LINE)) {
      render_part(0, till_x, FG_SUGGEST_GRAY);
      return;
    } */
  }
  else if (openfile->type.is_set<ASM>()) {}
  else if (openfile->type.is_set<BASH>()) {
    const char *comment = strchr(line->data, '#');
    if (comment) {
      if (comment == line->data || line->data[(comment - line->data) - 1] != '$') {
        render_part((comment - line->data), till_x, FG_COMMENT_GREEN);
      }
      else {
        comment = nullptr;
      }
    }
    line_word_t *head = line_word_list(line->data, till_x);
    while (head) {
      line_word_t *node = head;
      head              = node->next;
      if (comment && node->start > (comment - line->data)) {
        free_node(node);
        continue;
      }
      const auto &it = test_map.find(node->str);
      if (it != test_map.end()) {
        midwin_mv_add_nstr_color(row, get_start_col(line, node), node->str, node->len, it->second.color);
      }
      free_node(node);
    }
  }
}

void rendr_suggestion(void) {
  PROFILE_FUNCTION;
  suggest_str = nullptr;
  if (suggestwin) {
    delwin(suggestwin);
  }
  if (!openfile->current_x || (!is_word_char(&openfile->current->data[openfile->current_x - 1], false) &&
                               openfile->current->data[openfile->current_x - 1] != '_')) {
    clear_suggestion();
    return;
  }
  if (suggest_len < 0) {
    clear_suggestion();
  }
  add_char_to_suggest_buf();
  find_suggestion();
  draw_suggest_win();
}

/* Cleans up all thing related to rendering. */
void cleanup_rendr(void) {
  for (int i = 0; i < includes.get_size(); ++i) {
    free(includes[i]);
  }
  for (int i = 0; i < defines.get_size(); ++i) {
    free(defines[i]);
  }
}
