/** @file color.c

  @author  Melwin Svensson.
  @date    15-5-2025.

 */
#include "../../include/c_proto.h"


Color *color_create(float r, float g, float b, float a) {
  Color *color = xmalloc(sizeof(*color));
  color->r = r;
  color->g = g;
  color->b = b;
  color->a = a;
  return color;
}
