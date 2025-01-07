#include "../include/prototypes.h"

#ifdef HAVE_GLFW

/* Main window for nanox. */
static GLFWwindow *window = NULL;
/* Window dimentions. */
Uint window_width  = 1200;
Uint window_height = 800;
/* Holds the current fontsize for the editwindow. */
static _GL_UNUSED Uint edit_fontsize = 17;
/* Window title.  This may be changed so its dynamic. */
static const char *window_title = "NanoX";
/* Rect shader data. */
Uint fontshader, rectshader;
static glSsbo<vec2, 0> vertices_ssbo;
static glSsbo<Uint, 1> indices_ssbo;
/* The projection that all shaders follow. */
mat4 projection;
static mat4 view;
static mat4 model;
/* Hash based grid map for elements. */
uigridmapclass gridmap(GRIDMAP_GRIDSIZE);
/* Frame timer to keep a frame rate. */
frametimerclass frametimer;
/* The main edit element where the text is edited. */
uielementstruct *editelement = NULL;
/* The gutter of the main edit element. */
uielementstruct *gutterelement = NULL;
/* The top menu bar element. */
uielementstruct *top_bar = NULL;
/* The file menu button element. */
uielementstruct *file_menu_element = NULL;
uielementstruct *open_file_element = NULL;

/* Gui flags. */
bit_flag_t<8> guiflag;

markup_t markup;
texture_atlas_t *atlas = NULL;
vertex_buffer_t *vertbuf = NULL;
vertex_buffer_t *topbuf = NULL;
vec2 pen;

