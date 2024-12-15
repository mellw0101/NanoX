#include "../include/prototypes.h"

task_queue_t          *task_queue        = NULL;
pthread_t             *threads           = NULL;
volatile sig_atomic_t *stop_thread_flags = NULL;

// This function should be used to camly kill a subthread after the next task it perform.
// When halting a thread is needed use 'stop_thread'.  Note that this kills whatever
// thread happens to run the next task, therefor this is intended for clean shoutdown.
void *terminate_work(void *) {
  return NULL;
}

// Either lock or unlock the threadpools mutex, with full error reporting.  We will
// not interviene on fail as I want to see for now atleast what it takes to crash.
void lock_pthread_mutex(pthread_mutex_t *mutex, bool lock) {
  int result = (lock == TRUE) ? pthread_mutex_lock(mutex) : pthread_mutex_unlock(mutex);
  if (result == 0) {
    return;
  }
  switch (result) {
    case EAGAIN : {
      logE("Mutex: 'task_queue->mutex' exceeded the maximum number of recursive locks.");
      break;
    }
    case EDEADLK : {
      logE("Mutex: 'task_queue->mutex' caused a deadlock or was already owned by this thread.");
      break;
    }
    case EINVAL : {
      logE("Mutex: 'task_queue->mutex' is not valid, indicating a fatal error.");
      break;
    }
    case EPERM : {
      logE("Mutex: 'task_queue->mutex' was locked by another thread, or not at all.");
      break;
    }
    default : {
      logE("Mutex: 'task_queue->mutex' encountered an unexpected error code: %d.", result);
    }
  }
}

/* This is a helper function to easily lock the threadpool mutex. */
__inline__ void lock_threadpool_mutex(bool lock) {
  lock_pthread_mutex(&task_queue->mutex, lock);
}

// Helper function to perform actions protected by the 'task_queue->mutex'.
// This function uses 'RAII' standard, this is done by making a struct that
// locks the mutex when created and unlocked when it goes out of scope.
template <typename Callback>
static __inline__ void under_threadpool_mutex(Callback &&callback) {
  pthread_mutex_guard_t guard(&task_queue->mutex);
  callback();
}

void sub_thread_signal_handler(int sig) {
  if (sig == SIGUSR1) {
    block_pthread_sig(SIGUSR1, TRUE);
    sigset_t mask;
    int      received_sig;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
    /* Wait for SIGUSR2 */
    block_pthread_sig(SIGUSR2, FALSE);
    if (sigwait(&mask, &received_sig) != 0) {
      pthread_t this_thread = pthread_self();
      Uchar     thread_id   = thread_id_from_pthread(&this_thread);
      logI("Thread: '%u'. Failed to resume execution.", thread_id);
    }
    block_pthread_sig(SIGUSR2, TRUE);
    block_pthread_sig(SIGUSR1, FALSE);
  }
}

void pause_sub_thread(bool pause, Uchar thread_id) {
  if (thread_id < MAX_THREADS) {
    // pthread_mutex_guard_t guard(&task_queue->mutex);
    pthread_kill(threads[thread_id], (pause) ? SIGUSR1 : SIGUSR2);
  }
}

void pause_all_sub_threads(bool pause) {
  for (Uchar i = 0; i < MAX_THREADS; i++) {
    pause_sub_thread(pause, i);
  }
}

