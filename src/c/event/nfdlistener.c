/** @file nfdlistener.c

  @author  Melwin Svensson.
  @date    22-1-2025.

 */
#include "../../include/c_proto.h"

#include "../mem.h"
#include "../xstring.h"

#include <pthread.h>
#include <sys/inotify.h>

#define EVENT_SIZE    (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

/* `Internal`  Structure that reprecents the package that holds all nessesary data the
 * the event handler will need to perform the callback that is passed to the fd listener. */
typedef struct {
  nevhandler *handler;
  nfdlistener_cb cb;
  nfdlistener_event *ev;
} nfdlistener_package;

/* `Opaque`  Structure that reprecents the fd listener.  This is opaque to ensure correctness. */
struct nfdlistener {
  nevhandler *handler;
  int fd;
  int wd;
  char *file;
  pthread_t thread;
  pthread_mutex_t mutex;
  nfdlistener_cb cb;
  bool stop;
};

/* `Internal`  Create the package that gets sent to the event handler when its performing the callback. */
static nfdlistener_package *nfdlistener_package_create(nfdlistener *listener, struct inotify_event *ev) {
  nfdlistener_package *package = xmalloc(sizeof(*package));
  package->handler = listener->handler;
  package->cb      = listener->cb;
  package->ev      = xmalloc(sizeof(*package->ev));
  package->ev->mask   = ev->mask;
  package->ev->cookie = ev->cookie;
  package->ev->file   = listener->file;
  return package;
}

/* `Internal`  The task that the event handler runs. */
static void nfdlistener_handler_task(void *arg) {
  nfdlistener_package *package = arg;
  package->cb(package->ev);
  free(package->ev);
  free(package);
}

/* `Internal`  Free the fd listener, this is internal because this is done when the listener is stopped by the thread that ran it. */
static void nfdlistener_free(nfdlistener *listener) {
  if (!listener) {
    return;
  }
  if (listener->wd != -1) {
    inotify_rm_watch(listener->fd, listener->wd);
    listener->wd = -1;
  }
  if (listener->fd != -1) {
    close(listener->fd);
    listener->fd = -1;
  }
  pthread_mutex_destroy(&listener->mutex);
  free(listener->file);
  free(listener);
}

/* `Internal`  The task the fd listener performs in a seperate thread. */
static void *nfdlistener_task(void *arg) {
  nfdlistener *listener = arg;
  char buffer[EVENT_BUF_LEN];
  long len, i;
  while (1) {
    pthread_mutex_lock(&listener->mutex);
    if (listener->stop) {
      pthread_mutex_unlock(&listener->mutex);
      break;
    }
    pthread_mutex_unlock(&listener->mutex);
    len = read(listener->fd, buffer, EVENT_BUF_LEN);
    if (len <= 0) {
      break;
    }
    for (i = 0; i < len;) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      nevhandler_submit(listener->handler, nfdlistener_handler_task, nfdlistener_package_create(listener, event));
      i += (EVENT_SIZE + event->len);
    }
  }
  nfdlistener_free(listener);
  return NULL;
}

nfdlistener *nfdlistener_create(nevhandler *handler, const char *file, Uint mask, nfdlistener_cb cb) {
  nfdlistener *listener = xmalloc(sizeof(*listener));
  listener->stop    = FALSE;
  listener->handler = handler;
  listener->cb      = cb;
  listener->file    = xstr_copy(file);
  pthread_mutex_init(&listener->mutex, NULL);
  listener->wd = -1;
  listener->fd = inotify_init();
  if (listener->fd < 0) {
    nfdlistener_free(listener);
    return NULL;
  }
  listener->wd = inotify_add_watch(listener->fd, listener->file, mask);
  if (listener->wd < 0) {
    nfdlistener_free(listener);
    return NULL;
  }
  if (pthread_create(&listener->thread, NULL, nfdlistener_task, listener) != 0) {
    nfdlistener_free(listener);
    return NULL;
  }
  pthread_detach(listener->thread);
  return listener;
}

void nfdlistener_stop(nfdlistener *listener) {
  if (!listener) {
    return;
  }
  pthread_mutex_lock(&listener->mutex);
  listener->stop = TRUE;
  pthread_mutex_unlock(&listener->mutex);
}

#undef EVENT_SIZE
#undef EVENT_BUF_LEN
