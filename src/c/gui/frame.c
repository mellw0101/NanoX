/** @file gui/frame.c

  @author  Melwin Svensson.
  @date    14-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


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
/* If we are currently polling for the correct framerate. */
static bool should_poll = FALSE;
/* First `nano-second` sample frame used to poll for framerate using vsync. */
static Llong frame_sample_0 = -1;
/* Second `nano-second` sample frame used to poll for framerate using vsync. */
static Llong frame_sample_1 = -1;


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Frame start ----------------------------- */

/* The start of the frame.  */
void frame_start(void) {
  clock_gettime(CLOCK_MONOTONIC, &t0);
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
  expected_frametime = FRAME_SWAP_RATE_TIME_NS_INT(framerate);
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
      frame_sample_0 = frame_get_time_ns();
    }
    else if (frame_sample_1 < 0) {
      frame_sample_1 = frame_get_time_ns();
    }
    else if (LLABS(frame_sample_0 - frame_sample_1) > 200000) {
      frame_sample_0 = -1;
      frame_sample_1 = -1;
    }
    else {
      should_poll = FALSE;
      frame_set_rate(monitor_closest_refresh_rate(FRAME_SWAP_RATE_TIME_NS_INT((frame_sample_0 + frame_sample_1) / 2.0)));
      glfwSwapInterval(0);
      return FALSE;
    }
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
  /* Set the framerate to an extramly high value, so that vsync actualy kicks in. */
  frame_set_rate(2400);
  glfwSwapInterval(1);
}

/* ----------------------------- Frame elapsed ----------------------------- */

/* Returns the total number of currently elapsed frames. */
Ulong frame_elapsed(void) {
  return elapsed_frames;
}
