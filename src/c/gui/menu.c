/** @file gui/menu.c

  @author  Melwin Svensson.
  @date    17-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_MENU                        \
  /* Internal whole struct validation. */  \
  ASSERT(menu);                            \
  ASSERT(menu->buffer);                    \
  ASSERT(menu->element);                   \
  ASSERT(menu->entries);                   \
  ASSERT(menu->sb);                        \
  ASSERT(menu->font);                      \
  ASSERT(menu->data);                      \
  ASSERT(menu->position_routine);          \
  ASSERT(menu->accept_routine)

#define MENU_DEFAULT_BORDER_SIZE     1
#define MENU_DEFAULT_LABLE_OFFSET    0
#define MENU_DEFAULT_MAX_ROWS        8
#define MENU_DEFAULT_BORDER_COLOR    PACKED_UINT_FLOAT(0.5f, 0.5f, 0.5f, 1.0f)
#define MENU_DEFAULT_SELECTED_COLOR  PACKED_UINT_FLOAT(1.f, 1.f, 1.f, .4f)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The currently active menu, as we enforce only a single active menu at all times. */
static Menu *active_menu = NULL;


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


typedef enum {
  MENU_REFRESH_TEXT = (1 << 0),
  MENU_REFRESH_POS  = (1 << 1),
  /* When something has changed and the width of the menu must be recalculated. */
  MENU_REFRESH_WIDTH = (1 << 2),
  /* If tab should act like enter when this menu is active. */
  MENU_ACCEPT_ON_TAB = (1 << 3),
  /* The width of the menu is static, this can only be set by `menu_set_static_width()`. */
  MENU_WIDTH_IS_STATIC = (1 << 4),
  /* Whether right and left arrows allows for closing and opening submenu's. */
  MENU_ARROW_DEPTH_NAVIGATION = (1 << 5),
  /* Defines. */
# define MENU_REFRESH_TEXT            MENU_REFRESH_TEXT
# define MENU_REFRESH_POS             MENU_REFRESH_POS
# define MENU_REFRESH_WIDTH           MENU_REFRESH_WIDTH
# define MENU_ACCEPT_ON_TAB           MENU_ACCEPT_ON_TAB
# define MENU_WIDTH_IS_STATIC         MENU_WIDTH_IS_STATIC
# define MENU_ARROW_DEPTH_NAVIGATION  MENU_ARROW_DEPTH_NAVIGATION
# define MENU_HAS_LABLE_OFFSET        MENU_HAS_LABLE_OFFSET
  /* The Default flags used for initilizing. */
# define MENU_XFLAGS_DEFAULT (MENU_REFRESH_TEXT | MENU_REFRESH_POS | MENU_REFRESH_WIDTH | MENU_ARROW_DEPTH_NAVIGATION)
} MenuFlag;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


typedef struct {
  /* When this menu-entry is a singular clickable thing, this is its name. */
  char *lable;
  /* When this menu-entry is a submenu. */
  Menu *menu;
} MenuEntry;

struct Menu {
  /* State flags. */
  int xflags;

  /* Configuration variables. */
  Uchar border_size;
  float width;
  Ushort lable_offset;

  vertex_buffer_t *buffer;

  Element *element;

  /* The element representing the selected rect to be drawn.  TODO: Maybe add a way to add additional rects to an element...? */
  Element *selelem;
  
  CVec *entries;

  /* What the viewtop was before the last update to it. */
  int was_viewtop;

  int viewtop;
  int selected;
  int maxrows;
  int rows;

  /* The scrollbar of this menu. */
  Scrollbar *sb;

  /* The font this menu is using.  Note that this is just a ptr to either the text or ui font (for now). */
  Font *font;

  /* Used when this menu is a `submenu`, otherwise always `NULL`. */
  Menu *parent;

  /* The currently open submenu, if any. */
  Menu *active_submenu;

  /* This ptr gets passed to all callbacks and should be passed to `menu_create()` as `data`. */
  void *data;

  /* Callback's. */
  MenuPositionFunc position_routine;
  MenuAcceptFunc   accept_routine;
};


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Menu entry create ----------------------------- */

