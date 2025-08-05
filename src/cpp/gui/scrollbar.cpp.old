/** @file gui/scrollbar.cpp

  @author  Melwin Svensson.
  @date    28-3-2025.

 */
#include "../../include/prototypes.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_SB              \
  ASSERT(sb);                  \
  ASSERT(sb->base);            \
  ASSERT(sb->thumb);           \
  ASSERT(sb->update_routine);  \
  ASSERT(sb->moving_routine)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


// /* `Opaque` structure that represent's a scrollbar.  This is created as a child of any element. */
// typedef struct GuiScrollbar {
//   /* The base of the scrollbar, extends the entire length. */
//   guielement *base;

//   /* Movable part of the scrollbar, this can be grabbed to trigger `moving_routine`. */
//   guielement *thumb;

//   /* This ptr is passed to both `update_routine` and `moving_routine`. */
//   void *data;
  
//   /* Callback to fetch the needed data from `data`.  Note that all
//    * parameters needs to be assigned a value if it is not passed as null. */
//   GuiScrollbarUpdateFunc update_routine;
  
//   /* Callback witch will pass the calculated index based on current position as the second parameter. */
//   GuiScrollbarMoveFunc moving_routine;

//   bool refresh_needed : 1;
// } GuiScrollbar;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* ----------------------------- Static ----------------------------- */

// static inline float scrollbar_length(float total_length, Uint startidx, Uint endidx, Uint visible_idxno) {
//   return fclamp((((float)visible_idxno / ((endidx - startidx) + visible_idxno)) * total_length), 0, total_length);
// }

// static inline float scrollbar_step_value_for(Uint idx, float total_length, float length, Uint startidx, Uint endidx, Uint visible_idxno) {
//   float ratio = fclamp((float)(idx - startidx) / (endidx - startidx), 0.0f, 1.0f);
//   return fclamp((ratio * (total_length - length)), 0, (total_length - length));
// }

// /* Return's a allocated `float *` containing the preceding index position, the current index position and
//  * the following index position, and when either at the first or last index just return 2 positions. */
// static float *scrollbar_closest_step_values(Uint idx, float total_length, Uint startidx, Uint endidx, Uint visible_idxno, Ulong *const arraylen) {
//   ASSERT(arraylen);
//   float *array = (float *)xmalloc(sizeof(float) * 3);
//   float length = scrollbar_length(total_length, startidx, endidx, visible_idxno);
//   Ulong i = 0;
//   /* Only add the preceding index position if the current index is not zero. */
//   if (idx != 0) {
//     array[i++] = scrollbar_step_value_for((idx - 1 + startidx), total_length, length, startidx, endidx, visible_idxno);
//   }
//   /* Always add the current index position. */
//   array[i++] = scrollbar_step_value_for((idx + startidx), total_length, length, startidx, endidx, visible_idxno);
//   /* Only add the following index position if the current index is not the last. */
//   if (idx != (endidx - startidx)) {
//     array[i++] = scrollbar_step_value_for((idx + 1 + startidx), total_length, length, startidx, endidx, visible_idxno);
//   }
//   *arraylen = i;
//   return array;
// }

// /* Return's the true index based on the inaccuret index and current raw position of the scrollbar. */
// static Ulong scrollbar_closest_step_index(Uint idx, float rawpos, float *const array, Ulong arraylen) {
//   ASSERT(array);
//   ASSERT(arraylen);
//   /* Start of with the first index as the closest, as that would happen no matter what. */
//   Ulong index=0;
//   float closest = absf(array[0] - rawpos);
//   float value;
//   /* Then check the remaining indexes. */
//   for (Ulong i=1; i<arraylen; ++i) {
//     /* Set the closest entry as index. */
//     if ((value = absf(array[i] - rawpos)) < closest) {
//       index = i;
//       closest = value;
//     }
//   }
//   /* If the array only has a size of 2 that means we are either at the top or the bottom.  And we only need to change
//    * the return index if we are at the top, because even if we are at the bottom there are only 2 entries in the array. */
//   if (arraylen == 2 && idx == 0) {
//     return (idx + index);
//   }
//   return (idx - 1 + index);
// }

// /* ----------------------------- Global ----------------------------- */

