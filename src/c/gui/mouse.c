/** @file gui/mouse.c

  @author  Melwin Svensson.
  @date    13-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define MOUSE_FLAGS(flag)     mouse_flags[((flag) / sizeof(__TYPE(mouse_flags[0])) * 8)]
#define MOUSE_FLAGMASK(flag)  ((__TYPE(mouse_flags[0]))1 << ((flag) % (sizeof(__TYPE(mouse_flags[0])) * 8)))
#define MOUSE_SET(flag)       MOUSE_FLAGS(flag) |= MOUSE_FLAGMASK(flag)
#define MOUSE_UNSET(flag)     MOUSE_FLAGS(flag) &= ~MOUSE_FLAGMASK(flag)
#define MOUSE_ISSET(flag)     (MOUSE_FLAGS(flag) & MOUSE_FLAGMASK(flag))
#define MOUSE_TOGGLE(flag)    MOUSE_FLAGS(flag) ^= MOUSE_FLAGMASK(flag)

#define DOUBLE_CLICK_THRESHOLD  MILLI_TO_NANO(200)

#define MOUSE_LOCK_READ(...)   RWLOCK_RDLOCK_ACTION(&rwlock, __VA_ARGS__)
#define MOUSE_LOCK_WRITE(...)  RWLOCK_WRLOCK_ACTION(&rwlock, __VA_ARGS__)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static int last_mouse_button = 0;
/* Flags for the mouse, to indecate the current state of the mouse. */
static Uint mouse_flags[1];
/* The current x position of the mouse. */
static float mouse_xpos = 0;
/* The current y position of the mouse. */
static float mouse_ypos = 0;
/* The x position the mouse had before the last update. */
static float last_mouse_xpos = 0;
/* The y position the mouse had before the last update. */
static float last_mouse_ypos = 0;

static Llong mouse_last_click_time = 0;

static rwlock_t rwlock;

static Element *gl_mouse_element_clicked = NULL;
static Element *gl_mouse_element_entered = NULL;

static int mouse_gui_current_cursor = SDL_SYSTEM_CURSOR_DEFAULT;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Gl mouse system cursor get ----------------------------- */

static SDL_Cursor *gl_mouse_system_cursor_get(SDL_SystemCursor type) {
  static SDL_Cursor *def  = NULL;
  static SDL_Cursor *ptr  = NULL;
  static SDL_Cursor *text = NULL;
  static SDL_Cursor *wait = NULL;
  switch (type) {
    case SDL_SYSTEM_CURSOR_DEFAULT: {
      if (!def) {
        def = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
      }
      return def;
    }
    case SDL_SYSTEM_CURSOR_TEXT: {
      if (!text) {
        text = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
      }
      return text;
    }
    case SDL_SYSTEM_CURSOR_WAIT: {
      if (!wait) {
        wait = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
      }
      return wait;
    }
    case SDL_SYSTEM_CURSOR_POINTER: {
      if (!ptr) {
        ptr = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
      }
      return ptr;
    }
    default: {
      log_ERR_FA("Does not currently support the requested system cursor type");
    }
  }
}

/* ----------------------------- Gl mouse set cursor ----------------------------- */

