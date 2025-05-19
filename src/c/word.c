/** @file word.c

  @author  Melwin Svensson.
  @date    13-2-2025.

 */
#include "../include/c_proto.h"

#include "../include/c/wchars.h"

/* Return's the start index of the word at pos, if any Otherwise return's pos. */
Ulong wordstartindex(const char *const restrict string, Ulong pos, bool allow_underscore) {
  // ASSERT(string);
  // Ulong idx=pos, oneleft;
  // while (idx > 0) {
  //   oneleft = step_left(string, idx);
  //   if (!iswordc((string + oneleft), FALSE, (allowunderscore ? "_" : NULL))) {
  //     break;
  //   }
  //   idx = oneleft;
  // }
  // return idx;
  Ulong start_index = pos;
  while (start_index > 0) {
    Ulong oneleft = step_left(string, start_index);
    if (!is_word_char(&string[oneleft], FALSE) && (!allow_underscore || string[oneleft] != '_')) {
      break;
    }
    start_index = oneleft;
  }
  return start_index;
}

/* Return the end index of the word at `pos`, if any.  Otherwise, return `pos` if already at end index or if at whitespace. */
Ulong wordendindex(const char *const restrict string, Ulong pos, bool allow_underscore) {
  ASSERT(string);
  // Ulong idx = pos;
  // while (*(string + idx)) {
  //   if (!iswordc((string + idx), FALSE, (allowunderscore ? "_" : NULL))) {
  //     break;
  //   }
  //   idx = step_right(string, idx);
  // }
  // return idx;
  Ulong index = pos;
  while (string[index]) {
    if (!is_word_char(&string[index], FALSE) && (!allow_underscore || string[index] != '_')) {
      break;
    }
    index = step_right(string, index);
  }
  return index;
}
