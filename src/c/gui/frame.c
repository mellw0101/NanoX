/** @file gui/frametime.c

  @author  Melwin Svensson.
  @date    14-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ELAPSED_SEC(s, e)       (((e)->tv_sec - (s)->tv_sec) + ((double)((e)->tv_nsec - (s)->tv_nsec) / 1e9))
#define ELAPSED_MILLISEC(s, e)  ((((e)->tv_sec - (s)->tv_sec) * 1e3) + ((double)((e)->tv_nsec - (s)->tv_nsec) / 1e6))
#define ELAPSED_NANOSEC(s, e)   ((((e)->tv_sec - (s)->tv_sec) * 1000000000ULL) + ((e)->tv_nsec - (s)->tv_nsec))

#define MILLI_TO_NANO(x)  ((Ulong)((x) * 1000000ULL))
#define NANO_TO_MILLI(x)  ((double)(x) / 1000000.0)

#define SEGMENT_INTERVAL_MS  (0.1)
#define SEGMENT_INTERVAL_NS  MILLI_TO_NANO(SEGMENT_INTERVAL_MS)

/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The current running framerate. */
static double framerate = 60.0;
/* The total time that passed during the current frame. */
static double frametime = 0.0;
/* Total number of elapsed frames. */
static Ulong elapsed_frames = 0;
/* Frame start time. */
static struct timespec t0;
/* Frame end time. */ 
static struct timespec t1;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


_UNUSED
static inline void elapsed_time(const struct timespec *const s, const struct timespec *const e, struct timespec *const ret) {
  /* Save the diffrence to ret. */
  ret->tv_sec  = (e->tv_sec - s->tv_sec);
  ret->tv_nsec = (e->tv_nsec - s->tv_nsec);
  /* Correct the values. */
  if (ret->tv_nsec < 0) {
    ret->tv_sec  -= 1;
    ret->tv_nsec += 1000000000L;
  }
}

_UNUSED
static inline void mssleep(double ms) {
  long nano = MILLI_TO_NANO(ms); 
  struct timespec ns;
  ns.tv_sec  = (nano / 1000000000ULL);
  ns.tv_nsec = (nano - (ns.tv_sec * 1000000000ULL));
  nanosleep(&ns, NULL);
}

_UNUSED
static inline void mssleep_segmented(const struct timespec *const s, struct timespec *const n, Ulong fullnano) {
  long elapsed_nano;
  struct timespec sleeptime;
  sleeptime.tv_sec  = 0;
  sleeptime.tv_nsec = SEGMENT_INTERVAL_NS;
  /* Get the current time. */
  clock_gettime(CLOCK_MONOTONIC, n);
  elapsed_nano = ELAPSED_NANOSEC(s, n);
  /* Loop until there is one segment left. */
  while ((elapsed_nano + SEGMENT_INTERVAL_NS) < fullnano) {
    nanosleep(&sleeptime, NULL);
    clock_gettime(CLOCK_MONOTONIC, n);
    elapsed_nano = ELAPSED_NANOSEC(s, n);
  }
  /* For the last 0.1 ms, we busy wait. */
  while (ELAPSED_NANOSEC(s, n) < fullnano) {
    clock_gettime(CLOCK_MONOTONIC, n);
  }
}


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
  frametime = ELAPSED_MILLISEC(&t0, &t1);
  /* If less time has passed then a full frame, we sleep the remaining time away. */
  if (frametime < (1000.0 / framerate)) {
    mssleep_segmented(&t0, &t1, MILLI_TO_NANO((1000.0 / framerate)));
    // mssleep((1000 / framerate) - frametime);
    /* Calculate the total frametime after we sleept. */
    // clock_gettime(CLOCK_MONOTONIC, &t1);
    frametime = ELAPSED_MILLISEC(&t0, &t1);
  }
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
}

/* ----------------------------- Frame get time ----------------------------- */

/* Return the frametime of the previous frame. */
double frame_get_time(void) {
  return frametime;
}
