/** @file prompt.cpp */
#include "../include/prototypes.h"

/* The prompt string used for status-bar questions. */
static char *prompt = NULL;
/* The cursor position in answer. */
static Ulong typing_x = HIGHEST_POSITIVE;

static statusbar_undostruct *statusbar_undotop      = NULL;
static statusbar_undostruct *statusbar_current_undo = NULL;
static statusbar_undo_type   statusbar_last_action  = STATUSBAR_OTHER;

/* Discard the current redo stack to make the undo-redo path a singular path. */
static void statusbar_discard_until(const statusbar_undostruct *item) _NOTHROW {
  statusbar_undostruct *dropit = statusbar_undotop;
  while (dropit && dropit != item) {
    statusbar_undotop = dropit->next;
    free(dropit->answerdata);
    free(dropit);
    dropit = statusbar_undotop;
  }
  statusbar_current_undo = (statusbar_undostruct *)item;
  statusbar_last_action  = STATUSBAR_OTHER; 
}

/* Discard both the current undo stack and redo stack, used for clearing all undo-redo items. */
static void statusbar_discard_all_undo_redo(void) _NOTHROW {
  statusbar_undostruct *dropit;
  /* First clear the redo stack. */
  dropit = statusbar_undotop;
  while (dropit) {
    statusbar_undotop = dropit->next;
    free(dropit->answerdata);
    free(dropit);
    dropit = statusbar_undotop;
  }
  /* Then clear the undo stack. */
  dropit = statusbar_current_undo;
  while (dropit) {
    statusbar_current_undo = dropit->next;
    free(dropit->answerdata);
    free(dropit);
    dropit = statusbar_current_undo;
  }
}

/* Init a new 'statusbar_undostruct *' and add to the stack.  Then based on action perform nessesary actions. */
static void statusbar_add_undo(statusbar_undo_type action, const char *message) _NOTHROW {
  statusbar_undostruct *u = (statusbar_undostruct *)nmalloc(sizeof(*u));
  /* Init the new undo item. */
  u->type       = action;
  u->xflags     = 0;
  u->head_x     = typing_x;
  u->tail_x     = typing_x;
  u->answerdata = NULL;
  /* Discard the redo stack, as this will create a new branch. */
  statusbar_discard_until(statusbar_current_undo);
  /* Ensure correct order. */ {
    u->next = statusbar_undotop;
    statusbar_undotop      = u;
    statusbar_current_undo = u;
  }
  /* Record all relevent data for the undo type. */
  switch (u->type) {
    case STATUSBAR_ADD: {
      u->answerdata = STRLTR_COPY_OF("");
      break;
    }
    case STATUSBAR_BACK:
    case STATUSBAR_DEL: {
      if (answer[u->head_x]) {
        int charlen   = char_length(answer + u->head_x);
        u->answerdata = measured_copy((answer + u->head_x), charlen);
        if (u->type == STATUSBAR_BACK) {
          u->tail_x += charlen;
        }
      }
      break;
    }
    case STATUSBAR_CHOP_NEXT_WORD: {
      /* Save the data that will be cut. */
      u->answerdata = copy_of(message);
      /* And save the length of that data, so we can later inject and erase it. */
      u->tail_x = strlen(u->answerdata);
      break;
    }
    default: {
      die("%s: Bad undo type.\n", __func__);
    }
  }
  statusbar_last_action = action;
}

/* Update a undo of the same type as the current undo-stack top. */
static void statusbar_update_undo(statusbar_undo_type action) _NOTHROW {
  statusbar_undostruct *u = statusbar_undotop;
  if (u->type != action) {
    die("%s: Missmatching undo type.\n", __func__);
  }
  switch (u->type) {
    case STATUSBAR_ADD: {
      Ulong newlen = (typing_x - u->head_x);
      u->answerdata = arealloc(u->answerdata, (newlen + 1));
      memcpy(u->answerdata, (answer + u->head_x), newlen);
      u->answerdata[newlen] = '\0';
      u->tail_x = typing_x;
      break;
    }
    case STATUSBAR_BACK:
    case STATUSBAR_DEL: {
      char *textpos = (answer + typing_x);
      int   charlen = char_length(textpos);
      Ulong datalen = strlen(u->answerdata);
      /* One more deletion. */  
      if (typing_x == u->head_x) {
        inject_in(&u->answerdata, textpos, charlen, datalen);
        u->tail_x = typing_x;
      }
      /* Another backspace. */
      else if (typing_x == (u->head_x - charlen)) {
        inject_in(&u->answerdata, textpos, charlen, 0, TRUE);
        u->head_x = typing_x;
      }
      /* Deletion not related to current item. */
      else {
        statusbar_add_undo(u->type, NULL);
      }
      break;
    }
    default: {
      break;
    }
  }
}

