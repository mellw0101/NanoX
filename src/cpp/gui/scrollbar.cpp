/** @file gui/scrollbar.cpp

  @author  Melwin Svensson.
  @date    28-3-2025.

 */
#include "../../include/prototypes.h"


/* Calculate the height and the top y position relative to `total_pixel_length` of a scrollbar. */
void calculate_scrollbar(float total_pixel_length, Uint startidx, Uint endidx, Uint visable_idxno, Uint currentidx, float *height, float *ypos) {
  ASSERT(height);
  ASSERT(ypos);
  float ratio = fclamp(((float)(currentidx - startidx) / (endidx - startidx)), 0, 1);
  *height = fclamp((((float)visable_idxno / ((endidx - startidx) + visable_idxno - 1)) * total_pixel_length), 0, total_pixel_length);
  *ypos = fclamp((ratio * (total_pixel_length - (*height))), 0, (total_pixel_length - (*height)));
}

/* Return's the index based on a scrollbar's top `ypos` relative to `total_pixel_length`. */
long index_from_scrollbar_pos(float total_pixel_length, Uint startidx, Uint endidx, Uint visable_idxno, float ypos) {
  float height, max_ypos, ratio;
  /* Calculate the height that the scrollbar should be and the maximum y positon possible. */
  height   = (((float)visable_idxno / ((endidx - startidx) + visable_idxno - 1)) * total_pixel_length);
  max_ypos = (total_pixel_length - height);
  /* Clamp the y position to within the valid range (0 - (editor->text->size.h - height)). */
  ypos = fclamp(ypos, 0, max_ypos);
  /* Calculate the ratio of the max y position that should be used to calculate
   * the line number.  We also clamp the line number to ensure correctness. */
  ratio = fclamp((ypos / max_ypos), 0, 1);
  /* Ensure the returned line is valid in the context of the openfile. */
  return (lclamp((long)(ratio * (endidx - startidx)), 0, (endidx - startidx)) + startidx);
}