/* Create a allocated `MenuEntry` structure with a `lable` and no menu ptr. */
static MenuEntry *menu_entry_create(const char *const restrict lable) {
  MenuEntry *me = xmalloc(sizeof(*me));
  me->lable = copy_of(lable);
  me->menu  = NULL;
  return me;
}

/* ----------------------------- Menu entry free ----------------------------- */

/* Free callback for a `MenuEntry`. */
static void menu_entry_free(void *arg) {
  ASSERT(arg);
  MenuEntry *me = arg;
  free(me->lable);
  menu_free(me->menu);
  free(me);
}

/* ----------------------------- Menu entry create with menu ----------------------------- */

static MenuEntry *menu_entry_create_with_menu(const char *const restrict lable, Menu *const menu) {
  ASSERT(lable);
  ASSERT(menu);
  MenuEntry *me = menu_entry_create(lable);
  me->menu = menu;
  return me;
}

/* ----------------------------- Menu selected is above screen ----------------------------- */

static inline bool menu_selected_is_above_screen(Menu *const menu) {
  ASSERT_MENU;
  return (menu->selected < menu->viewtop);
}

/* ----------------------------- Menu selected is below screen ----------------------------- */

static inline bool menu_selected_is_below_screen(Menu *const menu) {
  ASSERT_MENU;
  return (menu->selected >= (menu->viewtop + menu->rows));
}

/* ----------------------------- Menu selected is off screen ----------------------------- */

static inline bool menu_selected_is_off_screen(Menu *const menu) {
  ASSERT_MENU;
  return (menu_selected_is_above_screen(menu) || menu_selected_is_below_screen(menu));
}

/* ----------------------------- Menu scrollbar update routine ----------------------------- */

/* The scrollbar update routine for the `Menu` structure. */
static void menu_scrollbar_update_routine(void *arg, float *total_length, Uint *start,
  Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset)
{
  ASSERT(arg);
  Menu *menu = arg;
  ASSIGN_IF_VALID(total_length, (menu->element->height - (menu->border_size * 2)));
  ASSIGN_IF_VALID(start, 0);
  ASSIGN_IF_VALID(total, cvec_len(menu->entries) - menu->rows);
  ASSIGN_IF_VALID(visible, menu->rows);
  ASSIGN_IF_VALID(current, menu->viewtop);
  ASSIGN_IF_VALID(top_offset, menu->border_size);
  ASSIGN_IF_VALID(right_offset, menu->border_size);
}

/* ----------------------------- Menu scrollbar moving routine ----------------------------- */

/* The scrollbar moving routine for the `Menu` structure. */
static void menu_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  Menu *menu = arg;
  menu->viewtop = lclamp(index, 0, (cvec_len(menu->entries) - menu->rows));
}

/* ----------------------------- Menu scrollbar create ----------------------------- */

/* TODO: Make the menu tall enough so that the scrollbar gets corrently initilazed before first use. */
static void menu_scrollbar_create(Menu *const menu) {
  ASSERT(menu);
  ASSERT(menu->element);
  menu->sb = scrollbar_create(menu->element, menu, menu_scrollbar_update_routine, menu_scrollbar_moving_routine);
}

/* ----------------------------- Menu get entry lable ----------------------------- */

/* Return's the `lable` of the entry at `index`. */
static char *menu_get_entry_lable(Menu *const menu, int index) {
  ASSERT_MENU;
  return ((MenuEntry *)cvec_get(menu->entries, index))->lable;
}

/* ----------------------------- Menu get entry menu ----------------------------- */

/* Return's the `menu` of the entry at `index`. */
static Menu *menu_get_entry_menu(Menu *const menu, int index) {
  ASSERT_MENU;
  return ((MenuEntry *)cvec_get(menu->entries, index))->menu;
}

/* ----------------------------- Menu event bounds ----------------------------- */

