/** @file cut.c

  @author  Melwin Svensson.
  @date    4-6-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Return `FALSE` when a cut command would not actually cut anything: when on an empty line at EOF, or when
 * the mark covers zero characters, or (when `test_cliff` is `TRUE`) when the magic line would be cut. */
static bool is_cuttable_for(openfilestruct *const file, bool test_cliff) {
  ASSERT(file);
  Ulong from = ((test_cliff) ? file->current_x : 0);
  if ((!file->current->next && !file->current->data[from] && !file->mark)
   || (file->mark == file->current && file->mark_x == file->current_x)
   || (from > 0 && !ISSET(NO_NEWLINES) && !file->current->data[from] && file->current->next == file->filebot)) {
    statusbar_all(_("Nothing was cut"));
    file->mark = NULL;
    return FALSE;
  }
  else {
    return TRUE;
  }
}

/* Returns `FALSE` when a cut command would not actually cut anything: when on an empty line at EOF, or when
 * the mark covers zero characters, or (when `test_cliff` is `TRUE`) when the magic line would be cut. */
_UNUSED static bool is_cuttable(bool test_cliff) {
  return is_cuttable_for(CONTEXT_OPENFILE, test_cliff);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Expunge ----------------------------- */

/* Delete the character at the current position, and add or update an undo item for the given action. */
void expunge_for(openfilestruct *const file, int cols, undo_type action) {
  ASSERT(file);
  int charlen;
  Ulong line_len;
  Ulong old_amount;
  linestruct *joining;
  set_pww_for(file);
  /* When in the middle of a line, delete the current character. */
  if (file->current->data[file->current_x]) {
    charlen    = char_length(file->current->data + file->current_x);
    line_len   = strlen(file->current->data + file->current_x);
    old_amount = (ISSET(SOFTWRAP) ? extra_chunks_in(file->current, cols) : 0);
    /* If the type of action changed or the cursor moved to a different line, create a new undo item, otherwise update the existing item. */
    if (action != file->last_action || file->current->lineno != file->current_undo->head_lineno) {
      add_undo_for(file, action, NULL);
    }
    else {
      update_undo_for(file, action);
    }
    /* Move the remainder of the line "in", over the current character. */
    memmove(&file->current->data[file->current_x], &file->current->data[file->current_x + charlen], (line_len - charlen + 1));
    /* When softwrapping, a changed number of chunks requires a refresh. */
    if (ISSET(SOFTWRAP) && extra_chunks_in(file->current, cols) != old_amount) {
      refresh_needed = TRUE;
    }
    /* Adjust the mark if it is after the cursor on the current line. */
    if (file->mark == file->current && file->mark_x > file->current_x) {
      file->mark_x -= charlen;
    }
  }
  /* Otherwise, when not at end of buffer, join this line with the next. */
  else if (file->current != file->filebot) {
    joining = file->current->next;
    /* If there is a magic line, and we're before it: don't eat it. */
    if (joining == file->filebot && file->current_x && !ISSET(NO_NEWLINES)) {
      if (action == BACK) {
        add_undo_for(file, BACK, NULL);
      }
      return;
    }
    add_undo_for(file, action, NULL);
    /* Adjust the mark if it is on the line that will be "eaten". */
    if (file->mark == joining) {
      file->mark    = file->current;
      file->mark_x += file->current_x;
    }
    file->current->has_anchor |= joining->has_anchor;
    /* Add the content of the next line to that of the current one. */
    file->current->data = xstrcat(file->current->data, joining->data);
    unlink_node_for(file, joining);
    /* Two lines were joined, so do a renumbering and refresh the screen. */
    renumber_from(file->current);
    refresh_needed = TRUE;
  }
  /* We're at the end-of-file: nothing to do. */
  else {
    return;
  }
  if (!refresh_needed) {
    check_the_multis_for(file, file->current);
  }
  if (!refresh_needed && !ISSET(USING_GUI)) {
    update_line_curses_for(file, file->current, file->current_x);
  }
  /* Adjust the file size, and remember it for a possible redo. */
  --file->totsize;
  file->current_undo->newsize = file->totsize;
  set_modified_for(file);
}

/* Delete the character at the current position, and add or update an undo item for the given action. */
void expunge(undo_type action) {
  if (IN_GUI_CONTEXT) {
    expunge_for(openeditor->openfile, openeditor->cols, action);
  }
  else {
    expunge_for(openfile, editwincols, action);
  }
}

/* ----------------------------- Extract segment ----------------------------- */

/* Excise the text between the given two points and add it to the cutbuffer. */
void extract_segment_for(openfilestruct *const file, int rows, int cols, linestruct *const top, Ulong top_x, linestruct *const bot, Ulong bot_x) {
  ASSERT(file);
  ASSERT(top);
  ASSERT(bot);
  static bool inherited_anchor = FALSE;
  linestruct *taken;
  linestruct *last;
  bool edittop_inside = (file->edittop->lineno >= top->lineno && file->edittop->lineno <= bot->lineno);
  bool same_line      = (file->mark == top);
  bool post_marked    = (file->mark && (file->mark->lineno > top->lineno || (same_line && file->mark_x > top_x)));
  bool had_anchor     = top->has_anchor;
  if (top == bot && top_x == bot_x) {
    return;
  }
  if (top != bot) {
    DLIST_FOR_NEXT_END(top->next, bot->next, line) {
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
    last->data       = COPY_OF("");
    last->has_anchor = bot->has_anchor;
    last->prev       = bot->prev;
    bot->prev->next  = last;
    last->next       = NULL;
    bot->prev        = top->prev;
    (top->prev) ? (top->prev->next = bot) : (file->filetop = bot);
    file->current = bot;
  }
  else {
    taken           = make_new_node(NULL);
    taken->data     = copy_of(top->data + top_x);
    taken->next     = top->next;
    top->next->prev = taken;
    top->next       = bot->next;
    (bot->next) ? bot->next->prev = top : 0;
    top->data = xrealloc(top->data, (top_x + strlen(bot->data + bot_x) + 1));
    strcpy((top->data + top_x), (bot->data + bot_x));
    last              = bot;
    last->data[bot_x] = '\0';
    last->next        = NULL;
    file->current = top;
  }
  /* Subtract the size of the excised text from the buffer size. */
  file->totsize -= number_of_characters_in(taken, last);
  /* If the cutbuffer is currently empty, just move all the text directly into it; otherwise, append the text to what is already there. */
  if (!cutbuffer) {
    cutbuffer        = taken;
    cutbottom        = last;
    inherited_anchor = taken->has_anchor;
  }
  else {
    cutbottom->data = xrealloc(cutbottom->data, (strlen(cutbottom->data) + strlen(taken->data) + 1));
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
  file->current_x = top_x;
  file->current->has_anchor = had_anchor;
  if (post_marked || same_line) {
    file->mark = file->current;
  }
  if (post_marked) {
    file->mark_x = file->current_x;
  }
  if (file->filebot == bot) {
    file->filebot = file->current;
  }
  renumber_from(file->current);
  /* When the beginning of the viewport was inside the excision, adjust. */
  if (edittop_inside) {
    adjust_viewport_for(file, STATIONARY, rows, cols);
    refresh_needed = TRUE;
  }
  /* If the text doesn't end with a newline, and it should, add one. */
  if (!ISSET(NO_NEWLINES) && file->filebot->data[0]) {
    new_magicline_for(file);
  }
}

/* Excise the text between the given two points and add it to the cutbuffer. */
void extract_segment(linestruct *const top, Ulong top_x, linestruct *const bot, Ulong bot_x) {
  if (IN_GUI_CONTEXT) {
    extract_segment_for(GUI_CONTEXT, top, top_x, bot, bot_x);
  }
  else {
    extract_segment_for(TUI_CONTEXT, top, top_x, bot, bot_x);
  }
}

/* ----------------------------- Cut marked region ----------------------------- */

/* Move all marked text from the current buffer into the cutbuffer. */
void cut_marked_region_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  linestruct *top;
  linestruct *bot;
  Ulong top_x;
  Ulong bot_x;
  get_region_for(file, &top, &top_x, &bot, &bot_x);
  extract_segment_for(file, rows, cols, top, top_x, bot, bot_x);
  set_pww_for(file);
}

/* Move all marked text from the current buffer into the cutbuffer. */
void cut_marked_region(void) {
  if (IN_GUI_CONTEXT) {
    cut_marked_region_for(GUI_CONTEXT);
  }
  else {
    cut_marked_region_for(TUI_CONTEXT);
  }
}

/* ----------------------------- Do snip ----------------------------- */

/* Move text from the current buffer into the cutbuffer.  If until_eof is 'TRUE', move all text from the current cursor position
 * to the end of the file into the cutbuffer.  If append is 'TRUE' (when zapping), always append the cut to the cutbuffer. */
void do_snip_for(openfilestruct *const file, int rows, int cols, bool marked, bool until_eof, bool append) {
  ASSERT(file);
  linestruct *line = file->current;
  keep_cutbuffer &= (file->last_action != COPY);
  /* If cuts were not continuous, or when cutting a region, clear the slate. */
  if ((marked || until_eof || !keep_cutbuffer) && !append) {
    free_lines(cutbuffer);
    cutbuffer = NULL;
  }
  /* Now move the relevant piece of text into the cutbuffer. */
  if (until_eof) {
    extract_segment_for(file, rows, cols, file->current, file->current_x, file->filebot, strlen(file->filebot->data));
  }
  else if (file->mark) {
    cut_marked_region_for(file, rows, cols);
    file->mark = NULL;
  }
  else if (ISSET(CUT_FROM_CURSOR)) {
    /* When not at the end of a line, move the rest of this line into the cutbuffer.  Otherwise,
     * when not at the end of the buffer, move just the "line separator" into the cutbuffer. */
    if (line->data[file->current_x]) {
      extract_segment_for(file, rows, cols, line, file->current_x, line, strlen(line->data));
    }
    else if (file->current != file->filebot) {
      extract_segment_for(file, rows, cols, line, file->current_x, line->next, 0);
      file->placewewant = xplustabs();
    }
  }
  else {
    /* When not at end-of-buffer, move one full line into the cutbuffer; otherwise, move all text until end-of-line into the cutbuffer. */
    if (file->current != file->filebot) {
      extract_segment_for(file, rows, cols, line, 0, line->next, 0);
    }
    else {
      extract_segment_for(file, rows, cols, line, 0, line, strlen(line->data));
    }
    file->placewewant = 0;
  }
  /* After a line operation, future ones should add to the cutbuffer. */
  keep_cutbuffer = (!marked && !until_eof);
  set_modified_for(file);
  refresh_needed = TRUE;
  perturbed      = TRUE;
}

/* Move text from the current buffer into the cutbuffer.  If until_eof is 'TRUE', move all text from the current cursor position
 * to the end of the file into the cutbuffer.  If append is 'TRUE' (when zapping), always append the cut to the cutbuffer. */
void do_snip(bool marked, bool until_eof, bool append) {
  if (IN_GUI_CONTEXT) {
    do_snip_for(GUI_CONTEXT, marked, until_eof, append);
  }
  else {
    do_snip_for(TUI_CONTEXT, marked, until_eof, append);
  }
}

/* ----------------------------- Cut text ----------------------------- */

/* Move text from the current buffer into the cutbuffer. */
void cut_text_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  if (!is_cuttable_for(file, (ISSET(CUT_FROM_CURSOR) && !file->mark))) {
    return;
  }
  /* Only add a new undo item when the current item is not a `CUT` or when the current cut is not contiguonus with the previous cutting. */
  if (file->last_action != CUT || !keep_cutbuffer) {
    keep_cutbuffer = FALSE;
    add_undo_for(file, CUT, NULL);
  }
  do_snip_for(file, rows, cols, file->mark, FALSE, FALSE);
  update_undo_for(file, CUT);
  wipe_statusbar();
}

/* Move text from the current buffer into the cutbuffer. */
void cut_text(void) {
  if (IN_GUI_CONTEXT) {
    cut_text_for(GUI_CONTEXT);
  }
  else {
    cut_text_for(TUI_CONTEXT);
  }
}

/* ----------------------------- Cut till end of file ----------------------------- */

/* Cut from the current cursor position to the end of the file. */
void cut_till_eof_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  ran_a_tool = TRUE;
  if (!file->current->data[file->current_x] && (!file->current->next || (!ISSET(NO_NEWLINES) && file->current_x > 0 && file->current->next == file->filebot))) {
    statusbar_all(_("Nothing was cut"));
    return;
  }
  add_undo_for(file, CUT_TO_EOF, NULL);
  do_snip_for(file, rows, cols, FALSE, TRUE, FALSE);
  update_undo_for(file, CUT_TO_EOF);
  wipe_statusbar();
}