/* Perform the undo at the top of the undo-stack. */
static void do_statusbar_undo(void) _NOTHROW {
  statusbar_undostruct *u = statusbar_current_undo;
  /* If there is no item at the top of the undo-stack, return. */
  if (!u) {
    return;
  }
  switch (u->type) {
    case STATUSBAR_ADD: {
      erase_in(&answer, u->head_x, strlen(u->answerdata));
      typing_x = u->head_x;
      break;
    }
    case STATUSBAR_BACK:
    case STATUSBAR_DEL: {
      inject_in(&answer, u->answerdata, u->head_x);
      typing_x = u->tail_x;
      break;
    }
    case STATUSBAR_CHOP_NEXT_WORD: {
      /* Inject the chop`ed string into the answer at the correct position. */
      inject_in(&answer, u->answerdata, u->head_x, TRUE);
      /* Set the cursor position in the status-bar correctly. */
      typing_x = u->head_x;
      break;
    }
    default: {
      break;
    }
  }
  statusbar_current_undo = statusbar_current_undo->next;
  statusbar_last_action  = STATUSBAR_OTHER;
}

/* Redo the last undid item. */
static void do_statusbar_redo(void) _NOTHROW {
  statusbar_undostruct *u = statusbar_undotop;
  /* If the redo item at the top of the redo-stack does not exist, or if we are on the last redo, return. */
  if (!u || u == statusbar_current_undo) {
    return;
  }
  /* Find the item before the current one in the status-bar undo-stack. */
  while (u->next != statusbar_current_undo) {
    u = u->next;
  }
  switch (u->type) {
    case STATUSBAR_ADD: {
      inject_in(&answer, u->answerdata, u->head_x);
      typing_x = u->tail_x;
      break;
    }
    case STATUSBAR_BACK:
    case STATUSBAR_DEL: {
      erase_in(&answer, u->head_x, strlen(u->answerdata));
      typing_x = u->head_x;
      break;
    }
    case STATUSBAR_CHOP_NEXT_WORD: {
      /* Erase the cut string`s length in the answer at the recorded position where it happend. */
      erase_in(&answer, u->head_x, u->tail_x, TRUE);
      /* Then set the status-bar cursor pos corrently. */
      typing_x = u->head_x;
      break;
    }
    default: {
      break;
    }
  }
  statusbar_current_undo = u;
  statusbar_last_action  = STATUSBAR_OTHER;
}

/* Move to the beginning of the answer. */
static void do_statusbar_home(void) _NOTHROW {
  typing_x = 0;
}

/* Move to the end of the answer. */
static void do_statusbar_end(void) _NOTHROW {
  typing_x = strlen(answer);
}

/* Move to the previous word in the answer. */
static void do_statusbar_prev_word(void) _NOTHROW {
  bool seen_a_word = FALSE, step_forward = FALSE;
  /* Move backward until we pass over the start of a word. */
  while (typing_x) {
    typing_x = step_left(answer, typing_x);
    if (is_word_char(answer + typing_x, FALSE)) {
      seen_a_word = TRUE;
    }
    else if (is_zerowidth(answer + typing_x)) {
      ; /* skip */
    }
    else if (seen_a_word) {
      /* This is space now: we've overshot the start of the word. */
      step_forward = TRUE;
      break;
    }
  }
  if (step_forward) {
    /* Move one character forward again to sit on the start of the word. */
    typing_x = step_right(answer, typing_x);
  }
}

