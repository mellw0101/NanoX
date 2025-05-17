/** @file gui/element.c

  @author  Melwin Svensson.
  @date    15-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


Uint element_rect_shader = 0;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static Element *element_create_internal(void) {
  Element *e = xmalloc(sizeof(*e));
  /* Boolian flags. */
  e->hidden                     = FALSE;
  e->has_lable                  = FALSE;
  e->has_relative_pos           = FALSE;
  e->has_relative_x_pos         = FALSE;
  e->has_relative_y_pos         = FALSE;
  e->has_reverse_relative_pos   = FALSE;
  e->has_reverse_relative_x_pos = FALSE;
  e->has_reverse_relative_y_pos = FALSE;
  e->has_relative_width         = FALSE;
  e->has_relative_height        = FALSE;
  e->is_border                  = FALSE;
  e->has_borders                = FALSE;
  e->not_in_gridmap             = FALSE;
  e->has_raw_data               = FALSE;
  e->has_file_data              = FALSE;
  e->has_editor_data            = FALSE;
  e->has_sb_data                = FALSE;
  e->has_menu_data              = FALSE;
  e->is_above                   = FALSE;
  /* Position. */
  e->x          = 0;
  e->y          = 0;
  e->relative_x = 0;
  e->relative_y = 0;
  /* Size. */
  e->width           = 0;
  e->height          = 0;
  e->relative_width  = 0;
  e->relative_height = 0;
  /* Color. */
  e->color      = color_create(0, 0, 0, 0);
  e->text_color = color_create(0, 0, 0, 0);
  /* Lable. */
  e->lable     = NULL;
  e->lable_len = 0;
  /* Family. */
  e->parent   = NULL;
  e->children = cvec_create();
  /* Cursor. */
  e->cursor = GLFW_ARROW_CURSOR;
  /* Layer. */
  e->layer = 0;
  return e;
}

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
    /* Normal relative position. */
    if (child->has_relative_pos) {
      x = (e->x + child->relative_x);
      y = (e->y + child->relative_y);
    }
    /* Reverse relative position. */
    else if (child->has_reverse_relative_pos) {
      x = ((e->x + e->width)  - child->relative_x); 
      y = ((e->y + e->height) - child->relative_y);
    }
    /* Check reverse or regular relative x position. */
    if (child->has_relative_x_pos) {
      x = (e->x + child->relative_x);
    }
    else if (child->has_reverse_relative_x_pos) {
      x = ((e->x + e->width) - child->relative_x);
    }
    /* Check reverse or regular relative y position. */
    if (child->has_relative_y_pos) {
      y = (e->y + child->relative_y);
    }
    else if (child->has_reverse_relative_y_pos) {
      y = ((e->y + e->height) - child->relative_y);
    }
    /* Relative width. */
    if (child->has_relative_width) {
      width = (e->width - child->relative_x - child->relative_width);
    }
    /* Relative_height. */
    if (child->has_relative_height) {
      height = (e->height - child->relative_y - child->relative_height);
    }
    /* If any value has changed, call `element_move_resize()`. */
    if (x != child->x || y != child->y || width != child->width || height != child->height) {
      element_move_resize(child, x, y, width, height);
    }
  );
}

static inline void element_draw_rect(Element *const e) {
  static int c_loc = 0;
  static int p_loc = 0;
  static int s_loc = 0;
  glUseProgram(element_rect_shader);
  !c_loc ? (c_loc = glGetUniformLocation(element_rect_shader, "rectcolor")) : ((int)0);
  !p_loc ? (p_loc = glGetUniformLocation(element_rect_shader, "elempos"))   : ((int)0);
  !s_loc ? (s_loc = glGetUniformLocation(element_rect_shader, "elemsize"))  : ((int)0);
  glUniform4fv(c_loc, 1, (void *)e->color);
  glUniform2fv(p_loc, 1, (void *)STRUCT_FIELD_PTR(e, x));
  glUniform2fv(s_loc, 1, (void *)STRUCT_FIELD_PTR(e, width));
  glDrawArrays(GL_TRIANGLES, 0, 6);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


Element *element_create(float x, float y, float width, float height, bool in_gridmap) {
  Element *e = element_create_internal();
  e->x      = x;
  e->y      = y;
  e->width  = width;
  e->height = height;
  if (in_gridmap) {
    element_grid_set(e);
  }
  else {
    e->not_in_gridmap = TRUE;
  }
  return e;
}

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
  free(e->color);
  free(e->text_color);
  free(e->lable);
  free(e);
}

void element_set_parent(Element *const e, Element *const parent) {
  ASSERT(e);
  ASSERT(parent);
  e->parent = parent;
  cvec_push(parent->children, e);
  element_set_layer(parent, parent->layer);
}

// void element_set_color(Element *const e, float r, float g, float b, float a) {
//   ASSERT(e);
//   color_set_rgba(e->color, r, g, b, a);
// }

