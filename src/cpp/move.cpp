#include "../include/prototypes.h"

/* Move to the first line of the file. */
// void to_first_line(void) _NOTHROW {
//   openfile->current     = openfile->filetop;
//   openfile->current_x   = 0;
//   openfile->placewewant = 0;
//   refresh_needed        = TRUE;
// }

/* Move to the last line of the file. */
// void to_last_line(void) _NOTHROW {
//   openfile->current     = openfile->filebot;
//   openfile->current_x   = (inhelp) ? 0 : strlen(openfile->filebot->data);
//   openfile->placewewant = xplustabs();
//   /* Set the last line of the screen as the target for the cursor. */
//   openfile->cursor_row = editwinrows - 1;
//   refresh_needed       = TRUE;
//   recook |= perturbed;
//   focusing = FALSE;
// }

/* Determine the actual current chunk and the target column. */
// void get_edge_and_target(Ulong *leftedge, Ulong *target_column) _NOTHROW {
//   if (ISSET(SOFTWRAP)) {
//     Ulong shim     = (editwincols * (1 + (tabsize / editwincols)));
//     *leftedge      = leftedge_for(xplustabs(), openfile->current);
//     *target_column = ((openfile->placewewant + shim - *leftedge) % editwincols);
//   }
//   else {
//     *leftedge      = 0;
//     *target_column = openfile->placewewant;
//   }
// }

/* Return the index in line->data that corresponds to the given column on the chunk that starts at the
 * given leftedge.  If the target column has landed on a tab, prevent the cursor from falling back a
 * row when moving forward, or from skipping a row when moving backward, by incrementing the index. */
static Ulong proper_x(linestruct *line, Ulong *leftedge, bool forward, Ulong column, bool *shifted) _NOTHROW {
  Ulong index = actual_x(line->data, column);
  if (ISSET(SOFTWRAP) && line->data[index] == '\t' && ((forward && wideness(line->data, index) < *leftedge)
   || (!forward && (column / tabsize) == ((*leftedge - 1) / tabsize) && (column / tabsize) < ((*leftedge + editwincols - 1) / tabsize)))) {
    ++index;
    if (shifted) {
      *shifted = TRUE;
    }
  }
  if (ISSET(SOFTWRAP)) {
    *leftedge = leftedge_for(editwincols, wideness(line->data, index), line);
  }
  return index;
}

/* Adjust the values for current_x and placewewant in case we have landed in the middle of a tab that crosses a row boundary. */
// static void set_proper_index_and_pww(Ulong *leftedge, Ulong target, bool forward) _NOTHROW {
//   Ulong was_edge = *leftedge;
//   bool  shifted  = FALSE;
//   openfile->current_x = proper_x(openfile->current, leftedge, forward, actual_last_column(*leftedge, target), &shifted);
//   /* If the index was incremented, try going to the target column. */
//   if (shifted || *leftedge < was_edge) {
//     openfile->current_x = proper_x(openfile->current, leftedge, forward, actual_last_column(*leftedge, target), &shifted);
//   }
//   openfile->placewewant = *leftedge + target;
// }

/* Move up almost one screenful. */
// void do_page_up(void) _NOTHROW {
//   int   mustmove = ((editwinrows < 3) ? 1 : editwinrows - 2);
//   Ulong leftedge, target_column;
//   /* If we're not in smooth scrolling mode, put the cursor at the beginning of the top line of the edit window, as Pico does. */
//   if (ISSET(JUMPY_SCROLLING)) {
//     openfile->current    = openfile->edittop;
//     leftedge             = openfile->firstcolumn;
//     openfile->cursor_row = 0;
//     target_column        = 0;
//   }
//   else {
//     get_edge_and_target(&leftedge, &target_column);
//   }
//   /* Move up the required number of lines or chunks.  If we can't, we're at the top of the file, so put the cursor there and get out. */
//   if (go_back_chunks(mustmove, &openfile->current, &leftedge) > 0) {
//     to_first_line();
//     return;
//   }
//   set_proper_index_and_pww(&leftedge, target_column, FALSE);
//   /* Move the viewport so that the cursor stays immobile, if possible. */
//   adjust_viewport(STATIONARY);
//   refresh_needed = TRUE;
// }

