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

#define GUI_MENU_DEFAULT_BORDER_SIZE   1
#define GUI_MENU_DEFAULT_MAX_ROWS      8
#define GUI_MENU_DEFAULT_BORDER_COLOR  vec4(vec3(0.5f), 1.0f)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


typedef struct {
  char *lable;
  Menu *menu;
} MenuEntry;

struct Menu {
  /* Boolian flags. */
  bool text_refresh_needed    : 1;
  bool pos_refresh_needed     : 1;
  bool accept_on_tab          : 1;  /* If tab should act like enter when this menu is active. */
  bool width_refresh_needed   : 1;  /* When something has changed and the width of the menu must be recalculated. */
  bool width_is_static        : 1;  /* The width of the menu is static, this can only be set by `gui_menu_set_static_width()`. */
  bool arrow_depth_navigation : 1;  /* Wether right and left arrows allows for closing and opening submenu's. */

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

  /* Used when this menu is a `submenu`, otherwise always `NULL`. */
  Menu *parent;

  /* The currently open submenu, if any. */
  Menu *active_submenu;

  /* Callback related data. */
  void          *data;              /* This ptr gets passed to all callbacks and should be passed to `gui_menu_create()` as `data`. */
  MenuPosFunc    position_routine;
  MenuAcceptFunc accept_routine;
};


/* ---------------------------------------------------------- MenuEntry function's ---------------------------------------------------------- */


/* Create a allocated `MenuEntry` structure with a `lable` and no menu ptr. */
static MenuEntry *gui_menu_entry_create(const char *const restrict lable) {
  MenuEntry *me;
  MALLOC_STRUCT(me);
  me->lable = copy_of(lable);
  me->menu  = NULL;
  return me;
}

/* Free callback for a `MenuEntry`. */
static void gui_menu_entry_free(void *arg) {
  ASSERT(arg);
  MenuEntry *me = (__TYPE(me))arg;
  free(me->lable);
  gui_menu_free(me->menu);
  free(me);
}

static MenuEntry *gui_menu_entry_create_with_menu(const char *const restrict lable, Menu *const menu) {
  ASSERT(lable);
  ASSERT(menu);
  MenuEntry *me = gui_menu_entry_create(lable);
  me->menu = menu;
  return me;
}


/* ---------------------------------------------------------- Menu static function's ---------------------------------------------------------- */


/* The scrollbar update routine for the `Menu` structure. */
static void gui_menu_scrollbar_update_routine(void *arg, float *total_length, Uint *start, Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset) {
  ASSERT(arg);
  Menu *menu = (__TYPE(menu))arg;
  ASSIGN_IF_VALID(total_length, (menu->element->size.h - (menu->border_size * 2)));
  ASSIGN_IF_VALID(start, 0);
  ASSIGN_IF_VALID(total, cvec_len(menu->entries) - menu->rows);
  ASSIGN_IF_VALID(visible, menu->rows);
  ASSIGN_IF_VALID(current, menu->viewtop);
  ASSIGN_IF_VALID(top_offset, menu->border_size);
  ASSIGN_IF_VALID(right_offset, menu->border_size);
}

/* The scrollbar moving routine for the `Menu` structure. */
static void gui_menu_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  Menu *menu = (__TYPE(menu))arg;
  menu->viewtop = lclamp(index, 0, (cvec_len(menu->entries) - menu->rows));
}

/* TODO: Make the menu tall enough so that the scrollbar gets corrently initilazed before first use. */
static void gui_menu_scrollbar_create(Menu *const menu) {
  ASSERT(menu);
  ASSERT(menu->element);
  menu->sb = gui_scrollbar_create(menu->element, menu, gui_menu_scrollbar_update_routine, gui_menu_scrollbar_moving_routine);
}

/* Return's the `lable` of the entry at `index`. */
static char *gui_menu_get_entry_lable(Menu *const menu, int index) {
  ASSERT_GUI_MENU;
  return ((MenuEntry *)cvec_get(menu->entries, index))->lable;
}

/* Return's the `menu` of the entry at `index`. */
static Menu *gui_menu_get_entry_menu(Menu *const menu, int index) {
  ASSERT_GUI_MENU;
  return ((MenuEntry *)cvec_get(menu->entries, index))->menu;
}

