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
/* Gui flags. */
bit_flag_t<8> guiflag;

markup_t font[2];
texture_atlas_t *atlas = NULL;
vertex_buffer_t *vertbuf = NULL;
vec2 pen;

static fvector4 _GL_UNUSED black_vec4 = {{0.0f, 0.0f, 0.0f, 1.0f}};
static fvector4 _GL_UNUSED white_vec4 = {{1.0f, 1.0f, 1.0f, 1.0f}};
static fvector4 _GL_UNUSED none_vec4  = {{0.0f, 0.0f, 1.0f, 0.0f}};

#define FALLBACK_FONT_PATH "/usr/share/root/fonts/monotype.ttf"
#define FALLBACK_FONT "fallback"
#define JETBRAINS_REGULAR_FONT_PATH "/home/mellw/.vscode-insiders/extensions/narasimapandiyan.jetbrainsmono-1.0.2/JetBrainsMono/JetBrainsMono-Regular.ttf"
#define JETBRAINS_REGULAR_FONT "jetbrains regular"

/* Define the vertices of a square (centered at the origin) */
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
  update_projection_uniform(fontshader);
  glUniformMatrix4fv(glGetUniformLocation(fontshader, "model"), 1, 0, &model[0][0]);
  glUniformMatrix4fv(glGetUniformLocation(fontshader, "view"), 1, 0, &view[0][0]);
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
    font[GUI_NORMAL_TEXT].family              = copy_of(JETBRAINS_REGULAR_FONT),
    font[GUI_NORMAL_TEXT].size                = edit_fontsize;
    font[GUI_NORMAL_TEXT].bold                = FALSE;
    font[GUI_NORMAL_TEXT].italic              = FALSE;
    font[GUI_NORMAL_TEXT].spacing             = 0.0f;
    font[GUI_NORMAL_TEXT].gamma               = 1.0f;
    font[GUI_NORMAL_TEXT].foreground_color    = white_vec4;
    font[GUI_NORMAL_TEXT].background_color    = none_vec4;
    font[GUI_NORMAL_TEXT].underline           = 0;
    font[GUI_NORMAL_TEXT].underline_color     = white_vec4;
    font[GUI_NORMAL_TEXT].overline            = 0;
    font[GUI_NORMAL_TEXT].overline_color      = white_vec4;
    font[GUI_NORMAL_TEXT].strikethrough       = 0;
    font[GUI_NORMAL_TEXT].strikethrough_color = white_vec4;
    font[GUI_NORMAL_TEXT].font = texture_font_new_from_file(atlas, edit_fontsize, JETBRAINS_REGULAR_FONT_PATH);
    font[GUI_SELECTED_TEXT] = font[GUI_NORMAL_TEXT];
    font[GUI_SELECTED_TEXT].foreground_color = {{ 0.0f, 0.0f, 0.5f, 1.0f }};
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
        #version 450 core \n
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
  /* Setup projection for the rect shader. */
  update_projection_uniform(rectshader);
}

/* Allocate and init the edit element. */
static void setup_edit_element(void) {
  /* Confirm the margin first to determen how wide the gutter has to be. */
  confirm_margin();
  /* Place the cursor at the correct position. */
  place_the_cursor();
  /* Alloc and init the gutter. */
  gutterelement = (uielementstruct *)nmalloc(sizeof(*gutterelement));
  gutterelement->pos    = 0.0f;
  gutterelement->size   = vec2(((font[GUI_NORMAL_TEXT].font->face->max_advance_width >> 6) * margin + 1), window_height);
  gutterelement->endoff = 0.0f;
  gridmap.set(gutterelement);
  /* Alloc and init the edit element. */
  editelement = (uielementstruct *)nmalloc(sizeof(*editelement));
  editelement->pos    = vec2(gutterelement->size.w, 0.0f);
  editelement->size   = vec2(window_width, window_height);
  editelement->endoff = 0.0f;
  gridmap.set(editelement);
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
  if (glewInit() != GLEW_OK) {
    glfwDestroyWindow(window);
    glfwTerminate();
    die("Failed to init glew.\n");
  }
  /* Init the font shader. */
  setup_font_shader();
  /* Init the rect shader. */
  setup_rect_shader();
  /* Init the edit element. */
  setup_edit_element();
  /* Set some callbacks. */
  glfwSetWindowSizeCallback(window, window_resize_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCharCallback(window, char_callback);
  glfwSetWindowMaximizeCallback(window, window_maximize_callback);
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
    glfwSwapBuffers(window);
    glfwPollEvents();
    frametimer.end();
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  glDeleteProgram(fontshader);
  glDeleteProgram(rectshader);
  free(editelement);
  free(gutterelement);
  free(font[GUI_NORMAL_TEXT].family);
  glDeleteTextures(1, &atlas->id);
  atlas->id = 0;
  texture_atlas_delete(atlas);
}

#endif