/* Move down almost one screenful. */
// void do_page_down(void) _NOTHROW {
//   int   mustmove = ((editwinrows < 3) ? 1 : (editwinrows - 2));
//   Ulong leftedge, target_column;
//   /* If we're not in smooth scrolling mode, put the cursor at the beginning of the top line of the edit window, as Pico does. */
//   if (ISSET(JUMPY_SCROLLING)) {
//     openfile->current    = openfile->edittop;
//     leftedge             = openfile->firstcolumn;
//     openfile->cursor_row = 0;
//     target_column        = 0;
//   }
//   else {
//     get_edge_and_target(&leftedge, &target_column);
//   }
//   /* Move down the required number of lines or chunks.  If we can't, we're at the bottom of the file, so put the cursor there and get out. */
//   if (go_forward_chunks(mustmove, &openfile->current, &leftedge) > 0) {
//     to_last_line();
//     return;
//   }
//   set_proper_index_and_pww(&leftedge, target_column, TRUE);
//   /* Move the viewport so that the cursor stays immobile, if possible. */
//   adjust_viewport(STATIONARY);
//   refresh_needed = TRUE;
// }

/* Place the cursor on the first row in the viewport. */
// void to_top_row(void) _NOTHROW {
//   Ulong leftedge, offset;
//   get_edge_and_target(&leftedge, &offset);
//   openfile->current = openfile->edittop;
//   leftedge          = openfile->firstcolumn;
//   set_proper_index_and_pww(&leftedge, offset, FALSE);
//   place_the_cursor();
// }

/* Place the cursor on the last row in the viewport, when possible. */
// void to_bottom_row(void) _NOTHROW {
//   Ulong leftedge, offset;
//   get_edge_and_target(&leftedge, &offset);
//   openfile->current = openfile->edittop;
//   leftedge          = openfile->firstcolumn;
//   go_forward_chunks(editwinrows - 1, &openfile->current, &leftedge);
//   set_proper_index_and_pww(&leftedge, offset, TRUE);
//   place_the_cursor();
// }

/* Put the cursor line at the center, then the top, then the bottom. */
// void do_cycle(void) {
//   if (cycling_aim == 0) {
//     adjust_viewport(CENTERING);
//   }
//   else {
//     openfile->cursor_row = ((cycling_aim == 1) ? 0 : (editwinrows - 1));
//     adjust_viewport(STATIONARY);
//   }
//   cycling_aim = ((cycling_aim + 1) % 3);
//   draw_all_subwindows();
//   full_refresh();
// }

/* Scroll the line with the cursor to the center of the screen. */
// void do_center(void) {
//   /* The main loop has set 'cycling_aim' to zero. */
//   do_cycle();
// }

/* Move to the first beginning of a paragraph before the current line. */
// void do_para_begin(linestruct **line) _NOTHROW {
//   if ((*line)->prev) {
//     *line = (*line)->prev;
//   }
//   while (!begpar(*line, 0)) {
//     *line = (*line)->prev;
//   }
// }

/* Move down to the last line of the first found paragraph. */
// void do_para_end(linestruct **line) _NOTHROW {
//   while ((*line)->next && !inpar(*line)) {
//     *line = (*line)->next;
//   }
//   while ((*line)->next && inpar((*line)->next) && !begpar((*line)->next, 0)) {
//     *line = (*line)->next;
//   }
// }

/* Move up to first start of a paragraph before the current line. */
// void to_para_begin(void) {
//   linestruct *was_current = openfile->current;
//   do_para_begin(&openfile->current);
//   openfile->current_x = 0;
//   edit_redraw(was_current, CENTERING);
// }

/* Move down to just after the first found end of a paragraph. */
// void to_para_end(void) {
//   linestruct *was_current = openfile->current;
//   do_para_end(&openfile->current);
//   /* Step beyond the last line of the paragraph, if possible.  Otherwise, move to the end of the line. */
//   if (openfile->current->next) {
//     openfile->current   = openfile->current->next;
//     openfile->current_x = 0;
//   }
//   else {
//     openfile->current_x = strlen(openfile->current->data);
//   }
//   edit_redraw(was_current, FLOWING);
//   recook |= perturbed;
// }

