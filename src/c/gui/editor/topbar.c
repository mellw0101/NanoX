/** @file gui/editor/topbar.c

  @author  Melwin Svensson.
  @date    10-5-2025.

 */
#include "../../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_ETB      \
  ASSERT(etb);          \
  ASSERT(etb->buffer);  \
  ASSERT(etb->editor);  \
  ASSERT(etb->element); \
  ASSERT(etb->context)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


#define ETB_BORDER_COLOR  PACKED_UINT_FLOAT( .5f,  .5f,  .5f, 1.f)
#define ETB_ACTIVE_COLOR  PACKED_UINT_FLOAT(.25f, .25f, .25f, 1.f)
#define ETB_BUTTON_COLOR  PACKED_UINT_FLOAT(.08f, .08f, .08f, 1.f)


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


typedef enum {
  ETB_REFRESH_SELECTED = (1 << 0),
  ETB_REFRESH_TEXT     = (1 << 1),
  ETB_REFRESH_ENTRIES  = (1 << 2),
# define ETB_REFRESH_SELECTED  ETB_REFRESH_SELECTED
# define ETB_REFRESH_TEXT      ETB_REFRESH_TEXT
# define ETB_REFRESH_ENTRIES   ETB_REFRESH_ENTRIES
# define ETB_XFLAGS_DEFAULT    (ETB_REFRESH_SELECTED | ETB_REFRESH_TEXT | ETB_REFRESH_ENTRIES)
} EtbStateFlag;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


/* `Internal` structure that represent's the context menu for a `EditorTb` structure. */
typedef struct {
  Element *clicked;
  Menu *button_menu;
  Menu *topbar_menu;
} EtbContextMenu;

struct EditorTb {
  /* State flags. */
  Uint xflags;

  vertex_buffer_t *buffer;
  Editor          *editor;
  Element         *element;

  EtbContextMenu *context;
};


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Etb refresh selected ----------------------------- */

static void etb_refresh_active(EditorTb *const etb) {
  ASSERT_ETB;
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

static void etb_refresh_text(EditorTb *const etb) {
  ASSERT_ETB;
  float pen_x;
  float pen_y;
  if (etb->xflags & ETB_REFRESH_TEXT) {
    vertex_buffer_clear(etb->buffer);
    ELEMENT_CHILDREN_ITER(etb->element, i, child,
      if (child->dt == ELEMENT_DATA_FILE) {
        pen_x = (child->x + font_breadth(uifont, " "));
        pen_y = (child->y + font_row_baseline(uifont, 0));
        font_vertbuf_add_mbstr(uifont, etb->buffer, child->lable, child->lable_len, " ", child->text_color, &pen_x, &pen_y);
      }
    );
    etb->xflags &= ~ETB_REFRESH_TEXT;
  }
}

static void etb_delete_entries(EditorTb *const etb) {
  ASSERT_ETB;
  ELEMENT_CHILDREN_ITER(etb->element, i, child,
    if (child->dt == ELEMENT_DATA_FILE) {
      element_free(child);
      --i;
    }
  );
}

static void etb_create_button(EditorTb *const etb, openfilestruct *const f, float *const pos_x, float *const pos_y) {
  ASSERT_ETB;
  ASSERT(f);
  ASSERT(pos_x);
  ASSERT(pos_y);
  Element *button;
  /* If `f` has a set name, then use it.  Otherwise, use the placeholder `Nameless`. */
  const char *lable = (*f->filename ? f->filename : "Nameless");
  button = element_create((*pos_x), (*pos_y), (font_breadth(uifont, lable) + font_breadth(uifont, "  ")), font_height(uifont), TRUE);
  element_set_parent(button, etb->element);
  button->xflags |= ELEMENT_REL_POS;
  button->cursor  = SDL_SYSTEM_CURSOR_POINTER;
  // button->cursor  = GLFW_HAND_CURSOR;
  element_set_lable(button, lable, strlen(lable));
  element_set_data_file(button, f);
  /* Set the correct color for the button based on if it's the currently open file in the editor. */
  button->color = ((f == etb->editor->openfile) ? ETB_ACTIVE_COLOR : ETB_BUTTON_COLOR);
  button->text_color = PACKED_UINT(255, 255, 255, 255);
  /* Set the relative position to the main element of the topbar. */
  button->rel_x = (button->x - etb->element->x);
  /* When there is only a single file open or when at the last file, all borders should be uniform.  Otherwise, the it should not have a right border. */
  element_set_borders(button, 1, 1, ((CLIST_SINGLE(f) || f->next == etb->editor->startfile) ? 1 : 0), 1, ETB_BORDER_COLOR);
  (*pos_x) += button->width;
}

static void etb_refresh_entries(EditorTb *const etb) {
  ASSERT_ETB;
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
    etb->xflags &= ~ETB_REFRESH_ENTRIES;
    etb->xflags |= ETB_REFRESH_TEXT;
  }
}