/* Move to the next word in the answer. */
static void do_statusbar_next_word(void) _NOTHROW {
  bool seen_space = !is_word_char(answer + typing_x, FALSE);
  bool seen_word  = !seen_space;
  /* Move forward until we reach either the end or the start of a word, depending on whether the AFTER_ENDS flag is set or not. */
  while (answer[typing_x]) {
    typing_x = step_right(answer, typing_x);
    if (ISSET(AFTER_ENDS)) {
      /* If this is a word character, continue; else it's a separator, and if we've already seen a word, then it's a word end. */
      if (is_word_char((answer + typing_x), FALSE)) {
        seen_word = TRUE;
      }
      else if (is_zerowidth(answer + typing_x)) {
        ; /* skip */
      }
      else if (seen_word) {
        break;
      }
    }
    else {
      if (is_zerowidth(answer + typing_x)) {
        ; /* skip */
      }
      else {
        /* If this is not a word character, then it's a separator; else if we've already seen a separator, then it's a word start. */
        if (!is_word_char((answer + typing_x), FALSE)) {
          seen_space = TRUE;
        }
        else if (seen_space) {
          break;
        }
      }
    }
  }
}

/* Move left one character in the answer. */
static void do_statusbar_left(void) _NOTHROW {
  if (typing_x > 0) {
    typing_x = step_left(answer, typing_x);
    while (typing_x > 0 && is_zerowidth(answer + typing_x)) {
      typing_x = step_left(answer, typing_x);
    }
  }
}

/* Move right one character in the answer. */
static void do_statusbar_right(void) _NOTHROW {
  if (answer[typing_x]) {
    typing_x = step_right(answer, typing_x);
    while (answer[typing_x] && is_zerowidth(answer + typing_x)) {
      typing_x = step_right(answer, typing_x);
    }
  }
}

/* Backspace over one character in the answer. */
static void do_statusbar_backspace(void) _NOTHROW {
  if (typing_x > 0) {
    Ulong was_x = typing_x;
    typing_x = step_left(answer, typing_x);
    if (statusbar_last_action != STATUSBAR_BACK) {
      statusbar_add_undo(STATUSBAR_BACK, NULL);
    }
    else {
      statusbar_update_undo(STATUSBAR_BACK);
    }
    memmove((answer + typing_x), (answer + was_x), (strlen(answer) - was_x + 1));
  }
}

static void statusbar_delete(void) _NOTHROW {
  if (answer[typing_x]) {
    int charlen = char_length(answer + typing_x);
    erase_in(&answer, typing_x, charlen, FALSE);
    if (is_zerowidth(answer + typing_x)) {
      statusbar_delete();
    }
  }
}

/* Delete one character in the answer. */
static void do_statusbar_delete(void) _NOTHROW {
  if (answer[typing_x]) {
    if (statusbar_last_action != STATUSBAR_DEL) {
      statusbar_add_undo(STATUSBAR_DEL, NULL);
    }
    else {
      statusbar_update_undo(STATUSBAR_DEL);
    }
    statusbar_delete();
  }
}

/* Zap the part of the answer after the cursor, or the whole answer. */
static void lop_the_answer(void) _NOTHROW {
  if (!answer[typing_x]) {
    typing_x = 0;
  }
  answer[typing_x] = '\0';
}

/* Copy the current answer (if any) into the cutbuffer. */
static void copy_the_answer(void) _NOTHROW {
  if (*answer) {
    free_lines(cutbuffer);
    cutbuffer       = make_new_node(NULL);
    cutbuffer->data = copy_of(answer);
    typing_x        = 0;
  }
}

/* Paste the first line of the cutbuffer into the current answer. */
static void paste_into_answer(void) _NOTHROW {
  Ulong pastelen = strlen(cutbuffer->data);
  answer = arealloc(answer, (strlen(answer) + pastelen + 1));
  memmove((answer + typing_x + pastelen), (answer + typing_x), (strlen(answer) - typing_x + 1));
  strncpy((answer + typing_x), cutbuffer->data, pastelen);
  typing_x += pastelen;
}

