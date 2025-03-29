/** @file guielement.cpp

  @author  Melwin Svensson.
  @date    28-1-2025.

 */
#include "../../include/prototypes.h"

/* Dont use yet. */
_UNUSED static guielement *make_new_element(void) {
  guielement *element   = (guielement *)xmalloc(sizeof(*element));
  element->pos          = -100;
  element->relative_pos = 0;
  element->endoff       = 0;
  element->size         = 20;
  element->data.raw     = NULL;
  element->color        = 0;
  element->textcolor    = 0;
  element->lable        = NULL;
  element->lablelen     = 0;
  element->flag         = bit_flag_t<GUIELEMENT_FLAG_SIZE>();
  element->parent       = NULL;
  element->children     = MVector<guielement *>();
  element->callback     = NULL;
  return element;
}

/* Create a ui element. */
guielement *make_element(vec2 pos, vec2 size, vec2 endoff, vec4 color, bool in_gridmap) _NOTHROW {
  guielement *element    = (guielement *)nmalloc(sizeof(*element));
  element->pos           = pos;
  element->relative_pos  = 0;
  element->size          = size;
  element->endoff        = endoff;
  element->relative_size = 0;
  element->data.raw      = NULL;
  element->color         = color;
  element->textcolor     = 0.0f;
  element->lable         = NULL;
  element->lablelen      = 0;
  element->callback      = NULL;
  element->parent        = NULL;
  element->children      = MVector<guielement*>{};
  element->flag.clear();
  if (!in_gridmap) {
    element->flag.set<GUIELEMENT_NOT_IN_GRIDMAP>();
  }
  gridmap.set(element);
  return element;
}

/* Create a blank element. */
guielement *make_element(bool in_gridmap) _NOTHROW {
  return make_element(-100.0f, 20.0f, 0.0f, 0.0f, in_gridmap);
}

/* Create a new child to element `parent`. */
guielement *make_element_child(guielement *parent, bool in_gridmap) {
  ASSERT(parent);
  guielement *element = make_element(in_gridmap);
  element->parent = parent;
  parent->children.push_back(element);
  return element;
}

/* Delete a element and all its children. */
void delete_element(guielement *element) {
  /* Assert that the passed element is valid. */
  ASSERT(element);
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
  ASSERT(element);
  if (element->flag.is_set<GUIELEMENT_HAS_LABLE>()) {
    free(element->lable);
  }
  element->lablelen = strlen(string);
  element->lable    = measured_copy(string, element->lablelen);
  element->flag.set<GUIELEMENT_HAS_LABLE>();
}

/* Delete all children of `element`. */
void delete_guielement_children(guielement *element) {
  ASSERT(element);
  for (Ulong i = 0; i < element->children.size(); ++i) {
    delete_element(element->children[i]);
    element->children[i] = NULL;
  }
  element->children.clear();
}

/* Get ui element from current mouse position. */
guielement *element_from_mousepos(void) {
  ivec2 pos(mousepos);
  if (gridmap.contains(pos)) {
    return gridmap.get(pos);
  }
  return NULL;
}

/* Ensure all children that has relative position or size gets updated. */
static void check_element_children_relative_pos_size(guielement *e) {
  /* For the most efficient loop, get the size of the vector, and the internal ptr of the data in the vector. */
  guielement *child;
  /* Then check all children. */
  for (Ulong i = 0; i < e->children.size(); ++i) {
    child = e->children[i];
    /* If this child uses relative based positioning the adjust the child position based on the parent. */
    if (child->flag.is_set<GUIELEMENT_RELATIVE_POS>()) {
      move_element(child, (e->pos + child->relative_pos));
    }
    /* Else, when the relative position is reversed as in the zero point is the right-down corner as apposed to normal (left-top side). */
    else if (child->flag.is_set<GUIELEMENT_REVERSE_RELATIVE_POS>()) {
      move_element(child, ((e->pos + e->size) - child->relative_pos));
    }
    /* Otherwise, check if relative x or y pos is set. */
    else if (child->flag.is_set<GUIELEMENT_REVERSE_RELATIVE_X_POS>() || child->flag.is_set<GUIELEMENT_REVERSE_RELATIVE_Y_POS>()
     || child->flag.is_set<GUIELEMENT_RELATIVE_X_POS>() || child->flag.is_set<GUIELEMENT_RELATIVE_Y_POS>()) {
      vec2 newpos = child->pos;
      if (child->flag.is_set<GUIELEMENT_RELATIVE_X_POS>()) {
        newpos.x = (e->pos.x + child->relative_pos.x);
      }
      if (child->flag.is_set<GUIELEMENT_RELATIVE_Y_POS>()) {
        newpos.y = (e->pos.y + child->relative_pos.y);
      } 
      if (child->flag.is_set<GUIELEMENT_REVERSE_RELATIVE_X_POS>()) {
        newpos.x = ((e->pos.x + e->size.x) - child->relative_pos.x);
      }
      if (child->flag.is_set<GUIELEMENT_REVERSE_RELATIVE_Y_POS>()) {
        newpos.y = ((e->pos.y + e->size.y) - child->relative_pos.y);
      }
      move_element(child, newpos);
    }
    /* If either relative width or height is active for this child then resize the element. */
    if (child->flag.is_set<GUIELEMENT_RELATIVE_WIDTH>() || child->flag.is_set<GUIELEMENT_RELATIVE_HEIGHT>()) {
      vec2 newsize = child->size;
      /* If width based relative size is turned on. */
      if (child->flag.is_set<GUIELEMENT_RELATIVE_WIDTH>()) {
        newsize.w = (e->size.w - child->relative_pos.x - child->relative_size.w);
      }
      /* If height based relative size is turned on. */
      if (child->flag.is_set<GUIELEMENT_RELATIVE_HEIGHT>()) {
        newsize.h = (e->size.h - child->relative_pos.y - child->relative_size.h);
      }
      resize_element(child, newsize);
    }
  }
}

