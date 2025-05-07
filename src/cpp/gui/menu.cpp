/** @file gui/menu.c

  @author  Melwin Svensson.
  @date    7-5-2025.

 */
#include "../../include/prototypes.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_GUI_MENU            \
  ASSERT(menu);                    \
  ASSERT(menu->buffer);            \
  ASSERT(menu->element);           \
  ASSERT(menu->entries);           \
  ASSERT(menu->sb);                \
  ASSERT(menu->font);              \
  ASSERT(menu->data);              \
  ASSERT(menu->position_routine);  \
  ASSERT(menu->accept_routine)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct Menu {
  /* Boolian flags. */
  bool text_refresh_needed  : 1;
  bool pos_refresh_needed   : 1;
  bool accept_on_tab        : 1;  /* If tab should act like enter when this menu is active. */
  bool width_refresh_needed : 1;  /* When something has changed and the width of the menu must be recalculated. */
  bool width_is_static      : 1;  /* The width of the menu is static, this can only be set by `gui_menu_set_static_width()`. */

  /* Configuration variables. */
  Uchar border_size;
  float width;

  vertex_buffer_t *buffer;
  guielement      *element;
  CVec            *entries;
  int              viewtop;
  int              selected;
  int              maxrows;
  int              rows;
  GuiScrollbar    *sb;
  GuiFont         *font;

  /* Callback related data. */
  void          *data;              /* This ptr gets passed to all callbacks and should be passed to `gui_menu_create()` as `data`. */
  MenuPosFunc    position_routine;
  MenuAcceptFunc accept_routine;
};


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* ----------------------------- Static ----------------------------- */

static void gui_menu_scrollbar_update_routine(void *arg, float *total_length, Uint *start, Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset) {
  ASSERT(arg);
  Menu *menu = (__TYPE(menu))arg;
  ASSIGN_IF_VALID(total_length, (menu->element->size.h - 2));
  ASSIGN_IF_VALID(start, 0);
  ASSIGN_IF_VALID(total, cvec_len(menu->entries) - menu->rows);
  ASSIGN_IF_VALID(visible, menu->rows);
  ASSIGN_IF_VALID(current, menu->viewtop);
  ASSIGN_IF_VALID(top_offset, 1);
  ASSIGN_IF_VALID(right_offset, 1);
}

static void gui_menu_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  Menu *menu = (__TYPE(menu))arg;
  menu->viewtop = index;
}

/* TODO: Make the menu tall enough so that the scrollbar gets corrently initilazed before first use. */
static void gui_menu_scrollbar_create(Menu *const menu) {
  ASSERT(menu);
  ASSERT(menu->element);
  menu->sb = guiscrollbar_create(menu->element, menu, gui_menu_scrollbar_update_routine, gui_menu_scrollbar_moving_routine);
}

/*  */
static float gui_menu_calculate_width(Menu *const menu) {
  ASSERT_GUI_MENU;
  int len = cvec_len(menu->entries);
  int longest_index;
  Ulong longest_string;
  Ulong value;
  if (len && !menu->width_is_static && menu->width_refresh_needed) {
    longest_index = 0;
    longest_string = strlen((char *)cvec_get(menu->entries, 0));
    for (int i=1; i<len; ++i) {
      if ((value = strlen((char *)cvec_get(menu->entries, i))) > longest_string) {
        longest_index = i;
        longest_string = value;
      }
    }
    menu->width = (pixbreadth(menu->font, (char *)cvec_get(menu->entries, longest_index)) + (menu->border_size * 2) + 2);
    menu->width_refresh_needed = FALSE;
  }
  return menu->width;
}

