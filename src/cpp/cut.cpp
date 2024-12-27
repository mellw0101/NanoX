/** @file cut.cpp */
#include "../include/prototypes.h"

/* Delete the character at the current position, and add or update an undo item for the given action. */
void expunge(undo_type action) {
  openfile->placewewant = xplustabs();
  /* When in the middle of a line, delete the current character. */
  if (openfile->current->data[openfile->current_x]) {
    int   charlen    = char_length(openfile->current->data + openfile->current_x);
    Ulong line_len   = strlen(openfile->current->data + openfile->current_x);
    Ulong old_amount = (ISSET(SOFTWRAP) ? extra_chunks_in(openfile->current) : 0);
    /* If the type of action changed or the cursor moved to a different line, create a new undo item, otherwise update the existing item. */
    if (action != openfile->last_action || openfile->current->lineno != openfile->current_undo->head_lineno) {
      add_undo(action, NULL);
    }
    else {
      update_undo(action);
    }
    /* Move the remainder of the line "in", over the current character. */
    memmove(&openfile->current->data[openfile->current_x], &openfile->current->data[openfile->current_x + charlen], (line_len - charlen + 1));
    /* When softwrapping, a changed number of chunks requires a refresh. */
    if (ISSET(SOFTWRAP) && extra_chunks_in(openfile->current) != old_amount) {
      refresh_needed = TRUE;
    }
    /* Adjust the mark if it is after the cursor on the current line. */
    if (openfile->mark == openfile->current && openfile->mark_x > openfile->current_x) {
      openfile->mark_x -= charlen;
    }
  }
  /* Otherwise, when not at end of buffer, join this line with the next. */
  else if (openfile->current != openfile->filebot) {
    linestruct *joining = openfile->current->next;
    /* If there is a magic line, and we're before it: don't eat it. */
    if (joining == openfile->filebot && openfile->current_x != 0 && !ISSET(NO_NEWLINES)) {
      if (action == BACK) {
        add_undo(BACK, NULL);
      }
      return;
    }
    add_undo(action, NULL);
    /* Adjust the mark if it is on the line that will be "eaten". */
    if (openfile->mark == joining) {
      openfile->mark = openfile->current;
      openfile->mark_x += openfile->current_x;
    }
    openfile->current->has_anchor |= joining->has_anchor;
    /* Add the content of the next line to that of the current one. */
    openfile->current->data = arealloc(openfile->current->data, (strlen(openfile->current->data) + strlen(joining->data) + 1));
    constexpr_strcat(openfile->current->data, joining->data);
    unlink_node(joining);
    /* Two lines were joined, so do a renumbering and refresh the screen. */
    renumber_from(openfile->current);
    refresh_needed = TRUE;
  }
  /* We're at the end-of-file: nothing to do. */
  else {
    return;
  }
  if (!refresh_needed) {
    check_the_multis(openfile->current);
  }
  if (!refresh_needed) {
    update_line(openfile->current, openfile->current_x);
  }
  /* Adjust the file size, and remember it for a possible redo. */
  openfile->totsize--;
  openfile->current_undo->newsize = openfile->totsize;
  set_modified();
}

/* Delete the character under the cursor plus any succeeding zero-widths, or, when the mark is on and --zap is active, delete the marked region. */
void do_delete(void) {
  if (openfile->mark && ISSET(LET_THEM_ZAP)) {
    zap_text();
  }
  else {
    expunge(DEL);
    while (openfile->current->data[openfile->current_x] != '\0' && is_zerowidth(openfile->current->data + openfile->current_x)) {
      expunge(DEL);
    }
  }
}

/* Backspace over one character.  That is, move the cursor left one character, and then delete the
 * character under the cursor.  Or, when mark is on and --zap is active, delete the marked region. */