/* Assigns the global absolute y top and bottom position as well as the most right allowed x position. */
static void gui_menu_event_bounds(Menu *const menu, float *const top, float *const bot, float *const right) {
  ASSERT_GUI_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* Top of the menu. */
    ASSIGN_IF_VALID(top, (menu->element->pos.y + menu->border_size));
    /* Bottom of the menu. */
    ASSIGN_IF_VALID(bot, ((menu->element->pos.y + menu->border_size) + (menu->element->size.h - menu->border_size)));
    /* The right most allowed position to register a event. */
    ASSIGN_IF_VALID(right, ((menu->element->pos.x + menu->border_size) + (menu->element->size.w - menu->border_size) - ((len > menu->maxrows) ? gui_scrollbar_width(menu->sb) : 0)));
  }
}

/* Reset's the state of menu and all its children recursivly. */
static void gui_menu_reset(Menu *const menu) {
  ASSERT_GUI_MENU;
  if (menu->active_submenu) {
    gui_menu_reset(menu->active_submenu);
  }
  /* Ensure this menu gets fully updated. */
  menu->text_refresh_needed = TRUE;
  menu->pos_refresh_needed  = TRUE;
  /* Also, reset the menu to the starting state. */
  menu->viewtop  = 0;
  menu->selected = 0;
  /* And tell the scrollbar it needs to be updated. */
  gui_scrollbar_refresh_needed(menu->sb);
}

static void gui_menu_show_internal(Menu *const menu, bool show) {
  ASSERT_GUI_MENU;
  if (show) {
    menu->element->flag.unset<GUIELEMENT_HIDDEN>();
    gui_menu_reset(menu);
  }
  else {
    menu->element->flag.set<GUIELEMENT_HIDDEN>();
    if (menu->active_submenu) {
      gui_menu_show_internal(menu->active_submenu, FALSE);
      menu->active_submenu = NULL;
    }
  }
  gui_scrollbar_show(menu->sb, show);
}

static bool gui_menu_selected_is_visible(Menu *const menu) {
  ASSERT_GUI_MENU;
  int row = (menu->selected - menu->viewtop);
  return (row >= 0 && row < menu->maxrows);
}

static void gui_menu_check_submenu(Menu *const menu) {
  ASSERT_GUI_MENU;
  Menu *submenu;
  if (gui_menu_selected_is_visible(menu) && (submenu = gui_menu_get_entry_menu(menu, menu->selected))) {
    if (menu->active_submenu && submenu != menu->active_submenu) {
      gui_menu_show_internal(menu->active_submenu, FALSE);
    }
    else {
      gui_menu_show_internal(submenu, TRUE);
      menu->active_submenu = submenu;
    }
  }
  /* The currently selected row is outside the visible rows. */
  else if (menu->active_submenu) {
    gui_menu_show_internal(menu->active_submenu, FALSE);
    menu->active_submenu = NULL;
  }
}

static float gui_menu_calculate_width(Menu *const menu) {
  ASSERT_GUI_MENU;
  int len = cvec_len(menu->entries);
  int longest_index;
  Ulong longest_string;
  Ulong value;
  if (len && !menu->width_is_static && menu->width_refresh_needed) {
    longest_index = 0;
    longest_string = strlen(gui_menu_get_entry_lable(menu, 0));
    for (int i=1; i<len; ++i) {
      if ((value = strlen(gui_menu_get_entry_lable(menu, i))) > longest_string) {
        longest_index = i;
        longest_string = value;
      }
    }
    menu->width = (pixbreadth(menu->font, gui_menu_get_entry_lable(menu, longest_index)) + (menu->border_size * 2) + 2);
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
      size.w += gui_scrollbar_width(menu->sb);
    }
    /* Get the wanted position based on the calculated size. */
    if (!menu->parent) {
      menu->position_routine(menu->data, size, &pos);
    }
    else {
      menu->position_routine(menu, size, &pos);
    }
    /* Move and resize the element. */
    gui_element_move_resize(menu->element, pos, size);
    menu->pos_refresh_needed = FALSE;
  }
}

