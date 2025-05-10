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
  ASSERT(etb->element)

#define ETB_BORDER_COLOR    vec4(vec3(0.5f),  1.0f)
#define ETB_ACTIVE_COLOR    vec4(vec3(0.25f), 1.0f)
#define ETB_INACTIVE_COLOR  vec4(vec3(0.08f), 1.0f)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct EditorTopbar {
  /* Boolian flags. */
  bool active_refresh_needed  : 1;
  bool text_refresh_needed    : 1;
  bool entries_refresh_needed : 1;

  vertex_buffer_t *buffer;
  guieditor  *editor;
  guielement *element;
};


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static void gui_editor_topbar_refresh_active(EditorTopbar *const etb) {
  ASSERT_ETB;
  guielement *b;
  if (etb->active_refresh_needed) {
    for (Ulong i=0; i<etb->element->children.size(); ++i) {
      b = etb->element->children[i];
      if (gui_element_has_file_data(b)) {
        b->color = ((b->data.file == etb->editor->openfile) ? ETB_ACTIVE_COLOR : ETB_INACTIVE_COLOR);
      }
    }
    etb->active_refresh_needed = FALSE;
  }
}

static void gui_editor_topbar_refresh_text(EditorTopbar *const etb) {
  ASSERT_ETB;
  guielement *b;
  if (etb->text_refresh_needed) {
    vertex_buffer_clear(etb->buffer);
    for (Ulong i=0; i<etb->element->children.size(); ++i) {
      b = etb->element->children[i];
      vertex_buffer_add_element_lable_offset(b, gui_font_get_font(gui->uifont), etb->buffer, vec2(pixbreadth(gui->uifont, " "), 0));
    }
    etb->text_refresh_needed = FALSE;
  }
}

static void gui_editor_topbar_refresh_entries(EditorTopbar *const etb) {
  ASSERT_ETB;
  vec2 pos;
  vec4 border;
  guielement *button;
  const char *lable;
  if (etb->entries_refresh_needed) {
    gui_element_delete_children(etb->element);
    /* Start at the same position as the topbar element. */
    pos = etb->element->pos;
    /* Iterate over all files open in the editor. */
    CLIST_ITER(etb->editor->startfile, f,
      lable = (*f->filename ? f->filename : "Nameless");
      button = gui_element_create(etb->element);
      button->flag = { GUIELEMENT_ABOVE, GUIELEMENT_RELATIVE_POS };
      button->cursor_type = GLFW_HAND_CURSOR;
      gui_element_move_resize(button, pos, vec2((pixbreadth(gui->uifont, lable) + pixbreadth(gui->uifont, "  ")), gui_font_height(gui->uifont)));
      gui_element_set_lable(button, lable);
      gui_element_set_file_data(button, f);
      /* Set the correct color for the button based on if it's the currently open file in the editor. */
      button->color = ((f == etb->editor->openfile) ? ETB_ACTIVE_COLOR : ETB_INACTIVE_COLOR);
      button->textcolor = 1;
      /* Set the relative position to the main element of the topbar. */
      button->relative_pos.x = (button->pos.x - etb->element->pos.x);
      button->relative_pos.y = 0;
      /* Get the correct border size. */
      border = ((f == f->next) ? 1 : (f == etb->editor->startfile) ? vec4(1, 0, 1, 1) : (f->next == etb->editor->startfile) ? 1 : vec4(1, 0, 1, 1));
      gui_element_set_borders(button, border, ETB_BORDER_COLOR);
      pos.x += button->size.w;
    );
    etb->entries_refresh_needed = FALSE;
    etb->text_refresh_needed = TRUE;
  }
}

static void gui_editor_topbar_draw_entries(EditorTopbar *const etb) {
  ASSERT_ETB;
  for (Ulong i=0; i<etb->element->children.size(); ++i) {
    gui_element_draw(etb->element->children[i]);
  }
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
  return etb;
}

void gui_editor_topbar_free(EditorTopbar *const etb) {
  /* Make this function `NO-OP`. */
  if (!etb) {
    return;
  }
  vertex_buffer_delete(etb->buffer);
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
}

void gui_editor_topbar_entries_refresh_needed(EditorTopbar *const etb) {
  ASSERT_ETB;
  etb->entries_refresh_needed = TRUE;
}

bool gui_editor_topbar_element_is_main(EditorTopbar *const etb, guielement *const e) {
  ASSERT_ETB;
  ASSERT(e);
  return (etb->element == e);
}
