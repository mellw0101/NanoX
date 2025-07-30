/** @file window.c

  @author  Melwin Svensson.
  @date    19-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* To inforce the glfw rules about only performing some things in the main thread,
 * we will store these things and only update them when a rezing chech happens. */
static bool gl_window_maximized = FALSE;
// static bool gl_window_decorated = TRUE;

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
static SDL_Window *gl_win = NULL;

/* The SDL3 main window id. */
static Uint gl_win_id = 0;

/* The sdl context. */
static SDL_GLContext gl_context = NULL;

/* The root element of the window. */
static Element *root = NULL;

static SDL_Event ev;

static bool gl_win_running = TRUE;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Gl window init SDL ----------------------------- */

_UNUSED
static void gl_window_init_SDL(void) {
  /* Init SDL3. */
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    log_ERR_FA("Failed to init SDL3: %s", SDL_GetError());
  }
  /* Create a openGL SDL3 window. */
  if (!(gl_win = SDL_CreateWindow(PROJECT_NAME, width, height, (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)))) {
    SDL_Quit();
    log_ERR_FA("Failed to create an openGL SDL3 window: %s", SDL_GetError());
  }
  gl_win_id = SDL_GetWindowID(gl_win);
  /* Create the context for the SDL3 window. */
  if (!(gl_context = SDL_GL_CreateContext(gl_win))) {
    SDL_DestroyWindow(gl_win);
    SDL_Quit();
    log_ERR_FA("Failed to create SDL3 openGL context: %s", SDL_GetError());
  }
  if (!SDL_StartTextInput(gl_win)) {
    log_ERR_FA("Could not start text input for the main window: %s", SDL_GetError());
  }
}

/* ----------------------------- Gl window callback resize ----------------------------- */

_UNUSED
static void gl_window_callback_resize(GLFWwindow *_UNUSED win, int w, int h) {
  if (!(w == ATOMIC_FETCH(width) && h == ATOMIC_FETCH(height))) {
    ATOMIC_STORE(width, w);
    ATOMIC_STORE(height, h);
    ATOMIC_STORE(resize_needed, TRUE);
  }
}

/* ----------------------------- Gl window callback maximize ----------------------------- */

_UNUSED
static void gl_window_callback_maximize(GLFWwindow *_UNUSED win, int _UNUSED maximized) {
  ATOMIC_STORE(fetch_needed, TRUE);
}

/* ----------------------------- Gl window callback framebuffer ----------------------------- */

_UNUSED
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
    // glfwGetWindowSize(window, &w, &h);
    SDL_GetWindowSize(gl_win, &w, &h);
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
    // ATOMIC_STORE(gl_window_maximized, !!glfwGetWindowAttrib(window, GLFW_MAXIMIZED));
    // ATOMIC_STORE(gl_window_decorated, !!glfwGetWindowAttrib(window, GLFW_DECORATED));
    /* If we should flip the maximized status, then do so. */
    if (ATOMIC_FETCH(gl_window_flip_maximized)) {
      ATOMIC_STORE(gl_window_flip_maximized, FALSE);
      /* If we are currently maximized. */
      if (gl_window_maximized) {
        // glfwRestoreWindow(window);
        SDL_SetWindowFullscreen(gl_win, FALSE);
      }
      /* Otherwize, when we are not. */
      else {
        // glfwMaximizeWindow(window);
        SDL_SetWindowFullscreen(gl_win, TRUE);
      }
      gl_window_maximized = !gl_window_maximized;
    }
    /* When we have a missmatch, we are a bit smart, as only the decorations can be incorrect. */
    // if (!(gl_window_maximized ^ gl_window_decorated)) {
    //   glfwSetWindowAttrib(window, GLFW_DECORATED, !gl_window_maximized);
    //   gl_window_decorated = !gl_window_decorated;
    // }
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
  gl_window_init_SDL();
  // window = glfwCreateWindow(width, height, PROJECT_NAME, NULL, NULL);
  // if (!window) {
  //   glfwTerminate();
  //   log_ERR_FA("Failed to create the main glfw window");
  // }
  // glfwMakeContextCurrent(window);
  // glfwSetWindowSizeCallback(window,      gl_window_callback_resize);
  // glfwSetWindowMaximizeCallback(window,  gl_window_callback_maximize);
  // glfwSetFramebufferSizeCallback(window, gl_window_callback_framebuffer);
  root = element_create(0, 0, width, height, FALSE);
}

/* ----------------------------- Gl window free ----------------------------- */

