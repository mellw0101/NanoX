/** @file gui/editor/topbar.c

  @author  Melwin Svensson.
  @date    10-5-2025.

 */
#include "../../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_ETB(x)   \
  ASSERT(x);            \
  ASSERT((x)->buffer);  \
  ASSERT((x)->editor);  \
  ASSERT((x)->element)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


#define ETB_BORDER_COLOR  PACKED_UINT_FLOAT( .5f,  .5f,  .5f, 1.f)
#define ETB_ACTIVE_COLOR  PACKED_UINT_FLOAT(.25f, .25f, .25f, 1.f)
#define ETB_BUTTON_COLOR  PACKED_UINT_FLOAT(.08f, .08f, .08f, 1.f)


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


enum {
  ETB_REFRESH_SELECTED = (1 << 0),
  ETB_REFRESH_TEXT     = (1 << 1),
  ETB_REFRESH_ENTRIES  = (1 << 2),
# define ETB_REFRESH_SELECTED  ETB_REFRESH_SELECTED
# define ETB_REFRESH_TEXT      ETB_REFRESH_TEXT
# define ETB_REFRESH_ENTRIES   ETB_REFRESH_ENTRIES
# define ETB_XFLAGS_DEFAULT    (ETB_REFRESH_SELECTED | ETB_REFRESH_TEXT | ETB_REFRESH_ENTRIES)
};


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct EditorTopBar {
  /* State flags. */
  Uint             xflags;
  vertex_buffer_t *buffer;
  EDITOR           editor;
  ELEMENT          element;
  struct {
    ELEMENT clicked;
    MENU    button_menu;
    MENU    topbar_menu;
  } context;
};


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Etb refresh active ----------------------------- */

static void etb_refresh_active(ETB etb) {
  ASSERT_ETB(etb);
  if (etb->xflags & ETB_REFRESH_SELECTED) {
    ELEMENT_CHILDREN_ITER(etb->element, i, button,
      if (button->dt == ELEMENT_DATA_FILE) {
        /* The currently selected file. */
        if (button->dp_file == etb->editor->openfile) {
          if (button->color != ETB_ACTIVE_COLOR) {
            button->color   = ETB_ACTIVE_COLOR;
            button->xflags |= ELEMENT_RECT_REFRESH;
          }
        }
        /* All other's. */
        else if (button->color != ETB_BUTTON_COLOR) {
          button->color   = ETB_BUTTON_COLOR;
          button->xflags |= ELEMENT_RECT_REFRESH;
        }
      }
    );
    etb->xflags &= ~ETB_REFRESH_SELECTED;
  }
}

/* ----------------------------- Etb refresh text ----------------------------- */

static void etb_refresh_text(ETB etb) {
  ASSERT_ETB(etb);
  float pen_x;
  float pen_y;
  if (etb->xflags & ETB_REFRESH_TEXT) {
    vertex_buffer_clear(etb->buffer);
    ELEMENT_CHILDREN_ITER(etb->element, i, child,
      if (child->dt == ELEMENT_DATA_FILE) {
        pen_x = (child->x + font_breadth(uifont, " "));
        pen_y = (child->y + font_row_baseline(uifont, 0));
        font_vertbuf_add_mbstr(
          uifont,
          etb->buffer,
          child->lable,
          child->lable_len,
          " ",
          child->text_color,
          &pen_x,
          &pen_y
        );
      }
    );
    etb->xflags &= ~ETB_REFRESH_TEXT;
  }
}

/* ----------------------------- Etb delete entries ----------------------------- */

static void etb_delete_entries(ETB etb) {
  ASSERT_ETB(etb);
  ELEMENT_CHILDREN_ITER(etb->element, i, child,
    if (child->dt == ELEMENT_DATA_FILE) {
      element_free(child);
      --i;
    }
  );
}

/* ----------------------------- Etb create button ----------------------------- */

