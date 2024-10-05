#include "../../include/prototypes.h"

inline namespace utils {
  const char *get_preprosses_type(linestruct *line, char **word) {
    const char *found = strchr(line->data, '#');
    if (found) {
      const char *start = found + 1;
      ADV_TO_NEXT_WORD(start);
      if (!*start) {
        return nullptr;
      }
      const char *end = start;
      ADV_PAST_WORD(end);
      if (start == end) {
        return nullptr;
      }
      *word = measured_copy(start, (end - start));
      return end;
    }
    return nullptr;
  }

  char *get_next_word(const char **ptr) {
    const char *start = *ptr;
    const char *end   = *ptr;
    ADV_TO_NEXT_WORD(start);
    if (!*start) {
      return nullptr;
    }
    end = start;
    ADV_PAST_WORD(end);
    *ptr = end;
    return measured_copy(start, (end - start));
  }

  static int define_is_equl_or_greater(const string &statement) {
    NLOG("statement:('%s')\n", statement.c_str());
    const char *start = &statement[0];
    const char *end   = start;
    ADV_TO_NEXT_WORD(start);
    end = start;
    ADV_PAST_WORD(end);
    if (end == start) {
      return -1;
    }
    string def(start, (end - start));
    string def_value = LSP->define_value(def);
    if (def_value.empty()) {
      return -1;
    }
    start = strstr(end, ">=");
    if (!start) {
      return -1;
    }
    start += 2;
    ADV_TO_NEXT_WORD(start);
    end = start;
    ADV_PAST_WORD(end);
    if (end == start) {
      return -1;
    }
    string compare(start, (end - start));
    if (atoi(def_value.c_str()) >= atoi(compare.c_str())) {
      return TRUE;
    }
    return FALSE;
  }

  char *get_file_dir(const char *file) {
    const char *tail = strrchr(file, '/');
    if (tail) {
      return measured_copy(file, (tail - file) + 1);
    }
    else {
      return copy_of("");
    }
  }

  int get_include_path(const char *current_file, const char **ptr, char **path, bool *local) {
    const char *start = *ptr;
    const char *end   = nullptr;
    ADV_PTR(start, (*start != '"' && *start != '<'));
    if (*start) {
      if (*start == '<') {
        ++start;
        end = start;
        ADV_PTR(end, (*end != '>'));
        if (!*end || start == end) {
          return -2;
        }
        *path  = measured_copy(start, (end - start));
        *local = false;
        return 0;
      }
      else if (*start == '"') {
        ++start;
        end = start;
        ADV_PTR(end, (*end != '"'));
        if (!*end) {
          return -2;
        }
        char *p      = measured_copy(start, (end - start));
        char *op_dir = get_file_dir(current_file);
        *path        = alloc_str_free_substrs(op_dir, p);
        *local       = true;
        return 0;
      }
    }
    return -1;
  }

  string parse_full_define(linestruct *from, const char **ptr, int *endline) {
    string      decl  = "";
    const char *start = *ptr;
    const char *end   = start;
    FOR_EACH_LINE_NEXT(line, from) {
      line->flags.set<PP_LINE>();
      /* Advance pointer 'end' to 'EOL' or a backslash ('\'), indicating a continued line. */
      do {
        ADV_PTR(end, (*end != '\\' && *end != '\'' && *end != '"'));
        /* If we find any ('/") chars.  We must skip them fully. */
        if (*end == '"' || *end == '\'') {
          ++end;
          ADV_PTR(end, (*end != '\'' && *end != '"'));
          if (!*end) {
            break;
          }
          ++end;
        }
      } while (*end && *end != '\\');
      /* 'EOL' was found, indecating end of define decl. */
      if (!*end) {
        decl += string(start, (end - start));
        endline ? (*endline = line->lineno) : 0;
        break;
      }
      /* A backslash ('\') was found, indicating a multi-line define. */
      else {
        /* Trim trailing whitespace between the backslash ('\') and the last non-whitespace character. */
        if (line->data[(end - start) - 1] == '\t' || line->data[(end - start) - 1] == ' ') {
          --end;
          DCR_PTR(end, &line->data[0], (*end == '\t' || *end == ' '));
          ++end;
        }
        decl += string(start, (end - start));
        decl += "\n";
        /* Set 'start' and 'end' to the beginning of the next line's data. */
        if (line->next) {
          start = &line->next->data[0];
          end   = start;
        }
      }
    }
    return decl;
  }
}