static void gui_menu_resize(Menu *const menu) {
  ASSERT_GUI_MENU;
  vec2 pos, size;
  int len = cvec_len(menu->entries);
  /* If there are entries in the menu or we need to recalculate the position. */
  if (len && menu->pos_refresh_needed) {
    /* Set the number of visable rows. */
    if (len > menu->maxrows) {
      menu->rows = menu->maxrows;
    }
    else {
      menu->rows = len;
    }
    /* Calculate the size of the suggestmenu window. */
    size.w = gui_menu_calculate_width(menu);
    size.h = ((menu->rows * gui_font_height(menu->font)) + (menu->border_size * 2) + 2);
    /* Add the size of the scrollbar if there is one. */
    if (len > menu->maxrows) {
      size.w += guiscrollbar_width(menu->sb);
    }
    /* Get the wanted position based on the calculated size. */
    menu->position_routine(menu->data, size, &pos);
    /* Move and resize the element. */
    guielement_move_resize(menu->element, pos, size);
    menu->pos_refresh_needed = FALSE;
  }
}

static void gui_menu_draw_selected(Menu *const menu) {
  ASSERT_GUI_MENU;
  int selected_row = (menu->selected - menu->viewtop);
  vec2 pos, size;
  /* Draw the selected entry, if its on screen. */
  if (selected_row >= 0 && selected_row < menu->rows) {
    pos.x  = (menu->element->pos.x + menu->border_size);
    size.w = (menu->element->size.w - (menu->border_size * 2));
    gui_font_row_top_bot(menu->font, selected_row, &pos.y, &size.h);
    size.h -= (pos.y - ((selected_row == menu->rows - 1) ? 1 : 0));
    pos.y  += (menu->element->pos.y + menu->border_size);
    draw_rect(pos, size, vec4(vec3(1.0f), 0.4f));
  }
}

static void gui_menu_draw_text(Menu *const menu) {
  ASSERT_GUI_MENU;
  static int was_viewtop = menu->viewtop;
  int row = 0;
  char *str;
  vec2 textpen;
  /* Only clear and reconstruct the vertex buffer when asked or when the viewtop has changed. */
  if (menu->text_refresh_needed || was_viewtop != menu->viewtop) {
    vertex_buffer_clear(menu->buffer);
    while (row < menu->rows) {
      str = (char *)cvec_get(menu->entries, (menu->viewtop + row));
      textpen = vec2(
        (menu->element->pos.x + menu->border_size + 1),
        (gui_font_row_baseline(menu->font, row) + menu->element->pos.y + menu->border_size + 1)
      );
      vertex_buffer_add_string(menu->buffer, str, strlen(str), NULL, gui_font_get_font(menu->font), 1, &textpen);
      ++row;
    }
    was_viewtop = menu->viewtop;
    menu->text_refresh_needed = FALSE;
  }
  upload_texture_atlas(gui_font_get_atlas(menu->font));
  render_vertex_buffer(gui->font_shader, menu->buffer);
}

/* ----------------------------- Global ----------------------------- */

/* Create a allocated `Menu`. */
Menu *gui_menu_create(guielement *const parent, GuiFont *const font, void *data, MenuPosFunc position_routine, MenuAcceptFunc accept_routine) {
  ASSERT(parent);
  ASSERT(font);
  ASSERT(data);
  ASSERT(position_routine);
  Menu *menu;
  MALLOC_STRUCT(menu);
  /* Boolian flags. */
  menu->text_refresh_needed  = TRUE;
  menu->pos_refresh_needed   = TRUE;
  menu->accept_on_tab        = FALSE;  /* The default for menu's if to have this disabled. */
  menu->width_refresh_needed = TRUE;
  /* Configuration variables. */
  menu->border_size = 1;  /* The default border size is 1px. */
  menu->width       = 0.0f;
  /* Vertex buffer. */
  menu->buffer  = make_new_font_buffer();
  /* Entries vector. */
  menu->entries = cvec_create_setfree(free);
  /* Create the element of the menu. */
  menu->element = guielement_create(parent);
  menu->element->color = GUI_BLACK_COLOR;  /* The default background color for menu's is black. */
  menu->element->flag.set<GUIELEMENT_ABOVE>();
  menu->element->flag.set<GUIELEMENT_HIDDEN>();
  /* As default all menus should have borders, to create a uniform look.  Note that this can be configured.  TODO: Implement the config of borders. */
  guielement_set_borders(menu->element, 1, vec4(vec3(0.5f), 1.0f));
  /* Row init. */
  menu->viewtop  = 0;
  menu->selected = 0;
  menu->maxrows  = 8;
  menu->rows     = 0;
  gui_menu_scrollbar_create(menu);
  menu->font = font;
  /* Callback's. */
  menu->data             = data;
  menu->position_routine = position_routine;
  menu->accept_routine   = accept_routine;
  return menu;
}

