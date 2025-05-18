#include "../../include/prototypes.h"

LanguageServer *LanguageServer::_instance = NULL;

int LanguageServer::find_endif(linestruct *from) {
  int         lvl   = 0;
  const char *found = NULL;
  const char *start = NULL;
  const char *end   = NULL;
  FOR_EACH_LINE_NEXT(line, from) {
    // if (!(line->flags.is_set(DONT_PREPROSSES_LINE))) {
    if (!(line->is_dont_preprocess_line)) {
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

void LanguageServer::fetch_compiler_defines(string compiler) {
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

      DefineEntry de;
      de.name      = measured_copy(name.data(), name.size());
      de.full_decl = measured_copy(name.data(), name.size());
      de.value     = measured_copy(value.data(), value.size());
      de.file      = copy_of("");

      index.defines[de.name] = de;
      free(lines[i]);
    }
    free(lines);
  }
}

/* This is the only way to access the language_server.  There also exist`s a shorthand for this function call named 'LSP'. */
LanguageServer *const &LanguageServer::instance(void) {
  if (!_instance) {
    _instance = new LanguageServer();
    if (!_instance) {
      logE("Failed to alloc 'LanguageServer'.");
      die("Failed to alloc 'LanguageServer'.");
    }
    atexit(_destroy);
  }
  return _instance;
}

void LanguageServer::_destroy(void) noexcept {
  LSP->index.delete_data();
  delete _instance;
}

/* Return`s index of entry if found otherwise '-1'. */
int LanguageServer::is_defined(const string &name) {
  return -1;
}

/* If define is in vector then return its value.  Note that the caller should check is define exists
 * before using this function.  As it returns "" if not found as well as if the define has no value. */
auto LanguageServer::define_value(const string &name) -> string {
  // int idx = is_defined(name);
  // if (idx != -1) {
  //   const auto *data = index.defines.begin();
  //   return data[idx].value;
  // }
  return "";
}

auto LanguageServer::split_if_statement(const string &str) -> vector<string> {
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

/* Parses a full preprossesor decl so that '\' are placed on the same line. */
string LanguageServer::parse_full_pp_delc(linestruct *line, const char **ptr, int *end_lineno) {
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
  } while (*end);
  end_lineno ? *end_lineno = (int)line->lineno : 0;
  return ret;
}

void LanguageServer::check(IndexFile *idfile) {
  PROFILE_FUNCTION;
  linestruct * from = idfile->top() ? idfile->top() : openfile->filetop;
  FOR_EACH_LINE_NEXT(line, from) {
    Parse::comment(line);
    // if (line->flags.is_set<BLOCK_COMMENT_START>() || line->flags.is_set<BLOCK_COMMENT_END>() ||
    //     line->flags.is_set<IN_BLOCK_COMMENT>() || line->flags.is_set<PP_LINE>()) {
    if (line->is_block_comment_start || line->is_block_comment_end || line->is_in_block_comment || line->is_pp_line) {
      continue;
    }
    // if (!(line->flags.is_set<DONT_PREPROSSES_LINE>())) {
    if (!(line->is_dont_preprocess_line)) {
      do_preprossesor(line, idfile->name());
    }
    // if (line->flags.is_set<PP_LINE>()) {
    if (line->is_pp_line) {
      continue;
    }
    // do_parse(&line, idfile->file);
  }
}

bool LanguageServer::has_been_included(const char *path) {
  const auto &it = index.include.find(path);
  if (it != index.include.end()) {
    return true;
  }
  return false;
}

int LanguageServer::index_file(const char *path, bool reindex) {
  PROFILE_FUNCTION;
  if (!path) {
    logE("Path: '%s', Is invalid.\n", path);
    return -1;
  }
  char *absolute_path = abs_path(path);
  if (!absolute_path || !file_exists(absolute_path)) {
    return -1;
  }
  if (has_been_included(absolute_path)) {
    if (!reindex) {
      free(absolute_path);
      return -1;
    }
    index.include[absolute_path].delete_data();
    index.include.erase(absolute_path);
    if (/* openfile->type.is_set<BASH>() */ openfile->is_bash_file) {
      index.bash_data.delete_data();
    }
  }
  IndexFile idfile;
  idfile.read_file(absolute_path);
  index.include[absolute_path] = idfile;
  free(absolute_path);
  FOR_EACH_LINE_NEXT(line, idfile.top()) {
    if (/* openfile->type.is_set<C_CPP>() */ openfile->is_c_file || openfile->is_cxx_file) {
      Parse::comment(line);
      // if (line->flags.is_set<BLOCK_COMMENT_START>() || line->flags.is_set<BLOCK_COMMENT_END>()
      // || line->flags.is_set<IN_BLOCK_COMMENT>() || line->flags.is_set<PP_LINE>()) {
      if (line->is_block_comment_start || line->is_block_comment_end || line->is_in_block_comment || line->is_pp_line) {
        continue;
      }
      // if (!(line->flags.is_set<DONT_PREPROSSES_LINE>())) {
      if (!(line->is_dont_preprocess_line)) {
        do_preprossesor(line, idfile.name());
      }
      // if (line->flags.is_set<PP_LINE>()) {
      if (line->is_pp_line) {
        continue;
      }
      do_parse(&line, idfile.name());
    }
    else if (/* openfile->type.is_set<BASH>() */ openfile->is_bash_file) {
      do_bash_parse(line, idfile.name());
    }
  }
  return 0;
}
