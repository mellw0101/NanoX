#include "../include/prototypes.h"

#ifdef HAVE_GLFW

/* Rect shader data. */
static glSsbo<vec2, 0> vertices_ssbo;
static glSsbo<Uint, 1> indices_ssbo;
/* Hash based grid map for elements. */
uigridmapclass gridmap(GRIDMAP_GRIDSIZE);
/* Frame timer to keep a frame rate. */
frametimerclass frametimer;
/* The file menu button element. */
guielement *file_menu_element = NULL;
guielement *open_file_element = NULL;

/* The list of all `gui-editors`. */
guieditor *openeditor = NULL;
/* The first open `gui-editor`. */
guieditor *starteditor = NULL;

/* The main structure that holds all the data the gui needs. */
guistruct *gui = NULL;

// #define FALLBACK_FONT_PATH "/usr/share/root/fonts/monotype.ttf"
#define FALLBACK_FONT_PATH  "/usr/share/fonts/TTF/Hack-Regular.ttf"
#define FALLBACK_FONT  "fallback"
// #define JETBRAINS_REGULAR_FONT_PATH "/home/mellw/.vscode-insiders/extensions/narasimapandiyan.jetbrainsmono-1.0.2/JetBrainsMono/JetBrainsMono-Regular.ttf"
#define JETBRAINS_REGULAR_FONT_PATH  "/usr/share/fonts/TTF/Hack-Regular.ttf"
#define JETBRAINS_REGULAR_FONT  "jetbrains regular"

/* Define the vertices of a square (centered at the origin). */
constexpr const vec2 vertices[] = {
  /* Pos. */
  vec2(0.0f, 0.0f), /* Bottom-left */
  vec2(1.0f, 0.0f), /* Bottom-right */
  vec2(1.0f, 1.0f), /* Top-right */
  vec2(0.0f, 1.0f), /* Top-left */
};
constexpr const Uint indices[] = {
  0, 1, 2, /* First triangle. */
  2, 3, 0  /* Second triangle. */
};

/* Print a error to stderr.  Used when using the gui. */
void log_error_gui(const char *format, ...) {
  va_list ap;
  int len;
  va_start(ap, format);
  len = vfprintf(stderr, format, ap);
  if (len < 0) {
    die("%s: vfprintf failed.\n", __func__);
  }
  va_end(ap);
}

/* Init font shader and buffers. */
static void setup_font_shader(void) {
  /* Create shader. */
  gui->font_shader = openGL_create_shader_program_raw({
    /* Font vertex shader. */
    { STRLITERAL(\
        uniform mat4 projection;
        attribute vec3 vertex;
        attribute vec2 tex_coord;
        attribute vec4 color;
        void main() {
          gl_TexCoord[0].xy = tex_coord.xy;
          gl_FrontColor     = color;
          gl_Position       = projection * (vec4(vertex,1.0));
        }
      ),
      GL_VERTEX_SHADER },
    /* Font fragment shader. */
    { STRLITERAL(\
        uniform sampler2D texture;
        void main() {
          float a = texture2D(texture, gl_TexCoord[0].xy).r;
          gl_FragColor = vec4(gl_Color.rgb, (gl_Color.a * a));
        }
      ),
      GL_FRAGMENT_SHADER }
  });
  /* If there is a problem with the shader creation just die as we are passing string literals so there should not be alot that can go wrong. */
  if (!gui->font_shader) {
    glfwDestroyWindow(gui->window);
    glfwTerminate();
    die("Failed to create font shader.\n");
  }
  /* Load the font and uifont. */
  gui_font_load(gui->font, FALLBACK_FONT_PATH, 17, 512);
  gui_font_load(gui->uifont, FALLBACK_FONT_PATH, 15, 512);
  // set_gui_font(FALLBACK_FONT_PATH, gui->font_size);
  // set_gui_uifont(FALLBACK_FONT_PATH, gui->uifont_size);
}

/* Init the rect shader and setup the ssbo`s for indices and vertices. */
static void setup_rect_shader(void) {
  gui->rect_shader = openGL_create_shader_program_raw({
    { STRLITERAL(\
        #version 450 core \n
        layout(std430, binding = 0) buffer VertexBuffer {
          vec2 vertices[];
        };
        layout(std430, binding = 1) buffer IndexBuffer {
          uint indices[];
        };
        /* Uniforms. */
        uniform mat4 projection;
        uniform vec2 elemsize;
        uniform vec2 elempos;
        /* Main vertex shader exec. */
        void main() {
          vec2 scaledPos = (vertices[indices[gl_VertexID]] * elemsize);
          vec4 worldPos  = (projection * vec4(scaledPos + elempos, 0.0f, 1.0f));
          gl_Position    = worldPos;
        }
      ),
      GL_VERTEX_SHADER },
    { STRLITERAL(\
        #version 450 core\n
        /* Output. */
        out vec4 FragColor;
        /* Uniforms. */
        uniform vec4 rectcolor;
        /* Main shader exec. */
        void main() {
          FragColor = rectcolor;
        }
      ),
      GL_FRAGMENT_SHADER }
  });
  /* Init and setup the static indices and vertices ssbo. */
  indices_ssbo.init(indices);
  vertices_ssbo.init(vertices);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertices_ssbo.ssbo());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indices_ssbo.ssbo());
}