/* Assigns the global absolute y top and bottom position as well as the most right allowed x position. */
static void menu_event_bounds(Menu *const menu, float *const top, float *const bot, float *const right) {
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

/* ----------------------------- Menu reset ----------------------------- */

/* Reset's the state of menu and all its children recursivly. */
static void menu_reset(Menu *const menu) {
  ASSERT_MENU;
  if (menu->active_submenu) {
    menu_reset(menu->active_submenu);
  }
  /* Ensure this menu gets fully updated. */
  menu->xflags |= (MENU_REFRESH_TEXT | MENU_REFRESH_POS);
  /* Also, reset the menu to the starting state. */
  menu->viewtop  = 0;
  menu->selected = 0;
  /* And tell the scrollbar it needs to be updated. */
  scrollbar_refresh(menu->sb);
}

/* ----------------------------- Menu show internal ----------------------------- */

static void menu_show_internal(Menu *const menu, bool show) {
  ASSERT_MENU;
  if (show) {
    menu->element->xflags &= ~ELEMENT_HIDDEN;
    /* TODO: Figure out if only resetting when hiding works for all things. */
    // menu_reset(menu);
  }
  else {
    menu->element->xflags |= ELEMENT_HIDDEN;
    if (menu->active_submenu) {
      menu_show_internal(menu->active_submenu, FALSE);
      menu->active_submenu = NULL;
    }
    menu_reset(menu);
  }
  scrollbar_show(menu->sb, show);
  refresh_needed = TRUE;
}

/* ----------------------------- Menu selected is visible ----------------------------- */

static bool menu_selected_is_visible(Menu *const menu) {
  ASSERT_MENU;
  int row = (menu->selected - menu->viewtop);
  return (row >= 0 && row < menu->maxrows);
}

/* ----------------------------- Menu check submenu ----------------------------- */

static void menu_check_submenu(Menu *const menu) {
  ASSERT_MENU;
  Menu *submenu;
  if (menu_selected_is_visible(menu) && (submenu = menu_get_entry_menu(menu, menu->selected))) {
    /* No currently open submenu. */
    if (!menu->active_submenu) {
      menu_show_internal(submenu, TRUE);
      menu->active_submenu = submenu;
    }
    /* The currently open submenu is not the currently selected submenu. */
    else if (menu->active_submenu != submenu) {
      menu_show_internal(menu->active_submenu, FALSE);
      menu_show_internal(submenu, TRUE);
      menu->active_submenu = submenu;
    }
    /* The currently selected submenu is the currently open submenu. */

    /* TODO: Here i think we need to show and assign the correct submenu. */
    // if (menu->active_submenu && submenu != menu->active_submenu) {
    //   menu_show_internal(menu->active_submenu, FALSE);
    // }
    // else {
    //   menu_show_internal(submenu, TRUE);
    //   menu->active_submenu = submenu;
    // }
  }
  /* The currently selected row is outside the visible rows. */
  else if (menu->active_submenu) {
    menu_show_internal(menu->active_submenu, FALSE);
    menu->active_submenu = NULL;
  }
}

/* ----------------------------- Menu calculate width ----------------------------- */

static float menu_calculate_width(Menu *const menu) {
  ASSERT_MENU;
  int len = cvec_len(menu->entries);
  int longest_index;
  Ulong longest_string;
  Ulong value;
  if (len && !(menu->xflags & MENU_WIDTH_IS_STATIC) && (menu->xflags & MENU_REFRESH_WIDTH)) {
    longest_index = 0;
    longest_string = strlen(menu_get_entry_lable(menu, 0));
    for (int i=1; i<len; ++i) {
      if ((value = strlen(menu_get_entry_lable(menu, i))) > longest_string) {
        longest_index = i;
        longest_string = value;
      }
    }
    menu->width = (font_breadth(menu->font, menu_get_entry_lable(menu, longest_index)) + (menu->border_size * 2) + 2);
    /* When the menu has more entries then rows, this means we need to add width for the scrollbar to fit. */
    if (len > menu->maxrows) {
      menu->width += scrollbar_width(menu->sb);
    }
    menu->xflags &= ~MENU_REFRESH_WIDTH;
  }
  return menu->width;
}

/* ----------------------------- Menu resize ----------------------------- */

static void menu_resize(Menu *const menu) {
  ASSERT_MENU;
  float x;
  float y;
  float width;
  float height;
  int len = cvec_len(menu->entries);
  /* If there are entries in the menu or we need to recalculate the position. */
  if (len && (menu->xflags & MENU_REFRESH_POS)) {
    /* Set the number of visable rows. */
    if (len > menu->maxrows) {
      menu->rows = menu->maxrows;
    }
    else {
      menu->rows = len;
    }
    /* Calculate the size of the suggestmenu window. */
    width  = menu_calculate_width(menu);
    height = ((menu->rows * font_height(menu->font)) + (menu->border_size * 2) + 2);
    /* Get the wanted position based on the calculated size. */
    if (!menu->parent) {
      menu->position_routine(menu->data, width, height, &x, &y);
    }
    else {
      menu->position_routine(menu, width, height, &x, &y);
    }
    /* Move and resize the element. */
    element_move_resize(menu->element, x, y, width, height);
    menu->xflags &= ~MENU_REFRESH_POS;
  }
}

/* ----------------------------- Menu draw selected ----------------------------- */

static void menu_draw_selected(Menu *const menu) {
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
    font_row_top_bot(menu->font, row, &y, &height);
    height -= (y - ((row == menu->rows - 1) ? 1 : 0));
    y      += (menu->element->y + menu->border_size);
    menu->selelem->xflags &= ~ELEMENT_HIDDEN;
    element_move_resize(menu->selelem, x, y, width, height);
    element_draw(menu->selelem);
  }
  else {
    menu->selelem->xflags |= ELEMENT_HIDDEN;
  }
}

