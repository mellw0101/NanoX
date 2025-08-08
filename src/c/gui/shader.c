/** @file gui/shader.c

@author  Melwin Svensson.
@date    13-7-2025.

*/
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


/* Font shader openGL 2.0. */
#define SHADER_FONT_VERT_DATA_120                                         \
  "uniform mat4 projection;"                                        "\n"  \
  "attribute vec2 vertex;"                                          "\n"  \
  "attribute vec2 tex_coord;"                                       "\n"  \
  "attribute vec4 color;"                                           "\n"  \
  "void main() {"                                                   "\n"  \
  "  gl_TexCoord[0].xy = tex_coord.xy;"                             "\n"  \
  "  gl_FrontColor     = color;"                                    "\n"  \
  "  gl_Position       = (projection * vec4(vertex, 0.0f, 1.0f));"  "\n"  \
  "}"                                                               "\n"
#define SHADER_FONT_FRAG_DATA_120                                 \
  "uniform sampler2D texture;"                              "\n"  \
  "void main() {"                                           "\n"  \
  "  float a = texture2D(texture, gl_TexCoord[0].xy).r;"    "\n"  \
  "  gl_FragColor = vec4(gl_Color.rgb, (gl_Color.a * a));"  "\n"  \
  "}"                                                       "\n"

/* Font shader openGL 3.0. */
#define SHADER_FONT_VERT_DATA_130                                   \
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
  "#version 130"                                        "\n"  \
  "in vec2 f_tex_coord;"                                "\n"  \
  "in vec4 f_color;"                                    "\n"  \
  "out vec4 frag_color;"                                "\n"  \
  "uniform sampler2D tex;"                              "\n"  \
  "void main() {"                                       "\n"  \
  "  float a   = texture2D(tex, f_tex_coord).r;"        "\n"  \
  "  frag_color = vec4(f_color.rgb, (f_color.a * a));"  "\n"  \
  "}"                                                   "\n"

/* Font shader openGL 3.3. */
#define SHADER_FONT_VERT_DATA_330                                   \
  "#version 330 core"                                         "\n"  \
  "uniform mat4 projection;"                                  "\n"  \
  "layout (location = 0) in vec2 vertex;"                     "\n"  \
  "layout (location = 1) in vec2 tex_coord;"                  "\n"  \
  "layout (location = 2) in vec4 color;"                      "\n"  \
  "out vec2 f_tex_coord;"                                     "\n"  \
  "out vec4 f_color;"                                         "\n"  \
  "void main() {"                                             "\n"  \
  "  f_tex_coord = tex_coord;"                                "\n"  \
  "  f_color     = color;"                                    "\n"  \
  "  gl_Position = (projection * vec4(vertex, 0.0f, 1.0f));"  "\n"  \
  "}"                                                         "\n"
#define SHADER_FONT_FRAG_DATA_330                             \
  "#version 330 core"                                   "\n"  \
  "in vec2 f_tex_coord;"                                "\n"  \
  "in vec4 f_color;"                                    "\n"  \
  "out vec4 frag_color;"                                "\n"  \
  "uniform sampler2D tex;"                              "\n"  \
  "void main() {"                                       "\n"  \
  "  float a   = texture(tex, f_tex_coord).r;"          "\n"  \
  "  frag_color = vec4(f_color.rgb, (f_color.a * a));"  "\n"  \
  "}"                                                   "\n"

/* Rect shader openGL 2.0. */
#define SHADER_RECT_VERT_DATA_120                                     \
  "uniform mat4 projection;"                                    "\n"  \
  "attribute vec2 vertex;"                                      "\n"  \
  "attribute vec4 color;"                                       "\n"  \
  "void main() {"                                               "\n"  \
  "  gl_FrontColor = color;"                                    "\n"  \
  "  gl_Position   = (projection * vec4(vertex, 0.0f, 1.0f));"  "\n"  \
  "}"                                                           "\n"
#define SHADER_RECT_FRAG_DATA_120         \
  "void main() {"                   "\n"  \
  "  gl_FragColor = gl_Color;"      "\n"  \
  "}"                               "\n"

/* Rect shader openGL 3.0. */
#define SHADER_RECT_VERT_DATA_130                                   \
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
  "#version 130"                    "\n"  \
  "in vec4 f_color;"                "\n"  \
  "out vec4 frag_color;"            "\n"  \
  "void main() {"                   "\n"  \
  "  frag_color = f_color;"         "\n"  \
  "}"                               "\n"

/* Rect shader openGL 3.3. */
#define SHADER_RECT_VERT_DATA_330                                   \
  "#version 330 core"                                         "\n"  \
  "uniform mat4 projection;"                                  "\n"  \
  "layout (location = 0) in vec2 vertex;"                     "\n"  \
  "layout (location = 1) in vec4 color;"                      "\n"  \
  "out vec4 f_color;"                                         "\n"  \
  "void main() {"                                             "\n"  \
  "  f_color = color;"                                        "\n"  \
  "  gl_Position = (projection * vec4(vertex, 0.0f, 1.0f));"  "\n"  \
  "}"                                                         "\n"
