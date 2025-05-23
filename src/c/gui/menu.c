/** @file gui/menu.c

  @author  Melwin Svensson.
  @date    17-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_MENU                \
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
#define GUI_MENU_DEFAULT_BORDER_COLOR  PACKED_UINT_FLOAT(0.5f, 0.5f, 0.5f, 1.0f)  /* vec4(vec3(0.5f), 1.0f) */


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static CMenu *active_menu = NULL;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


typedef struct {
  char *lable;
  CMenu *menu;
} MenuEntry;

struct CMenu {
  /* Boolian flags. */
  bool text_refresh_needed    : 1;
  bool pos_refresh_needed     : 1;
  bool accept_on_tab          : 1;  /* If tab should act like enter when this menu is active. */
  bool width_refresh_needed   : 1;  /* When something has changed and the width of the menu must be recalculated. */
  bool width_is_static        : 1;  /* The width of the menu is static, this can only be set by `menu_set_static_width()`. */
  bool arrow_depth_navigation : 1;  /* Wether right and left arrows allows for closing and opening submenu's. */

  /* Configuration variables. */
  Uchar border_size;
  float width;

  vertex_buffer_t *buffer;
  Element         *element;
  CVec            *entries;
  int              viewtop;
  int              selected;
  int              maxrows;
  int              rows;
  Scrollbar       *sb;
  Font            *font;

  /* Used when this menu is a `submenu`, otherwise always `NULL`. */
  CMenu *parent;

  /* The currently open submenu, if any. */
  CMenu *active_submenu;

  /* Callback related data. */
  void            *data;  /* This ptr gets passed to all callbacks and should be passed to `menu_create()` as `data`. */
  MenuPositionFunc position_routine;
  MenuAcceptFunc   accept_routine;
};


/* ---------------------------------------------------------- MenuEntry function's ---------------------------------------------------------- */


/* Create a allocated `MenuEntry` structure with a `lable` and no menu ptr. */
static MenuEntry *menu_entry_create(const char *const restrict lable) {
  MenuEntry *me;
  MALLOC_STRUCT(me);
  me->lable = copy_of(lable);
  me->menu  = NULL;
  return me;
}

/* Free callback for a `MenuEntry`. */
static void menu_entry_free(void *arg) {
  ASSERT(arg);
  MenuEntry *me = (__TYPE(me))arg;
  free(me->lable);
  menu_free(me->menu);
  free(me);
}

static MenuEntry *menu_entry_create_with_menu(const char *const restrict lable, CMenu *const menu) {
  ASSERT(lable);
  ASSERT(menu);
  MenuEntry *me = menu_entry_create(lable);
  me->menu = menu;
  return me;
}


/* ---------------------------------------------------------- CMenu static function's ---------------------------------------------------------- */


static bool menu_selected_is_above_screen(CMenu *const menu) {
  ASSERT_MENU;
  return (menu->selected < menu->viewtop);
}

static bool menu_selected_is_below_screen(CMenu *const menu) {
  ASSERT_MENU;
  return (menu->selected >= (menu->viewtop + menu->rows));
}

_UNUSED static bool menu_selected_is_off_screen(CMenu *const menu) {
  ASSERT_MENU;
  return (menu_selected_is_above_screen(menu) || menu_selected_is_below_screen(menu));
}

/* The scrollbar update routine for the `CMenu` structure. */
static void menu_scrollbar_update_routine(void *arg, float *total_length, Uint *start, Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset) {
  ASSERT(arg);
  CMenu *menu = arg;
  ASSIGN_IF_VALID(total_length, (menu->element->height - (menu->border_size * 2)));
  ASSIGN_IF_VALID(start, 0);
  ASSIGN_IF_VALID(total, cvec_len(menu->entries) - menu->rows);
  ASSIGN_IF_VALID(visible, menu->rows);
  ASSIGN_IF_VALID(current, menu->viewtop);
  ASSIGN_IF_VALID(top_offset, menu->border_size);
  ASSIGN_IF_VALID(right_offset, menu->border_size);
}

