/** @file gui/frame.c

  @author  Melwin Svensson.
  @date    14-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define FRAME_POLL_INTERVAL_SECONDS    (10)
#define FRAME_POLL_INTERVAL_FRAMES(x)  ((x) * FRAME_POLL_INTERVAL_SECONDS)

#define FRAME_SAMPLE_TOLERANCE  (200000)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* If we are currently polling for the correct framerate. */
static bool should_poll = FALSE;
/* If we should log the frame time. */
static bool should_log = FALSE;
/* The current running framerate. */
static int framerate = 60;
/* The total expected frametime in `nano-seconds`. */
static Llong expected_frametime = FRAME_SWAP_RATE_TIME_NS_INT(60);
/* The total time that passed during the previous frame in `nano-seconds`. */
static Llong frametime = 0;
/* Total number of elapsed frames. */
static Ulong elapsed_frames = 0;
/* Frame start time. */
static struct timespec t0;
/* Frame end time. */ 
static struct timespec t1;
/* First `nano-second` sample frame used to poll for framerate using vsync. */
static Llong frame_sample_0 = -1;
/* Second `nano-second` sample frame used to poll for framerate using vsync. */
static Llong frame_sample_1 = -1;
/* The total elapsed frames when the last polling was finished. */
static Ulong last_poll = 0;
/* The interval in frames between polling, based on the set framerate */
static Ulong poll_interval_frames = FRAME_POLL_INTERVAL_FRAMES(60);


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Frame samples within tolerance ----------------------------- */

/* Make sure both acuired samples are within the sample tolerance of any actual monitor. */
_NODISCARD
static inline bool frame_samples_within_tolerance(void) {
  int count;
  int *rates;
  if (LLABS(frame_sample_0 - frame_sample_1) > FRAME_SAMPLE_TOLERANCE) {
    return FALSE;
  }
  rates = monitor_refresh_rate_array(&count);
  for (int i=0; i<count; ++i) {
    if (LLABS(frame_sample_0 - FRAME_SWAP_RATE_TIME_NS_INT(rates[i])) < FRAME_SAMPLE_TOLERANCE
    &&  LLABS(frame_sample_1 - FRAME_SWAP_RATE_TIME_NS_INT(rates[i])) < FRAME_SAMPLE_TOLERANCE)
    {
      return TRUE;
    }
  }
  return FALSE;
}

/* ----------------------------- Frame log time ----------------------------- */

/* Print the time the last elapsed frame took from start to finish. */
_UNUSED
static inline void frame_log_time(void) {
  log_INFO_0("Frame: %lu: %.4f ms", elapsed_frames, NANO_TO_MILLI(frametime));
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Frame start ----------------------------- */

/* The start of the frame.  Note that this should only ever run from the main loop (in the main thread). */
void frame_start(void) {
  clock_gettime(CLOCK_MONOTONIC, &t0);
  if (should_poll || should_log) {
    frame_log_time();
  }
}

/* ----------------------------- Frame end ----------------------------- */

/* The end of the frame. */
void frame_end(void) {
  clock_gettime(CLOCK_MONOTONIC, &t1);
  frametime = TIMESPEC_ELAPSED_NS(&t0, &t1);
  /* If less time has passed then a full frame, we sleep the remaining time away. */
  if (frametime < expected_frametime) {
    hiactime_sleep_total_duration(&t0, &t1, expected_frametime);
    /* Calculate the total frametime after we sleept. */
    frametime = TIMESPEC_ELAPSED_NS(&t0, &t1);
  }
  /* Incrament the total elapsed frames. */
  ++elapsed_frames;
}

/* ----------------------------- Frame get rate ----------------------------- */

/* Get the current framerate. */
int frame_get_rate(void) {
  return framerate;
}

/* ----------------------------- Frame set rate ----------------------------- */

/* Set the framerate. */
void frame_set_rate(int x) {
  framerate = x;
  expected_frametime   = FRAME_SWAP_RATE_TIME_NS_INT(x);
  poll_interval_frames = FRAME_POLL_INTERVAL_FRAMES(x);
}

/* ----------------------------- Frame get time ms ----------------------------- */

/* Return the frametime of the previous frame in `milli-seconds`. */
double frame_get_time_ms(void) {
  return NANO_TO_MILLI(frametime);
}

/* ----------------------------- Frame get time ns ----------------------------- */

/* Returns the frame-time of the previous frame in `nano-seconds`.  */
Llong frame_get_time_ns(void) {
  return frametime;
}

/* ----------------------------- Frame should poll ----------------------------- */

/* Only performs the frame polling logic when `should_poll` is
 * `TRUE` and then also returns `TRUE` so that we redraw each frame. */
bool frame_should_poll(void) {
  /* When we are in  */
  if (should_poll) {
    if (frame_sample_0 < 0) {
      frame_sample_0 = frametime;
    }
    else if (frame_sample_1 < 0) {
      frame_sample_1 = frametime;
    }
    else if (!frame_samples_within_tolerance()) {
      frame_sample_0 = -1;
      frame_sample_1 = -1;
    }
    else {
      should_poll = FALSE;
      last_poll   = elapsed_frames;
      frame_set_rate(monitor_closest_refresh_rate(FRAME_SWAP_RATE_TIME_NS_INT((frame_sample_0 + frame_sample_1) / 2.0)));
      log_INFO_1("Re-polling ended: Fps: %d", FRAME_SWAP_RATE_TIME_NS_INT(expected_frametime));
      glfwSwapInterval(0);
      return FALSE;
    }
    return TRUE;
  }
  else if ((elapsed_frames - last_poll) > poll_interval_frames) {
    log_INFO_1("Re-polling started: Fps: %d", FRAME_SWAP_RATE_TIME_NS_INT(expected_frametime));
    frame_set_poll();
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/* ----------------------------- Frame set poll ----------------------------- */

/* Set all preconditions to start the current frame rate polling progress. */
void frame_set_poll(void) {
  should_poll = TRUE;
  frame_sample_0 = -1;
  frame_sample_1 = -1;
  /* Set the frame rate to 4 times the fastest monitor, so we know for a fact vsync will kick in. */
  frame_set_rate(monitor_fastest_refresh_rate() * 4);
  glfwSwapInterval(1);
}

/* ----------------------------- Frame elapsed ----------------------------- */

/* Returns the total number of currently elapsed frames. */
Ulong frame_elapsed(void) {
  return elapsed_frames;
}

/* ----------------------------- Frame should report ----------------------------- */

void frame_should_report(bool print_times) {
  ATOMIC_STORE(should_log, print_times);
}