/* Handle a mouse click on the status-bar prompt or the shortcut list. */
static int do_statusbar_mouse(void) _NOTHROW {
  int click_row = 0;
  int click_col = 0;
  int retval    = get_mouseinput(&click_row, &click_col, TRUE);
  /* We can click on the status-bar window text to move the cursor. */
  if (retval == 0 && wmouse_trafo(footwin, &click_row, &click_col, FALSE)) {
    Ulong start_col = (breadth(prompt) + 2);
    /* Move to where the click occurred. */
    if (click_row == 0 && click_col >= start_col) {
      typing_x = actual_x(answer, get_statusbar_page_start(start_col, ((start_col + wideness(answer, typing_x)) + click_col - start_col)));
    }
  }
  return retval;
}

/* Insert the given short burst of bytes into the answer. */
static void inject_into_answer(char *burst, Ulong count) _NOTHROW {
  /* First encode any embedded NUL byte as 0x0A. */
  for (Ulong index = 0; index < count; ++index) {
    if (!burst[index]) {
      burst[index] = '\n';
    }
  }
  if (statusbar_last_action != STATUSBAR_ADD || statusbar_current_undo->tail_x != typing_x) {
    statusbar_add_undo(STATUSBAR_ADD, NULL);
  }
  answer = arealloc(answer, (strlen(answer) + count + 1));
  memmove((answer + typing_x + count), (answer + typing_x), (strlen(answer) - typing_x + 1));
  strncpy((answer + typing_x), burst, count);
  typing_x += count;
  statusbar_update_undo(STATUSBAR_ADD);
}

/* Get a verbatim keystroke and insert it into the answer. */
static void do_statusbar_verbatim_input(void) _NOTHROW {
  Ulong count = 1;
  char *bytes;
  bytes = get_verbatim_kbinput(footwin, &count);
  if (0 < count && count < 999) {
    inject_into_answer(bytes, count);
  }
  else if (!count) {
    beep();
  }
  free(bytes);
}

/* Add the given input to the input buffer when it's a normal byte, and inject the gathered bytes into the answer when ready. */
static void absorb_character(int input, functionptrtype function) _NOTHROW {
  /* The input buffer. */
  static char *puddle = NULL;
  /* The size of the input buffer; gets doubled whenever needed. */
  static Ulong capacity = 8;
  /* The length of the input buffer. */
  static Ulong depth = 0;
  /* If not a command, discard anything that is not a normal character byte.  Apart from that, only accept input
   * when not in restricted mode, or when not at the "Write File" prompt, or when there is no filename yet. */
  if (!function) {
    if (input < 0x20 || input > 0xFF || meta_key) {
      beep();
    }
    else if (!ISSET(RESTRICTED) || currmenu != MWRITEFILE || !openfile->filename[0]) {
      /* When the input buffer (plus room for terminating NUL) is full, extend it; otherwise, if it does not exist yet, create it. */
      if ((depth + 1) == capacity) {
        capacity = (2 * capacity);
        puddle   = arealloc(puddle, capacity);
      }
      else if (!puddle) {
        puddle = (char *)nmalloc(capacity);
      }
      puddle[depth++] = (char)input;
    }
  }
  /* If there are gathered bytes and we have a command or no other key codes are waiting, it's time to insert these bytes into the answer. */
  if (depth > 0 && (function || !waiting_keycodes())) {
    puddle[depth] = '\0';
    inject_into_answer(puddle, depth);
    depth = 0;
  }
}

/* Chop prev word. */
static void do_statusbar_chop_next_word(void) {
  Ulong steps = 0, was_x;
  /* If there is more then one whitespace to the next word, just delete the white chars until the next word. */
  if (word_more_than_one_white_away(answer, typing_x, TRUE, &steps)) {
    ;
  }
  /* Otherwise, delete the next word. */
  else {
    /* Save the current x pos. */
    was_x = typing_x;
    /* Move to the next word. */
    do_statusbar_next_word();
    if (was_x != typing_x) {
      /* If there was any movement, calculate the steps, then restore the x pos. */
      steps = (typing_x - was_x);
      typing_x = was_x;
    }
  }
  /* Only perform the cutting action when appropriet. */
  if (steps) {
    /* Save the text that will be cut, and pass it when adding the undo item. */
    char *cutting_section = measured_copy((answer + typing_x), steps);
    statusbar_add_undo(STATUSBAR_CHOP_NEXT_WORD, cutting_section);
    free(cutting_section);
    /* Now perform the cutting. */
    while (steps--) {
      statusbar_delete();
    }
  }
}