/* The scrollbar moving routine for the `CMenu` structure. */
static void menu_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  CMenu *menu = arg;
  menu->viewtop = lclamp(index, 0, (cvec_len(menu->entries) - menu->rows));
}

/* TODO: Make the menu tall enough so that the scrollbar gets corrently initilazed before first use. */
static void menu_scrollbar_create(CMenu *const menu) {
  ASSERT(menu);
  ASSERT(menu->element);
  menu->sb = scrollbar_create(menu->element, menu, menu_scrollbar_update_routine, menu_scrollbar_moving_routine);
}

/* Return's the `lable` of the entry at `index`. */
static char *menu_get_entry_lable(CMenu *const menu, int index) {
  ASSERT_MENU;
  return ((MenuEntry *)cvec_get(menu->entries, index))->lable;
}

/* Return's the `menu` of the entry at `index`. */
static CMenu *menu_get_entry_menu(CMenu *const menu, int index) {
  ASSERT_MENU;
  return ((MenuEntry *)cvec_get(menu->entries, index))->menu;
}

/* Assigns the global absolute y top and bottom position as well as the most right allowed x position. */
static void menu_event_bounds(CMenu *const menu, float *const top, float *const bot, float *const right) {
  ASSERT_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* Top of the menu. */
    ASSIGN_IF_VALID(top, (menu->element->y + menu->border_size));
    /* Bottom of the menu. */
    ASSIGN_IF_VALID(bot, ((menu->element->y + menu->border_size) + (menu->element->height - menu->border_size)));
    /* The right most allowed position to register a event. */
    ASSIGN_IF_VALID(right, ((menu->element->x + menu->border_size) + (menu->element->width - menu->border_size) - ((len > menu->maxrows) ? scrollbar_width(menu->sb) : 0)));
  }
}

/* Reset's the state of menu and all its children recursivly. */
static void menu_reset(CMenu *const menu) {
  ASSERT_MENU;
  if (menu->active_submenu) {
    menu_reset(menu->active_submenu);
  }
  /* Ensure this menu gets fully updated. */
  menu->text_refresh_needed = TRUE;
  menu->pos_refresh_needed  = TRUE;
  /* Also, reset the menu to the starting state. */
  menu->viewtop  = 0;
  menu->selected = 0;
  /* And tell the scrollbar it needs to be updated. */
  scrollbar_refresh_needed(menu->sb);
}

static void menu_show_internal(CMenu *const menu, bool show) {
  ASSERT_MENU;
  if (show) {
    menu->element->hidden = FALSE;
    menu_reset(menu);
  }
  else {
    menu->element->hidden = TRUE;
    if (menu->active_submenu) {
      menu_show_internal(menu->active_submenu, FALSE);
      menu->active_submenu = NULL;
    }
  }
  scrollbar_show(menu->sb, show);
}

static bool menu_selected_is_visible(CMenu *const menu) {
  ASSERT_MENU;
  int row = (menu->selected - menu->viewtop);
  return (row >= 0 && row < menu->maxrows);
}

static void menu_check_submenu(CMenu *const menu) {
  ASSERT_MENU;
  CMenu *submenu;
  if (menu_selected_is_visible(menu) && (submenu = menu_get_entry_menu(menu, menu->selected))) {
    if (menu->active_submenu && submenu != menu->active_submenu) {
      menu_show_internal(menu->active_submenu, FALSE);
    }
    else {
      menu_show_internal(submenu, TRUE);
      menu->active_submenu = submenu;
    }
  }
  /* The currently selected row is outside the visible rows. */
  else if (menu->active_submenu) {
    menu_show_internal(menu->active_submenu, FALSE);
    menu->active_submenu = NULL;
  }
}

