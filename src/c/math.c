/** @file 

  @author  Melwin Svensson.
  @date    2-1-2025.

*/
#include "math.h"

float fclamp(float x, float min, float max) {
  return ((x > max) ? max : (x < min) ? min : x);
}

long lclamp(long x, long min, long max) {
  return ((x > max) ? max : (x < min) ? min : x);
}
