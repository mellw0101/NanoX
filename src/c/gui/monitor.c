/** @file gui/monitor.c

  @author  Melwin Svensson.
  @date    16-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Monitor current ----------------------------- */

/* Always returns a valid ptr to the current monitor based on the current position of the window. */
GLFWmonitor *monitor_current(void) {
  ASSERT(gui_window);
  int x;
  int y;
  int width;
  int height;
  int mon_x;
  int mon_y;
  int mon_width;
  int mon_height;
  int mon_count;
  int check_x;
  int check_y;
  GLFWmonitor **monitors;
  /* Failure to get monitors means this should not be run at all as glfw has not been init. */
  ALWAYS_ASSERT_MSG((monitors = glfwGetMonitors(&mon_count)), "GLFW needs to be init");
  /* Get the windows current position and size. */
  glfwGetWindowPos(gui_window, &x, &y);
  glfwGetWindowSize(gui_window, &width, &height);
  writef("%d\n", x);
  writef("%d\n", y);
  writef("%d\n", width);
  writef("%d\n", height);
  /* Get the center of the window. */
  check_x = (x + (width / 2));
  check_y = (y + (height / 2));
  /* Go through all monitors. */
  for (int i=0; i<mon_count; ++i) {
    /* Get the workarea of the monitor. */
    glfwGetMonitorWorkarea(monitors[i], &mon_x, &mon_y, &mon_width, &mon_height);
    if (check_x >= mon_x && check_x < (mon_x + mon_width) && check_y >= mon_y && check_y < (mon_y + mon_height)) {
      return monitors[i];
    }
  }
  return monitors[0];
}

/* ----------------------------- Monitor mode ----------------------------- */

const GLFWvidmode *monitor_mode(void) {
  GLFWmonitor *monitor = monitor_current();
  const GLFWvidmode *mode;
  ALWAYS_ASSERT_MSG((mode = glfwGetVideoMode(monitor)), "GLFW needs to be init");
  return mode;
}

/* ----------------------------- Monitor refresh rate ----------------------------- */

int monitor_refresh_rate(void) {
  int count;
  int rate = 60;
  GLFWmonitor **monitors;
  /* Failure to get monitors means this should not be run at all as glfw has not been init. */
  ALWAYS_ASSERT_MSG((monitors = glfwGetMonitors(&count)), "GLFW needs to be init");
  /* Go through all monitors. */
  for (int i=0; i<count; ++i) {
    if (glfwGetVideoMode(monitors[i])->refreshRate > rate) {
      rate = glfwGetVideoMode(monitors[i])->refreshRate;
    }
  }
  return rate;
}