static void etb_create_button(ETB etb, OPENFILE const f, float *const pos_x, float *const pos_y) {
  ASSERT_ETB(etb);
  ASSERT(f);
  ASSERT(pos_x);
  ASSERT(pos_y);
  ELEMENT button;
  /* If `f` has a set name, then use it.  Otherwise, use the placeholder `Nameless`. */
  const char *lable = (*f->filename ? f->filename : "Nameless");
  button = element_create(
    *pos_x,
    *pos_y,
    (font_breadth(uifont, lable) + font_breadth(uifont, "  ")),
    font_height(uifont),
    TRUE
  );
  element_set_parent(button, etb->element);
  button->xflags |= ELEMENT_REL_POS;
  button->cursor  = SDL_SYSTEM_CURSOR_POINTER;
  element_set_lable(button, lable, strlen(lable));
  element_set_data_file(button, f);
  /* Set the correct color for the button based on if it's the currently open file in the editor. */
  button->color      = ((f == etb->editor->openfile) ? ETB_ACTIVE_COLOR : ETB_BUTTON_COLOR);
  button->text_color = PACKED_UINT(255, 255, 255, 255);
  /* Set the relative position to the main element of the topbar. */
  button->rel_x = (button->x - etb->element->x);
  /* When there is only a single file open or when at the last file, all
   * borders should be uniform.  Otherwise, the it should not have a right border. */
  element_set_borders(button, 1, 1, ((CLIST_SINGLE(f) || f->next == etb->editor->startfile) ? 1 : 0), 1, ETB_BORDER_COLOR);
  *pos_x += button->width;
}

/* ----------------------------- Etb refresh entries ----------------------------- */

static void etb_refresh_entries(ETB etb) {
  ASSERT_ETB(etb);
  float x;
  float y;
  if (etb->xflags & ETB_REFRESH_ENTRIES) {
    etb_delete_entries(etb);
    /* Start at the same position as the topbar element. */
    x = etb->element->x;
    y = etb->element->y;
    /* Iterate over all files open in the editor. */
    CLIST_ITER(etb->editor->startfile, f,
      etb_create_button(etb, f, &x, &y);
    );
    etb->xflags = ((etb->xflags & ~ETB_REFRESH_ENTRIES) | ETB_REFRESH_TEXT);
  }
}

/* ----------------------------- Etb draw entries ----------------------------- */

static void etb_draw_entries(ETB etb) {
  ASSERT_ETB(etb);
  ELEMENT_CHILDREN_ITER(etb->element, i, child,
    if (child->dt == ELEMENT_DATA_FILE) {
      element_draw(child);
    }
  );
}

/* ----------------------------- Etb button context menu accept ----------------------------- */

/* The accept routine for the button context menu of the editor topbar. */
static void etb_button_context_menu_accept(ETB etb, const char *const restrict entry_string, int index) {
  ASSERT_ETB(etb);
  ASSERT(entry_string);
  OPENFILE file;
  /* Ensure this only perfoms any action when the clicked element is a button of the topbar. */
  if (etb->context.clicked && etb->context.clicked->dt == ELEMENT_DATA_FILE && etb->context.clicked->parent
  && etb->element == etb->context.clicked->parent && etb->context.clicked->parent->dt == ELEMENT_DATA_EDITOR)
  {
    file = etb->context.clicked->dp_file;
    /* TODO: Currently, none of these will check if the files are modified at all and as such will simply
     * close, this should not be the case, so when we have added a more dynamic way to call the
     * prompt-menu for such things, we should also make it able to resume some task, where it left off. */
    switch (index) {
      /* Close */
      case 0: {
        editor_close_a_open_buffer(file);
        break;
      }
      /* Close Others */
      case 1: {
        while (!CLIST_SINGLE(file)) {
          editor_close_a_open_buffer(file->next);
        }
        break;
      }
      /* Close All */
      case 2: {
        statusline(ALERT, "NOT IMPLEMENTED YET");
        // gui_editor_close_all_files(file);
        break;
      }
    }
    etb->context.clicked = NULL;
  }
}

/* ----------------------------- Etb context menu accept ----------------------------- */

/* The accept routine for the context menu of the editor topbar. */
static void etb_context_menu_accept(ETB etb, const char *const restrict entry_string, int index) {
  ASSERT_ETB(etb);
  ASSERT(entry_string);
  /* Ensure this only perfoms any action when the clicked element is a button of the topbar. */
  if (etb->context.clicked && etb->context.clicked == etb->element && etb->context.clicked->dt == ELEMENT_DATA_EDITOR) {
    switch (index) {
      case 0: {
        editor_set_open(etb->context.clicked->dp_editor);
        editor_open_new_empty_buffer();
        // gui_editor_set_open(etb->context.clicked->ed_editor);
        // gui_editor_open_new_empty_buffer();
        break;
      }
    }
    etb->context.clicked = NULL;
  }
}

static void etb_context_menu_debug_accept(ETB etb, const char *const restrict entry_string, int _UNUSED index) {
  ASSERT_ETB(etb);
  ASSERT(entry_string);
  FCIO_LOG_INFO("%s", entry_string);
  etb->context.clicked = NULL;
}

