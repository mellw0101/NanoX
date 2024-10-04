#pragma once
#include "definitions.h"

#define MAX_THREADS 8
#define QUEUE_SIZE  200

typedef void *(*task_functionptr_t)(void *);

/* Struct`s for task`s to the sub thread`s. */
TASK_STRUCT(task_t, void *(*function)(void *); void *arg; void **result; void (*callback)(void *);)
TASK_STRUCT(task_queue_t, task_t tasks[QUEUE_SIZE]; int front; int rear; int count; pthread_mutex_t mutex;
            pthread_cond_t cond;)

typedef void (*callback_functionptr_t)(void *);

/* These are the callback struct`s. */
LIST_STRUCT(callback_node_t, callback_functionptr_t callback; void *result; callback_node_t * next;)
TASK_STRUCT(callback_queue_t, callback_node_t *head; callback_node_t * tail; pthread_mutex_t mutex;)

/* Task struct`s to perform action`s. */
TASK_STRUCT(word_search_task_t, char **words; unsigned long nwords; char *path;)
TASK_STRUCT(dir_search_task_t, char *dir; char **entrys; char *find; bool found;)
TASK_STRUCT(delete_c_syntax_task_t, syntaxtype *syntax_type; char *word; unsigned long iter;)
TASK_STRUCT(add_c_syntax_task_t, char *color_fg; char *color_bg; char *rgxstr; colortype * *color_type;)
TASK_STRUCT(compile_rgx_task_t, char *rgxstr, *color_fg, *color_bg; regex_t * rgx; short fg, bg; int attr;
            colortype * *last_c;)

typedef struct syntax_word_t {
  char          *str;
  char          *return_type;
  syntax_word_t *next;
} syntax_word_t;

typedef struct syntax_search_t {
  syntax_word_t *functions_head;
  syntax_word_t *functions_tail;
} syntax_search_t;

/* Signal struct`s. */
TASK_STRUCT(signal_payload_t, void (*func)(void *); void *arg;)
TASK_STRUCT(main_thread_t, pthread_t thread; pid_t pid;)

bool is_main_thread(void);
void pause_all_sub_threads(bool pause);

struct pause_sub_threads_guard_t {
  bool from_main_thread;
  explicit pause_sub_threads_guard_t(void)
      : from_main_thread(is_main_thread()) {
    if (from_main_thread) {
      pause_all_sub_threads(TRUE);
    }
  }
  ~pause_sub_threads_guard_t(void) {
    if (from_main_thread) {
      pause_all_sub_threads(FALSE);
    }
  }
  pause_sub_threads_guard_t(const pause_sub_threads_guard_t &)            = delete;
  pause_sub_threads_guard_t &operator=(const pause_sub_threads_guard_t &) = delete;
};
