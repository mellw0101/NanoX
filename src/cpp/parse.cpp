#include "../include/prototypes.h"

string parse_multiline_bracket_var(linestruct *from, const char **data, const char **end) {
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

/* Decr ptr until space or tab found or end == data.  This will jump over string literals. */
void move_ptr_to_prev_word(const char **data, const char **end) {
  while (true) {
    for (; *end > *data && (**end != ' ' && **end != '\t' && **end != '"' && **end != ']'); (*end)--);
    if (**end == '"') {
      (*end) -= 1;
      dcr_ptr((*end), (*data), *(*end) != '"');
      if (**end == '"') {
        (*end) -= 1;
        continue;
      }
      break;
    }
    if (**end == ']') {
      dcr_ptr((*end), (*data), *(*end) != '[');
      if (**end == '[') {
        (*end) -= 1;
        continue;
      }
    }
    break;
  }
}

void parse_var_type(const char *data) {
  if (word_strstr(data, "using") || word_strstr(data, "return") || *data == '{' || word_strstr(data, "//") ||
      word_strstr(data, "/*") || word_strstr(data, "==") || word_strstr(data, "||") || word_strstr(data, "&&") ||
      word_strstr(data, "|=") || word_strstr(data, "+=") || word_strstr(data, "-=")) {
    return;
  }
  const char *end = data;
  adv_ptr(end, (*end != ';' && *end != ',' && *end != '=' && *end != '{' && *end != '('));
  if (!*end || *end == '(') {
    return;
  }
  if (*end == ',') {
    const char *p = data;
    adv_ptr(p, (*p != '('));
    if (*p || p < end) {
      return;
    }
  }
  end -= 1;
  dcr_to_prev_ch(end, data);
  move_ptr_to_prev_word(&data, &end);
  if (end == data) {
    return;
  }
  string rest = end + 1;
  dcr_to_prev_ch(end, data);
  if (end == data) {
    return;
  }
  string type(data, (end - data) + 1);
  if (type == "if" || type == "for") {
    return;
  }
  nlog("type: %s, ", type.c_str());
  nlog("rest: %s\n", rest.data());
}

void line_variable(linestruct *line, vector<var_t> &var_vector) {
  const char *data = &line->data[indent_char_len(line)];
  if (invalid_variable_sig(data)) {
    return;
  }
  if (strstr(data, "operator") || word_strstr(line->data, "class")) {
    return;
  }
  const char *end = data;
  adv_ptr(end, (*end != '=' && *end != '{' && *end != ';' && *end != ','));
  if (*end == '=' || *end == '{') {
    end -= 1;
    dcr_to_prev_ch(end, data);
    if (end == data) {
      return;
    }
  }
  dcr_past_prev_word(end, data);
  if (end == data) {
    return;
  }
  dcr_to_prev_ch(end, data);
  if (end == data) {
    return;
  }
  end += 1;
  string type(data, (end - data));
  data = end;
  do {
    string type_addon = "";
    string name       = "";
    string value      = "";
    adv_to_next_ch(data);
    ADV_PTR(end, (*end != ';' && *end != ',' && *end != '=' && *end != '{'));
    const char *save = end;
    end -= 1;
    dcr_to_prev_ch(end, data);
    // NLOG("end: %s\n", end);
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
      adv_to_next_ch(data);
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
      adv_ptr_to_ch(end, '}');
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
    var_vector.push_back({
      type + type_addon,
      name,
      value,
      decl_line,
      scope_end,
    });
    end = data;
  } while (*data);
}

void func_decl(linestruct *line) {
  const char *data = &line->data[indent_char_len(line)];
  if (!*data || strchr(data, '=') || strchr(data, '<') || strchr(data, '>') || word_strstr(data, "if") ||
      word_strstr(data, "for") || word_strstr(data, "//")) {
    return;
  }
  const char *found = strchr(data, '(');
  if (found) {
    const char *start = found;
    DCR_PTR(start, data, (*start != ' ' && *start != '\t'));
    if (start != data && *start != '(') {
      NLOG("func_decl: %s\n", start);
    }
    else if (start == data) {
      if (!strchr(line->prev->data, ';') && !strchr(line->prev->data, '{') && !strchr(line->prev->data, '}')) {
        string full_decl = line->prev->data;
        full_decl += " " + string(start);
        NLOG("%s\n", full_decl.c_str());
      }
    }
  }
}

void parse_class_data(linestruct *from) {
  class_info_t class_info;
  const char  *class_name = word_strstr(from->data, "class");
  class_name += 5;
  adv_ptr_to_next_word(class_name);
  const char *end = class_name;
  adv_ptr_past_word(end);
  string name(class_name, (end - class_name));
  nlog("class name: %s\n", class_name);
  class_info.name = name;
  int end_line    = find_class_end_line(from);
  for (; from; from = from->next) {
    if (strchr(from->data, '{')) {
      from = from->next;
      break;
    }
  }
  for (linestruct *line = from; line != NULL && line->lineno < end_line; line = line->next) {
    line_variable(line, class_info.variables);
  }
  class_info_vector.push_back(class_info);
  test_map[class_info.name] = {FG_VS_CODE_GREEN};
}
