/** @file gui/shader.c

  @author  Melwin Svensson.
  @date    13-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define SHADER_FONT_VERTEX_DATA                                           \
  /* Font vertex shader openGL 2.0. */                                    \
  "uniform mat4 projection;"                                        "\n"  \
  "attribute vec2 vertex;"                                          "\n"  \
  "attribute vec2 tex_coord;"                                       "\n"  \
  "attribute vec4 color;"                                           "\n"  \
  "void main() {"                                                   "\n"  \
  "  gl_TexCoord[0].xy = tex_coord.xy;"                             "\n"  \
  "  gl_FrontColor     = color;"                                    "\n"  \
  "  gl_Position       = (projection * vec4(vertex, 0.0f, 1.0f));"  "\n"  \
  "}"                                                               "\n"

#define SHADER_FONT_FRAGMENT_DATA                                 \
  /* Font fragment shader openGL 2.0. */                          \
  "uniform sampler2D texture;"                              "\n"  \
  "void main() {"                                           "\n"  \
  "  float a = texture2D(texture, gl_TexCoord[0].xy).r;"    "\n"  \
  "  gl_FragColor = vec4(gl_Color.rgb, (gl_Color.a * a));"  "\n"  \
  "}"                                                       "\n"

#define SHADER_RECT_VERTEX_DATA                                       \
  /* Rect vertex shader openGL 2.0. */                                \
  "uniform mat4 projection;"                                    "\n"  \
  "attribute vec2 vertex;"                                      "\n"  \
  "attribute vec4 color;"                                       "\n"  \
  "void main() {"                                               "\n"  \
  "  gl_FrontColor = color;"                                    "\n"  \
  "  gl_Position   = (projection * vec4(vertex, 0.0f, 1.0f));"  "\n"  \
  "}"                                                           "\n"

#define SHADER_RECT_FRAGMENT_DATA         \
  /* Rect fragment shader openGL 2.0. */  \
  "void main() {"                   "\n"  \
  "  gl_FragColor = gl_Color;"      "\n"  \
  "}"                               "\n"

#define SHADER_FONT_VERT_DATA_130                                   \
  /* Font vertex shader openGL 3.0. */                              \
  "#version 130"                                              "\n"  \
  "uniform mat4 projection;"                                  "\n"  \
  "in vec2 vertex;"                                           "\n"  \
  "in vec2 tex_coord;"                                        "\n"  \
  "in vec4 color;"                                            "\n"  \
  "out vec2 f_tex_coord;"                                     "\n"  \
  "out vec4 f_color;"                                         "\n"  \
  "void main() {"                                             "\n"  \
  "  f_tex_coord = tex_coord;"                                "\n"  \
  "  f_color     = color;"                                    "\n"  \
  "  gl_Position = (projection * vec4(vertex, 0.0f, 1.0f));"  "\n"  \
  "}"                                                         "\n"

#define SHADER_FONT_FRAG_DATA_130                             \
  /* Font fragment shader openGL 3.0. */                      \
  "#version 130"                                        "\n"  \
  "in vec2 f_tex_coord;"                                "\n"  \
  "in vec4 f_color;"                                    "\n"  \
  "out vec4 frag_color;"                                "\n"  \
  "uniform sampler2D tex;"                              "\n"  \
  "void main() {"                                       "\n"  \
  "  float a   = texture2D(tex, f_tex_coord).r;"        "\n"  \
  "  frag_color = vec4(f_color.rgb, (f_color.a * a));"  "\n"  \
  "}"                                                   "\n"

#define SHADER_RECT_VERT_DATA_130                                   \
  /* Rect vertex shader openGL 3.0. */                        "\n"  \
  "#version 130"                                              "\n"  \
  "uniform mat4 projection;"                                  "\n"  \
  "in vec2 vertex;"                                           "\n"  \
  "in vec4 color;"                                            "\n"  \
  "out vec4 f_color;"                                         "\n"  \
  "void main() {"                                             "\n"  \
  "  f_color = color;"                                        "\n"  \
  "  gl_Position = (projection * vec4(vertex, 0.0f, 1.0f));"  "\n"  \
  "}"                                                         "\n"

#define SHADER_RECT_FRAG_DATA_130         \
  /* Rect fragment shader openGL 3.0. */  \
  "#version 130"                    "\n"  \
  "in vec4 f_color;"                "\n"  \
  "out vec4 frag_color;"            "\n"  \
  "void main() {"                   "\n"  \
  "  frag_color = f_color;"         "\n"  \
  "}"                               "\n"

#define HACK_FONT_PATH     "/usr/share/fonts/TTF/Hack-Regular.ttf"
#define DEFAULT_FONT_PATH  HACK_FONT_PATH

