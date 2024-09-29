#include "../../include/prototypes.h"

LSP_t          *LSP_t::_instance   = nullptr;
pthread_mutex_t LSP_t::_init_mutex = PTHREAD_MUTEX_INITIALIZER;

int LSP_t::find_endif(linestruct *from) {
  int         lvl   = 0;
  const char *found = NULL;
  const char *start = NULL;
  const char *end   = NULL;
  FOR_EACH_LINE_NEXT(line, from) {
    if (!(line->flags.is_set(DONT_PREPROSSES_LINE))) {
      found = strchr(line->data, '#');
      if (found) {
        start = found + 1;
        ADV_TO_NEXT_WORD(start);
        if (!*start) {
          return -1;
        }
        end = start;
        ADV_PAST_WORD(end);
        string word(start, (end - start));
        if (word == "ifndef" || word == "ifdef" || word == "if") {
          lvl += 1;
        }
        else if (word == "endif") {
          if (lvl == 0) {
            return line->lineno;
          }
          lvl -= 1;
        }
      }
    }
  }
  return 0;
}

void LSP_t::fetch_compiler_defines(string compiler) {
  if ((compiler != "clang" && compiler != "clang++") && (compiler != "gcc" && compiler != "g++")) {
    logE("'%s' is an invalid compiler.", compiler.c_str());
    return;
  }
  compiler += " -dM -E - < /dev/null";
  Uint   n_lines;
  char **lines = retrieve_exec_output(compiler.c_str(), &n_lines);
  if (lines) {
    for (Uint i = 0; i < n_lines; i++) {
      const char *start = lines[i];
      const char *end   = NULL;
      ADV_PAST_WORD(start);
      ADV_TO_NEXT_WORD(start);
      if (!*start) {
        free(lines[i]);
        continue;
      }
      end = start;
      ADV_PAST_WORD(end);
      string name(start, (end - start));
      ADV_TO_NEXT_WORD(end);
      start = end;
      for (; *end; end++);
      if (start == end) {
        free(lines[i]);
        continue;
      }
      string value(start, (end - start));
      add_define({
          measured_copy(name.data(), name.size()),
          measured_copy(name.data(), name.size()),
          measured_copy(value.data(), value.size()),
      });
      free(lines[i]);
    }
    free(lines);
  }
}

/* This is the only way to access the language_server.
 * There also exist`s a shorthand for this function
 * call named 'LSP'. */
LSP_t &LSP_t::instance(void) {
  if (!_instance) {
    pthread_mutex_guard_t guard(&_init_mutex);
    if (!_instance) {
      _instance = new LSP_t();
    }
    atexit(LSP_t::_destroy);
  }
  return *_instance;
}

LSP_t::LSP_t(void) {
  pthread_mutex_init(&_mutex, NULL);
  fetch_compiler_defines("clang++");
  fetch_compiler_defines("clang");
  fetch_compiler_defines("g++");
  fetch_compiler_defines("gcc");
#ifdef __cplusplus
  ADD_BASE_DEF(__cplusplus);
#endif
  ADD_BASE_DEF(_GNU_SOURCE);
}

void LSP_t::_destroy(void) noexcept {
  pthread_mutex_destroy(&LSP._mutex);
  LSP.index.delete_data();
  delete _instance;
}

/* Return`s index of entry if found otherwise '-1'. */
int LSP_t::is_defined(const string &name) {
  const auto *data = index.defines.begin();
  for (int i = 0; i < index.defines.size(); ++i) {
    if (data[i].name == name) {
      return i;
    }
  }
  return -1;
}

bool LSP_t::has_been_included(const char *path) {
  for (const auto &i : index.include) {
    if (strcmp(i.file, path) == 0) {
      return true;
    }
  }
  return false;
}

/* If define is in vector then return its value.
 * Note that the caller should check is define
 * exists before using this function.
 * As it returns "" if not found as well as if
 * the define has no value. */
string LSP_t::define_value(const string &name) {
  int idx = is_defined(name);
  if (idx != -1) {
    const auto *data = index.defines.begin();
    return data[idx].value;
  }
  return "";
}

