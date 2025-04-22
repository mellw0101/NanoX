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
guielement *guielement_create(vec2 pos, vec2 size, vec2 endoff, vec4 color, bool in_gridmap) _NOTHROW {
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
  element->cursor_type = GLFW_ARROW_CURSOR;
  return element;
}

/* Create a blank element. */
guielement *guielement_create(bool in_gridmap) _NOTHROW {
  return guielement_create(-100.0f, 20.0f, 0.0f, 0.0f, in_gridmap);
}

/* Create a new child to element `parent`. */
guielement *guielement_create(guielement *const parent, bool in_gridmap) _NOTHROW {
  ASSERT(parent);
  guielement *element = guielement_create(in_gridmap);
  element->parent = parent;
  parent->children.push_back(element);
  return element;
}

/* Delete a element and all its children. */
void guielement_free(guielement *const element) {
  /* Assert that the passed element is valid. */
  ASSERT(element);
  /* First start by running this function recursivly on all children of element. */
  for (Ulong i = 0; i < element->children.size(); ++i) {
    guielement_free(element->children[i]);
  }
  /* Then remove it from the global gridmap and freeing the lable and the element itself. */
  gridmap.remove(element);
  free(element->lable);
  free(element);
}

/* Set an elements lable text. */
void guielement_set_lable(guielement *const element, const char *string) _NOTHROW {
  ASSERT(element);
  if (element->flag.is_set<GUIELEMENT_HAS_LABLE>()) {
    free(element->lable);
  }
  element->lablelen = strlen(string);
  element->lable    = measured_copy(string, element->lablelen);
  element->flag.set<GUIELEMENT_HAS_LABLE>();
}

/* Delete all children of `element`. */
void guielement_delete_children(guielement *const element) {
  ASSERT(element);
  for (Ulong i=0; i<element->children.size(); ++i) {
    guielement_free(element->children[i]);
    element->children[i] = NULL;
  }
  element->children.clear();
}

/* Get ui element from current mouse position. */
guielement *guielement_from_mousepos(void) {
  ivec2 pos(mousepos);
  if (gridmap.contains(pos)) {
    return gridmap.get(pos);
  }
  return NULL;
}

/* Ensure all children that has relative position or size gets updated. */
static void guielement_check_children_relative_pos_size(guielement *const e) {
  /* For the most efficient loop, get the size of the vector, and the internal ptr of the data in the vector. */
  guielement *child;
  /* Then check all children. */
  for (Ulong i = 0; i < e->children.size(); ++i) {
    child = e->children[i];
    /* If this child uses relative based positioning the adjust the child position based on the parent. */
    if (child->flag.is_set<GUIELEMENT_RELATIVE_POS>()) {
      guielement_move(child, (e->pos + child->relative_pos));
    }
    /* Else, when the relative position is reversed as in the zero point is the right-down corner as apposed to normal (left-top side). */
    else if (child->flag.is_set<GUIELEMENT_REVERSE_RELATIVE_POS>()) {
      guielement_move(child, ((e->pos + e->size) - child->relative_pos));
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
      guielement_move(child, newpos);
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
      guielement_resize(child, newsize);
    }
  }
}

/* Resize an element as well as correctly adjust it in the gridmap. */
void guielement_resize(guielement *const e, vec2 size) {
  ASSERT(e);
  gridmap.remove(e);
  e->size = size;
  guielement_check_children_relative_pos_size(e);
  gridmap.set(e);
}

/* Move an element as well as correctly adjust it in the gridmap. */
void guielement_move(guielement *const e, vec2 pos) {
  ASSERT(e);
  gridmap.remove(e);
  e->pos = pos;
  guielement_check_children_relative_pos_size(e);
  gridmap.set(e);
}

/* Move an `guielement's` y position to `ypos` while clamping it to be constrained to `min` and `max`. */
void guielement_move_y_clamp(guielement *const e, float ypos, float min, float max) {
  ASSERT(e);
  gridmap.remove(e);
  e->pos.y = fclamp(ypos, min, max);
  guielement_check_children_relative_pos_size(e);
  gridmap.set(e);
}

/* Move and resize an element and correctly set it in the gridmap. */
void guielement_move_resize(guielement *const e, vec2 pos, vec2 size) {
  ASSERT(e);
  gridmap.remove(e);
  e->pos  = pos;
  e->size = size;
  guielement_check_children_relative_pos_size(e);
  gridmap.set(e);
}

/* Delete the border elements of element `e`. */
void guielement_delete_borders(guielement *const e) {
  ASSERT(e);
  for (Ulong i = 0; i < e->children.size(); ++i) {
    if (e->children[i]->flag.is_set<GUIELEMENT_IS_BORDER>()) {
      guielement_free(e->children[i]);
      e->children.erase_at(i);
      --i;
    }
  }
  e->flag.unset<GUIELEMENT_HAS_BORDERS>();
}

