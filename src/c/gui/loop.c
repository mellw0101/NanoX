/** @file gui/loop.c

  @author  Melwin Svensson.
  @date    5-8-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ELEMENT_GRID_CELL_SIZE  (20)


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Gl loop init glew ----------------------------- */

/* Init glew and check for errors.  Terminates on fail to init glew. */
static void gl_loop_init_glew(void) {
  Uint err;
  /* Enable glew experimental features. */
  glewExperimental = TRUE;
  /* If we could not init glew, terminate directly. */
  if ((err = glewInit()) != 0) {
    log_ERR_FA("GLEW: ERROR: %s", glewGetErrorString(err));
  }
  log_INFO_0("Using GLEW %s", glewGetString(GLEW_VERSION));
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
}

/* ----------------------------- Gl loop init ----------------------------- */

static void gl_loop_init(void) {
  element_grid_create(ELEMENT_GRID_CELL_SIZE);
  gl_window_init();
  gl_loop_init_glew();
  mouse_gui_init();
  shader_compile();
  suggestmenu_create();
  promptmenu_create();
  statusbar_init();
  /* Create the first editor by taking ownership of the already made openfilestruct in main.cpp.  This will be changed later. */
  editor_create(FALSE);
  /* Ensure we poll for the correct frame-rate. */
  frame_set_poll();
  gl_window_resize((gl_window_width() + 1), (gl_window_height() + 1));
  gl_window_resize((gl_window_width() - 1), (gl_window_height() - 1));
}

/* ----------------------------- Gl loop cleanup ----------------------------- */

static void gl_loop_clean(void) {
  /* Temporary fix. */
  TUI_SF = GUI_SF;
  TUI_OF = GUI_OF;
  editor_free(openeditor);
  statusbar_free();
  gl_window_free();
  shader_free();
  promptmenu_free();
  suggestmenu_free();
  element_grid_free();
  mouse_gui_free(); 
  close_and_go();
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Gl loop ----------------------------- */

void gl_loop(void) {
  gl_loop_init();
  while (gl_window_running()) {
    frame_start();
    statusbar_count_frame();
    if (frame_should_poll() || refresh_needed) {
      log_INFO_0("Swapping buffer");
      place_the_cursor();
      glClear(GL_COLOR_BUFFER_BIT);
      editor_check_should_close();
      CLIST_ITER(starteditor, editor,
        editor_confirm_margin(editor);
        draw_editor(editor);
      );
      promptmenu_draw();
      statusbar_draw();
      gl_window_swap();
      refresh_needed = FALSE; 
    }
    gl_window_poll_events();
    frame_end();
  }
  gl_loop_clean();
}