/* ----------------------------- Menu draw text ----------------------------- */

static void menu_draw_text(Menu *const menu) {
  ASSERT_MENU;
  int row = 0;
  char *str;
  float pen_x;
  float pen_y;
  /* Only clear and reconstruct the vertex buffer when asked or when the viewtop has changed. */
  if ((menu->xflags & MENU_REFRESH_TEXT) || menu->was_viewtop != menu->viewtop) {
    vertex_buffer_clear(menu->buffer);
    while (row < menu->rows) {
      pen_x = ((menu->element->x + menu->border_size + 1) + menu->lable_offset);
      pen_y = (font_row_baseline(menu->font, row) + menu->element->y + menu->border_size + 1);
      str   = menu_get_entry_lable(menu, (menu->viewtop + row));
      font_vertbuf_add_mbstr(menu->font, menu->buffer, str, strlen(str), NULL, PACKED_UINT(255, 255, 255, 255), &pen_x, &pen_y);
      ++row;
    }
    menu->was_viewtop = menu->viewtop;
    menu->xflags &= ~MENU_REFRESH_TEXT;
  }
  render_vertbuf(menu->font, menu->buffer);
}

/* ----------------------------- Menu submenu pos routine ----------------------------- */

/* For a submenu the submenu itself needs to be passed as the routine. */
static void menu_submenu_pos_routine(void *arg, float width, float height, float *const x, float *const y) {
  ASSERT(arg);
  ASSERT(width);
  ASSERT(height);
  ASSERT(x);
  ASSERT(y);
  Menu *menu = arg;
  int index = 0;
  /* Get the true index of this submenu entry. */
  while (((MenuEntry *)cvec_get(menu->parent->entries, index))->menu != menu) {
    ++index;
  }
  /* Then offset the index to the visible entries. */
  index -= menu->parent->viewtop;
  writef("index: %d\n", index);
  writef("menu->parent->maxrows: %d\n", menu->parent->maxrows);
  writef("menu->parent->viewtop: %d\n", menu->parent->viewtop);
  /* And always ensure it falls inside it, as this function should not be called otherwise. */
  ALWAYS_ASSERT(index >= 0 && index < menu->parent->maxrows);
  (*x) = (menu->parent->element->x + menu->parent->element->width);
  font_row_top_bot(menu->font, index, y, NULL);
  (*y) += menu->parent->element->y;
}

/* ----------------------------- Menu push back submenu ----------------------------- */

static void menu_push_back_submenu(Menu *const menu, const char *const restrict lable, Menu *const submenu) {
  ASSERT_MENU;
  ASSERT(lable);
  ASSERT(submenu);
  cvec_push(menu->entries, menu_entry_create_with_menu(lable, submenu));
  menu->xflags |= MENU_REFRESH_WIDTH;
}

