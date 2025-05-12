/** @file gui/editor/topbar.c

  @author  Melwin Svensson.
  @date    10-5-2025.

 */
#include "../../../include/prototypes.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_ETB      \
  ASSERT(etb);          \
  ASSERT(etb->buffer);  \
  ASSERT(etb->editor);  \
  ASSERT(etb->element); \
  ASSERT(etb->context)

#define ETB_BORDER_COLOR    vec4(vec3(0.5f),  1.0f)
#define ETB_ACTIVE_COLOR    vec4(vec3(0.25f), 1.0f)
#define ETB_INACTIVE_COLOR  vec4(vec3(0.08f), 1.0f)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


/* `Internal` structure that represent's the context menu for a `EditorTopbar` structure. */
typedef struct {
  guielement *clicked;
  Menu *button_menu;
  Menu *topbar_menu;
} EditorTopbarContextMenu;

struct EditorTopbar {
  /* Boolian flags. */
  bool active_refresh_needed  : 1;
  bool text_refresh_needed    : 1;
  bool entries_refresh_needed : 1;

  vertex_buffer_t *buffer;
  guieditor       *editor;
  guielement      *element;

  EditorTopbarContextMenu *context;
};


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static void gui_editor_topbar_refresh_active(EditorTopbar *const etb) {
  ASSERT_ETB;
  if (etb->active_refresh_needed) {
    GUI_ELEMENT_CHILDREN_ITER(etb->element, i, button,
      if (gui_element_has_file_data(button)) {
        button->color = ((button->ed_file == etb->editor->openfile) ? ETB_ACTIVE_COLOR : ETB_INACTIVE_COLOR);
      }
    );
    etb->active_refresh_needed = FALSE;
  }
}

static void gui_editor_topbar_refresh_text(EditorTopbar *const etb) {
  ASSERT_ETB;
  if (etb->text_refresh_needed) {
    vertex_buffer_clear(etb->buffer);
    GUI_ELEMENT_CHILDREN_ITER(etb->element, i, child,
      if (gui_element_has_file_data(child)) {
        vertex_buffer_add_element_lable_offset(child, gui_font_get_font(gui->uifont), etb->buffer, vec2(pixbreadth(gui->uifont, " "), 0));
      }
    );
    etb->text_refresh_needed = FALSE;
  }
}

static void gui_editor_topbar_delete_entries(EditorTopbar *const etb) {
  ASSERT_ETB;
  GUI_ELEMENT_CHILDREN_ITER(etb->element, i, child,
    if (gui_element_has_file_data(child)) {
      gui_element_free(child);
      --i;
    }
  );
}

static void gui_editor_topbar_create_button(EditorTopbar *const etb, openfilestruct *const f, vec2 *const pos) {
  ASSERT_ETB;
  ASSERT(f);
  ASSERT(pos);
  guielement *button;
  /* If `f` has a set name, then use it.  Otherwise, use the placeholder `Nameless`. */
  const char *lable = (*f->filename ? f->filename : "Nameless");
  button = gui_element_create(etb->element);
  button->flag = { GUIELEMENT_ABOVE, GUIELEMENT_RELATIVE_POS };
  button->cursor_type = GLFW_HAND_CURSOR;
  gui_element_move_resize(button, *pos, vec2((pixbreadth(gui->uifont, lable) + pixbreadth(gui->uifont, "  ")), gui_font_height(gui->uifont)));
  gui_element_set_lable(button, lable);
  gui_element_set_file_data(button, f);
  /* Set the correct color for the button based on if it's the currently open file in the editor. */
  button->color = ((f == etb->editor->openfile) ? ETB_ACTIVE_COLOR : ETB_INACTIVE_COLOR);
  button->textcolor = 1;
  /* Set the relative position to the main element of the topbar. */
  button->relative_pos.x = (button->pos.x - etb->element->pos.x);
  button->relative_pos.y = 0;
  /* When there is only a single file open or when at the last file, all borders should be uniform.  Otherwise, the it should not have a right border. */
  gui_element_set_borders(button, ((CLIST_SINGLE(f) || f->next == etb->editor->startfile) ? 1 : vec4(1, 0, 1, 1)), ETB_BORDER_COLOR);
  pos->x += button->size.w;
}

