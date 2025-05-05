/** @file gui/menu.cpp

  @author  Melwin Svensson.
  @date    5-5-2025.

 */
#include "../../include/prototypes.h"



/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_GM       \
  ASSERT(gm);           \
  ASSERT(gm->sb);       \
  ASSERT(gm->element);  \
  ASSERT(gm->vertbuf);  \
  ASSERT(gm->entries);  \
  ASSERT(gm->get_font); \
  ASSERT(gm->get_pos)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct GuiMenu {
  /* State flags for this menu. */
  bool text_refresh_needed : 1;
  bool pos_refresh_needed  : 1;

  /* This ptr can be anything needed, it will be passed to all user assignalbe callbacks. */
  void *data;

  /* The scrollbar for this gui menu, will only be used when there are more entries then maxrows. */
  GuiScrollbar *sb;

  /* The root element of this gui menu, this will be a child of another element. */
  guielement *element;

  /* The reason every menu should have its own vertex buffer is to reduce calls to construct the glyph
   * vertex'es, this also makes it so that static menu's only need to update the buffer when moved. */
  vertex_buffer_t *vertbuf;

  /* The entries in this menu. */
  CVec *entries;

  /* The index where the viewable part start.  Note that this only changes from 0 when there are more entries then maxrows. */
  int viewtop;
  int selected;
  int maxrows;
  int rows;

  /* For now this will be our font getter function ptr, this will be made into the updater function ptr to get all needed data. */
  GuiMenuGetFontFunc get_font;

  GuiMenuGetPosFunc get_pos;
};


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


static void gui_menu_scrollbar_update(void *arg, float *length, Uint *start, Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset) {
  ASSERT(arg);
  GuiMenu *gm;
  CAST(gm, arg);
  ASSIGN_IF_VALID(length, gm->element->size.h);
  ASSIGN_IF_VALID(start, 0);
  ASSIGN_IF_VALID(total, (cvec_len(gm->entries) - gm->rows));
  ASSIGN_IF_VALID(visible, gm->rows);
  ASSIGN_IF_VALID(current, gm->viewtop);
  ASSIGN_IF_VALID(top_offset, 0);
  ASSIGN_IF_VALID(right_offset, 0);
}

static void gui_menu_scrollbar_moving(void *arg, long index) {
  ASSERT(arg);
  GuiMenu *gm;
  CAST(gm, arg);
  gm->viewtop = fclamp(index, 0, (cvec_len(gm->entries) - gm->rows));
  gm->text_refresh_needed = TRUE;
}

static void gui_menu_scrollbar_create(GuiMenu *const gm) {
  ASSERT(gm);
  ASSERT(gm->element);
  gm->sb = guiscrollbar_create(gm->element, gm, gui_menu_scrollbar_update, gui_menu_scrollbar_moving);
  /* Resize the parent of the scrollbar (the main element of the menu) to the height of the given font times (maxrows + 1), this
   * is needed because we need to make sure that when this call is made it adjusts the scrollbar element as well otherwise
   * it will not be initilazed on first use, and this does not matter anyway as this menu has the `pos_resize_needed` flag set. */
  guielement_resize(gm->element, vec2(gm->element->size.w, (gui_font_height(gm->get_font(gm->data)) * (gm->maxrows + 1))));
  /* Now when we call the draw function for the scrollbar it will recalculate the
   * position directly, otherwise it would just hide the scrollbar as its not needed. */
  guiscrollbar_draw(gm->sb);
}


static void gui_menu_resize(GuiMenu *const gm) {
  ASSERT_GM;
  vec2 pos, size;
  int len = cvec_len(gm->entries);
  if (len && gm->pos_refresh_needed) {
    /* There are more entries then visible rows. */
    if (len > gm->maxrows) {
      gm->rows = gm->maxrows;
    }
    /* Otherwise, when all entries fit on screen. */
    else {
      gm->rows = len;
    }
    /* Calculate the given size based on the number of entries. */
    size.w = (pixbreadth(gm->get_font(gm->data), (char *)cvec_get(gm->entries, (len - 1))));
    size.h = (gm->rows * gui_font_height(gm->get_font(gm->data)));
    if (len > gm->maxrows) {
      size.w += guiscrollbar_width(gm->sb);
    }
    /* Then get the position using the provided function. */
    gm->get_pos(gm->data, &pos);
    guielement_move_resize(gm->element, pos, size);
    gm->pos_refresh_needed = FALSE;
  }
}

static void gui_menu_draw_selected(GuiMenu *const gm) {
  ASSERT_GM;
  int selected;
  vec2 pos, size;
  /* Only draw the selected row when there are more then 0 entries in the menu. */
  if (cvec_len(gm->entries)) {
    selected = (gm->selected - gm->viewtop);
    /* And when the selected row is inside the viewport of the menu. */
    if (selected >= 0 && selected <= gm->rows) {
      pos.x  = gm->element->pos.x;
      size.w = gm->element->size.w;
      gui_font_row_top_bot(gm->get_font(gm->data), (selected + 1), &pos.y, &size.h);
      size.h -= pos.y;
      pos.y  += gm->element->pos.y;
      draw_rect(pos, size, vec4(vec3(1.0f), 0.4f));
    }
  }
}

