/** @file gui/scrollbar.c

  @author  Melwin Svensson.
  @date    17-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_SCROLLBAR  \
  ASSERT(sb);             \
  ASSERT(sb->base);       \
  ASSERT(sb->thumb);      \
  ASSERT(sb->data);       \
  ASSERT(sb->update);     \
  ASSERT(sb->moving)

#define SCROLLBAR_LENGTH(total_length, start, end, visible)                                            \
  (fclamp(                                                                                             \
    (((float)(visible) / (((Uint)(end) - (Uint)(start)) + (Uint)(visible))) * (float)(total_length)),  \
    0,                                                                                                 \
    (float)(total_length))                                                                             \
  )

#define SB_WIDTH  (10.F)

#define SB_THUMB_COLOR          PACKED_UINT_FLOAT(1.0f, 1.0f, 1.0f, 0.3f)
#define SB_ACTIVE_THUMB_COLOR   PACKED_UINT_FLOAT(1.0f, 1.0f, 1.0f, 0.7f)


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


typedef enum {
  SCROLLBAR_REFRESH = (1 << 0),
# define SCROLLBAR_REFRESH  SCROLLBAR_REFRESH
} ScrollbarStateType;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct Scrollbar {
  /* The current state of the scrollbar. */
  Uint xflags;

  Element *base;
  Element *thumb;

  void *data;
  ScrollbarUpdateFunc update;
  ScrollbarMovingFunc moving;
};


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Scrollbar length ----------------------------- */

static inline float scrollbar_length(float total_length, Uint start, Uint end, Uint visible) {
  return fclamp((((float)visible / ((end - start) + visible)) * total_length), 0, total_length);
}

/* ----------------------------- Scrollbar step value for ----------------------------- */

static inline float scrollbar_step_value_for(Uint idx, float total_length, float length, Uint startidx, Uint endidx) {
  float ratio = fclamp((float)(idx - startidx) / (endidx - startidx), 0.0f, 1.0f);
  return fclamp((ratio * (total_length - length)), 0, (total_length - length));
}

/* ----------------------------- Scrollbar closest step values ----------------------------- */

/* Return's a allocated `float *` containing the preceding index position, the current index position and
 * the following index position, and when either at the first or last index just return 2 positions. */
static float *scrollbar_closest_step_values(Uint idx, float total_length,
  Uint startidx, Uint endidx, Uint visible_idxno, Ulong *const arraylen)
{
  ASSERT(arraylen);
  float *array = xmalloc(sizeof(float) * 3);
  float length = scrollbar_length(total_length, startidx, endidx, visible_idxno);
  Ulong i = 0;
  /* Only add the preceding index position if the current index is not zero. */
  if (idx != 0) {
    array[i++] = scrollbar_step_value_for((idx - 1 + startidx), total_length, length, startidx, endidx);
  }
  /* Always add the current index position. */
  array[i++] = scrollbar_step_value_for((idx + startidx), total_length, length, startidx, endidx);
  /* Only add the following index position if the current index is not the last. */
  if (idx != (endidx - startidx)) {
    array[i++] = scrollbar_step_value_for((idx + 1 + startidx), total_length, length, startidx, endidx);
  }
  *arraylen = i;
  return array;
}

/* ----------------------------- Scrollbar closest step index ----------------------------- */

/* Return's the true index based on the inaccuret index and current raw position of the scrollbar. */
static Ulong scrollbar_closest_step_index(Uint idx, float rawpos, float *const array, Ulong arraylen) {
  ASSERT(array);
  ASSERT(arraylen);
  /* Start of with the first index as the closest, as that would happen no matter what. */
  Ulong index=0;
  float closest = FABSF(array[0] - rawpos);
  float value;
  /* Then check the remaining indexes. */
  for (Ulong i=1; i<arraylen; ++i) {
    /* Set the closest entry as index. */
    if ((value = FABSF(array[i] - rawpos)) < closest) {
      index = i;
      closest = value;
    }
  }
  /* If the array only has a size of 2 that means we are either at the top or the bottom.  And we only need to change
   * the return index if we are at the top, because even if we are at the bottom there are only 2 entries in the array. */
  if (arraylen == 2 && idx == 0) {
    return (idx + index);
  }
  return (idx - 1 + index);
}

/* ----------------------------- Scrollbar calculate ----------------------------- */

/* Calculate the height and the top y position relative to `total_pixel_length` of a scrollbar. */
static void scrollbar_calculate(float total_length, Uint start, Uint end,
  Uint visible, Uint current, float *const length, float *const relative_pos)
{
  ASSERT(length);
  ASSERT(relative_pos);
  *length       = scrollbar_length(total_length, start, end, visible);
  *relative_pos = scrollbar_step_value_for(current, total_length, *length, start, end);
}