/* Handle any editing shortcut, and return TRUE when handled. */
static bool handle_editing(functionptrtype function) _NOTHROW {
  if (function == chop_next_word) {
    do_statusbar_chop_next_word();
  }
  else if (function == do_undo) {
    do_statusbar_undo();
  }
  else if (function == do_redo) {
    do_statusbar_redo();
  }
  else if (function == do_left) {
    do_statusbar_left();
  }
  else if (function == do_right) {
    do_statusbar_right();
  }
  else if (function == to_prev_word) {
    do_statusbar_prev_word();
  }
  else if (function == to_next_word) {
    do_statusbar_next_word();
  }
  else if (function == do_home) {
    do_statusbar_home();
  }
  else if (function == do_end) {
    do_statusbar_end();
  }
  /* When in restricted mode at the "Write File" prompt and the filename isn't blank, disallow any input and deletion. */
  else if (ISSET(RESTRICTED) && currmenu == MWRITEFILE && openfile->filename[0]
   && (function == do_verbatim_input || function == do_delete || function == do_backspace || function == cut_text || function == paste_text)) {
    ;
  }
  else if (function == do_verbatim_input) {
    do_statusbar_verbatim_input();
  }
  else if (function == do_delete) {
    do_statusbar_delete();
  }
  else if (function == do_backspace) {
    do_statusbar_backspace();
  }
  else if (function == cut_text) {
    lop_the_answer();
  }
  else if (function == copy_text) {
    copy_the_answer();
  }
  else if (function == paste_text) {
    if (cutbuffer) {
      paste_into_answer();
    }
  }
  else {
    return FALSE;
  }
  /* Don't handle any handled function again. */
  return TRUE;
}

/* Return the column number of the first character of the answer that is displayed in the status bar when the cursor is at the given
 * column, with the available room for the answer starting at base.  Note that (0 <= column - get_statusbar_page_start(column) < COLS). */
Ulong get_statusbar_page_start(Ulong base, Ulong column) _NOTHROW {
  if (column == base || column < (COLS - 1)) {
    return 0;
  }
  else if (COLS > base + 2) {
    return (column - base - 1 - (column - base - 1) % (COLS - base - 2));
  }
  else {
    return (column - 2);
  }
}

/* Reinitialize the cursor position in the answer. */
void put_cursor_at_end_of_answer(void) _NOTHROW {
  typing_x = HIGHEST_POSITIVE;
}

/* Redraw the prompt bar and place the cursor at the right spot. */
static void draw_the_promptbar(void) _NOTHROW {
  Ulong base   = (breadth(prompt) + 2);
  Ulong column = (base + wideness(answer, typing_x));
  Ulong the_page, end_page;
  char *expanded;
  the_page = get_statusbar_page_start(base, column);
  end_page = get_statusbar_page_start(base, (base + breadth(answer) - 1));
  /* Color the prompt bar over its full width. */
  wattron(footwin, interface_color_pair[config->prompt.color]);
  mvwprintw(footwin, 0, 0, "%*s", COLS, " ");
  mvwaddstr(footwin, 0, 0, prompt);
  waddch(footwin, ':');
  waddch(footwin, (the_page == 0) ? ' ' : '<');
  expanded = display_string(answer, the_page, COLS - base, FALSE, TRUE);
  waddstr(footwin, expanded);
  free(expanded);
  if (the_page < end_page && (base + breadth(answer) - the_page) > COLS) {
    mvwaddch(footwin, 0, (COLS - 1), '>');
  }
  wattroff(footwin, interface_color_pair[config->prompt.color]);
#if defined(NCURSES_VERSION_PATCH) && (NCURSES_VERSION_PATCH < 20210220)
  /* Work around a cursor-misplacement bug -- https://sv.gnu.org/bugs/?59808. */
  if (ISSET(NO_HELP)) {
    wmove(footwin, 0, 0);
    wrefresh(footwin);
  }
#endif
  /* Place the cursor at the right spot. */
  wmove(footwin, 0, (column - the_page));
  wnoutrefresh(footwin);
}