void do_backspace(void) {
  if (openfile->mark && ISSET(LET_THEM_ZAP)) {
    zap_text();
  }
  else if (openfile->current_x > 0) {
    if (suggest_len > 0) {
      suggest_len -= 2;
      suggest_buf[suggest_len] = '\0';
    }
    /* If the last char injected was a open bracket char, this means that a closing bracket was plased next to it, and
     * therefor this flag was set.  Here we check if the flag is set, and if so we delete the closing bracket as well. */
    if (last_key_was_bracket) {
      expunge(BACK);
    }
    openfile->current_x = step_left(openfile->current->data, openfile->current_x);
    expunge(BACK);
  }
  else if (openfile->current != openfile->filetop) {
    do_left();
    expunge(BACK);
  }
}

/* Return 'FALSE' when a cut command would not actually cut anything: when on an empty line at EOF, or when
 * the mark covers zero characters, or (when test_cliff is 'TRUE') when the magic line would be cut. */
static bool is_cuttable(bool test_cliff) _NO_EXCEPT {
  Ulong from = (test_cliff) ? openfile->current_x : 0;
  if ((!openfile->current->next && !openfile->current->data[from] && !openfile->mark)
   || (openfile->mark == openfile->current && openfile->mark_x == openfile->current_x)
   || (from > 0 && !ISSET(NO_NEWLINES) && !openfile->current->data[from] && openfile->current->next == openfile->filebot)) {
    statusbar(_("Nothing was cut"));
    openfile->mark = NULL;
    return FALSE;
  }
  else {
    return TRUE;
  }
}

/* Delete text from the cursor until the first start of a word to the left, or to the right when forward is 'TRUE'. */
void chop_word(bool forward) {
  /* Remember the current cursor position. */
  linestruct *is_current   = openfile->current;
  Ulong       is_current_x = openfile->current_x, steps;
  /* Remember where the cutbuffer is, then make it seem blank. */
  linestruct *is_cutbuffer = cutbuffer;
  cutbuffer                = NULL;
  /* Move the cursor to a word start, to the left or to the right.  If that word is on another line
   * and the cursor was not already on the edge of the original line, then put the cursor on that
   * edge instead, so that lines will not be joined unexpectedly.  I also made it so that if next
   * word is more than one 'tab/space' away, then just put put the cursor to remove the 'tabs/spaces'. */
  if (!forward) {
    if (word_more_than_one_white_away(FALSE, &steps)) {
      openfile->current_x -= steps;
    }
    else {
      do_prev_word();
      if (openfile->current != is_current) {
        if (is_current_x > 0) {
          openfile->current   = is_current;
          openfile->current_x = 0;
        }
        else {
          openfile->current_x = strlen(openfile->current->data);
        }
      }
    }
  }
  else {
    if (word_more_than_one_white_away(TRUE, &steps)) {
      openfile->current_x += steps;
    }
    else {
      do_next_word(ISSET(AFTER_ENDS));
      if (openfile->current != is_current && is_current->data[is_current_x]) {
        openfile->current   = is_current;
        openfile->current_x = strlen(is_current->data);
      }
    }
  }
  /* Set the mark at the start of that word. */
  openfile->mark   = openfile->current;
  openfile->mark_x = openfile->current_x;
  /* Put the cursor back where it was, so an undo will put it there too. */
  openfile->current   = is_current;
  openfile->current_x = is_current_x;
  /* Now kill the marked region and a word is gone. */
  add_undo(CUT, NULL);
  do_snip(TRUE, FALSE, FALSE);
  update_undo(CUT);
  /* Discard the cut word and restore the cutbuffer. */
  free_lines(cutbuffer);
  cutbuffer = is_cutbuffer;
}

/* Delete a word leftward. */
void chop_previous_word(void) {
  if (!openfile->current->prev && !openfile->current_x) {
    statusbar(_("Nothing was cut"));
  }
  else {
    chop_word(BACKWARD);
  }
}

/* Delete a word rightward. */
void chop_next_word(void) {
  openfile->mark = NULL;
  if (is_cuttable(TRUE)) {
    chop_word(FORWARD);
  }
}

