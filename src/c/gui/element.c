/** @file gui/element.c

  @author  Melwin Svensson.
  @date    15-5-2025.

 */
#include "../../include/c_proto.h"

Uint element_rect_shader = 0;

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
  return e;
}

Element *element_create(float x, float y, float width, float height, bool in_gridmap) {
  Element *e = element_create_internal();
  e->x      = x;
  e->y      = y;
  e->width  = width;
  e->height = height;
  if (in_gridmap) {
    element_grid_set(element_grid, e);
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
  element_grid_remove(element_grid, e);
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
}

/* Draw a `Element` structure using its internal values. */
void element_draw(Element *const e) {
  ASSERT(e);
  /* Only draw the element rect when the element is not hidden and the rect shader has been init. */
  if (!e->hidden && element_rect_shader) {
    glUseProgram(element_rect_shader);
    glUniform4f(glGetUniformLocation(element_rect_shader, "rectcolor"), e->color->r, e->color->g, e->color->b, e->color->a);
    glUniform2f(glGetUniformLocation(element_rect_shader, "elempos"), e->x, e->y);
    glUniform2f(glGetUniformLocation(element_rect_shader, "elemsize"), e->width, e->height);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
}

Element *element_from_pos(float x, float y) {
  if (element_grid_contains(element_grid, x, y)) {
    return element_grid_get(element_grid, x, y);
  }
  return NULL;
}

void element_move(Element *const e, float x, float y) {
  ASSERT(e);
  element_grid_remove(element_grid, e);
  e->x = x;
  e->y = y;
  element_grid_remove(element_grid, e);
}

void element_resize(Element *const e, float width, float height) {
  ASSERT(e);
  element_grid_remove(element_grid, e);
  e->width  = width;
  e->height = height;
  element_grid_set(element_grid, e);
}

void element_move_resize(Element *const e, float x, float y, float width, float height) {
  ASSERT(e);
  element_grid_remove(element_grid, e);
  e->x      = x;
  e->y      = y;
  e->width  = width;
  e->height = height;
  element_grid_set(element_grid, e);
}