/* ----------------------------- Scrollbar index ----------------------------- */

/* Return's the index based on a scrollbar's top `ypos` relative to `total_length`. */
static long scrollbar_index(float total_length, Uint start, Uint end, Uint visible, float y_pos) {
  float max_y_pos;
  float *array;
  float ratio;
  Ulong index;
  Ulong arraylen;
  /* Get the maximum possible y position. */
  max_y_pos = (total_length - scrollbar_length(total_length, start, end, visible));
  /* Because we alreade have a passed y position, we do a cheap clamp, meaning we only modify `y_pos` when it's outside the limit's. */
  CLAMP(y_pos, 0, max_y_pos);
  /* Calculate the ratio of the max y position that should be used to calculate the index.*/
  ratio = fclamp((y_pos / max_y_pos), 0, 1);
  /* First, we get the most likely index, this may be incorrect due to rounding and such. */
  index = lclamp((long)(ratio * (end - start)), 0, (end - start));
  /* Now we get an float array of at most the 3 closest index y position. */
  array = scrollbar_closest_step_values(index, total_length, start, end, visible, &arraylen);
  /* Then based on the first index and given y position, we adjust the index to the closest actual one to `y_pos`. */
  index = scrollbar_closest_step_index(index, y_pos, array, arraylen);
  free(array);
  return (index + start);
}

/* ----------------------------- Scrollbar calc routine ----------------------------- */

static void scrollbar_calc_routine(Scrollbar *const sb) {
  ASSERT_SCROLLBAR;
  float total_length;
  float length;
  float y_pos;
  float t_offset;
  float r_offset;
  Uint start;
  Uint end;
  Uint visible;
  Uint current;
  /* Use the update routine to get all relevent data. */
  sb->update(sb->data, &total_length, &start, &end, &visible, &current, &t_offset, &r_offset);
  /* Calculate the size and position of the thumb. */
  scrollbar_calculate(total_length, start, end, visible, current, &length, &y_pos);
  if (total_length <= 0 || length >= total_length) {
    sb->base->xflags  |= ELEMENT_HIDDEN;
    sb->thumb->xflags |= ELEMENT_HIDDEN;
  }
  else {
    /* Thumb. */
    sb->thumb->xflags &= ~ELEMENT_HIDDEN;
    sb->thumb->rel_y = y_pos;
    element_resize(sb->thumb, sb->thumb->width, length);
    /* Base. */
    sb->base->xflags &= ~ELEMENT_HIDDEN;
    sb->base->rel_x = (sb->base->width + r_offset);
    sb->base->rel_y = t_offset;
    element_resize(sb->base, sb->base->width, total_length);
  }
}



/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Scrollbar create ----------------------------- */

Scrollbar *scrollbar_create(Element *const parent, void *const data, ScrollbarUpdateFunc update, ScrollbarMovingFunc moving) {
  ASSERT(parent);
  ASSERT(data);
  ASSERT(update);
  ASSERT(moving);
  /* Top and right offset values. */
  float t_offset;
  float r_offset;
  /* Get these values directly so we can create the elements at the correct position directly. */
  update(data, NULL, NULL, NULL, NULL, NULL, &t_offset, &r_offset);
  /* Create the scrollbar. */
  Scrollbar *sb = xmalloc(sizeof(*sb));
  /* Init the state flag. */
  sb->xflags = 0;
  /* Element's. */
  sb->base  = element_create((parent->x + parent->width - (SB_WIDTH + r_offset)), (parent->y + t_offset), SB_WIDTH, (parent->height - t_offset), TRUE);
  sb->thumb = element_create(sb->base->x, sb->base->y, SB_WIDTH, 10, TRUE);
  element_set_parent(sb->base, parent);
  element_set_parent(sb->thumb, sb->base);
  element_set_data_sb(sb->base, sb);
  element_set_data_sb(sb->thumb, sb);
  sb->base->color  = PACKED_UINT_FLOAT(0.05f, 0.05f, 0.05f, 0.3f);
  sb->thumb->color = PACKED_UINT_FLOAT(1.0f, 1.0f, 1.0f, 0.3f);
  /* Base. */
  sb->base->xflags |= (ELEMENT_REVREL_X | ELEMENT_REL_Y | ELEMENT_HIDDEN);
  sb->base->rel_x = (SB_WIDTH + r_offset);
  sb->base->rel_y = t_offset;
  /* Thumb. */
  sb->thumb->xflags |= (ELEMENT_REL_POS | ELEMENT_ROUNDED_RECT | ELEMENT_HIDDEN);
  /* Data ptr. */
  sb->data = data;
  /* Update routine. */
  sb->update = update;
  /* Moving routine. */
  sb->moving = moving;
  return sb;
}