/* Excise the text between the given two points and add it to the cutbuffer. */
void extract_segment(linestruct *top, Ulong top_x, linestruct *bot, Ulong bot_x) {
  linestruct *taken, *last;
  bool edittop_inside = (openfile->edittop->lineno >= top->lineno && openfile->edittop->lineno <= bot->lineno);
  bool same_line      = (openfile->mark == top);
  bool post_marked    = (openfile->mark && (openfile->mark->lineno > top->lineno || (same_line && openfile->mark_x > top_x)));
  bool had_anchor     = top->has_anchor;
  static bool inherited_anchor = FALSE;
  if (top == bot && top_x == bot_x) {
    return;
  }
  if (top != bot) {
    for (linestruct *line = top->next; line != bot->next; line = line->next) {
      had_anchor |= line->has_anchor;
    }
  }
  if (top == bot) {
    taken       = make_new_node(NULL);
    taken->data = measured_copy((top->data + top_x), (bot_x - top_x));
    memmove((top->data + top_x), (top->data + bot_x), (strlen(top->data + bot_x) + 1));
    last = taken;
  }
  else if (!top_x && !bot_x) {
    taken            = top;
    last             = make_new_node(NULL);
    last->data       = STRLTR_COPY_OF("");
    last->has_anchor = bot->has_anchor;
    last->prev       = bot->prev;
    bot->prev->next  = last;
    last->next       = NULL;
    bot->prev        = top->prev;
    (top->prev) ? top->prev->next = bot : openfile->filetop = bot;
    openfile->current = bot;
  }
  else {
    taken           = make_new_node(NULL);
    taken->data     = copy_of(top->data + top_x);
    taken->next     = top->next;
    top->next->prev = taken;
    top->next       = bot->next;
    (bot->next) ? bot->next->prev = top : 0;
    top->data = arealloc(top->data, (top_x + strlen(bot->data + bot_x) + 1));
    strcpy((top->data + top_x), (bot->data + bot_x));
    last              = bot;
    last->data[bot_x] = '\0';
    last->next        = NULL;
    openfile->current = top;
  }
  /* Subtract the size of the excised text from the buffer size. */
  openfile->totsize -= number_of_characters_in(taken, last);
  /* If the cutbuffer is currently empty, just move all the text directly
   * into it; otherwise, append the text to what is already there. */
  if (!cutbuffer) {
    cutbuffer        = taken;
    cutbottom        = last;
    inherited_anchor = taken->has_anchor;
  }
  else {
    cutbottom->data = arealloc(cutbottom->data, (strlen(cutbottom->data) + strlen(taken->data) + 1));
    strcat(cutbottom->data, taken->data);
    cutbottom->has_anchor = taken->has_anchor && !inherited_anchor;
    inherited_anchor |= taken->has_anchor;
    cutbottom->next = taken->next;
    delete_node(taken);
    if (cutbottom->next) {
      cutbottom->next->prev = cutbottom;
      cutbottom = last;
    }
  }
  openfile->current_x = top_x;
  openfile->current->has_anchor = had_anchor;
  if (post_marked || same_line) {
    openfile->mark = openfile->current;
  }
  if (post_marked) {
    openfile->mark_x = openfile->current_x;
  }
  if (openfile->filebot == bot) {
    openfile->filebot = openfile->current;
  }
  renumber_from(openfile->current);
  /* When the beginning of the viewport was inside the excision, adjust. */
  if (edittop_inside) {
    adjust_viewport(STATIONARY);
    refresh_needed = TRUE;
  }
  /* If the text doesn't end with a newline, and it should, add one. */
  if (!ISSET(NO_NEWLINES) && openfile->filebot->data[0]) {
    new_magicline();
  }
}

