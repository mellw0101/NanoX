/** @file gui/cursor.c

  @author  Melwin Svensson.
  @date    4-4-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */

bool glfw_cursors_have_been_init = FALSE;
/* Ptrs to the available cursors. */
static GLFWcursor *glfw_cursor_arrow     = NULL;
static GLFWcursor *glfw_cursor_ibeam     = NULL;
static GLFWcursor *glfw_cursor_crosshair = NULL;
static GLFWcursor *glfw_cursor_hand      = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Glfw init cursors ----------------------------- */

/* TODO: Make a wrapper to create these cursors with error-checking. */
static void glfw_init_cursors(void) {
  glfw_cursor_arrow           = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
  glfw_cursor_ibeam           = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
  glfw_cursor_crosshair       = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
  glfw_cursor_hand            = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
  glfw_cursors_have_been_init = TRUE;
}

/* ----------------------------- Glfw get cursor ----------------------------- */

static GLFWcursor *glfw_get_cursor(int type) {
  /* Only allocate the cursors once. */
  if (!glfw_cursors_have_been_init) {
    glfw_init_cursors();
  }
  switch (type) {
    case GLFW_ARROW_CURSOR: {
      return glfw_cursor_arrow;
    }
    case GLFW_IBEAM_CURSOR: {
      return glfw_cursor_ibeam;
    }
    case GLFW_CROSSHAIR_CURSOR: {
      return glfw_cursor_crosshair;
    }
    case GLFW_HAND_CURSOR: {
      return glfw_cursor_hand;
    }
    default: {
      die("%s: Unknown type", __func__);
    }
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Set cursor type ----------------------------- */

/* Set the cursor for window, this is a wrapper that holds all types once they have been used once. */
void set_cursor_type(GLFWwindow *const window, int type) {
  glfwSetCursor(window, glfw_get_cursor(type));
}
