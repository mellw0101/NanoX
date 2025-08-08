#include "../include/prototypes.h"

#ifdef HAVE_GLFW


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The main structure that holds all the data the gui needs. */
// guistruct *gui = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Setup the bottom bar for the gui. */
// static void setup_botbar(void) {
//   gui->botbuf = make_new_font_buffer();
//   gui->botbar = element_create(0, (gl_window_height() - font_height(uifont)), gl_window_width(), font_height(uifont), TRUE);
//   gl_window_add_root_child(gui->botbar);
//   gui->botbar->color = PACKED_UINT(0, 0, 0, 255);
//   gui->botbar->xflags |= (ELEMENT_REVREL_Y | ELEMENT_REL_WIDTH);
//   gui->botbar->rel_y = gui->botbar->height;
// }

/* Set up the bottom bar. */
// static void setup_statusbar(void) {
//   gui->statusbuf = make_new_font_buffer();
//   gui->statusbar = element_create(0, gl_window_height(), gl_window_width(), font_height(uifont), FALSE);
//   gui->statusbar->color = PACKED_UINT_VS_CODE_RED;
//   gl_window_add_root_child(gui->statusbar);
//   gui->statusbar->xflags |= ELEMENT_HIDDEN;
// }

/* Allocate and init the edit element. */
static void setup_edit_element(void) {
  editor_create(FALSE);
}

/* Create the main guistruct, completely blank. */
// static void make_guistruct(void) {
  /* Allocate the gui object. */
  // MALLOC_STRUCT(gui);
  // gui->flag                 = bit_flag_t<8>();
  // gui->handler              = NULL;
  // gui->botbar               = NULL;
  // gui->statusbar            = NULL;
  // gui->entered              = NULL;
  // gui->clicked              = NULL;
  // gui->botbuf               = NULL;
  // gui->statusbuf            = NULL;
  // gui->promptmenu           = NULL;
  // gui->current_cursor_type  = GLFW_ARROW_CURSOR;
  // gui->suggestmenu          = NULL;
  // gui->context_menu         = NULL;
// }

/* Init the gui struct, it reprecents everything that the gui needs. */
static void init_guistruct(void) {
  /* Create the fully blank guistruct. */
  // make_guistruct();
  /* Then create the glfw window. */
  gl_window_init();
  gl_mouse_init();
  /* Create and start the event handler. */
  // gui->handler = nevhandler_create();
  // nevhandler_start(gui->handler, TRUE);
}

/* Delete the gui struct. */
static void delete_guistruct(void) {
  /* Destroy glfw window, then terminate glfw. */
  gl_window_free();
  // glfwTerminate();
  /* Destroy the shaders, and the allocated text and ui font. */
  shader_free();
  /* Delete all the vertex buffers. */
  // if (gui->botbuf) {
  //   vertex_buffer_delete(gui->botbuf);
  // }
  // if (gui->statusbuf) {
  //   vertex_buffer_delete(gui->statusbuf);
  // }
  /* Stop and free the event handler. */
  // nevhandler_stop(gui->handler, 0);
  // nevhandler_free(gui->handler);
  /* Delete the prompt-menu struct. */
  // gui_promptmenu_free();
  promptmenu_free();
  /* Free the gui suggestmenu substructure. */
  // gui_suggestmenu_free();
  suggestmenu_free();
  // context_menu_free(gui->context_menu);
  // free(gui);
  // gui = NULL;
}

/* Cleanup before exit. */
static void cleanup(void) {
  /* Temporaty fix. */
  TUI_SF = GUI_SF;
  TUI_OF = GUI_OF;
  editor_free(openeditor);
  statusbar_free();
  delete_guistruct();
  element_grid_free();
  gl_mouse_free();
}

/* Init glew and check for errors.  Terminates on fail to init glew. */
static void init_glew(void) {
  Uint err;
  /* Enable glew experimental features. */
  glewExperimental = TRUE;
  /* If we could not init glew, terminate directly. */
  if ((err = glewInit()) != 0) {
    gl_window_free();
    glfwTerminate();
    log_ERR_FA("GLEW: ERROR: %s", glewGetErrorString(err));
  }
  log_INFO_1("Using GLEW %s", glewGetString(GLEW_VERSION));
}

