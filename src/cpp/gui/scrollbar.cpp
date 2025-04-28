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


/* `Opaque` structure that represent's a scrollbar.  This is created as a child of any element. */
typedef struct GuiScrollbar {
  /* The base of the scrollbar, extends the entire length. */
  guielement *base;

  /* Movable part of the scrollbar, this can be grabbed to trigger `moving_routine`. */
  guielement *thumb;

  /* This ptr is passed to both `update_routine` and `moving_routine`. */
  void *data;
  
  /* Callback to fetch the needed data from `data`.  Note that all
   * parameters needs to be assigned a value if it is not passed as null. */
  GuiScrollbarUpdateFunc update_routine;
  
  /* Callback witch will pass the calculated index based on current position as the second parameter. */
  GuiScrollbarMoveFunc moving_routine;

  bool refresh_needed : 1;
} GuiScrollbar;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */

static inline float scrollbar_length(float total_length, Uint startidx, Uint endidx, Uint visible_idxno) {
  return fclamp((((float)visible_idxno / ((endidx - startidx) + visible_idxno)) * total_length), 0, total_length);
}

static inline float scrollbar_step_value_for(Uint idx, float total_length, float length, Uint startidx, Uint endidx, Uint visible_idxno) {
  float ratio = fclamp((float)(idx - startidx) / (endidx - startidx), 0.0f, 1.0f);
  return fclamp((ratio * (total_length - length)), 0, (total_length - length));
} 

static float *scrollbar_step_values(float total_length, Uint startidx, Uint endidx, Uint visible_idxno) {
  float *array;
  float len = ((endidx - startidx) + 1);
  float length = scrollbar_length(total_length, startidx, endidx, visible_idxno);
  __avx avx_startidx((float)startidx);
  __avx avx_endidx((float)endidx);
  __avx avx_total_entries(avx_endidx - avx_startidx);
  __avx avx_length(length);
  __avx avx_visible_idxno((float)visible_idxno);
  __avx avx_total_length(total_length);
  __avx<float> values;
  __avx<float> ratio;
  __avx<float> result;
  Ulong i = 0;
  ALWAYS_ASSERT((array = (float *)aligned_alloc((sizeof(float) * 8), (sizeof(float) * len))));
  for (; (i + 8)<len; i+=8) {
    values = __avx<float>(i, i+1, i+2, i+3, i+4, i+5, i+6, i+7);
    ratio  = (values / avx_total_entries);
    result = ((ratio * (avx_total_length - avx_length)));
    for (Ulong k=0; k<8; ++k) {
      array[i+k] = result[k];
    }
  }
  for (; i<len; ++i) {
    array[i] = scrollbar_step_value_for((i + startidx), total_length, length, startidx, endidx, visible_idxno);
  }
  return array;
}

_UNUSED static Ulong scrollbar_closest_idx(float *const array, Ulong len, float rawpos) {
  ASSERT(array);
  ASSERT(len);
  Ulong index=0;
  float closest = absf(array[0] - rawpos);
  float value;
  for (Ulong i=1; i<len; ++i) {
    if ((value = absf(array[i] - rawpos)) < closest) {
      index = i;
      closest = value;
    }
  }
  return index;
}

/* Calculate the height and the top y position relative to `total_pixel_length` of a scrollbar. */
void calculate_scrollbar(float total_pixel_length, Uint startidx, Uint endidx, Uint visable_idxno, Uint currentidx, float *length, float *relative_pos) {
  ASSERT(length);
  ASSERT(relative_pos);
  *length       = scrollbar_length(total_pixel_length, startidx, endidx, visable_idxno);
  *relative_pos = scrollbar_step_value_for(currentidx, total_pixel_length, *length, startidx, endidx, visable_idxno);
}