void gl_window_free(void) {
  SDL_GL_DestroyContext(gl_context);
  SDL_DestroyWindow(gl_win);
  SDL_Quit();
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

void gl_window_poll_events(void) {
  while (SDL_PollEvent(&ev)) {
    switch (ev.type) {
      case SDL_EVENT_QUIT: {
        gl_win_running = FALSE;
        break;
      }
      case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        mouse_gui_update_state(TRUE, ev.button.button);
        mouse_gui_button_down(ev.button.button, SDL_GetModState(), ev.button.x, ev.button.y);
        break;
      }
      case SDL_EVENT_MOUSE_BUTTON_UP: {
        mouse_gui_update_state(FALSE, ev.button.button);
        mouse_gui_button_up(ev.button.button, SDL_GetModState(), ev.button.x, ev.button.y);
        break;
      }
      case SDL_EVENT_MOUSE_MOTION: {
        mouse_gui_update_pos(ev.motion.x, ev.motion.y);
        mouse_gui_position(ev.button.x, ev.button.y);
        break;
      }
      case SDL_EVENT_TEXT_INPUT: {
        if (promptmenu_active()) {
          kb_prompt_char_input(ev.text.text, SDL_GetModState());
        }
        else {
          kb_char_input(ev.text.text, SDL_GetModState());
        }
        break;
      }
      case SDL_EVENT_KEY_DOWN: {
        if (promptmenu_active()) {
          kb_prompt_key_pressed(ev.key.key, ev.key.scancode, ev.key.mod, ev.key.repeat);
        }
        else {
          kb_key_pressed(ev.key.key, ev.key.scancode, ev.key.mod, ev.key.repeat);
        }
        break;
      }
      case SDL_EVENT_WINDOW_RESIZED: {
        if (!(ev.window.data1 == ATOMIC_FETCH(width) && ev.window.data2 == ATOMIC_FETCH(height))) {
          ATOMIC_STORE(width,  ev.window.data1);
          ATOMIC_STORE(height, ev.window.data2);
          ATOMIC_STORE(resize_needed, TRUE);
        }
        break;
      }
      case SDL_EVENT_WINDOW_MAXIMIZED: {
        log_INFO_1("Window maximized");
        break;
      }
      case SDL_EVENT_WINDOW_MINIMIZED: {
        log_INFO_1("Window minimized");
        break;
      }
      case SDL_EVENT_WINDOW_RESTORED: {
        log_INFO_1("window restored");
        break;
      }
      case SDL_EVENT_WINDOW_DISPLAY_CHANGED: {
        log_INFO_1("Window display changed");
        frame_set_poll();
        break;
      }
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
        log_INFO_1("Window close requested");
        break;
      }
      case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED: {
        log_INFO_1("Window Safe area changed");
        break;
      }
      /* TODO: For these two FOCUS_(GAINED/LOST) we should have a efficent mode, to
       * regulate frame-pacing and, either half, or a quarter of the set frame rate. */
      case SDL_EVENT_WINDOW_FOCUS_GAINED: {
        log_INFO_1("Gained focus");
        break;
      }
      case SDL_EVENT_WINDOW_FOCUS_LOST: {
        log_INFO_1("Lost focus");
        break;
      }
      case SDL_EVENT_WINDOW_MOUSE_ENTER: {
        log_INFO_1("Entered window");
        break;
      }
      case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
        mouse_gui_left();
        break;
      }
      case SDL_EVENT_MOUSE_WHEEL: {
        mouse_gui_scroll(ev.wheel.mouse_x, ev.wheel.mouse_y, ev.wheel.integer_x, ev.wheel.integer_y, ev.wheel.direction);
        break;
      }
    }
  }
}

void gl_window_swap(void) {
  SDL_GL_SwapWindow(gl_win);
}

bool gl_window_running(void) {
  return gl_win_running;
}

bool gl_window_quit(void) {
  ASSERT_EDITOR(openeditor);
  /* If the currently open buffer has a lock file, delete it. */
  if (GUI_OF->lock_filename) {
    delete_lockfile(GUI_OF->lock_filename);
  }
  /* Whwn there is more then a single file open in the current editor.  Close that file. */
  if (!CLIST_SINGLE(GUI_OF)) {
    editor_close_open_buffer();
    editor_redecorate(openeditor);
    editor_resize(openeditor);
    return FALSE;
  }
  else {
    /* When there is only one editor open with a single file.  We quit. */
    if (CLIST_SINGLE(openeditor)) {
      gl_win_running = FALSE;
      return TRUE;
    }
    else {
      editor_close(openeditor);
      editor_hide(openeditor, FALSE);
      editor_redecorate(openeditor);
      editor_resize(openeditor);
      return FALSE;
    }
  }
}