/* Meld the buffer that starts at topline into the current file buffer at the current cursor position. */
void ingraft_buffer(linestruct *topline) _NO_EXCEPT {
  linestruct *line    = openfile->current;
  linestruct *botline = topline;
  Ulong length       = strlen(line->data);
  Ulong extralen     = strlen(topline->data);
  Ulong xpos         = openfile->current_x;
  char *tailtext     = copy_of(line->data + xpos);
  bool  mark_follows = (openfile->mark == line && !mark_is_before_cursor());
  while (botline->next) {
    botline = botline->next;
  }
  /* Add the size of the text to be grafted to the buffer size. */
  openfile->totsize += number_of_characters_in(topline, botline);
  if (topline != botline) {
    length = xpos;
  }
  if (extralen > 0) {
    /* Insert the text of topline at the current cursor position. */
    line->data = arealloc(line->data, (length + extralen + 1));
    memmove((line->data + xpos + extralen), (line->data + xpos), (length - xpos + 1));
    constexpr_strncpy(line->data + xpos, topline->data, extralen);
  }
  if (topline != botline) {
    /* When inserting at end-of-buffer, update the relevant pointer. */
    if (!line->next) {
      openfile->filebot = botline;
    }
    line->data[xpos + extralen] = '\0';
    /* Hook the grafted lines in after the current one. */
    botline->next = openfile->current->next;
    if (botline->next) {
      botline->next->prev = botline;
    }
    openfile->current->next = topline->next;
    topline->next->prev     = openfile->current;
    /* Add the text after the cursor position at the end of botline. */
    length   = strlen(botline->data);
    extralen = strlen(tailtext);
    botline->data = arealloc(botline->data, (length + extralen + 1));
    constexpr_strcpy((botline->data + length), tailtext);
    /* Put the cursor at the end of the grafted text. */
    openfile->current   = botline;
    openfile->current_x = length;
  }
  else {
    openfile->current_x += extralen;
  }
  /* When needed, update the mark's pointer and position. */
  if (mark_follows && topline != botline) {
    openfile->mark = botline;
    openfile->mark_x += (length - xpos);
  }
  else if (mark_follows) {
    openfile->mark_x += extralen;
  }
  delete_node(topline);
  free(tailtext);
  renumber_from(line);
  /* If the text doesn't end with a newline, and it should, add one. */
  if (!ISSET(NO_NEWLINES) && openfile->filebot->data[0]) {
    new_magicline();
  }
}

/* Meld a copy of the given buffer into the current file buffer. */
void copy_from_buffer(linestruct *somebuffer) {
  Ulong       threshold = (openfile->edittop->lineno + editwinrows - 1);
  linestruct *the_copy  = copy_buffer(somebuffer);
  ingraft_buffer(the_copy);
  if (openfile->current->lineno > threshold || ISSET(SOFTWRAP)) {
    recook = TRUE;
  }
  else {
    perturbed = TRUE;
  }
}

/* Move all marked text from the current buffer into the cutbuffer. */
void cut_marked_region(void) {
  linestruct *top, *bot;
  Ulong top_x, bot_x;
  get_region(&top, &top_x, &bot, &bot_x);
  extract_segment(top, top_x, bot, bot_x);
  openfile->placewewant = xplustabs();
}

/* Move text from the current buffer into the cutbuffer.  If until_eof is 'TRUE', move all text from the current cursor position
 * to the end of the file into the cutbuffer.  If append is 'TRUE' (when zapping), always append the cut to the cutbuffer. */
void do_snip(bool marked, bool until_eof, bool append) {
  PROFILE_FUNCTION;
  linestruct *line = openfile->current;
  keep_cutbuffer &= (openfile->last_action != COPY);
  /* If cuts were not continuous, or when cutting a region, clear the slate. */
  if ((marked || until_eof || !keep_cutbuffer) && !append) {
    free_lines(cutbuffer);
    cutbuffer = NULL;
  }
  /* Now move the relevant piece of text into the cutbuffer. */
  if (until_eof) {
    extract_segment(openfile->current, openfile->current_x, openfile->filebot, strlen(openfile->filebot->data));
  }
  else if (openfile->mark) {
    cut_marked_region();
    openfile->mark = NULL;
  }
  else if (ISSET(CUT_FROM_CURSOR)) {
    /* When not at the end of a line, move the rest of this line into the cutbuffer.  Otherwise,
     * when not at the end of the buffer, move just the "line separator" into the cutbuffer. */
    if (line->data[openfile->current_x]) {
      extract_segment(line, openfile->current_x, line, strlen(line->data));
    }
    else if (openfile->current != openfile->filebot) {
      extract_segment(line, openfile->current_x, line->next, 0);
      openfile->placewewant = xplustabs();
    }
  }
  else {
    /* When not at end-of-buffer, move one full line into the cutbuffer; otherwise, move all text until end-of-line into the cutbuffer. */
    if (openfile->current != openfile->filebot) {
      extract_segment(line, 0, line->next, 0);
    }
    else {
      extract_segment(line, 0, line, strlen(line->data));
    }
    openfile->placewewant = 0;
  }
  /* After a line operation, future ones should add to the cutbuffer. */
  keep_cutbuffer = !marked && !until_eof;
  set_modified();
  refresh_needed = TRUE;
  perturbed      = TRUE;
}

