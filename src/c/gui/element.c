/** @file gui/element.c

  @author  Melwin Svensson.
  @date    15-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- element_create_internal ----------------------------- */

static Element *element_create_internal(void) {
  Element *e = xmalloc(sizeof *e);
  /* State flags. */
  e->xflags    = ELEMENT_XFLAGS_DEFAULT;
  e->xrectopts = 0U;
  /* Boolian flags. */
  e->dt = ELEMENT_DATA_NONE;
  /* Position. */
  e->x     = 0;
  e->y     = 0;
  e->rel_x = 0;
  e->rel_y = 0;
  /* Size. */
  e->width      = 0;
  e->height     = 0;
  e->rel_width  = 0;
  e->rel_height = 0;
  /* Color. */
  e->color      = 0;
  e->text_color = 0;
  /* Lable. */
  e->lable     = NULL;
  e->lable_len = 0;
  /* Family. */
  e->parent   = NULL;
  e->children = cvec_create();
  /* Cursor. */
  e->cursor = SDL_SYSTEM_CURSOR_DEFAULT;
  /* Rect buffer. */
  e->rect_buffer = vertex_buffer_new(RECT_VERTBUF);
  /* Layer. */
  e->layer = 0;
  e->border_lsize = 0;
  e->border_tsize = 0;
  e->border_rsize = 0;
  e->border_bsize = 0;
  e->border_color = 0;
  return e;
}

/* ----------------------------- Element children relative pos ----------------------------- */

/* TODO: Rename. */
static void element_children_relative_pos(Element *const e) {
  ASSERT(e);
  float x;
  float y;
  float width;
  float height;
  /* Check all children. */
  ELEMENT_CHILDREN_ITER(e, i, child,
    /* Set these to the current values for `child`.  So we can check later if a move or resize is needed. */
    x      = child->x;
    y      = child->y;
    width  = child->width;
    height = child->height;
    /* If the element whishes to be placed in the absolute center of its parent, we oblige. */
    if (child->xflags & ELEMENT_CENTER_X) {
      x = (e->x + ((e->width / 2) - (child->width / 2)));
      /* We should constrain the width of the element to the parent when the width would be smaller then the prefered width. */
      if ((child->xflags & ELEMENT_CONSTRAIN_WIDTH)) {
        if (x < e->x || child->prefered_width > e->width) {
          x     = e->x;
          width = e->width;
        }
        else {
          width = child->prefered_width;
        }
      }
    }
    /* Otherwise, check for other relative position/size. */
    else {
      /* Check reverse or regular relative x position. */
      if (child->xflags & ELEMENT_REL_X) {
        x = (e->x + child->rel_x);
      }
      else if (child->xflags & ELEMENT_REVREL_X) {
        x = ((e->x + e->width) - child->rel_x);
      }
      /* Check reverse or regular relative y position. */
      if (child->xflags & ELEMENT_REL_Y) {
        y = (e->y + child->rel_y);
      }
      else if (child->xflags & ELEMENT_REVREL_Y) {
        y = ((e->y + e->height) - child->rel_y);
      }
      /* Relative width. */
      if (child->xflags & ELEMENT_REL_WIDTH) {
        width = (e->width - child->rel_x - child->rel_width);
      }
      /* Relative_height. */
      if (child->xflags & ELEMENT_REL_HEIGHT) {
        height = (e->height - child->rel_y - child->rel_height);
      }
    }
    /* If any value has changed, call `element_move_resize()`. */
    if (x != child->x || y != child->y || width != child->width || height != child->height) {
      element_move_resize(child, x, y, width, height);
    }
  );
}

/* ----------------------------- Bazier indices create ----------------------------- */

static Uint *bazier_indices_create(Ulong n, Ulong *const len) {
  ASSERT(len);
  *len = ((n - 1) * 3);
  Uint *ret = xmalloc(sizeof(Uint) * (*len));
  for (Ulong i=0, no=1; i<(*len); i+=3, ++no) {
    ret[i]   = 0;
    ret[i+1] = no;
    ret[i+2] = (no + 1);
  }
  return ret;
}

/* ----------------------------- Bazier corner arc ----------------------------- */