/* ----------------------------- Etb context menu create ----------------------------- */

static void etb_context_menu_create(ETB etb) {
  ASSERT_ETB(etb);
  MENU debug_submenu;
  etb->context.clicked = NULL;
  etb->context.button_menu = menu_create(
    etb->element,
    uifont,
    etb,
    /* etb_button_context_menu_pos */
    menu_position_routine_mouse,
    (MenuAcceptFunc)etb_button_context_menu_accept
  );
  menu_push_back(etb->context.button_menu, "Close");
  menu_push_back(etb->context.button_menu, "Close Others");
  menu_push_back(etb->context.button_menu, "Close All");
  etb->context.topbar_menu = menu_create(
    etb->element,
    uifont,
    etb,
    /* etb_context_menu_pos */
    menu_position_routine_mouse,
    (MenuAcceptFunc)etb_context_menu_accept
  );
  menu_push_back(etb->context.topbar_menu, "New Text File");
  debug_submenu = menu_create_submenu(
    etb->context.topbar_menu,
    "DEBUG SUBMENU",
    etb,
    (MenuAcceptFunc)etb_context_menu_debug_accept
  );
  menu_push_back(debug_submenu, "TEST 1");
  menu_push_back(debug_submenu, "TEST 2");
  menu_push_back(debug_submenu, "TEST 3");
  menu_push_back(debug_submenu, "TEST 4");
  menu_push_back(debug_submenu, "TEST 5");
}

/* ----------------------------- Etb context menu free ----------------------------- */

/* Free the editor topbar context menu's. */
static void etb_context_menu_free(ETB etb) {
  ASSERT_ETB(etb);
  menu_free(etb->context.button_menu);
  menu_free(etb->context.topbar_menu);
}

/* ----------------------------- etb_button_sinks_array ----------------------------- */

