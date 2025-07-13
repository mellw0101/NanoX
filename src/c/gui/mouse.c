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


#define DOUBLE_CLICK_THRESHOLD  (0.2)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static double last_mouse_click_time = 0;
static int    last_mouse_button     = 0;
/* Flags for the mouse, to indecate the current state of the mouse. */
static Uint mouse_flags[1];
/* The current x position of the mouse. */
static float mouse_xpos = 0;
/* The current y position of the mouse. */
static float mouse_ypos = 0;
/* The x position the mouse had before the last update. */
static float last_mouse_xpos = 0;
/* The y position the mouse had before the last update. */
static float last_mouse_ypos = 0;


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Update mouse state ----------------------------- */

/* Update the mouse click and flag state. */
void update_mouse_state(int action, int button) {
  /* Press */
  if (action == GLFW_PRESS) {
    /* Give some wiggle room for the position of a repeated mouse click. */
    if (button == last_mouse_button && (glfwGetTime() - last_mouse_click_time) < DOUBLE_CLICK_THRESHOLD
    && mouse_xpos > (last_mouse_xpos - 3.0f) && mouse_xpos < (last_mouse_xpos + 3.0f)
    && mouse_ypos > (last_mouse_ypos - 3.0f) && mouse_ypos < (last_mouse_ypos + 3.0f))
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
    last_mouse_click_time = (MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE) ? 0 : glfwGetTime());
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
}

/* ----------------------------- Update mouse pos ----------------------------- */

/* Update the mouse position state, ensuring we save the last position. */
void update_mouse_pos(float x, float y) {
  /* Save the current mouse position. */
  last_mouse_xpos = mouse_xpos;
  last_mouse_ypos = mouse_ypos;
  /* Then update the current mouse position. */
  mouse_xpos = x;
  mouse_ypos = y;
}

/* ----------------------------- Get mouse xpos ----------------------------- */

/* The only way to get the mouse x position. */
float get_mouse_xpos(void) {
  return mouse_xpos;
}

/* ----------------------------- Get mouse ypos ----------------------------- */

/* The only way to get the mouse y position. */
float get_mouse_ypos(void) {
  return mouse_ypos;
}

/* ----------------------------- Get mouse pos ----------------------------- */

/* Get both mouse positions at once. */
void get_mouse_pos(float *const x, float *const y) {
  ASSERT(x);
  ASSERT(y);
  *x = mouse_xpos;
  *y = mouse_ypos;
}

/* ----------------------------- Get last mouse xpos ----------------------------- */

float get_last_mouse_xpos(void) {
  return last_mouse_xpos;
}

/* ----------------------------- Get last mouse ypos ----------------------------- */

float get_last_mouse_ypos(void) {
  return last_mouse_ypos;
}

/* ----------------------------- Is mouse flag set ----------------------------- */

/* Check if a mouse flag is set. */
bool is_mouse_flag_set(Uint flag) {
  return MOUSE_ISSET(flag);
}

void clear_mouse_flags(void) {
  memset(mouse_flags, 0, sizeof(mouse_flags));
}