static inline void gl_mouse_set_cursor(SDL_SystemCursor type) {
  SDL_SetCursor(gl_mouse_system_cursor_get(type));
  mouse_gui_current_cursor = type;
  refresh_needed = TRUE;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Gl mouse init ----------------------------- */

void gl_mouse_init(void) {
  RWLOCK_INIT(&rwlock, NULL);
}

/* ----------------------------- Gl mouse free ----------------------------- */

void gl_mouse_free(void) {
  RWLOCK_DESTROY(&rwlock);
}

/* ----------------------------- Mouse gui update state ----------------------------- */

/* Update the mouse click and flag state. */
void gl_mouse_update_state(bool press, int button) {
  MOUSE_LOCK_WRITE(
    /* Press */
    if (press) {
      /* Give some wiggle room for the position of a repeated mouse click. */
      if (button == last_mouse_button && (frame_elapsed_time() - mouse_last_click_time) < DOUBLE_CLICK_THRESHOLD
      && mouse_xpos > (last_mouse_xpos - 3) && mouse_xpos < (last_mouse_xpos + 3)
      && mouse_ypos > (last_mouse_ypos - 3) && mouse_ypos < (last_mouse_ypos + 3))
      {
        /* First check for a tripple click. */
        if (MOUSE_ISSET(MOUSE_PRESS_WAS_DOUBLE) && !MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE)) {
          MOUSE_SET(MOUSE_PRESS_WAS_TRIPPLE);
        }
        else {
          MOUSE_UNSET(MOUSE_PRESS_WAS_TRIPPLE);
        }
        /* Then check for a double click. */
        if (!MOUSE_ISSET(MOUSE_PRESS_WAS_DOUBLE) && !MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE)) {
          MOUSE_SET(MOUSE_PRESS_WAS_DOUBLE);
        }
        else {
          MOUSE_UNSET(MOUSE_PRESS_WAS_DOUBLE);
        }
      }
      /* Otherwise, unset both states. */
      else {
        MOUSE_UNSET(MOUSE_PRESS_WAS_DOUBLE);
        MOUSE_UNSET(MOUSE_PRESS_WAS_TRIPPLE);
      }
      /* If the left mouse button was pressed, set it as held. */
      if (button == SDL_BUTTON_LEFT) {
        MOUSE_SET(MOUSE_BUTTON_HELD_LEFT);
      }
      /* If the right mouse button was pressed, set it as held. */
      else if (button == SDL_BUTTON_RIGHT) {
        MOUSE_SET(MOUSE_BUTTON_HELD_RIGHT);
      }
      /* If we just had a tripple click, ensure the next click will not be detected as a double click. */
      mouse_last_click_time = (MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE) ? 0 : frame_elapsed_time());
      last_mouse_button = button;
      last_mouse_xpos   = mouse_xpos;
      last_mouse_ypos   = mouse_ypos;
    }
    /* Unpress */
    else {
      /* If the left mouse button was released, unset it as held. */
      if (button == SDL_BUTTON_LEFT) {
        MOUSE_UNSET(MOUSE_BUTTON_HELD_LEFT);
      }
      /* If the right mouse button was released, unset it as held. */
      else if (button == SDL_BUTTON_RIGHT) {
        MOUSE_UNSET(MOUSE_BUTTON_HELD_RIGHT);
      }
    }
  );
}

/* ----------------------------- Mouse gui update pos ----------------------------- */

/* Update the mouse position state, ensuring we save the last position. */
void gl_mouse_update_pos(float x, float y) {
  MOUSE_LOCK_WRITE(
    /* Save the current mouse position. */
    last_mouse_xpos = mouse_xpos;
    last_mouse_ypos = mouse_ypos;
    /* Then update the current mouse position. */
    mouse_xpos = x;
    mouse_ypos = y;
  );
}

/* ----------------------------- Gl mouse x ----------------------------- */

/* The only way to get the mouse x position. */
float gl_mouse_x(void) {
  float ret;
  MOUSE_LOCK_READ(
    ret = mouse_xpos;
  );
  return ret;
}

/* ----------------------------- Gl mouse y ----------------------------- */

/* The only way to get the mouse y position. */
float gl_mouse_y(void) {
  float ret;
  MOUSE_LOCK_READ(
    ret = mouse_ypos;
  );
  return ret;
}

/* ----------------------------- Mouse gui get pos ----------------------------- */

/* Get both mouse positions at once. */
void mouse_gui_get_pos(float *const x, float *const y) {
  ASSERT(x);
  ASSERT(y);
  MOUSE_LOCK_READ(
    *x = mouse_xpos;
    *y = mouse_ypos;
  );
}

/* ----------------------------- Mouse gui get last x ----------------------------- */

float gl_mouse_last_x(void) {
  float ret;
  MOUSE_LOCK_READ(
    ret = last_mouse_xpos;
  );
  return ret;
}

/* ----------------------------- Mouse gui get last y ----------------------------- */

float gl_mouse_last_y(void) {
  float ret;
  MOUSE_LOCK_READ(
    ret = last_mouse_ypos;
  );
  return ret;
}

/* ----------------------------- Gl mouse flag is set ----------------------------- */

/* Check if a mouse flag is set. */
bool gl_mouse_flag_is_set(Uint flag) {
  bool ret;
  MOUSE_LOCK_READ(
    ret = MOUSE_ISSET(flag);
  );
  return ret;
}

/* ----------------------------- Gl mouse flag clear all ----------------------------- */

void gl_mouse_flag_clear_all(void) {
  MOUSE_LOCK_WRITE(
    memset(mouse_flags, 0, sizeof(mouse_flags));
  );
}

