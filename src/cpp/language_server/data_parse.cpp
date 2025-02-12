#include "../../include/prototypes.h"

namespace Parse {
  inline namespace utils {
    static string parse_multiline_bracket_var(linestruct *from, const char **data, const char **end) {
      string value;
      adv_ptr_to_ch((*end), '}');
      if (*(*end) == '\0') {
        value           = string((*data), ((*end) - (*data)));
        const char *br  = (*data);
        int         lvl = 0;
        do {
          br = strchr(br, '{');
          if (br) {
            lvl += 1;
            br += 1;
          }
        } while (br);
        const char *stop = nullptr;
        for (linestruct *end = from->next; end != NULL; end = end->next) {
          br = end->data;
          do {
            br = strchr(br, '{');
            if (br) {
              lvl += 1;
              br += 1;
            }
          } while (br);
          br = end->data;
          do {
            br = strchr(br, '}');
            if (br) {
              if (lvl == 0) {
                break;
              }
              lvl -= 1;
              br += 1;
            }
          } while (br);
          stop = strchr(end->data, ';');
          if (stop) {
            value += string(&end->data[indent_char_len(end)], (stop - &end->data[indent_char_len(end)]));
            break;
          }
          value += string(&end->data[indent_char_len(end)]);
        }
      }
      return value;
    }

    string parse_var_type(linestruct *from, const char **data_ptr, const char **end_ptr) {
      const char *data = *data_ptr;
      const char *end  = *end_ptr;
      ADV_PTR(end, (*end != '=' && *end != '{' && *end != ';' && *end != ',' && *end != '"' && *end != '<'));
      if (*end == '<') {
        ADV_PTR(end, (*end != '>'));
        if (!*end) {
          return "";
        }
        ADV_PTR(end, (*end != '=' && *end != '{' && *end != ';' && *end != ',' && *end != '"' && *end != '<'));
      }
      if (*end == '"') {
        return "";
      }
      if (*end == '=' || *end == '{') {
        end -= 1;
        DCR_TO_PREV_CH(end, data);
        if (end == data) {
          return "";
        }
      }
      DCR_PAST_PREV_WORD(end, data);
      if (end == data) {
        return "";
      }
      DCR_TO_PREV_CH(end, data);
      if (end == data) {
        return "";
      }
      end += 1;
      string type(data, (end - data));
      data      = end;
      *data_ptr = data;
      *end_ptr  = end;
      return type;
    }

    void parse_typedef_name_alias(const string &expr, const char **ptr, char **name, char **alias) {
      const char *start = *ptr;
      const char *end   = nullptr;
      ADV_PTR(start, (*start == '\t' || *start == ' ') && *start != '{');
      if (*start != '{') {
        end = start;
        ADV_PTR(end, *end != '\t' && *end != ' ' && *end != '{');
        *name = measured_copy(start, (end - start));
      }
      else {
        *name = copy_of("");
      }
      end   = &expr[expr.length() - 2];
      start = end;
      DCR_PTR(start, &expr[0], *start != '}');
      if (*start == '}') {
        ++start;
        ADV_TO_NEXT_WORD(start);
      }
      if (start != end) {
        *alias = measured_copy(start, (end - start));
      }
      else {
        *alias = copy_of("error");
      }
    }

    linestruct *lines_from_EOL_str(const char *str) {
      const char *start = &str[0];
      const char *end   = &str[0];
      linestruct *head  = nullptr;
      linestruct *tail  = nullptr;
      while (*end) {
        start = end;
        ADV_PTR(end, *end != '\n');
        linestruct *line = make_new_node(tail);
        line->data       = measured_copy(start, (end - start));
        if (tail == nullptr) {
          head = line;
          tail = line;
        }
        else {
          tail->next = line;
          tail       = tail->next;
        }
        if (*end) {
          ++end;
        }
      }
      return head;
    }

    string process_line(linestruct *line) {
      string      ret   = "";
      const char *start = line->data;
      const char *end   = line->data;
      Ulong       len   = strlen(line->data);
      while (end < (line->data + len)) {
        ADV_TO_NEXT_WORD(end);
        if (!*end) {
          break;
        }
        start = end;
        ADV_PAST_WORD(end);
        if (start == end) {
          break;
        }
        ret += string(start, (end - start)) + " ";
      }
      return ret;
    }

    /* Return a path without any '..'. */
    string normalise_path(const char *path) {
      const char *start = nullptr;
      string      save  = path;
      while ((start = strstr(save.c_str(), "/.."))) {
        if (start == save.c_str()) {
          break;
        }
        const char *end = start + 3;
        if (!*end) {
          break;
        }
        --start;
        DCR_PTR(start, save.c_str(), (*start != '/'));
        string ret(save.c_str(), (start - &save[0]));
        ret += end;
        save = ret;
      }
      return save;
    }

    void construct_ClassData(const string &data, const string &visual_data) {
      ClassData cd;
      cd.raw_data = data;
      // unix_socket_debug("%s\n", cd.raw_data.c_str());
      const char *start = strstr(cd.raw_data.c_str(), "class");
      if (!start) {
        return;
      }
      start += 5;
      ADV_TO_NEXT_WORD(start);
      const char *end = start;
      ADV_PTR(end, (*end != '\t' && *end != ' ' && *end != '{'));
      if (start == end) {
        return;
      }
      cd.name = measured_copy(start, (end - start));
      // unix_socket_debug("%s\n", cd.name);
      LSP->index.classes.push_back(cd);
    }
  }