/* Move to the preceding block of text. */
// void to_prev_block(void) {
//   linestruct *was_current = openfile->current;
//   int  cur_indent, was_indent = -1;
//   bool is_text = FALSE, seen_text = FALSE;
//   /* Skip backward until first blank line after some nonblank line(s). */
//   while (openfile->current->prev && (!seen_text || is_text)) {
//     /* Current line is empty. */
//     if (!openfile->current->data[0]) {
//       /* Find first line that is not empty. */
//       for (; openfile->current->prev && !openfile->current->data[0]; openfile->current = openfile->current->prev)
//         ;
//       openfile->current_x = indent_length(openfile->current->data);
//       edit_redraw(was_current, FLOWING);
//       return;
//     }
//     else if (is_line_comment(openfile->current)) {
//       /* If not on the first line of a '//' comment block. */
//       if (openfile->current != was_current) {
//         /* Iterate to the top of the comment. */
//         for (; openfile->current->prev && is_line_comment(openfile->current); openfile->current = openfile->current->prev)
//           ;
//         /* Back down one line. */
//         openfile->current   = openfile->current->next;
//         openfile->current_x = indent_length(openfile->current->data);
//         edit_redraw(was_current, FLOWING);
//         return;
//       }
//     }
//     cur_indent = line_indent(openfile->current);
//     if (was_indent == -1) {
//       was_indent = cur_indent;
//     }
//     /* Line indentation has changed. */
//     else if (was_indent != cur_indent) {
//       /* Place cursor at top of current indent block, unless called from it. */
//       if (openfile->current != was_current->prev) {
//         openfile->current = openfile->current->next;
//       }
//       openfile->current_x = indent_length(openfile->current->data);
//       edit_redraw(was_current, FLOWING);
//       return;
//     }
//     openfile->current = openfile->current->prev;
//     is_text   = !white_string(openfile->current->data);
//     seen_text = seen_text || is_text;
//   }
//   /* Step forward one line again if we passed text but this line is blank. */
//   if (seen_text && openfile->current->next && white_string(openfile->current->data)) {
//     openfile->current = openfile->current->next;
//   }
//   openfile->current_x = indent_length(openfile->current->data);
//   edit_redraw(was_current, FLOWING);
// }

/* Move to the next block of text. */
// void to_next_block(void) {
//   linestruct *was_current = openfile->current;
//   int cur_indent, was_indent = -1;
//   bool is_white   = white_string(openfile->current->data);
//   bool seen_white = is_white;
//   /* Skip forward until first nonblank line after some blank line(s). */
//   while (openfile->current->next && (!seen_white || is_white)) {
//     /* Current line is empty and is not starting line. */
//     if (!openfile->current->data[0] && openfile->current != was_current) {
//       /* When line after calling line is comment. */
//       if (openfile->current == was_current->next) {
//         /* Find first not empty line. */
//         for (; openfile->current->next && !openfile->current->data[0]; openfile->current = openfile->current->next);
//       }
//       /* Stop at the end of text block when we see the first empty line. */
//       else {
//         openfile->current = openfile->current->prev;
//       }
//       openfile->current_x = indent_length(openfile->current->data);
//       edit_redraw(was_current, FLOWING);
//       recook |= perturbed;
//       return;
//     }
//     else if (is_line_comment(openfile->current)) {
//       if (openfile->current != was_current) {
//         for (; openfile->next && is_line_comment(openfile->current); openfile->current = openfile->current->next)
//           ;
//         openfile->current   = openfile->current->prev;
//         openfile->current_x = indent_length(openfile->current->data);
//         edit_redraw(was_current, FLOWING);
//         recook |= perturbed;
//         return;
//       }
//     }
//     cur_indent = line_indent(openfile->current);
//     if (was_indent == -1) {
//       was_indent = cur_indent;
//     }
//     /* Line indentation has changed. */
//     else if (was_indent != cur_indent) {
//       /* Place cursor at bottom of indent block, unless called from it. */
//       if (openfile->current != was_current->next) {
//         openfile->current = openfile->current->prev;
//       }
//       openfile->current_x = indent_length(openfile->current->data);
//       edit_redraw(was_current, FLOWING);
//       recook |= perturbed;
//       return;
//     }
//     openfile->current = openfile->current->next;
//     is_white   = white_string(openfile->current->data);
//     seen_white = seen_white || is_white;
//   }
//   openfile->current_x = indent_length(openfile->current->data);
//   edit_redraw(was_current, FLOWING);
//   recook |= perturbed;
// }