/* ----------------------------- Gl mouse routine button dn ----------------------------- */

void gl_mouse_routine_button_dn(Uchar button, Ushort _UNUSED mod, float x, float y) {
  Ulong st;
  Ulong end;
  Element *e = gl_mouse_element_clicked = element_grid_get(x, y);
  if (!e) {
    return;
  }
  else if (button == SDL_BUTTON_LEFT) {
    /* Prompt-Menu */
    if (promptmenu_active()) {
      if (!promptmenu_owns_element(e)) {
        promptmenu_close();
      }
      else if (promptmenu_element_is_main(e)) {
        promptmenu_routine_mouse_click_left(x);
        refresh_needed = TRUE;
        return;
      }
      else if (e->dt == ELEMENT_DATA_MENU && menu_get_active() && menu_get_active() == e->dp_menu) {
        menu_routine_click(e->dp_menu, x, y);
        promptmenu_routine_enter();
        return;
      }
    }
    if (menu_get_active() && !menu_owns_element(menu_get_active(), e)) {
      menu_show(menu_get_active(), FALSE);
    }
    /* Menu-Main */
    else if (menu_get_active() && e->dt == ELEMENT_DATA_MENU
    && menu_is_ancestor(e->dp_menu, menu_get_active()) && menu_element_is_main(e->dp_menu, e))
    {
      menu_routine_click(e->dp_menu, x, y);
    }
    /* Scrollbar-Base */
    else if (e->dt == ELEMENT_DATA_SB && scrollbar_element_is_base(e->dp_sb, e)) {
      scrollbar_base_routine_mouse_button_dn(e->dp_sb, e, x, y);
    }
    /* Editor-Text */
    else if (e->dt == ELEMENT_DATA_EDITOR && e == e->dp_editor->text) {
      editor_set_open(e->dp_editor);
      editor_get_text_line_index(e->dp_editor, x, y, &GUI_OF->current, &GUI_OF->current_x);
      /* As this is a click, we can always set the mark at the same position as the cursor.  This way, if
       * the user drags the mouse after, we will select that, and the mark is always cleared as the norm. */
      GUI_OF->mark     = GUI_OF->current;
      GUI_OF->mark_x   = GUI_OF->current_x;
      GUI_OF->softmark = TRUE;
      SET_PWW(GUI_OF);
      if (gl_mouse_flag_is_set(MOUSE_PRESS_WAS_DOUBLE)) {
        st  = wordstartindex(GUI_OF->current->data, GUI_OF->current_x, FALSE);
        end = wordendindex(  GUI_OF->current->data, GUI_OF->current_x, FALSE);
        /* Click inside, or at the start of a word. */
        if (end != GUI_OF->current_x) {
          GUI_OF->mark_x    = st;
          GUI_OF->current_x = end;
        }
        /* Click at the end of a word. */
        else if (st != GUI_OF->current_x && end == GUI_OF->current_x) {
          GUI_OF->mark_x = st;
        }
      }
      /* On a tripple click, select the entire line. */
      else if (gl_mouse_flag_is_set(MOUSE_PRESS_WAS_TRIPPLE)) {
        GUI_OF->mark_x = 0;
        GUI_OF->current_x = STRLEN(GUI_OF->current->data);
      }
      refresh_needed = TRUE;
    }
    /* Editor-Topbar-Tab */
    else if (e->dt == ELEMENT_DATA_FILE && e->parent && e->parent->dt == ELEMENT_DATA_EDITOR
    && etb_element_is_main(e->parent->dp_editor->tb, e->parent) /* && Gui not in prompt-mode. */)
    {
      /* The file of e is not the currently open buffer of the related editor, change that. */
      if (e->dp_file && e->dp_file != e->parent->dp_editor->openfile) {
        e->parent->dp_editor->openfile = e->dp_file;
        editor_redecorate(e->parent->dp_editor);
        editor_resize(e->parent->dp_editor);
        etb_active_refresh_needed(e->parent->dp_editor->tb);
        refresh_needed = TRUE;
      }
    }
  }
  else if (button == SDL_BUTTON_RIGHT) {
    /* Prompt-Menu */
    if (promptmenu_active()) {
      /* When the prompt-menu owns the clicked element, do no further processing. */
      if (promptmenu_owns_element(e)) {
        return;
      }
      /* Otherwise, close the prompt-menu, and let the event get processed normally. */
      else {
        promptmenu_close();
      }
    }
    /* Clicked element is a child of an editor-tab-bar. */
    if (e && e->dt == ELEMENT_DATA_FILE && e->parent && e->parent->dt == ELEMENT_DATA_EDITOR
    && etb_element_is_main(e->parent->dp_editor->tb, e->parent))
    {
      etb_show_context_menu(e->parent->dp_editor->tb, e, TRUE);
    }
    /* Clicked-element is any editor-tab-bar's main element. */
    else if (e && e->dt == ELEMENT_DATA_EDITOR && etb_element_is_main(e->dp_editor->tb, e)) {
      etb_show_context_menu(e->dp_editor->tb, e, TRUE);
    }
    else if (e && e->dt == ELEMENT_DATA_MENU) {
      menu_show(e->dp_menu, FALSE);
    }
    /* Here we should show the context menu, if we need one yet. */
    refresh_needed = TRUE;
  }
}

