#include "../include/c_proto.h"


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct Event {
  EVENT_CB callback;
  void    *arg;
};


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static thread_t main_thread;
static pid_t    pid;
static mutex_t  mut   = mutex_init_static;
static QUEUE    queue = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static void main_thread_sig_handler(int sig, siginfo_t *si, void *_UNUSED context) {
  EVENT ev;
  if (sig >= SIGRTMIN && sig <= SIGRTMAX && (ev = si->si_value.sival_ptr)) {
    CALL_IF_VALID(ev->callback, ev->arg);
    free(ev);
  }
}

/* This must be called from the main thread. */
static void init_main_thread(void) {
  struct sigaction sa;
  sigset_t set;
  main_thread     = pthread_self();
  pid             = getpid();
  sa.sa_flags     = SA_SIGINFO;
  sa.sa_sigaction = main_thread_sig_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGRTMIN, &sa, NULL);
  sigemptyset(&set);
  sigaddset(&set, SIGRTMIN);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* This must be called on the main thread. */
void event_init(void) {
  queue = queue_create();
  init_main_thread();
}

void event_free(void) {
  queue_free(queue);
}

void event_process(void) {
  EVENT ev;
  mutex_lock(&mut);
  while (queue_size(queue)) {
    ev = queue_pop_front(queue);
    mutex_unlock(&mut);
    ev->callback(ev->arg);
    free(ev);
    mutex_lock(&mut);
  }
  mutex_unlock(&mut);
}

void event_enqueue(EVENT_CB callback, void *arg) {
  ASSERT(callback);
  EVENT ev = xmalloc(sizeof(*ev));
  ev->callback = callback;
  ev->arg      = arg;
  MUTEX_ACTION(&mut,
    queue_push(queue, ev);
  );
}

/* Note that this uses signals to interupt the main thread. */
void event_enqueue_on_main_thread(EVENT_CB callback, void *arg) {
  ASSERT(callback);
  union sigval data;
  EVENT ev = xmalloc(sizeof(*ev));
  ev->callback   = callback;
  ev->arg        = arg;
  data.sival_ptr = ev;
  if (sigqueue(pid, SIGRTMIN, data) != 0) {
    log_ERR_FA("Failed to enqueue event on main thread: %s", strerror(errno));
  }
}