void do_include(linestruct *line, const char *current_file, const char **ptr) {
  char *path;
  bool  local;
  int   result = get_include_path(current_file, ptr, &path, &local);
  if (result < 0 || !path) {
    logE("current file: '%s', line: %s, get_include_path: Error: %d", current_file, line->data, result);
    return;
  }
  if (local) {
    LSP->index_file(path);
    free(path);
    return;
  }
  else {
    string check_file = string(path);
    free(path);
    if (check_file.empty()) {
      return;
    }
    vector<string> dirs = {
      "/usr/local/include/" + check_file,
      "/usr/include/" + check_file,
      "/usr/lib/clang/18/include/" + check_file,
      "/usr/include/c++/v1/" + check_file,
    };
    for (const auto &it : dirs) {
      if (LSP->has_been_included(it.c_str())) {
        break;
      }
      LSP->index_file(it.c_str());
      if (is_file_and_exists(it.c_str())) {
        break;
      }
    }
    return;
  }
}

void do_if(linestruct *line, const char **ptr) {
  string full_delc = LSP->parse_full_pp_delc(line, ptr);
  // NLOG("%s\n", full_delc.c_str());
  // split_if_statement(full_delc);
  const char *start = &full_delc[0];
  const char *end   = &full_delc[0];
  ADV_TO_NEXT_WORD(start);
  if (!*start) {
    return;
  }
  const char        *found     = NULL;
  string             def       = "";
  string             found_str = "";
  unsigned int       index;
  const char        *next_rule         = NULL;
  const unsigned int rule_count        = 5;
  const char        *rules[rule_count] = {"&&", "||", ">=", "defined", "?"};
  bool               should_be_defined = FALSE;
  bool               was_correct       = FALSE;
  do {
    should_be_defined = FALSE;
    was_correct       = FALSE;
    /* If no more rules are found we break. */
    found = strstr_array(start, rules, rule_count, &index);
    if (!found) {
      break;
    }
    switch (index) {
      case 0 : /* && */
      {
      }
      case 1 : /* || */
      {
      }
      case 2 : /* >= */
      {
        found += 2;
        start = found;
        break;
      }
      case 3 :                                                            /* defined */
      {
        should_be_defined =
          (found != start && start[(found - start) - 1] == '!') ? FALSE : /* This def should not be defined. */
            TRUE;                                                         /* This def should be defined. */
        start = found + 7;
        ADV_TO_NEXT_WORD(start);
        if (!*start) {
          return;
        }
        end = start;
        ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != ')'));
        def = string(start, (end - start));
        found += 7;
        start     = end;
        next_rule = strstr_array(start, rules, rule_count, &index);
        if (!next_rule) {
          return;
        }
        if (index == 4) {
          NLOG("full_decl: %s\n", full_delc.c_str());
          start = next_rule + 1;
          end   = start;
          ADV_PTR(end, (*end != ':'));
          if (!*end) {
            return;
          }
          if (LSP->is_defined(def) == -1) {
            end += 1;
            start = end;
            for (; *end; end++);
            if (start == end) {
              return;
            }
          }
          ADV_TO_NEXT_WORD(start);
          next_rule = strstr_array(start, rules, rule_count, &index);
          if (index == 3) {
            should_be_defined = (next_rule != start && start[(next_rule - start) - 1] == '!')
                                ? FALSE
                                :       /* This def should not be defined. */
                                  TRUE; /* This def should be defined. */
            NLOG("%s\n", BOOL_STR(should_be_defined));
            start += 7;
            ADV_TO_NEXT_WORD(start);
            end = start;
            ADV_PAST_WORD(end);
            if (end == start) {
              return;
            }
            def = string(start, (end - start));
            /* This def should be defined. */
            if (should_be_defined) {
              if (LSP->is_defined(def) != -1) {
                was_correct = TRUE;
              }
            }
            /* Should not be defined. */
            else {
              if (LSP->is_defined(def) == -1) {
                was_correct = TRUE;
              }
            }
            NLOG("was_correct: %s\n", BOOL_STR(was_correct));
            if (!was_correct) {
              int endif = LSP->find_endif(line->next);
              for (linestruct *l = line->next; l && l->lineno != endif; l = l->next) {
                l->flags.set(DONT_PREPROSSES_LINE);
              }
            }
          }
          else if (index == 2) {
            if (!define_is_equl_or_greater(string(start, (end - start)))) {
              int endif = LSP->find_endif(line->next);
              for (linestruct *l = line->next; l && l->lineno != endif; l = l->next) {
                l->flags.set(DONT_PREPROSSES_LINE);
              }
              return;
            }
            /* ADV_TO_NEXT_WORD(start);
            end = start;
            ADV_PAST_WORD(end);
            if (start == end)
            {
                return;
            }
            def           = string(start, (end - start));
            int def_index = is_defined(def);
            if (def_index == -1)
            {
                return;
            }
            string def_value = define_value(def);
            if (def_value == "")
            {
                return;
            }
            start = next_rule + 2;
            ADV_TO_NEXT_WORD(start);
            end = start;
            ADV_PAST_WORD(end);
            if (start == end)
            {
                return;
            }
            string compare(start, (end - start));
            if (!(atoi(def_value.c_str()) >= atoi(compare.c_str())))
            {
                int endif = find_endif(line->next);
                for (linestruct *l              = line->next;
                     l && l->lineno != endif; l = l->next)
                {
                    LINE_SET(l, DONT_PREPROSSES_LINE);
                }
            } */
          }
        }
        break;
      }
      case 4 : /* ? */
      {

        break;
      }
    }
    /* This def should be defined. */
    if (should_be_defined) {
      if (LSP->is_defined(def) != -1) {
        was_correct = TRUE;
      }
    }
    /* Should not be defined. */
    else {
      if (LSP->is_defined(def) == -1) {
        was_correct = TRUE;
      }
    }
    if (!was_correct) {
      if (next_rule) {
        string nrule(next_rule, 2);
        if (nrule == "&&") {
          int endif = LSP->find_endif(line->next);
          for (linestruct *l = line->next; l && l->lineno != endif; l = l->next) {
            l->flags.set(DONT_PREPROSSES_LINE);
          }
        }
      }
    }
  } while (found);
}

