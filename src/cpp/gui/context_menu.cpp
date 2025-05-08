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
  gui_menu_push_back(cxm->menu, "Root menu test: 0");
  gui_menu_push_back(cxm->menu, "Root menu test: 1");
  gui_menu_push_back(cxm->menu, "Root menu test: 2");
  gui_menu_push_back(cxm->menu, "Root menu test: 3");
  gui_menu_push_back(cxm->menu, "Root menu test: 4");
  gui_menu_push_back(cxm->menu, "Root menu test: 5");
  gui_menu_push_back(cxm->menu, "Root menu test: 6");
  gui_menu_push_back(cxm->menu, "Root menu test: 7");
  gui_menu_push_back(cxm->menu, "Root menu test: 8");
  gui_menu_push_back(cxm->menu, "Root menu test: 9");
  gui_menu_push_back(cxm->menu, "Root menu test: 10");
  gui_menu_push_back(cxm->menu, "Root menu test: 11");
  gui_menu_push_back(cxm->menu, "Root menu test: 12");
  gui_menu_push_back(cxm->menu, "Root menu test: 13");
  gui_menu_push_back(cxm->menu, "Root menu test: 14");
  gui_menu_push_back(cxm->menu, "Root menu test: 15");
  gui_menu_push_back(cxm->menu, "Root menu test: 16");
  gui_menu_push_back(cxm->menu, "Root menu test: 17");
  cxm->edit_menu = gui_menu_create_submenu(cxm->menu, "Edit", gui, context_menu_accept_routine);
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 0");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 1");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 2");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 3");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 4");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 5");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 6");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 7");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 8");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 9");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 10");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 11");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 12");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 13");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 14");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 15");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 16");
  gui_menu_push_back(cxm->edit_menu, "Sub menu test: 17");
  gui_menu_set_tab_accept_behavior(cxm->menu, TRUE);
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