void gui_menu_free(Menu *const menu) {
  if (!menu) {
    return;
  }
  vertex_buffer_delete(menu->buffer);
  cvec_free(menu->entries);
  free(menu->sb);
  free(menu);
}

void gui_menu_draw(Menu *const menu) {
  ASSERT_GUI_MENU;
  /* Only draw the suggestmenu if there are any available suggestions. */
  if (!menu->element->flag.is_set<GUIELEMENT_HIDDEN>() && cvec_len(menu->entries)) {
    gui_menu_resize(menu);
    /* Draw the main element of the suggestmenu. */
    guielement_draw(menu->element);
    /* Highlight the selected entry in the suggestmenu when its on the screen. */
    gui_menu_draw_selected(menu);
    /* Draw the scrollbar of the suggestmenu. */
    guiscrollbar_draw(menu->sb);
    /* Draw the text of the suggestmenu entries. */
    gui_menu_draw_text(menu);
  }
}

void gui_menu_push_back(Menu *const menu, const char *const restrict string) {
  ASSERT_GUI_MENU;
  cvec_push(menu->entries, copy_of(string));
  menu->width_refresh_needed = TRUE;
}

void gui_menu_pos_refresh_needed(Menu *const menu) {
  ASSERT_GUI_MENU;
  menu->pos_refresh_needed = TRUE;
}

void gui_menu_text_refresh_needed(Menu *const menu) {
  ASSERT_GUI_MENU;
  menu->text_refresh_needed = TRUE;
}

void gui_menu_scrollbar_refresh_needed(Menu *const menu) {
  ASSERT_GUI_MENU;
  guiscrollbar_refresh_needed(menu->sb);
}

void gui_menu_show(Menu *const menu, bool show) {
  ASSERT_GUI_MENU;
  /* Showing this menu. */
  if (show) {
    if (gui->active_menu && gui->active_menu != menu) {
      guielement_set_flag_recurse(gui->active_menu->element, TRUE, GUIELEMENT_HIDDEN);
    }
    guielement_set_flag_recurse(menu->element, FALSE, GUIELEMENT_HIDDEN);
    gui->active_menu = menu;
    /* Ensure this menu gets fully updated. */
    menu->text_refresh_needed = TRUE;
    menu->pos_refresh_needed  = TRUE;
    guiscrollbar_refresh_needed(menu->sb);
    /* Also, reset the menu. */
    menu->viewtop  = 0;
    menu->selected = 0;
  }
  /* Hiding this menu. */
  else {
    if (gui->active_menu) {
      guielement_set_flag_recurse(gui->active_menu->element, TRUE, GUIELEMENT_HIDDEN);
      gui->active_menu = NULL;
    }
    guielement_set_flag_recurse(menu->element, TRUE, GUIELEMENT_HIDDEN);
  }
}

void gui_menu_selected_up(Menu *const menu) {
  ASSERT_GUI_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* If we are at the first entry. */
    if (menu->selected == 0) {
      menu->selected = (len - 1);
      if (len > menu->maxrows) {
        menu->viewtop = (len - menu->maxrows);
      }
    }
    else {
      if (menu->viewtop == menu->selected) {
        --menu->viewtop;
      }
      --menu->selected;
    }
    guiscrollbar_refresh_needed(menu->sb);
  }
}

void gui_menu_selected_down(Menu *const menu) {
  ASSERT_GUI_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* If we are at the last entry. */
    if (menu->selected == (len - 1)) {
      menu->selected = 0;
      menu->viewtop  = 0;
    }
    else {
      if (menu->selected == (menu->viewtop + menu->maxrows - 1)) {
        ++menu->viewtop;
      }
      ++menu->selected;
    }
    guiscrollbar_refresh_needed(menu->sb);
  }
}