static void bazier_corner_arc(float corner_x, float corner_y, float center_x,
  float center_y, float t, float *const arcx, float *const arcy, Ulong n)
{
  ASSERT(arcx);
  ASSERT(arcy);
  float ax;
  float ay;
  flerp(t, corner_x, corner_y, center_x, center_y, &ax, &ay);
  bazier_arc(corner_x, center_y, center_x, corner_y, ax, ay, arcx, arcy, n);
}

/* ----------------------------- Bazier vertex create ----------------------------- */

static RectVertex *bazier_vertex_create(Ulong n) {
  return xmalloc(sizeof(RectVertex) * (n + 1));
}

/* ----------------------------- Bazier vertex insert ----------------------------- */

static void bazier_vertex_insert(RectVertex *vert, Ulong n, float cx, float cy, float *const arcx, float *const arcy, Uint color) {
  UNPACK_FUINT_VARS(color, r,g,b,a);
  vert[0] = (RectVertex){ cx,cy, r,g,b,a };
  for (Ulong i=0; i<n; ++i) {
    vert[i+1] = (RectVertex){ arcx[i],arcy[i], r,g,b,a };
  }
}

/* ----------------------------- Element add rounded center rect ----------------------------- */

static void element_add_rounded_center_rect(Element *const e, float offset) {
  ASSERT(e);
  RectVertex vert[4];
  /* Create the center rect. */
  if (e->width > e->height) {
    shader_rect_vertex_load(vert, (e->x + offset), e->y, (e->width - (offset * 2)), e->height, e->color);
  }
  else {
    shader_rect_vertex_load(vert, e->x, (e->y + offset), e->width, (e->height - (offset * 2)), e->color);
  }
  vertex_buffer_push_back(e->rect_buffer, ARRAY__LEN(vert), ARRAY__LEN(RECT_INDICES));
}

/* ----------------------------- Element instert bazier arc corner ----------------------------- */

static void element_insert_bazier_arc_corner(Element *const e, RectVertex *vert, Uint *indi,
  Ulong indi_len, Ulong n, float corner_x, float corner_y, float center_x, float center_y)
{
  ASSERT(e);
  float arcx[n];
  float arcy[n];
  float t = UCHAR_TO_FLOAT(UNPACK_INT_DATA(e->xrectopts, 0, Uchar));
  bazier_corner_arc(corner_x, corner_y, center_x, center_y, t, arcx, arcy, n);
  bazier_vertex_insert(vert, n, center_x, center_y, arcx, arcy, e->color);
  vertex_buffer_push_back(e->rect_buffer, vert, (n + 1), indi, indi_len);
}
 
/* ----------------------------- Element rounded rect ----------------------------- */

static void element_rounded_rect(Element *const e) {
  ASSERT(e);
  Ulong n = 19;
  RectVertex *vert = bazier_vertex_create(n);
  Ulong indi_len;
  Uint *indi = bazier_indices_create(n, &indi_len);
  float offset = (FMINF(e->width, e->height) / 2);
  element_add_rounded_center_rect(e, offset);
  /* Top-Left */
  element_insert_bazier_arc_corner(e, vert, indi, indi_len, n, e->x, e->y, (e->x + offset), (e->y + offset));
  /* Top-Right */
  element_insert_bazier_arc_corner(e, vert, indi, indi_len, n,
    (e->x + e->width), e->y, ((e->x + e->width - offset)), (e->y + offset));
  /* Bottom-Left */
  element_insert_bazier_arc_corner(e, vert, indi, indi_len, n,
    e->x, (e->y + e->height), (e->x + offset), ((e->y + e->height) - offset));
  /* Bottom-Right */
  element_insert_bazier_arc_corner(e, vert, indi, indi_len, n,
    (e->x + e->width), (e->y + e->height), ((e->x + e->width) - offset), ((e->y + e->height) - offset));
  free(indi);
  free(vert);
}

/* ----------------------------- Element draw rect ----------------------------- */