/* This is where all sub-threads wait for work. */
void *worker_thread(void *arg) {
  /* Init  */
  setup_signal_handler_on_sub_thread(sub_thread_signal_handler);
  /* Assign 'thread_id' with the id passed via 'arg'. */
  Uchar thread_id = (Uchar)(uintptr_t)arg;
  while (1) {
    lock_threadpool_mutex(TRUE);
    /* Wait for a task to become avaliable. */
    while (!task_queue->count && !stop_thread_flags[thread_id]) {
      pthread_cond_wait(&task_queue->cond, &task_queue->mutex);
    }
    /* Check if stop flag is set for this thread. */
    if (stop_thread_flags[thread_id]) {
      lock_threadpool_mutex(FALSE);
      break;
    }
    /* Get a task from the queue. */
    task_t task = task_queue->tasks[task_queue->front];
    task_queue->front = (task_queue->front + 1) % QUEUE_SIZE;
    --task_queue->count;
    lock_threadpool_mutex(FALSE);
    /* Check if 'function' reciveved were the 'terminate_work' task. */
    if (task.function == terminate_work) {
      break;
    }
    /* Execute the task. */
    void *result = task.function(task.arg);
    /* If we want to be assign the result directly to a var. */
    if (task.result) {
      *(void **)task.result = result;
    }
    /* When we want a predefined function to run when this thread has finished
     * execution, we add the prefifined callback function to the callback queue. */
    if (task.callback) {
      enqueue_callback(task.callback, result);
    }
  }
  return NULL;
}

/* Init the threadpool and the queue.  As well as start all subthreds. */
void init_queue_task(void) {
  /* Init the queue. */
  if (task_queue) {
    return;
  }
  task_queue        = (task_queue_t *)nmalloc(sizeof(task_queue_t));
  task_queue->front = 0;
  task_queue->rear  = 0;
  task_queue->count = 0;
  pthread_mutex_init(&task_queue->mutex, NULL);
  pthread_cond_init(&task_queue->cond, NULL);
  /* Allocate memory for the 'threads'. */
  threads = (pthread_t *)nmalloc(MAX_THREADS * sizeof(pthread_t));
  /* As well as allocating space for the stop flags. */
  stop_thread_flags = (volatile sig_atomic_t *)nmalloc(MAX_THREADS * sizeof(sig_atomic_t));
  /* Create the threads, and start them running 'worker_thread'. */
  for (Uchar i = 0; i < MAX_THREADS; i++) {
    stop_thread_flags[i] = FALSE;
    if (pthread_create(&threads[i], NULL, worker_thread, (void *)(uintptr_t)i) != 0) {
      logE("Failed to create thread: '%u'.", i);
    }
  }
}

int task_queue_count(void) {
  PROFILE_FUNCTION;
  pthread_mutex_guard_t guard(&task_queue->mutex);
  int current_count = task_queue->count;
  return (current_count);
}

/* Cleanup the threadpool and join all threads. */
void shutdown_queue(void) {
  for (Uchar i = 0; i < MAX_THREADS; i++) {
    stop_thread(i);
    pthread_join(threads[i], NULL);
  }
  free(threads);
  free((void *)stop_thread_flags);
  pthread_mutex_destroy(&task_queue->mutex);
  free(task_queue);
}

/* Add a task to the threadpool queue for for the subthreads to perform. */
void submit_task(task_functionptr_t function, void *arg, void **result, callback_functionptr_t callback) {
  under_threadpool_mutex([function, arg, result, callback] {
    if (task_queue->count < QUEUE_SIZE) {
      task_queue->tasks[task_queue->rear].function = function;
      task_queue->tasks[task_queue->rear].arg      = arg;
      task_queue->tasks[task_queue->rear].result   = result;
      task_queue->tasks[task_queue->rear].callback = callback;
      task_queue->rear = (task_queue->rear + 1) % QUEUE_SIZE;
      task_queue->count++;
      /* Send a signal that there is a new task. */
      pthread_cond_signal(&task_queue->cond);
    }
  });
}

/* Stop a thread by 'thread_id'.  Note that this will halt execution on that thread. */
void stop_thread(Uchar thread_id) {
  if (thread_id < MAX_THREADS) {
    under_threadpool_mutex([thread_id] {
      stop_thread_flags[thread_id] = TRUE;
      pthread_cond_broadcast(&task_queue->cond);
    });
  }
}

Uchar thread_id_from_pthread(pthread_t *thread) {
  Uchar i = 0;
  for (; i < MAX_THREADS; i++) {
    if (pthread_equal(threads[i], *thread)) {
      break;
    }
  }
  return i;
}