/* ----------------------------- Menu selected up internal ----------------------------- */

static void menu_selected_up_internal(Menu *const menu) {
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
        scrollbar_refresh(menu->sb);
      }
    }
    /* Otherwise, we are anywhere below that. */
    else {
      /* If the currently selected is the current viewtop.  Decrease it by one. */
      if (menu->viewtop == menu->selected) {
        --menu->viewtop;
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh(menu->sb);
      }
      else if (menu_selected_is_off_screen(menu)) {
        menu->viewtop = (menu->selected - 1);
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh(menu->sb);
      }
      --menu->selected;
    }
    menu_check_submenu(menu);
    refresh_needed = TRUE;
  }
}

/* ----------------------------- Menu selected down internal ----------------------------- */

static void menu_selected_down_internal(Menu *const menu) {
  ASSERT_MENU;
  int len = cvec_len(menu->entries);
  if (len) {
    /* If we are at the last entry. */
    if (menu->selected == (len - 1)) {
      menu->selected = 0;
      menu->viewtop  = 0;
      /* Only update the scrollbar when the viewtop has changed. */
      scrollbar_refresh(menu->sb);
    }
    else {
      /* If the currently selected entry is the last visible entry, move the viewtop down by one. */
      if (menu->selected == (menu->viewtop + menu->maxrows - 1)) {
        ++menu->viewtop;
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh(menu->sb);
      }
      /* Otherwise, if the currently selected entry if fully off screen, adjust the viewtop so that the selected is the last visible entry. */
      else if (menu_selected_is_off_screen(menu)) {
        menu->viewtop = fclamp(((menu->selected + 1) - menu->rows + 1), 0, (cvec_len(menu->entries) - menu->rows));
        /* Only update the scrollbar when the viewtop has changed. */
        scrollbar_refresh(menu->sb);
      }
      ++menu->selected;
    }
    menu_check_submenu(menu);
    refresh_needed = TRUE;
  }
}

/* ----------------------------- Menu exit submenu internal ----------------------------- */

/* Used to exit a submenu when pressing left, this is used by `menu_exit_submenu()`. */
static void menu_exit_submenu_internal(Menu *const menu) {
  ASSERT_MENU;
  if (menu->parent) {
    menu_show_internal(menu, FALSE);
    menu->parent->active_submenu = NULL;
  }
}


/* ---------------------------------------------------------- Menu global function's ---------------------------------------------------------- */


/* ----------------------------- Menu create ----------------------------- */

