/** @file window.c

  @author  Melwin Svensson.
  @date    19-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* To inforce the glfw rules about only performing some things in the main thread,
 * we will store these things and only update them when a rezing chech happens. */
static bool gl_window_maximized = FALSE;
static bool gl_window_decorated = TRUE;

static bool gl_window_flip_maximized = FALSE;

static bool resize_needed = TRUE;
static bool fetch_needed = FALSE;
/* Start with this at the highest possible positive
 * value so that we ensure that we will resize at startup. */
static Ulong last_resize = HIGHEST_POSITIVE;

static int width  = 1100;
static int height = 650;

static int last_width  = -1;
static int last_height = -1;

/* The main window. */
static GLFWwindow *window = NULL;

/* The root element of the window. */
static Element *root = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Gl window callback resize ----------------------------- */

static void gl_window_callback_resize(GLFWwindow *_UNUSED win, int w, int h) {
  if (!(w == ATOMIC_FETCH(width) && h == ATOMIC_FETCH(height))) {
    ATOMIC_STORE(width, w);
    ATOMIC_STORE(height, h);
    ATOMIC_STORE(resize_needed, TRUE);
  }
}

/* ----------------------------- Gl window callback maximize ----------------------------- */

static void gl_window_callback_maximize(GLFWwindow *_UNUSED win, int _UNUSED maximized) {
  ATOMIC_STORE(fetch_needed, TRUE);
}

/* ----------------------------- Gl window callback framebuffer ----------------------------- */

static void gl_window_callback_framebuffer(GLFWwindow *_UNUSED win, int w, int h) {
  if (!(w == ATOMIC_FETCH(width) && h == ATOMIC_FETCH(height))) {
    ATOMIC_STORE(width, w);
    ATOMIC_STORE(height, h);
    ATOMIC_STORE(resize_needed, TRUE);
  }
}

/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Gl window resize needed ----------------------------- */

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
    if (!(ATOMIC_FETCH(width) == w && ATOMIC_FETCH(height) == h)) {
      ATOMIC_STORE(width,  w);
      ATOMIC_STORE(height, h);
      ATOMIC_STORE(resize_needed, TRUE);
      log_INFO_1("Fetched window size: %d, %d", w, h);
    }
  }
  /* When a resize has been called, and the last refresh, or failed refresh was 4 or more frames away, we do the actual refresh. */
  else if (ATOMIC_FETCH(resize_needed) && (frame_elapsed() - ATOMIC_FETCH(last_resize)) >= 4) {
    /* Load in the width, and height of the window. */
    w = ATOMIC_FETCH(width);
    h = ATOMIC_FETCH(height);
    /* Get the borderless fullscreen state of the window. */
    ATOMIC_STORE(gl_window_maximized, !!glfwGetWindowAttrib(window, GLFW_MAXIMIZED));
    ATOMIC_STORE(gl_window_decorated, !!glfwGetWindowAttrib(window, GLFW_DECORATED));
    /* If we should flip the maximized status, then do so. */
    if (ATOMIC_FETCH(gl_window_flip_maximized)) {
      ATOMIC_STORE(gl_window_flip_maximized, FALSE);
      /* If we are currently maximized. */
      if (gl_window_maximized) {
        glfwRestoreWindow(window);
      }
      /* Otherwize, when we are not. */
      else {
        glfwMaximizeWindow(window);
      }
      gl_window_maximized = !gl_window_maximized;
    }
    /* When we have a missmatch, we are a bit smart, as only the decorations can be incorrect. */
    if (!(gl_window_maximized ^ gl_window_decorated)) {
      glfwSetWindowAttrib(window, GLFW_DECORATED, !gl_window_maximized);
      gl_window_decorated = !gl_window_decorated;
    }
    /* Save the current elapsed frames for the next time. */
    ATOMIC_STORE(last_resize, frame_elapsed());
    ATOMIC_STORE(resize_needed, FALSE);
    if (!(ATOMIC_FETCH(last_width) == w && ATOMIC_FETCH(last_height) == h)) {
      glViewport(0, 0, w, h);
      shader_set_projection(0, w, 0, h, -1.f, 1.f);
      shader_upload_projection();
      CLIST_ITER(starteditor, editor,
        editor_resize(editor);
      );
      element_resize(root, w, h);
      /* Store the size when we last resized. */
      ATOMIC_STORE(last_width,  w);
      ATOMIC_STORE(last_height, h);
      ATOMIC_STORE(refresh_needed, TRUE);
      log_INFO_1("Resized window to: %d, %d", w, h);
    }
    return TRUE;
  }
  /* Default to returning `FALSE`. */
  return FALSE;
}

/* ----------------------------- Gl window init ----------------------------- */

void gl_window_init(void) {
  window = glfwCreateWindow(width, height, PROJECT_NAME, NULL, NULL);
  if (!window) {
    glfwTerminate();
    log_ERR_FA("Failed to create the main glfw window");
  }
  glfwMakeContextCurrent(window);
  glfwSetWindowSizeCallback(window,      gl_window_callback_resize);
  glfwSetWindowMaximizeCallback(window,  gl_window_callback_maximize);
  glfwSetFramebufferSizeCallback(window, gl_window_callback_framebuffer);
  root = element_create(0, 0, width, height, FALSE);
}

/* ----------------------------- Gl window free ----------------------------- */

void gl_window_free(void) {
  glfwDestroyWindow(window);
  element_free(root);
}

/* ----------------------------- Gl window ----------------------------- */

GLFWwindow *gl_window(void) {
  return window;
}

/* ----------------------------- Gl window root ----------------------------- */

Element *gl_window_root(void) {
  return root;
}

/* ----------------------------- Gl window width ----------------------------- */

int gl_window_width(void) {
  return ATOMIC_FETCH(width);
}

/* ----------------------------- Gl window height ----------------------------- */

int gl_window_height(void) {
  return ATOMIC_FETCH(height);
}

/* ----------------------------- Gl window add root child ----------------------------- */

void gl_window_add_root_child(Element *const child) {
  element_set_parent(child, root);
}

void gl_window_borderless_fullscreen(void) {
  ATOMIC_STORE(resize_needed, TRUE);
  ATOMIC_STORE(gl_window_flip_maximized, TRUE);
}