/* Return's the index based on a scrollbar's top `ypos` relative to `total_pixel_length`. */
long index_from_scrollbar_pos(float total_pixel_length, Uint startidx, Uint endidx, Uint visable_idxno, float ypos) __THROW {
  float height, max_ypos, *array;
  Ulong index;
  /* Calculate the height that the scrollbar should be and the maximum y positon possible. */
  height   = (((float)visable_idxno / ((endidx - startidx) + visable_idxno /* - 1 */)) * total_pixel_length);
  max_ypos = (total_pixel_length - height);
  /* Clamp the y position to within the valid range (0 - (editor->text->size.h - height)). */
  ypos = fclamp(ypos, 0, max_ypos);
  /* Calculate the ratio of the max y position that should be used to calculate
  * the line number.  We also clamp the line number to ensure correctness. */
  array = scrollbar_step_values(total_pixel_length, startidx, endidx, visable_idxno);
  index = scrollbar_closest_idx(array, (endidx - startidx + 1), ypos);
  free(array);
  return (index + startidx);
}

/* Return's the index based on a scrollbar's top `ypos` relative to `total_pixel_length`. */
// long index_from_scrollbar_pos(float total_pixel_length, Uint startidx, Uint endidx, Uint visable_idxno, float ypos) __THROW {
//   float height, max_ypos, ratio;
//   /* Calculate the height that the scrollbar should be and the maximum y positon possible. */
//   height   = (((float)visable_idxno / ((endidx - startidx) + visable_idxno /* - 1 */)) * total_pixel_length);
//   max_ypos = (total_pixel_length - height);
//   /* Clamp the y position to within the valid range (0 - (editor->text->size.h - height)). */
//   ypos = fclamp(ypos, 0, max_ypos);
//   /* Calculate the ratio of the max y position that should be used to calculate
//   * the line number.  We also clamp the line number to ensure correctness. */
//   ratio = fclamp((ypos / max_ypos), 0, 1);
//   /* Ensure the returned line is valid in the context of the openfile. */
//   return (lclamp((long)(ratio * (endidx - startidx)), 0, (endidx - startidx)) + startidx);
// }

/* ----------------------------- GuiScrollbar ----------------------------- */

/* At the surface this seams to be much better to use then then directly handleing the logic in inside the main function,
 * mainly because we would not need any checking of the element as only this element would have this callback.  But be that
 * as it may i find the structure of the whole becomes mush worse to manage and to implement complext things that relay on
 * much more context that would be mush easier to design as a whole not as scattered callbacks.  At least for now. */
_UNUSED static void guiscrollbar_callback(guielement *e, guielement_callback_type type) {
  ASSERT(e);
  switch (type) {
    case GUIELEMENT_ENTER_CALLBACK: {
      if (!gui->clicked) {
        e->color = GUISB_ACTIVE_THUMB_COLOR;
      }
      break;
    }
    case GUIELEMENT_LEAVE_CALLBACK: {
      if (!gui->clicked) {
        e->color = GUISB_THUMB_COLOR;
      }
      break;
    }
    case GUIELEMENT_LEFT_MOUSE_UNCLICK: {
      if (e != guielement_from_mousepos()) {
        e->color = GUISB_THUMB_COLOR;
      }
      e->data.sb->refresh_needed = TRUE;
      break;
    }
    default: {
      break;
    }
  }
}

/* Create a scrollbar that will be on the left side of `parent` and use `userdata` as the parameter when running all callbacks. */
GuiScrollbar *guiscrollbar_create(guielement *const parent, void *const userdata, GuiScrollbarUpdateFunc update_routine, GuiScrollbarMoveFunc moving_routine) __THROW {
  ASSERT(parent);
  GuiScrollbar *sb = (__TYPE(sb))xmalloc(sizeof(*sb));
  sb->data  = userdata;
  sb->base  = guielement_create(parent);
  sb->thumb = guielement_create(sb->base);
  guielement_set_sb_data(sb->base, sb);
  guielement_set_sb_data(sb->thumb, sb);
  guielement_set_flag_recurse(sb->base, TRUE, GUIELEMENT_ABOVE);
  guielement_set_flag_recurse(sb->base, TRUE, GUIELEMENT_RELATIVE_Y_POS);
  sb->base->flag.set<GUIELEMENT_REVERSE_RELATIVE_X_POS>();
  sb->thumb->flag.set<GUIELEMENT_RELATIVE_X_POS>();
  guielement_move_resize(sb->base, 10, 10);
  guielement_move_resize(sb->thumb, 10, 10);
  sb->base->relative_pos.x = sb->base->size.w;
  sb->base->color          = GUISB_BASE_COLOR;
  sb->thumb->color         = GUISB_THUMB_COLOR;
  sb->update_routine       = update_routine;
  sb->moving_routine       = moving_routine;
  sb->refresh_needed       = TRUE;
  return sb;
}