static void gui_menu_draw_text(GuiMenu *const gm) {
  ASSERT_GM;
  /* Used to save the last viewtop. */
  static int was_viewtop = gm->viewtop;
  /* The current row. */
  int row = 0;
  /* Ptr to the string on the given row. */
  char *text;
  vec2 textpen;
  if (cvec_len(gm->entries)){
    if ((gm->text_refresh_needed || (was_viewtop != gm->viewtop))) {
      vertex_buffer_clear(gm->vertbuf);
      while (row < gm->rows) {
        text = (char *)cvec_get(gm->entries, (gm->viewtop + row));
        textpen = vec2(
          (gm->element->pos.x + pixbreadth(gm->get_font(gm->data), " ")),
          (gui_font_row_baseline(gm->get_font(gm->data), (row + 1)) + gm->element->pos.y)
        );
        vertex_buffer_add_string(gm->vertbuf, text, strlen(text), NULL, gui_font_get_font(gm->get_font(gm->data)), 1, &textpen);
        ++row;
      }
      was_viewtop = gm->viewtop;
      gm->text_refresh_needed = FALSE;
    }
    upload_texture_atlas(gui_font_get_atlas(gm->get_font(gm->data)));
    render_vertex_buffer(gui->font_shader, gm->vertbuf);
  }
}


GuiMenu *gui_menu_create(guielement *const parent, void *const data, GuiMenuGetFontFunc get_font, GuiMenuGetPosFunc get_pos) {
  ASSERT(parent);
  ASSERT(data);
  ASSERT(get_font);
  ASSERT(get_pos);
  GuiMenu *gm;
  MALLOC_STRUCT(gm);
  gm->text_refresh_needed = TRUE;
  gm->pos_refresh_needed  = TRUE;
  gm->data = data;
  gm->vertbuf = make_new_font_buffer();
  gm->element = guielement_create(parent);
  gm->element->color = VEC4_VS_CODE_MAGENTA;
  gm->element->flag += {GUIELEMENT_ABOVE, GUIELEMENT_HIDDEN};
  gm->entries  = cvec_create_setfree(free);
  gm->viewtop  = 0;
  gm->selected = 0;
  gm->maxrows  = 8;  /* For now we default to a max of 8 rows. */
  gui_menu_scrollbar_create(gm);
  gm->get_font = get_font;
  gm->get_pos  = get_pos;
  return gm;
}

void gui_menu_free(GuiMenu *const gm) {
  ASSERT_GM;
  free(gm->sb);
  guielement_free(gm->element);
  vertex_buffer_delete(gm->vertbuf);
  cvec_free(gm->entries);
  free(gm);
}

void gui_menu_draw(GuiMenu *const gm) {
  ASSERT_GM;
  /* Only do anything when the menu is not hidden and has more then 0 entries. */
  if (!gm->element->flag.is_set<GUIELEMENT_HIDDEN>() && cvec_len(gm->entries)) {
    gui_menu_resize(gm);
    guielement_draw(gm->element);
    gui_menu_draw_selected(gm);
    guiscrollbar_draw(gm->sb);
    gui_menu_draw_text(gm);
  }
}

/* Clear all entries from `gm`. */
void gui_menu_clear(GuiMenu *const gm) {
  ASSERT_GM;
  cvec_clear(gm->entries);
  gm->viewtop  = 0;
  gm->selected = 0;
}

void gui_menu_text_refresh_needed(GuiMenu *const gm) {
  ASSERT_GM;
  gm->text_refresh_needed = TRUE;
}

void gui_menu_pos_refresh_needed(GuiMenu *const gm) {
  ASSERT_GM;
  gm->pos_refresh_needed = TRUE;
}

void gui_menu_scroll_refresh_needed(GuiMenu *const gm) {
  ASSERT_GM;
  guiscrollbar_refresh_needed(gm->sb);
}

void gui_menu_full_refresh_needed(GuiMenu *const gm) {
  ASSERT_GM;
  gm->text_refresh_needed = TRUE;
  gm->pos_refresh_needed = TRUE;
  guiscrollbar_refresh_needed(gm->sb);
}

void gui_menu_hide(GuiMenu *const gm, bool hide) {
  ASSERT_GM;
  guielement_set_flag_recurse(gm->element, hide, GUIELEMENT_HIDDEN);
  if (!hide) {
  }
}

void gui_menu_push_back(GuiMenu *const gm, char *const ptr) {
  ASSERT_GM;
  cvec_push(gm->entries, ptr);
  gui_menu_full_refresh_needed(gm);
}

void gui_menu_qsort(GuiMenu *const gm, CmpFuncPtr cmp_func) {
  ASSERT_GM;
  cvec_qsort(gm->entries, cmp_func);
}

/* Return's the current number of entries in `gm`. */
int gui_menu_len(GuiMenu *const gm) {
  ASSERT_GM;
  return cvec_len(gm->entries);
}