/* Create a `root-menu`.  Note that all passed argument's must be valid. */
Menu *menu_create(Element *const parent, Font *const font, void *data,
  MenuPositionFunc position_routine, MenuAcceptFunc accept_routine)
{
  ASSERT(parent);
  ASSERT(font);
  ASSERT(data);
  ASSERT(position_routine);
  ASSERT(accept_routine);
  Menu *menu = xmalloc(sizeof *menu);
  menu->xflags = MENU_XFLAGS_DEFAULT;
  /* Configuration variables. */
  menu->border_size = MENU_DEFAULT_BORDER_SIZE;
  menu->width = 0.0f;
  menu->lable_offset = MENU_DEFAULT_LABLE_OFFSET;
  /* Vertex buffer. */
  menu->buffer = vertex_buffer_new(FONT_VERTBUF);
  /* Entries vector. */
  menu->entries = cvec_create_setfree(menu_entry_free);
  /* Create the element of the menu. */
  menu->element = element_create(100, 100, 100, 100, TRUE);
  /* The default background color for menu's is black. */
  menu->element->color = PACKED_UINT(0, 0, 0, 255);
  element_set_parent(menu->element, parent);
  menu->element->xflags |= ELEMENT_HIDDEN;
  /* As default all menus should have borders, to create a uniform look.  Note that this can be configured.  TODO: Implement the config of borders. */
  element_set_borders(menu->element, menu->border_size, menu->border_size, menu->border_size, menu->border_size, PACKED_UINT_DEFAULT_BORDERS);
  element_set_data_menu(menu->element, menu);
  /* Create the selected rect element. */
  menu->selelem = element_create(100, 100, 100, font_height(font), FALSE);
  menu->selelem->xflags |= ELEMENT_HIDDEN;
  menu->selelem->color = MENU_DEFAULT_SELECTED_COLOR;
  /* Row init. */
  menu->was_viewtop = -1;
  menu->viewtop     = 0;
  menu->selected    = 0;
  menu->maxrows     = MENU_DEFAULT_MAX_ROWS;
  menu->rows        = 0;
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

/* ----------------------------- Menuu create submenu ----------------------------- */

Menu *menu_create_submenu(Menu *const parent, const char *const restrict lable, void *data, MenuAcceptFunc accept_routine) {
  ASSERT(parent);
  ASSERT(data);
  ASSERT(accept_routine);
  Menu *menu = menu_create(parent->element, parent->font, data, menu_submenu_pos_routine, accept_routine);
  menu->parent = parent;
  menu_push_back_submenu(parent, lable, menu);
  return menu;
}

/* ----------------------------- Menu free ----------------------------- */

void menu_free(Menu *const menu) {
  if (!menu) {
    return;
  }
  element_free(menu->selelem);
  menu->selelem = NULL;
  vertex_buffer_delete(menu->buffer);
  cvec_free(menu->entries);
  free(menu->sb);
  free(menu);
}

/* ----------------------------- Menu get active ----------------------------- */

Menu *menu_get_active(void) {
  return active_menu;
}

/* ----------------------------- Menu draw ----------------------------- */

/* Perform a draw call for a `Menu`.  Note that this should be
 * called every frame for all `root-menu's` and never for `sub-menu's`. */
void menu_draw(Menu *const menu) {
  ASSERT_MENU;
  Menu *submenu;
  /* Only draw the suggestmenu if there are any available suggestions. */
  if (!(menu->element->xflags & ELEMENT_HIDDEN) && cvec_len(menu->entries)) {
    menu_resize(menu);
    /* Draw the main element of the suggestmenu. */
    element_draw(menu->element);
    /* Highlight the selected entry in the suggestmenu when its on the screen. */
    menu_draw_selected(menu);
    /* Draw the scrollbar of the suggestmenu. */
    scrollbar_draw(menu->sb);
    /* Draw the text of the suggestmenu entries. */
    menu_draw_text(menu);
    /* Recursivly draw all submenus of this menu, this way we can have any number of nested menus. */
    for (int i=0; i<cvec_len(menu->entries); ++i) {
      if ((submenu = menu_get_entry_menu(menu, i))) {
        menu_draw(submenu);
      }
    }
  }
}

/* ----------------------------- Menu push back ----------------------------- */

void menu_push_back(Menu *const menu, const char *const restrict string) {
  ASSERT_MENU;
  ASSERT(string);
  cvec_push(menu->entries, menu_entry_create(string));
  menu->xflags |= MENU_REFRESH_WIDTH;
}

/* ----------------------------- Menu refresh pos ----------------------------- */

/* When refreshing the postition, we also always refresh the text of the
 * menu, as the text must always be refreshed when the position changes. */
void menu_refresh_pos(Menu *const menu) {
  ASSERT_MENU;
  menu->xflags |= (MENU_REFRESH_POS | MENU_REFRESH_TEXT);
}

/* ----------------------------- Menu refresh text ----------------------------- */

void menu_refresh_text(Menu *const menu) {
  ASSERT_MENU;
  menu->xflags |= MENU_REFRESH_TEXT;
}

/* ----------------------------- Menu refresh scrollbar ----------------------------- */

void menu_refresh_scrollbar(Menu *const menu) {
  ASSERT_MENU;
  scrollbar_refresh(menu->sb);
}

/* ----------------------------- Menu show ----------------------------- */

void menu_show(Menu *const menu, bool show) {
  ASSERT_MENU;
  /* Showing this menu. */
  if (show) {
    /* Always close the currently active menu, even when its the
     * same as `menu`.  This ensures correctness related to submenus. */
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

/* ----------------------------- Menu selected up ----------------------------- */

void menu_selected_up(Menu *const menu) {
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

/* ----------------------------- Menu selected down ----------------------------- */

void menu_selected_down(Menu *const menu) {
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

/* ----------------------------- Menu submenu exit ----------------------------- */

void menu_submenu_exit(Menu *const menu) {
  ASSERT_MENU;
  if (menu->active_submenu) {
    menu_submenu_exit(menu->active_submenu);
  }
  else {
    menu_exit_submenu_internal(menu);
  }
}

/* ----------------------------- Menu submenu enter ----------------------------- */

void menu_submenu_enter(Menu *const menu) {
  ASSERT_MENU;
  if (menu->active_submenu) {
    menu_submenu_enter(menu->active_submenu);
  }
  else {
    menu_check_submenu(menu);
  }
}

/* ----------------------------- Menu routine accept ----------------------------- */

/* This is used to perform the accept action of the depest opened menu's currently selected entry,
 * or if that selected entry has a submenu and its not open, then it will open it.  This is used
 * for both clicking and kb related execution of the accept routine for that menu. */
void menu_routine_accept(Menu *const menu) {
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
      menu_routine_accept(menu->active_submenu);
    }
  }
}

/* ----------------------------- Menu routine hover ----------------------------- */

void menu_routine_hover(Menu *const menu, float x_pos, float y_pos) {
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
    if (font_row_from_pos(menu->font, top, bot, y_pos, &row) && x_pos < right) {
      menu->selected = lclamp((menu->viewtop + row), menu->viewtop, (menu->viewtop + menu->rows - 1));
      menu_check_submenu(menu);
    }
  }
}

/* ----------------------------- Menu routine scroll ----------------------------- */

void menu_routine_scroll(Menu *const menu, bool direction, float x_pos, float y_pos) {
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
      menu->xflags |= MENU_REFRESH_TEXT;
      /* Ensure that the currently selected entry gets correctly set based on where the mouse is. */
      menu_routine_hover(menu, x_pos, y_pos);
      scrollbar_refresh(menu->sb);
      refresh_needed = TRUE;
    }
  }
}

