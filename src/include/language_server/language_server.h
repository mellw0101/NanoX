#pragma once

#include "def.h"

inline namespace LSP_utils {
  /* fd[0] is the read pipe, fd[1] is the write pipe.  Remember to exit at the end of the function and make sure it terminates. */
  template <typename Function, typename... Args>
  __inline__ int fork_function(pid_t *pid, Function &&function, Args &&...args) {
    int fd[2];
    if (pipe(fd) == -1) {
      logE("pipe");
      exit(1);
    }
    if ((*pid = fork()) == 0) {
      close(fd[0]);
      if (dup2(fd[1], STDOUT_FILENO) < 0) {
        logE("dup2");
        exit(1);
      }
      if (dup2(fd[1], STDERR_FILENO) < 0) {
        logE("dup2");
        exit(1);
      }
      function(args...);
    }
    close(fd[1]);
    return fd[0];
  }
  string parse_full_define(linestruct *from, const char **ptr, int *endline = NULL);
}

/* preprossesor */
void do_preprossesor(linestruct *, const char *);
/* data_parse */
namespace Parse {
  inline namespace utils {
    string normalise_path(const char *path);
  }
  void comment(linestruct *line);
  void struct_type(linestruct **from, const char *current_file);
  void enum_type(linestruct **from);
  void variable(linestruct *line, const char *current_file, vector<var_t> &var_vector);
  void function(linestruct **line, const char *current_file);
}
void do_parse(linestruct **line, const char *current_file);
void do_bash_parse(linestruct *line, const char *current_file);

class LanguageServer {
  static LanguageServer *_instance;

  LanguageServer(void) {};
  static void _destroy(void) noexcept;

  void           fetch_compiler_defines(string);
  vector<string> split_if_statement(const string &str);

 public:
  Index index;

  static LanguageServer *const &instance(void);

  auto find_endif(linestruct *) -> int;
  auto is_defined(const string &) -> int;
  auto define_value(const string &) -> string;
  string parse_full_pp_delc(linestruct *, const char **, int * = NULL);
  void check(IndexFile *idfile);

  bool has_been_included(const char *path);
  int index_file(const char *path, bool reindex = FALSE);
};
#define LSP LanguageServer::instance()

struct bash_calleble {

};

#define blsp bash_language_server_protocol
class blsp {
 private:
  static blsp *_instance;

 public:
  static blsp *const &instance(void);

  unordered_map<string, bash_calleble> calleble;
};
#define bash_lsp blsp::instance()