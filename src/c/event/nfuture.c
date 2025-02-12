#include "../../include/c_defs.h"
#include "nfuture.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <malloc.h>

/* The struct that holds the opaque future data, hidden from api. */
typedef struct nfuture {
  pthread_mutex_t mutex;  /* The mutex to protect the setting of the result and ready flag. */
  pthread_cond_t  cond;   /* Condition to signal when `result` is ready, if there is anyone listening. */
  bool  ready;            /* Flag to tell potention listeners (or checkers) that the data is ready to be read. */
  void *result;           /* Ptr to the data. */
} nfuture;

/* Fully intenal data struct that represents the task. */
typedef struct {
  nfuture *future;
  void *(*task)(void *);
  void *arg;
} nfuture_task;

/* Init the fatal error callback. */
void (*nfuture_fatal_error_cb)(const char *format, ...) = NULL;

/* `Internal function`  Create the opaque `nfuture` structure. */
static nfuture *nfuture_create(void) {
  nfuture *future = malloc(sizeof(*future));
  if (!future) {
    if (nfuture_fatal_error_cb) {
      nfuture_fatal_error_cb("%s: Failed to malloc nfuture.\n", __func__);
    }
    return NULL;
  }
  pthread_mutex_init(&future->mutex, NULL);
  pthread_cond_init(&future->cond, NULL);
  future->ready = FALSE;
  future->result = NULL;
  return future;
}

/* `Internal function`  This function runs and assigns the result to the future. */
static void *nfuture_task_callback(void *arg) {
  nfuture_task *data = arg;
  void *result = data->task(data->arg);
  pthread_mutex_lock(&data->future->mutex);
  data->future->result = result;
  data->future->ready = TRUE;
  pthread_cond_signal(&data->future->cond);
  pthread_mutex_unlock(&data->future->mutex);
  free(data);
  return NULL;
}

void nfuture_free(nfuture *future) {
  pthread_mutex_destroy(&future->mutex);
  pthread_cond_destroy(&future->cond);
  free(future);
}

void *nfuture_get(nfuture *future) {
  if (!future) {
    /* If the fatal error callback terminates as it should, then we die here.  Never 'handle' missuse, always punish. */
    if (nfuture_fatal_error_cb) {
      nfuture_fatal_error_cb("%s: Invalid input parameters.\n", __func__);
    }
    return NULL;
  }
  /* Lock the mutex so we can wait if the future is not ready yet. */
  pthread_mutex_lock(&future->mutex);
  while (!future->ready) {
    /* Wait until we recieve a signal. */
    pthread_cond_wait(&future->cond, &future->mutex);
  }
  /* Fetch the result from the future, then unlock it. */
  void *result = future->result;
  pthread_mutex_unlock(&future->mutex);
  /* Free the used future as it is of no use. */
  nfuture_free(future);
  future = NULL;
  /* Return the result. */
  return result;
}

bool nfuture_try_get(nfuture *future, void **result) {
  if (!future || !result) {
    /* If the fatal error callback terminates as it should, then we die here.  Never 'handle' missuse, always punish. */
    if (nfuture_fatal_error_cb) {
      nfuture_fatal_error_cb("%s: Invalid input parameters.\n", __func__);
    }
    return FALSE;
  }
  pthread_mutex_lock(&future->mutex);
  if (future->ready) {
    *result = future->result;
  }
  else {
    *result = NULL;
  }
  pthread_mutex_unlock(&future->mutex);
  return (*result);
}

nfuture *nfuture_submit(void *(*task)(void *), void *arg) {
  /* Create the internal data struct the thread will use. */
  nfuture_task *data = malloc(sizeof(*data));
  if (!data) {
    if (nfuture_fatal_error_cb) {
      nfuture_fatal_error_cb("%s: Failed to malloc nfuture_task.\n", __func__);
    }
    return NULL;
  }
  /* Create the actual future and insert the task and arg. */
  data->future = nfuture_create();
  data->task = task;
  data->arg  = arg;
  /* Then start and detach the task. */
  pthread_t thread;
  pthread_create(&thread, NULL, nfuture_task_callback, data);
  pthread_detach(thread);
  /* And return the pointer to the future. */
  return data->future;
}

void nfuture_set_fatal_error_callback(void (*cb)(const char *, ...)) {
  nfuture_fatal_error_cb = cb;
}
