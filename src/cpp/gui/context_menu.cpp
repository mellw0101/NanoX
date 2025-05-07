/** @file gui/context_menu.c

  @author  Melwin Svensson.
  @date    7-5-2025.

 */
#include "../../include/prototypes.h"


struct ContextMenu {
  Menu *menu;
  Menu *edit_menu;
};


static void context_menu_pos_routine(void *arg, vec2 size, vec2 *pos) {
  ASSERT(arg);
  ASSERT(pos);
  /* Calculate the correct position for the suggestmenu window. */
  pos->x = mousepos.x;
  gui_font_row_top_bot(gui->font, 0, NULL, &pos->y);
  pos->y += mousepos.y;
}

static void context_menu_accept_routine(void *arg, const char *const restrict entry_string, int index) {
  ASSERT(arg);
  ASSERT(entry_string);
  writef("%s\n", entry_string);
}

ContextMenu *context_menu_create(void) {
  ContextMenu *cxm;
  MALLOC_STRUCT(cxm);
  cxm->menu = gui_menu_create(gui->root, gui->font, gui, context_menu_pos_routine, context_menu_accept_routine);
  gui_menu_push_back(cxm->menu, "Test");
  gui_menu_push_back(cxm->menu, "Balle");
  gui_menu_push_back(cxm->menu, "Edit");
  gui_menu_set_tab_accept_behavior(cxm->menu, TRUE);
  gui_menu_set_static_width(cxm->menu, 100.0f);
  return cxm;
}

void context_menu_free(ContextMenu *const cxm) {
  ASSERT(cxm);
  gui_menu_free(cxm->menu);
  free(cxm);
}

void context_menu_draw(ContextMenu *const cxm) {
  ASSERT(cxm);
  gui_menu_draw(cxm->menu);
}

void context_menu_show(ContextMenu *const cxm, bool show) {
  ASSERT(cxm);
  gui_menu_show(cxm->menu, show);
}
