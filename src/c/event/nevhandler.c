/** @file nevhandler.c

  @author  Melwin Svensson.
  @date    17-1-2025.

 */
#include "nevhandler.h"

#include "../../include/c_proto.h"
#include "../mem.h"

#include <pthread.h>
#include <ck_array.h>

typedef struct {
  void (*cb)(void *);
  void *arg;
} nevent;

#define EVENTS_SIZE 1024

struct nevhandler {
  nevent events[EVENTS_SIZE];
  int front;
  int rear;
  int count;
  pthread_t       thread;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
  bool running;
  int status;
};

nevhandler *tui_handler = NULL;

nevhandler *nevhandler_create(void) {
  nevhandler *handler = xmalloc(sizeof(*handler));
  handler->front = 0;
  handler->rear  = 0;
  handler->count = 0;
  pthread_mutex_init(&handler->mutex, NULL);
  pthread_cond_init(&handler->cond, NULL);
  handler->running = FALSE;
  handler->status  = 0;
  return handler;
}

void nevhandler_free(nevhandler *handler) {
  if (!handler) {
    return;
  }
  pthread_mutex_destroy(&handler->mutex);
  pthread_cond_destroy(&handler->cond);
  free(handler);
  handler = NULL;
}

void nevhandler_submit(nevhandler *handler, void (*cb)(void *), void *arg) {
  if (!handler) {
    return;
  }
  pthread_mutex_lock(&handler->mutex);
  if (handler->count < EVENTS_SIZE) {
    handler->events[handler->rear].cb  = cb;
    handler->events[handler->rear].arg = arg;
    handler->rear = ((handler->rear + 1) % EVENTS_SIZE);
    ++handler->count;
    pthread_cond_signal(&handler->cond);
  }
  pthread_mutex_unlock(&handler->mutex);
}

void nevhandler_stop(nevhandler *handler, int status) {
  if (!handler) {
    return;
  }
  pthread_mutex_lock(&handler->mutex);
  handler->running = FALSE;
  handler->status  = status;
  pthread_cond_signal(&handler->cond);
  pthread_mutex_unlock(&handler->mutex);
}

/* `Internal`  Task that the handler will perform. */
static void *nevhandler_task(void *arg) {
  nevhandler *handler = arg;
  while (1) {
    pthread_mutex_lock(&handler->mutex);
    while (!handler->count && handler->running) {
      pthread_cond_wait(&handler->cond, &handler->mutex);
    }
    if (!handler->running) {
      pthread_mutex_unlock(&handler->mutex);
      break;
    }
    nevent event = handler->events[handler->front];
    handler->front = ((handler->front + 1) % EVENTS_SIZE);
    --handler->count;
    pthread_mutex_unlock(&handler->mutex);
    event.cb(event.arg);
  }
  return NULL;
}

int nevhandler_start(nevhandler *handler, bool spawn_thread) {
  if (!handler) {
    return -1;
  }
  pthread_mutex_lock(&handler->mutex);
  handler->running = TRUE;
  pthread_mutex_unlock(&handler->mutex);
  if (spawn_thread) {
    if (pthread_create(&handler->thread, NULL, nevhandler_task, handler) != 0) {
      return -1;
    }
    pthread_detach(handler->thread);
  }
  else {
    nevhandler_task(handler);
  }
  return 0;
}
