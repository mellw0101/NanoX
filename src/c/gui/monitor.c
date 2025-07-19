/** @file gui/monitor.c

  @author  Melwin Svensson.
  @date    16-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Monitor get array ----------------------------- */

/* A fail-safe way to ensure we either get the monitor array or we die. */
GLFWmonitor **monitor_get_all(int *const count) {
  GLFWmonitor **monitors;
  ALWAYS_ASSERT(monitors = glfwGetMonitors(count));
  return monitors;
}

/* ----------------------------- Monitor get mode ----------------------------- */

const GLFWvidmode *monitor_get_mode(GLFWmonitor *const monitor) {
  const GLFWvidmode *mode;
  ALWAYS_ASSERT(mode = glfwGetVideoMode(monitor));
  return mode;
}

/* ----------------------------- Monitor refresh rate array ----------------------------- */

int *monitor_refresh_rate_array(int *const count) {
  GLFWmonitor **monitors = monitor_get_all(count);
  int *ret = xmalloc(sizeof(int) * (*count));
  for (int i=0; i<(*count); ++i) {
    ret[i] = monitor_get_mode(monitors[i])->refreshRate;
  }
  return ret;
}

/* ----------------------------- Monitor closest refresh rate ----------------------------- */

int monitor_closest_refresh_rate(int rate) {
  int ret;
  int count;
  int *mon_rates = monitor_refresh_rate_array(&count);
  int closest_index = 0;
  int closest_value = 2400;
  for (int i=0; i<count; ++i) {
    if (abs(rate - mon_rates[i]) < closest_value) {
      closest_index = i;
      closest_value = abs(rate - mon_rates[i]);
    }
  }
  ret = mon_rates[closest_index];
  free(mon_rates);
  return ret;
}

/* ----------------------------- Monitor current ----------------------------- */

/* Always returns a valid ptr to the current monitor based on the current position of the window. */
GLFWmonitor *monitor_current(void) {
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
  glfwGetWindowPos(gl_window(), &x, &y);
  glfwGetWindowSize(gl_window(), &width, &height);
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