/* Resize an element as well as correctly adjust it in the gridmap. */
void resize_element(guielement *e, vec2 size) {
  ASSERT(e);
  gridmap.remove(e);
  e->size = size;
  check_element_children_relative_pos_size(e);
  gridmap.set(e);
}

/* Move an element as well as correctly adjust it in the gridmap. */
void move_element(guielement *e, vec2 pos) {
  ASSERT(e);
  gridmap.remove(e);
  e->pos = pos;
  check_element_children_relative_pos_size(e);
  gridmap.set(e);
}

/* Move and resize an element and correctly set it in the gridmap. */
void move_resize_element(guielement *e, vec2 pos, vec2 size) {
  ASSERT(e);
  gridmap.remove(e);
  e->pos  = pos;
  e->size = size;
  check_element_children_relative_pos_size(e);
  gridmap.set(e);
}

/* Delete the border elements of element `e`. */
void delete_element_borders(guielement *e) {
  ASSERT(e);
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
  ASSERT(e);
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
  left->relative_pos = 0;
  top->relative_pos  = 0;
  /* And reverse relative positioning for the bottom and right border. */
  right->flag.set<GUIELEMENT_REVERSE_RELATIVE_X_POS>();
  right->flag.set<GUIELEMENT_RELATIVE_Y_POS>();
  right->relative_pos = vec2(size.right, 0);
  bottom->flag.set<GUIELEMENT_RELATIVE_X_POS>();
  bottom->flag.set<GUIELEMENT_REVERSE_RELATIVE_Y_POS>();
  bottom->relative_pos = vec2(0, size.bottom);
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
  /* Set relative height for left and right borders. */
  left->flag.set<GUIELEMENT_RELATIVE_HEIGHT>();
  right->flag.set<GUIELEMENT_RELATIVE_HEIGHT>();
  left->relative_size  = 0;
  right->relative_size = 0;
  /* Set relative width for top and bottom borders. */
  top->flag.set<GUIELEMENT_RELATIVE_WIDTH>();
  bottom->flag.set<GUIELEMENT_RELATIVE_WIDTH>();
  top->relative_size    = 0;
  bottom->relative_size = 0;
  e->flag.set<GUIELEMENT_HAS_BORDERS>();
}

/* Draw a ui-element`s rect. */
void draw_element_rect(guielement *element) {
  ASSERT(element);
  /* If the element is hidden, dont draw it. */
  if (element->flag.is_set<GUIELEMENT_HIDDEN>()) {
    return;
  }
  /* Otherwise, we draw it. */
  else {
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
}

/* Set the raw data of an element.  This should be used as apposed to directly setting the raw ptr. */
void set_element_raw_data(guielement *element, void *data) _NOTHROW {
  ASSERT(element);
  ASSERT(data);
  element->flag.set<GUIELEMENT_HAS_RAW_DATA>();
  element->flag.unset<GUIELEMENT_HAS_FILE_DATA>();
  element->flag.unset<GUIELEMENT_HAS_EDITOR_DATA>();
  element->data.raw = data;
}

/* Set the file data for `element`.  This should be used as apposed to directly setting the file ptr. */
void set_element_file_data(guielement *element, openfilestruct *file) _NOTHROW {
  ASSERT(element);
  ASSERT(file);
  element->flag.set<GUIELEMENT_HAS_FILE_DATA>();
  element->flag.unset<GUIELEMENT_HAS_RAW_DATA>();
  element->flag.unset<GUIELEMENT_HAS_EDITOR_DATA>();
  element->data.file = file;
}

/* Set the editor data for `element`.  This should be used as apposed to directly setting the editor ptr. */
void set_element_editor_data(guielement *element, guieditor *editor) _NOTHROW {
  ASSERT(element);
  ASSERT(editor);
  element->flag.set<GUIELEMENT_HAS_EDITOR_DATA>();
  element->flag.unset<GUIELEMENT_HAS_RAW_DATA>();
  element->flag.unset<GUIELEMENT_HAS_FILE_DATA>();
  element->data.editor = editor;
}

/* Returns `TRUE` when `element` has editor data. */
bool element_has_file_data(guielement *element) _NOTHROW {
  return (element && element->flag.is_set<GUIELEMENT_HAS_FILE_DATA>());
}

/* Returns `TRUE` when `element` has editor data. */
bool element_has_editor_data(guielement *element) _NOTHROW {
  return (element && element->flag.is_set<GUIELEMENT_HAS_EDITOR_DATA>());
}

/* If `set` is `TRUE` set `flag` for all children of `element` recurivly, otherwise,
 * when `set` is `FALSE` unset `flag` for all children of `element` recurivly. */
void set_element_flag_recurse(guielement *element, bool set, Uint flag) {
  ASSERT(element);
  /* Always assert that the passed flag is valid in the context of a elemnent flag. */
  ALWAYS_ASSERT(flag >= 0 && flag <= GUIELEMENT_FLAG_SIZE);
  const Ulong size = element->children.size();
  guielement **const &ptr = element->children.data();
  if (set) {
    element->flag.set(flag);
  }
  else {
    element->flag.unset(flag);
  }
  for (Ulong i = 0; i < size; ++i) {
    set_element_flag_recurse(ptr[i], set, flag);
  }
}
