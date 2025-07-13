/** @file gui/context_menu.c

  @author  Melwin Svensson.
  @date    7-5-2025.

 */
#include "../../include/prototypes.h"


struct ContextMenu {
  Element *element;
  Menu *menu;
  Menu *sm0;
  Menu *sm1;
  Menu *sm2;
};


static void context_menu_pos_routine(void *arg, float width, float height, float *const x, float *const y) {
  ASSERT(arg);
  ASSERT(x);
  ASSERT(y);
  (*x) = get_mouse_xpos();
  (*y) = get_mouse_ypos();
}

static void context_menu_accept_routine(void *arg, const char *const restrict entry_string, int index) {
  ASSERT(arg);
  ASSERT(entry_string);
  writef("%s\n", entry_string);
}

ContextMenu *context_menu_create(void) {
  ContextMenu *cxm;
  MALLOC_STRUCT(cxm);
  cxm->element = element_create(100, 100, 100, 100, TRUE);
  cxm->menu = menu_create(cxm->element, textfont, gui, context_menu_pos_routine, context_menu_accept_routine);
  menu_push_back(cxm->menu, "Root menu test: 0");
  menu_push_back(cxm->menu, "Root menu test: 1");
  menu_push_back(cxm->menu, "Root menu test: 2");
  menu_push_back(cxm->menu, "Root menu test: 3");
  menu_push_back(cxm->menu, "Root menu test: 4");
  menu_push_back(cxm->menu, "Root menu test: 5");
  menu_push_back(cxm->menu, "Root menu test: 6");
  menu_push_back(cxm->menu, "Root menu test: 7");
  menu_push_back(cxm->menu, "Root menu test: 8");
  menu_push_back(cxm->menu, "Root menu test: 9");
  menu_push_back(cxm->menu, "Root menu test: 10");
  menu_push_back(cxm->menu, "Root menu test: 11");
  menu_push_back(cxm->menu, "Root menu test: 12");
  menu_push_back(cxm->menu, "Root menu test: 13");
  menu_push_back(cxm->menu, "Root menu test: 14");
  menu_push_back(cxm->menu, "Root menu test: 15");
  menu_push_back(cxm->menu, "Root menu test: 16");
  menu_push_back(cxm->menu, "Root menu test: 17");
  cxm->sm0 = menu_create_submenu(cxm->menu, "Edit", gui, context_menu_accept_routine);
  cxm->sm1 = menu_create_submenu(cxm->sm0, "Edit sub menu", gui, context_menu_accept_routine);
  menu_push_back(cxm->sm1, "Sm1: 0");
  menu_push_back(cxm->sm1, "Sm1: 1");
  menu_push_back(cxm->sm1, "Sm1: 2");
  menu_push_back(cxm->sm1, "Sm1: 3");
  menu_push_back(cxm->sm1, "Sm1: 4");
  menu_push_back(cxm->sm1, "Sm1: 5");
  menu_push_back(cxm->sm1, "Sm1: 6");
  menu_push_back(cxm->sm1, "Sm1: 7");
  menu_push_back(cxm->sm1, "Sm1: 8");
  menu_push_back(cxm->sm1, "Sm1: 9");
  menu_push_back(cxm->sm1, "Sm1: 10");
  menu_push_back(cxm->sm1, "Sm1: 11");
  menu_push_back(cxm->sm1, "Sm1: 12");
  menu_push_back(cxm->sm1, "Sm1: 13");
  menu_push_back(cxm->sm1, "Sm1: 14");
  menu_push_back(cxm->sm1, "Sm1: 15");
  menu_push_back(cxm->sm1, "Sm1: 16");
  menu_push_back(cxm->sm1, "Sm1: 17");
  cxm->sm2 = menu_create_submenu(cxm->sm1, "sm2", gui, context_menu_accept_routine);
  menu_push_back(cxm->sm2, "sm2: 0");
  menu_push_back(cxm->sm2, "sm2: 1");
  menu_push_back(cxm->sm2, "sm2: 2");
  menu_push_back(cxm->sm2, "sm2: 3");
  menu_push_back(cxm->sm2, "sm2: 4");
  menu_push_back(cxm->sm2, "sm2: 5");
  menu_push_back(cxm->sm2, "sm2: 6");
  menu_push_back(cxm->sm2, "sm2: 7");
  menu_push_back(cxm->sm2, "sm2: 8");
  menu_push_back(cxm->sm2, "sm2: 9");
  menu_push_back(cxm->sm2, "sm2: 10");
  menu_push_back(cxm->sm2, "sm2: 11");
  menu_push_back(cxm->sm2, "sm2: 12");
  menu_push_back(cxm->sm2, "sm2: 13");
  menu_push_back(cxm->sm2, "sm2: 14");
  menu_push_back(cxm->sm2, "sm2: 15");
  menu_push_back(cxm->sm2, "sm2: 16");
  menu_push_back(cxm->sm2, "sm2: 17");
  menu_push_back(cxm->sm0, "Sm0: 0");
  menu_push_back(cxm->sm0, "Sm0: 1");
  menu_push_back(cxm->sm0, "Sm0: 2");
  menu_push_back(cxm->sm0, "Sm0: 3");
  menu_push_back(cxm->sm0, "Sm0: 4");
  menu_push_back(cxm->sm0, "Sm0: 5");
  menu_push_back(cxm->sm0, "Sm0: 6");
  menu_push_back(cxm->sm0, "Sm0: 7");
  menu_push_back(cxm->sm0, "Sm0: 8");
  menu_push_back(cxm->sm0, "Sm0: 9");
  menu_push_back(cxm->sm0, "Sm0: 10");
  menu_push_back(cxm->sm0, "Sm0: 11");
  menu_push_back(cxm->sm0, "Sm0: 12");
  menu_push_back(cxm->sm0, "Sm0: 13");
  menu_push_back(cxm->sm0, "Sm0: 14");
  menu_push_back(cxm->sm0, "Sm0: 15");
  menu_push_back(cxm->sm0, "Sm0: 16");
  menu_push_back(cxm->sm0, "Sm0: 17");
  menu_set_tab_accept_behavior(cxm->menu, TRUE);
  return cxm;
}

void context_menu_free(ContextMenu *const cxm) {
  ASSERT(cxm);
  menu_free(cxm->menu);
  element_free(cxm->element);
  free(cxm);
}

void context_menu_draw(ContextMenu *const cxm) {
  ASSERT(cxm);
  menu_draw(cxm->menu);
}

void context_menu_show(ContextMenu *const cxm, bool show) {
  ASSERT(cxm);
  if (show && menu_is_shown(cxm->menu)) {
    menu_show(cxm->menu, FALSE);
  }
  else {
    menu_show(cxm->menu, show);
  }
}
