/** @file gui/cursor.c

  @author  Melwin Svensson.
  @date    4-4-2025.

 */
#include "../../include/c_proto.h"


/* Set the cursor for window, this is a wrapper that holds all types once they have been used once. */
void set_cursor_type(GLFWwindow *const window, int type) {
  static GLFWcursor *arrow=NULL, *ibeam=NULL, *crosshair=NULL, *hand=NULL;
  GLFWcursor *cursor=NULL;
  switch (type) {
    case GLFW_ARROW_CURSOR: {
      if (!arrow) {
        arrow = glfwCreateStandardCursor(type);
      }
      cursor = arrow;
      break;
    }
    case GLFW_IBEAM_CURSOR: {
      if (!ibeam) {
        ibeam = glfwCreateStandardCursor(type);
      }
      cursor = ibeam;
      break;
    }
    case GLFW_CROSSHAIR_CURSOR: {
      if (!crosshair) {
        crosshair = glfwCreateStandardCursor(type);
      }
      cursor = crosshair;
      break;
    }
    case GLFW_HAND_CURSOR: {
      if (!hand) {
        hand = glfwCreateStandardCursor(type);
      }
      cursor = hand;
      break;
    }
    default: {
      die("%s: Unknown cursor passed.\n");
    }
  }
  glfwSetCursor(window, cursor);
}
