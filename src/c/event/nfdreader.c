/** @file nfdreader.c

  @author  Melwin Svensson.
  @date    1-13-2025.

 */
#include "../../include/c_proto.h"

#include <pthread.h>
#include <fcntl.h>

/* `Internal`  Structure that holds the data that the event-handler will execute. */
typedef struct {
  nevhandler *handler;
  Uchar *data;
  long len;
  nfdreader_cb callback;  
} nfdreader_package;

/* `Opaque`  Structure that reads from a fd and safely sends the data to the event-handler. */
struct nfdreader {
  nevhandler *handler;
  int fd;
  nfdreader_cb callback;
  pthread_t thread;
  pthread_mutex_t mutex;
  bool stop;
};

nfdreader *nfdreader_stdin = NULL;

/* `Internal`  Callback that will run in the event-handler. */
static void nfdreader_package_callback(void *arg) {
  nfdreader_package *package = arg;
  package->callback(package->handler, package->data, package->len);
  free(package->data);
  free(package);
}

/* `Internal`  Create a callback package to send to the event-handler. */
static nfdreader_package *nfdreader_package_create(nfdreader *reader, const Uchar *data, long len) {
  nfdreader_package *package = xmalloc(sizeof(*package));
  package->handler = reader->handler;
  package->data    = xmeml_copy(data, len);
  package->len     = len;
  package->callback      = reader->callback;
  return package;
}

/* Close the fd and free the reader.  Note that this will only do that if `nfdreader_stop()` has been called. */
static void nfdreader_free(nfdreader *reader) {
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

/* `Internal`  Task that the thread will run. */
static void *nfdreader_thread_task(void *arg) {
  nfdreader *reader = arg;
  Uchar buffer[4096];
  long bytes_read;
  while (1) {
    mutex_action(&reader->mutex,
      if (reader->stop) {
        mutex_unlock(&reader->mutex);
        break;
      }
    );
    /* Lock then read the file descriptor. */
    fdlock_action(reader->fd, F_RDLCK,
      bytes_read = read(reader->fd, buffer, sizeof(buffer));
    );
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
  nfdreader_free(reader);
  return NULL;
}

nfdreader *nfdreader_create(nevhandler *handler, int fd, nfdreader_cb callback) {
  if (!handler || fd < 0 || !callback) {
    die("%s: Invalid input parameters.\n", __func__);
  }
  nfdreader *reader = xmalloc(sizeof(*reader));
  reader->handler = handler;
  reader->fd = fd;
  reader->callback = callback;
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
}