/* Cut from the current cursor position to the end of the file, in the currently open file. */
void cut_till_eof(void) {
  if (IN_GUI_CONTEXT) {
    cut_till_eof_for(GUI_CONTEXT);
  }
  else {
    cut_till_eof_for(TUI_CONTEXT);
  }
}

/* ----------------------------- Zap text ----------------------------- */

/* Erase text (current line or marked region), sending it into oblivion. */
void zap_text_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  /* Remember the current cutbuffer so it can be restored after the zap. */
  linestruct *was_cutbuffer = cutbuffer;
  if (!is_cuttable_for(file, (ISSET(CUT_FROM_CURSOR) && !file->mark))) {
    return;
  }
  /* Add a new undo item only when the current item is not a ZAP or when
   * the current zap is not contiguous with the previous zapping. */
  if (file->last_action != ZAP || !keep_cutbuffer) {
    add_undo_for(file, ZAP, NULL);
  }
  /* Use the cutbuffer from the ZAP undo item, so the cut can be undone. */
  cutbuffer = file->current_undo->cutbuffer;
  do_snip_for(file, rows, cols, (file->mark != NULL), FALSE, TRUE);
  update_undo_for(file, ZAP);
  wipe_statusbar();
  cutbuffer = was_cutbuffer;
}

/* Erase text (current line or marked region), sending it into oblivion. */
void zap_text(void) {
  if (IN_GUI_CONTEXT) {
    zap_text_for(GUI_CONTEXT);
  }
  else {
    zap_text_for(TUI_CONTEXT);
  }
}

/* ----------------------------- Do delete ----------------------------- */

/* Delete the character under the cursor plus any succeeding zero-width chars, or,
 * when the mark is on and `LET_THEM_ZAP/--zap` is active, delete the marked region. */
void do_delete_for(openfilestruct *const file, int rows, int cols) {
  ASSERT(file);
  if (file->mark && ISSET(LET_THEM_ZAP)) {
    zap_text_for(file, rows, cols);
  }
  else {
    expunge_for(file, cols, DEL);
    while (file->current->data[file->current_x] && is_zerowidth(file->current->data + file->current_x)) {
      expunge_for(file, cols, DEL);
    }
  }
}

/* Delete the character under the cursor plus any succeeding zero-width chars, or,
 * when the mark is on and `LET_THEM_ZAP/--zap` is active, delete the marked region. */
void do_delete(void) {
  if (IN_GUI_CONTEXT) {
    do_delete_for(GUI_CONTEXT);
  }
  else {
    do_delete_for(TUI_CONTEXT);
  }
}