/* ----------------------------- Mouse gui button up ----------------------------- */

void gl_mouse_routine_button_up(Uchar button, Ushort _UNUSED mod, float x, float y) {
  Element *e = gl_mouse_element_clicked;
  if (button == SDL_BUTTON_LEFT && e) {
    /* If the clicked element was the editors text element,
     * then remove the mark if there has been no movement. */
    if (e->dt == ELEMENT_DATA_EDITOR && e == e->dp_editor->text
    && GUI_OF->mark == GUI_OF->current && GUI_OF->mark_x == GUI_OF->current_x)
    {
      GUI_OF->mark = NULL;
    }
    else if (e->dt == ELEMENT_DATA_SB) {
      if (scrollbar_element_is_thumb(e->dp_sb, e) && e != element_grid_get(x, y)) {
        scrollbar_set_thumb_color(e->dp_sb, FALSE);
      }
      scrollbar_refresh(e->dp_sb);
    }
    gl_mouse_element_clicked = NULL;
    refresh_needed = TRUE;
  }
}

/* ----------------------------- Mouse gui position ----------------------------- */

void gl_mouse_routine_position(float x, float y) {
  Ulong st;
  Ulong end;
  Element *clicked = gl_mouse_element_clicked;
  Element *entered = gl_mouse_element_entered;
  Element *hovered;
  if (MOUSE_ISSET(MOUSE_BUTTON_HELD_LEFT)) {
    if (clicked) {
      if (clicked->dt == ELEMENT_DATA_SB) {
        scrollbar_mouse_pos_routine(clicked->dp_sb, clicked, last_mouse_ypos, y);
        refresh_needed = TRUE;
      }
      else if (clicked->dt == ELEMENT_DATA_EDITOR && clicked == clicked->dp_editor->text) {
        /* If the clicked element is ever not the text element of the currently open editor, we terminate directly. */
        if (clicked->dp_editor != openeditor) {
          log_ERR_FA("The clicked text element is not the text element of the currently open editor");
        }
        editor_get_text_line_index(clicked->dp_editor, x, y, &GUI_OF->current, &GUI_OF->current_x);
        if (MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE)) {
          st  = 0;
          end = STRLEN(GUI_OF->mark->data);
          if (GUI_OF->mark == GUI_OF->current) {
            GUI_OF->mark_x    = st;
            GUI_OF->current_x = end;
          }
          else if (GUI_OF->current->lineno < GUI_OF->mark->lineno) {
            GUI_OF->mark_x = end;
          }
          else {
            GUI_OF->mark_x = st;
          }
        }
        else if (MOUSE_ISSET(MOUSE_PRESS_WAS_DOUBLE)) {
          st  = wordstartindex(GUI_OF->mark->data, GUI_OF->mark_x, TRUE);
          end = wordendindex  (GUI_OF->mark->data, GUI_OF->mark_x, TRUE);
          if (GUI_OF->current == GUI_OF->mark) {
            /* Cursor is inside the word, mark the entire word. */
            if (GUI_OF->current_x >= st && GUI_OF->current_x <= end) {
              GUI_OF->mark_x    = st;
              GUI_OF->current_x = end;
            }
            else if (GUI_OF->current_x < st) {
              GUI_OF->mark_x = end;
            }
            else if (GUI_OF->current_x > end) {
              GUI_OF->mark_x = st;
            }
          }
          else if (GUI_OF->current->lineno < GUI_OF->mark->lineno) {
            GUI_OF->mark_x = end;
          }
          else {
            GUI_OF->mark_x = st;
          }
        }
        SET_PWW(GUI_OF);
        refresh_needed = TRUE;
      }
      else if (promptmenu_active() && promptmenu_element_is_main(clicked)) {
        promptmenu_routine_mouse_click_left(x);
        refresh_needed = TRUE;
      }
    }
  }
  /* Otherwise, look at the currently hovered element, if any. */
  else {
    hovered = element_grid_get(x, y);
    if (hovered) {
      /* When the hovered elements cursor differs, change the current one. */
      if (hovered->cursor != mouse_gui_current_cursor) {
        gl_mouse_set_cursor(hovered->cursor);
      }
      if (menu_get_active() && hovered->dt == ELEMENT_DATA_MENU
      && menu_is_ancestor(hovered->dp_menu, menu_get_active()) && menu_element_is_main(hovered->dp_menu, hovered))
      {
        menu_routine_hover(hovered->dp_menu, x, y);
        refresh_needed = TRUE;
      }
      else if (!entered || hovered != entered) {
        if (hovered->dt == ELEMENT_DATA_SB && scrollbar_element_is_thumb(hovered->dp_sb, hovered)) {
          scrollbar_set_thumb_color(hovered->dp_sb, TRUE);
        }
        /* If the last hovered element was not NULL. */
        if (entered && entered->dt == ELEMENT_DATA_SB && scrollbar_element_is_thumb(entered->dp_sb, entered)) {
          scrollbar_set_thumb_color(entered->dp_sb, FALSE);
        }
        gl_mouse_element_entered = hovered;
      }
    }
    else if (mouse_gui_current_cursor != SDL_SYSTEM_CURSOR_DEFAULT) {
      gl_mouse_set_cursor(SDL_SYSTEM_CURSOR_DEFAULT);
    }
  }
}