/* Remove or add the pipe character at the answer's head. */
void add_or_remove_pipe_symbol_from_answer(void) _NOTHROW {
  if (answer[0] == '|') {
    memmove(answer, (answer + 1), strlen(answer));
    if (typing_x > 0) {
      --typing_x;
    }
  }
  else {
    answer = arealloc(answer, (strlen(answer) + 2));
    memmove((answer + 1), answer, (strlen(answer) + 1));
    answer[0] = '|';
    ++typing_x;
  }
}

/* Get a string of input at the status-bar prompt. */
static functionptrtype acquire_an_answer(int *actual, bool *listed, linestruct **history_list, functionptrtype refresh_func) {
  /* Whatever the answer was before the user foraged into history. */
  char *stored_string = NULL;
  /* Whether the previous keystroke was an attempt at tab completion. */
  bool previous_was_tab = FALSE;
  /* The length of the fragment that the user tries to tab complete. */
  Ulong fragment_length = 0;
  const keystruct *shortcut;
  functionptrtype  function;
  int input;
  if (typing_x > strlen(answer)) {
    typing_x = strlen(answer);
  }
  while (TRUE) {
    draw_the_promptbar();
    /* Read in one keystroke. */
    input = get_kbinput(footwin, VISIBLE);
    /* If the window size changed, go reformat the prompt string. */
    if (input == KEY_WINCH) {
      /* Only needed when in file browser. */
      refresh_func();
      *actual = KEY_WINCH;
      free(stored_string);
      return NULL;
    }
    /* For a click on a shortcut, read in the resulting keycode. */
    if (input == KEY_MOUSE && do_statusbar_mouse() == 1) {
      input = get_kbinput(footwin, BLIND);
    }
    if (input == KEY_MOUSE) {
      continue;
    }
    /* Check for a shortcut in the current list. */
    shortcut = get_shortcut(input);
    function = (shortcut ? shortcut->func : NULL);
    /* When it's a normal character, add it to the answer. */
    absorb_character(input, function);
    if (function == do_cancel || function == do_enter) {
      statusbar_discard_all_undo_redo();
      break;
    }
    if (function == do_tab) {
      if (history_list) {
        if (!previous_was_tab) {
          fragment_length = strlen(answer);
        }
        if (fragment_length > 0) {
          answer   = get_history_completion(history_list, answer, fragment_length);
          typing_x = strlen(answer);
        }
      }
      else {
        /* Allow tab completion of filenames, but not in restricted mode. */
        if ((currmenu & (MINSERTFILE | MWRITEFILE | MGOTODIR)) && !ISSET(RESTRICTED)) {
          answer = input_tab(answer, &typing_x, refresh_func, listed);
        }
      }
    }
    else {
      if (function == get_older_item && history_list) {
        /* If this is the first step into history, start at the bottom. */
        if (!stored_string) {
          reset_history_pointer_for(*history_list);
        }
        /* When moving up from the bottom, remember the current answer. */
        if (!(*history_list)->next) {
          stored_string = mallocstrcpy(stored_string, answer);
        }
        /* If there is an older item, move to it and copy its string. */
        if ((*history_list)->prev) {
          *history_list = (*history_list)->prev;
          answer        = mallocstrcpy(answer, (*history_list)->data);
          typing_x      = strlen(answer);
        }
      }
      else if (function == get_newer_item && history_list) {
        /* If there is a newer item, move to it and copy its string. */
        if ((*history_list)->next) {
          *history_list = (*history_list)->next;
          answer        = mallocstrcpy(answer, (*history_list)->data);
          typing_x      = strlen(answer);
        }
        /* When at the bottom of the history list, restore the old answer. */
        if (!(*history_list)->next && stored_string && !*answer) {
          answer   = mallocstrcpy(answer, stored_string);
          typing_x = strlen(answer);
        }
      }
      else {
        if (function == do_help || function == full_refresh) {
          function();
        }
        else if (function == do_toggle && shortcut->toggle == NO_HELP) {
          TOGGLE(NO_HELP);
          window_init();
          focusing = FALSE;
          refresh_func();
          bottombars(currmenu);
        }
        else if (function == do_nothing) {
          ;
        }
        else if (function == (functionptrtype)implant) {
          implant(shortcut->expansion);
        }
        else if (function && !handle_editing(function)) {
          /* When it's a permissible shortcut, run it and done. */
          if (!ISSET(VIEW_MODE) || !changes_something(function)) {
            function();
            break;
          }
          else {
            beep();
          }
        }
      }
      previous_was_tab = (function == do_tab);
    }
  }
  /* If the history pointer was moved, point it at the bottom again. */
  if (stored_string) {
    reset_history_pointer_for(*history_list);
    free(stored_string);
  }
  *actual = input;
  return function;
}

