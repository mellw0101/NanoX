/** @file guiprompt.cpp

  @author  Melwin Svensson.
  @date    19-1-2025.

 */
#include "../../include/prototypes.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* If `gui_prompt_mark` is set, then this holds the `x position` of the mark. */
Ulong gui_prompt_mark_x = 0;

/* Whether or not the prompt has its mark set, to be able to select words or the entire `gui_answer`. */
bool gui_prompt_mark = FALSE;

/* The type of prompt this is */
int gui_prompt_type = 0;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* Enter the gui prompt mode, by setting the prompt mode flag and showing the topbar. */
void gui_enter_prompt_mode(void) {
  statusbar_discard_all_undo_redo();
  gui->flag.set<GUI_PROMPT>();
  gui->promptmenu->element->flag.unset<GUIELEMENT_HIDDEN>();
  gui->promptmenu->flag.refresh_needed = TRUE;
  refresh_needed = TRUE;
}

/* Leave the gui prompt mode, by unsetting the prompt mode flag and hiding the topbar. */
void gui_leave_prompt_mode(void) {
  statusbar_discard_all_undo_redo();
  gui->flag.unset<GUI_PROMPT>();
  gui->promptmenu->element->flag.set<GUIELEMENT_HIDDEN>();
  refresh_needed = TRUE;
}

/* Set up the prompt. */
void gui_ask_user(const char *question, guiprompt_type type) {
  /* Set this flag so that the key and char callbacks both go into prompt-mode. */
  gui_enter_prompt_mode();
  prompt = free_and_assign(prompt, copy_of(question));
  append_to(&prompt, S__LEN(": "));
  /* Reset the gui answer string. */
  if (answer) {
    *answer = '\0';
  }
  else {
    answer = STRLTR_COPY_OF("");
  }
  typing_x = 0;
  /* Set the type so that we know what to do upon a responce. */
  gui_prompt_type = type;
}

/* Return's the index of a mouse press, but only when inside the scope of the `top-bar`, in the case it's not inside the scope, then return `-1`. */
long prompt_index_from_mouse(bool allow_outside) {
  bool no_linenums = !ISSET(LINE_NUMBERS);
  long ret = 0;
  /* When we dont allow a valid return value when outside the confinement of top-bar, just return -1. 
   * This is usefull for when we are tracking a hold after the user has pressed inside the top-bar, then drags outside it. */
  if (!allow_outside && (mousepos.y < gui->promptmenu->element->pos.y || mousepos.y > (gui->promptmenu->element->pos.y + gui->promptmenu->element->size.h))) {
    return -1;
  }
  if (no_linenums) {
    SET(LINE_NUMBERS);
  }
  ret = index_from_mouse_x(answer, gui->uifont, (gui->promptmenu->element->pos.x + (pixbreadth(gui->uifont, " ") / 2) + pixbreadth(gui->uifont, prompt)));
  if (no_linenums) {
    UNSET(LINE_NUMBERS);
  }
  return ret;
}

_UNUSED static GuiPromptMenuEntry *gui_promptmenu_entry_create(GuiPromptMenuEntry *prev) {
  GuiPromptMenuEntry *entry = (__TYPE(entry))xmalloc(sizeof(*entry));
  entry->description = NULL;
  entry->next = NULL;
  entry->prev = prev;
  return entry;
}

/* Create the gui `prompt-menu` struct. */
void gui_promptmenu_init(void) {
  ASSERT(gui);
  gui->promptmenu = (GuiPromptMenu *)xmalloc(sizeof(*gui->promptmenu));
  gui->promptmenu->flag.refresh_needed = FALSE;
  gui->promptmenu->buffer  = make_new_font_buffer();
  gui->promptmenu->element = guielement_create(gui->root, FALSE);
  guielement_move_resize(
    gui->promptmenu->element,
    vec2(0/* , -FONT_HEIGHT(gui->uifont) */),
    vec2(gui->width, FONT_HEIGHT(gui->uifont))
  );
  gui->promptmenu->element->color = VEC4_VS_CODE_RED;
  gui->promptmenu->element->flag.set<GUIELEMENT_RELATIVE_WIDTH>();
  gui->promptmenu->element->relative_size = 0;
  gui->promptmenu->element->flag.set<GUIELEMENT_HIDDEN>();
}

/* Delete the gui `prompt-menu` struct. */
void gui_promptmenu_delete(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->buffer);
  vertex_buffer_delete(gui->promptmenu->buffer);
  free(gui->promptmenu);
}

void gui_promptmenu_open_file(void) {
  if (*answer) {
    /* Path is a directory. */
    if (dir_exists(answer)) {
      show_statusmsg(AHEM, 2, "'%s' is a directory", answer);
    }
    /* Path is a block device. */
    else if (blkdev_exists(answer)) {
      show_statusmsg(AHEM, 2, "'%s' is a block device", answer);
    }
    /* If there is no file at the given path. */
    else if (!file_exists(answer)) {
      show_statusmsg(AHEM, 2, "'%s' does not exist", answer);
    }
    else {
      guieditor_open_buffer(answer);
    }
  }
  gui_leave_prompt_mode();
}
