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

vec2 pen;

static fvector4 _GL_UNUSED black_vec4 = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
static fvector4 _GL_UNUSED white_vec4 = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
static fvector4 _GL_UNUSED none_vec4  = {{ 0.0f, 0.0f, 1.0f, 0.0f }};

/* The list of all `gui-editors`. */
guieditor *openeditor = NULL;
/* The first open `gui-editor`. */
guieditor *starteditor = NULL;

/* The main structure that holds all the data the gui needs. */
guistruct *gui = NULL;

#define FALLBACK_FONT_PATH "/usr/share/root/fonts/monotype.ttf"
#define FALLBACK_FONT "fallback"
#define JETBRAINS_REGULAR_FONT_PATH "/home/mellw/.vscode-insiders/extensions/narasimapandiyan.jetbrainsmono-1.0.2/JetBrainsMono/JetBrainsMono-Regular.ttf"
#define JETBRAINS_REGULAR_FONT "jetbrains regular"

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
  /* Load fallback font. */
  if (is_file_and_exists(FALLBACK_FONT_PATH)) {
    ;
  }
  else {
    glfwDestroyWindow(gui->window);
    glfwTerminate();
    glDeleteProgram(gui->font_shader);
    die("Failed to find fallback font: '%s' does not exist.\n");
  }
  /* Look for jetbrains regular font. */
  if (is_file_and_exists(JETBRAINS_REGULAR_FONT_PATH)) {
    gui->atlas = texture_atlas_new(512, 512, 1);
    glGenTextures(1, &gui->atlas->id);
    gui->font = texture_font_new_from_file(gui->atlas, gui->font_size, JETBRAINS_REGULAR_FONT_PATH);
  }
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
        /* Output. */
        out vec4 vertexColor;
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

/* Setup the top menu bar. */
static void setup_top_bar(void) {
  gui->topbuf = vertex_buffer_new("vertex:3f,tex_coord:2f,color:4f");
  /* Make the top bar parent element. */
  gui->topbar = make_element(
    0.0f,
    vec2(gui->width, FONT_HEIGHT(gui->font)),
    0.0f,
    vec4(vec3(0.1f), 1.0f)
  );
  /* Make file menu element. */
  file_menu_element = make_element(
    0.0f,
    vec2(pixel_breadth(gui->font, " File "), gui->topbar->size.h),
    0.0f,
    vec4(vec3(0.0f), 1.0f)
  );
  set_element_lable(file_menu_element, " File ");
  file_menu_element->textcolor = vec4(1.0f);
  file_menu_element->parent    = gui->topbar;
  gui->topbar->children.push_back(file_menu_element);
  file_menu_element->callback = [](guielement *self, guielement_callback_type type) {
    if (type == GUIELEMENT_ENTER_CALLBACK) {
      file_menu_element->color     = vec4(1.0f);
      file_menu_element->textcolor = vec4(vec3(0.0f), 1.0f);
      /* Show all children of the file menu. */
      for (auto child : file_menu_element->children) {
        child->flag.unset<GUIELEMENT_HIDDEN>();
      }
    }
    else if (type == GUIELEMENT_LEAVE_CALLBACK) {
      file_menu_element->color     = vec4(vec3(0.0f), 1.0f);
      file_menu_element->textcolor = vec4(1.0f);
      /* Only close all this when the currently entered window is not file_menu_element or its children. */
      if (!is_ancestor(element_from_mousepos(), file_menu_element)) {
        for (auto child : file_menu_element->children) {
          child->flag.set<GUIELEMENT_HIDDEN>();
        }
      }
      refresh_needed = TRUE;  
    }
  };
  /* Make the open file element in the file menu. */
  open_file_element = make_element(
    vec2(file_menu_element->pos.x, (file_menu_element->pos.y + file_menu_element->size.h)),
    vec2(pixel_breadth(gui->font, " Open File "), gui->topbar->size.h),
    0.0f,
    file_menu_element->color
  );
  set_element_lable(open_file_element, " Open File ");
  open_file_element->textcolor = vec4(1.0f);
  open_file_element->flag.set<GUIELEMENT_HIDDEN>();
  open_file_element->parent = file_menu_element;
  file_menu_element->children.push_back(open_file_element);
  open_file_element->callback = [](guielement *self, guielement_callback_type type) {
    if (type == GUIELEMENT_ENTER_CALLBACK) {
      open_file_element->color     = vec4(1.0f);
      open_file_element->textcolor = vec4(vec3(0.0f), 1.0f);  
    }
    else if (type == GUIELEMENT_LEAVE_CALLBACK) {
      open_file_element->color     = vec4(vec3(0.0f), 1.0f);
      open_file_element->textcolor = vec4(1.0f);
      if (!is_ancestor(element_from_mousepos(), file_menu_element)) {
        open_file_element->flag.set<GUIELEMENT_HIDDEN>();
      }
      refresh_needed = TRUE;  
    }
  };
}

