#pragma once

#include "def.h"

inline namespace utils {
  /* fd[0] is the read pipe.
   * fd[1] is the write pipe.
   * Remember to exit at the end of the function and make sure it terminates. */
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
}

/* preprossesor */
void do_preprossesor(linestruct *, const char *);
/* data_parse */
namespace Parse {
  inline namespace utils {
    string normalise_path(const char *path);
  }
  void comment(linestruct *line);
  void variable(linestruct *line, vector<var_t> &var_vector);
}

class language_server_t
{
  static language_server_t *_instance;
  static pthread_mutex_t    _init_mutex;
  pthread_mutex_t           _mutex;
  // vector<define_entry_t>    _defines;

  language_server_t(void);
  static void _destroy(void) noexcept;

  void           fetch_compiler_defines(string);
  vector<string> split_if_statement(const string &str);

  public:
  Index index;

  static language_server_t &instance(void);

  int    find_endif(linestruct *);
  int    is_defined(const string &);
  bool   has_been_included(const char *);
  string define_value(const string &);

  void   check(linestruct *, string);
  void   add_defs_to_color_map(void);
  string parse_full_pp_delc(linestruct *, const char **, int * = NULL);
  void   add_define(const DefineEntry &entry);

  MVector<DefineEntry> retrieve_defines(void);
};
#define LSP_t             language_server_t
#define LSP               LSP_t::instance()
#define ADD_BASE_DEF(def) add_define({copy_of(#def), copy_of(#def), copy_of(to_string(def).data())})