/** @file gui/mouse.c

  @author  Melwin Svensson.
  @date    13-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define MOUSE_FLAGS(flag)     mouse_flags[((flag) / sizeof(__TYPE(mouse_flags[0])) * 8)]
#define MOUSE_FLAGMASK(flag)  ((__TYPE(mouse_flags[0]))1 << ((flag) % (sizeof(__TYPE(mouse_flags[0])) * 8)))
#define MOUSE_SET(flag)       MOUSE_FLAGS(flag) |= MOUSE_FLAGMASK(flag)
#define MOUSE_UNSET(flag)     MOUSE_FLAGS(flag) &= ~MOUSE_FLAGMASK(flag)
#define MOUSE_ISSET(flag)     (MOUSE_FLAGS(flag) & MOUSE_FLAGMASK(flag))
#define MOUSE_TOGGLE(flag)    MOUSE_FLAGS(flag) ^= MOUSE_FLAGMASK(flag)

#define DOUBLE_CLICK_THRESHOLD  MILLI_TO_NANO(200)

#define MOUSE_LOCK_READ(...)   RWLOCK_RDLOCK_ACTION(&rwlock, __VA_ARGS__)
#define MOUSE_LOCK_WRITE(...)  RWLOCK_WRLOCK_ACTION(&rwlock, __VA_ARGS__)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* TODO: Fully move this to use our frametimes to measure time, as we cannot trust glfw timekeeping, as it states it does not atomicly set the time,
 * and also the fact that it uses floating point time, this means it can never be equvilent, or even close to as accurate as our frame pacing.
 * This is now done.*/
// static double last_mouse_click_time = 0;
static int    last_mouse_button     = 0;
/* Flags for the mouse, to indecate the current state of the mouse. */
static Uint mouse_flags[1];
/* The current x position of the mouse. */
static double mouse_xpos = 0;
/* The current y position of the mouse. */
static double mouse_ypos = 0;
/* The x position the mouse had before the last update. */
static double last_mouse_xpos = 0;
/* The y position the mouse had before the last update. */
static double last_mouse_ypos = 0;

static Llong mouse_last_click_time = 0;

static rwlock_t rwlock;


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Mouse gui init ----------------------------- */

void mouse_gui_init(void) {
  RWLOCK_INIT(&rwlock, NULL);
}

/* ----------------------------- Mouse gui free ----------------------------- */

void mouse_gui_free(void) {
  RWLOCK_DESTROY(&rwlock);
}

/* ----------------------------- Mouse gui update state ----------------------------- */

