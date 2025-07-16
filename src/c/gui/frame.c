/** @file gui/frametime.c

  @author  Melwin Svensson.
  @date    14-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The current running framerate. */
static double framerate = 60.0;
/* The total expected frametime in milliseconds. */
static Ulong expected_frametime_ms = (1000.0 / 60.0);
/* The total expected frametime in nanoseconds. */
static Ulong expected_frametime_ns = MILLI_TO_NANO(1000.0 / 60.0);
/* The total time that passed during the current frame in `milli-seconds`. */
static double frametime_ms = 0.0;
/* The total time that passed during the current frame in `nano-seconds`. */
static Ulong  frametime_ns = 0;
/* Total number of elapsed frames. */
static Ulong elapsed_frames = 0;
/* Frame start time. */
static struct timespec t0;
/* Frame end time. */ 
static struct timespec t1;
/* If we are currently polling for the correct framerate. */
static bool should_poll;
static double frame_sample_0 = -1;
static double frame_sample_1 = -1;


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
  frametime_ms = TIMESPEC_ELAPSED_MS(&t0, &t1);
  /* Incrament the total elapsed frames. */
  ++elapsed_frames;
}

/* ----------------------------- Frame get rate ----------------------------- */

/* Get the current framerate. */
double frame_get_rate(void) {
  return framerate;
}

/* ----------------------------- Frame set rate ----------------------------- */

/* Set the framerate. */
void frame_set_rate(double x) {
  framerate = x;
  expected_frametime_ms = (1000.0 / framerate);
  expected_frametime_ns = MILLI_TO_NANO(1000.0 / framerate);
}

/* ----------------------------- Frame get time ----------------------------- */

/* Return the frametime of the previous frame. */
double frame_get_time(void) {
  return frametime_ms;
}

/* ----------------------------- Frame poll rate ----------------------------- */

/* Here we try to be smart as there is not really another way to probe
 * for the current monitor, and all we want is to know the refresh-rate. */
void frame_poll_rate(void) {
  double f0 = -1;
  double f1 = -1;
  /* Set a way to high framerate so we can determine the actual rate. */
  frame_set_rate(1000);
  glfwSwapInterval(1);
  while (1) {
    frame_start();
    // writef("%.4f ms\n", frame_get_time());
    if (f0 < 0) {
      f0 = frame_get_time();
    }
    else if (f1 < 0) {
      f1 = frame_get_time();
    }
    else if (!(f0 >= (f1 - 0.2) && f0 <= (f1 + 0.2))) {
      f0 = -1;
      f1 = -1;
    }
    else {
      frame_set_rate((int)(1000 / ((f0 + f1) / 2)));
      break;
    }
    glfwSwapBuffers(gui_window);
    frame_end();
  }
  glfwSwapInterval(0);
}

/* ----------------------------- Frame should poll ----------------------------- */

bool frame_should_poll(void) {
  if (should_poll) {
    if (frame_sample_0 < 0) {
      frame_sample_0 = frame_get_time();
    }
    else if (frame_sample_1 < 0) {
      frame_sample_1 = frame_get_time();
    }
    else if (!(frame_sample_0 >= (frame_sample_1 - 0.2) && frame_sample_0 <= (frame_sample_1 + 0.2))) {
      frame_sample_0 = -1;
      frame_sample_1 = -1;
    }
    else {
      frame_set_rate((int)(1000.0 / ((frame_sample_0 + frame_sample_1) / 2.0)));
      glfwSwapInterval(0);
    }
    return TRUE;
  }
  else {
    return FALSE;
  }
}
