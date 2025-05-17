/** @file color.c

  @author  Melwin Svensson.
  @date    15-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define STATIC_COLOR(name, r, g, b, a) \
  static const Color name = {(r), (g), (b), (a)}

#define STATIC_COLOR_SET_DECL(name, r, g, b, a)  \
  void color_set_##name(Color *const color) {    \
    ASSERT(color);                               \
    STATIC_COLOR(name, r, g, b, a);              \
    color_copy(color, &name);                    \
  }


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


Color *color_create(float r, float g, float b, float a) {
  Color *color = xmalloc(sizeof(*color));
  color->r = r;
  color->g = g;
  color->b = b;
  color->a = a;
  return color;
}

void color_copy(Color *const dst, const Color *const src) {
  ASSERT(dst);
  ASSERT(src);
  memcpy(dst, src, sizeof(*dst));
}

void color_set_rgba(Color *const color, float r, float g, float b, float a) {
  ASSERT(color);
  color->r = r;
  color->g = g;
  color->b = b;
  color->a = a;
}

STATIC_COLOR_SET_DECL(white, 1, 1, 1, 1)
STATIC_COLOR_SET_DECL(black, 0, 0, 0, 1)