void gui_menu_accept_action(Menu *const menu) {
  ASSERT_GUI_MENU;
  /* As a sanity check only perform any action when the menu is not empty. */
  if (cvec_len(menu->entries)) {
    /* Run the user's accept routine. */
    menu->accept_routine(menu->data, (char *)cvec_get(menu->entries, menu->selected), menu->selected);
    /* Then stop showing the menu. */
    gui_menu_show(menu, FALSE);
  }
}

void gui_menu_hover_action(Menu *const menu, float y_pos) {
  ASSERT_GUI_MENU;
  long row;
  float top;
  float bot;
  if (cvec_len(menu->entries)) {
    /* Top of the completions menu. */
    top = (menu->element->pos.y);
    /* Bottom of the completions menu. */
    bot = (menu->element->pos.y + menu->element->size.h);
    /* If `y_pos` relates to a valid row in the suggestmenu completion menu, then adjust the selected to that row. */
    if (gui_font_row_from_pos(menu->font, top, bot, y_pos, &row)) {
      menu->selected = lclamp((menu->viewtop + row), menu->viewtop, (menu->viewtop + menu->rows));
    }
  }
}

void gui_menu_scroll_action(Menu *const menu, bool direction, float y_pos) {
  ASSERT_GUI_MENU;
  int len = cvec_len(menu->entries);
  /* Only do anything if there are more entries then rows in the suggestmenu. */
  if (len > menu->maxrows) {
    /* Only scroll when not already at the top or bottom. */
    if ((direction == BACKWARD && menu->viewtop > 0) || (direction == FORWARD && menu->viewtop < (len - menu->maxrows))) {
      menu->viewtop += (!direction ? -1 : 1);
      menu->text_refresh_needed = TRUE;
      /* Ensure that the currently selected entry gets correctly set based on where the mouse is. */
      gui_menu_hover_action(menu, y_pos);
      guiscrollbar_refresh_needed(menu->sb);
    }
  }
}

void gui_menu_click_action(Menu *const menu, float y_pos) {
  ASSERT_GUI_MENU;
  float top;
  float bot;
  /* Only perform any action when there are completions available. */
  if (cvec_len(menu->entries)) {
    top = menu->element->pos.y;
    bot = (menu->element->pos.y + menu->element->size.h);
    if (y_pos >= top && y_pos <= bot) {
      gui_menu_hover_action(menu, y_pos);
      gui_menu_accept_action(menu);
    }
  }
}

void gui_menu_clear_entries(Menu *const menu) {
  ASSERT_GUI_MENU;
  cvec_clear(menu->entries);
  menu->viewtop  = 0;
  menu->selected = 0;
  menu->text_refresh_needed  = TRUE;
  menu->pos_refresh_needed   = TRUE;
  menu->width_refresh_needed = TRUE;
}

/* Set a static width for `menu`.  TODO: Implement this into text drawing function, so that when there is not enough room it cuts of the entry. */
void gui_menu_set_static_width(Menu *const menu, float width) {
  ASSERT_GUI_MENU;
  ALWAYS_ASSERT_MSG((width > 0.0f), "The width of a menu must be positive");
  menu->width = width;
  menu->width_is_static = TRUE;
}

/* Configure's the tab behavior for `menu`, if `accept_on_tab` is `TRUE` then tab will act like enter when this menu is active. */
void gui_menu_set_tab_accept_behavior(Menu *const menu, bool accept_on_tab) {
  ASSERT_GUI_MENU;
  menu->accept_on_tab = accept_on_tab;
}

/* Return's `TRUE` if `e` is part of `menu`. */
bool gui_menu_owns_element(Menu *const menu, guielement *const e) {
  ASSERT_GUI_MENU;
  return is_ancestor(e, menu->element);
}

/* Return's `TRUE` if `e` is the main element of `menu`. */
bool gui_menu_element_is_main(Menu *const menu, guielement *const e) {
  ASSERT_GUI_MENU;
  ASSERT(e);
  return (menu->element == e);
}

/* Return's `TRUE` if `menu` has `accept_on_tab` flag set. */
bool gui_menu_should_accept_on_tab(Menu *const menu) {
  ASSERT_GUI_MENU;
  return menu->accept_on_tab;
}
