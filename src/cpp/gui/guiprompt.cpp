/** @file guiprompt.cpp

  @author  Melwin Svensson.
  @date    19-1-2025.

 */
#include "../../include/prototypes.h"

/* If `gui_prompt_mark` is set, then this holds the `x position` of the mark. */
Ulong gui_prompt_mark_x = 0;

/* Whether or not the prompt has its mark set, to be able to select words or the entire `gui_answer`. */
bool gui_prompt_mark = FALSE;

/* The type of prompt this is */
int gui_prompt_type = 0;

/* Enter the gui prompt mode, by setting the prompt mode flag and showing the topbar. */
void gui_enter_prompt_mode(void) {
  gui->flag.set<GUI_PROMPT>();
  move_element(gui->topbar, 0);
  gui->topbar->relative_pos.y = 0;
  resize_element(gui->root, gui->root->size);
  ITER_OVER_ALL_OPENEDITORS(starteditor, editor, 
    move_element(editor->main, (editor->main->pos + vec2(0, gui->topbar->size.h)));
    editor->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
  );
}

/* Leave the gui prompt mode, by unsetting the prompt mode flag and hiding the topbar. */
void gui_leave_prompt_mode(void) {
  gui->flag.unset<GUI_PROMPT>();
  move_element(gui->topbar, vec2(0, -gui->topbar->size.h));
  gui->topbar->relative_pos.y = -gui->topbar->size.h;
  resize_element(gui->root, gui->root->size);
  ITER_OVER_ALL_OPENEDITORS(starteditor, editor, 
    move_element(editor->main, (editor->main->pos + vec2(0, -gui->topbar->size.h)));
    editor->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
  );
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
  refresh_needed = TRUE;
}

/* Return's the index of a mouse press, but only when inside the scope of the `top-bar`, in the case it's not inside the scope, then return `-1`. */
long prompt_index_from_mouse(bool allow_outside) {
  /* When we dont allow a valid return value when outside the confinement of top-bar, just return -1. 
   * This is usefull for when we are tracking a hold after the user has pressed inside the top-bar, then drags outside it. */
  if (!allow_outside && (mousepos.y < gui->topbar->pos.y || mousepos.y > gui->topbar->pos.y + gui->topbar->size.h)) {
    return -1;
  }
  bool no_linenums = !ISSET(LINE_NUMBERS);
  long ret = 0;
  if (no_linenums) {
    SET(LINE_NUMBERS);
  }
  ret = index_from_mouse_x(answer, gui->font, (gui->topbar->pos.x + (pixel_breadth(gui->font, " ") / 2) + pixel_breadth(gui->font, prompt)));
  if (no_linenums) {
    UNSET(LINE_NUMBERS);
  }
  return ret;
}