static void gui_menu_draw_selected(Menu *const menu) {
  ASSERT_GUI_MENU;
  int row = (menu->selected - menu->viewtop);
  vec2 pos, size;
  /* Draw the selected entry, if its on screen. */
  if (row >= 0 && row < menu->rows) {
    pos.x  = (menu->element->pos.x + menu->border_size);
    size.w = (menu->element->size.w - (menu->border_size * 2));
    gui_font_row_top_bot(menu->font, row, &pos.y, &size.h);
    size.h -= (pos.y - ((row == menu->rows - 1) ? 1 : 0));
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
      str = gui_menu_get_entry_lable(menu, (menu->viewtop + row));
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

/* For a submenu the submenu itself needs to be passed as the routine. */
static void gui_menu_submenu_pos_routine(void *arg, vec2 size, vec2 *pos) {
  ASSERT(arg);
  ASSERT(pos);
  Menu *menu;
  int index = 0;
  CAST(menu, arg);
  /* Get the true index of this submenu entry. */
  while (((MenuEntry *)cvec_get(menu->parent->entries, index))->menu != menu) {
    ++index;
  }
  /* Then offset the index to the visible entries. */
  index -= menu->parent->viewtop;
  /* And always ensure it falls inside it, as this function should not be called otherwise. */
  ALWAYS_ASSERT(index >= 0 && index < menu->parent->maxrows);
  pos->x = (menu->parent->element->pos.x + menu->parent->element->size.w);
  gui_font_row_top_bot(menu->font, index, &pos->y, NULL);
  pos->y += menu->parent->element->pos.y;
}

static void gui_menu_push_back_submenu(Menu *const menu, const char *const restrict lable, Menu *const submenu) {
  ASSERT_GUI_MENU;
  ASSERT(lable);
  ASSERT(submenu);
  cvec_push(menu->entries, gui_menu_entry_create_with_menu(lable, submenu));
  menu->width_refresh_needed = TRUE;
}

static void gui_menu_selected_up_internal(Menu *const menu) {
  ASSERT_GUI_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* If we are at the first entry. */
    if (menu->selected == 0) {
      menu->selected = (len - 1);
      /* If there are more entries then visible rows, we also need to change the viewtop. */
      if (len > menu->maxrows) {
        menu->viewtop = (len - menu->maxrows);
        /* Only update the scrollbar when the viewtop has changed. */
        gui_scrollbar_refresh_needed(menu->sb);
      }
    }
    /* Otherwise, we are anywhere below that. */
    else {
      /* If the currently selected is the current viewtop.  Decrease it by one. */
      if (menu->viewtop == menu->selected) {
        --menu->viewtop;
        /* Only update the scrollbar when the viewtop has changed. */
        gui_scrollbar_refresh_needed(menu->sb);
      }
      --menu->selected;
    }
    gui_menu_check_submenu(menu);
  }
}

static void gui_menu_selected_down_internal(Menu *const menu) {
  ASSERT_GUI_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* If we are at the last entry. */
    if (menu->selected == (len - 1)) {
      menu->selected = 0;
      menu->viewtop  = 0;
      /* Only update the scrollbar when the viewtop has changed. */
      gui_scrollbar_refresh_needed(menu->sb);
    }
    else {
      if (menu->selected == (menu->viewtop + menu->maxrows - 1)) {
        ++menu->viewtop;
        /* Only update the scrollbar when the viewtop has changed. */
        gui_scrollbar_refresh_needed(menu->sb);
      }
      ++menu->selected;
    }
    gui_menu_check_submenu(menu);
  }
}

/* Used to exit a submenu when pressing left, this is used by `gui_menu_exit_submenu()`. */
static void gui_menu_exit_submenu_internal(Menu *const menu) {
  ASSERT_GUI_MENU;
  if (menu->parent) {
    gui_menu_show_internal(menu, FALSE);
    menu->parent->active_submenu = NULL;
  }
}


/* ---------------------------------------------------------- Menu global function's ---------------------------------------------------------- */