#define SHADER_RECT_FRAG_DATA_330  \
  "#version 330 core"        "\n"  \
  "in vec4 f_color;"         "\n"  \
  "out vec4 frag_color;"     "\n"  \
  "void main() {"            "\n"  \
  "  frag_color = f_color;"  "\n"  \
  "}"                        "\n"

#define HACK_FONT_PATH     "/usr/share/fonts/TTF/Hack-Regular.ttf"
#define DEFAULT_FONT_PATH  HACK_FONT_PATH


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static int openGL_major = -1;
static int openGL_minor = -1;

static int shader_location_font_tex        = -1;
static int shader_location_font_projection = -1;

static int shader_location_rect_projection = -1;

static mat4x4 projection;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* GLSL openGL table:
 *   `GLSL/openGL` --- `Release Year`.
 *   `110 --- 2.0` --- `2004`.
 *   `120 --- 2.1` --- `2006`.
 *   `130 --- 3.0` --- `2008`.
 *   `140 --- 3.1` --- `2009`.
 *   `150 --- 3.2` --- `2009`.
 *   `330 --- 3.3` --- `2010`.
 *   `400 --- 4.0` --- `2010`.
 *   `410 --- 4.1` --- `2010`.
 *   `420 --- 4.2` --- `2011`.
 *   `430 --- 4.3` --- `2012`.
 *   `440 --- 4.4` --- `2013`.
 *   `450 --- 4.5` --- `2014`.
 *   `460 --- 4.6` --- `2017`. */
static bool glsl_ver(int major, int minor) {
  static int glsl_major = -1;
  static int glsl_minor = -1;
  const Uchar *glsl_ver;
  if (glsl_major == -1 || glsl_minor == -1) {
    glsl_ver = glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (!glsl_ver || sscanf((const char *)glsl_ver, "%d.%d", &glsl_major, &glsl_minor) != 2) {
      die("Could not get the latest supported glsl version.\n");
    }
  }
  return ((glsl_major > (major)) || (glsl_major == (major) && glsl_minor >= (minor)));
}

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
static inline void shader_set_openGL_version(int glsl_major, int glsl_minor, bool core) {
  switch (glsl_major) {
    case 1: {
      switch (glsl_minor) {
        case 10: {
          openGL_major = 2;
          openGL_minor = 0;
          break;
        }
        case 20: {
          openGL_major = 2;
          openGL_minor = 1;
          break;
        }
        case 30: {
          openGL_major = 3;
          openGL_minor = 0;
          break;
        }
        case 40: {
          openGL_major = 3;
          openGL_minor = 1;
          break;
        }
        case 50: {
          openGL_major = 3;
          openGL_minor = 2;
          break;
        }
        default: {
          die("\nGLSL: Major version: `%d` does not have minor version: '%d'.\n", glsl_major, glsl_minor);
        }
      }
      break;
    }
    case 3: {
      switch (glsl_minor) {
        case 30: {
          openGL_major = 3;
          openGL_minor = 3;
          break;
        }
        default: {
          die("\nGLSL: Major version: `%d` does not have minor version: '%d'.\n", glsl_major, glsl_minor);
        }
      }
      break;
    }
    case 4: {
      if (!((glsl_minor % 10) == 0 && glsl_minor >= 0 && glsl_minor <= 60)) {
        die("\nGLSL: Major version: `%d` does not have minor version: '%d'.\n", glsl_major, glsl_minor);
      }
      openGL_major = 4;
      openGL_minor = (glsl_minor / 10);
      break;
    }
    default: {
      die("\nGLSL: Major version: '%d' does not exist.\n", glsl_major);
    }
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, openGL_major);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, openGL_minor);
  if (core) {
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  }
}

/* ----------------------------- Mat4x4 orthographic ----------------------------- */