static inline void element_draw_rect(Element *const e) {
  ASSERT(e);
  RectVertex vert[4];
  /* Only update the rect vertex buffer when needed. */
  if (e->xflags & ELEMENT_RECT_REFRESH) {
    /* If the user has set a extra routine to run when refreshing the rect, we call it here. */
    if (e->cbdata && e->extra_routine_rect) {
      e->extra_routine_rect(e, e->cbdata);
    }
    vertex_buffer_clear(e->rect_buffer);
    if (e->xflags & ELEMENT_ROUNDED_RECT) {
      element_rounded_rect(e);
    }
    else {
      shader_rect_vertex_load(vert, e->x, e->y, e->width, e->height, e->color);
      vertex_buffer_push_back(e->rect_buffer, vert, 4, RECT_INDICES, RECT_INDICES_LEN);
      if (e->xflags & ELEMENT_HAS_BORDERS) {
        shader_rect_vertex_load(vert, e->x, e->y, e->border_lsize, e->height, e->border_color);
        vertex_buffer_push_back(e->rect_buffer, vert, 4, RECT_INDICES, RECT_INDICES_LEN);
        shader_rect_vertex_load(vert, e->x, e->y, e->width, e->border_tsize, e->border_color);
        vertex_buffer_push_back(e->rect_buffer, vert, 4, RECT_INDICES, RECT_INDICES_LEN);
        shader_rect_vertex_load(vert, (e->x + e->width - e->border_rsize), e->y, e->border_rsize, e->height, e->border_color);
        vertex_buffer_push_back(e->rect_buffer, vert, 4, RECT_INDICES, RECT_INDICES_LEN);
        shader_rect_vertex_load(vert, e->x, (e->y + e->height - e->border_bsize), e->width, e->border_bsize, e->border_color);
        vertex_buffer_push_back(e->rect_buffer, vert, 4, RECT_INDICES, RECT_INDICES_LEN);
      }
    }
    e->xflags &= ~ELEMENT_RECT_REFRESH;
  }
  glUseProgram(rect_shader); {
    vertex_buffer_render(e->rect_buffer, GL_TRIANGLES);
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- element_create ----------------------------- */

/* Create a new allocated `element`.  Note that is `in_gridmap` is `TRUE`
 * then the element must also be hidden to not react to mouse events. */
Element *element_create(float x, float y, float width, float height, bool in_gridmap) {
  Element *e = element_create_internal();
  e->x      = x;
  e->y      = y;
  e->width  = width;
  e->height = height;
  /* Always set the prefered width. */
  e->prefered_width = width;
  if (in_gridmap) {
    element_grid_set(e);
  }
  else {
    e->xflags |= ELEMENT_NOT_IN_MAP;
  }
  e->cbdata = NULL;
  e->extra_routine_rect = NULL;
  return e;
}

/* ----------------------------- element_free ----------------------------- */

void element_free(Element *const e) {
  if (!e) {
    return;
  }
  if (e->parent) {
    cvec_remove_by_value(e->parent->children, e);
  }
  while (cvec_len(e->children)) {
    element_free(cvec_get(e->children, 0));
  }
  element_grid_remove(e);
  free(e->lable);
  vertex_buffer_delete(e->rect_buffer);
  free(e);
}

/* ----------------------------- Element draw ----------------------------- */

/* Draw a `Element` structure using its internal values. */
void element_draw(Element *const e) {
  ASSERT(e);
  /* Only draw the element rect when the element is not hidden and the rect shader has been init. */
  if (!(e->xflags & ELEMENT_HIDDEN) && rect_shader) {
    element_draw_rect(e);
  }
}

/* ----------------------------- Element move ----------------------------- */

void element_move(Element *const e, float x, float y) {
  ASSERT(e);
  element_grid_remove(e);
  if (!(e->x == x && e->y == y)) {
    e->xflags |= ELEMENT_RECT_REFRESH;
  }
  e->x = x;
  e->y = y;
  element_children_relative_pos(e);
  element_grid_set(e);
}

/* ----------------------------- Element resize ----------------------------- */

void element_resize(Element *const e, float width, float height) {
  ASSERT(e);
  element_grid_remove(e);
  if (e->width != width || e->height != height) {
    e->xflags |= ELEMENT_RECT_REFRESH;
  }
  e->width  = width;
  e->height = height;
  element_children_relative_pos(e);
  element_grid_set(e);
}

/* ----------------------------- Element move resize ----------------------------- */
 
void element_move_resize(Element *const e, float x, float y, float width, float height) {
  ASSERT(e);
  element_grid_remove(e);
  if (!(e->x == x && e->y == y && e->width == width && e->height == height)) {
    e->xflags |= ELEMENT_RECT_REFRESH;
  }
  e->x      = x;
  e->y      = y;
  e->width  = width;
  e->height = height;
  element_children_relative_pos(e);
  element_grid_set(e);
}

/* ----------------------------- Element move y clamp ----------------------------- */

/* Move `e` to a new y position `y` while clamping it so it can be no less then `min` and no more then `max`. */
void element_move_y_clamp(Element *const e, float y, float min, float max) {
  ASSERT(e);
  float newy = fclamp(y, min, max);
  /* Only perform any action when the passed y value is not the same as the current. */
  element_grid_remove(e);
  if (e->y != newy) {
    e->xflags |= ELEMENT_RECT_REFRESH;
  }
  e->y = newy;
  element_children_relative_pos(e);
  element_grid_set(e);
}

/* ----------------------------- Element delete borders ----------------------------- */

/* Delete the borders of `e`.  If any exests. */
void element_delete_borders(Element *const e) {
  ASSERT(e);
  e->xflags &= ~ELEMENT_HAS_BORDERS;
}

/* ----------------------------- Element set color ----------------------------- */

/* Set the color of `e`, and also make the element refresh its rect buffer and tell the gui it needs to redraw. */
void element_set_color(Element *const e, Uint color) {
  ASSERT(e);
  if (e->color != color) {
    e->color = color;
    e->xflags |= ELEMENT_RECT_REFRESH;
    refresh_needed = TRUE;
  }
}

/* ----------------------------- Element is ancestor ----------------------------- */

/* Returns 'TRUE' when 'ancestor' is an ancestor to e or is e itself. */
bool element_is_ancestor(Element *const e, Element *const ancestor) {
  if (!e || !ancestor) {
    return FALSE;
  }
  Element *element = e;
  while (element) {
    if (element == ancestor) {
      return TRUE;
    }
    element = element->parent;
  }
  return FALSE;
}

/* ----------------------------- Element set lable ----------------------------- */

void element_set_lable(Element *const e, const char *const restrict lable, Ulong len) {
  ASSERT(e);
  ASSERT(lable);
  if (e->xflags & ELEMENT_LABLE) {
    free(e->lable);
  }
  e->lable_len = len;
  e->lable     = measured_copy(lable, len);
  e->xflags   |= ELEMENT_LABLE;
}

/* ----------------------------- Element set borders ----------------------------- */

void element_set_borders(Element *const e, float lsize, float tsize, float rsize, float bsize, Uint color) {
  ASSERT(e);
  ASSERT(color);
  e->xflags |= (ELEMENT_HAS_BORDERS | ELEMENT_RECT_REFRESH);
  e->border_lsize = lsize;
  e->border_tsize = tsize;
  e->border_rsize = rsize;
  e->border_bsize = bsize;
  e->border_color = color;
}

/* ----------------------------- Element set layers ----------------------------- */

/* Set the event layer of `e`.  Note that this does not change drawing layer as that depends on the order of drawing. */
void element_set_layer(Element *const e, Ushort layer) {
  ASSERT(e);
  e->layer = layer;
  ELEMENT_CHILDREN_ITER(e, i, child,
    if (child->xflags & ELEMENT_SET_OWN_LAYER) {
      element_set_layer(child, child->layer);
    }
    else {
      element_set_layer(child, (layer + 1));
    }
  );
}

/* ----------------------------- Element set parent ----------------------------- */

/* Set the parent of `e` to `p`, as well as adding `e` to the children vector of `p`. */
void element_set_parent(Element *const e, Element *const p) {
  ASSERT(e);
  ASSERT(p);
  e->parent = p;
  cvec_push(p->children, e);
  element_set_layer(p, p->layer);
}

/* ----------------------------- Element set raw data ----------------------------- */

/* Set the internal data ptr of `e` to `void *` data.  Note that this
 * should be the only way of setting the internal data of a element. */
void element_set_data_raw(Element *const e, void *const data) {
  ASSERT(e);
  ASSERT(data);
  e->dt = ELEMENT_DATA_RAW;
  e->dp_raw = data;
}

/* ----------------------------- Element set sb data ----------------------------- */

/* Set the internal data ptr of `e` to `Scrollbar *` data.  Note that
 * this should be the only way of setting the internal data of a element. */
void element_set_data_sb(Element *const e, Scrollbar *const data) {
  ASSERT(e);
  ASSERT(data);
  e->dt = ELEMENT_DATA_SB;
  e->dp_sb = data;
}

/* ----------------------------- Element set menu data ----------------------------- */

/* Set the internal data ptr of `e` to `Menu *` data.  Note that this
 * should be the only way of setting the internal data of a element. */
void element_set_data_menu(Element *const e, Menu *const data) {
  ASSERT(e);
  ASSERT(data);
  e->dt = ELEMENT_DATA_MENU;
  e->dp_menu = data;
}

/* ----------------------------- Element set file data ----------------------------- */

/* Set the internal data ptr of `e` to `openfilestruct *` data.  Note that
 * this should be the only way of setting the internal data of a element. */
void element_set_data_file(Element *const e, openfilestruct *const data) {
  ASSERT(e);
  ASSERT(data);
  e->dt = ELEMENT_DATA_FILE;
  e->dp_file = data;
}

/* ----------------------------- Element set editor data ----------------------------- */

/* Set the internal data ptr of `e` to `Editor *` data.  Note that this
 * should be the only way of setting the internal data of a element. */
void element_set_data_editor(Element *const e, Editor *const data) {
  ASSERT(e);
  ASSERT(data);
  e->dt = ELEMENT_DATA_EDITOR;
  e->dp_editor = data;
}

/* ----------------------------- Element set rounded apex  ----------------------------- */

/* Set the apex fraction in terms of the precentage (0-1) in total
 * magnitude from the true corner and the center of the arch. */
void element_set_rounded_apex_fraction(Element *const e, float t) {
  ASSERT(e);
  PACK_INT_DATA(e->xrectopts, FLOAT_TO_UCHAR(t), 0);
}

/* ----------------------------- element_set_data_callback ----------------------------- */

/* Set the `void *` that will be passed to all extra routine callbacks. */
void element_set_data_callback(Element *const e, void *const ptr) {
  ASSERT(e);
  ASSERT(ptr);
  e->cbdata = ptr;
}

/* ----------------------------- element_set_extra_routine_rect ----------------------------- */

/* Set the callback that will be called when updating the rect of `e`.
 * Note that this is an extra thing to be performed, and not a replacement. */
void element_set_extra_routine_rect(Element *const e, ElementExtraCallback callback) {
  ASSERT(e);
  /* Ensure that the caller must have set the callback data ptr before setting the callback. */
  ASSERT(e->cbdata);
  ASSERT(callback);
  e->extra_routine_rect = callback;
}

/* ----------------------------- element_copy ----------------------------- */

Element *element_copy(Element *src, Element *parent) {
  ASSERT(src);
  Element *copy = element_create(src->x, src->y, src->width, src->height, !(src->xflags & ELEMENT_NOT_IN_MAP));
  if (parent) {
    element_set_parent(copy, parent);
  }
  copy->layer          = src->layer;
  copy->rel_x          = src->rel_x;
  copy->rel_y          = src->rel_y;
  copy->rel_width      = src->rel_width;
  copy->rel_height     = src->rel_height;
  copy->prefered_width = src->prefered_width;
  copy->color          = src->color;
  copy->text_color     = src->text_color;
  /* This needs to be verified, but should correctly work. */
  ELEMENT_CHILDREN_ITER(src, i, c,
    element_set_parent(element_copy(c, NULL), copy);
  );
  copy->cursor             = src->cursor;
  copy->xflags             = src->xflags;
  copy->xrectopts          = src->xrectopts;
  copy->dt                 = src->dt;
  copy->dp_raw             = src->dp_raw;
  copy->border_lsize       = src->border_lsize;
  copy->border_tsize       = src->border_tsize;
  copy->border_rsize       = src->border_rsize;
  copy->border_bsize       = src->border_bsize;
  copy->border_color       = src->border_color;
  copy->cbdata             = src->cbdata;
  copy->extra_routine_rect = src->extra_routine_rect;
  /* Ensure the lable is the same. */
  if (src->xflags & ELEMENT_LABLE) {
    element_set_lable(copy, src->lable, src->lable_len);
  }
  return copy;
}