/* Move to the previous word. */
void do_prev_word(void) _NOTHROW {
  bool punctuation_as_letters = ISSET(WORD_BOUNDS);
  bool seen_a_word  = FALSE;
  bool step_forward = FALSE;
  /* Move backward until we pass over the start of a word. */
  while (TRUE) {
    /* If at the head of a line, move to the end of the preceding one. */
    if (!openfile->current_x) {
      /* If we are at the first line, do nothing. */
      if (!openfile->current->prev) {
        break;
      }
      openfile->current   = openfile->current->prev;
      openfile->current_x = strlen(openfile->current->data);
      break;
    }
    /* Step back one character. */
    openfile->current_x = step_left(openfile->current->data, openfile->current_x);
    if (is_cursor_word_char(punctuation_as_letters) || is_cursor_language_word_char()) {
      seen_a_word = TRUE;
      /* If at the head of a line now, this surely is a word start. */
      if (!openfile->current_x) {
        break;
      }
    }
    else if (is_cursor_zerowidth()) {
      ; /* Do nothing. */
    }
    else if (seen_a_word) {
      /* This is space now: we've overshot the start of the word. */
      step_forward = TRUE;
      break;
    }
  }
  if (step_forward) {
    /* Move one character forward again to sit on the start of the word. */
    openfile->current_x = step_right(openfile->current->data, openfile->current_x);
  }
}

/* Move to the next word.  If after_ends is 'TRUE', stop at the ends of words
 * instead of at their beginnings.  If 'after_ends' is 'TRUE', stop at the ends
 * of words instead of at their beginnings. Return 'TRUE' if we started on a word. */
bool do_next_word(bool after_ends) _NOTHROW {
  bool punct_as_letters = ISSET(WORD_BOUNDS);
  bool started_on_word  = (is_cursor_word_char(punct_as_letters) || is_cursor_language_word_char());
  bool seen_space       = !started_on_word;
  bool seen_word        = started_on_word;
  Uint i = 0;
  /* Move forward until we reach the start of a word. */
  while (TRUE) {
    /* If at the end of a line, move to the beginning of the next one. */
    if (!openfile->current->data[openfile->current_x]) {
      /* If not called starting at eof, stop here. */
      if (i != 0) {
        break;
      }
      /* When at end of file, stop. */
      if (!openfile->current->next) {
        break;
      }
      openfile->current   = openfile->current->next;
      openfile->current_x = 0;
      if (after_ends) {
        /* If we stop after the end of words then then the first iter will be at 0 were we reach '\0'. */
        if (i == 0) {
          openfile->current_x = indent_length(openfile->current->data);
          break;
        }
      }
      seen_space = TRUE;
    }
    else {
      /* Step forward one character. */
      openfile->current_x = step_right(openfile->current->data, openfile->current_x);
    }
    if (after_ends) {
      /* If this is a word character, continue. */
      if (is_cursor_word_char(punct_as_letters) || is_cursor_language_word_char()) {
        seen_word = TRUE;
      }
      else if (is_cursor_zerowidth()) {
        ; /* Skip */
      }
      /* Else, it's a separator, and if we've already seen a word, then it's a word end. */
      else if (seen_word) {
        break;
      }
    }
    else {
      if (is_cursor_zerowidth()) {
        ;
      }
      else {
        /* If this is not a word character, then it's a separator. */
        if (!is_cursor_word_char(punct_as_letters)) {
          seen_space = TRUE;
        }
        /* Else, if we've already seen a separator, then it's a word start. */
        else if (seen_space) {
          break;
        }
      }
    }
    ++i;
  }
  return started_on_word;
}

/* Move to the previous word in the file, and update the screen afterwards. */
void to_prev_word(void) {
  linestruct *was_current = openfile->current;
  do_prev_word();
  edit_redraw(was_current, FLOWING);
}

/* Move to the next word in the file. If the AFTER_ENDS flag is set, stop at word ends instead of beginnings.  Update the screen afterwards. */
void to_next_word(void) {
  linestruct *was_current = openfile->current;
  do_next_word(ISSET(AFTER_ENDS));
  edit_redraw(was_current, FLOWING);
}

/* Move to the beginning of the current line (or softwrapped chunk).  When enabled, do a smart home.
 * When softwrapping, go the beginning of the full line when already at the start of a chunk. */