/* Draw a `Element` structure using its internal values. */
void element_draw(Element *const e) {
  ASSERT(e);
  /* Only draw the element rect when the element is not hidden and the rect shader has been init. */
  if (!e->hidden && element_rect_shader) {
    element_draw_rect(e);
    /* If the element has borders, draw them to. */
    if (e->has_borders) {
      ELEMENT_CHILDREN_ITER(e, i, c,
        if (c->is_border) {
          element_draw_rect(c);
        }
      );
    }
  }
}

Element *element_from_pos(float x, float y) {
  if (element_grid_contains(x, y)) {
    return element_grid_get(x, y);
  }
  return NULL;
}

void element_move(Element *const e, float x, float y) {
  ASSERT(e);
  element_grid_remove(e);
  e->x = x;
  e->y = y;
  element_children_relative_pos(e);
  element_grid_remove(e);
}

void element_resize(Element *const e, float width, float height) {
  ASSERT(e);
  element_grid_remove(e);
  e->width  = width;
  e->height = height;
  element_children_relative_pos(e);
  element_grid_set(e);
}

void element_move_resize(Element *const e, float x, float y, float width, float height) {
  ASSERT(e);
  element_grid_remove(e);
  e->x      = x;
  e->y      = y;
  e->width  = width;
  e->height = height;
  element_children_relative_pos(e);
  element_grid_set(e);
}

/* Move `e` to a new y position `y` while clamping it so it can be no less then `min` and no more then `max`. */
void element_move_y_clamp(Element *const e, float y, float min, float max) {
  ASSERT(e);
  /* Only perform any action when the passed y value is not the same as the current. */
  element_grid_remove(e);
  e->y = fclamp(y, min, max);
  element_children_relative_pos(e);
  element_grid_set(e);
}

/* Delete the borders of `e`.  If any exests. */
void element_delete_borders(Element *const e) {
  ASSERT(e);
  ELEMENT_CHILDREN_ITER(e, i, c,
    if (c->is_border) {
      element_free(c);
      --i;
    }
  );
}

void element_set_borders(Element *const e, float lsize, float tsize, float rsize, float bsize, Color *color) {
  ASSERT(e);
  ASSERT(color);
  /* Create all element's. */
  Element *l = element_create(e->x,                      e->y,                       lsize,    e->height, FALSE);
  Element *t = element_create(e->x,                      e->y,                       e->width, tsize,     FALSE);
  Element *r = element_create((e->x + e->width - rsize), e->y,                       rsize,    e->height, FALSE);
  Element *b = element_create(e->x,                      (e->y + e->height - bsize), e->width, bsize,     FALSE);
  /* Delete the current border's of `e` if it has them.  TODO: implement resizing and recoloring the existing ones when they exist. */
  element_delete_borders(e);
  e->has_borders = TRUE;
  /* Set `e` as the parent for all borders. */
  element_set_parent(l, e);
  element_set_parent(t, e);
  element_set_parent(r, e);
  element_set_parent(b, e);
  /* Set the color of all borders. */
  color_copy(l->color, color);
  color_copy(t->color, color);
  color_copy(r->color, color);
  color_copy(b->color, color);
  /* Left. */
  l->is_border           = TRUE;
  l->has_relative_pos    = TRUE;
  l->has_relative_height = TRUE;
  /* Top. */
  t->is_border          = TRUE;
  t->has_relative_pos   = TRUE;
  t->has_relative_width = TRUE;
  /* Right. */
  r->is_border                  = TRUE;
  r->has_reverse_relative_x_pos = TRUE;
  r->has_relative_y_pos         = TRUE;
  r->has_relative_height        = TRUE;
  r->relative_x                 = rsize;
  /* Bottom. */
  b->is_border                  = TRUE;
  b->has_reverse_relative_y_pos = TRUE;
  b->has_relative_x_pos         = TRUE;
  b->has_relative_width         = TRUE;
  b->relative_y                 = bsize;
}

/* Set the event layer of `e`.  Note that this does not change drawing layer as that depends on the order of drawing. */
void element_set_layer(Element *const e, Ushort layer) {
  ASSERT(e);
  if (e->parent) {
    e->layer = (e->parent->layer + 1);
  }
  else {
    e->layer = layer;
  }
  ELEMENT_CHILDREN_ITER(e, i, child,
    element_set_layer(child, 0);
  );
}

/* ----------------------------- Boolian function's ----------------------------- */

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

/* ----------------------------- Internal data ptr set function's ----------------------------- */

void element_set_raw_data(Element *const e, void *const data) {
  ASSERT(e);
  ASSERT(data);
  e->has_raw_data  = TRUE;
  e->has_sb_data   = FALSE;
  e->has_menu_data = FALSE;
  e->dp_raw        = data;
}

void element_set_sb_data(Element *const e, Scrollbar *const data) {
  ASSERT(e);
  ASSERT(data);
  e->has_raw_data  = FALSE;
  e->has_sb_data   = TRUE;
  e->has_menu_data = FALSE;
  e->dp_sb         = data;
}

void element_set_menu_data(Element *const e, CMenu *const data) {
  ASSERT(e);
  ASSERT(data);
  e->has_raw_data  = FALSE;
  e->has_sb_data   = FALSE;
  e->has_menu_data = TRUE;
  e->dp_menu       = data;
}