/* Create a allocated `Menu`. */
Menu *gui_menu_create(guielement *const parent, GuiFont *const font, void *data, MenuPosFunc position_routine, MenuAcceptFunc accept_routine) {
  ASSERT(parent);
  ASSERT(font);
  ASSERT(data);
  ASSERT(position_routine);
  ASSERT(accept_routine);
  Menu *menu;
  MALLOC_STRUCT(menu);
  /* Boolian flags. */
  menu->text_refresh_needed    = TRUE;
  menu->pos_refresh_needed     = TRUE;
  menu->accept_on_tab          = FALSE;  /* The default for menu's if to have this disabled. */
  menu->width_refresh_needed   = TRUE;
  menu->arrow_depth_navigation = TRUE;   /* Default `arrow_depth_navigation` is `TRUE`/enabled. */
  /* Configuration variables. */
  menu->border_size = GUI_MENU_DEFAULT_BORDER_SIZE;
  menu->width       = 0.0f;
  /* Vertex buffer. */
  menu->buffer  = make_new_font_buffer();
  /* Entries vector. */
  menu->entries = cvec_create_setfree(gui_menu_entry_free);
  /* Create the element of the menu. */
  menu->element = gui_element_create(parent);
  menu->element->color = GUI_BLACK_COLOR;  /* The default background color for menu's is black. */
  menu->element->flag.set<GUIELEMENT_ABOVE>();
  menu->element->flag.set<GUIELEMENT_HIDDEN>();
  /* As default all menus should have borders, to create a uniform look.  Note that this can be configured.  TODO: Implement the config of borders. */
  gui_element_set_borders(menu->element, menu->border_size, GUI_MENU_DEFAULT_BORDER_COLOR);
  gui_element_set_menu_data(menu->element, menu);
  /* Row init. */
  menu->viewtop  = 0;
  menu->selected = 0;
  menu->maxrows  = GUI_MENU_DEFAULT_MAX_ROWS;
  menu->rows     = 0;
  gui_menu_scrollbar_create(menu);
  menu->font = font;
  /* Always init the `parent` and `active_submenu` as `NULL`. */
  menu->parent = NULL;
  menu->active_submenu = NULL;
  /* Callback's. */
  menu->data             = data;
  menu->position_routine = position_routine;
  menu->accept_routine   = accept_routine;
  return menu;
}

