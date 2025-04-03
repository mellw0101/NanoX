/** @file gui/strpx.c

  @author  Melwin Svensson.
  @date    2-4-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* Return's the width in pixels of the codepoint that `current` points to.  And if
 * there was a codepoint before it also add the kerning of the `previous` codepoint. */
float strpx_glyphlen(const char *const restrict current, const char *const restrict previous, texture_font_t *const font) {
  ASSERT(current);
  /* Glyph derived from the `current` codepoint. */
  texture_glyph_t *glyph = texture_font_get_glyph(font, current);
  ALWAYS_ASSERT(glyph);
  /* If there was a codepoint before the `current` one, then also add the kerning of that previous codepoint. */
  return (glyph->advance_x + (previous ? texture_glyph_get_kerning(glyph, previous) : 0));
}

/* Return's an array with the pixel position of where each char in `str` start's.  Note that this also includes where the `NULL-TERMINATOR` starts. */
float *strpx_array(const char *const restrict str, Ulong len, float normx, Ulong *const outlen, texture_font_t *const font) {
  ASSERT(str);
  ASSERT(outlen);
  ASSERT(font);
  /* Array we will fill with the pixel each glyph starts at. */
  float *array = xmalloc((len + 1) * sizeof(float));
  /* Start and end pixel for the current char. */
  float start=normx, end=normx;
  /* Ptr's to the current and previous char. */
  const char *current, *prev=NULL;
  for (Ulong i=0; i<len; ++i, start=end, prev=current) {
    /* If the current codepoint is a tabulator, then calculate the total width based on the set tabsize. */
    if (*(current = &str[i]) == '\t') {
      /* Add the first space of the tabulator with the kerning from the previous char. */
      current = " ";
      end += strpx_glyphlen(current, prev, font);
      /* Then, only if tabsize is greater then one, add the rest of the tabulator after setting the previous char to a space as well. */
      if (tabsize > 1) {
        prev = " ";
        end += (strpx_glyphlen(current, prev, font) * (tabsize - 1));
      }
    }
    /* Otherwise, just use the given value. */
    else {
      end += strpx_glyphlen(current, prev, font);
    }
    /* Add the start of this glyph to the array. */
    array[i] = start;
  }
  /* Add the final place in the array when we point the null terminator. */
  array[len] = end;
  /* Assign the total number of entries to `*outlen`. */
  *outlen = (len + 1);
  return array;
}

/* Return's the index of the point closest to `rawx`. */
Ulong strpx_array_index(float *const array, Ulong len, float rawx) {
  ASSERT(array);
  ASSERT(len);
  /* The index we will return.  Start by setting it to zero as that would happen anyway. */
  Ulong index=0;
  /* Set the closest distance as the first element in the array. */
  float closest = absf(array[0] - rawx);
  float value;
  for (Ulong i=1; i<len; ++i) {
    if ((value = absf(array[i] - rawx)) < closest) {
      index = i;
      closest = value;
    }
  }
  return index;
}

