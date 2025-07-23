/** @file word.c

  @author  Melwin Svensson.
  @date    13-2-2025.

 */
#include "../include/c_proto.h"

/* Return's the start index of the word at pos, if any Otherwise return's pos. */
Ulong wordstartindex(const char *const restrict string, Ulong pos, bool allow_underscore) {
  ASSERT(string);
  Ulong start_index = pos;
  Ulong oneleft;
  while (start_index > 0) {
    oneleft = step_left(string, start_index);
    if (!is_word_char((string + oneleft), FALSE) && (!allow_underscore || string[oneleft] != '_')) {
      break;
    }
    start_index = oneleft;
  }
  return start_index;
}

/* Return the end index of the word at `pos`, if any.  Otherwise, return `pos` if already at end index or if at whitespace. */
Ulong wordendindex(const char *const restrict string, Ulong pos, bool allow_underscore) {
  ASSERT(string);
  Ulong index = pos;
  while (string[index]) {
    if (!is_word_char(&string[index], FALSE) && (!allow_underscore || string[index] != '_')) {
      break;
    }
    index = step_right(string, index);
  }
  return index;
}

/* Assigns the number of white char`s to the prev/next word to 'nchars'.  Return`s 'true' when word is more then 2 white`s away. */
bool more_than_a_blank_away(const char *const restrict string, Ulong index, bool forward, Ulong *const restrict nsteps) {
  ASSERT(string);
  ASSERT(nsteps);
  /* The current index we are checking. */
  Ulong i = index;
  /* The number of blank chars we have found. */
  Ulong chars = 0;
  /* Backwards. */
  if (!forward) {
    i = step_left(string, i);
    while (is_blank_char(string + i)) {
      ++chars;
      if (!i) {
        break;
      }
      i = step_left(string, i);
    }
  }
  /* Forwards. */
  else {
    /* Iterate as long as the char at `string + i` is a blank one. */
    for (; is_blank_char(string + i); i += char_length(string), ++chars);
  }
  /* If there was more then one blank char in a row, set `*nsteps` to `chars`, and return `TRUE`. */
  if (chars > 1) {
    *nsteps = chars;
    return TRUE;
  }
  /* Otherwise, set `*nsteps` to zero, and return `FALSE`. */
  else {
    *nsteps = 0;
    return FALSE;
  }
}

/* ----------------------------- Prev word get ----------------------------- */

char *prev_word_get(const char *const restrict data, Ulong index, Ulong *const outlen) {
  Ulong start = wordstartindex(data, index, TRUE);
  if (start != index) {
    ASSIGN_IF_VALID(outlen, (index - start));
    return measured_copy((data + start), (index - start));
  }
  return NULL;
}
