/** @file prompt.c

  @author  Melwin Svensson.
  @date    22-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The prompt string used for status-bar questions. */
char *prompt = NULL;
/* The cursor position in answer. */
Ulong typing_x = HIGHEST_POSITIVE;

static statusbar_undostruct *statusbar_undotop      = NULL;
static statusbar_undostruct *statusbar_current_undo = NULL;
static statusbar_undo_type   statusbar_last_action  = STATUSBAR_OTHER;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Discard the current redo stack to make the undo-redo path a singular path. */
static void statusbar_discard_until(const statusbar_undostruct *item) {
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

/* Init a new 'statusbar_undostruct *' and add to the stack.  Then based on action perform nessesary actions. */
static void statusbar_add_undo(statusbar_undo_type action, const char *message) {
  statusbar_undostruct *u = xmalloc(sizeof(*u));
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
      u->answerdata = COPY_OF("");
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
    case STATUSBAR_CHOP_PREV_WORD:
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
static void statusbar_update_undo(statusbar_undo_type action) {
  statusbar_undostruct *u = statusbar_undotop;
  if (u->type != action) {
    die("%s: Missmatching undo type.\n", __func__);
  }
  switch (u->type) {
    case STATUSBAR_ADD: {
      Ulong newlen = (typing_x - u->head_x);
      u->answerdata = xrealloc(u->answerdata, (newlen + 1));
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
        u->answerdata = xnstrninj(u->answerdata, datalen, textpos, charlen, datalen);
        // inject_in(&u->answerdata, textpos, charlen, datalen);
        u->tail_x = typing_x;
      }
      /* Another backspace. */
      else if (typing_x == (u->head_x - charlen)) {
        u->answerdata = xstrninj(u->answerdata, textpos, charlen, 0);
        // inject_in(&u->answerdata, textpos, charlen, 0, TRUE);
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

static void statusbar_delete(void) {
  int charlen;
  if (answer[typing_x]) {
    charlen = char_length(answer + typing_x);
    // erase_in(&answer, typing_x, charlen, FALSE);
    answer = xstr_erase(answer, typing_x, charlen);
    if (is_zerowidth(answer + typing_x)) {
      statusbar_delete();
    }
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Zap the part of the answer after the cursor, or the whole answer. */
void lop_the_answer(void) {
  if (!answer[typing_x]) {
    typing_x = 0;
  }
  answer[typing_x] = '\0';
}

/* Copy the current answer (if any) into the cutbuffer. */
void copy_the_answer(void) {
  if (*answer) {
    free_lines(cutbuffer);
    cutbuffer       = make_new_node(NULL);
    cutbuffer->data = copy_of(answer);
    typing_x        = 0;
  }
}

/* Paste the first line of the cutbuffer into the current answer. */
void paste_into_answer(void) {
  Ulong pastelen = strlen(cutbuffer->data);
  // answer = xrealloc(answer, (strlen(answer) + pastelen + 1));
  // memmove((answer + typing_x + pastelen), (answer + typing_x), (strlen(answer) - typing_x + 1));
  // strncpy((answer + typing_x), cutbuffer->data, pastelen);
  answer = xstrninj(answer, cutbuffer->data, pastelen, typing_x);
  typing_x += pastelen;
}

/* Add the given input to the input buffer when it's a normal byte, and inject the gathered bytes into the answer when ready. */
void absorb_character(int input, functionptrtype function) {
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
        puddle   = xrealloc(puddle, capacity);
      }
      else if (!puddle) {
        puddle = xmalloc(capacity);
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

/* Discard both the current undo stack and redo stack, used for clearing all undo-redo items. */
void statusbar_discard_all_undo_redo(void) {
  /* Start at the very top of the undo stack. */
  statusbar_undostruct *dropit = statusbar_undotop;
  while (dropit) {
    statusbar_undotop = dropit->next;
    free(dropit->answerdata);
    free(dropit);
    dropit = statusbar_undotop;
  }
  /* Once all items have been deleted, reset both pointers and the last action. */
  statusbar_undotop      = NULL;
  statusbar_current_undo = NULL;
  statusbar_last_action  = STATUSBAR_OTHER;
}

/* Perform the undo at the top of the undo-stack. */
void do_statusbar_undo(void) {
  statusbar_undostruct *u = statusbar_current_undo;
  /* If there is no item at the top of the undo-stack, return. */
  if (!u) {
    return;
  }
  switch (u->type) {
    case STATUSBAR_ADD: {
      // erase_in(&answer, u->head_x, strlen(u->answerdata));
      answer   = xstr_erase(answer, u->head_x, strlen(u->answerdata));
      typing_x = u->head_x;
      break;
    }
    case STATUSBAR_BACK:
    case STATUSBAR_DEL: {
      // inject_in(&answer, u->answerdata, u->head_x);
      answer   = xstrinj(answer, u->answerdata, u->head_x);
      typing_x = u->tail_x;
      break;
    }
    case STATUSBAR_CHOP_PREV_WORD:
    case STATUSBAR_CHOP_NEXT_WORD: {
      /* Inject the chop`ed string into the answer at the correct position. */
      // inject_in(&answer, u->answerdata, u->head_x, TRUE);
      answer = xstrinj(answer, u->answerdata, u->head_x);
      /* Set the cursor position in the status-bar correctly. */
      typing_x = u->head_x;
      if (u->type == STATUSBAR_CHOP_PREV_WORD) {
        typing_x += u->tail_x;
      }
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
void do_statusbar_redo(void) {
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
      // inject_in(&answer, u->answerdata, u->head_x);
      answer   = xstrinj(answer, u->answerdata, u->head_x);
      typing_x = u->tail_x;
      break;
    }
    case STATUSBAR_BACK:
    case STATUSBAR_DEL: {
      // erase_in(&answer, u->head_x, strlen(u->answerdata));
      answer   = xstr_erase(answer, u->head_x, strlen(u->answerdata));
      typing_x = u->head_x;
      break;
    }
    case STATUSBAR_CHOP_PREV_WORD:
    case STATUSBAR_CHOP_NEXT_WORD: {
      /* Erase the cut string`s length in the answer at the recorded position where it happend. */
      // erase_in(&answer, u->head_x, u->tail_x, TRUE);
      answer = xstr_erase(answer, u->head_x, u->tail_x);
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
void do_statusbar_home(void) {
  typing_x = 0;
}

/* Move to the end of the answer. */
void do_statusbar_end(void) {
  typing_x = strlen(answer);
}

/* Move to the previous word in the answer. */
void do_statusbar_prev_word(void) {
  bool seen_a_word = FALSE, step_forward = FALSE;
  /* Move backward until we pass over the start of a word. */
  while (typing_x) {
    typing_x = step_left(answer, typing_x);
    if (is_word_char((answer + typing_x), FALSE)) {
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
void do_statusbar_next_word(void) {
  bool seen_space = !is_word_char((answer + typing_x), FALSE);
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
void do_statusbar_left(void) {
  if (typing_x > 0) {
    typing_x = step_left(answer, typing_x);
    while (typing_x > 0 && is_zerowidth(answer + typing_x)) {
      typing_x = step_left(answer, typing_x);
    }
  }
}

/* Move right one character in the answer. */
void do_statusbar_right(void) {
  if (answer[typing_x]) {
    typing_x = step_right(answer, typing_x);
    while (answer[typing_x] && is_zerowidth(answer + typing_x)) {
      typing_x = step_right(answer, typing_x);
    }
  }
}

/* Backspace over one character in the answer.  And if `with_undo` is `TRUE`, add a undo-object. */
void do_statusbar_backspace(bool with_undo) {
  int charlen;
  if (typing_x > 0) {
    typing_x = step_left(answer, typing_x);
    if (with_undo) {
      /* Only add a new undo-object when the last undo-action was not 'STATUSBAR_BACK'. */
      if (statusbar_last_action != STATUSBAR_BACK) {
        statusbar_add_undo(STATUSBAR_BACK, NULL);
      }
      /* Otherwise, if create a new undo-object. */
      else {
        statusbar_update_undo(STATUSBAR_BACK);
      }
    }
    charlen = char_length(answer + typing_x);
    answer = xstr_erase_norealloc(answer, typing_x, charlen);
  }
}

/* Delete one character in the answer. */
void do_statusbar_delete(void) {
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

/* Insert the given short burst of bytes into the answer. */
void inject_into_answer(char *burst, Ulong count) {
  /* First encode any embedded NUL byte as 0x0A. */
  recode_NUL_to_LF(burst, count);
  /* Only add a new undo-object if the last undo-action was not also 'STATUSBAR_ADD'. */
  if (statusbar_last_action != STATUSBAR_ADD || statusbar_current_undo->tail_x != typing_x) {
    statusbar_add_undo(STATUSBAR_ADD, NULL);
  }
  answer = xstrninj(answer, burst, count, typing_x);
  typing_x += count;
  statusbar_update_undo(STATUSBAR_ADD);
}

/* Chop next word. */
void do_statusbar_chop_next_word(void) {
  Ulong steps = 0;
  Ulong was_x;
  char *cutting_section;
  /* If there is more then one whitespace to the next word, just delete the white chars until the next word. */
  if (word_more_than_one_white_away(answer, typing_x, FORWARD, &steps)) {
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
    cutting_section = measured_copy((answer + typing_x), steps);
    statusbar_add_undo(STATUSBAR_CHOP_NEXT_WORD, cutting_section);
    free(cutting_section);
    /* Now perform the cutting. */
    while (steps--) {
      statusbar_delete();
    }
  }
}

/* Chop prev word. */
void do_statusbar_chop_prev_word(void) {
  Ulong steps = 0;
  Ulong was_x;
  char *cutting_section;
  /* If there is more then one whitespace to the prev word, just delete all white chars until the prev word. */
  if (word_more_than_one_white_away(answer, typing_x, BACKWARD, &steps)) {
    typing_x -= steps;
  }
  else {
    /* Save the prompt x position. */
    was_x = typing_x;
    /* Move to the prev word. */
    do_statusbar_prev_word();
    if (was_x != typing_x) {
      /* If there was any movement, calculate the steps, then restore the prompt x position. */
      steps = (was_x - typing_x);
    }
  }
  /* Only perform the cutting action when appropriet. */
  if (steps) {
    cutting_section = measured_copy((answer + typing_x), steps);
    statusbar_add_undo(STATUSBAR_CHOP_PREV_WORD, cutting_section);
    free(cutting_section);
    /* Now do the cutting. */
    typing_x += steps;
    while (steps--) {
      do_statusbar_backspace(FALSE);
    }
  }
}

/* Return the column number of the first character of the answer that is displayed in the status bar when the cursor is at the given
 * column, with the available room for the answer starting at base.  Note that (0 <= column - get_statusbar_page_start(column) < COLS). */
Ulong get_statusbar_page_start(Ulong base, Ulong column) {
  if (column == base || (int)column < (COLS - 1)) {
    return 0;
  }
  else if (COLS > (int)(base + 2)) {
    return (column - base - 1 - (column - base - 1) % (COLS - base - 2));
  }
  else {
    return (column - 2);
  }
}

/* Reinitialize the cursor position in the answer. */
void put_cursor_at_end_of_answer(void) {
  typing_x = HIGHEST_POSITIVE;
}

/* Remove or add the pipe character at the answer's head. */
void add_or_remove_pipe_symbol_from_answer(void) {
  if (answer[0] == '|') {
    memmove(answer, (answer + 1), strlen(answer));
    if (typing_x > 0) {
      --typing_x;
    }
  }
  else {
    answer = xrealloc(answer, (strlen(answer) + 2));
    memmove((answer + 1), answer, (strlen(answer) + 1));
    answer[0] = '|';
    ++typing_x;
  }
}