/* Move text from the current buffer into the cutbuffer. */
void cut_text(void) {
  if (!is_cuttable(ISSET(CUT_FROM_CURSOR) && !openfile->mark)) {
    return;
  }
  /* Only add a new undo item when the current item is not a CUT or when
   * the current cut is not contiguous with the previous cutting. */
  if (openfile->last_action != CUT || !keep_cutbuffer) {
    keep_cutbuffer = FALSE;
    add_undo(CUT, NULL);
  }
  do_snip((openfile->mark != NULL), FALSE, FALSE);
  update_undo(CUT);
  wipe_statusbar();
}

/* Cut from the current cursor position to the end of the file. */
void cut_till_eof(void) {
  ran_a_tool = TRUE;
  if (!openfile->current->data[openfile->current_x]
   && (!openfile->current->next || (!ISSET(NO_NEWLINES) && openfile->current_x > 0 && openfile->current->next == openfile->filebot))) {
    statusbar(_("Nothing was cut"));
    return;
  }
  add_undo(CUT_TO_EOF, NULL);
  do_snip(FALSE, TRUE, FALSE);
  update_undo(CUT_TO_EOF);
  wipe_statusbar();
}

/* Erase text (current line or marked region), sending it into oblivion. */
void zap_text(void) {
  /* Remember the current cutbuffer so it can be restored after the zap. */
  linestruct *was_cutbuffer = cutbuffer;
  if (!is_cuttable(ISSET(CUT_FROM_CURSOR) && !openfile->mark)) {
    return;
  }
  /* Add a new undo item only when the current item is not a ZAP or when
   * the current zap is not contiguous with the previous zapping. */
  if (openfile->last_action != ZAP || !keep_cutbuffer) {
    add_undo(ZAP, NULL);
  }
  /* Use the cutbuffer from the ZAP undo item, so the cut can be undone. */
  cutbuffer = openfile->current_undo->cutbuffer;
  do_snip((openfile->mark != NULL), FALSE, TRUE);
  update_undo(ZAP);
  wipe_statusbar();
  cutbuffer = was_cutbuffer;
}

/* Make a copy of the marked region, putting it in the cutbuffer. */
void copy_marked_region(void) {
  linestruct *topline, *botline, *afterline;
  char       *was_datastart, saved_byte;
  Ulong       top_x, bot_x;
  get_region(&topline, &top_x, &botline, &bot_x);
  openfile->last_action = OTHER;
  keep_cutbuffer = FALSE;
  refresh_needed = TRUE;
  if (topline == botline && top_x == bot_x) {
    statusbar(_("Copied nothing"));
    return;
  }
  /* Make the area that was marked look like a separate buffer. */
  afterline            = botline->next;
  botline->next        = NULL;
  saved_byte           = botline->data[bot_x];
  botline->data[bot_x] = '\0';
  was_datastart        = topline->data;
  topline->data += top_x;
  cutbuffer = copy_buffer(topline);
  /* Restore the proper state of the buffer. */
  topline->data        = was_datastart;
  botline->data[bot_x] = saved_byte;
  botline->next        = afterline;
}

/* Copy text from the current buffer into the cutbuffer.  The text is either the marked region, the whole line,
 * the text from cursor to end-of-line, just the line break, or nothing, depending on mode and cursor position. */