/* Setup the bottom bar for the gui. */
static void setup_botbar(void) {
  gui->botbuf = make_new_font_buffer();
  gui->botbar = gui_element_create(gui->root, FALSE);
  gui_element_move_resize(
    gui->botbar,
    vec2(0, (gui->height - FONT_HEIGHT(gui_font_get_font(gui->font)))),
    vec2(gui->width, FONT_HEIGHT(gui_font_get_font(gui->font)))
  );
  gui->botbar->color = GUI_BLACK_COLOR;
  gui->botbar->flag.set<GUIELEMENT_REVERSE_RELATIVE_Y_POS>();
  gui->botbar->relative_pos = vec2(0, gui->botbar->size.h);
  gui->botbar->flag.set<GUIELEMENT_RELATIVE_WIDTH>();
  gui->botbar->relative_size = 0;
}

/* Set up the bottom bar. */
static void setup_statusbar(void) {
  gui->statusbuf = make_new_font_buffer();
  gui->statusbar = gui_element_create(
    vec2(0, gui->height),
    vec2(gui->width, gui_font_height(gui->uifont)),
    color_idx_to_vec4(FG_VS_CODE_RED)
  );
  gui->statusbar->flag.set<GUIELEMENT_HIDDEN>();
}

/* Allocate and init the edit element. */
static void setup_edit_element(void) {
  /* Confirm the margin first to determen how wide the gutter has to be. */
  confirm_margin();
  /* Create the editor circular list. */
  make_new_editor(FALSE);
}

/* Create the main guistruct, completely blank. */
static void make_guistruct(void) {
  /* Allocate the gui object. */
  MALLOC_STRUCT(gui);
  /* Then init all fields to something invalid, this is important if we need to abort. */
  gui->title                 = NULL;
  gui->width                 = 0;
  gui->height                = 0;
  gui->window                = NULL;
  gui->flag                  = bit_flag_t<8>();
  gui->handler               = NULL;
  gui->botbar                = NULL;
  gui->statusbar             = NULL;
  gui->entered               = NULL;
  gui->clicked               = NULL;
  gui->botbuf                = NULL;
  gui->statusbuf             = NULL;
  gui->projection            = NULL;
  gui->font_shader           = 0;
  gui->uifont                = NULL;
  gui->font                  = NULL;
  gui->rect_shader           = 0;
  gui->promptmenu            = NULL;
  gui->current_cursor_type   = GLFW_ARROW_CURSOR;
  gui->suggestmenu           = NULL;
  gui->active_menu           = NULL;
  gui->context_menu          = NULL;
}

/* Init the gui struct, it reprecents everything that the gui needs. */
static void init_guistruct(const char *win_title, Uint win_width, Uint win_height, int fps, Uint font_size, Uint uifont_size) {
  /* Create the fully blank guistruct. */
  make_guistruct();
  /* Set the basic data needed to init the window. */
  gui->title      = copy_of(win_title);
  gui->width      = win_width;
  gui->height     = win_height;
  gui->projection = matrix4x4_new();
  /* Then create the glfw window. */
  gui->window = glfwCreateWindow(gui->width, gui->height, gui->title, NULL, NULL);
  if (!gui->window) {
    glfwTerminate();
    die("Failed to create glfw window.\n");
  }
  glfwMakeContextCurrent(gui->window);
  frametimer.fps = ((fps == -1) ? 240 : fps);
  /* Create and start the event handler. */
  gui->handler = nevhandler_create();
  nevhandler_start(gui->handler, TRUE);
  gui->font   = gui_font_create();
  gui->uifont = gui_font_create();
  gui->root = gui_element_create(0, vec2(gui->height, gui->width), 0, FALSE);
}

/* Delete the gui struct. */
static void delete_guistruct(void) {
  free(gui->title);
  /* Destroy glfw window, then terminate glfw. */
  glfwDestroyWindow(gui->window);
  glfwTerminate();
  /* Destroy the shaders. */
  if (gui->font_shader) {
    glDeleteProgram(gui->font_shader);
  }
  if (gui->rect_shader) {
    glDeleteProgram(gui->rect_shader);
  }
  /* Delete all elements used by 'gui'. */
  gui_element_free(gui->root);
  gui_element_free(gui->statusbar);
  /* Free the main font and the uifont. */
  gui_font_free(gui->font);
  gui_font_free(gui->uifont);
  /* Delete all the vertex buffers. */
  if (gui->botbuf) {
    vertex_buffer_delete(gui->botbuf);
  }
  if (gui->statusbuf) {
    vertex_buffer_delete(gui->statusbuf);
  }
  /* Stop and free the event handler. */
  nevhandler_stop(gui->handler, 0);
  nevhandler_free(gui->handler);
  /* Delete the prompt-menu struct. */
  gui_promptmenu_free();
  /* Free the gui suggestmenu substructure. */
  gui_suggestmenu_free();
  context_menu_free(gui->context_menu);
  free(gui);
  gui = NULL;
}

