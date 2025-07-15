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

#define SEGMENT_INTERVAL0_MS  (1.0)
#define SEGMENT_INTERVAL0_NS  (1000000ULL) /* MILLI_TO_NANO(SEGMENT_INTERVAL0_MS) */
#define SEGMENT_INTERVAL1_MS  (0.3)
#define SEGMENT_INTERVAL1_NS  (300000ULL) /* MILLI_TO_NANO(SEGMENT_INTERVAL1_MS) */
#define SEGMENT_INTERVAL2_MS  (0.1)
#define SEGMENT_INTERVAL2_NS  (100000ULL) /* MILLI_TO_NANO(SEGMENT_INTERVAL2_MS) */

#define SEGMENT_0_JITTER_NS  (300000ULL)
#define SEGMENT_1_JITTER_NS  (150000ULL)
#define SEGMENT_2_JITTER_NS  (60000ULL)

#define DO_SLEEP_SEGMENT(elapsed, total, start, now, stage)                             \
  while (((elapsed) + SEGMENT_INTERVAL##stage##_NS + /* MILLI_TO_NANO(0.05) */ SEGMENT_##stage##_JITTER_NS) < (total)) {  \
    nanosleep(&sleeptime##stage, NULL);                                                 \
    clock_gettime(CLOCK_MONOTONIC, (now));                                              \
    (elapsed) = ELAPSED_NANOSEC((start), (now));                                        \
  }


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
/* Sleep segment 0 timespec. */
static struct timespec sleeptime0 = {0, SEGMENT_INTERVAL0_NS};
/* Sleep segment 1 timespec. */
static struct timespec sleeptime1 = {0, SEGMENT_INTERVAL1_NS};
/* Sleep segment 2 timespec. */
static struct timespec sleeptime2 = {0, SEGMENT_INTERVAL2_NS};


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

/* Perform segmented sleep to reach the `fullnano` duration ensuring maximum timing accuracy. */
static inline void frame_sleep_segmented(const struct timespec *const s, struct timespec *const n, Ulong fullnano) {
  long elapsed_nano;
  /* Get the current time. */
  clock_gettime(CLOCK_MONOTONIC, n);
  elapsed_nano = ELAPSED_NANOSEC(s, n);
  /* Perform all sleep stages. */
  DO_SLEEP_SEGMENT(elapsed_nano, fullnano, s, n, 0);
  DO_SLEEP_SEGMENT(elapsed_nano, fullnano, s, n, 1);
  DO_SLEEP_SEGMENT(elapsed_nano, fullnano, s, n, 2);
  /* For the last 0.1 ms, we busy wait. */
  while (ELAPSED_NANOSEC(s, n) < fullnano) {
    clock_gettime(CLOCK_MONOTONIC, n);
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Nsleep ----------------------------- */

/* High-accuracy `nano-second` sleep. */
void nsleep(Ulong ns) {
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  frame_sleep_segmented(&start, &end, ns);
}

/* ----------------------------- Msleep ----------------------------- */

/* High-accuracy whole `milli-second` sleep. */
void msleep(Ulong ms) {
  nsleep(MILLI_TO_NANO(ms));
}

/* ----------------------------- Frame start ----------------------------- */

/* The start of the frame.  */
void frame_start(void) {
  clock_gettime(CLOCK_MONOTONIC, &t0);
}

/* ----------------------------- Frame end ----------------------------- */

/* The end of the frame. */
void frame_end(void) {
  clock_gettime(CLOCK_MONOTONIC, &t1);
  frametime_ns = ELAPSED_NANOSEC(&t0, &t1);
  /* If less time has passed then a full frame, we sleep the remaining time away. */
  if (frametime_ns < expected_frametime_ns) {
    frame_sleep_segmented(&t0, &t1, expected_frametime_ns);
  }
  /* Calculate the total frametime after we sleept. */
  frametime_ms = ELAPSED_MILLISEC(&t0, &t1);
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