/* ----------------------------- Scrollbar move ----------------------------- */

void scrollbar_move(Scrollbar *const sb, float change) {
  ASSERT_SCROLLBAR;
  float total_length;
  Uint start;
  Uint end;
  Uint visible;
  sb->update(sb->data, &total_length, &start, &end, &visible, NULL, NULL, NULL);
  element_move_y_clamp(sb->thumb, (sb->thumb->y + change), sb->base->y, (sb->base->y + sb->base->height - sb->thumb->height));
  sb->moving(sb->data, scrollbar_index(total_length, start, end, visible, (sb->thumb->y - sb->base->y)));
}

/* ----------------------------- Scrollbar draw ----------------------------- */

void scrollbar_draw(Scrollbar *const sb) {
  ASSERT_SCROLLBAR;
  if (sb->xflags & SCROLLBAR_REFRESH) {
    scrollbar_calc_routine(sb);
    sb->xflags &= ~SCROLLBAR_REFRESH;
    if (sb->base->xflags & ELEMENT_HIDDEN) {
      return;
    }
  }
  element_draw(sb->base);
  element_draw(sb->thumb);
}

/* ----------------------------- Scrollbar refresh ----------------------------- */

void scrollbar_refresh(Scrollbar *const sb) {
  ASSERT_SCROLLBAR;
  sb->xflags |= SCROLLBAR_REFRESH;
}

/* ----------------------------- Scrollbar element is base ----------------------------- */

/* Return's `TRUE` when `e` is the `base` element of `sb`. */
bool scrollbar_element_is_base(Scrollbar *const sb, Element *const e) {
  ASSERT_SCROLLBAR;
  ASSERT(e);
  return (e == sb->base);
}

/* ----------------------------- Scrollbar element is thumb ----------------------------- */

/* Return's `TRUE` when `e` is the `thumb` element of `sb`. */
bool scrollbar_element_is_thumb(Scrollbar *const sb, Element *const e) {
  ASSERT_SCROLLBAR;
  ASSERT(e);
  return (e == sb->thumb);
}

/* ----------------------------- Scrollbar mouse pos routine ----------------------------- */

void scrollbar_mouse_pos_routine(Scrollbar *const sb, Element *const e, float was_y_pos, float y_pos) {
  ASSERT_SCROLLBAR;
  ASSERT(e);
  if (e == sb->thumb && y_pos != was_y_pos) {
    scrollbar_move(sb, (y_pos - was_y_pos));
  }
}

/* ----------------------------- Scrollbar width ----------------------------- */

float scrollbar_width(Scrollbar *const sb) {
  ASSERT_SCROLLBAR;
  return sb->base->width;
}

/* ----------------------------- Scrollbar show ----------------------------- */

void scrollbar_show(Scrollbar *const sb, bool show) {
  ASSERT_SCROLLBAR;
  if (show) {
    sb->base->xflags  &= ~ELEMENT_HIDDEN;
    sb->thumb->xflags &= ~ELEMENT_HIDDEN;
  }
  else {
    sb->base->xflags  |= ELEMENT_HIDDEN;
    sb->thumb->xflags |= ELEMENT_HIDDEN;
  }
  scrollbar_refresh(sb);
}

/* ----------------------------- Scrollbar set thumb color ----------------------------- */

void scrollbar_set_thumb_color(Scrollbar *const sb, bool active) {
  ASSERT_SCROLLBAR;
  if (!active) {
    element_set_color(sb->thumb, SB_THUMB_COLOR);
  }
  else {
    element_set_color(sb->thumb, SB_ACTIVE_THUMB_COLOR);
  }
}

/* ----------------------------- Scrollbar base routine mouse button dn ----------------------------- */

void scrollbar_base_routine_mouse_button_dn(Scrollbar *const sb, Element *const e, float x, float y) {
  ASSERT_SCROLLBAR;
  ASSERT(e);
  /* Never accept faulty input.  The caller must ensure this.  ALWAYS... */
  ALWAYS_ASSERT(e == sb->base);
  /* Move the thumb element to the correct position.  And set the thumb color to active. */
  scrollbar_move(sb, ((y - sb->thumb->y) - (sb->thumb->height / 2)));
  scrollbar_set_thumb_color(sb, TRUE);
  sb->xflags |= SCROLLBAR_REFRESH;
  /* Then call our mouse button click routine, and it will handle itself. */
  gl_mouse_routine_button_dn(SDL_BUTTON_LEFT, SDL_GetModState(), x, y);
}
