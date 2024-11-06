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
      openfile->type.clear_and_set<C_CPP>();
      /* Types. */
      test_map["bool"]      = {FG_VS_CODE_BLUE};
      test_map["char"]      = {FG_VS_CODE_BLUE};
      test_map["short"]     = {FG_VS_CODE_BLUE};
      test_map["int"]       = {FG_VS_CODE_BLUE, -1, -1, DEFAULT_TYPE_SYNTAX};
      test_map["long"]      = {FG_VS_CODE_BLUE};
      test_map["unsigned"]  = {FG_VS_CODE_BLUE};
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
      test_map["rax"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rbx"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rcx"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rdx"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rsi"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rdi"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rbp"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rsp"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["rip"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r8"]   = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r9"]   = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r10"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r11"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r12"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r13"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r14"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["r15"]  = {FG_VS_CODE_GREEN, -1, -1, ASM_REG};
      test_map["mov"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["xor"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["cmp"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["cmpb"] = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["je"]   = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["inc"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["jmp"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["ret"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["add"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["sub"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
      test_map["mul"]  = {FG_VS_CODE_BLUE, -1, -1, ASM_INSTRUCT};
    }
    else if (ext == "sh") {
      do_bash_synx();
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

bool check_func_syntax(char ***words, Ulong *i) {
  Ulong          at   = 0;
  unsigned short type = 0;
  if ((*words)[++(*i)] == nullptr) {
    return false;
  }
  /* Remove all if any preceding '*' char`s. */
  while (*((*words)[*i]) == '*') {
    (*words)[*i] += 1;
  }
  if (is_word_func((*words)[*i], &at)) {
    if (!syntax_func((*words)[*i])) {
      new_syntax_func((*words)[*i]);
      // add_syntax_word(FUNC_COLOR, nullptr, rgx_word((*words)[*i]));
      // sub_thread_compile_add_rgx(FUNC_COLOR, nullptr,
      // rgx_word((*words)[*i]), &last_c_color);
    }
    (*words)[*i] += at + 1;
    type = retrieve_c_syntax_type((*words)[*i]);
    if (type) {
      if ((*words)[++(*i)] != nullptr) {}
    }
    return true;
  }
  if ((*words)[(*i) + 1] != nullptr) {
    if (*((*words)[(*i) + 1]) == '(') {
      if (!syntax_func((*words)[*i])) {
        new_syntax_func((*words)[*i]);
        // add_syntax_word(FUNC_COLOR, nullptr, rgx_word((*words)[*i]));
        // sub_thread_compile_add_rgx(FUNC_COLOR, nullptr,
        // rgx_word((*words)[*i]), &last_c_color);
      }
      return true;
    }
  }
  return false;
}

/* Check a file for syntax, and add relevent syntax. */
void check_syntax(const char *path) {
  if (!is_file_and_exists(path)) {
    return;
  }
  char *buf = nullptr, **words;
  Ulong size, len, i;
  FILE *f = fopen(path, "rb");
  if (f == nullptr) {
    return;
  }
  while ((len = getline(&buf, &size, f)) != EOF) {
    if (buf[len - 1] == '\n') {
      buf[--len] = '\0';
    }
    words = words_in_str(buf);
    if (*words == nullptr) {
      continue;
    }
    for (i = 0; words[i] != nullptr; i++) {
      const unsigned short type = retrieve_c_syntax_type(words[i]);
      if (words[i + 1] == nullptr) {
        break;
      }
      if (!type) {
        continue;
      }
      else if (type & CS_STRUCT) {
        if (*words[i + 1] == '{' || *words[i + 1] == '*')
          ;
        else if (!is_syntax_struct(words[++i])) {
          // sub_thread_compile_add_rgx("brightgreen", nullptr,
          // rgx_word(words[i]), &last_c_color);
        }
      }
      else if (type & CS_CLASS) {
        if (*words[i + 1] == '{')
          ;
        else if (!is_syntax_class(words[++i])) {
          // sub_thread_compile_add_rgx("brightgreen", nullptr,
          // rgx_word(words[i]), &last_c_color);
        }
      }
      else if (type & CS_ENUM) {
        /* If this is a anonumus enum skip, (will be implemented later).
         */
        if (*words[i + 1] == '{')
          ;
        else {
          // sub_thread_compile_add_rgx("brightgreen", nullptr,
          // rgx_word(words[++i]), &last_c_color);
        }
      }
      else if (type & CS_CHAR || type & CS_VOID || type & CS_INT || type & CS_LONG || type & CS_BOOL ||
               type & CS_SIZE_T || type & CS_SSIZE_T) {
        if (check_func_syntax(&words, &i)) {
          continue;
        }
      }
      else if (type & CS_DEFINE) {
        handle_define(words[++i]);
      }
      /* TODO: Fix check_multis before. */
      // else if (type & CS_INCLUDE)
      // {
      //     /* If the include file is a 'local' file, then base
      //      * the full path on current path. */
      //     if (*words[++i] == '"')
      //     {
      //         char         *rpath = strdup(path);
      //         Ulong j = 0, pos = 0;
      //         for (; rpath[j]; j++)
      //         {
      //             if (rpath[j] == '/')
      //             {
      //                 pos = j;
      //             }
      //         }
      //         if (pos != 0)
      //         {
      //             rpath[pos] = '\0';
      //         }
      //         const char *full_rpath = concat_path(rpath,
      //         extract_include(words[i])); if
      //         (is_file_and_exists(full_rpath))
      //         {
      //             if (!is_in_handled_includes_vec(full_rpath))
      //             {
      //                 add_to_handled_includes_vec(full_rpath);
      //                 check_syntax(full_rpath);
      //             }
      //         }
      //         free(rpath);
      //     }
      //     else if (*words[i] == '<')
      //     {
      //         const char *rpath = concat_path("/usr/include/",
      //         extract_include(words[i])); if
      //         (is_file_and_exists(rpath))
      //         {
      //             if (!is_in_handled_includes_vec(rpath))
      //             {
      //                 add_to_handled_includes_vec(rpath);
      //                 check_syntax(rpath);
      //             }
      //             continue;
      //         }
      //         rpath = concat_path("/usr/include/c++/v1/",
      //         extract_include(words[i])); if
      //         (is_file_and_exists(rpath))
      //         {
      //             if (!is_in_handled_includes_vec(rpath))
      //             {
      //                 add_to_handled_includes_vec(rpath);
      //                 check_syntax(rpath);
      //             }
      //         }
      //     }
      // }
    }
    prosses_callback_queue();
    free(words);
  }
  fclose(f);
}

void check_include_file_syntax(const char *path) {
  if (!is_file_and_exists(path)) {
    return;
  }
  PROFILE_FUNCTION;
  unsigned int type;
  unsigned     i;
  Ulong        nwords;
  char       **words = words_from_file(path, &nwords);
  if (words == nullptr) {
    return;
  }
  bool in_comment = false;
  for (i = 0; i < nwords; i++) {
    if (*words[i] == '#') {
      if (words[i][1] == '\0') {
        ++i;
      }
      else {
        Ulong slen = strlen(words[i]) - 1;
        memmove(words[i], words[i] + 1, slen);
        words[i][slen] = '\0';
      }
    }
    if (strcmp(words[i], "/*") == 0) {
      in_comment = true;
    }
    else if (strcmp(words[i], "*/") == 0) {
      in_comment = false;
    }
    if (in_comment) {
      continue;
    }
    type = retrieve_c_syntax_type(words[i]);
    if (!type) {
      continue;
    }
    else if (type & CS_STRUCT) {
      if (words[++i] != nullptr) {
        if (*words[i] == '{' || *words[i] == '*') {
          continue;
        }
        else if (!is_syntax_struct(words[i])) {
          char *struct_str = copy_of(words[i]);
          // color_map[struct_str].color = FG_VS_CODE_BRIGHT_GREEN;
          structs.push_back(struct_str);
        }
      }
    }
    else if (type & CS_CLASS) {
      if (words[++i] != nullptr) {
        if (*words[i] == '{' || *words[i] == '*') {
          continue;
        }
        else if (!is_syntax_class(words[i])) {
          char *class_str = copy_of(words[i]);
          // color_map[class_str].color = FG_VS_CODE_BRIGHT_GREEN;
          classes.push_back(class_str);
        }
      }
    }
    /* else if (type & CS_ENUM)
    {
        if (words[++i] != nullptr)
        {
            if (*words[i] == '{' || *words[i] == '*')
            {
                continue;
            }
            add_syntax_word(STRUCT_COLOR, nullptr, rgx_word(words[i]));
        }
    } */
    else if (type & CS_CHAR || type & CS_VOID || type & CS_INT || type & CS_LONG || type & CS_BOOL ||
             type & CS_SIZE_T || type & CS_SSIZE_T) {
      if (words[++i] != nullptr) {
        Ulong at;
        if (char_is_in_word(words[i], '(', &at)) {
          if (at != 0) {
            words[i][at] = '\0';
            if (!char_is_in_word(words[i], '=', &at)) {
              strip_leading_chars_from(words[i], '*');
              if (words[i] != nullptr) {
                if (!syntax_func(words[i])) {
                  char *func_str = copy_of(words[i]);
                  // color_map[func_str].color =
                  //     FG_VS_CODE_BRIGHT_YELLOW;
                  funcs.push_back(func_str);
                }
              }
            }
          }
        }
        else if (words[i + 1] && *words[i + 1] == '(') {
          strip_leading_chars_from(words[i], '*');
          if (!syntax_func(words[i])) {
            char *func_str = copy_of(words[i]);
            // color_map[func_str].color = FG_VS_CODE_BRIGHT_YELLOW;
            funcs.push_back(func_str);
          }
        }
      }
    }
    else if (type & CS_DEFINE) {
      if (words[++i] != nullptr) {
        handle_define(words[i]);
      }
    }
  }
  for (i = 0; i < nwords; i++) {
    free(words[i]);
  }
  free(words);
}

/* Add a '#define' to syntax. */
void handle_define(char *str) {
  unsigned int i;
  if (*str == '\\') {
    return;
  }
  for (i = 0; str[i]; i++) {
    if (str[i] == '(') {
      str[i] = '\0';
      break;
    }
  }
  if (!define_exists(str)) {
    char *define_str = copy_of(str);
    // color_map[define_str].color = FG_VS_CODE_BLUE;
    funcs.push_back(define_str);
    defines.push_back(define_str);
  }
}

/* This keeps track of the last type when a type is descovered and there is no
 * next word. */
static Ushort last_type = 0;
/* Check a line for syntax words, also index files that are included and add
 * functions as well. */
void check_for_syntax_words(linestruct *line) {
  Ulong  i;
  char **words;
  if (is_empty_line(line)) {
    return;
  }
  words = words_in_str(line->data);
  if (words == nullptr) {
    return;
  }
  for (i = 0; words[i] != nullptr; i++) {
    if (is_syntax_struct(words[i])) {
      if (words[i + 1] != nullptr) {
        if (*words[i + 1] == '(')
          ;
        else {
          handle_struct_syntax(&words[i + 1]);
          if (!syntax_var(words[++i])) {
            new_syntax_var(words[i]);
            // sub_thread_compile_add_rgx(VAR_COLOR, nullptr,
            // rgx_word(words[i]), &last_c_color);
          }
        }
      }
    }
    else if (is_syntax_class(words[i])) {
      if (words[i + 1] != nullptr) {
        handle_struct_syntax(&words[i + 1]);
        if (!syntax_var(words[i])) {
          new_syntax_var(words[i]);
          // sub_thread_compile_add_rgx(VAR_COLOR, nullptr,
          // rgx_word(words[++i]), &last_c_color);
        }
      }
    }
    const unsigned short type = retrieve_c_syntax_type(words[i]);
    if (last_type != 0) {
      if (last_type & CS_VOID || last_type & CS_INT || last_type & CS_CHAR || last_type & CS_LONG ||
          last_type & CS_BOOL || last_type & CS_SIZE_T || last_type & CS_SSIZE_T || last_type & CS_SHORT) {
        unsigned int j;
        for (j = 0; (words[i])[j]; j++) {
          if ((words[i])[j] == '(') {
            (words[i])[j] = '\0';
            break;
          }
        }
        if (!syntax_func(words[i])) {
          new_syntax_func(words[i]);
          // sub_thread_compile_add_rgx(FUNC_COLOR, nullptr,
          // rgx_word(words[i]), &last_c_color);
        }
        words[i] += j + 1;
        --i;
        last_type = 0;
        continue;
      }
    }
    if (words[i + 1] == nullptr) {
      last_type = type;
      break;
    }
    if (!type) {
      continue;
    }
    else if (type & CS_STRUCT) {
      if (!is_syntax_struct(words[++i])) {
        // sub_thread_compile_add_rgx(STRUCT_COLOR, nullptr,
        // rgx_word(words[i]), &last_c_color);
      }
    }
    else if (type & CS_CLASS) {
      if (!is_syntax_class(words[++i])) {
        // sub_thread_compile_add_rgx(STRUCT_COLOR, nullptr,
        // rgx_word(words[i]), &last_c_color);
      }
    }
    else if (type & CS_ENUM) {}
    else if (type & CS_LONG || type & CS_VOID || type & CS_INT || type & CS_CHAR || type & CS_BOOL ||
             type & CS_SIZE_T || type & CS_SSIZE_T || type & CS_SHORT) {
      if (check_func_syntax(&words, &i)) {
        continue;
      }
      /* if (add_syntax(&type, words[i]) == NEXT_WORD_ALSO)
      {
          while (true)
          {
              if (words[++i] == nullptr)
              {
                  break;
              }
              if (add_syntax(&type, words[i]) != NEXT_WORD_ALSO)
              {
                  break;
              }
          }
          continue;
      } */
    }
    else if (type & CS_INCLUDE) {
      // handle_include(words[++i]);
    }
    else if (type & CS_DEFINE) {
      handle_define(words[++i]);
    }
  }
  prosses_callback_queue();
  free(words);
}

/* Add some "basic" cpp syntax. */
void do_syntax(void) {
  // flag_all_brackets();
  // flag_all_block_comments(openfile->filetop);
}

/* Return`s 'true' if 'str' is in the 'syntax_structs' vector. */
bool is_syntax_struct(std::string_view str) {
  for (int i = 0; i < structs.get_size(); i++) {
    if (strcmp(&str[0], structs[i]) == 0) {
      return true;
    }
  }
  return false;
}

bool is_syntax_class(std::string_view str) {
  for (int i = 0; i < classes.get_size(); i++) {
    if (strcmp(&str[0], classes[i]) == 0) {
      return true;
    }
  }
  return false;
}

bool define_exists(const char *str) {
  for (int i = 0; i < defines.get_size(); i++) {
    if (strcmp(str, defines[i]) == 0) {
      return true;
    }
  }
  return false;
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