// /* Calculate the height and the top y position relative to `total_pixel_length` of a scrollbar. */
// void calculate_scrollbar(float total_pixel_length, Uint startidx, Uint endidx, Uint visable_idxno, Uint currentidx, float *length, float *relative_pos) {
//   ASSERT(length);
//   ASSERT(relative_pos);
//   *length       = scrollbar_length(total_pixel_length, startidx, endidx, visable_idxno);
//   *relative_pos = scrollbar_step_value_for(currentidx, total_pixel_length, *length, startidx, endidx, visable_idxno);
// }

// /* Return's the index based on a scrollbar's top `ypos` relative to `total_pixel_length`. */
// long index_from_scrollbar_pos(float total_pixel_length, Uint startidx, Uint endidx, Uint visable_idxno, float ypos) __THROW {
//   float height, max_ypos, *array, ratio;
//   Ulong index, arraylen;
//   /* Calculate the height that the scrollbar should be and the maximum y positon possible. */
//   height   = (((float)visable_idxno / ((endidx - startidx) + visable_idxno /* - 1 */)) * total_pixel_length);
//   max_ypos = (total_pixel_length - height);
//   /* Clamp the y position to within the valid range (0 - (editor->text->size.h - height)). */
//   ypos = fclamp(ypos, 0.0f, max_ypos);
//   /* Calculate the ratio of the max y position that should be used to calculate
//    * the line number.  We also clamp the line number to ensure correctness. */
//   ratio = fclamp((ypos / max_ypos), 0.0f, 1.0f);
//   /* First we get the most likely index.  Note that this will sometimes be inacurret. */
//   index = lclamp((long)(ratio * (endidx - startidx)), 0, (endidx - startidx));
//   /* Then based on that index get the 3 closest positons to it, or just 2 when at the top(0) or bottom(endidx - startidx). */
//   array = scrollbar_closest_step_values(index, total_pixel_length, startidx, endidx, visable_idxno, &arraylen);
//   /* Now get the true index based on the exact pixel position. */
//   index = scrollbar_closest_step_index(index, ypos, array, arraylen);
//   free(array);
//   /* Now return the appropriet index based on the start index. */
//   return (index + startidx);
// }

// /* ----------------------------- GuiScrollbar ----------------------------- */


// /* Create a scrollbar that will be on the left side of `parent` and use `data` as the parameter when running all callbacks. */
// GuiScrollbar *gui_scrollbar_create(guielement *const parent, void *const data, GuiScrollbarUpdateFunc update_routine, GuiScrollbarMoveFunc moving_routine) __THROW {
//   ASSERT(parent);
//   ASSERT(data);
//   ASSERT(update_routine);
//   ASSERT(moving_routine);
//   float top_offset, right_offset;
//   GuiScrollbar *sb;
//   MALLOC_STRUCT(sb);
//   sb->data  = data;
//   sb->base  = gui_element_create(parent);
//   sb->thumb = gui_element_create(sb->base);
//   gui_element_set_sb_data(sb->base, sb);
//   gui_element_set_sb_data(sb->thumb, sb);
//   gui_element_set_flag_recurse(sb->base, TRUE, GUIELEMENT_ABOVE);
//   gui_element_set_flag_recurse(sb->base, TRUE, GUIELEMENT_RELATIVE_Y_POS);
//   sb->base->flag.set<GUIELEMENT_REVERSE_RELATIVE_X_POS>();
//   sb->thumb->flag.set<GUIELEMENT_RELATIVE_X_POS>();
//   gui_element_move_resize(sb->base, 10, 10);
//   gui_element_move_resize(sb->thumb, 10, 10);
//   sb->base->relative_pos.x = sb->base->size.w;
//   sb->base->color          = GUISB_BASE_COLOR;
//   sb->thumb->color         = GUISB_THUMB_COLOR;
//   sb->update_routine       = update_routine;
//   sb->moving_routine       = moving_routine;
//   sb->refresh_needed       = TRUE;
//   /* Ensure correct placement directly. */
//   sb->update_routine(sb->data, NULL, NULL, NULL, NULL, NULL, &top_offset, &right_offset);
//   sb->base->relative_pos.x = (sb->base->size.w + right_offset);
//   sb->base->relative_pos.y = top_offset;
//   return sb;
// }