static float menu_calculate_width(CMenu *const menu) {
  ASSERT_MENU;
  int len = cvec_len(menu->entries);
  int longest_index;
  Ulong longest_string;
  Ulong value;
  if (len && !menu->width_is_static && menu->width_refresh_needed) {
    longest_index = 0;
    longest_string = strlen(menu_get_entry_lable(menu, 0));
    for (int i=1; i<len; ++i) {
      if ((value = strlen(menu_get_entry_lable(menu, i))) > longest_string) {
        longest_index = i;
        longest_string = value;
      }
    }
    menu->width = (font_breadth(menu->font, menu_get_entry_lable(menu, longest_index)) + (menu->border_size * 2) + 2);
    menu->width_refresh_needed = FALSE;
  }
  return menu->width;
}

static void menu_resize(CMenu *const menu) {
  ASSERT_MENU;
  float x;
  float y;
  float width;
  float height;
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
    width  = menu_calculate_width(menu);
    height = ((menu->rows * gui_font_height(menu->font)) + (menu->border_size * 2) + 2);
    /* Add the size of the scrollbar if there is one. */
    if (len > menu->maxrows) {
      width += scrollbar_width(menu->sb);
    }
    /* Get the wanted position based on the calculated size. */
    if (!menu->parent) {
      menu->position_routine(menu->data, width, height, &x, &y);
    }
    else {
      menu->position_routine(menu, width, height, &x, &y);
    }
    /* Move and resize the element. */
    element_move_resize(menu->element, x, y, width, height);
    menu->pos_refresh_needed = FALSE;
  }
}

static void menu_draw_selected(CMenu *const menu) {
  ASSERT_MENU;
  int row = (menu->selected - menu->viewtop);
  float x;
  float y;
  float width;
  float height;
  /* Draw the selected entry, if its on screen. */
  if (row >= 0 && row < menu->rows) {
    x     = (menu->element->x + menu->border_size);
    width = (menu->element->width - (menu->border_size * 2));
    gui_font_row_top_bot(menu->font, row, &y, &height);
    height -= (y - ((row == menu->rows - 1) ? 1 : 0));
    y      += (menu->element->y + menu->border_size);
    draw_rect_rgba(x, y, width, height, 1, 1, 1, 0.4f);
  }
}

static void menu_draw_text(CMenu *const menu) {
  ASSERT_MENU;
  static int was_viewtop = -1;
  // static Color color = {1, 1, 1, 1};
  int row = 0;
  char *str;
  float pen_x;
  float pen_y;
  /* Only clear and reconstruct the vertex buffer when asked or when the viewtop has changed. */
  if (menu->text_refresh_needed || was_viewtop != menu->viewtop) {
    vertex_buffer_clear(menu->buffer);
    while (row < menu->rows) {
      str   = menu_get_entry_lable(menu, (menu->viewtop + row));
      pen_x = (menu->element->x + menu->border_size + 1);
      pen_y = (gui_font_row_baseline(menu->font, row) + menu->element->y + menu->border_size + 1);
      font_vertbuf_add_mbstr(menu->font, menu->buffer, str, strlen(str), NULL, PACKED_UINT(255, 255, 255, 255), &pen_x, &pen_y);
      ++row;
    }
    was_viewtop = menu->viewtop;
    menu->text_refresh_needed = FALSE;
  }
  render_vertbuf(menu->font, menu->buffer);
}

/* For a submenu the submenu itself needs to be passed as the routine. */
static void menu_submenu_pos_routine(void *arg, float width, float height, float *const x, float *const y) {
  ASSERT(arg);
  ASSERT(width);
  ASSERT(height);
  ASSERT(x);
  ASSERT(y);
  CMenu *menu = arg;
  int index = 0;
  /* Get the true index of this submenu entry. */
  while (((MenuEntry *)cvec_get(menu->parent->entries, index))->menu != menu) {
    ++index;
  }
  /* Then offset the index to the visible entries. */
  index -= menu->parent->viewtop;
  /* And always ensure it falls inside it, as this function should not be called otherwise. */
  ALWAYS_ASSERT(index >= 0 && index < menu->parent->maxrows);
  (*x) = (menu->parent->element->x + menu->parent->element->width);
  gui_font_row_top_bot(menu->font, index, y, NULL);
  (*y) += menu->parent->element->y;
}