static void gui_editor_topbar_refresh_entries(EditorTopbar *const etb) {
  ASSERT_ETB;
  vec2 pos;
  if (etb->entries_refresh_needed) {
    gui_editor_topbar_delete_entries(etb);
    /* Start at the same position as the topbar element. */
    pos = etb->element->pos;
    /* Iterate over all files open in the editor. */
    CLIST_ITER(etb->editor->startfile, f,
      gui_editor_topbar_create_button(etb, f, &pos);
    );
    etb->entries_refresh_needed = FALSE;
    etb->text_refresh_needed = TRUE;
  }
}

static void gui_editor_topbar_draw_entries(EditorTopbar *const etb) {
  ASSERT_ETB;
  GUI_ELEMENT_CHILDREN_ITER(etb->element, i, child,
    if (gui_element_has_file_data(child)) {
      gui_element_draw(child);
    }
  );
}

/* ----------------------------- Context menu ----------------------------- */

/* The position routine for the button context menu of the editor topbar. */
static void gui_editor_topbar_button_context_menu_pos(void *arg, vec2 size, vec2 *pos) {
  ASSERT(arg);
  ASSERT(pos);
  *pos = mousepos;
}

/* The accept routine for the button context menu of the editor topbar. */
static void gui_editor_topbar_button_context_menu_accept(void *arg, const char *const restrict entry_string, int index) {
  ASSERT(arg);
  ASSERT(entry_string);
  EditorTopbar *etb = (__TYPE(etb))arg;
  ASSERT_ETB;
  openfilestruct *file;
  /* Ensure this only perfoms any action when the clicked element is a button of the topbar. */
  if (etb->context->clicked && gui_element_has_file_data(etb->context->clicked) && gui_element_has_editor_data(etb->context->clicked->parent)) {
    file = etb->context->clicked->ed_file;
    switch (index) {
      /* Close */
      case 0: {
        gui_editor_close_single(file);
        break;
      }
      /* Close Others */
      case 1: {
        gui_editor_close_other_files(file);
        break;
      }
      /* Close All */
      case 2: {
        gui_editor_close_all_files(file);
        break;
      }
    }
    etb->context->clicked = NULL;
  }
}

/* The position routine for the context menu of the editor topbar. */
static void gui_editor_topbar_context_menu_pos(void *arg, vec2 size, vec2 *pos) {
  ASSERT(arg);
  ASSERT(pos);
  *pos = mousepos;
}

/* The accept routine for the context menu of the editor topbar. */
static void gui_editor_topbar_context_menu_accept(void *arg, const char *const restrict entry_string, int index) {
  ASSERT(arg);
  ASSERT(entry_string);
  EditorTopbar *etb = (__TYPE(etb))arg;
  ASSERT_ETB;
  /* Ensure this only perfoms any action when the clicked element is a button of the topbar. */
  if (etb->context->clicked && etb->context->clicked == etb->element && gui_element_has_editor_data(etb->context->clicked)) {
    switch (index) {
      case 0: {
        gui_editor_set_open(etb->context->clicked->ed_editor);
        gui_editor_open_new_empty_buffer();
        break;
      }
    }
    etb->context->clicked = NULL;
  }
}

static void gui_editor_topbar_context_menu_create(EditorTopbar *const etb) {
  ASSERT(etb);
  ASSERT(etb->buffer);
  ASSERT(etb->editor);
  ASSERT(etb->element);
  MALLOC_STRUCT(etb->context);
  etb->context->clicked = NULL;
  etb->context->button_menu = gui_menu_create(etb->element, gui->uifont, etb, gui_editor_topbar_button_context_menu_pos, gui_editor_topbar_button_context_menu_accept);
  gui_menu_push_back(etb->context->button_menu, "Close");
  gui_menu_push_back(etb->context->button_menu, "Close Others");
  gui_menu_push_back(etb->context->button_menu, "Close All");
  etb->context->topbar_menu = gui_menu_create(etb->element, gui->uifont, etb, gui_editor_topbar_context_menu_pos, gui_editor_topbar_context_menu_accept);
  gui_menu_push_back(etb->context->topbar_menu, "New Text File");
}

