/** @file word.c

  @author  Melwin Svensson.
  @date    13-2-2025.

 */
#include "../include/c_proto.h"

#include "../include/c/wchars.h"

/* Return's the start index of the word at pos, if any Otherwise return's pos. */
Ulong wordstartindex(const char *const __restrict string, Ulong pos, bool allowunderscore) {
  ASSERT(string);
  Ulong idx=pos, oneleft;
  while (idx > 0) {
    oneleft = step_left(string, idx);
    if (!iswordc((string + oneleft), FALSE, (allowunderscore ? "_" : NULL))) {
      break;
    }
    idx = oneleft;
  }
  return idx;
}

/* Return the end index of the word at `pos`, if any.  Otherwise, return `pos` if already at end index or if at whitespace. */
Ulong wordendindex(const char *const __restrict string, Ulong pos, bool allowunderscore) {
  ASSERT(string);
  Ulong idx = pos;
  while (*(string + idx)) {
    if (!iswordc((string + idx), FALSE, (allowunderscore ? "_" : NULL))) {
      break;
    }
    idx = step_right(string, idx);
  }
  return idx;
}
