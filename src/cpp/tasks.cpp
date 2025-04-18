#include "../include/prototypes.h"

/* These are the functions that the sub threads perform. */
struct sub_thread_function {
  static void *make_line_list_from_file(void *arg) {
    char *path = (char *)arg;
    if (!file_exists(path)) {
      logE("Path: '%s' is not a file or does not exist.", path);
      free(path);
      return NULL;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
      logE("Failed to open file '%s'.", path);
      free(path);
      return NULL;
    }
    static thread_local char *buf = NULL;
    Ulong             size;
    long                      len;
    linestruct               *head = NULL;
    linestruct               *tail = NULL;
    while ((len = getline(&buf, &size, file)) != EOF) {
      if (buf[len - 1] == '\n') {

        buf[--len] = '\0';
      }
      linestruct *line = make_new_node(tail);
      line->data       = measured_copy(buf, len);
      if (tail == NULL) {
        head = line;
        tail = line;
      }
      else {
        tail->next = line;
        tail       = tail->next;
      }
    }
    fclose(file);
    free(path);
    return head;
  }

  // static void *functions_from(void *arg) {
  //   char  *path      = (char *)arg;
  //   char **functions = find_functions_in_file(path);
  //   free(path);
  //   return functions;
  // }

  // static void *glob_vars_from(void *arg) {
  //   char  *path      = (char *)arg;
  //   char **glob_vars = find_variabels_in_file(path);
  //   free(path);
  //   return glob_vars;
  // }
};

/* Callback`s that the main thread performs. */
struct main_thread_function {
  /* This is the function that the main thread will perform when it is placed in the callback queue. */
  static void on_search_complete(void *result) {
    word_search_task_t *search_result = (word_search_task_t *)result;
    if (!search_result->words) {
      free(search_result);
      return;
    }
    Ulong i;
    NETLOGGER.log("words fetched: %lu.\n", search_result->nwords);
    for (i = 0; i < search_result->nwords; i++) {
      free(search_result->words[i]);
    }
    free(search_result->words);
    free(search_result->path);
    free(search_result);
  }

  static void handle_found_functions(void *arg) {
    if (!arg) {
      return;
    }
    char **functions = (char **)arg;
    for (int i = 0; functions[i]; i++) {
      nlog("function found %s\n", functions[i]);
      function_info_t info = parse_local_func(functions[i]);
      if (info.name) {
        /* char       *func = copy_of(info.name);
        const auto &it   = color_map.find(func);
        if (it == color_map.end())
        {
            color_map[func] = {FG_VS_CODE_BRIGHT_YELLOW};
        }
        else
        {
            free(func);
        } */
      }
      free(functions[i]);
    }
    free(functions);
  }

  static void handle_found_glob_vars(void *arg) {
    if (!arg) {
      return;
    }
    char **vars = (char **)arg;
    for (int i = 0; vars[i]; i++) {
      glob_var_t gv;
      parse_variable(vars[i], &gv.type, &gv.name, &gv.value);
      if (gv.name) {
        /* const auto &it = color_map.find(gv.name);
        if (it == color_map.end())
        {
            color_map[gv.name] = {FG_VS_CODE_BRIGHT_CYAN};
            glob_vars.push_back(gv);
        }
        else
        {
            NULL_safe_free(gv.type);
            NULL_safe_free(gv.name);
            NULL_safe_free(gv.value);
        } */
      }
      else {
        free(gv.type);
        free(gv.value);
      }
      free(vars[i]);
    }
    free(vars);
  }

  static void handle_parsed_funcs(void *arg) {
    if (arg == NULL) {
      return;
    }
    char            **funcs = (char **)arg;
    int               cap = 10, size = 0;
    function_info_t **info_array = (function_info_t **)nmalloc(cap * sizeof(**info_array));
    for (int i = 0; funcs[i]; i++) {
      function_info_t *info = parse_func(funcs[i]);
      if (info != NULL) {
        (cap == size) ? cap *= 2, info_array = (function_info_t **)nrealloc(info_array, cap * sizeof(**info_array)) : 0;
        info_array[size++] = info;
        free(funcs[i]);
      }
    }
    free(funcs);
    info_array[size] = NULL;
    /* if (func_info == NULL)
    {
        func_info = info_array;
    }
    else
    {
        for (int i = 0;; i++)
        {
            if (func_info[i] == NULL)
            {
                func_info =
                    (function_info_t **)nrealloc(func_info, (i + size + 1) *
    sizeof(function_info_t *)); int k = 0; for (; info_array[k]; k++)
                {
                    func_info[i + k] = info_array[k];
                }
                func_info[i + k] = NULL;
                break;
            }
        }
    } */
  }

  static void get_line_list(void *arg) {
    if (arg == NULL) {
      return;
    }
    linestruct *head = (linestruct *)arg;
    for (linestruct *line = head; line; line = line->next) {
      const char *class_p = word_strstr(line->data, "class");
      if (class_p && strstr(line->data, "/*") == NULL) {
        // const char *start = NULL;
        const char *end = class_p + 5;
        nlog("%s\n", line->data);
        if (*end == ' ' || *end == '\t') {
          adv_ptr_to_next_word(end);
          if (*end) {
            // start = end;
            adv_ptr_past_word(end);
            parse_class_data(line);
          }
        }
      }
    }
    while (head) {
      linestruct *line = head;
      head             = line->next;
      free(line->data);
      free(line);
    }
  }

  static void on_find_file_in_dir(void *arg) {
    PROFILE_FUNCTION;
    dir_search_task_t *result = (dir_search_task_t *)arg;
    if (result->found == TRUE) {
      pause_sub_threads_guard_t pause_guard;
      LOUT_logI("Found file: '%s' in dir: '%s'.", result->find, result->dir);
    }
    free(result->find);
    free(result->dir);
    free(result);
  }
};

/* Helpers to simplyfy task ptr creation. */
struct task_creator {
  static word_search_task_t *create_word_search_task(const char *str) {
    word_search_task_t *task = (word_search_task_t *)nmalloc(sizeof(word_search_task_t));
    task->path               = copy_of(str);
    return task;
  }

  static dir_search_task_t *create_dir_search_task(const char *find, const char *in_dir) {
    dir_search_task_t *task = (dir_search_task_t *)nmalloc(sizeof(dir_search_task_t));
    task->find              = copy_of(find);
    task->dir               = copy_of(in_dir);
    return task;
  }
};

/* Calleble functions begin here.  Above are staic functions. */

// void find_functions_task(const char *path) {
//   char *alloced_path = copy_of(path);
//   submit_task(sub_thread_function::functions_from, alloced_path, NULL, main_thread_function::handle_found_functions);
// }

// void find_glob_vars_task(const char *path) {
//   char *alloced_path = copy_of(path);
//   submit_task(sub_thread_function::glob_vars_from, alloced_path, NULL, main_thread_function::handle_found_glob_vars);
// }

void get_line_list_task(const char *path) {
  char *arg = copy_of(path);
  submit_task(sub_thread_function::make_line_list_from_file, arg, NULL, main_thread_function::get_line_list);
}