Menu *gui_menu_create_submenu(Menu *const parent, const char *const restrict lable, void *data, MenuAcceptFunc accept_routine) {
  ASSERT(parent);
  ASSERT(data);
  ASSERT(accept_routine);
  Menu *menu = gui_menu_create(parent->element, parent->font, data, gui_menu_submenu_pos_routine, accept_routine);
  menu->parent = parent;
  gui_menu_push_back_submenu(parent, lable, menu);
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

/* Perform a draw call for a `Menu`.  Note that this should be called every frame for all root menu's and never for submenu's. */
void gui_menu_draw(Menu *const menu) {
  ASSERT_GUI_MENU;
  Menu *submenu;
  /* Only draw the suggestmenu if there are any available suggestions. */
  if (!menu->element->flag.is_set<GUIELEMENT_HIDDEN>() && cvec_len(menu->entries)) {
    gui_menu_resize(menu);
    /* Draw the main element of the suggestmenu. */
    gui_element_draw(menu->element);
    /* Highlight the selected entry in the suggestmenu when its on the screen. */
    gui_menu_draw_selected(menu);
    /* Draw the scrollbar of the suggestmenu. */
    gui_scrollbar_draw(menu->sb);
    /* Draw the text of the suggestmenu entries. */
    gui_menu_draw_text(menu);
    /*  */
    for (int i=0; i<cvec_len(menu->entries); ++i) {
      if ((submenu = gui_menu_get_entry_menu(menu, i))) {
        gui_menu_draw(submenu);
      }
    }
  }
}

void gui_menu_push_back(Menu *const menu, const char *const restrict string) {
  ASSERT_GUI_MENU;
  ASSERT(string);
  cvec_push(menu->entries, gui_menu_entry_create(string));
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
  gui_scrollbar_refresh_needed(menu->sb);
}

void gui_menu_show(Menu *const menu, bool show) {
  ASSERT_GUI_MENU;
  /* Showing this menu. */
  if (show) {
    /* Always close the currently active menu, even when its the same as `menu`.  This ensures correctness related to submenus. */
    if (gui->active_menu) {
      gui_menu_show_internal(gui->active_menu, FALSE);
    }
    gui_menu_show_internal(menu, TRUE);
    gui->active_menu = menu;
  }
  /* Hiding this menu. */
  else {
    if (gui->active_menu) {
      gui_menu_show_internal(gui->active_menu, FALSE);
      gui->active_menu = NULL;
    }
    gui_menu_show_internal(menu, FALSE);
  }
}

void gui_menu_selected_up(Menu *const menu) {
  ASSERT_GUI_MENU;
  /* Recursivly call this function, until we reach the bottom. */
  if (menu->active_submenu) {
    gui_menu_selected_up(menu->active_submenu);
  }
  /* Then call the internal function that performs the action. */
  else {
    gui_menu_selected_up_internal(menu);
  }
}

void gui_menu_selected_down(Menu *const menu) {
  ASSERT_GUI_MENU;
  /* Recursivly call this function, until we reach the bottom. */
  if (menu->active_submenu) {
    gui_menu_selected_down(menu->active_submenu);
  }
  /* Then call the internal function that performs the action. */
  else {
    gui_menu_selected_down_internal(menu);
  }
}

void gui_menu_exit_submenu(Menu *const menu) {
  ASSERT_GUI_MENU;
  if (menu->active_submenu) {
    gui_menu_exit_submenu(menu->active_submenu);
  }
  else {
    gui_menu_exit_submenu_internal(menu);
  }
}

void gui_menu_enter_submenu(Menu *const menu) {
  ASSERT_GUI_MENU;
  if (menu->active_submenu) {
    gui_menu_enter_submenu(menu->active_submenu);
  }
  else {
    gui_menu_check_submenu(menu);
  }
}

/* This is used to perform the accept action of the depest opened menu's currently selected entry,
 * or if that selected entry has a submenu and its not open, then it will open it.  This is used
 * for both clicking and kb related execution of the accept routine for that menu. */
void gui_menu_accept_action(Menu *const menu) {
  ASSERT_GUI_MENU;
  /* As a sanity check only perform any action when the menu is not empty. */
  if (cvec_len(menu->entries)) {
    /* If the currently selected entry of `menu` does not have a submenu, call the accept routine for `menu`. */
    if (!gui_menu_get_entry_menu(menu, menu->selected)) {
      /* Run the user's accept routine. */
      menu->accept_routine(menu->data, gui_menu_get_entry_lable(menu, menu->selected), menu->selected);
      /* Then stop showing the menu. */
      gui_menu_show(menu, FALSE);
    }
    /* Otherwise, if the entry has a submenu but its not currently open, open it. */
    else if (!menu->active_submenu) {
      gui_menu_check_submenu(menu);
    }
    /* And if that menu is currently active, recursivly call this function on that menu. */
    else {
      gui_menu_accept_action(menu->active_submenu);
    }
  }
}

void gui_menu_hover_action(Menu *const menu, float x_pos, float y_pos) {
  ASSERT_GUI_MENU;
  long row;
  float top;
  float bot;
  float right;
  /* Only perform any action when there are entries in the menu. */
  if (cvec_len(menu->entries)) {
    /* Get the absolute values where events are allowed. */
    gui_menu_event_bounds(menu, &top, &bot, &right);
    /* If `y_pos` relates to a valid row in the suggestmenu completion menu, then adjust the selected to that row. */
    if (gui_font_row_from_pos(menu->font, top, bot, y_pos, &row) && x_pos < right) {
      menu->selected = lclamp((menu->viewtop + row), menu->viewtop, (menu->viewtop + menu->rows));
      gui_menu_check_submenu(menu);
    }
  }
}

void gui_menu_scroll_action(Menu *const menu, bool direction, float x_pos, float y_pos) {
  ASSERT_GUI_MENU;
  float top;
  float bot;
  float right;
  int len = cvec_len(menu->entries);
  /* Only do anything if there are more entries then rows in the menu then maximum number of rows allowed. */
  if (len > menu->maxrows) {
    /* Get the absolute values where events are allowed. */
    gui_menu_event_bounds(menu, &top, &bot, &right);
    /* Only scroll when not already at the top or bottom.  And the call was made from a valid position. */
    if (y_pos >= top && y_pos <= bot && x_pos < right && ((direction == BACKWARD && menu->viewtop > 0) || (direction == FORWARD && menu->viewtop < (len - menu->maxrows)))) {
      menu->viewtop += (!direction ? -1 : 1);
      menu->text_refresh_needed = TRUE;
      /* Ensure that the currently selected entry gets correctly set based on where the mouse is. */
      gui_menu_hover_action(menu, x_pos, y_pos);
      gui_scrollbar_refresh_needed(menu->sb);
    }
  }
}

void gui_menu_click_action(Menu *const menu, float x_pos, float y_pos) {
  ASSERT_GUI_MENU;
  float top;
  float bot;
  float right;
  /* Only perform any action when there are entries in the menu. */
  if (cvec_len(menu->entries)) {
    /* Get the absolute values where events are allowed. */
    gui_menu_event_bounds(menu, &top, &bot, &right);
    if (y_pos >= top && y_pos <= bot && x_pos < right) {
      gui_menu_hover_action(menu, x_pos, y_pos);
      /* Only allow clickes on non submenu entries. */
      if (!gui_menu_get_entry_menu(menu, menu->selected)) {
        gui_menu_accept_action(menu);
      }
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

/* Configure's if `menu` should use right arrow to open the submenu at the currently selected entry if it exists and left arrow to close the currently open submenu. */
void gui_menu_set_arrow_depth_navigation(Menu *const menu, bool enable_arrow_depth_navigation) {
  ASSERT_GUI_MENU;
  menu->arrow_depth_navigation = enable_arrow_depth_navigation;
}

/* ----------------------------- Boolian function's ----------------------------- */

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

/* Return's `TRUE` if `menu` has `arrow_depth_navigation` flag set.  This means that `left` and `right arrow` close's and open's `submenu's`. */
bool gui_menu_allows_arrow_navigation(Menu *const menu) {
  ASSERT_GUI_MENU;
  return menu->arrow_depth_navigation;
}

/* Return's `TRUE` when `ancestor` is an ancestor to `menu`. */
bool gui_menu_is_ancestor(Menu *const menu, Menu *const ancestor) {
  ASSERT(menu);
  ASSERT(ancestor);
  Menu *m = menu;
  while (m) {
    if (m == ancestor) {
      return TRUE;
    }
    m = m->parent;
  }
  return FALSE;
}

bool gui_menu_is_shown(Menu *const menu) {
  ASSERT_GUI_MENU;
  return (!menu->element->flag.is_set<GUIELEMENT_HIDDEN>() && cvec_len(menu->entries));
}

/* ----------------------------- Getter function's ----------------------------- */

GuiFont *gui_menu_get_font(Menu *const menu) {
  ASSERT_GUI_MENU;
  return menu->font;
}

int gui_menu_len(Menu *const menu) {
  ASSERT_GUI_MENU;
  return cvec_len(menu->entries);
}

/* ----------------------------- Menu qsort callback's ----------------------------- */

/* Function callback used for sorting menu entries by length of lable.  Note that this should be passed to `gui_menu_qsort()` */
int gui_menu_entry_qsort_strlen_cb(const void *a, const void *b) {
  const MenuEntry *lhs = *(const MenuEntry **)a;
  const MenuEntry *rhs = *(const MenuEntry **)b;
  long lhs_len = strlen(lhs->lable);
  long rhs_len = strlen(rhs->lable);
  if (lhs_len == rhs_len) {
    return strcmp(lhs->lable, rhs->lable);
  }
  return (lhs_len - rhs_len); 
}

/* ----------------------------- Menu qsort call ----------------------------- */

void gui_menu_qsort(Menu *const menu, CmpFuncPtr cmp_func) {
  ASSERT_GUI_MENU;
  cvec_qsort(menu->entries, cmp_func);
}