static float *etb_button_sinks_array(ETB etb, OPENFILE ignore, Ulong *const outlen) {
  ASSERT_ETB(etb);
  ASSERT(outlen);
  ASSERT(ignore);
  Ulong  max = editor_number_of_open_files(etb->editor);
  Ulong  idx = 0;
  float *arr = xmalloc(sizeof(float) * max);
  ELEMENT last = NULL;
  ELEMENT_CHILDREN_ITER(etb->element, i, button,
    if (button->dt == ELEMENT_DATA_FILE) {
      /* We are at the last button. */
      if (etb->editor->startfile->prev == button->dp_file && button->dp_file != ignore) {
        if (!last || last->dp_file != ignore) {
          arr[idx++] = button->x;
        }
        arr[idx++] = (button->x + button->width);
        break;
      }
      if (button->dp_file == ignore || (last && last->dp_file == ignore)) {
        last = button;
        continue;
      }
      arr[idx++] = button->x;
      last = button;
    }
  );
  *outlen = idx;
  return arr;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Etb create ----------------------------- */

ETB etb_create(EDITOR const editor) {
  ASSERT(uifont);
  ASSERT(editor);
  ASSERT(editor->main);
  ETB etb = xmalloc(sizeof(*etb));
  /* State flags. */
  etb->xflags  = ETB_XFLAGS_DEFAULT;
  etb->buffer  = vertex_buffer_new(FONT_VERTBUF);
  etb->editor  = editor;
  etb->element = element_create(editor->main->x, editor->main->y, editor->main->width, font_height(uifont), TRUE);
  element_set_parent(etb->element, editor->main);
  etb->element->color   = PACKED_UINT_EDIT_BACKGROUND;
  etb->element->xflags |= (ELEMENT_REL_POS | ELEMENT_REL_WIDTH);
  element_set_data_editor(etb->element, editor);
  etb_context_menu_create(etb);
  return etb;
}

/* ----------------------------- Etb free ----------------------------- */

void etb_free(ETB etb) {
  /* Make this function `NO-OP`. */
  if (!etb) {
    return;
  }
  vertex_buffer_delete(etb->buffer);
  etb_context_menu_free(etb);
  free(etb);
}

/* ----------------------------- Etb draw ----------------------------- */

void etb_draw(ETB etb) {
  ASSERT_ETB(etb);
  element_draw(etb->element);
  etb_refresh_active(etb);
  etb_refresh_entries(etb);
  etb_refresh_text(etb);
  etb_draw_entries(etb);
  render_vertbuf(uifont, etb->buffer);
  menu_draw(etb->context.button_menu);
  menu_draw(etb->context.topbar_menu);
}

/* ----------------------------- Etb active refresh needed ----------------------------- */

/* When the open file of the topbar has changed, this should be
 * called to just update the currently active entry in the topbar. */
void etb_active_refresh_needed(ETB etb) {
  ASSERT_ETB(etb);
  etb->xflags |= ETB_REFRESH_SELECTED;
}

/* ----------------------------- Etb text refresh needed ----------------------------- */

/* When the position has changed so the text needs to be re-input into the vertex buffer. */
void etb_text_refresh_needed(ETB etb) {
  ASSERT_ETB(etb);
  etb->xflags |= ETB_REFRESH_TEXT;
}

/* ----------------------------- Etb entries refresh needed ----------------------------- */

/* When rebuilding the entire topbar is requiered. */
void etb_entries_refresh_needed(ETB etb) {
  ASSERT_ETB(etb);
  etb->xflags |= ETB_REFRESH_ENTRIES;
}

/* ----------------------------- Etb show context menu ----------------------------- */

void etb_show_context_menu(ETB etb, ELEMENT const from_element, bool show) {
  ASSERT_ETB(etb);
  /* Null passed. */
  if (!from_element) {
    menu_show(etb->context.button_menu, FALSE);
    menu_show(etb->context.topbar_menu, FALSE);
    etb->context.clicked = NULL;
  }
  /* Main topbar element. */
  else if (from_element == etb->element) {
    if (show && !menu_is_shown(etb->context.topbar_menu)) {
      etb->context.clicked = from_element;
      menu_show(etb->context.topbar_menu, TRUE);
    }
    else {
      etb->context.clicked = NULL;
      menu_show(etb->context.topbar_menu, FALSE);
    }
  }
  else if (element_is_ancestor(from_element, etb->element)) {
    /* If there is a call to show the menu, and the menu is not already shown. */
    if (show && !menu_is_shown(etb->context.button_menu)) {
      etb->context.clicked = from_element;
      menu_show(etb->context.button_menu, TRUE);
    }
    /* Otherwise, hide the menu. */
    else {
      etb->context.clicked = NULL;
      menu_show(etb->context.button_menu, FALSE);
    }
  }
  else {
    etb->context.clicked = NULL;
    menu_show(etb->context.button_menu, FALSE);
    menu_show(etb->context.topbar_menu, FALSE);
  }
}

/* ----------------------------- Etb element is main ----------------------------- */

/* Return's `TRUE` when `e` is the main element of `etb`. */
bool etb_element_is_main(ETB etb, ELEMENT const e) {
  ASSERT_ETB(etb);
  ASSERT(e);
  return (etb->element == e);
}

/* ----------------------------- Etb owns element ----------------------------- */

/* Return's `TRUE` when `e` is the main element of `etb` or related to the main element of `etb`. */
bool etb_owns_element(ETB etb, ELEMENT const e) {
  ASSERT_ETB(etb);
  return element_is_ancestor(e, etb->element);
}

/* ----------------------------- etb_tab_routine_mouse_button_left_dn ----------------------------- */

void etb_tab_routine_mouse_button_left_dn(ETB etb, ELEMENT const e) {
  ASSERT_ETB(etb);
  ASSERT(e);
  ASSERT(e->dp_file);
  /* Only perform any action when the file is not the currently active one. */
  if (e->dp_file != etb->editor->openfile) {
    etb->editor->openfile = e->dp_file;
    editor_redecorate(etb->editor);
    editor_resize(etb->editor);
    etb->xflags |= ETB_REFRESH_SELECTED;
    refresh_needed = TRUE;
  }
}

/* ----------------------------- etb_tab_routine_mouse_pos ----------------------------- */

void etb_tab_routine_mouse_held_left(ETB etb, ELEMENT e, float x, float y) {
  ASSERT_ETB(etb);
  ASSERT(e);
  ASSERT(e->dp_file);
  Ulong len;
  float *arr = etb_button_sinks_array(etb, e->dp_file, &len);
  float fx = x;
  Ulong index = 0;
  float closest;
  float value;
  if (len && y >= etb->element->y && y < (etb->element->y + etb->element->height)) {
    closest = fabsf(arr[0] - x);
    for (Ulong i=1; i<len; ++i) {
      if ((value = fabsf(arr[i] - x)) < closest) {
        index = i;
        closest = value;
      }
    }
    if (closest <= 40.f) {
      fx = arr[index];
    }
  }
  element_move(e, fx, y);
  free(arr);
  etb->xflags |= ETB_REFRESH_TEXT;
  refresh_needed = TRUE;
}