/* If define is not already in vector then add it. */
void LSP_t::add_define(const DefineEntry &entry) {
  if (is_defined(entry.name) == -1) {
    /* NLOG("define added: %s\n", entry.name.c_str()); */
    index.defines.push_back(entry);
  }
}

vector<string> LSP_t::split_if_statement(const string &str) {
  /* vector<string> result;
  const auto    *data  = str.data();
  const char    *found = NULL;
  Uint   index;
  do {
      found = string_strstr_array(
          data, {"defined", "&&", "!defined", "(", ")"}, &index);
      if (found)
      {
          result.push_back(str.substr((data - str.data()), (found - data)));
          data = found;
          NLOG("found: %s\n", found);
          if (index == 0)
          {
              data += 7;
          }
          else if (index == 1)

          {
              data += 2;
          }
          else if (index == 2)
          {
              data += 8;
          }
          else if (index == 3 || index == 4)
          {
              data += 1;
          }
      }
  }
  while (found);
  for (const auto &s : result)
  {
      NLOG("%s ", s.c_str());
  }
  NLOG("\n"); */
  return {};
}

const char *get_preprosses_type(linestruct *line, string &word) {
  const char *found = strchr(line->data, '#');
  const char *start = nullptr;
  const char *end   = nullptr;
  if (found) {
    start = found + 1;
    ADV_TO_NEXT_WORD(start);
    if (!*start) {
      return nullptr;
    }
    end = start;
    ADV_PAST_WORD(end);
    word = string(start, (end - start));
    return end;
  }
  return nullptr;
}

void LSP_t::check(linestruct *from, string file) {
  PROFILE_FUNCTION;
  if (!from) {
    from = openfile->filetop;
    file = openfile->filename;
    IndexFile idfile;
    idfile.file = openfile->filename;
    idfile.head = openfile->filetop;
    index.include.push_back(idfile);
  }
  FOR_EACH_LINE_NEXT(line, from) {
    Parse::comment(line);
    if (line->flags.is_set<BLOCK_COMMENT_START>() || line->flags.is_set<BLOCK_COMMENT_END>() ||
        line->flags.is_set<IN_BLOCK_COMMENT>() || line->flags.is_set<PP_LINE>()) {
      continue;
    }
    if (!(line->flags.is_set<DONT_PREPROSSES_LINE>())) {
      do_preprossesor(line, file.c_str());
    }
    vector<var_t> vars;
    Parse::variable(line, vars);
    if (!vars.empty()) {
      // for (const auto &[type, name, value, decl_line, scope_end] : vars)
      // {
      //     // NLOG("type: %s\n"
      //     //      "         name: %s\n"
      //     //      "        value: %s\n"
      //     //      "    decl_line: %d\n"
      //     //      "    scope_end: %d\n"
      //     //      "         file: %s\n\n",
      //     //      type.c_str(), name.c_str(), value.c_str(), decl_line, scope_end, file.c_str());
      // }
    }
  }
}

/* Add the current defs to color map. */
void LSP_t::add_defs_to_color_map(void) {
  const auto *data = index.defines.begin();
  for (int i = 0; i < index.defines.size(); ++i) {
    test_map[data[i].name] = {
        FG_VS_CODE_BLUE,
        -1,
        -1,
        DEFINE_SYNTAX,
    };
  }
  unix_socket_debug("LSP: Added %u full defines with name full decl and full value\n", index.defines.size());
}

/* Parses a full preprossesor decl so that '\' are placed on the same line. */
string LSP_t::parse_full_pp_delc(linestruct *line, const char **ptr, int *end_lineno) {
  PROFILE_FUNCTION;
  string      ret   = "";
  const char *start = *ptr;
  const char *end   = *ptr;
  do {
    ADV_TO_NEXT_WORD(start);
    if (!*start) {
      break;
    }
    end = start;
    ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '\\'));
    if (*end == '\\') {
      line  = line->next;
      start = line->data;
      end   = start;
    }
    else {
      if (!ret.empty()) {
        ret += " ";
      }
      ret += string(start, (end - start));
    }
    start = end;
  }
  while (*end);
  end_lineno ? *end_lineno = (int)line->lineno : 0;
  return ret;
}

MVector<DefineEntry> LSP_t::retrieve_defines(void) {
  return index.defines;
}
