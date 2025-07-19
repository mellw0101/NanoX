/** @file window.c

  @author  Melwin Svensson.
  @date    19-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static bool resize_needed = TRUE;
static bool fetch_needed = FALSE;
static Ulong last_resize = 0;

static int width  = 1100;
static int height = 650;

static int last_width  = -1;
static int last_height = -1;

/* The main window. */
static GLFWwindow *window = NULL;

/* The root element of the window. */
static Element *root = NULL;


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Returns `TRUE` when a redraw will be required.  Note that this should only be called from the main loop. */
bool gl_window_resize_needed(void) {
  int w;
  int h;
  /* If we need to fetch the size of the window for some reason, we do it before all other things. */
  if (ATOMIC_FETCH(fetch_needed)) {
    ATOMIC_STORE(fetch_needed, FALSE);
    /* Set the last resize to this frame so that we will resize in 4 frames. */
    ATOMIC_STORE(last_resize, frame_elapsed());
    glfwGetWindowSize(window, &w, &h);
    gl_window_update_size(w, h);
    log_I_0("fetch: %d, %d", width, height);
  }
  /* When a resize has been called, and the last refresh, or failed refresh was 4 or more frames away, we do the actual refresh. */
  else if (ATOMIC_FETCH(resize_needed) && (frame_elapsed() - ATOMIC_FETCH(last_resize)) >= 4) {
    w = ATOMIC_FETCH(width);
    h = ATOMIC_FETCH(height);
    log_I_0("resize: %d, %d", w, h);
    /* Save the current elapsed frames for the next time. */
    ATOMIC_STORE(last_resize, frame_elapsed());
    ATOMIC_STORE(resize_needed, FALSE);
    glViewport(0, 0, w, h);
    shader_set_projection(0, w, 0, h, -1.f, 1.f);
    shader_upload_projection();
    CLIST_ITER(starteditor, editor,
      editor_resize(editor);
    );
    element_resize(root, w, h);
    ATOMIC_STORE(refresh_needed, TRUE);
    return TRUE;
  }
  /* Default to returning `FALSE`. */
  return FALSE;
}

void gl_window_should_fetch_size(void) {
  ATOMIC_STORE(fetch_needed, TRUE);
}

void gl_window_update_size(int new_width, int new_height) {
  ATOMIC_STORE(last_width,  ATOMIC_FETCH(width));
  ATOMIC_STORE(last_height, ATOMIC_FETCH(height));
  ATOMIC_STORE(width,  new_width);
  ATOMIC_STORE(height, new_height);
  ATOMIC_STORE(resize_needed, TRUE);
}

void gl_window_init(void) {
  window = glfwCreateWindow(width, height, PROJECT_NAME, NULL, NULL);
  if (!window) {
    glfwTerminate();
    log_EFA("Failed to create the main glfw window");
  }
  glfwMakeContextCurrent(window);
  root = element_create(0, 0, width, height, FALSE);
}

void gl_window_free(void) {
  glfwDestroyWindow(window);
  element_free(root);
}

GLFWwindow *gl_window(void) {
  return window;
}

Element *gl_window_root(void) {
  return root;
}

int gl_window_width(void) {
  return ATOMIC_FETCH(width);
}

int gl_window_height(void) {
  return ATOMIC_FETCH(height);
}

void gl_window_add_root_child(Element *const child) {
  element_set_parent(child, root);
}