  void comment(linestruct *line) {
    if (!line) {
      return;
    }
    const char *start = strstr(line->data, "/*");
    const char *end   = strstr(line->data, "*/");
    const char *slash = strstr(line->data, "//");
    /* First line for a block comment. */
    if (start && !end) {
      /* If a slash comment is found and it is before the block start, we adjust the start
       * and end pos.  We also make sure to unset 'BLOCK_COMMENT_START' for the line. */
      if (slash && slash < start) {
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
          ((line->prev->flags.is_set<IN_BLOCK_COMMENT>()) || (line->prev->flags.is_set<BLOCK_COMMENT_START>())) &&
          !(line->prev->flags.is_set<SINGLE_LINE_BLOCK_COMMENT>())) {
        line->flags.set<IN_BLOCK_COMMENT>();
        line->flags.unset<BLOCK_COMMENT_START>();
        line->flags.unset<BLOCK_COMMENT_END>();
        line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
      }
      /* If the prev line is not in a block comment or the
       * start block line we are not inside a comment block. */
      else {
        line->flags.unset<IN_BLOCK_COMMENT>();
        line->flags.unset<BLOCK_COMMENT_START>();
        line->flags.unset<BLOCK_COMMENT_END>();
        line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
      }
    }
    /* End of a block comment. */
    else if (!start && end) {
      /* If last line is in a comment block or is the start of the block. */
      if (line->prev &&
          ((line->prev->flags.is_set<IN_BLOCK_COMMENT>()) || (line->prev->flags.is_set<BLOCK_COMMENT_START>())) &&
          !(line->prev->flags.is_set<SINGLE_LINE_BLOCK_COMMENT>())) {
        line->flags.set<BLOCK_COMMENT_END>();
        line->flags.unset<IN_BLOCK_COMMENT>();
        line->flags.unset<BLOCK_COMMENT_START>();
        line->flags.unset<SINGLE_LINE_BLOCK_COMMENT>();
      }
    }
  }