static inline bool should_swap_buffer(void) {
  bool ret = refresh_needed;
  if (frame_should_poll()) {
    ret = TRUE;
  }
  if (gl_window_resize_needed()) {
    ret = TRUE;
  }
  return ret;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Init glfw. */
void init_gui(void) {
  /* Init glfw. */
  if (!glfwInit()) {
    die("Failed to init glfw.\n");
  }
  element_grid_create(GRIDMAP_GRIDSIZE);
  /* Init the main gui structure. */
  init_guistruct();
  /* Init glew. */
  init_glew();
  /* Compile the shaders. */
  shader_compile();
  // gui->context_menu = context_menu_create();
  /* Init the gui suggestmenu substructure. */
  // gui_suggestmenu_create();
  suggestmenu_create();
  /* Init the top bar. */
  // gui_promptmenu_create();
  promptmenu_create();
  /* Init the bottom bar. */
  // setup_botbar();
  /* Init the bottom bar, that will be used for status updates, among other thing. */
  // setup_statusbar();
  // statusbar_init(gui->root);
  statusbar_init();
  /* Init the edit element. */
  setup_edit_element();
  
  /* Set some callbacks. */
  // glfwSetKeyCallback(gl_window(), key_callback);
  // glfwSetCharCallback(gl_window(), char_callback);
  // glfwSetMouseButtonCallback(gl_window(), mouse_button_callback);
  // glfwSetCursorPosCallback(gl_window(), mouse_pos_callback);
  // glfwSetCursorEnterCallback(gl_window(), window_enter_callback);
  // glfwSetScrollCallback(gl_window(), scroll_callback);

  /* Ensure we poll for the correct frame rate at the start. */
  frame_set_poll();
  // frame_should_report(TRUE);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  /* Set the window size. */
  editor_confirm_margin(openeditor);
  editor_redecorate(openeditor);
  editor_resize(openeditor);
}

/* Main gui loop. */
void glfw_loop(void) {
  // while (!glfwWindowShouldClose(gl_window())) {
  while (gl_window_running()) {
    frame_start();
    statusbar_count_frame();
    if (should_swap_buffer()) {
      log_INFO_1("Swapping buffer");
      place_the_cursor();
      glClear(GL_COLOR_BUFFER_BIT);
      /* Check if any editor's has it's `should_close` flag set, and if so close them. */
      editor_check_should_close();
      /* Draw the editors. */
      CLIST_ITER(starteditor, editor,
        editor_confirm_margin(editor);
        draw_editor(editor);
      );
      /* Draw the top menu bar. */
      // draw_topbar();
      promptmenu_draw();
      /* Draw the bottom bar. */
      // draw_botbar();
      /* Draw the status bar, if there is any status messages. */
      statusbar_draw();
      // context_menu_draw(gui->context_menu);
      /* Draw the suggestmenu. */
      // draw_suggestmenu();
      suggestmenu_draw();
      /* If refresh was needed it has been done so set it to FALSE. */
      // glfwSwapBuffers(gl_window());
      gl_window_swap();
      refresh_needed = FALSE;
    }
    // glfwPollEvents();
    gl_window_poll_events();
    frame_end();
  }
  log_INFO_1("Total time elapsed: %.2f seconds", (NANO_TO_MILLI(frame_elapsed_time()) / 1e3));
  cleanup();
  close_and_go();
}

/* Quit the current context, and if we are at only one editor open with only one buffer open, then this function return's `TRUE`.
 * Indecating we should shut down.  Note that this function handles everything and should be used as the only thing to close the current context.
 * Also this function is designed so that when it tells glfw that we should close the window there should be one editor and one buffer in that 
 * editor open this is to ensure that from start to termination there is always an active editor and openeditor. */
bool gui_quit(void) {
  ASSERT_EDITOR(openeditor);
  /* If the buffer has a lock file, delete it. */
  if (openeditor->openfile->lock_filename) {
    delete_lockfile(openeditor->openfile->lock_filename);
  }
  /* When there is more then a single file open in the currently open editor. */
  if (!CLIST_SINGLE(openeditor->openfile)) {
    editor_close_open_buffer();
    editor_redecorate(openeditor);
    editor_resize(openeditor);
    return FALSE;
  }
  else {
    /* If there is only one editor open.  We tell glfw we should quit, we also return `TRUE` so that the calling function can halt execution. */
    if (CLIST_SINGLE(openeditor)) {
      // glfwSetWindowShouldClose(gl_window(), TRUE);
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

#endif