static void etb_draw_entries(EditorTb *const etb) {
  ASSERT_ETB;
  ELEMENT_CHILDREN_ITER(etb->element, i, child,
    if (child->dt == ELEMENT_DATA_FILE) {
      element_draw(child);
    }
  );
}

/* ----------------------------- Context menu ----------------------------- */

/* The position routine for the button context menu of the editor topbar. */
static void etb_button_context_menu_pos(void *arg, float _UNUSED width, float _UNUSED height, float *const x, float *const y) {
  ASSERT(arg);
  ASSERT(x);
  ASSERT(y);
  (*x) = gl_mouse_x();
  (*y) = gl_mouse_y();
}

/* The accept routine for the button context menu of the editor topbar. */
static void etb_button_context_menu_accept(void *arg, const char *const restrict entry_string, int index) {
  ASSERT(arg);
  ASSERT(entry_string);
  EditorTb *etb = arg;
  ASSERT_ETB;
  // openfilestruct *file;
  /* Ensure this only perfoms any action when the clicked element is a button of the topbar. */
  if (etb->context->clicked && etb->context->clicked->dt == ELEMENT_DATA_FILE && etb->context->clicked->parent->dt == ELEMENT_DATA_EDITOR) {
    // file = etb->context->clicked->dp_file;
    switch (index) {
      /* Close */
      case 0: {
        // gui_editor_close_single(file);
        break;
      }
      /* Close Others */
      case 1: {
        // gui_editor_close_other_files(file);
        break;
      }
      /* Close All */
      case 2: {
        // gui_editor_close_all_files(file);
        break;
      }
    }
    etb->context->clicked = NULL;
  }
}

/* The position routine for the context menu of the editor topbar. */
static void etb_context_menu_pos(void *arg, float _UNUSED width, float _UNUSED height, float *const x, float *const y) {
  ASSERT(arg);
  ASSERT(x);
  ASSERT(y);
  (*x) = gl_mouse_x();
  (*y) = gl_mouse_y();
}

/* The accept routine for the context menu of the editor topbar. */
static void etb_context_menu_accept(void *arg, const char *const restrict entry_string, int index) {
  ASSERT(arg);
  ASSERT(entry_string);
  EditorTb *etb = arg;
  ASSERT_ETB;
  /* Ensure this only perfoms any action when the clicked element is a button of the topbar. */
  if (etb->context->clicked && etb->context->clicked == etb->element && etb->context->clicked->dt == ELEMENT_DATA_EDITOR) {
    switch (index) {
      case 0: {
        // gui_editor_set_open(etb->context->clicked->ed_editor);
        // gui_editor_open_new_empty_buffer();
        break;
      }
    }
    etb->context->clicked = NULL;
  }
}

static void etb_context_menu_create(EditorTb *const etb) {
  ASSERT(etb);
  ASSERT(etb->buffer);
  ASSERT(etb->editor);
  ASSERT(etb->element);
  MALLOC_STRUCT(etb->context);
  etb->context->clicked = NULL;
  etb->context->button_menu = menu_create(etb->element, uifont, etb, etb_button_context_menu_pos, etb_button_context_menu_accept);
  menu_push_back(etb->context->button_menu, "Close");
  menu_push_back(etb->context->button_menu, "Close Others");
  menu_push_back(etb->context->button_menu, "Close All");
  etb->context->topbar_menu = menu_create(etb->element, uifont, etb, etb_context_menu_pos, etb_context_menu_accept);
  menu_push_back(etb->context->topbar_menu, "New Text File");
}