/* Set up the bottom bar. */
static void setup_botbar(void) {
  gui->botbuf = vertex_buffer_new("vertex:3f,tex_coord:2f,color:4f");
  gui->botbar = make_element(
    vec2(0, gui->height),
    vec2(gui->height, FONT_HEIGHT(gui->font)),
    0.0f,
    color_idx_to_vec4(FG_VS_CODE_RED)
  );
  gui->botbar->flag.set<GUIELEMENT_HIDDEN>();
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
  gui = (guistruct *)nmalloc(sizeof(*gui));
  /* Then init all fields to something invalid, this is important if we need to abort. */
  gui->title       = NULL;
  gui->width       = 0;
  gui->height      = 0;
  gui->window      = NULL;
  gui->flag        = bit_flag_t<8>();
  gui->handler     = NULL;
  gui->topbar      = NULL;
  gui->topbuf      = NULL;
  gui->botbar      = NULL;
  gui->botbuf      = NULL;
  gui->entered     = NULL;
  gui->projection  = NULL;
  gui->font_shader = 0;
  gui->uifont      = NULL;
  gui->uifont_size = 0;
  gui->font        = NULL;
  gui->font_size   = 0;
  gui->atlas       = NULL;
  gui->rect_shader = 0;
}

/* Init the gui struct, it reprecents everything that the gui needs. */
static void init_guistruct(const char *win_title, Uint win_width, Uint win_height, Uint fps, Uint font_size) {
  /* Create the fully blank guistruct. */
  make_guistruct();
  /* Set the basic data needed to init the window. */
  gui->title      = copy_of(win_title);
  gui->width      = win_width;
  gui->height     = win_height;
  gui->projection = (matrix4x4 *)nmalloc(sizeof(*gui->projection));
  /* Then create the glfw window. */
  gui->window = glfwCreateWindow(gui->width, gui->height, gui->title, NULL, NULL);
  if (!gui->window) {
    glfwTerminate();
    die("Failed to create glfw window.\n");
  }
  glfwMakeContextCurrent(gui->window);
  frametimer.fps = fps;
  /* Create and start the event handler. */
  gui->handler = nevhandler_create();
  nevhandler_start(gui->handler, TRUE);
  gui->font_size = font_size;
}

static void delete_guistruct(void) {
  free(gui->title);
  glfwDestroyWindow(gui->window);
  glfwTerminate();
  if (gui->font_shader) {
    glDeleteProgram(gui->font_shader);
  }
  if (gui->rect_shader) {
    glDeleteProgram(gui->rect_shader);
  }
  delete_element(gui->topbar);
  delete_element(gui->botbar);
  if (gui->atlas) {
    glDeleteTextures(1, &gui->atlas->id);
    gui->atlas->id = 0;
    texture_atlas_delete(gui->atlas);
  }
  if (gui->topbuf) {
    vertex_buffer_delete(gui->topbuf);
  }
  if (gui->botbuf) {
    vertex_buffer_delete(gui->botbuf);
  }
  texture_font_delete(gui->font);
  nevhandler_stop(gui->handler, 0);
  nevhandler_free(gui->handler);
}

/* Cleanup before exit. */
static void cleanup(void) {
  delete_editor(openeditor);
  delete_guistruct();
}

/* Init glew and check for errors.  Terminates on fail to init glew. */
static void init_glew(void) {
  /* Enable glew experimental features. */
  glewExperimental = TRUE;
  Uint err = glewInit();
  /* If we could not init glew, terminate directly. */
  if (err != GLEW_OK) {
    glfwDestroyWindow(gui->window);
    glfwTerminate();
    die("GLEW: ERROR: %s\n", glewGetErrorString(err));
  }
  fprintf(stderr, "Using GLEW %s\n", glewGetString(GLEW_VERSION));
}

/* Init glfw. */
void init_gui(void) {
  /* Init glfw. */
  if (!glfwInit()) {
    die("Failed to init glfw.\n");
  }
  /* Init the main gui structure. */
  init_guistruct("NanoX", 1200, 800, 120, 17);
  /* Init glew. */
  init_glew();
  /* Init the font shader. */
  setup_font_shader();
  /* Init the rect shader. */
  setup_rect_shader();
  /* Init the top menu bar. */
  setup_top_bar();
  /* Init the bottom bar, that will be used for status updates, among other thing. */
  setup_botbar();
  /* Init the edit element. */
  setup_edit_element();
  /* Set some callbacks. */
  glfwSetWindowSizeCallback(gui->window, window_resize_callback);
  glfwSetKeyCallback(gui->window, key_callback);
  glfwSetCharCallback(gui->window, char_callback);
  glfwSetWindowMaximizeCallback(gui->window, window_maximize_callback);
  glfwSetMouseButtonCallback(gui->window, mouse_button_callback);
  glfwSetCursorPosCallback(gui->window, mouse_pos_callback);
  glfwSetCursorEnterCallback(gui->window, window_enter_callback);
  glfwSetScrollCallback(gui->window, scroll_callback);
  frametimer.fps = 120;
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
    /* Draw the edit element. */
    // draw_editelement();
    draw_editor(openeditor);
    /* Draw the top menu bar. */
    draw_top_bar();
    /* Draw the bottom bar, if there is any status messages. */
    draw_botbar();
    /* If refresh was needed it has been done so set it to FALSE. */
    refresh_needed = FALSE;
    glfwSwapBuffers(gui->window);
    glfwPollEvents();
    frametimer.end();
  }
  cleanup();
  close_and_go();
}

#endif
