#include "../../include/prototypes.h"

LanguageServer *LanguageServer::_instance = nullptr;

auto LanguageServer::find_endif(linestruct *from) -> int {
  int         lvl   = 0;
  const char *found = nullptr;
  const char *start = nullptr;
  const char *end   = nullptr;
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

auto LanguageServer::fetch_compiler_defines(string compiler) -> void {
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
      const char *end   = nullptr;
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

/* This is the only way to access the language_server.  There
 * also exist`s a shorthand for this function call named 'LSP'. */
auto LanguageServer::instance(void) -> LanguageServer *const & {
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

auto LanguageServer::_destroy(void) noexcept -> void {
  LSP->index.delete_data();
  delete _instance;
}

/* Return`s index of entry if found otherwise '-1'. */
auto LanguageServer::is_defined(const string &name) -> int {
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
  const char    *found = nullptr;
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
auto LanguageServer::parse_full_pp_delc(linestruct *line, const char **ptr, int *end_lineno) -> string {
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

auto LanguageServer::check(IndexFile *idfile) -> void {
  PROFILE_FUNCTION;
  linestruct *from = idfile->head ? idfile->head : openfile->filetop;
  FOR_EACH_LINE_NEXT(line, from) {
    Parse::comment(line);
    if (line->flags.is_set<BLOCK_COMMENT_START>() || line->flags.is_set<BLOCK_COMMENT_END>() ||
        line->flags.is_set<IN_BLOCK_COMMENT>() || line->flags.is_set<PP_LINE>()) {
      continue;
    }
    if (!(line->flags.is_set<DONT_PREPROSSES_LINE>())) {
      do_preprossesor(line, idfile->file);
    }
    if (line->flags.is_set<PP_LINE>()) {
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

int LanguageServer::index_file(const char *path) {
  PROFILE_FUNCTION;
  if (!path) {
    logE("Path: '%s', Is invalid.\n", path);
    return -1;
  }
  char *absolute_path = nullptr;
  if (*path == '/') {
    absolute_path = copy_of(path);
  }
  else {
    absolute_path  =  get_full_path(path);
  }
  if (!absolute_path || !is_file_and_exists(absolute_path)) {
    // logE("Failed to get absolute path for file: '%s'.", path);
    return -1;
  }
  if (has_been_included(absolute_path)) {
    free(absolute_path);
    logE("File: '%s', Has already been include.", absolute_path);
    return -1;
  }
  unix_socket_debug("path %s\n", absolute_path);
  IndexFile idfile;
  idfile.file = absolute_path;
  idfile.head = retrieve_file_as_lines(absolute_path);
  index.include[absolute_path] = idfile;
  linestruct *head = idfile.head;
  while (head) {
    linestruct *line = head;
    head = line->next;
    Parse::comment(line);
    if (line->flags.is_set<BLOCK_COMMENT_START>() || line->flags.is_set<BLOCK_COMMENT_END>() ||
        line->flags.is_set<IN_BLOCK_COMMENT>() || line->flags.is_set<PP_LINE>()) {
      continue;
    }
    if (!(line->flags.is_set<DONT_PREPROSSES_LINE>())) {
      do_preprossesor(line, absolute_path);
    }
    if (line->flags.is_set<PP_LINE>()) {
      continue;
    }
  }
  return 0;
}
