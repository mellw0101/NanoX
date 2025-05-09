/** @file gui/context_menu.c

  @author  Melwin Svensson.
  @date    7-5-2025.

 */
#include "../../include/prototypes.h"


struct ContextMenu {
  Menu *menu;
  Menu *sm0;
  Menu *sm1;
  Menu *sm2;
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
  cxm->sm0 = gui_menu_create_submenu(cxm->menu, "Edit", gui, context_menu_accept_routine);
  cxm->sm1 = gui_menu_create_submenu(cxm->sm0, "Edit sub menu", gui, context_menu_accept_routine);
  gui_menu_push_back(cxm->sm1, "Sm1: 0");
  gui_menu_push_back(cxm->sm1, "Sm1: 1");
  gui_menu_push_back(cxm->sm1, "Sm1: 2");
  gui_menu_push_back(cxm->sm1, "Sm1: 3");
  gui_menu_push_back(cxm->sm1, "Sm1: 4");
  gui_menu_push_back(cxm->sm1, "Sm1: 5");
  gui_menu_push_back(cxm->sm1, "Sm1: 6");
  gui_menu_push_back(cxm->sm1, "Sm1: 7");
  gui_menu_push_back(cxm->sm1, "Sm1: 8");
  gui_menu_push_back(cxm->sm1, "Sm1: 9");
  gui_menu_push_back(cxm->sm1, "Sm1: 10");
  gui_menu_push_back(cxm->sm1, "Sm1: 11");
  gui_menu_push_back(cxm->sm1, "Sm1: 12");
  gui_menu_push_back(cxm->sm1, "Sm1: 13");
  gui_menu_push_back(cxm->sm1, "Sm1: 14");
  gui_menu_push_back(cxm->sm1, "Sm1: 15");
  gui_menu_push_back(cxm->sm1, "Sm1: 16");
  gui_menu_push_back(cxm->sm1, "Sm1: 17");
  cxm->sm2 = gui_menu_create_submenu(cxm->sm1, "sm2", gui, context_menu_accept_routine);
  gui_menu_push_back(cxm->sm2, "sm2: 0");
  gui_menu_push_back(cxm->sm2, "sm2: 1");
  gui_menu_push_back(cxm->sm2, "sm2: 2");
  gui_menu_push_back(cxm->sm2, "sm2: 3");
  gui_menu_push_back(cxm->sm2, "sm2: 4");
  gui_menu_push_back(cxm->sm2, "sm2: 5");
  gui_menu_push_back(cxm->sm2, "sm2: 6");
  gui_menu_push_back(cxm->sm2, "sm2: 7");
  gui_menu_push_back(cxm->sm2, "sm2: 8");
  gui_menu_push_back(cxm->sm2, "sm2: 9");
  gui_menu_push_back(cxm->sm2, "sm2: 10");
  gui_menu_push_back(cxm->sm2, "sm2: 11");
  gui_menu_push_back(cxm->sm2, "sm2: 12");
  gui_menu_push_back(cxm->sm2, "sm2: 13");
  gui_menu_push_back(cxm->sm2, "sm2: 14");
  gui_menu_push_back(cxm->sm2, "sm2: 15");
  gui_menu_push_back(cxm->sm2, "sm2: 16");
  gui_menu_push_back(cxm->sm2, "sm2: 17");
  gui_menu_push_back(cxm->sm0, "Sm0: 0");
  gui_menu_push_back(cxm->sm0, "Sm0: 1");
  gui_menu_push_back(cxm->sm0, "Sm0: 2");
  gui_menu_push_back(cxm->sm0, "Sm0: 3");
  gui_menu_push_back(cxm->sm0, "Sm0: 4");
  gui_menu_push_back(cxm->sm0, "Sm0: 5");
  gui_menu_push_back(cxm->sm0, "Sm0: 6");
  gui_menu_push_back(cxm->sm0, "Sm0: 7");
  gui_menu_push_back(cxm->sm0, "Sm0: 8");
  gui_menu_push_back(cxm->sm0, "Sm0: 9");
  gui_menu_push_back(cxm->sm0, "Sm0: 10");
  gui_menu_push_back(cxm->sm0, "Sm0: 11");
  gui_menu_push_back(cxm->sm0, "Sm0: 12");
  gui_menu_push_back(cxm->sm0, "Sm0: 13");
  gui_menu_push_back(cxm->sm0, "Sm0: 14");
  gui_menu_push_back(cxm->sm0, "Sm0: 15");
  gui_menu_push_back(cxm->sm0, "Sm0: 16");
  gui_menu_push_back(cxm->sm0, "Sm0: 17");
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
  if (show && gui_menu_is_shown(cxm->menu)) {
    gui_menu_show(cxm->menu, FALSE);
  }
  else {
    gui_menu_show(cxm->menu, show);
  }
}