  void struct_type(linestruct **from_line, const char *current_file) {
    if (!from_line) {
      return;
    }
    linestruct *from = *from_line;
    PROFILE_FUNCTION;
    const char *data = &from->data[indent_char_len(from)];
    if (invalid_variable_sig(data)) {
      return;
    }
    if (strstr(data, "operator")) {
      return;
    }
    const char *end = data;
    string      t   = parse_var_type(from, &data, &end);
    if (t.empty()) {
      return;
    }
    if (strstr(t.c_str(), "/*") == t.c_str() || strncmp(t.c_str(), "//", 2) == 0) {
      return;
    }
    const char *type = t.c_str();
    Uint        idx;
    const char *found =
      strstr_array(type, (const char *[]) {"typedef", "enum", "struct", "class"}, 4, &idx);
    if (found && (found == type || (type[(found - type) - 1] == ' ' || type[(found - type) - 1] == '\t'))) {
      /* typedef */
      if (idx == 0) {
        const char *typedef_type = nullptr;
        linestruct *br_line      = nullptr;
        FOR_EACH_LINE_NEXT(line, from) {
          if ((typedef_type = strchr(line->data, '{'))) {
            br_line = line;
            break;
          }
          if ((typedef_type = strchr(line->data, ';'))) {
            break;
          }
        }
        if (typedef_type) {
          /* Single line typedef; */
          if (*typedef_type == ';') {
            found = strstr(from->data, "typedef");
            if (found) {
              found += 7;
              ADV_TO_NEXT_WORD(found);
              if (!*found) {
                return;
              }
              const char *st = found;
              ADV_PAST_WORD(found);
              if (found != st) {
                char *word = measured_copy(st, (found - st));
                /* Single line 'struct' typedef. */
                if (strcmp(word, "struct") == 0) {
                  const char *end = st;
                  ADV_PTR(end, *end != ';');
                  if (*end == ';') {
                    const char *val_st = end;
                    DCR_PTR(val_st, st, (*val_st != '\t' && *val_st != ' ' && *val_st != '*' && *val_st != '&'));
                    if (val_st == st || val_st == end) {
                      return;
                    }
                    ++val_st;
                    char       *name  = measured_copy(st, (val_st - st));
                    char       *alias = measured_copy(val_st, (end - val_st));
                    const auto &it    = LSP->index.tdstructs.find(alias);
                    /* If not inside map, we add it. */
                    if (it == LSP->index.tdstructs.end()) {
                      TypedefStruct tds;
                      tds.name                        = name;
                      tds.alias                       = alias;
                      LSP->index.tdstructs[tds.alias] = tds;
                    }
                    /* Otherwise free the alloced data. */
                    else {
                      free(name);
                      free(alias);
                    }
                  }
                }
                free(word);
              }
            }
            return;
          }
          /* Typedef with body. */
          else {
            found = strstr(from->data, "typedef");
            if (found) {
              found += 7;
              ADV_TO_NEXT_WORD(found);
              if (!*found) {
                return;
              }
              const char *st = found;
              ADV_PTR(found, *found != '\t' && *found != ' ' && *found != '{');
              if (found != st) {
                char *word = measured_copy(st, (found - st));
                /* Found 'struct' typedef. */
                if (strcmp(word, "struct") == 0) {
                  TypedefStruct tsc;
                  ADV_TO_NEXT_WORD(found);
                  /* Anonomus struct. */
                  if (!*found || *found == '{') {
                    tsc.name = copy_of("");
                  }
                  else {
                    st = found;
                    ADV_PTR(found, *found != '\t' && *found != ' ' && *found != '{');
                    tsc.name = measured_copy(st, (found - st));
                  }
                  if (br_line) {
                    linestruct *end_line;
                    Ulong       end_idx;
                    if (find_end_bracket(br_line, (typedef_type - br_line->data), &end_line, &end_idx)) {
                      FOR_EACH_LINE_NEXT(line, end_line) {
                        if (strchr(line->data, ';')) {
                          end_line = line;
                          break;
                        }
                      }
                      const char *alias_st  = &end_line->data[indent_char_len(end_line)];
                      const char *alias_end = nullptr;
                      if (*alias_st == '}') {
                        ++alias_st;
                      }
                      ADV_TO_NEXT_WORD(alias_st);
                      /** TODO: Fix this. */
                      if (!*alias_st || *alias_st == '*') {
                        free(tsc.name);
                        free(word);
                        return;
                      }
                      alias_end = alias_st;
                      ADV_PTR(alias_end, *alias_end != '\t' && *alias_end != ' ' && *alias_end != ';');
                      tsc.alias                       = measured_copy(alias_st, (alias_end - alias_st));
                      LSP->index.tdstructs[tsc.alias] = tsc;
                      *from_line                      = end_line;
                    }
                  }
                }
                free(word);
              }
            }
            return;
          }
        }
      }
      /* enum */
      else if (idx == 1) {
        found = strstr(from->data, "enum");
        if (found) {
          found += 4;
          ADV_TO_NEXT_WORD(found);
          if (strncmp(found, "class", 5) == 0) {
            found += 5;
            ADV_TO_NEXT_WORD(found);
            // unix_socket_debug("enum class: %s\n", found);
          }
          else {
            unix_socket_debug("enum: %s\n", found);
          }
        }
      }
      /* struct */
      else if (idx == 2) {
        if (strchr(from->data, ')') || strchr(from->data, ',') || strchr(from->data, '[') || strchr(from->data, '<') ||
            strchr(from->data, '*')) {
          return;
        }
        /* Single line decl. */
        if (strchr(from->data, ';')) {
          // unix_socket_debug("decl: %s\n", from->data);
          return;
        }
        /* Defenition. */
        else {
          const char *br      = nullptr;
          linestruct *br_line = nullptr;
          Uint        i = 0, max_lines = 10;
          FOR_EACH_LINE_NEXT(line, from) {
            if ((br = strchr(line->data, '{'))) {
              br_line = line;
              break;
            }
            if (++i == max_lines) {
              break;
            }
          }
          if (br_line) {
            Ulong       end_idx;
            linestruct *end_line;
            if (find_end_bracket(br_line, (br - br_line->data), &end_line, &end_idx)) {
              // unix_socket_debug("Data: %s\n", from->data);
              *from_line      = end_line;
              const char *end = from->data;
              const char *st  = end;
              ADV_PTR(end, *end != '{' && *end != '/');
              if (st == end) {
                return;
              }
              --end;
              DCR_TO_PREV_CH(end, st);
              if (end == st) {
                return;
              }
              st = end;
              ++end;
              DCR_PAST_PREV_WORD(st, from->data);
              if (st == from->data) {
                return;
              }
              ++st;
              StructEntry ste;
              ste.filename                 = copy_of(current_file);
              ste.name                     = measured_copy(st, (end - st));
              ste.decl_st                  = from->lineno;
              ste.decl_end                 = end_line->lineno;
              LSP->index.structs[ste.name] = ste;
              return;
            }
          }
        }
      }
      /* class */
      else if (idx == 3) {
        // unix_socket_debug("%s\n", from->data);
        const char *is_decl = nullptr;
        linestruct *st_line = nullptr;
        FOR_EACH_LINE_NEXT(line, from) {
          if ((is_decl = strchr(line->data, '{'))) {
            st_line = line;
            break;
          }
          if ((is_decl = strchr(line->data, ';'))) {
            break;
          }
        }
        if (is_decl) {
          /* Decl. */
          if (*is_decl == ';') {
            if (strchr(from->data, ';')) {
              // unix_socket_debug("class decl: %s\n", from->data);
              return;
            }
          }
          /* Defenition. */
          else if (st_line) {
            linestruct *end_line;
            Ulong       end_idx;
            if (find_end_bracket(st_line, (is_decl - st_line->data), &end_line, &end_idx)) {
              *from_line = end_line;
              const char *st = strstr(from->data, "class");
              if (st) {
                st += 5;
                ADV_TO_NEXT_WORD(st);
                const char *end = st;
                ADV_PTR(end, *end != '{');
                if (end != st) {
                  if (*end != '{') {
                    if (from->data[(end - from->data) - 1] == ',') {
                      return;
                    }
                  }
                  --end;
                  unix_socket_debug("class defenition: %s\n", st);
                }
              }
              return;
            }
          }
        }
      }
      string      expr        = "";
      string      visual_data = "";
      int         lvl         = 0;
      const char *b_start     = nullptr;
      const char *b_end       = nullptr;
      FOR_EACH_LINE_NEXT(line, from) {
        if (!visual_data.empty()) {
          visual_data += '\n';
        }
        visual_data += line->data;
        expr += process_line(line);
        b_start = line->data;
        do {
          b_start = strchr(b_start, '{');
          if (b_start) {
            if (!(line->data[(b_start - line->data) - 1] == '\'' && line->data[(b_start - line->data) + 1] == '\'')) {
              lvl += 1;
            }
            b_start += 1;
          }
        } while (b_start);
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
        } while (b_end);
        if (strchr(line->data, ';') && lvl == 0) {
          break;
        }
      }
      const char *brace  = strchr(expr.c_str(), '{');
      const char *paranR = strchr(expr.c_str(), ')');
      if (((brace && paranR) && paranR < brace) || (paranR && !brace)) {
        return;
      }
      switch (idx) {
        case 0 : {
          LSP->index.rawtypedef.push_back(expr);
          break;
        }
        case 1 : { /* enum */
          // unix_socket_debug("%s\n", expr.c_str());
          LSP->index.rawenum.push_back(expr);
          break;
        }
        case 2 : {
          LSP->index.rawstruct.push_back(expr);
          break;
        }
        case 3 : { /* 'class' */
          construct_ClassData(expr, visual_data);
          break;
        }
      }
    }
  }

  void enum_type(linestruct **from) {
    if (!from) {
      return;
    }
    const char *data = &(*from)->data[indent_char_len(*from)];
    const char *end  = data;
    string      type = parse_var_type(*from, &data, &end);
    if (type == "enum") {
      EnumEntry ee;
      ee.name = nullptr;
      ADV_TO_NEXT_WORD(end);
      if (*end) {
        const char *start = end;
        ADV_PTR(end, *end != ' ' && *end != '\t' && *end != '{');
        if (end != start) {
          char *name = measured_copy(start, (end - start));
          unix_socket_debug("name: %s\n", name);
          ee.name = name;
        }
      }
      const char *found      = nullptr;
      linestruct *found_line = nullptr;
      FOR_EACH_LINE_NEXT(line, *from) {
        if ((found = strchr(line->data, '{'))) {
          found_line = line;
          break;
        }
        if ((found = strchr(line->data, ';'))) {
          break;
        }
      }
      if (!found_line) {
        return;
      }
      linestruct *end_bracket;
      Ulong       end_index;
      if (find_matching_bracket(found_line, (found - found_line->data), &end_bracket, &end_index)) {
        *from = end_bracket;
      }
      char *body = fetch_bracket_body(found_line, (found - found_line->data));
      if (body) {
        // unix_socket_debug("  body: %s\n", body);
        free(body);
      }
      if (ee.name) {
        LSP->index.enums[ee.name] = ee;
      }
    }
  }

  void variable(linestruct *line, const char *current_file, vector<var_t> &v_vec) {
    PROFILE_FUNCTION;
    const char *data = &line->data[indent_char_len(line)];
    if (invalid_variable_sig(data)) {
      return;
    }
    if (strstr(data, "operator") /* || word_strstr(line->data, "class") */) {
      return;
    }
    const char *end  = data;
    string      type = parse_var_type(line, &data, &end);
    if (type.empty()) {
      return;
    }
    if (strstr(type.c_str(), "/*") == type.c_str() || strncmp(type.c_str(), "//", 2) == 0) {
      return;
    }
    do {
      string type_addon = "";
      string name       = "";
      string value      = "";
      ADV_TO_NEXT_CH(data);
      ADV_PTR(end, (*end != ';' && *end != ',' && *end != '=' && *end != '{'));
      const char *save = end;
      end -= 1;
      DCR_TO_PREV_CH(end, data);
      end += 1;
      type_addon = "";
      if (*data == '*' || *data == '&') {
        const char *addons = data;
        adv_ptr(data, (*data == '*' || *data == '&'));
        if (*data == '\0') {
          return;
        }
        type_addon = string(addons, (data - addons));
      }
      name          = string(data, (end - data));
      int decl_line = (int)line->lineno;
      int scope_end = current_line_scope_end(line);
      data          = save + 1;
      if (*save == '=') {
        ADV_TO_NEXT_CH(data);
        end = data;
        adv_ptr(end, (*end != ';' && *end != ',' && *end != '{'));
        if (*end == '\0') {
          return;
        }
        if (*end == '{') {
          data  = end;
          value = parse_multiline_bracket_var(line, &data, &end);
          end += 1;
        }
        if (value == "") {
          value = string(data, (end - data));
        }
        data = end + 1;
      }
      else if (*save == '{') {
        end  = save;
        data = end;
        ADV_PTR_TO_CH(end, '}');
        if (*end == '\0') {
          value = parse_multiline_bracket_var(line, &data, &end);
          if (value == "") {
            return;
          }
        }
        else {
          end += 1;
          value = string(save, (end - save));
        }
        data = end + 1;
      }
      v_vec.push_back({type + type_addon, name, value, decl_line, scope_end, current_file});
      end = data;
    } while (*data);
  }

  /* Parse a function signature. */
  void function(linestruct **from, const char *current_file) {
    if (!from) {
      return;
    }
    const char *found = strchr((*from)->data, '(');
    if (found) {
      const char *com_st    = strstr((*from)->data, "/*");
      const char *com_end   = strstr((*from)->data, "*/");
      const char *com_slash = strstr((*from)->data, "//");
      /* If inside of a block comment. */
      if (com_st && com_end && (found > com_st && found < com_end)) {
        return;
      }
      /* If behind a slash comment. */
      if (com_slash && (found > com_slash)) {
        return;
      }
      const char       *param_st      = found;
      const linestruct *param_st_line = *from;
      Uint              idx;
      const char       *not_good = strstr_array((*from)->data, (const char *[]) {"__attribute__"}, 1, &idx);
      if (not_good) {
        return;
      }
      char       *param_str  = fetch_bracket_body((*from), (found - (*from)->data));
      linestruct *found_line = nullptr;
      FOR_EACH_LINE_NEXT(line, *from) {
        if ((found = strchr(line->data, '{'))) {
          found_line = line;
          break;
        }
        if ((found = strchr(line->data, ';'))) {
          break;
        }
      }
      if (found_line) {
        if (*found == '{') {
          linestruct *end_bracket;
          Ulong       end_index;
          if (find_end_bracket(found_line, (found - found_line->data), &end_bracket, &end_index)) {
            *from = end_bracket;
          }
          else {
            if (param_str) {
              free(param_str);
            }
            return;
          }
          char       *body = fetch_bracket_body(found_line, (found - found_line->data));
          const char *end  = param_st - 1;
          DCR_TO_PREV_CH(end, param_st_line->data);
          if (end == param_st_line->data) {
            return;
          }
          ++end;
          const char *st = end;
          for (; st > param_st_line->data && (*st != ' ' && *st != '\t' && *st != '*' && *st != '&'); st--);
          if (st == end) {
            return;
          }
          if (st != param_st_line->data) {
            ++st;
          }
          char *name = measured_copy(st, (end - st));
          if (strstr(name, "operator") || strstr(name, "::") || strchr(name, '>') || strchr(name, '<') ||
              strchr(name, '=') || strchr(name, '!')) {
            free(name);
            return;
          }
          /* For now we only take current file func def`s. */
          if (strcmp(tail(current_file), tail(openfile->filename)) == 0) {
            // unix_socket_debug("Name: %s\n", name);
            FunctionDef fd;
            fd.file     = copy_of(current_file);
            fd.name     = name;
            fd.decl_st  = param_st_line->lineno;
            fd.decl_end = end_bracket->lineno;
            if (param_str) {
              if (*param_str && strcmp(param_str, "void") != 0) {
                const char *p_st  = param_str;
                const char *p_end = p_st;
                do {
                  ADV_TO_NEXT_WORD(p_end);
                  p_st = p_end;
                  ADV_PTR(p_end, *p_end != ',');
                  if (p_st == p_end) {
                    break;
                  }
                  char       *var   = measured_copy(p_st, (p_end - p_st));
                  const char *v_st  = var;
                  const char *v_end = v_st;
                  for (; *v_end; ++v_end);
                  DCR_PTR(v_end, v_st, *v_end != '\t' && *v_end != ' ' && *v_end != '*' && *v_end != '&');
                  if (v_st != v_end) {
                    ++v_end;
                    DCR_TO_PREV_CH(v_end, var);
                    if (v_end != var) {
                      if (*v_end == '\t' || *v_end == ' ') {
                        --v_end;
                      }
                      VarDecl vd;
                      vd.file = copy_of(current_file);
                      vd.type = measured_copy(var, (v_end - var));
                      vd.name = copy_of(v_end);
                      /* For now set the value to a empty str. */
                      vd.value    = copy_of("");
                      vd.decl_st  = fd.decl_st;
                      vd.decl_end = fd.decl_end;
                      LSP->index.vars[vd.name].push_back(vd);
                    }
                  }
                  free(var);
                  if (*p_end) {
                    ++p_end;
                  }
                } while (*p_end);
                // unix_socket_debug("Params: %s\n", param_str);
              }
              free(param_str);
            }
            if (body) {
              // unix_socket_debug("  body: %s\n", body);
              free(body);
            }
            LSP->index.functiondefs[fd.name] = fd;
            return;
          }
          if (param_str) {
            free(param_str);
          }
          if (body) {
            free(body);
          }
          free(name);
        }
        else if (*found == ';') {
          if (param_str) {
            free(param_str);
          }
        }
      }
    }
  }

  /* For now this just skips these lines to make sure the rest of the parsing works better. */
  void do_template(linestruct **from, const char *current_file) {
    if (!*from) {
      return;
    }
    linestruct *was = *from;
    const char *pos = strstr(was->data, "template");
    const char *slash_com = strstr(was->data, "//");
    if (pos && (!slash_com || slash_com > pos)) {
      const char *st = strchr(was->data, '<');
      if (!st) {
        return;
      }
      Ulong end_idx;
      linestruct *end_line;
      if (find_end_bracket(was, (st - was->data), &end_line, &end_idx)) {
        if (end_line->next) {
          end_line = end_line->next;
        }
        *from = end_line;
      }
    }
  }
}