/* GLSL openGL table:
 *   `GLSL/openGL` --- `Release Year`.
 *    `110 --- 2.0` --- `2004`.
 *    `120 --- 2.1` --- `2006`.
 *    `130 --- 3.0` --- `2008`.
 *    `140 --- 3.1` --- `2009`.
 *    `150 --- 3.2` --- `2009`.
 *    `330 --- 3.3` --- `2010`.
 *    `400 --- 4.0` --- `2010`.
 *    `410 --- 4.1` --- `2010`.
 *    `420 --- 4.2` --- `2011`.
 *    `430 --- 4.3` --- `2012`.
 *    `440 --- 4.4` --- `2013`.
 *    `450 --- 4.5` --- `2014`.
 *    `460 --- 4.6` --- `2017`.
 */
#define USING_OPENGL_CORE  FALSE
#define OPENGL_MAJOR 3
#define OPENGL_MINOR 0


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Shader load ----------------------------- */

/* Returns a compiled part of a openGL shader. */
_NODISCARD _NONNULL(2)
static Uint shader_load(Uint type, const char *const data) {
  ASSERT(data);
  int success;
  int loglen;
  char *log;
  Uint ret = glCreateShader(type);
  glShaderSource(ret, 1, &data, NULL);
  glCompileShader(ret);
  glGetShaderiv(ret, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderiv(ret, GL_INFO_LOG_LENGTH, &loglen);
    log = xmalloc(loglen);
    glGetShaderInfoLog(ret, loglen, NULL, log);
    writeferr("Failed to compile shader.\n\nError log: %s\n", log);
    free(log);
  }
  return ret;
}

/* ----------------------------- Shader create ----------------------------- */

/* Returns a linked openGL program based on `parts`. */
_NODISCARD _NONNULL(2)
static Uint shader_create(Ulong len, Uint *const parts) {
  ASSERT(parts);
  ASSERT(len > 0);
  int success;
  int loglen;
  char *log;
  Uint ret = glCreateProgram();
  for (Ulong i=0; i<len; ++i) {
    glAttachShader(ret, parts[i]);
  }
  glLinkProgram(ret);
  glGetProgramiv(ret, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramiv(ret, GL_INFO_LOG_LENGTH, &loglen);
    log = xmalloc(loglen);
    glGetProgramInfoLog(ret, loglen, NULL, log);
    writeferr("Failed to link shader program.\n\nError log: %s\n", log);
    free(log);
    return 0;
  }
  return ret;
}

/* ----------------------------- Shader set openGL version ----------------------------- */

/* Inform glfw about the openGL version we are using. */
static inline void shader_set_openGL_version(void) {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MINOR);
  if (USING_OPENGL_CORE) {
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Shader font create ----------------------------- */

/* Create the font shader for the gui. */
void shader_font_create(void) {
  font_shader = shader_create(2, (Uint[]) {
    shader_load(GL_VERTEX_SHADER,   SHADER_FONT_VERT_DATA_130),
    shader_load(GL_FRAGMENT_SHADER, SHADER_FONT_FRAG_DATA_130)
  });
}

/* ----------------------------- Shader rect create ----------------------------- */

/* Create the rect shader for the gui. */
void shader_rect_create(void) {
  rect_shader = shader_create(2, (Uint[]) {
    shader_load(GL_VERTEX_SHADER,   SHADER_RECT_VERT_DATA_130),
    shader_load(GL_FRAGMENT_SHADER, SHADER_RECT_FRAG_DATA_130)
  });
}

/* ----------------------------- Shader compile ----------------------------- */

/* Compile the `font-shader` and the `rect-shader` and then load load the fonts. */
void shader_compile(void) {
  /* Create the font shader. */
  shader_font_create();
  /* If we fail to compile the font-shader, we must terminate. */
  if (!font_shader) {
    glfwDestroyWindow(gui_window);
    glfwTerminate();
    die("Failed to compile the font shader, this is fatal.\n");
  }
  /* Create both fonts. */
  textfont = font_create();
  uifont   = font_create();
  /* Then load the default font. */
  font_load(textfont, DEFAULT_FONT_PATH, 17, 4096);
  font_load(uifont,   DEFAULT_FONT_PATH, 15, 2048);
  /* Create the rect-shader, here we can run with a failed compilation. */
  shader_rect_create();
  if (!rect_shader) {
    writeferr("Failed to comile the rect shader.  We can continue without this, but we only have text.\n");
  }
  shader_set_openGL_version();
}

/* ----------------------------- Shader rect vertex load ----------------------------- */

/* Takes a `RectVertex[4]` as `buf`. */
void shader_rect_vertex_load(RectVertex *buf, float x, float y, float w, float h, Uint color) {
  UNPACK_FUINT_VARS(color, r, g, b, a);
  buf[0] = (RectVertex){x, y,             r,g,b,a};
  buf[1] = (RectVertex){(x + w), y,       r,g,b,a};
  buf[2] = (RectVertex){(x + w), (y + h), r,g,b,a};
  buf[3] = (RectVertex){x, (y + h),       r,g,b,a};
}

/* ----------------------------- Shader rect vertex load array ----------------------------- */

void shader_rect_vertex_load_array(RectVertex *buf, float *const array, Uint color) {
  shader_rect_vertex_load(buf, array[0], array[1], array[2], array[3], color);
}
