/** @file nfdwriter.c

  @author  Melwin Svensson.
  @date    14-1-2025.

 */
#include "nfdwriter.h"

#include "../../include/c_proto.h"

#include <pthread.h>
#include <fcntl.h>

typedef struct {
  void *data;
  Ulong len;
} nfdwriter_task;

#define TASKS_SIZE  1024
#define BUFFER_SIZE 4096

struct nfdwriter {
  nfdwriter_task  tasks[TASKS_SIZE];
  char  buffer[BUFFER_SIZE];
  Ulong buffer_size;
  int front;
  int rear;
  int count;
  int fd;
  pthread_t       thread;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
  bool stop;
  bool flush;
};

nfdwriter *nfdwriter_stdout = NULL;
nfdwriter *nfdwriter_netlog = NULL;

/* `Internal`  Add data to the internal buffer. */
static inline void add_data_to_buffer(nfdwriter *writer, const void *data, Ulong len) {
  memcpy((writer->buffer + writer->buffer_size), data, len);
  writer->buffer_size += len;
}

/* `Internal`  The task the fd-writer run's. */
static void *nfdwriter_thread_task(void *arg) {
  nfdwriter *writer = arg;
  while (1) {
    pthread_mutex_lock(&writer->mutex);
    while (!writer->count && !writer->stop) {
      pthread_cond_wait(&writer->cond, &writer->mutex);
    }
    if (writer->stop) {
      lock_fd(writer->fd, F_WRLCK);
      write(writer->fd, writer->buffer, writer->buffer_size);
      unlock_fd(writer->fd);
      pthread_mutex_unlock(&writer->mutex);
      break;
    }
    nfdwriter_task task = writer->tasks[writer->front];
    writer->front = ((writer->front + 1) % TASKS_SIZE);
    --writer->count;
    pthread_mutex_unlock(&writer->mutex);
    lock_fd(writer->fd, F_WRLCK);
    write(writer->fd, task.data, task.len);
    unlock_fd(writer->fd);
    free(task.data);
    task.data = NULL;
  }
  return NULL;
}

nfdwriter *nfdwriter_create(int fd) {
  nfdwriter *writer = xmalloc(sizeof(*writer));
  writer->buffer_size = 0;
  writer->fd    = fd;
  writer->front = 0;
  writer->rear  = 0;
  writer->count = 0;
  pthread_mutex_init(&writer->mutex, NULL);
  pthread_cond_init(&writer->cond, NULL);
  writer->stop = FALSE;
  if (pthread_create(&writer->thread, NULL, nfdwriter_thread_task, writer) != 0) {
    pthread_mutex_destroy(&writer->mutex);
    pthread_cond_destroy(&writer->cond);
    free(writer);
    return NULL;
  }
  pthread_detach(writer->thread);
  return writer;
}

void nfdwriter_write(nfdwriter *writer, const void *data, Ulong len) {
  if (!writer || !data || !len) {
    return;
  }
  pthread_mutex_lock(&writer->mutex);
  {
    if ((writer->buffer_size + len) > BUFFER_SIZE) {
      writer->tasks[writer->rear].data = xmeml_copy(writer->buffer, writer->buffer_size);
      writer->tasks[writer->rear].len  = writer->buffer_size;
      writer->rear = ((writer->rear + 1) % TASKS_SIZE);
      ++writer->count;
      writer->buffer_size = 0;
      pthread_cond_signal(&writer->cond);
    }
    add_data_to_buffer(writer, data, len);
    // if (writer->count < TASKS_SIZE) {
    //   writer->tasks[writer->rear].data = xmeml_copy(data, len);
    //   writer->tasks[writer->rear].len  = len;
    //   writer->rear = ((writer->rear + 1) % TASKS_SIZE);
    //   ++writer->count;
    //   pthread_cond_signal(&writer->cond);
    // }
  }
  pthread_mutex_unlock(&writer->mutex);
}

int nfdwriter_printf(nfdwriter *writer, const char *format, ...) {
  if (!writer) {
    return 0;
  }
  va_list ap;
  va_start(ap,format);
  char *buf=NULL;
  int size=vasprintf(&buf,format,ap);
  va_end(ap);
  if (!buf) {
    return -1;
  }
  else if (size<0) {
    free(buf);
    return -1;
  }
  nfdwriter_write(writer,buf,size);
  free(buf);
  return 0;
}

void nfdwriter_stop(nfdwriter *writer) {
  if (!writer) {
    return;
  }
  pthread_mutex_lock(&writer->mutex);
  if (!writer->stop) {
    writer->stop = TRUE;
    pthread_cond_signal(&writer->cond);
  }
  pthread_mutex_unlock(&writer->mutex);
}

void nfdwriter_free(nfdwriter *writer) {
  if (!writer) {
    return;
  }
  pthread_mutex_lock(&writer->mutex);
  bool is_stopped = writer->stop;
  pthread_mutex_unlock(&writer->mutex);
  if (is_stopped) {
    pthread_mutex_destroy(&writer->mutex);
    pthread_cond_destroy(&writer->cond);
    free(writer);
    writer = NULL;
  }
}

void nfdwriter_flush(nfdwriter *writer) {
  pthread_mutex_lock(&writer->mutex);
  writer->tasks[writer->rear].data = xmeml_copy(writer->buffer, writer->buffer_size);
  writer->tasks[writer->rear].len  = writer->buffer_size;
  writer->rear = ((writer->rear + 1) % TASKS_SIZE);
  ++writer->count;
  writer->buffer_size = 0;
  pthread_cond_signal(&writer->cond);
  pthread_mutex_unlock(&writer->mutex);
}

static void nfdwriter_netlog_free(void) {
  nfdwriter_stop(nfdwriter_netlog);
  nfdwriter_free(nfdwriter_netlog);
  nfdwriter_netlog = NULL;
}

void nfdwriter_netlog_init(const char *addr, Ushort port) {
  if (!nfdwriter_netlog) {
    int fd = opennetfd(addr, port);
    if (fd < 0) {
      return;
    }
    nfdwriter_netlog = nfdwriter_create(fd);
    atexit(nfdwriter_netlog_free);
  }
}