static vec4 _GL_UNUSED black_vec4 = { 0.0f, 0.0f, 0.0f, 1.0f };
static vec4 _GL_UNUSED white_vec4 = { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4 _GL_UNUSED none_vec4  = { 0.0f, 0.0f, 1.0f, 0.0f };

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

uielementstruct *make_element(vec2 pos, vec2 size, vec2 endoff, vec4 color) _NOTHROW {
  uielementstruct *newelement = (uielementstruct *)nmalloc(sizeof(*newelement));
  newelement->pos       = pos;
  newelement->size      = size;
  newelement->endoff    = endoff;
  newelement->color     = color;
  newelement->textcolor = 0.0f;
  newelement->lable     = NULL;
  newelement->lablelen  = 0;
  newelement->callback  = NULL;
  newelement->parent    = NULL;
  newelement->children  = MVector<uielementstruct*>{};
  gridmap.set(newelement);
  newelement->flag.clear();
  return newelement;
}

/* Set an elements lable text. */
void set_element_lable(uielementstruct *element, const char *string) _NOTHROW {
  if (element->flag.is_set<UIELEMENT_HAS_LABLE>()) {
    free(element->lable);
  }
  element->lable    = copy_of(string);
  element->lablelen = strlen(string);
  element->flag.set<UIELEMENT_HAS_LABLE>();
}

/* Init font shader and buffers. */
static void setup_font_shader(void) {
  /* Create shader. */
  fontshader = openGL_create_shader_program_raw({
    /* Font vertex shader. */
    { STRLITERAL(\
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        attribute vec3 vertex;
        attribute vec2 tex_coord;
        attribute vec4 color;
        void main() {
          gl_TexCoord[0].xy = tex_coord.xy;
          gl_FrontColor     = color;
          gl_Position       = projection * (view * (model * vec4(vertex, 1.0)));
        }
      ),
      GL_VERTEX_SHADER },
    /* Font fragment shader. */
    { STRLITERAL(\
        uniform sampler2D texture;
        void main() {
          float a = texture2D(texture, gl_TexCoord[0].xy).r;
          gl_FragColor = vec4(gl_Color.rgb, gl_Color.a * a);
        }
      ),
      GL_FRAGMENT_SHADER }
  });
  /* If there is a problem with the shader creation just die as we are
   * passing string literals so there should not be alot that can go wrong. */
  if (!fontshader) {
    glfwDestroyWindow(window);
    glfwTerminate();
    die("Failed to create font shader.");
  }
  /* Setup projection. */
  glUseProgram(fontshader); {
    glUniformMatrix4fv(glGetUniformLocation(fontshader, "model"), 1, 0, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(fontshader, "view"), 1, 0, &view[0][0]);
  }
  /* Load fallback font. */
  if (is_file_and_exists(FALLBACK_FONT_PATH)) {
    ;
  }
  else {
    glfwDestroyWindow(window);
    glfwTerminate();
    glDeleteProgram(fontshader);
    die("Failed to find fallback font: '%s' does not exist.");
  }
  vertbuf = vertex_buffer_new("vertex:3f,tex_coord:2f,color:4f");
  /* Look for jetbrains regular font. */
  if (is_file_and_exists(JETBRAINS_REGULAR_FONT_PATH)) {
    atlas = texture_atlas_new(512, 512, 1);
    glGenTextures(1, &atlas->id);
    markup.family              = copy_of(JETBRAINS_REGULAR_FONT),
    markup.size                = edit_fontsize;
    markup.bold                = FALSE;
    markup.italic              = FALSE;
    markup.spacing             = 0.0f;
    markup.gamma               = 1.0f;
    markup.foreground_color    = white_vec4;
    markup.background_color    = none_vec4;
    markup.underline           = 0;
    markup.underline_color     = white_vec4;
    markup.overline            = 0;
    markup.overline_color      = white_vec4;
    markup.strikethrough       = 0;
    markup.strikethrough_color = white_vec4;
    markup.font = texture_font_new_from_file(atlas, edit_fontsize, JETBRAINS_REGULAR_FONT_PATH);
  }
}

/* Init the rect shader and setup the ssbo`s for indices and vertices. */
static void setup_rect_shader(void) {
  rectshader = openGL_create_shader_program_raw({
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
  topbuf = vertex_buffer_new("vertex:3f,tex_coord:2f,color:4f");
  /* Make the top bar parent element. */
  top_bar = make_element(
    0.0f,
    vec2(window_width, FONT_HEIGHT(markup.font)),
    0.0f,
    vec4(vec3(0.1f), 1.0f)
  );
  /* Make file menu element. */
  file_menu_element = make_element(
    0.0f,
    vec2(pixel_breadth(markup.font, " File "), top_bar->size.h),
    0.0f,
    vec4(vec3(0.0f), 1.0f)
  );
  set_element_lable(file_menu_element, " File ");
  file_menu_element->textcolor = vec4(1.0f);
  file_menu_element->parent    = top_bar;
  top_bar->children.push_back(file_menu_element);
  file_menu_element->callback = [](int type) {
    if (type == UIELEMENT_ENTER_CALLBACK) {
      file_menu_element->color     = vec4(1.0f);
      file_menu_element->textcolor = vec4(vec3(0.0f), 1.0f);
      /* Show all children of the file menu. */
      for (auto child : file_menu_element->children) {
        child->flag.unset<UIELEMENT_HIDDEN>();
      }
    }
    else if (type == UIELEMENT_LEAVE_CALLBACK) {
      file_menu_element->color     = vec4(vec3(0.0f), 1.0f);
      file_menu_element->textcolor = vec4(1.0f);
      /* Only close all this when the currently entered window is not file_menu_element or its children. */
      if (!is_ancestor(element_from_mousepos(), file_menu_element)) {
        for (auto child : file_menu_element->children) {
          child->flag.set<UIELEMENT_HIDDEN>();
        }
      }
      refresh_needed = TRUE;  
    }
  };
  /* Make the open file element in the file menu. */
  open_file_element = make_element(
    vec2(file_menu_element->pos.x, (file_menu_element->pos.y + file_menu_element->size.h)),
    vec2(pixel_breadth(markup.font, " Open File "), top_bar->size.h),
    0.0f,
    file_menu_element->color
  );
  set_element_lable(open_file_element, " Open File ");
  open_file_element->textcolor = vec4(1.0f);
  open_file_element->flag.set<UIELEMENT_HIDDEN>();
  open_file_element->parent = file_menu_element;
  file_menu_element->children.push_back(open_file_element);
  open_file_element->callback = [](int type) {
    if (type == UIELEMENT_ENTER_CALLBACK) {
      open_file_element->color     = vec4(1.0f);
      open_file_element->textcolor = vec4(vec3(0.0f), 1.0f);  
    }
    else if (type == UIELEMENT_LEAVE_CALLBACK) {
      open_file_element->color     = vec4(vec3(0.0f), 1.0f);
      open_file_element->textcolor = vec4(1.0f);
      if (!is_ancestor(element_from_mousepos(), file_menu_element)) {
        open_file_element->flag.set<UIELEMENT_HIDDEN>();
      }
      refresh_needed = TRUE;  
    }
  };
}

/* Allocate and init the edit element. */
static void setup_edit_element(void) {
  /* Confirm the margin first to determen how wide the gutter has to be. */
  confirm_margin();
  /* Create the gutter and the edit element for the main edit portion. */
  gutterelement = make_element(
    vec2(0.0f, top_bar->size.h),
    vec2(((markup.font->face->max_advance_width >> 6) * margin + 1), window_height),
    0.0f,
    vec4(vec3(0.2f), 1.0f)
  );
  editelement = make_element(
    vec2(gutterelement->size.w, top_bar->size.h),
    vec2(window_width, window_height),
    0.0f,
    vec4(vec3(0.2f), 1.0f)
  );
}

/* Cleanup before exit. */
static void cleanup(void) {
  glfwDestroyWindow(window);
  glfwTerminate();
  glDeleteProgram(fontshader);
  glDeleteProgram(rectshader);
  free(open_file_element);
  free(file_menu_element);
  free(top_bar);
  free(editelement);
  free(gutterelement);
  free(markup.family);
  glDeleteTextures(1, &atlas->id);
  atlas->id = 0;
  texture_atlas_delete(atlas);
  vertex_buffer_delete(vertbuf);
  vertex_buffer_delete(topbuf);
}

/* Init glew and check for errors.  Terminates on fail to init glew. */
void init_glew(void) {
  /* Enable glew experimental features. */
  glewExperimental = TRUE;
  Uint err = glewInit();
  /* If we could not init glew, terminate directly. */
  if (err != GLEW_OK) {
    glfwDestroyWindow(window);
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
  window = glfwCreateWindow(window_width, window_height, window_title, NULL, NULL);
  if (!window) {
    glfwTerminate();
    die("Failed to create glfw window.\n");
  }
  glfwMakeContextCurrent(window);
  /* Init glew. */
  init_glew();
  /* Init the font shader. */
  setup_font_shader();
  /* Init the rect shader. */
  setup_rect_shader();
  /* Init the top menu bar. */
  setup_top_bar();
  /* Init the edit element. */
  setup_edit_element();
  /* Set some callbacks. */
  glfwSetWindowSizeCallback(window, window_resize_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCharCallback(window, char_callback);
  glfwSetWindowMaximizeCallback(window, window_maximize_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_pos_callback);
  glfwSetCursorEnterCallback(window, window_enter_callback);
  glfwSetScrollCallback(window, scroll_callback);
  frametimer.fps = 120;
  // glClearColor(1.00, 1.00, 1.00, 1.00);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  /* Set the window size. */
  window_resize_callback(window, window_width, window_height);
}

/* Main gui loop. */
void glfw_loop(void) {
  guiflag.set<GUI_RUNNING>();
  while (!glfwWindowShouldClose(window) && guiflag.is_set<GUI_RUNNING>()) {
    frametimer.start();
    confirm_margin();
    place_the_cursor();
    glClear(GL_COLOR_BUFFER_BIT);
    /* Draw the edit element. */
    draw_editelement();
    /* Draw the top menu bar. */
    draw_top_bar();
    /* If refresh was needed it has been done so set it to FALSE. */
    refresh_needed = FALSE;
    glfwSwapBuffers(window);
    glfwPollEvents();
    frametimer.end();
  }
  cleanup();
}

#endif