inline namespace DefineTools {
  
};

void do_define(linestruct *line, const char *current_file, const char **ptr) {
  DefineEntry de;
  const char *start = *ptr;
  const char *end   = *ptr;
  ADV_TO_NEXT_WORD(start);
  if (!*start) {
    return;
  }
  end = start;
  ADV_PTR(end, (*end != '(' && *end != ' ' && *end != '\t'));
  if (start == end) {
    return;
  }
  de.name            = string(start, (end - start));
  de.decl_start_line = line->lineno;
  if (*end == '(') {
    ADV_PTR(end, (*end != ')'));
    if (!*end || end == start) {
      return;
    }
    end += 1;
  }
  de.full_decl = string(start, (end - start));
  start        = end;
  ADV_TO_NEXT_WORD(start);
  if (!*start) {
    return;
  }
  string val                  = parse_full_define(line, &start, &de.decl_end_line);
  de.value                    = string(val.c_str(), val.length());
  de.file                     = string(current_file);
  LSP->index.defines[de.name] = de;
}

void do_ifndef(const string &define, linestruct *current_line) {
  if (LSP->is_defined(define) == -1) {
    // NLOG("define: '%s' was not defined.\n", define.c_str());
    return;
  }
  int endif = LSP->find_endif(current_line->next);
  for (linestruct *line = current_line->next; line != NULL && (line->lineno != endif); line = line->next) {
    line->flags.set(DONT_PREPROSSES_LINE);
  }
}

void do_ifdef(const string &define, linestruct *current_line) {
  if (LSP->is_defined(define) != -1) {
    return;
  }
  int endif = LSP->find_endif(current_line->next);
  for (linestruct *line = current_line->next; line != nullptr && (line->lineno != endif); line = line->next) {
    line->flags.set<DONT_PREPROSSES_LINE>();
  }
}

void do_undef(const string &define) {
  /* auto it = LSP.index.defines.begin();
  while (it != LSP.index.defines.end()) {
    if (it->name == define) {
      it = LSP.index.defines.erase(it);
    }
    else {
      it++;
    }
  } */
}

/* This is the main handler of the language server for preprossesing. */
void do_preprossesor(linestruct *line, const char *current_file) {
  char       *word;
  const char *found     = get_preprosses_type(line, &word);
  const char *slash_com = strstr(line->data, "//");
  const char *com_st    = strstr(line->data, "/*");
  const char *com_end   = strstr(line->data, "*/");
  if (found) {
    if (slash_com && slash_com < found) {
      free(word);
      return;
    }
    if (com_st && com_end && found > com_st && found < com_end) {
      free(word);
      return;
    }
    else if (com_st && found > com_st) {
      free(word);
      return;
    }
    line->flags.set<PP_LINE>();
    if (strcmp(word, "include") == 0) {
      do_include(line, current_file, &found);
    }
    /* else if (strcmp(word, "ifndef") == 0) {
      free(word);
      word = get_next_word(&found);
      do_ifndef(word, line);
    }
    else if (strcmp(word, "ifdef") == 0) {
      free(word);
      word = get_next_word(&found);
      do_ifdef(word, line);
    } */
    else if (strcmp(word, "define") == 0) {
      do_define(line, current_file, &found);
    }
    /* else if (strcmp(word, "if") == 0) {
      ++found;
      do_if(line, &found);
    } */
    free(word);
  }
}