void copy_text(void) {
  bool  at_eol = !openfile->current->data[openfile->current_x];
  Ulong from_x = ((ISSET(CUT_FROM_CURSOR)) ? openfile->current_x : 0);
  bool  sans_newline = (ISSET(NO_NEWLINES) && !openfile->current->next);
  linestruct *was_current = openfile->current;
  linestruct *addition;
  if (openfile->mark || openfile->last_action != COPY) {
    keep_cutbuffer = FALSE;
  }
  if (!keep_cutbuffer) {
    free_lines(cutbuffer);
    cutbuffer = NULL;
  }
  wipe_statusbar();
  if (openfile->mark) {
    copy_marked_region();
    return;
  }
  /* When at the very end of the buffer, there is nothing to do. */
  if (!openfile->current->next && at_eol && (ISSET(CUT_FROM_CURSOR) || !openfile->current_x || cutbuffer)) {
    statusbar(_("Copied nothing"));
    return;
  }
  addition       = make_new_node(NULL);
  addition->data = copy_of(openfile->current->data + from_x);
  if (ISSET(CUT_FROM_CURSOR)) {
    sans_newline = !at_eol;
  }
  /* Create the cutbuffer OR add to it, depending on the mode, the position of the cursor, and whether or not the cutbuffer is currently empty. */
  if (!cutbuffer && sans_newline) {
    cutbuffer = addition;
    cutbottom = addition;
  }
  else if (!cutbuffer) {
    cutbuffer       = addition;
    cutbottom       = make_new_node(cutbuffer);
    cutbottom->data = copy_of("");
    cutbuffer->next = cutbottom;
  }
  else if (sans_newline) {
    addition->prev       = cutbottom->prev;
    addition->prev->next = addition;
    delete_node(cutbottom);
    cutbottom = addition;
  }
  else if (ISSET(CUT_FROM_CURSOR)) {
    addition->prev  = cutbottom;
    cutbottom->next = addition;
    cutbottom       = addition;
  }
  else {
    addition->prev       = cutbottom->prev;
    addition->prev->next = addition;
    addition->next       = cutbottom;
    cutbottom->prev      = addition;
  }
  // /* When needed and possible, move the cursor to the next line. */
  // if ((!ISSET(CUT_FROM_CURSOR) || at_eol) && openfile->current->next) {
  //   openfile->current   = openfile->current->next;
  //   openfile->current_x = 0;
  // }
  // else {
  //   openfile->current_x = strlen(openfile->current->data);
  // }
  edit_redraw(was_current, FLOWING);
  openfile->last_action = COPY;
  // keep_cutbuffer        = TRUE;
}

/* Copy text from the cutbuffer into the current buffer. */
void paste_text(void) {
  /* Remember where the paste started. */
  linestruct *was_current  = openfile->current;
  bool        had_anchor   = was_current->has_anchor;
  long        was_lineno   = openfile->current->lineno;
  Ulong       was_leftedge = 0;
  if (!cutbuffer) {
    statusline(AHEM, _("Cutbuffer is empty"));
    return;
  }
  add_undo(PASTE, NULL);
  if (ISSET(SOFTWRAP)) {
    was_leftedge = leftedge_for(xplustabs(), openfile->current);
  }
  /* Add a copy of the text in the cutbuffer to the current buffer at the current cursor position. */
  copy_from_buffer(cutbuffer);
  /* Wipe any anchors in the pasted text, so that they don't proliferate. */
  for (linestruct *line = was_current; line != openfile->current->next; line = line->next) {
    line->has_anchor = FALSE;
  }
  was_current->has_anchor = had_anchor;
  update_undo(PASTE);
  /* When still on the same line and doing hard-wrapping, limit the width. */
  if (openfile->current == was_current && ISSET(BREAK_LONG_LINES)) {
    do_wrap();
  }
  /* If we pasted less than a screenful, don't center the cursor. */
  if (less_than_a_screenful(was_lineno, was_leftedge)) {
    focusing = FALSE;
  }
  else {
    precalc_multicolorinfo();
  }
  /* Set the desired x position to where the pasted text ends. */
  openfile->placewewant = xplustabs();
  set_modified();
  wipe_statusbar();
  refresh_needed = TRUE;
}