void do_home(void) {
  linestruct *was_current = openfile->current;
  bool moved_off_chunk, moved;
  Ulong was_column, leftedge, left_x, cur_indent;
  moved_off_chunk = TRUE;
  moved           = FALSE;
  /* Save current column position. */
  was_column = xplustabs();
  /* Save current indent of line. */
  cur_indent = indent_length(openfile->current->data);
  if (ISSET(SOFTWRAP)) {
    leftedge = leftedge_for(editwincols, was_column, openfile->current);
    left_x   = proper_x(openfile->current, &leftedge, FALSE, leftedge, NULL);
  }
  if (ISSET(SMART_HOME)) {
    Ulong indent_x = indent_length(openfile->current->data);
    if (openfile->current->data[indent_x]) {
      /* If we're exactly on the indent, move fully home.  Otherwise, when not softwrapping
       * or not after the first nonblank chunk, move to the first nonblank character. */
      if (openfile->current_x == indent_x) {
        openfile->current_x = 0;
        moved               = TRUE;
      }
      else if (left_x <= indent_x) {
        openfile->current_x = indent_x;
        moved               = TRUE;
      }
    }
  }
  if (!moved && ISSET(SOFTWRAP)) {
    /* If already at the left edge of the screen, move fully home. Otherwise, move to the left edge. */
    if (openfile->current_x == left_x) {
      openfile->current_x = 0;
    }
    else {
      openfile->current_x   = left_x;
      openfile->placewewant = leftedge;
      moved_off_chunk       = FALSE;
    }
  }
  else if (!moved) {
    /* If column and indent is the same move to begining. */
    if (openfile->current_x == cur_indent) {
      openfile->current_x = 0;
    }
    /* Else move to (indent/first char after indent) of line. */
    else {
      openfile->current_x = cur_indent;
    }
  }
  if (moved_off_chunk) {
    openfile->placewewant = xplustabs();
  }
  /* Return early here when using the gui. */
  if (ISSET(USING_GUI)) {
    return;
  }
  /* If we changed chunk, we might be offscreen.  Otherwise, update current if the mark is on or we changed 'page'. */
  if (ISSET(SOFTWRAP) && moved_off_chunk) {
    edit_redraw(was_current, FLOWING);
  }
  else if (line_needs_update(was_column, openfile->placewewant)) {
    update_line_curses(openfile->current, openfile->current_x);
  }
}

/* Move to the end of the current line (or softwrapped 'chunk').  When softwrapping and already at the end of a 'chunk', go to the end of the full line. */
void do_end(void) {
  bool  moved_off_chunk, kickoff, last_chunk;
  Ulong was_column, line_len, leftedge, rightedge, right_x;
  linestruct *was_current;
  was_current = openfile->current;
  was_column  = xplustabs();
  line_len    = strlen(openfile->current->data);
  moved_off_chunk = TRUE;
  if (ISSET(SOFTWRAP)) {
    kickoff    = TRUE;
    last_chunk = FALSE;
    leftedge   = leftedge_for(editwincols, was_column, openfile->current);
    rightedge  = get_softwrap_breakpoint(openfile->current->data, leftedge, &kickoff, &last_chunk, editwincols);
    /* If we're on the last chunk, we're already at the end of the line.  Otherwise, we're one column past the end of the line.
     * Shifting backwards one column might put us in the middle of a multi-column character, but actual_x() will fix that. */
    if (!last_chunk) {
      --rightedge;
    }
    right_x = actual_x(openfile->current->data, rightedge);
    /* If already at the right edge of the screen, move fully to the end of the line.  Otherwise, move to the right edge. */
    if (openfile->current_x == right_x) {
      openfile->current_x = line_len;
    }
    else {
      openfile->current_x   = right_x;
      openfile->placewewant = rightedge;
      moved_off_chunk       = FALSE;
    }
  }
  else {
    openfile->current_x = line_len;
  }
  if (moved_off_chunk) {
    openfile->placewewant = xplustabs();
  }
  /* Return early here when using the gui. */
  if (ISSET(USING_GUI)) {
    return;
  }
  /* If we changed chunk, we might be offscreen.  Otherwise, update current if the mark is on or we changed "page". */
  if (ISSET(SOFTWRAP) && moved_off_chunk) {
    edit_redraw(was_current, FLOWING);
  }
  else if (line_needs_update(was_column, openfile->placewewant)) {
    update_line_curses(openfile->current, openfile->current_x);
  }
}