// /* `Internal`  This function calculates the scrollbar thumbs position and length using the values from the callback's. */
// static void gui_scrollbar_calculate(GuiScrollbar *const sb) {
//   ASSERT_SB;
//   float total_length, length, ypos, top_offset, right_offset;
//   Uint start, total, visible, current;
//   /* Use the update routine to get all needed data. */
//   sb->update_routine(sb->data, &total_length, &start, &total, &visible, &current, &top_offset, &right_offset);
//   /* Calculate the size and position of the thumb. */
//   calculate_scrollbar(total_length, start, total, visible, current, &length, &ypos);
//   /* If the height of the thumb is the entire length of the base or longer, then just hide the entire scrollbar. */
//   if (!total_length || length >= total_length) {
//     gui_element_set_flag_recurse(sb->base, TRUE, GUIELEMENT_HIDDEN);
//   }
//   else {
//     sb->base->relative_pos.y = top_offset;
//     sb->base->relative_pos.x = (sb->base->size.w + right_offset);
//     gui_element_set_flag_recurse(sb->base, FALSE, GUIELEMENT_HIDDEN);
//     gui_element_resize(sb->thumb, vec2(sb->thumb->size.w, length));
//     sb->thumb->relative_pos.y = ypos;
//     /* Set the base of the scrollbar to the total movable length of the thumb. */
//     gui_element_resize(sb->base, vec2(sb->base->size.w, total_length));
//     /* Move the thumb again so it's above the base. */
//     gui_element_move(sb->thumb, sb->thumb->pos);
//   }
// }

// /* Move the thumb part of `sb` by `change`.  Note that this is fully clamped and cannot exceed the base element.  Note that this is the only way `moving_routine` gets called. */
// void gui_scrollbar_move(GuiScrollbar *const sb, float change) {
//   ASSERT_SB;
//   float total_length;
//   Uint start, total, visible;
//   /* Use the update routine to get all needed data. */
//   sb->update_routine(sb->data, &total_length, &start, &total, &visible, NULL, NULL, NULL);
//   /* Move the element by the given change, this is clamped to never go outside the constraint's of the base element. */
//   gui_element_move_y_clamp(
//     sb->thumb,
//     (sb->thumb->pos.y + change),
//     (sb->base->pos.y),
//     (sb->base->pos.y + sb->base->size.h - sb->thumb->size.h)
//   );
//   /* Run the moving routine using the index the current position represents. */
//   sb->moving_routine(sb->data, index_from_scrollbar_pos(total_length, start, total, visible, (sb->thumb->pos.y - sb->base->pos.y)));
// }

// /* Draw the scrollbar as well as updating it if `sb->refresh_needed` has been set. */
// void gui_scrollbar_draw(GuiScrollbar *const sb) {
//   ASSERT_SB;
//   if (sb->refresh_needed) {
//     gui_scrollbar_calculate(sb);
//     sb->refresh_needed = FALSE;
//     if (sb->base->flag.is_set<GUIELEMENT_HIDDEN>()) {
//       return;
//     }
//   }
//   gui_element_draw(sb->base);
//   gui_element_draw(sb->thumb);
// }

// /* Tell the scrollbar that it needs to recalculate before the next draw.  The reason this needs to be a flag and not directly
//  * recalculating is so that no matter how meny times or from how meny seperate function's this gets called it only recalculates once. */
// void gui_scrollbar_refresh_needed(GuiScrollbar *const sb) __THROW {
//   ASSERT_SB;
//   sb->refresh_needed = TRUE;
// }

// /* Return's `TRUE` when `e` is the `base` element of `sb`. */
// bool gui_scrollbar_element_is_base(GuiScrollbar *const sb, guielement *const e) __THROW {
//   ASSERT_SB;
//   ASSERT(e);
//   return (e == sb->base);
// }

// /* Return's `TRUE` when `e` is the `thumb` element of `sb`. */
// bool gui_scrollbar_element_is_thumb(GuiScrollbar *const sb, guielement *const e) __THROW {
//   ASSERT_SB;
//   ASSERT(e);
//   return (e == sb->thumb);
// }

// /* Return's the current width of the scrollbar.  TODO: Add the ability to influens the width later. */
// float gui_scrollbar_width(GuiScrollbar *const sb) {
//   ASSERT_SB;
//   return sb->base->size.w;
// }

// void gui_scrollbar_show(GuiScrollbar *const sb, bool show) {
//   ASSERT_SB;
//   gui_element_set_flag_recurse(sb->base, !show, GUIELEMENT_HIDDEN);
// }
