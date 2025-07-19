/** @file gui/utils.c

  @author  Melwin Svensson.
  @date    17-5-2025.

 */
#include "../../include/c_proto.h"


// void draw_rect_rgba(float x, float y, float width, float height, float r, float g, float b, float a) {
//   static int c_loc = 0;
//   static int p_loc = 0;
//   static int s_loc = 0;
//   glUseProgram(element_rect_shader);
//   !c_loc ? (c_loc = glGetUniformLocation(element_rect_shader, "rectcolor")) : ((int)0);
//   !p_loc ? (p_loc = glGetUniformLocation(element_rect_shader, "elempos"))   : ((int)0);
//   !s_loc ? (s_loc = glGetUniformLocation(element_rect_shader, "elemsize"))  : ((int)0);
//   glUniform4f(c_loc, r, g, b, a);
//   glUniform2f(p_loc, x, y);
//   glUniform2f(s_loc, width, height);
//   glDrawArrays(GL_TRIANGLES, 0, 6);
// }

// void draw_rect_rgba(float x, float y, float width, float height, float r, float g, float b, float a) {
//   static int c_loc = 0;
//   static int p_loc = 0;
//   static int s_loc = 0;
//   glUseProgram(element_rect_shader);
//   !c_loc ? (c_loc = glGetUniformLocation(element_rect_shader, "rectcolor")) : ((int)0);
//   !p_loc ? (p_loc = glGetUniformLocation(element_rect_shader, "elempos"))   : ((int)0);
//   !s_loc ? (s_loc = glGetUniformLocation(element_rect_shader, "elemsize"))  : ((int)0);
//   glUniform4f(c_loc, r, g, b, a);
//   glUniform2f(p_loc, x, y);
//   glUniform2f(s_loc, width, height);
//   glDrawArrays(GL_TRIANGLES, 0, 6);
// }

void render_vertbuf(Font *const f, vertex_buffer_t *buf) {
  ASSERT(f);
  ASSERT(buf);
  font_upload_texture_atlas(f);
  glEnable(GL_TEXTURE_2D);
  glUseProgram(font_shader); {
    glUniform1i(glGetUniformLocation(font_shader, "tex"), 0);
    vertex_buffer_render(buf, GL_TRIANGLES);
  }
}

/* Create a buffer using the structure of the font shader. */
vertex_buffer_t *vertbuf_create(void) {
  vertex_buffer_t *buf = vertex_buffer_new(FONT_VERTBUF);
  ALWAYS_ASSERT(buf);
  return buf;
}

// void mat4x4_orthographic(mat4x4 m, float left, float right, float top, float bot, float znear, float zfar) {
//   if (!(left == right || top == bot || znear == zfar)) {
//     /* Clear the matrix. */
//     memset(m, 0, sizeof(mat4x4));
//     m[0][0] = (+2.f / (right - left));
//     m[3][0] = (-(right + left) / (right - left));
//     m[1][1] = (+2.f / (top - bot));
//     m[3][1] = (-(top + bot) / (top - bot));
//     m[2][2] = (-2.f / (zfar - znear));
//     m[3][2] = (-(zfar + znear) / (zfar - znear));
//     m[3][3] = 1.f;
//   }
// }