/* Cleanup before exit. */
static void cleanup(void) { 
  gui_editor_free(openeditor);
  delete_guistruct();
}

/* Init glew and check for errors.  Terminates on fail to init glew. */
static void init_glew(void) {
  Uint err;
  /* Enable glew experimental features. */
  glewExperimental = TRUE;
  /* If we could not init glew, terminate directly. */
  if ((err = glewInit()) != GLEW_OK) {
    glfwDestroyWindow(gui->window);
    glfwTerminate();
    die("GLEW: ERROR: %s\n", glewGetErrorString(err));
  }
  writef("Using GLEW %s\n", glewGetString(GLEW_VERSION));
}

/* Init glfw. */
void init_gui(void) {
  /* Init glfw. */
  if (!glfwInit()) {
    die("Failed to init glfw.\n");
  }
  /* Init the main gui structure. */
  init_guistruct("NanoX", 1400, 800, glfw_get_framerate(), 17, 15);
  /* Init glew. */
  init_glew();
  /* Init the font shader. */
  setup_font_shader();
  gui->context_menu = context_menu_create();
  /* Init the rect shader. */
  setup_rect_shader();
  /* Init the gui suggestmenu substructure. */
  gui_suggestmenu_create();
  /* Init the top bar. */
  gui_promptmenu_create();
  /* Init the bottom bar. */
  setup_botbar();
  /* Init the bottom bar, that will be used for status updates, among other thing. */
  setup_statusbar();
  /* Init the edit element. */
  setup_edit_element();
  /* Set some callbacks. */
  glfwSetWindowSizeCallback(gui->window, window_resize_callback);
  glfwSetWindowMaximizeCallback(gui->window, window_maximize_callback);
  glfwSetFramebufferSizeCallback(gui->window, framebuffer_resize_callback);
  glfwSetKeyCallback(gui->window, key_callback);
  glfwSetCharCallback(gui->window, char_callback);
  glfwSetMouseButtonCallback(gui->window, mouse_button_callback);
  glfwSetCursorPosCallback(gui->window, mouse_pos_callback);
  glfwSetCursorEnterCallback(gui->window, window_enter_callback);
  glfwSetScrollCallback(gui->window, scroll_callback);
  writef("Current fps: %d\n", frametimer.fps);
  // glClearColor(1.00, 1.00, 1.00, 1.00);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  /* Set the window size. */
  window_resize_callback(gui->window, gui->width, gui->height);
}

/* Main gui loop. */
void glfw_loop(void) {
  while (!glfwWindowShouldClose(gui->window)) {
    frametimer.start();
    confirm_margin();
    place_the_cursor();
    glClear(GL_COLOR_BUFFER_BIT);
    /* Check if any editor's has it's `should_close` flag set, and if so close them. */
    gui_editor_check_should_close();
    /* Draw the editors. */
    CLIST_ITER(starteditor, editor,
      draw_editor(editor);
    );
    /* Draw the suggestmenu. */
    draw_suggestmenu();
    /* Draw the top menu bar. */
    draw_topbar();
    /* Draw the bottom bar. */
    draw_botbar();
    /* Draw the status bar, if there is any status messages. */
    draw_statusbar();
    context_menu_draw(gui->context_menu);
    /* If refresh was needed it has been done so set it to FALSE. */
    refresh_needed = FALSE;
    glfwSwapBuffers(gui->window);
    glfwPollEvents();
    frametimer.end();
  }
  cleanup();
  close_and_go();
}

/* Quit the current context, and if we are at only one editor open with only one buffer open, then this function return's `TRUE`.
 * Indecating we should shut down.  Note that this function handles everything and should be used as the only thing to close the current context.
 * Also this function is designed so that when it tells glfw that we should close the window there should be one editor and one buffer in that 
 * editor open this is to ensure that from start to termination there is always an active editor and openeditor. */
bool gui_quit(void) {
  ASSERT(openeditor);
  ASSERT(openeditor->openfile);
  /* If the buffer has a lock file, delete it. */
  if (openeditor->openfile->lock_filename) {
    delete_lockfile(openeditor->openfile->lock_filename);
  }
  /* When there is more then a single file open in the currently open editor. */
  if (!CLIST_SINGLE(openeditor->openfile)) {
    gui_editor_close_open_buffer();
    gui_editor_redecorate(openeditor);
    gui_editor_resize(openeditor);
    return FALSE;
  }
  else {
    /* If there is only one editor open.  We tell glfw we should quit, we also return `TRUE` so that the calling function can halt execution. */
    if (CLIST_SINGLE(openeditor)) {
      glfwSetWindowShouldClose(gui->window, TRUE);
      return TRUE;
    }
    else {
      // gui_editor_close();
      gui_editor_close(openeditor);
      gui_editor_hide(openeditor, FALSE);
      gui_editor_redecorate(openeditor);
      gui_editor_resize(openeditor);
      return FALSE;
    }
  }
}

#endif