/* Move the cursor to the preceding line or chunk. */
// void do_up(void) {
//   linestruct *was_current = openfile->current;
//   Ulong leftedge, target_column;
//   get_edge_and_target(&leftedge, &target_column);
//   /* If we can't move up one line or chunk, we're at top of file. */
//   if (go_back_chunks(1, &openfile->current, &leftedge) > 0) {
//     return;
//   }
//   set_proper_index_and_pww(&leftedge, target_column, FALSE);
//   if (!openfile->cursor_row && !ISSET(JUMPY_SCROLLING) && (tabsize < editwincols || !ISSET(SOFTWRAP))) {
//     edit_scroll(BACKWARD);
//   }
//   else {
//     edit_redraw(was_current, FLOWING);
//   }
//   /* <Up> should not change placewewant, so restore it. */
//   openfile->placewewant = (leftedge + target_column);
// }

/* Move the cursor to next line or chunk. */
// void do_down(void) {
//   linestruct *was_current = openfile->current;
//   Ulong leftedge, target_column;
//   get_edge_and_target(&leftedge, &target_column);
//   /* If we can't move down one line or chunk, we're at bottom of file. */
//   if (go_forward_chunks(1, &openfile->current, &leftedge) > 0) {
//     return;
//   }
//   set_proper_index_and_pww(&leftedge, target_column, TRUE);
//   if (openfile->cursor_row == (editwinrows - 1) && !ISSET(JUMPY_SCROLLING) && (tabsize < editwincols || !ISSET(SOFTWRAP))) {
//     edit_scroll(FORWARD);
//   }
//   else {
//     edit_redraw(was_current, FLOWING);
//   }
//   /* <Down> should not change placewewant, so restore it. */
//   openfile->placewewant = (leftedge + target_column);
// }

/* Scroll up one line or chunk without moving the cursor textwise. */
void do_scroll_up(void) {
  /* When the top of the file is onscreen, we can't scroll. */
  if (!openfile->edittop->prev && !openfile->firstcolumn) {
    return;
  }
  if (openfile->cursor_row == (editwinrows - 1)) {
    do_up();
  }
  if (editwinrows > 1) {
    edit_scroll(BACKWARD);
  }
}

/* Scroll down one line or chunk without moving the cursor textwise. */
void do_scroll_down(void) {
  if (openfile->cursor_row == 0) {
    do_down();
  }
  if (editwinrows > 1
   && (openfile->edittop->next != NULL || (ISSET(SOFTWRAP) && (extra_chunks_in(openfile->edittop, editwincols) > chunk_for(openfile->firstcolumn, openfile->edittop, editwincols))))) {
    edit_scroll(FORWARD);
  }
}

/* Move left one character. */
// void do_left(void) {
//   linestruct *was_current = openfile->current;
//   /* If a section is highlighted and shift is not held, then place the cursor at the left side of the marked area. */
//   if (openfile->mark && !shift_held) {
//     if (mark_is_before_cursor()) {
//       openfile->current   = openfile->mark;
//       openfile->current_x = openfile->mark_x;
//     }
//   }
//   else {
//     if (openfile->current_x > 0) {
//       openfile->current_x = step_left(openfile->current->data, openfile->current_x);
//       while (openfile->current_x > 0 && is_zerowidth(openfile->current->data + openfile->current_x)) {
//         openfile->current_x = step_left(openfile->current->data, openfile->current_x);
//       }
//     }
//     else if (openfile->current != openfile->filetop) {
//       openfile->current   = openfile->current->prev;
//       openfile->current_x = strlen(openfile->current->data);
//     }
//   }
//   edit_redraw(was_current, FLOWING);
// }

/* Move right one character. */
void do_right(void) {
  linestruct *was_current = openfile->current;
  /* If a section is highlighted and shift is not held, then place the cursor at the right side of the marked area. */
  if (openfile->mark && !shift_held) {
    if (!mark_is_before_cursor()) {
      openfile->current   = openfile->mark;
      openfile->current_x = openfile->mark_x;
    }
  }
  else {
    if (openfile->current->data[openfile->current_x]) {
      openfile->current_x = step_right(openfile->current->data, openfile->current_x);
      while (openfile->current->data[openfile->current_x] && is_zerowidth(openfile->current->data + openfile->current_x)) {
        openfile->current_x = step_right(openfile->current->data, openfile->current_x);
      }
    }
    else if (openfile->current != openfile->filebot) {
      openfile->current   = openfile->current->next;
      openfile->current_x = 0;
    }
  }
  edit_redraw(was_current, FLOWING);
}
