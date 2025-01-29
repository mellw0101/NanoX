/** @file guielement.cpp

  @author  Melwin Svensson.
  @date    28-1-2025.

 */
#include "../../include/prototypes.h"

/* Create a ui element. */
guielement *make_element(vec2 pos, vec2 size, vec2 endoff, vec4 color, bool in_gridmap) _NOTHROW {
  guielement *newelement = (guielement *)nmalloc(sizeof(*newelement));
  newelement->pos          = pos;
  newelement->relative_pos = 0;
  newelement->size         = size;
  newelement->endoff       = endoff;
  newelement->color        = color;
  newelement->textcolor    = 0.0f;
  newelement->lable        = NULL;
  newelement->lablelen     = 0;
  newelement->callback     = NULL;
  newelement->parent       = NULL;
  newelement->children     = MVector<guielement*>{};
  newelement->flag.clear();
  if (!in_gridmap) {
    newelement->flag.set<GUIELEMENT_NOT_IN_GRIDMAP>();
  }
  gridmap.set(newelement);
  return newelement;
}

/* Create a blank element. */
guielement *make_element(bool in_gridmap) _NOTHROW {
  return make_element(-100.0f, 20.0f, 0.0f, 0.0f, in_gridmap);
}

/* Create a new child to element `parent`. */
guielement *make_element_child(guielement *parent, bool in_gridmap) {
  guielement *element = make_element(in_gridmap);
  element->parent = parent;
  parent->children.push_back(element);
  return element;
}

/* Delete a element and all its children. */
void delete_element(guielement *element) {
  /* Make this function NULL-SAFE, by exiting early when the passed element is not valid. */
  if (!element) {
    return;
  }
  /* First start by running this function recursivly on all children of element. */
  for (Ulong i = 0; i < element->children.size(); ++i) {
    delete_element(element->children[i]);
  }
  /* Then remove it from the global gridmap and freeing the lable and the element itself. */
  gridmap.remove(element);
  free(element->lable);
  free(element);
}

/* Set an elements lable text. */
void set_element_lable(guielement *element, const char *string) _NOTHROW {
  if (element->flag.is_set<GUIELEMENT_HAS_LABLE>()) {
    free(element->lable);
  }
  element->lable    = copy_of(string);
  element->lablelen = strlen(string);
  element->flag.set<GUIELEMENT_HAS_LABLE>();
}

/* Delete all children of `element`. */
void delete_guielement_children(guielement *element) _NOTHROW {
  if (element->children.size()) {
    for (Ulong i = 0; i < element->children.size(); ++i) {
      delete_element(element->children[i]);
    }
    element->children.resize(0);
  }
}

/* Get ui element from current mouse position. */
guielement *element_from_mousepos(void) {
  ivec2 pos(mousepos);
  if (gridmap.contains(pos)) {
    return gridmap.get(pos);
  }
  return NULL;
}

/* Resize an element as well as correctly adjust it in the gridmap. */
void resize_element(guielement *e, vec2 size) {
  gridmap.remove(e);
  e->size = size;
  gridmap.set(e);
}

/* Move an element as well as correctly adjust it in the gridmap. */
void move_element(guielement *e, vec2 pos) {
  gridmap.remove(e);
  e->pos = pos;
  for (Ulong i = 0; i < e->children.size(); ++i) {
    /* If this child uses relative based positioning the adjust the child position based on the parent. */
    if (e->children[i]->flag.is_set<GUIELEMENT_RELATIVE_POS>()) {
      move_element(e->children[i], (e->pos + e->children[i]->relative_pos));
    }
    /* Else, when the relative position is reversed as in the zero point is the right-down corner as apposed to normal (left-top side). */
    else if (e->children[i]->flag.is_set<GUIELEMENT_REVERSE_RELATIVE_POS>()) {
      move_element(e->children[i], ((e->pos + e->size) - e->children[i]->relative_pos));
    }
  }
  gridmap.set(e);
}