/* Update the mouse click and flag state. */
void mouse_gui_update_state(int action, int button) {
  MOUSE_LOCK_WRITE(
    /* Press */
    if (action == GLFW_PRESS) {
      /* Give some wiggle room for the position of a repeated mouse click. */
      if (button == last_mouse_button && (frame_elapsed_time() - mouse_last_click_time) < DOUBLE_CLICK_THRESHOLD
      && mouse_xpos > (last_mouse_xpos - 3) && mouse_xpos < (last_mouse_xpos + 3)
      && mouse_ypos > (last_mouse_ypos - 3) && mouse_ypos < (last_mouse_ypos + 3))
      {
        /* First check for a tripple click. */
        if (MOUSE_ISSET(MOUSE_PRESS_WAS_DOUBLE) && !MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE)) {
          MOUSE_SET(MOUSE_PRESS_WAS_TRIPPLE);
        }
        else {
          MOUSE_UNSET(MOUSE_PRESS_WAS_TRIPPLE);
        }
        /* Then check for a double click. */
        if (!MOUSE_ISSET(MOUSE_PRESS_WAS_DOUBLE) && !MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE)) {
          MOUSE_SET(MOUSE_PRESS_WAS_DOUBLE);
        }
        else {
          MOUSE_UNSET(MOUSE_PRESS_WAS_DOUBLE);
        }
      }
      /* Otherwise, unset both states. */
      else {
        MOUSE_UNSET(MOUSE_PRESS_WAS_DOUBLE);
        MOUSE_UNSET(MOUSE_PRESS_WAS_TRIPPLE);
      }
      /* If the left mouse button was pressed, set it as held. */
      if (button == GLFW_MOUSE_BUTTON_1) {
        MOUSE_SET(MOUSE_BUTTON_HELD_LEFT);
      }
      /* If the right mouse button was pressed, set it as held. */
      else if (button == GLFW_MOUSE_BUTTON_2) {
        MOUSE_SET(MOUSE_BUTTON_HELD_RIGHT);
      }
      /* If we just had a tripple click, ensure the next click will not be detected as a double click. */
      // last_mouse_click_time = (MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE) ? 0 : glfwGetTime());
      mouse_last_click_time = (MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE) ? 0 : frame_elapsed_time());
      last_mouse_button     = button;
      last_mouse_xpos = mouse_xpos;
      last_mouse_ypos = mouse_ypos;
    }
    /* Unpress */
    else if (action == GLFW_RELEASE) {
      /* If the left mouse button was released, unset it as held. */
      if (button == GLFW_MOUSE_BUTTON_1) {
        MOUSE_UNSET(MOUSE_BUTTON_HELD_LEFT);
      }
      /* If the right mouse button was released, unset it as held. */
      else if (button == GLFW_MOUSE_BUTTON_2) {
        MOUSE_UNSET(MOUSE_BUTTON_HELD_RIGHT);
      }
    }
  );
}

/* ----------------------------- Mouse gui update pos ----------------------------- */

/* Update the mouse position state, ensuring we save the last position. */
void mouse_gui_update_pos(double x, double y) {
  MOUSE_LOCK_WRITE(
    /* Save the current mouse position. */
    last_mouse_xpos = mouse_xpos;
    last_mouse_ypos = mouse_ypos;
    /* Then update the current mouse position. */
    mouse_xpos = x;
    mouse_ypos = y;
  );
}

/* ----------------------------- Mouse gui get x ----------------------------- */

/* The only way to get the mouse x position. */
double mouse_gui_get_x(void) {
  double ret;
  MOUSE_LOCK_READ(
    ret = mouse_xpos;
  );
  return ret;
}

/* ----------------------------- Mouse gui get y ----------------------------- */

/* The only way to get the mouse y position. */
double mouse_gui_get_y(void) {
  double ret;
  MOUSE_LOCK_READ(
    ret = mouse_ypos;
  );
  return ret;
}

/* ----------------------------- Mouse gui get pos ----------------------------- */

/* Get both mouse positions at once. */
void mouse_gui_get_pos(double *const x, double *const y) {
  ASSERT(x);
  ASSERT(y);
  MOUSE_LOCK_READ(
    *x = mouse_xpos;
    *y = mouse_ypos;
  );
}

/* ----------------------------- Mouse gui get last x ----------------------------- */

double mouse_gui_get_last_x(void) {
  double ret;
  MOUSE_LOCK_READ(
    ret = last_mouse_xpos;
  );
  return ret;
}

/* ----------------------------- Mouse gui get last y ----------------------------- */

double mouse_gui_get_last_y(void) {
  double ret;
  MOUSE_LOCK_READ(
    ret = last_mouse_ypos;
  );
  return ret;
}

/* ----------------------------- Mouse gui is flag set ----------------------------- */

/* Check if a mouse flag is set. */
bool mouse_gui_is_flag_set(Uint flag) {
  bool ret;
  MOUSE_LOCK_READ(
    ret = MOUSE_ISSET(flag);
  );
  return ret;
}

/* ----------------------------- Mouse gui clear flags ----------------------------- */

void mouse_gui_clear_flags(void) {
  MOUSE_LOCK_WRITE(
    memset(mouse_flags, 0, sizeof(mouse_flags));
  );
}