/* Free the editor topbar context menu's. */
static void etb_context_menu_free(EditorTb *const etb) {
  ASSERT_ETB;
  menu_free(etb->context->button_menu);
  menu_free(etb->context->topbar_menu);
  free(etb->context);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


EditorTb *etb_create(Editor *const editor) {
  ASSERT(uifont);
  ASSERT(editor);
  ASSERT(editor->main);
  EditorTb *etb = xmalloc(sizeof *etb);
  /* State flags. */
  etb->xflags  = ETB_XFLAGS_DEFAULT;
  etb->buffer  = vertex_buffer_new(FONT_VERTBUF);
  etb->editor  = editor;
  etb->element = element_create(etb->editor->main->x, etb->editor->main->y, etb->editor->main->width, font_height(uifont), TRUE);
  element_set_parent(etb->element, etb->editor->main);
  etb->element->color   = PACKED_UINT_EDIT_BACKGROUND;
  etb->element->xflags |= (ELEMENT_REL_POS | ELEMENT_REL_WIDTH);
  element_set_data_editor(etb->element, etb->editor);
  etb_context_menu_create(etb);
  return etb;
}

void etb_free(EditorTb *const etb) {
  /* Make this function `NO-OP`. */
  if (!etb) {
    return;
  }
  vertex_buffer_delete(etb->buffer);
  etb_context_menu_free(etb);
  free(etb);
}

void etb_draw(EditorTb *const etb) {
  ASSERT_ETB;
  element_draw(etb->element);
  etb_refresh_active(etb);
  etb_refresh_entries(etb);
  etb_refresh_text(etb);
  etb_draw_entries(etb);
  render_vertbuf(uifont, etb->buffer);
  menu_draw(etb->context->button_menu);
  menu_draw(etb->context->topbar_menu);
}

/* When the open file of the topbar has changed, this should be called to just update the currently active entry in the topbar. */
void etb_active_refresh_needed(EditorTb *const etb) {
  ASSERT_ETB;
  etb->xflags |= ETB_REFRESH_SELECTED;
}

/* When the position has changed so the text needs to be re-input into the vertex buffer. */
void etb_text_refresh_needed(EditorTb *const etb) {
  ASSERT_ETB;
  etb->xflags |= ETB_REFRESH_TEXT;
}

/* When rebuilding the entire topbar is requiered. */
void etb_entries_refresh_needed(EditorTb *const etb) {
  ASSERT_ETB;
  etb->xflags |= ETB_REFRESH_ENTRIES;
}

void etb_show_context_menu(EditorTb *const etb, Element *const from_element, bool show) {
  ASSERT_ETB;
  /* Null passed. */
  if (!from_element) {
    menu_show(etb->context->button_menu, FALSE);
    menu_show(etb->context->topbar_menu, FALSE);
    etb->context->clicked = NULL;
  }
  /* Main topbar element. */
  else if (from_element == etb->element) {
    if (show && !menu_is_shown(etb->context->topbar_menu)) {
      etb->context->clicked = from_element;
      menu_show(etb->context->topbar_menu, TRUE);
    }
    else {
      etb->context->clicked = NULL;
      menu_show(etb->context->topbar_menu, FALSE);
    }
  }
  else if (element_is_ancestor(from_element, etb->element)) {
    /* If there is a call to show the menu, and the menu is not already shown. */
    if (show && !menu_is_shown(etb->context->button_menu)) {
      etb->context->clicked = from_element;
      menu_show(etb->context->button_menu, TRUE);
    }
    /* Otherwise, hide the menu. */
    else {
      etb->context->clicked = NULL;
      menu_show(etb->context->button_menu, FALSE);
    }
  }
  else {
    etb->context->clicked = NULL;
    menu_show(etb->context->button_menu, FALSE);
    menu_show(etb->context->topbar_menu, FALSE);
  }
}

/* Return's `TRUE` when `e` is the main element of `etb`. */
bool etb_element_is_main(EditorTb *const etb, Element *const e) {
  ASSERT_ETB;
  ASSERT(e);
  return (etb->element == e);
}

/* Return's `TRUE` when `e` is the main element of `etb` or related to the main element of `etb`. */
bool etb_owns_element(EditorTb *const etb, Element *const e) {
  ASSERT_ETB;
  return element_is_ancestor(e, etb->element);
}