static void menu_push_back_submenu(CMenu *const menu, const char *const restrict lable, CMenu *const submenu) {
  ASSERT_MENU;
  ASSERT(lable);
  ASSERT(submenu);
  cvec_push(menu->entries, menu_entry_create_with_menu(lable, submenu));
  menu->width_refresh_needed = TRUE;
}

static void menu_selected_up_internal(CMenu *const menu) {
  ASSERT_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* If we are at the first entry. */
    if (menu->selected == 0) {
      menu->selected = (len - 1);
      /* If there are more entries then visible rows, we also need to change the viewtop. */
      if (len > menu->maxrows) {
        menu->viewtop = (len - menu->maxrows);
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh_needed(menu->sb);
      }
    }
    /* Otherwise, we are anywhere below that. */
    else {
      /* If the currently selected is the current viewtop.  Decrease it by one. */
      if (menu->viewtop == menu->selected) {
        --menu->viewtop;
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh_needed(menu->sb);
      }
      else if (menu_selected_is_off_screen(menu)) {
        menu->viewtop = (menu->selected - 1);
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh_needed(menu->sb);
      }
      --menu->selected;
    }
    menu_check_submenu(menu);
  }
}

static void menu_selected_down_internal(CMenu *const menu) {
  ASSERT_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* If we are at the last entry. */
    if (menu->selected == (len - 1)) {
      menu->selected = 0;
      menu->viewtop  = 0;
      /* Only update the scrollbar when the viewtop has changed. */
      scrollbar_refresh_needed(menu->sb);
    }
    else {
      /* If the currently selected entry is the last visible entry, move the viewtop down by one. */
      if (menu->selected == (menu->viewtop + menu->maxrows - 1)) {
        ++menu->viewtop;
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh_needed(menu->sb);
      }
      /* Otherwise, if the currently selected entry if fully off screen, adjust the viewtop so that the selected is the last visible entry. */
      else if (menu_selected_is_off_screen(menu)) {
        menu->viewtop = fclamp(((menu->selected + 1) - menu->rows + 1), 0, (cvec_len(menu->entries) - menu->rows));
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh_needed(menu->sb);
      }
      ++menu->selected;
    }
    menu_check_submenu(menu);
  }
}

/* Used to exit a submenu when pressing left, this is used by `menu_exit_submenu()`. */
static void menu_exit_submenu_internal(CMenu *const menu) {
  ASSERT_MENU;
  if (menu->parent) {
    menu_show_internal(menu, FALSE);
    menu->parent->active_submenu = NULL;
  }
}


/* ---------------------------------------------------------- CMenu global function's ---------------------------------------------------------- */