/* `Internal`  This function calculates the scrollbar thumbs position and length using the values from the callback's. */
static void guiscrollbar_calculate(GuiScrollbar *const sb) {
  ASSERT_SB;
  float total_length, length, ypos, offset;
  Uint start, total, visible, current;
  /* Use the update routine to get all needed data. */
  sb->update_routine(sb->data, &total_length, &start, &total, &visible, &current, &offset);
  /* Calculate the size and position of the thumb. */
  calculate_scrollbar(total_length, start, total, visible, current, &length, &ypos);
  /* If the height of the thumb is the entire length of the base or longer, then just hide the entire scrollbar. */
  if (length >= total_length) {
    guielement_set_flag_recurse(sb->base, TRUE, GUIELEMENT_HIDDEN);
  }
  else {
    sb->base->relative_pos.y = offset;
    sb->base->relative_pos.x = (sb->base->size.w + offset);
    guielement_set_flag_recurse(sb->base, FALSE, GUIELEMENT_HIDDEN);
    guielement_resize(sb->thumb, vec2(sb->thumb->size.w, length));
    sb->thumb->relative_pos.y = ypos;
    /* Set the base of the scrollbar to the total movable length of the thumb. */
    guielement_resize(sb->base, vec2(sb->base->size.w, total_length));
    /* Move the thumb again so it's above the base. */
    guielement_move(sb->thumb, sb->thumb->pos);
  }
}

/* Move the thumb part of `sb` by `change`.  Note that this is fully clamped and cannot exceed the base element.  Note that this is the only way `moving_routine` gets called. */
void guiscrollbar_move(GuiScrollbar *const sb, float change) {
  ASSERT_SB;
  float total_length, offset;
  Uint start, total, visible;
  /* Use the update routine to get all needed data. */
  sb->update_routine(sb->data, &total_length, &start, &total, &visible, NULL, &offset);
  /* Move the element by the given change, this is clamped to never go outside the constraint's of the base element. */
  guielement_move_y_clamp(
    sb->thumb,
    (sb->thumb->pos.y + change),
    (sb->base->pos.y),
    (sb->base->pos.y + sb->base->size.h - sb->thumb->size.h)
  );
  /* Run the moving routine using the index the current position represents. */
  sb->moving_routine(sb->data, index_from_scrollbar_pos(total_length, start, total, visible, (sb->thumb->pos.y - sb->base->pos.y)));
}

/* Draw the scrollbar as well as updating it if `sb->refresh_needed` has been set. */
void guiscrollbar_draw(GuiScrollbar *const sb) {
  ASSERT_SB;
  if (sb->refresh_needed) {
    guiscrollbar_calculate(sb);
    sb->refresh_needed = FALSE;
    if (sb->base->flag.is_set<GUIELEMENT_HIDDEN>()) {
      return;
    }
  }
  guielement_draw(sb->base);
  guielement_draw(sb->thumb);
}

/* Tell the scrollbar that it needs to recalculate before the next draw.  The reason this needs to be a flag and not directly
 * recalculating is so that no matter how meny times or from how meny seperate function's this gets called it only recalculates once. */
void guiscrollbar_refresh_needed(GuiScrollbar *const sb) __THROW {
  ASSERT_SB;
  sb->refresh_needed = TRUE;
}

/* Return's `TRUE` when `e` is the `base` element of `sb`. */
bool guiscrollbar_element_is_base(GuiScrollbar *const sb, guielement *const e) __THROW {
  ASSERT_SB;
  ASSERT(e);
  return (e == sb->base);
}

/* Return's `TRUE` when `e` is the `thumb` element of `sb`. */
bool guiscrollbar_element_is_thumb(GuiScrollbar *const sb, guielement *const e) __THROW {
  ASSERT_SB;
  ASSERT(e);
  return (e == sb->thumb);
}

/* Return's the current width of the scrollbar.  TODO: Add the ability to influens the width later. */
float guiscrollbar_width(GuiScrollbar *const sb) {
  ASSERT_SB;
  return sb->base->size.w;
}
