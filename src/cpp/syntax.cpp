#include "../include/prototypes.h"

static void do_bash_synx(void) {
  openfile->type.clear_and_set<BASH>();
  const auto &it = test_map.find("if");
  if (it == test_map.end()) {
    test_map["if"]    = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["elif"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["else"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["fi"]    = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["then"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["case"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["in"]    = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["esac"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["for"]   = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["while"] = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["do"]    = {FG_VS_CODE_BRIGHT_MAGENTA};
    test_map["done"]  = {FG_VS_CODE_BRIGHT_MAGENTA};
  }
}

string get_word_after(const char *data, const char *word) {
  const char *found = word_strstr(data, word);
  if (!found) {
    return "";
  }
  const char *start = found + strlen(word);
  ADV_TO_NEXT_WORD(start);
  if (!*start) {
    return "";
  }
  const char *end = start;
  ADV_PAST_WORD(end);
  return string(start, (end - start));
}

/* Function to check syntax for a open buffer. */
void syntax_check_file(openfilestruct *file) {
  PROFILE_FUNCTION;
  file->type.clear();
  if (!file->filetop->next) {
    return;
  }
  if (ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX)) {
    const string ext = file_extention_str();
    if (ext == "cpp" || ext == "c" || ext == "cc" || ext == "h" || ext == "hpp") {
      file->type.clear_and_set<C_CPP>();
      /* Types. */
      test_map["bool"]      = {FG_VS_CODE_BLUE};
      test_map["char"]      = {FG_VS_CODE_BLUE};
      test_map["short"]     = {FG_VS_CODE_BLUE};
      test_map["int"]       = {FG_VS_CODE_BLUE, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["long"]      = {FG_VS_CODE_BLUE};
      test_map["unsigned"]  = {FG_VS_CODE_BLUE};
      test_map["float"]     = {FG_VS_CODE_BLUE};
      test_map["double"]    = {FG_VS_CODE_BLUE};
      test_map["void"]      = {FG_VS_CODE_BLUE};
      test_map["static"]    = {FG_VS_CODE_BLUE};
      test_map["extern"]    = {FG_VS_CODE_BLUE};
      test_map["constexpr"] = {FG_VS_CODE_BLUE};
      test_map["const"]     = {FG_VS_CODE_BLUE};
      test_map["true"]      = {FG_VS_CODE_BLUE};
      test_map["false"]     = {FG_VS_CODE_BLUE};
      test_map["true"]      = {FG_VS_CODE_BLUE};
      test_map["false"]     = {FG_VS_CODE_BLUE};
      test_map["nullptr"]   = {FG_VS_CODE_BLUE};
      test_map["typedef"]   = {FG_VS_CODE_BLUE};
      test_map["sizeof"]    = {FG_VS_CODE_BLUE};
      test_map["struct"]    = {FG_VS_CODE_BLUE, -1, -1, IS_WORD_STRUCT};
      test_map["class"]     = {FG_VS_CODE_BLUE, -1, -1, IS_WORD_CLASS};
      test_map["enum"]      = {FG_VS_CODE_BLUE};
      test_map["namespace"] = {FG_VS_CODE_BLUE};
      test_map["inline"]    = {FG_VS_CODE_BLUE};
      test_map["typename"]  = {FG_VS_CODE_BLUE};
      test_map["template"]  = {FG_VS_CODE_BLUE};
      test_map["volatile"]  = {FG_VS_CODE_BLUE};
      test_map["public"]    = {FG_VS_CODE_BLUE};
      test_map["private"]   = {FG_VS_CODE_BLUE};
      test_map["explicit"]  = {FG_VS_CODE_BLUE};
      test_map["this"]      = {FG_VS_CODE_BLUE};
      test_map["union"]     = {FG_VS_CODE_BLUE};
      test_map["auto"]      = {FG_VS_CODE_BLUE};
      test_map["noexcept"]  = {FG_VS_CODE_BLUE, -1, -1, DEFAULT_TYPE_SYNTAX};
      /* Control statements. */
      test_map["if"]       = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["else"]     = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["case"]     = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["switch"]   = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["default"]  = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["for"]      = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["while"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["return"]   = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["break"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["do"]       = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["continue"] = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["using"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["operator"] = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["try"]      = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["catch"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, DEFAULT_TYPE_SYNTAX};
    }
    else if (ext == "asm" || ext == "s" || ext == "S") {
      file->type.clear_and_set<ASM>();
      /* 64-bit registers. */
      test_map["rax"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rbx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rcx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rdx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rsi"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rdi"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rbp"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rsp"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rip"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r8"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r9"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r10"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r11"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r12"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r13"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r14"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r15"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      /* 32-bit registers. */
      test_map["edx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      /* 16-bit registers. */
      test_map["dx"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      /* Lover 8-bit of 16-bit registers. */
      test_map["dl"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["al"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      /* Higher 8-bit of 16-bit registers. */
      test_map["dh"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      /* Simd 'sse' registers. */
      test_map["xmm0"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["xmm1"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["xmm2"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["xmm3"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["xmm4"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["xmm5"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["xmm6"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["xmm7"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      /* Simd 'avx' registers. */
      test_map["ymm0"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["ymm1"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["ymm2"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["ymm3"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["ymm4"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["ymm5"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["ymm6"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["ymm7"] = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      /* 'avx' instructions. */
      test_map["vmovdqa"] = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["vmovaps"] = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["vaddps"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["vsubps"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      /* Instructions. */
      test_map["syscall"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["mov"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["xor"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["pxor"]     = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["cmp"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["cmpb"]     = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["je"]       = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["inc"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["int"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["jmp"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["ret"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["add"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["sub"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["mul"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["and"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["test"]     = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["jz"]       = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["jnz"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["jmp"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["bsf"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["equ"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["lea"]      = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["movdqa"]   = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["movdqu"]   = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["pcmpeqb"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["pmovmskb"] = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      /* Control words. */
      test_map["section"] = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, ASM_CONTROL};
      test_map["global"]  = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, ASM_CONTROL};
      test_map["db"]      = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, ASM_CONTROL};
      test_map["byte"]    = {FG_VS_CODE_BRIGHT_MAGENTA, -1, -1, ASM_CONTROL};
    }
    else if (ext == "sh") {
      do_bash_synx();
    }
    else if (ext == "glsl") {
      file->type.clear_and_set<GLSL>();
      /* Standard types. */
      test_map["mat4"]  = {FG_VS_CODE_BLUE};
      test_map["mat3"]  = {FG_VS_CODE_BLUE};
      test_map["vec4"]  = {FG_VS_CODE_BLUE};
      test_map["vec3"]  = {FG_VS_CODE_BLUE};
      test_map["vec2"]  = {FG_VS_CODE_BLUE};
      test_map["int"]   = {FG_VS_CODE_BLUE};
      test_map["float"] = {FG_VS_CODE_BLUE};
      /* Control types. */
      test_map["return"] = {FG_VS_CODE_BRIGHT_MAGENTA};
      test_map["if"]     = {FG_VS_CODE_BRIGHT_MAGENTA};
      test_map["else"]   = {FG_VS_CODE_BRIGHT_MAGENTA};
      // LSP->index_file(file->filename);
    }
    /** TODO: Check that this is fully safe. */
    else {
      linestruct *line = file->filetop;
      while (line && is_empty_line(line)) {
        line = line->next;
      }
      if (line && strstr(line->data, "#!")) {
        do_bash_synx();
      }
    }
  }
}

bool parse_color_opts(const char *color_fg, const char *color_bg, short *fg, short *bg, int *attr) {
  bool vivid, thick;
  *attr = A_NORMAL;
  if (color_fg != nullptr) {
    if (strncmp(color_fg, "bold", 4) == 0) {
      *attr |= A_BOLD, color_fg += 5;
    }
    if (strncmp(color_fg, "italic", 6) == 0) {
      *attr |= A_ITALIC, color_fg += 7;
    }
    *fg = color_to_short(color_fg, vivid, thick);
    if (*fg == BAD_COLOR) {
      return false;
    }
    if (vivid && !thick && COLORS > 8) {
      fg += 8;
    }
    else if (vivid) {
      *attr |= A_BOLD;
    }
  }
  else {
    *fg = THE_DEFAULT;
  }
  if (color_bg != nullptr) {
    if (strncmp(color_bg, "bold", 4) == 0) {
      *attr |= A_BOLD;
      color_bg += 5;
    }
    if (strncmp(color_bg, "italic", 6) == 0) {
      *attr |= A_ITALIC;
      color_bg += 7;
    }
    *bg = color_to_short(color_bg, vivid, thick);
    if (*bg == BAD_COLOR) {
      return false;
    }
    if (vivid && COLORS > 8) {
      bg += 8;
    }
  }
  else {
    *bg = THE_DEFAULT;
  }
  return true;
}

/* Add some "basic" cpp syntax. */
void do_syntax(void) {
  // flag_all_brackets();
  // flag_all_block_comments(openfile->filetop);
}

void handle_struct_syntax(char **word) {
  Ulong i;
  while (*(*word) == '*') {
    *word += 1;
  }
  for (i = 0; (*word)[i]; i++) {
    if ((*word)[i] == ';') {
      (*word)[i] = '\0';
      break;
    }
    if ((*word)[i] == ')') {
      (*word)[i] = '\0';
      break;
    }
  }
}

void find_block_comments(int from, int end) {
  PROFILE_FUNCTION;
  linestruct *line = line_from_number(from);
  for (; line && line->lineno != end; line = line->next) {
    const char *found_start = strstr(line->data, "/*");
    const char *found_end   = strstr(line->data, "*/");
    if (found_start && found_end)
      ;
    else if (found_start && !found_end) {
      line->flags.set(BLOCK_COMMENT_START);
      continue;
    }
    else if (!found_start && found_end) {
      line->flags.set(BLOCK_COMMENT_END);
      continue;
    }
    else if (!found_start && !found_end) {
      if (line->prev != nullptr &&
          ((line->prev->flags.is_set(BLOCK_COMMENT_START)) || (line->prev->flags.is_set(IN_BLOCK_COMMENT)))) {
        line->flags.set(IN_BLOCK_COMMENT);
      }
    }
  }
}

char **find_functions_in_file(char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    return nullptr;
  }
  long         len;
  Ulong        size;
  static char *line;
  Ulong        acap = 10, asize = 0;
  char       **func_str_array = (char **)nmalloc(acap * sizeof(char *));
  bool         in_bracket     = false;
  while ((len = getline(&line, &size, file)) != EOF) {
    if (line[len - 1] == '\n') {
      line[--len] = '\0';
    }
    if (strchr(line, '{')) {
      in_bracket = true;
    }
    if (strchr(line, '}')) {
      in_bracket = false;
    }
    if (in_bracket) {
      continue;
    }
    const char *p = line;
    while (*p == ' ' || *p == '\t') {
      p++;
    }
    if (strchr(line, '#') || strstr(line, "operator") || strchr(line, '[')) {
      continue;
    }
    const char *start        = line;
    const char *end          = line;
    const char *parent_start = strchr(line, '(');
    if (parent_start) {
      start = parent_start;
      for (; start > line && *start != ' ' && *start != '\t' && *start != '*' && *start != '&'; start--);
      if (start > p) {
        start += 1;
        if (strstr(line, "__nonnull") != nullptr || line[(start - line) + 1] == '*' || *start == '(') {
          continue;
        }
        end = strchr(line, ';');
        if (end) {
          char *func_str    = measured_memmove_copy(start, (end - start));
          char *return_type = copy_of("void ");
          char *full_func   = alloc_str_free_substrs(return_type, func_str);
          if (acap == asize) {
            acap *= 2;
            func_str_array = (char **)nrealloc(func_str_array, acap * sizeof(char *));
          }
          func_str_array[asize++] = full_func;
        }
        /* Here we can check for edge cases. */
      }
    }
  }
  fclose(file);
  func_str_array[asize] = nullptr;
  return func_str_array;
}

char **find_variabels_in_file(char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    return nullptr;
  }
  long         len;
  Ulong        size;
  static char *line;
  Ulong        acap = 10, asize = 0;
  char       **var_str_array = (char **)nmalloc(acap * sizeof(char *));
  bool         in_bracket    = false;
  while ((len = getline(&line, &size, file)) != EOF) {
    if (line[len - 1] == '\n') {
      line[--len] = '\0';
    }
    if (strchr(line, '{')) {
      in_bracket = true;
    }
    if (strchr(line, '}')) {
      in_bracket = false;
    }
    if (in_bracket) {
      continue;
    }
    const char *parent_start = strchr(line, '(');
    const char *parent_end   = strchr(line, ')');
    const char *assign       = strchr(line, ';');
    if (parent_start || parent_end) {
      continue;
    }
    if (assign) {
      char *str = measured_memmove_copy(line, (assign - line) + 1);
      if (acap == asize) {
        acap *= 2;
        var_str_array = (char **)nrealloc(var_str_array, acap * sizeof(char *));
      }
      var_str_array[asize++] = str;
    }
  }
  fclose(file);
  var_str_array[asize] = nullptr;
  return var_str_array;
}