/* Free the editor topbar context menu's. */
static void gui_editor_topbar_context_menu_free(EditorTopbar *const etb) {
  ASSERT_ETB;
  gui_menu_free(etb->context->button_menu);
  gui_menu_free(etb->context->topbar_menu);
  free(etb->context);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


EditorTopbar *gui_editor_topbar_create(guieditor *const editor) {
  ASSERT(gui);
  ASSERT(gui->uifont);
  ASSERT(editor);
  ASSERT(editor->main);
  EditorTopbar *etb;
  MALLOC_STRUCT(etb);
  /* editor->topbar = et; */
  /* Boolian flags. */
  etb->active_refresh_needed  = TRUE;
  etb->text_refresh_needed    = TRUE;
  etb->entries_refresh_needed = TRUE;
  etb->buffer = make_new_font_buffer();
  etb->editor = editor;
  etb->element = gui_element_create(etb->editor->main);
  etb->element->color = EDIT_BACKGROUND_COLOR;
  etb->element->flag = { GUIELEMENT_RELATIVE_POS, GUIELEMENT_RELATIVE_WIDTH };
  etb->element->relative_pos  = 0;
  etb->element->relative_size = 0;
  gui_element_move_resize(etb->element, etb->editor->main->pos, vec2(etb->editor->main->size.w, gui_font_height(gui->uifont)));
  gui_element_set_editor_data(etb->element, etb->editor);
  gui_editor_topbar_context_menu_create(etb);
  return etb;
}

void gui_editor_topbar_free(EditorTopbar *const etb) {
  /* Make this function `NO-OP`. */
  if (!etb) {
    return;
  }
  vertex_buffer_delete(etb->buffer);
  gui_editor_topbar_context_menu_free(etb);
  free(etb);
}

void gui_editor_topbar_draw(EditorTopbar *const etb) {
  ASSERT_ETB;
  gui_element_draw(etb->element);
  gui_editor_topbar_refresh_active(etb);
  gui_editor_topbar_refresh_entries(etb);
  gui_editor_topbar_refresh_text(etb);
  gui_editor_topbar_draw_entries(etb);
  upload_texture_atlas(gui_font_get_atlas(gui->uifont));
  render_vertex_buffer(gui->font_shader, etb->buffer);
  gui_menu_draw(etb->context->button_menu);
  gui_menu_draw(etb->context->topbar_menu);
}

/* When the open file of the topbar has changed, this should be called to just update the currently active entry in the topbar. */
void gui_editor_topbar_active_refresh_needed(EditorTopbar *const etb) {
  ASSERT_ETB;
  etb->active_refresh_needed = TRUE;
}

/* When rebuilding the entire topbar is requiered. */
void gui_editor_topbar_entries_refresh_needed(EditorTopbar *const etb) {
  ASSERT_ETB;
  etb->entries_refresh_needed = TRUE;
}

void gui_editor_topbar_show_context_menu(EditorTopbar *const etb, guielement *const from_element, bool show) {
  ASSERT_ETB;
  /* Null passed. */
  if (!from_element) {
    gui_menu_show(etb->context->button_menu, FALSE);
    gui_menu_show(etb->context->topbar_menu, FALSE);
    etb->context->clicked = NULL;
  }
  /* Main topbar element. */
  else if (from_element == etb->element) {
    if (show && !gui_menu_is_shown(etb->context->topbar_menu)) {
      etb->context->clicked = from_element;
      gui_menu_show(etb->context->topbar_menu, TRUE);
    }
    else {
      etb->context->clicked = NULL;
      gui_menu_show(etb->context->topbar_menu, FALSE);
    }
  }
  else if (is_ancestor(from_element, etb->element)) {
    /* If there is a call to show the menu, and the menu is not already shown. */
    if (show && !gui_menu_is_shown(etb->context->button_menu)) {
      etb->context->clicked = from_element;
      gui_menu_show(etb->context->button_menu, TRUE);
    }
    /* Otherwise, hide the menu. */
    else {
      etb->context->clicked = NULL;
      gui_menu_show(etb->context->button_menu, FALSE);
    }
  }
  else {
    etb->context->clicked = NULL;
    gui_menu_show(etb->context->button_menu, FALSE);
    gui_menu_show(etb->context->topbar_menu, FALSE);
  }
}

/* Return's `TRUE` when `e` is the main element of `etb`. */
bool gui_editor_topbar_element_is_main(EditorTopbar *const etb, guielement *const e) {
  ASSERT_ETB;
  ASSERT(e);
  return (etb->element == e);
}

/* Return's `TRUE` when `e` is the main element of `etb` or related to the main element of `etb`. */
bool gui_editor_topbar_owns_element(EditorTopbar *const etb, guielement *const e) {
  ASSERT_ETB;
  return is_ancestor(e, etb->element);
}