/* Move and resize an element and correctly set it in the gridmap. */
void move_resize_element(guielement *e, vec2 pos, vec2 size) {
  gridmap.remove(e);
  e->pos  = pos;
  for (Ulong i = 0; i < e->children.size(); ++i) {
    /* If this child uses relative based positioning the adjust the child position based on the parent. */
    if (e->children[i]->flag.is_set<GUIELEMENT_RELATIVE_POS>()) {
      move_element(e->children[i], (e->pos + e->children[i]->relative_pos));
    }
    /* Else, when the relative position is reversed as in the zero point is the right-down corner as apposed to normal (left-top side). */
    else if (e->children[i]->flag.is_set<GUIELEMENT_REVERSE_RELATIVE_POS>()) {
      move_element(e->children[i], ((e->pos + e->size) - e->children[i]->relative_pos));
    }
  }
  e->size = size;
  gridmap.set(e);
}

/* Delete the border elements of element `e`. */
void delete_element_borders(guielement *e) {
  for (Ulong i = 0; i < e->children.size(); ++i) {
    if (e->children[i]->flag.is_set<GUIELEMENT_IS_BORDER>()) {
      delete_element(e->children[i]);
      e->children.erase_at(i);
      --i;
    }
  }
  e->flag.unset<GUIELEMENT_HAS_BORDERS>();
}

/* Create borders for element `e` of `size` with `color`. */
void set_element_borders(guielement *e, vec4 size, vec4 color) {
  guielement *left, *top, *right, *bottom;
  /* First remove the current border, if it has them. */
  delete_element_borders(e);
  /* Create all sides as children of e. */
  left   = make_element_child(e, FALSE);
  top    = make_element_child(e, FALSE);
  right  = make_element_child(e, FALSE);
  bottom = make_element_child(e, FALSE);
  /* Then set the flag to mark these as border elements. */
  left->flag.set<GUIELEMENT_IS_BORDER>();
  top->flag.set<GUIELEMENT_IS_BORDER>();
  right->flag.set<GUIELEMENT_IS_BORDER>();
  bottom->flag.set<GUIELEMENT_IS_BORDER>();
  /* Set normal relative positioning for the left and top border. */
  left->flag.set<GUIELEMENT_RELATIVE_POS>();
  top->flag.set<GUIELEMENT_RELATIVE_POS>();
  /* And reverse relative positioning for the bottom and right border. */
  right->flag.set<GUIELEMENT_REVERSE_RELATIVE_POS>();
  bottom->flag.set<GUIELEMENT_REVERSE_RELATIVE_POS>();
  /* Set the color. */
  left->color   = color;
  top->color    = color;
  right->color  = color;
  bottom->color = color;
  /* Set the border positions. */
  move_resize_element(left, e->pos, vec2(size.left, e->size.h));
  move_resize_element(top, e->pos, vec2(e->size.w, size.top));
  move_resize_element(right, vec2((e->pos.x + e->size.w - size.right), e->pos.y), vec2(size.right, e->size.h));
  move_resize_element(bottom, vec2(e->pos.x, (e->pos.y + e->size.h - size.bottom)), vec2(e->size.w, size.bottom));
  /* And then set the relative positions. */
  left->relative_pos   = 0;
  top->relative_pos    = 0;
  right->relative_pos  = vec2(size.right, right->size.h);
  bottom->relative_pos = vec2(bottom->size.w, size.bottom);
  e->flag.set<GUIELEMENT_HAS_BORDERS>();
}

/* Draw a ui-element`s rect. */
void draw_element_rect(guielement *element) {
  draw_rect(element->pos, element->size, element->color);
  /* If element has borders, draw them to. */
  if (element->flag.is_set<GUIELEMENT_HAS_BORDERS>()) {
    for (Ulong i = 0; i < element->children.size(); ++i) {
      guielement *child = element->children[i];
      if (child->flag.is_set<GUIELEMENT_IS_BORDER>()) {
        draw_rect(child->pos, child->size, child->color);
      }
    }
  }
}