/* ----------------------------- Gl mouse routine window left ----------------------------- */

void gl_mouse_routine_window_left(void) {
  Element *entered = gl_mouse_element_entered;
  Element *clicked = gl_mouse_element_clicked;
  float x = mouse_xpos;
  float y = mouse_ypos;
  if (x <= (gl_window_width() / 2.F)) {
    x = -30;
  }
  else if (x >= (gl_window_width() / 2.F)) {
    x = (gl_window_width() + 30);
  }
  if (y <= (gl_window_height() / 2.F)) {
    y = -30;
  }
  else if (y >= (gl_window_height() / 2.F)) {
    y = (gl_window_height() + 30);
  }
  gl_mouse_update_pos(x, y);
  /* If there is a last entered element set, clear it. */
  if (entered) {
    if (!clicked && entered->dt == ELEMENT_DATA_SB && scrollbar_element_is_thumb(entered->dp_sb, entered)) {
      scrollbar_set_thumb_color(entered->dp_sb, FALSE);
    }
    gl_mouse_element_entered = NULL;
  }
}

/* ----------------------------- Mouse gui scroll ----------------------------- */

void gl_mouse_routine_scroll(float mx, float my, int _UNUSED ix, int iy, SDL_MouseWheelDirection type) {
  Element *hovered;
  bool direction;
  if (iy) {
    hovered = element_grid_get(mx, my);
    if (hovered) {
      direction = (((type == SDL_MOUSEWHEEL_NORMAL) ^ (iy < 0)) ? BACKWARD : FORWARD);
      /* Hovered element is the text element of an editor. */
      if (hovered->dt == ELEMENT_DATA_EDITOR && hovered == hovered->dp_editor->text) {
        edit_scroll_for(hovered->dp_editor->openfile, direction);
        if (MOUSE_ISSET(MOUSE_BUTTON_HELD_LEFT)) {
          editor_get_text_line_index(
            hovered->dp_editor, mx, my,
            &hovered->dp_editor->openfile->current,
            &hovered->dp_editor->openfile->current_x
          );
        }
        /* TODO: Add the suggest-menu refreshing here once its done. */
        scrollbar_refresh(hovered->dp_editor->sb);
        refresh_needed = TRUE;
      }
      else if (hovered->dt == ELEMENT_DATA_MENU && menu_get_active()
      && menu_is_ancestor(hovered->dp_menu, menu_get_active()) && menu_element_is_main(hovered->dp_menu, hovered))
      {
        menu_routine_scroll(hovered->dp_menu, direction, mx, my);
      }
    }
  }
}