/* Main parse function. */
void do_parse(linestruct **line, const char *current_file) {
  Parse::do_template(line, current_file);
  Parse::struct_type(line, current_file);
  /* vector<var_t> vars;
  Parse::variable(*line, current_file, vars);
  if (!vars.empty()) {
    for (const auto &[type, name, value, decl_line, scope_end, file] : vars) {
        unix_socket_debug("type: %s\n"
              "         name: %s\n"
              "        value: %s\n"
              "    decl_line: %d\n"
              "    scope_end: %d\n"
              "         file: %s\n\n",
              type.c_str(), name.c_str(), value.c_str(), decl_line, scope_end, file.c_str());
    }
    for (const auto &it : vars) {
      LSP->index.variabels.push_back(it);
    }
  } */
  Parse::function(line, current_file);
  Parse::enum_type(line);
}

void do_bash_parse(linestruct *line, const char *current_file) {
  const char *ptr   = NULL;
  const char *start = NULL;
  const char *end   = NULL;
  /* Look for variables. */
  if ((ptr = strchr(line->data, '='))) {
    start = ptr;
    end   = ptr;
    DCR_PAST_PREV_WORD(start, line->data);
    (*start == ' ' || *start == '\t') ? ++start : 0;
    /* Invalid variable decl.  There is a space behind the '=' char. */
    if (start == end) {
      LSP->index.bash_data.error.push_back({
        copy_of("Decl has no name"),
        (int)line->lineno,
        (int)(ptr - line->data),
        (int)((ptr - line->data) + 1)
      });
      return;
    }
    char *var_name = measured_copy(start, (end - start));
    start = (ptr + 1);
    end   = (ptr + 1);
    if (!*end) {
      free(var_name);
      return;
    }
    ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '('));
    /* Invalid variable decl. */
    if (end == start) {
      free(var_name);
      return;
    }
    else if (*end == '(') {
      linestruct *endl;
      Ulong endx;
      if (!find_end_bracket(line, (end - line->data), &endl, &endx) || line != endl) {
        free(var_name);
        return;
      }
      end = &endl->data[endx];
      ADV_PAST_WORD(end);
    }
    char *var_value = measured_copy(start, (end - start));
    // NLOG("%s %s\n", var_name, var_value);
    LSP->index.bash_data.variable[var_name] = {var_name, var_value, (int)line->lineno};
  }
}