/* Ask a question on the status bar.  Return 0 when text was entered, -1 for a cancelled entry, -2 for a blank string, and the relevant
 * keycode when a valid shortcut key was pressed.  The 'provided' parameter is the default answer for when simply Enter is typed. */
int do_prompt(int menu, const char *provided, linestruct **history_list, functionptrtype refresh_func, const char *msg, ...) {
  functionptrtype function = NULL;
  va_list ap;
  bool listed = FALSE;
  int retval;
  /* Save a possible current status-bar x position and prompt. */
  Ulong was_typing_x = typing_x;
  char *saved_prompt = prompt;
  bottombars(menu);
  if (answer != provided) {
    answer = mallocstrcpy(answer, provided);
  }
redo_theprompt:
  prompt = (char *)nmalloc((COLS * MAXCHARLEN) + 1);
  va_start(ap, msg);
  vsnprintf(prompt, (COLS * MAXCHARLEN), msg, ap);
  va_end(ap);
  /* Reserve five columns for colon plus angles plus answer, ":<aa>". */
  prompt[actual_x(prompt, ((COLS < 5) ? 0 : (COLS - 5)))] = '\0';
  lastmessage = VACUUM;
  function    = acquire_an_answer(&retval, &listed, history_list, refresh_func);
  free(prompt);
  if (retval == KEY_WINCH) {
    goto redo_theprompt;
  }
  /* Restore a possible previous prompt and maybe the typing position. */
  prompt = saved_prompt;
  if (function == do_cancel || function == do_enter || function == to_first_file || function == to_last_file || function == to_first_line || function == to_last_line) {
    typing_x = was_typing_x;
  }
  /* Set the proper return value for Cancel and Enter. */
  if (function == do_cancel) {
    retval = -1;
  }
  else if (function == do_enter) {
    retval = ((*answer == '\0') ? -2 : 0);
  }
  if (lastmessage == VACUUM) {
    wipe_statusbar();
  }
  /* If possible filename completions are still listed, clear them off. */
  if (listed) {
    refresh_func();
  }
  return retval;
}

