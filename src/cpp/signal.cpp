#include "../include/prototypes.h"

void send_SIGUSR1_to_main_thread(void) {
  pthread_kill(main_thread->thread, SIGUSR1);
}

/* This function is called by the main thread when it recieves a signal. */
void handle_main_thread_signal(int sig, siginfo_t *si, void *context) {
  if (sig >= SIGRTMIN && sig <= SIGRTMAX) {
    signal_payload_t *payload = (signal_payload_t *)si->si_value.sival_ptr;
    if (payload && payload->func) {
      payload->func(payload->arg);
    }
    free(payload);
  }
}

/* Send a function and arg to the main thread for direct handeling. */
void send_signal_to_main_thread(callback_functionptr_t func, void *arg) {
  signal_payload_t *payload = (signal_payload_t *)nmalloc(sizeof(signal_payload_t));
  payload->func             = func;
  payload->arg              = arg;
  union sigval sig_data;
  sig_data.sival_ptr = payload;
  if (sigqueue(main_thread->pid, SIGRTMIN, sig_data) != 0) {
    switch (errno) {
      case EAGAIN :
      {
        LOUT_logE("The signal could not be queued due to lack of memory.");
        break;
      }
      case EINVAL :
      {
        LOUT_logE("An invalid signal was specified.");
        break;
      }
      case ESRCH :
      {
        LOUT_logE("The thread does not exist.");
        break;
      }
      default :
      {
        LOUT_logE("sigqueue failed with errno: %d.", errno);
      }
    }
  }
}

/* Init the main thread, then setup SIGRTMIN. */
void init_main_thread(void) {
  if (main_thread != NULL) {
    logE("main_thread should only be allocated once.");
    return;
  }
  main_thread         = (main_thread_t *)nmalloc(sizeof(main_thread_t));
  main_thread->thread = pthread_self();
  main_thread->pid    = getpid();
  struct sigaction sa;
  sa.sa_flags     = SA_SIGINFO;
  sa.sa_sigaction = handle_main_thread_signal;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGRTMIN, &sa, NULL);
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGRTMIN);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

/* Free the ptr to the main thread. */
void cleanup_main_thread(void) {
  (main_thread) ? free(main_thread) : void();
}

/* Setup 'SIGUSR1' and 'SIGUSR2' for subtread. to be used for pause then resume. */
void setup_signal_handler_on_sub_thread(void (*handler)(int)) {
  struct sigaction sig_act;
  sig_act.sa_handler = handler;
  sigemptyset(&sig_act.sa_mask);
  sig_act.sa_flags = 0;
  if (sigaction(SIGUSR1, &sig_act, NULL) != 0 || sigaction(SIGUSR2, &sig_act, NULL) != 0) {
    switch (errno) {
      case EINVAL :
      {
        logE("Invalid signal or invalid signal handler flags.");
        break;
      }
      case EFAULT :
      {
        logE("Invalid memory address provided for the action.");
        break;
      }
      case EPERM :
      {
        logE("Insufficient premission to change signal action.");
        break;
      }
      default :
      {
        logE("Unknown error (errno: %d).", errno);
      }
    }
  }
  /* First block the resume (SIGUSR2) signal. */
  block_pthread_sig(SIGUSR2, TRUE);
  /* Then make sure pause (SIGUSR1) signal is unblocked. */
  block_pthread_sig(SIGUSR1, FALSE);
}

void block_pthread_sig(int sig, bool block) {
  sigset_t mask;
  /* Apply mask to calling thread. */
  sigemptyset(&mask);
  sigaddset(&mask, sig);
  if (pthread_sigmask((block) ? SIG_BLOCK : SIG_UNBLOCK, &mask, NULL) != 0) {
    logE("Failed to %s.", (block) ? "SIG_BLOCK" : "SIG_UNBLOCK");
    return;
  }
  /* Then check if we were successfull, by getting the current mask. */
  sigemptyset(&mask);
  if (pthread_sigmask(SIG_SETMASK, NULL, &mask) != 0) {
    logE("Failed to get current mask.");
    return;
  }
  if (sigismember(&mask, sig) == (block) ? 0 : 1) {
    logE("Failed to %s.", (block) ? "SIG_BLOCK" : "SIG_UNBLOCK");
  }
}