/* Create borders for element `e` of `size` with `color`. */
void guielement_set_borders(guielement *const e, vec4 size, vec4 color) {
  ASSERT(e);
  guielement *left, *top, *right, *bottom;
  /* First remove the current border, if it has them. */
  guielement_delete_borders(e);
  /* Create all sides as children of e. */
  left   = guielement_create(e, FALSE);
  top    = guielement_create(e, FALSE);
  right  = guielement_create(e, FALSE);
  bottom = guielement_create(e, FALSE);
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
  guielement_move_resize(left, e->pos, vec2(size.left, e->size.h));
  guielement_move_resize(top, e->pos, vec2(e->size.w, size.top));
  guielement_move_resize(right, vec2((e->pos.x + e->size.w - size.right), e->pos.y), vec2(size.right, e->size.h));
  guielement_move_resize(bottom, vec2(e->pos.x, (e->pos.y + e->size.h - size.bottom)), vec2(e->size.w, size.bottom));
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
void guielement_draw(guielement *const e) {
  ASSERT(e);
  guielement *child;
  /* If the element is hidden, dont draw it. */
  if (e->flag.is_set<GUIELEMENT_HIDDEN>()) {
    return;
  }
  /* Otherwise, we draw it. */
  else {
    draw_rect(e->pos, e->size, e->color);
    /* If element has borders, draw them to. */
    if (e->flag.is_set<GUIELEMENT_HAS_BORDERS>()) {
      for (Ulong i = 0; i < e->children.size(); ++i) {
        child = e->children[i];
        if (child->flag.is_set<GUIELEMENT_IS_BORDER>()) {
          draw_rect(child->pos, child->size, child->color);
        }
      }
    }
  }
}

/* Set the union data ptr for `e` as a `void *`.  This should be used as apposed to directly setting the data ptr. */
void guielement_set_raw_data(guielement *const e, void *const data) _NOTHROW {
  ASSERT(e);
  ASSERT(data);
  e->flag.set<GUIELEMENT_HAS_RAW_DATA>();
  e->flag.unset<GUIELEMENT_HAS_FILE_DATA>();
  e->flag.unset<GUIELEMENT_HAS_EDITOR_DATA>();
  e->flag.unset<GUIELEMENT_HAS_SB_DATA>();
  e->data.raw = data;
}

/* Set the union data ptr for `e` as a `openfilestruct *`.  This should be used as apposed to directly setting the data ptr. */
void guielement_set_file_data(guielement *const e, openfilestruct *const file) _NOTHROW {
  ASSERT(e);
  ASSERT(file);
  e->flag.unset<GUIELEMENT_HAS_RAW_DATA>();
  e->flag.set<GUIELEMENT_HAS_FILE_DATA>();
  e->flag.unset<GUIELEMENT_HAS_EDITOR_DATA>();
  e->flag.unset<GUIELEMENT_HAS_SB_DATA>();
  e->data.file = file;
}

/* Set the union data ptr for `e` as a `guieditor *`.  This should be used as apposed to directly setting the data ptr. */
void guielement_set_editor_data(guielement *const e, guieditor *const editor) _NOTHROW {
  ASSERT(e);
  ASSERT(editor);
  e->flag.unset<GUIELEMENT_HAS_RAW_DATA>();
  e->flag.unset<GUIELEMENT_HAS_FILE_DATA>();
  e->flag.set<GUIELEMENT_HAS_EDITOR_DATA>();
  e->flag.unset<GUIELEMENT_HAS_SB_DATA>();
  e->data.editor = editor;
}

/* Set the union data ptr for `e` as a `GuiScrollbar *`.  This should be used as apposed to directly setting the data ptr. */
void guielement_set_sb_data(guielement *const e, GuiScrollbar *const sb) _NOTHROW {
  ASSERT(e);
  ASSERT(sb);
  e->flag.unset<GUIELEMENT_HAS_EDITOR_DATA>();
  e->flag.unset<GUIELEMENT_HAS_RAW_DATA>();
  e->flag.unset<GUIELEMENT_HAS_FILE_DATA>();
  e->flag.set<GUIELEMENT_HAS_SB_DATA>();
  e->data.sb = sb;
}

/* Return's `TRUE` when `e` has `void *` data. */
bool guielement_has_raw_data(guielement *const e) _NOTHROW {
  return (e && e->flag.is_set<GUIELEMENT_HAS_RAW_DATA>());
}

/* Return's `TRUE` when `e` has `openfilestruct *` data. */
bool guielement_has_file_data(guielement *const e) _NOTHROW {
  return (e && e->flag.is_set<GUIELEMENT_HAS_FILE_DATA>());
}

/* Return's `TRUE` when `e` has `guieditor *` data. */
bool guielement_has_editor_data(guielement *const e) _NOTHROW {
  return (e && e->flag.is_set<GUIELEMENT_HAS_EDITOR_DATA>());
}

/* Return's `TRUE` when `e` has `GuiScrollbar *` data. */
bool guielement_has_sb_data(guielement *const e) _NOTHROW {
  return (e && e->flag.is_set<GUIELEMENT_HAS_SB_DATA>());
}

/* If `set` is `TRUE` set `flag` for all children of `element` recurivly, otherwise,
 * when `set` is `FALSE` unset `flag` for all children of `element` recurivly. */
void guielement_set_flag_recurse(guielement *const element, bool set, Uint flag) {
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
  for (Ulong i=0; i<size; ++i) {
    guielement_set_flag_recurse(ptr[i], set, flag);
  }
}