/* Set the projection matrix `m` to the correct orthographic layout based on the rect of the 2d plane. */
static inline void mat4x4_orthographic(mat4x4 m, float left, float right, float top, float bot, float znear, float zfar) {
  if (left == right) {
    log_ERR_FA("'left' and `right` cannot be the same value");
  }
  else if (top == bot) {
    log_ERR_FA("'top' and `bot` cannot be the same value");
  }
  else if (znear == zfar) {
    log_ERR_FA("'znear' and `zfar` cannot be the same value");
  }
  else {
    /* Clear the matrix. */
    memset(m, 0, sizeof(mat4x4));
    m[0][0] = (+2.F / (right - left));
    m[3][0] = (-(right + left) / (right - left));
    m[1][1] = (+2.F / (top - bot));
    m[3][1] = (-(top + bot) / (top - bot));
    m[2][2] = (-2.F / (zfar - znear));
    m[3][2] = (-(zfar + znear) / (zfar - znear));
    m[3][3] = 1.F;
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Shader compile ----------------------------- */

/* Compile the `font-shader` and the `rect-shader` and then load load the fonts. */
void shader_compile(void) {
  const char *font_vert;
  const char *font_frag;
  const char *rect_vert;
  const char *rect_frag;
  if (glsl_ver(3, 30)) {
    font_vert = SHADER_FONT_VERT_DATA_330;
    font_frag = SHADER_FONT_FRAG_DATA_330;
    rect_vert = SHADER_RECT_VERT_DATA_330;
    rect_frag = SHADER_RECT_FRAG_DATA_330;
    shader_set_openGL_version(3, 30, TRUE);
  }
  else if (glsl_ver(1, 30)) {
    font_vert = SHADER_FONT_VERT_DATA_130;
    font_frag = SHADER_FONT_FRAG_DATA_130;
    rect_vert = SHADER_RECT_VERT_DATA_130;
    rect_frag = SHADER_RECT_FRAG_DATA_130;
    shader_set_openGL_version(1, 30, FALSE);
  }
  else if (glsl_ver(1, 2)) {
    font_vert = SHADER_FONT_VERT_DATA_120;
    font_frag = SHADER_FONT_FRAG_DATA_120;
    rect_vert = SHADER_RECT_VERT_DATA_120;
    rect_frag = SHADER_RECT_FRAG_DATA_120;
    shader_set_openGL_version(1, 20, FALSE);
  }
  else {
    die(
      "Could not verify support for even the oldest version of GLSL/OPENGL (120/2.0)\n\
      made in 2004, maybe just burn this hardware and get something post 2004.\n"
    );
  }
  /* Create the font shader. */
  font_shader = shader_create(2, (Uint[]) {
    shader_load(GL_VERTEX_SHADER,   font_vert),
    shader_load(GL_FRAGMENT_SHADER, font_frag)
  });
  /* If we fail to compile the font-shader, we must terminate. */
  if (!font_shader) {
    gl_window_free();
    glfwTerminate();
    die("Failed to compile the font shader, this is fatal.\n");
  }
  /* Create both fonts. */
  textfont = font_create();
  uifont   = font_create();
  /* Then load the default font. */
  font_load(textfont, DEFAULT_FONT_PATH, 17, 4096);
  font_load(uifont,   DEFAULT_FONT_PATH, 15, 2048);
  glUseProgram(font_shader); {
    shader_location_font_tex        = glGetUniformLocation(font_shader, "tex");
    shader_location_font_projection = glGetUniformLocation(font_shader, "projection");
  }
  /* Create the rect-shader, here we can run with a failed compilation. */
  rect_shader = shader_create(2, (Uint[]) {
    shader_load(GL_VERTEX_SHADER,   rect_vert),
    shader_load(GL_FRAGMENT_SHADER, rect_frag)
  });
  if (!rect_shader) {
    writeferr("Failed to comile the rect shader.  We can continue without this, but we only have text.\n");
  }
  else {
    glUseProgram(rect_shader); {
      shader_location_rect_projection = glGetUniformLocation(rect_shader, "projection");
    }
  }
}

/* ----------------------------- Shader free ----------------------------- */

/* Free the font shader and rect shader, as well as both fonts. */
void shader_free(void) {
  /* Free the font shader. */
  glDeleteProgram(font_shader);
  /* Also free the rect shader, if it was made. */
  if (rect_shader) {
    glDeleteProgram(rect_shader);
  }
  font_free(textfont);
  font_free(uifont);
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

/* ----------------------------- Shader set projection ----------------------------- */

void shader_set_projection(float left, float right, float top, float bot, float znear, float zfar) {
  mat4x4_orthographic(projection, left, right, top, bot, znear, zfar);
}

/* ----------------------------- Shader upload projection ----------------------------- */

void shader_upload_projection(void) {
  glUseProgram(font_shader); {
    glUniformMatrix4fv(shader_location_font_projection, 1, FALSE, &projection[0][0]);
  }
  glUseProgram(rect_shader); {
    glUniformMatrix4fv(shader_location_rect_projection, 1, FALSE, &projection[0][0]);
  }
}

/* ----------------------------- Shader get location font tex ----------------------------- */

int shader_get_location_font_tex(void) {
  return shader_location_font_tex;
}

/* ----------------------------- Shader get location font projection ----------------------------- */

int shader_get_location_font_projection(void) {
  return shader_location_font_projection;
}

/* ----------------------------- Shader get location rect projection ----------------------------- */

int shader_get_location_rect_projection(void) {
  return shader_location_rect_projection;
}
