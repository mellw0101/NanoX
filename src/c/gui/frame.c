/** @file gui/frametime.c

  @author  Melwin Svensson.
  @date    14-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The current running framerate. */
static int framerate = 60;
/* The total expected frametime in nanoseconds. */
static Ulong expected_frametime_ns = MILLI_TO_NANO(1000.0 / 60.0);
/* The total time that passed during the current frame in `nano-seconds`. */
static Ulong frametime_ns = 0;
/* Total number of elapsed frames. */
static Ulong elapsed_frames = 0;
/* Frame start time. */
static struct timespec t0;
/* Frame end time. */ 
static struct timespec t1;
/* If we are currently polling for the correct framerate. */
static bool should_poll = FALSE;
/* First sample of a frame used to poll for current framerate. */
// static double frame_sample_0_ms = -1;
/* Second sample of a frame used to poll for current framerate. */
// static double frame_sample_1_ms = -1;
/* First sample of a frame used to poll for current framerate. */
static Llong frame_sample_0_ns = -1;
/* Second sample of a frame used to poll for current framerate. */
static Llong frame_sample_1_ns = -1;

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
  frametime_ns = TIMESPEC_ELAPSED_NS(&t0, &t1);
  /* If less time has passed then a full frame, we sleep the remaining time away. */
  if (frametime_ns < expected_frametime_ns) {
    hiactime_sleep_total_duration(&t0, &t1, expected_frametime_ns);
  }
  /* Calculate the total frametime after we sleept. */
  frametime_ns = TIMESPEC_ELAPSED_NS(&t0, &t1);
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
  expected_frametime_ns = MILLI_TO_NANO(1000.0 / (double)framerate);
}

/* ----------------------------- Frame get time ms ----------------------------- */

/* Return the frametime of the previous frame in `milli-seconds`. */
double frame_get_time_ms(void) {
  return NANO_TO_MILLI(frametime_ns);
}

/* ----------------------------- Frame get time ns ----------------------------- */

/* Returns the frame-time of the previous frame in `nano-seconds`.  */
Ulong frame_get_time_ns(void) {
  return frametime_ns;
}

/* ----------------------------- Frame should poll ----------------------------- */

/* Only performs the frame polling logic when `should_poll` is
 * `TRUE` and then also returns `TRUE` so that we redraw each frame. */
// bool frame_should_poll(void) {
//   if (should_poll) {
//     if (frame_sample_0_ms < 0) {
//       frame_sample_0_ms = frame_get_time_ms();
//     }
//     else if (frame_sample_1_ms < 0) {
//       frame_sample_1_ms = frame_get_time_ms();
//     }
//     else if (!(frame_sample_0_ms >= (frame_sample_1_ms - 0.2) && frame_sample_0_ms <= (frame_sample_1_ms + 0.2))) {
//       frame_sample_0_ms = -1;
//       frame_sample_1_ms = -1;
//     }
//     else {
//       should_poll = FALSE;
//       frame_set_rate(monitor_closest_refresh_rate((int)(1000.0 / ((frame_sample_0_ms + frame_sample_1_ms) / 2.0))));
//       glfwSwapInterval(0);
//     }
//     return TRUE;
//   }
//   else {
//     return FALSE;
//   }
// }

/* ----------------------------- Frame should poll ----------------------------- */

/* Only performs the frame polling logic when `should_poll` is
 * `TRUE` and then also returns `TRUE` so that we redraw each frame. */
bool frame_should_poll(void) {
  if (should_poll) {
    if (frame_sample_0_ns < 0) {
      frame_sample_0_ns = frame_get_time_ns();
    }
    else if (frame_sample_1_ns < 0) {
      frame_sample_1_ns = frame_get_time_ns();
    }
    else if (llabs(frame_sample_0_ns - frame_sample_1_ns) > 200000) {
      frame_sample_0_ns = -1;
      frame_sample_1_ns = -1;
    }
    else {
      should_poll = FALSE;
      frame_set_rate(monitor_closest_refresh_rate((int)FRAMERATE_FROM_NS((frame_sample_0_ns + frame_sample_1_ns) / 2.0)));
      glfwSwapInterval(0);
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
  frame_sample_0_ns = -1;
  frame_sample_1_ns = -1;
  /* Set the framerate to an extramly high value, so that vsync actualy kicks in. */
  frame_set_rate(2400);
  glfwSwapInterval(1);
}

/* ----------------------------- Frame elapsed ----------------------------- */

/* Returns the total number of currently elapsed frames. */
Ulong frame_elapsed(void) {
  return elapsed_frames;
}