/* Create a allocated `CMenu`. */
CMenu *menu_create(Element *const parent, Font *const font, void *data, MenuPositionFunc position_routine, MenuAcceptFunc accept_routine) {
  ASSERT(parent);
  ASSERT(font);
  ASSERT(data);
  ASSERT(position_routine);
  ASSERT(accept_routine);
  // Color border_color;
  CMenu *menu = xmalloc(sizeof(*menu));
  // color_set_default_borders(&border_color);
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
  menu->buffer  = vertbuf_create();
  /* Entries vector. */
  menu->entries = cvec_create_setfree(menu_entry_free);
  /* Create the element of the menu. */
  menu->element = element_create(100, 100, 100, 100, TRUE);
  // color_set_black(menu->element->color);
  menu->element->color = PACKED_UINT(0, 0, 0, 255);
  element_set_parent(menu->element, parent);
  // menu->element->color = GUI_BLACK_COLOR;  /* The default background color for menu's is black. */
  // menu->element->flag.set<GUIELEMENT_ABOVE>();
  // menu->element->flag.set<GUIELEMENT_HIDDEN>();
  menu->element->hidden = TRUE;
  /* As default all menus should have borders, to create a uniform look.  Note that this can be configured.  TODO: Implement the config of borders. */
  element_set_borders(menu->element, menu->border_size, menu->border_size, menu->border_size, menu->border_size, PACKED_UINT_DEFAULT_BORDERS);
  element_set_menu_data(menu->element, menu);
  /* Row init. */
  menu->viewtop  = 0;
  menu->selected = 0;
  menu->maxrows  = GUI_MENU_DEFAULT_MAX_ROWS;
  menu->rows     = 0;
  menu_scrollbar_create(menu);
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

CMenu *menu_create_submenu(CMenu *const parent, const char *const restrict lable, void *data, MenuAcceptFunc accept_routine) {
  ASSERT(parent);
  ASSERT(data);
  ASSERT(accept_routine);
  CMenu *menu = menu_create(parent->element, parent->font, data, menu_submenu_pos_routine, accept_routine);
  menu->parent = parent;
  menu_push_back_submenu(parent, lable, menu);
  return menu;
}

void menu_free(CMenu *const menu) {
  if (!menu) {
    return;
  }
  vertex_buffer_delete(menu->buffer);
  cvec_free(menu->entries);
  free(menu->sb);
  free(menu);
}

CMenu *menu_get_active(void) {
  return active_menu;
}

/* Perform a draw call for a `CMenu`.  Note that this should be called every frame for all root menu's and never for submenu's. */
void menu_draw(CMenu *const menu) {
  ASSERT_MENU;
  CMenu *submenu;
  /* Only draw the suggestmenu if there are any available suggestions. */
  if (!menu->element->hidden && cvec_len(menu->entries)) {
    menu_resize(menu);
    /* Draw the main element of the suggestmenu. */
    element_draw(menu->element);
    /* Highlight the selected entry in the suggestmenu when its on the screen. */
    menu_draw_selected(menu);
    /* Draw the scrollbar of the suggestmenu. */
    scrollbar_draw(menu->sb);
    /* Draw the text of the suggestmenu entries. */
    menu_draw_text(menu);
    /*  */
    for (int i=0; i<cvec_len(menu->entries); ++i) {
      if ((submenu = menu_get_entry_menu(menu, i))) {
        menu_draw(submenu);
      }
    }
  }
}

void menu_push_back(CMenu *const menu, const char *const restrict string) {
  ASSERT_MENU;
  ASSERT(string);
  cvec_push(menu->entries, menu_entry_create(string));
  menu->width_refresh_needed = TRUE;
}

void menu_pos_refresh_needed(CMenu *const menu) {
  ASSERT_MENU;
  menu->pos_refresh_needed = TRUE;
}

void menu_text_refresh_needed(CMenu *const menu) {
  ASSERT_MENU;
  menu->text_refresh_needed = TRUE;
}

void menu_scrollbar_refresh_needed(CMenu *const menu) {
  ASSERT_MENU;
  scrollbar_refresh_needed(menu->sb);
}

void menu_show(CMenu *const menu, bool show) {
  ASSERT_MENU;
  /* Showing this menu. */
  if (show) {
    /* Always close the currently active menu, even when its the same as `menu`.  This ensures correctness related to submenus. */
    if (active_menu) {
      menu_show_internal(active_menu, FALSE);
    }
    menu_show_internal(menu, TRUE);
    active_menu = menu;
  }
  /* Hiding this menu. */
  else {
    if (active_menu) {
      menu_show_internal(active_menu, FALSE);
      active_menu = NULL;
    }
    menu_show_internal(menu, FALSE);
  }
}

void menu_selected_up(CMenu *const menu) {
  ASSERT_MENU;
  /* Recursivly call this function, until we reach the bottom. */
  if (menu->active_submenu) {
    menu_selected_up(menu->active_submenu);
  }
  /* Then call the internal function that performs the action. */
  else {
    menu_selected_up_internal(menu);
  }
}

void menu_selected_down(CMenu *const menu) {
  ASSERT_MENU;
  /* Recursivly call this function, until we reach the bottom. */
  if (menu->active_submenu) {
    menu_selected_down(menu->active_submenu);
  }
  /* Then call the internal function that performs the action. */
  else {
    menu_selected_down_internal(menu);
  }
}

void menu_exit_submenu(CMenu *const menu) {
  ASSERT_MENU;
  if (menu->active_submenu) {
    menu_exit_submenu(menu->active_submenu);
  }
  else {
    menu_exit_submenu_internal(menu);
  }
}

void menu_enter_submenu(CMenu *const menu) {
  ASSERT_MENU;
  if (menu->active_submenu) {
    menu_enter_submenu(menu->active_submenu);
  }
  else {
    menu_check_submenu(menu);
  }
}

/* This is used to perform the accept action of the depest opened menu's currently selected entry,
 * or if that selected entry has a submenu and its not open, then it will open it.  This is used
 * for both clicking and kb related execution of the accept routine for that menu. */
void menu_accept_action(CMenu *const menu) {
  ASSERT_MENU;
  /* As a sanity check only perform any action when the menu is not empty. */
  if (cvec_len(menu->entries)) {
    /* If the currently selected entry of `menu` does not have a submenu, call the accept routine for `menu`. */
    if (!menu_get_entry_menu(menu, menu->selected)) {
      /* Run the user's accept routine. */
      menu->accept_routine(menu->data, menu_get_entry_lable(menu, menu->selected), menu->selected);
      /* Then stop showing the menu. */
      menu_show(menu, FALSE);
    }
    /* Otherwise, if the entry has a submenu but its not currently open, open it. */
    else if (!menu->active_submenu) {
      menu_check_submenu(menu);
    }
    /* And if that menu is currently active, recursivly call this function on that menu. */
    else {
      menu_accept_action(menu->active_submenu);
    }
  }
}

void menu_hover_action(CMenu *const menu, float x_pos, float y_pos) {
  ASSERT_MENU;
  long row;
  float top;
  float bot;
  float right;
  /* Only perform any action when there are entries in the menu. */
  if (cvec_len(menu->entries)) {
    /* Get the absolute values where events are allowed. */
    menu_event_bounds(menu, &top, &bot, &right);
    /* If `y_pos` relates to a valid row in the suggestmenu completion menu, then adjust the selected to that row. */
    if (gui_font_row_from_pos(menu->font, top, bot, y_pos, &row) && x_pos < right) {
      menu->selected = lclamp((menu->viewtop + row), menu->viewtop, (menu->viewtop + menu->rows - 1));
      menu_check_submenu(menu);
    }
  }
}

void menu_scroll_action(CMenu *const menu, bool direction, float x_pos, float y_pos) {
  ASSERT_MENU;
  float top;
  float bot;
  float right;
  int len = cvec_len(menu->entries);
  /* Only do anything if there are more entries then rows in the menu then maximum number of rows allowed. */
  if (len > menu->maxrows) {
    /* Get the absolute values where events are allowed. */
    menu_event_bounds(menu, &top, &bot, &right);
    /* Only scroll when not already at the top or bottom.  And the call was made from a valid position. */
    if (y_pos >= top && y_pos <= bot && x_pos < right && ((direction == BACKWARD && menu->viewtop > 0) || (direction == FORWARD && menu->viewtop < (len - menu->maxrows)))) {
      menu->viewtop += (!direction ? -1 : 1);
      menu->text_refresh_needed = TRUE;
      /* Ensure that the currently selected entry gets correctly set based on where the mouse is. */
      menu_hover_action(menu, x_pos, y_pos);
      scrollbar_refresh_needed(menu->sb);
    }
  }
}

void menu_click_action(CMenu *const menu, float x_pos, float y_pos) {
  ASSERT_MENU;
  float top;
  float bot;
  float right;
  /* Only perform any action when there are entries in the menu. */
  if (cvec_len(menu->entries)) {
    /* Get the absolute values where events are allowed. */
    menu_event_bounds(menu, &top, &bot, &right);
    if (y_pos >= top && y_pos <= bot && x_pos < right) {
      menu_hover_action(menu, x_pos, y_pos);
      /* Only allow clickes on non submenu entries. */
      if (!menu_get_entry_menu(menu, menu->selected)) {
        menu_accept_action(menu);
      }
    }
  }
}

void menu_clear_entries(CMenu *const menu) {
  ASSERT_MENU;
  cvec_clear(menu->entries);
  menu->viewtop  = 0;
  menu->selected = 0;
  menu->text_refresh_needed  = TRUE;
  menu->pos_refresh_needed   = TRUE;
  menu->width_refresh_needed = TRUE;
}

/* Set a static width for `menu`.  TODO: Implement this into text drawing function, so that when there is not enough room it cuts of the entry. */
void menu_set_static_width(CMenu *const menu, float width) {
  ASSERT_MENU;
  ALWAYS_ASSERT_MSG((width > 0.0f), "The width of a menu must be positive");
  menu->width = width;
  menu->width_is_static = TRUE;
}

/* Configure's the tab behavior for `menu`, if `accept_on_tab` is `TRUE` then tab will act like enter when this menu is active. */
void menu_set_tab_accept_behavior(CMenu *const menu, bool accept_on_tab) {
  ASSERT_MENU;
  menu->accept_on_tab = accept_on_tab;
}

/* Configure's if `menu` should use right arrow to open the submenu at the currently selected entry if it exists and left arrow to close the currently open submenu. */
void menu_set_arrow_depth_navigation(CMenu *const menu, bool enable_arrow_depth_navigation) {
  ASSERT_MENU;
  menu->arrow_depth_navigation = enable_arrow_depth_navigation;
}

/* ----------------------------- Boolian function's ----------------------------- */

/* Return's `TRUE` if `e` is part of `menu`. */
bool menu_owns_element(CMenu *const menu, Element *const e) {
  ASSERT_MENU;
  return element_is_ancestor(e, menu->element);
}

/* Return's `TRUE` if `e` is the main element of `menu`. */
bool menu_element_is_main(CMenu *const menu, Element *const e) {
  ASSERT_MENU;
  ASSERT(e);
  return (menu->element == e);
}

/* Return's `TRUE` if `menu` has `accept_on_tab` flag set. */
bool menu_should_accept_on_tab(CMenu *const menu) {
  ASSERT_MENU;
  return menu->accept_on_tab;
}

/* Return's `TRUE` if `menu` has `arrow_depth_navigation` flag set.  This means that `left` and `right arrow` close's and open's `submenu's`. */
bool menu_allows_arrow_navigation(CMenu *const menu) {
  ASSERT_MENU;
  return menu->arrow_depth_navigation;
}

/* Return's `TRUE` when `ancestor` is an ancestor to `menu`. */
bool menu_is_ancestor(CMenu *const menu, CMenu *const ancestor) {
  ASSERT(menu);
  ASSERT(ancestor);
  CMenu *m = menu;
  while (m) {
    if (m == ancestor) {
      return TRUE;
    }
    m = m->parent;
  }
  return FALSE;
}

/* Return's `TRUE` when `menu` is currently being shown and has more then zero entries. */
bool menu_is_shown(CMenu *const menu) {
  ASSERT_MENU;
  return (!menu->element->hidden && cvec_len(menu->entries));
}

/* ----------------------------- Getter function's ----------------------------- */

Font *menu_get_font(CMenu *const menu) {
  ASSERT_MENU;
  return menu->font;
}

int menu_len(CMenu *const menu) {
  ASSERT_MENU;
  return cvec_len(menu->entries);
}

/* ----------------------------- CMenu qsort callback's ----------------------------- */

/* Function callback used for sorting menu entries by length of lable.  Note that this should be passed to `menu_qsort()` */
int menu_entry_qsort_strlen_cb(const void *a, const void *b) {
  const MenuEntry *lhs = *(const MenuEntry **)a;
  const MenuEntry *rhs = *(const MenuEntry **)b;
  long lhs_len = strlen(lhs->lable);
  long rhs_len = strlen(rhs->lable);
  if (lhs_len == rhs_len) {
    return strcmp(lhs->lable, rhs->lable);
  }
  return (lhs_len - rhs_len); 
}

/* ----------------------------- CMenu qsort call ----------------------------- */

void menu_qsort(CMenu *const menu, CmpFuncPtr cmp_func) {
  ASSERT_MENU;
  cvec_qsort(menu->entries, cmp_func);
}