/* ----------------------------- Menu routine click ----------------------------- */

void menu_routine_click(Menu *const menu, float x_pos, float y_pos) {
  ASSERT_MENU;
  float top;
  float bot;
  float right;
  /* Only perform any action when there are entries in the menu. */
  if (cvec_len(menu->entries)) {
    /* Get the absolute values where events are allowed. */
    menu_event_bounds(menu, &top, &bot, &right);
    if (y_pos >= top && y_pos <= bot && x_pos < right) {
      menu_routine_hover(menu, x_pos, y_pos);
      /* Only allow clickes on non submenu entries. */
      if (!menu_get_entry_menu(menu, menu->selected)) {
        menu_routine_accept(menu);
      }
    }
  }
}

/* ----------------------------- Menu clear entries ----------------------------- */

void menu_clear_entries(Menu *const menu) {
  ASSERT_MENU;
  cvec_clear(menu->entries);
  menu->viewtop  = 0;
  menu->selected = 0;
  menu->xflags |= (MENU_REFRESH_TEXT | MENU_REFRESH_POS | MENU_REFRESH_WIDTH);
  // menu->text_refresh_needed  = TRUE;
  // menu->pos_refresh_needed   = TRUE;
  // menu->width_refresh_needed = TRUE;
}

/* ----------------------------- Menu set static width ----------------------------- */

/* Set a static width for `menu`.  TODO: Implement this into text drawing
 * function, so that when there is not enough room it cuts of the entry. */
void menu_set_static_width(Menu *const menu, float width) {
  ASSERT_MENU;
  ALWAYS_ASSERT_MSG((width > 0.0f), "The width of a menu must be positive");
  menu->width   = width;
  menu->xflags |= MENU_WIDTH_IS_STATIC;
}

/* ----------------------------- Menu behavior tab accept ----------------------------- */

/* Configure's the tab behavior for `menu`, if `accept_on_tab` is `TRUE` then tab will act like enter when this menu is active. */
void menu_behavior_tab_accept(Menu *const menu, bool accept_on_tab) {
  ASSERT_MENU;
  if (accept_on_tab) {
    menu->xflags |= MENU_ACCEPT_ON_TAB;
  }
  else {
    menu->xflags &= ~MENU_ACCEPT_ON_TAB;
  }
  // menu->accept_on_tab = accept_on_tab;
}

