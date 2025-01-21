#include "../../include/c_proto.h"
#include "../fd.h"
#include "../xstring.h"
#include "../mem.h"
#include "nfdreader.h"

#include <pthread.h>
#include <fcntl.h>

/* `Internal`  Structure that holds the data that the event-handler will execute. */
typedef struct {
  nevhandler *handler;
  Uchar *data;
  long len;
  void (*cb)(nevhandler *handler, const Uchar *data, long);
} nfdreader_package;

/* `Opaque`  Structure that reads from a fd and safely sends the data to the event-handler. */
struct nfdreader {
  nevhandler *handler;
  int fd;
  void (*cb)(nevhandler *handler, const Uchar *data, long len);
  pthread_t thread;
  pthread_mutex_t mutex;
  bool stop;
};

nfdreader *nfdreader_stdin = NULL;

/* `Internal`  Callback that will run in the event-handler. */
static void nfdreader_package_callback(void *arg) {
  nfdreader_package *package = arg;
  package->cb(package->handler, package->data, package->len);
  free(package->data);
  free(package);
}

/* `Internal`  Create a callback package to send to the event-handler. */
static nfdreader_package *nfdreader_package_create(nfdreader *reader, const Uchar *data, long len) {
  nfdreader_package *package = xmalloc(sizeof(*package));
  package->handler = reader->handler;
  package->data    = xmeml_copy(data, len);
  package->len     = len;
  package->cb      = reader->cb;
  return package;
}

/* `Internal`  Task that the thread will run. */
static void *nfdreader_thread_task(void *arg) {
  nfdreader *reader = arg;
  Uchar buffer[4096];
  long bytes_read;
  while (1) {
    pthread_mutex_lock(&reader->mutex);
    if (reader->stop) {
      pthread_mutex_unlock(&reader->mutex);
      break;
    }
    pthread_mutex_unlock(&reader->mutex);
    /* Lock then read the file descriptor. */
    lock_fd(reader->fd, F_RDLCK);
    bytes_read = read(reader->fd, buffer, sizeof(buffer));
    /* Once we have read the file-descriptor, unlock it. */
    unlock_fd(reader->fd);
    if (bytes_read > 0) {
      nevhandler_submit(reader->handler, nfdreader_package_callback, nfdreader_package_create(reader, buffer, bytes_read));
    }
    else if (bytes_read == 0) {
      break;
    }
    else if (bytes_read < 0) {
      break;
    }
  }
  return NULL;
}

nfdreader *nfdreader_create(nevhandler *handler, int fd, void (*cb)(nevhandler *handler, const Uchar *data, long len)) {
  if (!handler || fd < 0 || !cb) {
    die("%s: Invalid input parameters.\n", __func__);
  }
  nfdreader *reader = xmalloc(sizeof(*reader));
  reader->handler = handler;
  reader->fd = fd;
  reader->cb = cb;
  reader->stop = FALSE;
  pthread_mutex_init(&reader->mutex, NULL);
  if (pthread_create(&reader->thread, NULL, nfdreader_thread_task, reader) != 0) {
    pthread_mutex_destroy(&reader->mutex);
    free(reader);
    die("%s: Failed to create pthread.\n", __func__);
  }
  pthread_detach(reader->thread);
  return reader;
}

void nfdreader_stop(nfdreader *reader) {
  if (!reader) {
    return;
  }
  pthread_mutex_lock(&reader->mutex);
  reader->stop = TRUE;
  pthread_mutex_unlock(&reader->mutex);
  // pthread_join(reader->thread, NULL);
}

void nfdreader_free(nfdreader *reader) {
  if (!reader) {
    return;
  }
  pthread_mutex_lock(&reader->mutex);
  bool should_free = reader->stop;
  pthread_mutex_unlock(&reader->mutex);
  if (should_free) {
    pthread_mutex_destroy(&reader->mutex);
    close(reader->fd);
    free(reader);
    reader = NULL;
  }
}