#define UNDECIDED -2
/* Ask a simple Yes/No (and optionally All) question on the status bar and return the choice -- either YES or NO or ALL or CANCEL. */
int ask_user(bool withall, const char *question) {
  int choice = UNDECIDED;
  int width  = 16;
  /* TRANSLATORS : For the next three strings, specify the starting letters of the translations for "Yes"/"No"/"All".
   *               The first letter of each of these strings MUST be a single-byte letter; others may be multi-byte. */
  const char *yesstr = _("Yy");
  const char *nostr  = _("Nn");
  const char *allstr = _("Aa");
  const keystruct *shortcut;
  functionptrtype function;
  while (choice == UNDECIDED) {
    char letter[MAXCHARLEN + 1];
    int  index = 0;
    int  kbinput;
    if (!ISSET(NO_HELP)) {
      /* Temporary string for (translated) " Y", " N" and " A". */
      char shortstr[MAXCHARLEN + 2];
      /* The keystroke that is bound to the Cancel function. */
      const keystruct *cancelshortcut = first_sc_for(MYESNO, do_cancel);
      if (COLS < 32) {
        width = (COLS / 2);
      }
      /* Clear the shortcut list from the bottom of the screen. */
      blank_bottombars();
      /* Now show the ones for "Yes", "No", "Cancel" and maybe "All". */
      sprintf(shortstr, " %c", yesstr[0]);
      wmove(footwin, 1, 0);
      post_one_key(shortstr, _("Yes"), width);
      shortstr[1] = nostr[0];
      wmove(footwin, 2, 0);
      post_one_key(shortstr, _("No"), width);
      if (withall) {
        shortstr[1] = allstr[0];
        wmove(footwin, 1, width);
        post_one_key(shortstr, _("All"), width);
      }
      wmove(footwin, 2, width);
      post_one_key(cancelshortcut->keystr, _("Cancel"), width);
    }
    /* Color the prompt bar over its full width and display the question. */
    wattron(footwin, interface_color_pair[PROMPT_BAR]);
    mvwprintw(footwin, 0, 0, "%*s", COLS, " ");
    mvwaddnstr(footwin, 0, 0, question, actual_x(question, (COLS - 1)));
    wattroff(footwin, interface_color_pair[PROMPT_BAR]);
    wnoutrefresh(footwin);
    currmenu = MYESNO;
    /* When not replacing, show the cursor while waiting for a key. */
    kbinput = get_kbinput(footwin, !withall);
    if (kbinput == KEY_WINCH) {
      continue;
    }
    /* Accept first character of an external paste and ignore the rest. */
    if (bracketed_paste) {
      kbinput = get_kbinput(footwin, BLIND);
    }
    while (bracketed_paste) {
      get_kbinput(footwin, BLIND);
    }
    letter[index++] = (Uchar)kbinput;
    /* If the received code is a UTF-8 starter byte, get also the continuation bytes and assemble them into one letter. */
    if (using_utf8() && 0xC0 <= kbinput && kbinput <= 0xF7) {
      int extras = ((kbinput / 16) % 4 + (kbinput <= 0xCF ? 1 : 0));
      while (extras <= waiting_keycodes() && extras-- > 0) {
        letter[index++] = (Uchar)get_kbinput(footwin, !withall);
      }
    }
    letter[index] = '\0';
    /* See if the typed letter is in the Yes, No, or All strings. */
    if (strstr(yesstr, letter)) {
      choice = YES;
    }
    else if (strstr(nostr, letter)) {
      choice = NO;
    }
    else if (withall && strstr(allstr, letter)) {
      choice = ALL;
    }
    else {
      if (strchr("Yy", kbinput)) {
        choice = YES;
      }
      else if (strchr("Nn", kbinput)) {
        choice = NO;
      }
      else if (withall && strchr("Aa", kbinput)) {
        choice = ALL;
      }
    }
    if (choice != UNDECIDED) {
      break;
    }
    shortcut = get_shortcut(kbinput);
    function = (shortcut ? shortcut->func : NULL);
    if (function == do_cancel) {
      choice = CANCEL;
    }
    else if (function == full_refresh) {
      full_refresh();
    }
    else if (function == do_toggle && shortcut->toggle == NO_HELP) {
      TOGGLE(NO_HELP);
      window_init();
      titlebar(NULL);
      focusing = FALSE;
      edit_refresh();
      focusing = TRUE;
    }
    /* Interpret ^N as "No", to allow exiting in anger, and ^Q or ^X too. */
    else if (kbinput == '\x0E'|| (kbinput == '\x11' && !ISSET(MODERN_BINDINGS)) || (kbinput == '\x18' && ISSET(MODERN_BINDINGS))) {
      choice = NO;
    }
    /* And interpret ^Y as "Yes". */
    else if (kbinput == '\x19') {
      choice = YES;
    }
    else if (kbinput == KEY_MOUSE) {
      int mouse_x, mouse_y;
      /* We can click on the Yes/No/All shortcuts to select an answer. */
      if (get_mouseinput(&mouse_y, &mouse_x, FALSE) == 0 && wmouse_trafo(footwin, &mouse_y, &mouse_x, FALSE) && mouse_x < (width * 2) && mouse_y > 0) {
        int x = mouse_x / width;
        int y = mouse_y - 1;
        /* x == 0 means Yes or No, y == 0 means Yes or All. */
        choice = (-2 * x * y + x - y + 1);
        if (choice == ALL && !withall) {
          choice = UNDECIDED;
        }
      }
    }
    else {
      beep();
    }
  }
  return choice;
}