/* ----------------------------- Menu behavior arrow depth navigation ----------------------------- */

/* Configure's if `menu` should use right arrow to open the submenu at the currently
 * selected entry if it exists and left arrow to close the currently open submenu. */
void menu_behavior_arrow_depth_navigation(Menu *const menu, bool enable_arrow_depth_navigation) {
  ASSERT_MENU;
  if (enable_arrow_depth_navigation) {
    menu->xflags |= MENU_ARROW_DEPTH_NAVIGATION;
  }
  else {
    menu->xflags &= ~MENU_ARROW_DEPTH_NAVIGATION;
  }
  // menu->arrow_depth_navigation = enable_arrow_depth_navigation;
}

/* ----------------------------- Menu set lable offset ----------------------------- */

void menu_set_lable_offset(Menu *const menu, Ushort pixels) {
  ASSERT_MENU;
  menu->lable_offset = pixels;
}

/* ----------------------------- Menu owns element ----------------------------- */

/* Return's `TRUE` if `e` is part of `menu`. */
bool menu_owns_element(Menu *const menu, Element *const e) {
  ASSERT_MENU;
  return element_is_ancestor(e, menu->element);
}

/* ----------------------------- Menu element is main ----------------------------- */

/* Return's `TRUE` if `e` is the main element of `menu`. */
bool menu_element_is_main(Menu *const menu, Element *const e) {
  ASSERT_MENU;
  ASSERT(e);
  return (menu->element == e);
}

/* ----------------------------- Menu allows accept on tab ----------------------------- */

/* Return's `TRUE` if `menu` has `accept_on_tab` flag set. */
bool menu_allows_accept_on_tab(Menu *const menu) {
  ASSERT_MENU;
  return (menu->xflags & MENU_ACCEPT_ON_TAB);
}

/* ----------------------------- Menu allows arrow depth navigation ----------------------------- */

/* Return's `TRUE` if `menu` has `arrow_depth_navigation` flag set.
 * This means that `left` and `right arrow` close's and open's `submenu's`. */
bool menu_allows_arrow_depth_navigation(Menu *const menu) {
  ASSERT_MENU;
  return (menu->xflags & MENU_ARROW_DEPTH_NAVIGATION);
}

/* ----------------------------- Menu is ancestor ----------------------------- */

/* Return's `TRUE` when `ancestor` is an ancestor to `menu`. */
bool menu_is_ancestor(Menu *const menu, Menu *const ancestor) {
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

/* ----------------------------- Menu is shown ----------------------------- */

/* Return's `TRUE` when `menu` is currently being shown and has more then zero entries. */
bool menu_is_shown(Menu *const menu) {
  ASSERT_MENU;
  return (!(menu->element->xflags & ELEMENT_HIDDEN) && cvec_len(menu->entries));
}

/* ----------------------------- Menu get font ----------------------------- */

Font *menu_get_font(Menu *const menu) {
  ASSERT_MENU;
  return menu->font;
}

/* ----------------------------- Menu len ----------------------------- */

int menu_len(Menu *const menu) {
  ASSERT_MENU;
  return cvec_len(menu->entries);
}

/* ----------------------------- Menu qsort cb strlen ----------------------------- */

/* Function callback used for sorting menu entries by length of lable.  Note that this should be passed to `menu_qsort()` */
int menu_qsort_cb_strlen(const void *a, const void *b) {
  const MenuEntry *lhs = *(const MenuEntry **)a;
  const MenuEntry *rhs = *(const MenuEntry **)b;
  long lhs_len = strlen(lhs->lable);
  long rhs_len = strlen(rhs->lable);
  if (lhs_len == rhs_len) {
    return strcmp(lhs->lable, rhs->lable);
  }
  return (lhs_len - rhs_len); 
}

/* ----------------------------- Menu qsort ----------------------------- */

void menu_qsort(Menu *const menu, CmpFuncPtr cmp_func) {
  ASSERT_MENU;
  cvec_qsort(menu->entries, cmp_func);
}

