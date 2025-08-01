/** @file text.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


/* The maximum depth of recursion.  Note that this MUST be an even number. */
#define RECURSION_LIMIT  222

/* Create callable paste names that correctly describe there task. */
#define redo_paste_for  undo_cut_for
#define undo_paste_for  redo_cut_for

#define OPEN_BRACKETS  "{[("

#define ONE_PARAGRAPH  FALSE
#define WHOLE_BUFFER   TRUE


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static inline undostruct *undostruct_create_for(openfilestruct *const file, undo_type *const action) {
  undostruct *u = xmalloc(sizeof(*u));
  u->type        = *action;
  u->strdata     = NULL;
  u->cutbuffer   = NULL;
  u->head_lineno = file->current->lineno;
  u->head_x      = file->current_x;
  u->tail_lineno = file->current->lineno;
  u->tail_x      = file->current_x;
  u->wassize     = file->totsize;
  u->newsize     = file->totsize;
  u->grouping    = NULL;
  u->xflags      = 0;
  /* Blow away any undone items. */
  discard_until_for(file, file->current_undo);
  /* If some action caused automating long-line wrapping, insert the SPLIT_BEGIN item underneath that action's undo item. */
  if (u->type == SPLIT_BEGIN) {
    *action             = file->undotop->type;
    u->wassize          = file->undotop->wassize;
    u->next             = file->undotop->next;
    file->undotop->next = u;
  }
  /* Otherwise, just add the new item to the top of the undo stack. */
  else {
    u->next            = file->undotop;
    file->undotop      = u;
    file->current_undo = u;
  }
  return u;
}

/* ----------------------------- Undo cut ----------------------------- */

/* Undo a cut, or redo a paste. */
static void undo_cut_for(openfilestruct *const file, int rows, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  goto_line_posx_for(file, rows, u->head_lineno, ((u->xflags & WAS_WHOLE_LINE) ? 0 : u->head_x));
  /* Clear an inherited anchor but not a user-placed one. */
  if (!(u->xflags & HAD_ANCHOR_AT_START)) {
    file->current->has_anchor = FALSE;
  }
  /* If the undo-cutbuffer contains data, copy the data from it. */
  if (u->cutbuffer) {
    copy_from_buffer_for(file, rows, u->cutbuffer);
  }
  /* If originally the last line was cut too, remove the extra magic line. */
  if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES) && file->current->next && !*file->filebot->prev->data) {
    remove_magicline();
  }
  /* If the action was a `ZAP`, then restore the mark as well as the cursor. */
  if (u->type == ZAP) {
    restore_undo_posx_and_mark_for(file, rows, u);
  }
  /* Otherwise, just restore the cursor to where it was. */
  else {
    /* Head */
    if (u->xflags & CURSOR_WAS_AT_HEAD) {
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
    }
    /* Tail */
    else {
      goto_line_posx_for(file, rows, u->tail_lineno, u->tail_x);
    }
  }
}

/* ----------------------------- Redo cut ----------------------------- */

/* Redo a cut, or undo a paste. */
static void redo_cut_for(CTX_ARGS, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  /* Save the cutbuffer. */
  linestruct *was_cutbuffer = cutbuffer;
  cutbuffer = NULL;
  /* Set the mark at the head, and cursor at the tail. */
  set_mark_for(file, u->head_lineno, ((u->xflags & WAS_WHOLE_LINE) ? 0 : u->head_x));
  goto_line_posx_for(file, rows, u->tail_lineno, u->tail_x);
  /* Perform the cut.  Then discard the cut lines. */
  do_snip_for(STACK_CTX, TRUE, FALSE, (u->type == ZAP));
  free_lines_for(NULL, cutbuffer);
  /* Restore the cutbuffer. */
  cutbuffer = was_cutbuffer;
}

/* ----------------------------- Handle move line action ----------------------------- */

/* Undo a move_line(s)_(up/down). */
_UNUSED
static void undo_move_line_for(openfilestruct *const file, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  linestruct *top;
  linestruct *bot;
  /* Single-line move. */
  if (u->head_lineno == u->tail_lineno) {
    file->current = line_from_number_for(file, u->head_lineno);
    move_line_data(file->current, ((u->type == MOVE_LINE_UP) ? UP : DOWN));
    /* Restore the mark, if it was set. */
    if (u->xflags & MARK_WAS_SET) {
      set_marked_region_for(file, file->current, u->head_x, file->current, u->tail_x, (u->xflags & CURSOR_WAS_AT_HEAD));
      /* Indecate the mark should not be removed. */
      keep_mark = TRUE;
    }
    /* Otherwise, just set the cursor position. */
    else {
      file->current_x = u->head_x;
    }
  }
  /* Multi-line move. */
  else {
    top = line_from_number_for(file, u->head_lineno);
    bot = line_from_number_for(file, u->tail_lineno);
    if (u->type == MOVE_LINE_UP) {
      DLIST_FOR_PREV_END(bot, top->prev, line) {
        move_line_data(line, UP);
      }
    }
    else {
      DLIST_FOR_NEXT_END(top, bot->next, line) {
        move_line_data(line, DOWN);
      }
    }
    /* Restore the mark. */
    set_marked_region_for(file, top, u->head_x, bot, u->tail_x, (u->xflags & CURSOR_WAS_AT_HEAD));
    /* Indecate that the mark should not be removed. */
    keep_mark = TRUE;
  }
  refresh_needed = TRUE;
}

/* Redo a move_line(s)_(up/down). */
_UNUSED
static void redo_move_line_for(openfilestruct *const file, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  linestruct *top;
  linestruct *bot;
  int offset = ((u->type == MOVE_LINE_UP) ? -1 : 1);
  /* Single-line move. */
  if (u->head_lineno == u->tail_lineno) {
    file->current = line_from_number_for(file, (u->head_lineno + offset));
    move_line_data(file->current, ((u->type == MOVE_LINE_UP) ? DOWN : UP));
    /* Restore the mark, if it was set. */
    if (u->xflags & MARK_WAS_SET) {
      set_marked_region_for(file, file->current, u->head_x, file->current, u->tail_x, (u->xflags & CURSOR_WAS_AT_HEAD));
      /* Indecate the mark should not be removed. */
      keep_mark = TRUE;
    }
    /* Otherwise, just set the cursor position. */
    else {
      file->current_x = u->head_x;
    }
  }
  /* Multi-line move. */
  else {
    top = line_from_number_for(file, (u->head_lineno + offset));
    bot = line_from_number_for(file, (u->tail_lineno + offset));
    if (u->type == MOVE_LINE_UP) {
      DLIST_FOR_NEXT_END(top, bot->next, line) {
        move_line_data(line, DOWN);
      }
    }
    else {
      DLIST_FOR_PREV_END(bot, top->prev, line) {
        move_line_data(line, UP);
      }
    }
    /* Restore the mark. */
    set_marked_region_for(file, top, u->head_x, bot, u->tail_x, (u->xflags & CURSOR_WAS_AT_HEAD));
    /* Indecate the mark should not be removed. */
    keep_mark = TRUE;
  }
  refresh_needed = TRUE;
}

/* Undo a move_line(s)_(up/down). */
static void handle_move_line_action_for(openfilestruct *const file, undostruct *const u, bool undoing) {
  ASSERT(file);
  ASSERT(u);
  linestruct *top;
  linestruct *bot;
  int offset = (undoing ? 0 : ((u->type == MOVE_LINE_UP) ? -1 : 1));
  bool direction = ((undoing ^ (u->type == MOVE_LINE_UP)) ? DOWN : UP);
  /* Single-line move. */
  if (u->head_lineno == u->tail_lineno) {
    file->current = line_from_number_for(file, (u->head_lineno + offset));
    move_line_data(file->current, direction);
    /* Restore the mark, if it was set. */
    if (u->xflags & MARK_WAS_SET) {
      set_marked_region_for(file, file->current, u->head_x, file->current, u->tail_x, (u->xflags & CURSOR_WAS_AT_HEAD));
      /* Indecate the mark should not be removed. */
      keep_mark = TRUE;
    }
    /* Otherwise, just set the cursor position. */
    else {
      file->current_x = u->head_x;
    }
  }
  /* Multi-line move. */
  else {
    top = line_from_number_for(file, (u->head_lineno + offset));
    bot = line_from_number_for(file, (u->tail_lineno + offset));
    if (direction == UP) {
      DLIST_FOR_PREV_END(bot, top->prev, line) {
        move_line_data(line, UP);
      }
    }
    else {
      DLIST_FOR_NEXT_END(top, bot->next, line) {
        move_line_data(line, DOWN);
      }
    }
    /* Restore the mark. */
    set_marked_region_for(file, top, u->head_x, bot, u->tail_x, (u->xflags & CURSOR_WAS_AT_HEAD));
    /* Indecate that the mark should not be removed. */
    keep_mark = TRUE;
  }
  refresh_needed = TRUE;
}

/* ----------------------------- Undo enclose ----------------------------- */

/* Undo a enclose. */
static void undo_enclose_for(openfilestruct *const file, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  char *s1;
  char *s2;
  /* Get the top and bottom line from the undo-object. */
  linestruct *top = line_from_number_for(file, u->head_lineno);
  linestruct *bot = line_from_number_for(file, u->tail_lineno);
  /* Decode the data to get the starting and ending enclose strings. */
  enclose_str_decode(u->strdata, &s1, &s2);
  /* Then erase the lengths of the decoded strings in both lines. */
  xstr_erase_norealloc(top->data, u->head_x, strlen(s1));
  xstr_erase_norealloc(bot->data, u->tail_x, strlen(s2));
  free(s1);
  free(s2);
  /* Restore the marked region. */
  set_marked_region_for(file, top, u->head_x, bot, u->tail_x, (u->xflags & CURSOR_WAS_AT_HEAD));
  /* If the enclose involved the last line, remove it. */
  if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
    remove_magicline_for(file);
  }
  keep_mark      = TRUE;
  refresh_needed = TRUE;
}

/* Redo a enclose. */
static void redo_enclose_for(openfilestruct *const file, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  char *s1;
  char *s2;
  Ulong len;
  Ulong head_x;
  Ulong tail_x;
  linestruct *top = line_from_number_for(file, u->head_lineno);
  linestruct *bot = line_from_number_for(file, u->tail_lineno);
  /* If the enclose involved the last line, add another magic line. */
  if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
    new_magicline_for(file);
  }
  /* Decode the undo-string to get the start and end enclose strings. */
  enclose_str_decode(u->strdata, &s1, &s2);
  len = strlen(s1);
  /* Calculate where the head and tail x position will be after the enclose. */
  head_x = (u->head_x + len);
  tail_x = (u->tail_x + ((top == bot) ? len : 0));
  /* Inject the beginning and end enclose strings into the top and bottom lines. */
  top->data = xstrninj(top->data, s1, len, u->head_x);
  bot->data = xstrinj(bot->data, s2, tail_x);
  free(s1);
  free(s2);
  /* Restore the mark. */
  set_marked_region_for(file, top, head_x, bot, tail_x, (u->xflags & CURSOR_WAS_AT_HEAD));
  /* Indecate the mark should not be removed. */
  keep_mark = TRUE;
  refresh_needed = TRUE;
}

/* ----------------------------- Undo auto bracket ----------------------------- */

/* Undo a auto bracket. */
static void undo_auto_bracket_for(openfilestruct *const file, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  /* Restore the state of the cursor. */
  file->current   = line_from_number_for(file, u->head_lineno);
  file->current_x = u->head_x;
  /* Unlink the next line twice, to remove both added lines. */
  unlink_node_for(file, file->current->next);
  unlink_node_for(file, file->current->next);
  /* Append the data at the cursor position. */
  file->current->data = xnstrcat(file->current->data, file->current_x, u->strdata);
  /* Renumber the lines after. */
  renumber_from(file->current);
  set_pww_for(file);
  refresh_needed = TRUE;
}

/* ----------------------------- Handle zap replace action ----------------------------- */

/* Hande a zap-replace undo-redo action. */
static void handle_zap_replace_action(CTX_ARGS, undostruct *const u, bool undoing) {
  ASSERT(file);
  ASSERT(u);
  linestruct *was_cutbuffer;
  Ulong len;
  /* Undo */
  if (undoing) {
    /* Get the length of the data. */
    len = strlen(u->strdata);
    /* Set the cursor line and x at the head. */
    goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
    /* Ensure the data currently at the cursor matches the undo-object string data. */
    ALWAYS_ASSERT_MSG((strncmp((file->current->data + file->current_x), u->strdata, len) == 0), "Data does not match for `ZAP_REPLACE` undo.");
    /* Erase the inserted data.  And restore the cut data. */
    xstr_erase_norealloc(file->current->data, file->current_x, len);
    copy_from_buffer_for(file, rows, u->cutbuffer);
    /* Restrore the cursor and mark, as they were before this event. */
    restore_undo_posx_and_mark_for(file, rows, u);
  }
  /* Redo */
  else {
    set_marked_region_for(
      file,
      line_from_number_for(file, u->head_lineno), u->head_x,
      line_from_number_for(file, u->tail_lineno), u->tail_x,
      (u->xflags & CURSOR_WAS_AT_HEAD)
    );
    /* Save the cutbuffer. */
    was_cutbuffer = cutbuffer;
    cutbuffer     = NULL;
    /* Perform the cut. */
    cut_marked_region_for(STACK_CTX);
    u->cutbuffer = cutbuffer;
    /* Restore the original cutbuffer. */
    cutbuffer = was_cutbuffer;
    /* Inject the text. */
    file->current->data = xstrinj(file->current->data, u->strdata, file->current_x);
  }
}

/* ----------------------------- Undo insert empty line ----------------------------- */

/* Undo a insert empty line. */
static void undo_insert_empty_line_for(openfilestruct *const file, int rows, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  /* Restore the cursor and mark in `file`. */
  restore_undo_posx_and_mark_for(file, rows, u);
  /* Ensure filetop and/or edittop gets changed if they are currently the line we will delete. */
  if ((u->xflags & INSERT_WAS_ABOVE)) {
    if (file->current->prev == file->filetop) {
      file->filetop = file->current;
    }
    if (file->current->prev == file->edittop) {
      file->edittop = file->current;
    }
  }
  /* Remove the line that was inserted. */
  unlink_node_for(file, ((u->xflags & INSERT_WAS_ABOVE) ? file->current->prev : file->current->next));
  renumber_from(file->current);
  refresh_needed = TRUE;
}

/* ----------------------------- Handle tab auto indent ----------------------------- */

/* Undo-redo handler for `tab-auto-indent`. */
static void handle_tab_auto_indent(openfilestruct *const file, undostruct *const u, bool undoing) {
  ASSERT(file);
  ASSERT(u);
  linestruct *line;
  /* When redoing, this is the indent of the current line. */
  Ulong cur_indent;
  char *data;
  /* Set the correct current line and position. */
  file->current = line_from_number_for(file, u->head_lineno);
  file->current_x = u->head_x;
  /* Undo */
  if (undoing) {
    /* Restore the current line's data to how it was before anything was done. */
    file->current->data = xstrcpy(file->current->data, u->strdata);
  }
  /* Redo */
  else {
    /* If the previous char is a open bracket char, there is special handeling to be done, to add another indent. */
    if (is_previous_char_one_of(file->current, file->current_x, "{[(:", &line, NULL) && line != file->current) {
      /* Get the indent of the current line. */
      cur_indent = indent_length(file->current->data);
      /* The current line is a blank line. */
      if (white_string(file->current->data)) {
        file->current->data = free_and_assign(
          file->current->data,
          line_indent_plus_tab(line_from_number_for(file, u->tail_lineno)->data, &file->current_x)
        );
      }
      /* Otherwise, the current line has data, in which case we only allow tab
       * help when the cursor is in the leading whitespace of the current line. */
      else if (file->current_x <= cur_indent) {
        data = line_indent_plus_tab(line_from_number_for(file, u->tail_lineno)->data, &file->current_x);
        xstr_erase_norealloc(file->current->data, 0, cur_indent);
        file->current->data = free_and_assign(data, xstrninj(file->current->data, data, file->current_x, 0));
      }
    }
    /* Otherwise, this was a stright tab helper, to get to the same indentation as the previous line.  Note that we only allow for
     * this type of help when the current line is a blank line, and the current cursor position is less then the given indent. */
    else if (file->current->prev && white_string(file->current->data) && file->current_x < u->tail_x) {
      file->current->data = xstrncpy(file->current->data, line_from_number_for(file, u->tail_lineno)->data, u->tail_x);
      file->current_x     = u->tail_x;
    }
  }
  SET_PWW(file);
  refresh_needed = TRUE;
}

/* ----------------------------- Copy character ----------------------------- */

/* Copy a character form one place to another.  TODO: Make a macro
 * that always performs the copy, thus we will inline when needed. */
/* static */ void copy_character(char **const from, char **const to) {
  ASSERT(from);
  ASSERT(to);
  int len = char_length(*from);
  /* This just compares the ptrs...? */
  if (*from == *to) {
    *from += len;
    *to   += len;
  }
  else {
    while (--len >= 0) {
      *((*to)++) = *((*from)++);
    }
  }
}

/* ----------------------------- Squeeze ----------------------------- */

/* In the given `line`, replace any series of blanks with a single space, but keep two
 * spaces (if if there are two) after any closing punctuation, and remove all blanks
 * from the end of the line.  Leave the first skip number of characters untreated. */
/* static */ void squeeze(linestruct *const line, Ulong skip) {
  ASSERT(line);
  char *start = (line->data + skip);
  char *from  = start;
  char *to    = start;
  /* Go through each character. */
  while (*from) {
    /* When we are at a blank. */
    if (is_blank_char(from)) {
      from += char_length(from);
      /* Add one space. */
      *(to++) = SP;
      /* Then advance past all blank characters. */
      while (*from && is_blank_char(from)) {
        from += char_length(from);
      }
    }
    /* We found a punctuation, copy it. */
    else if (mbstrchr(punct, from)) {
      copy_character(&from, &to);
      /* If there is a trailing bracket, copy it to. */
      if (*from && mbstrchr(brackets, from)) {
        copy_character(&from, &to);
      }
      /* At most change two following blanks to spaces. */
      if (*from && is_blank_char(from)) {
        from += char_length(from);
        *(to++) = SP;
      }
      if (*from && is_blank_char(from)) {
        from += char_length(from);
        *(to++) = SP;
      }
      /* Now advance past all following blanks. */
      while (*from && is_blank_char(from)) {
        from += char_length(from);
      }
    }
    /* Otherwise, just do a straight copy. */
    else {
      copy_character(&from, &to);
    }
  }
  /* If there are spaces at the end of the line, remove them. */
  while (to > start && *(to - 1) == SP) {
    --to;
  }
  *to = NUL;
}

/* ----------------------------- Fix spello ----------------------------- */

/* Let the user edit the misspelled word.  Returns `FALSE` if user cancels. */
/* static */ bool fix_spello_for(CTX_ARGS, const char *const restrict word) {
  ASSERT(file);
  ASSERT(word);
  linestruct *was_edittop = file->edittop;
  linestruct *was_current = file->current;
  linestruct *was_mark;
  linestruct *top;
  linestruct *bot;
  Ulong top_x;
  Ulong bot_x;
  Ulong was_firstcolumn = file->firstcolumn;
  Ulong was_x           = file->current_x;
  bool proceed       = FALSE;
  bool right_side_up = (file->mark && mark_is_before_cursor_for(file));
  int result;
  /* TODO: For now we only allow curses-mode operation.  And later we sould add a a replacement parameter for the gui. */
  if (IN_CURSES_CTX) {
    /* If the mark is on, start at the beginning of the marked region. */
    if (file->mark) {
      get_region_for(file, &top, &top_x, &bot, &bot_x);
      /* If the region is marked normaly, swap the end points, so that (current, current_x) (where searching starts) is at the top. */
      if (right_side_up) {
        file->current   = top;
        file->current_x = top_x;
        file->mark      = bot;
        file->mark_x    = bot_x;
      }
    }
    /* Otherwise, start from the top of the file. */
    else {
      file->current   = file->filetop;
      file->current_x = 0;
    }
    /* Find the first whole occurence of word. */
    result = findnextstr_for(file, word, TRUE, INREGION, NULL, FALSE, NULL, 0);
    /* If the word isn't found, alert the user; if it is, allow correction. */
    if (!result) {
      statusline(ALERT, _("Unfindable word: %s"), word);
      lastmessage = VACUUM;
      proceed     = TRUE;
      napms(2800);
    }
    else if (result == 1) {
      spotlighted    = TRUE;
      light_from_col = XPLUSTABS(file);
      light_to_col   = (light_from_col + breadth(word));
      was_mark       = file->mark;
      file->mark     = NULL;
      edit_refresh_for(STACK_CTX);
      /* TODO: Ensure this is gui-safe. */
      put_cursor_at_end_of_answer();
      /* Let the user supply a correctly spelled alternative. */
      proceed     = (do_prompt(MSPELL, word, NULL, edit_refresh, /* TRANSPLATORS: This is a prompt. */ _("Edit a replacement")) != -1);
      spotlighted = FALSE;
      file->mark  = was_mark;
      /* If a replacement was given go through all occurrences. */
      if (proceed && strcmp(word, answer) != 0) {
        do_replace_loop_for(STACK_CTX, word, TRUE, was_current, &was_x);
        /* TRANSLATORS: Shown after fixing missppellings in one word...? -- What does this even mean. */
        statusbar_all(_("Next word..."));
        napms(400);
      }
    }
    /* If there was a marked region. */
    if (file->mark) {
      /* Restore the (compensated) end points of the marked region. */
      if (right_side_up) {
        file->current   = file->mark;
        file->current_x = file->mark_x;
        file->mark      = top;
        file->mark_x    = top_x;
      }
      else {
        file->current   = top;
        file->current_x = top_x;
      }
    }
    else {
      /* Restore the (compensated) cursor position. */
      file->current   = was_current;
      file->current_x = was_x;
    }
    /* Restore the viewtop to where it was. */
    file->edittop     = was_edittop;
    file->firstcolumn = was_firstcolumn;
  }
  return proceed;
}

/* Let the user edit the misspelled word.  Returns `FALSE` if user cancels.  Note that this is `context-safe`. */
/* static */ bool fix_spello(const char *const restrict word) {
  RET_CTX_CALL_WARGS(fix_spello_for, word);
}

/* ----------------------------- Concat paragraph ----------------------------- */

/* Concatenate into a single line all the lines of the paragraph that starts at `line` and
 * consists of `count` lines, skipping the quoting and indentation of all lines after the first. */
/* static */ void concat_paragraph_for(openfilestruct *const file, linestruct *const line, Ulong count) {
  ASSERT(line);
  linestruct *next;
  Ulong quot_len;
  Ulong lead_len;
  Ulong len;
  while (count-- > 1) {
    next     = line->next;
    quot_len = quote_length(next->data);
    lead_len = (quot_len + indent_length(next->data + quot_len));
    len      = strlen(line->data);
    /* We're just about to tack the next line onto the one.  If this line isn't empty, make sure it ends in a space. */
    line->data = fmtstrncat(line->data, len, "%s%s", ((len && line->data[len - 1] != SP) ? " " : ""), (next->data + lead_len));
    line->has_anchor |= next->has_anchor;
    unlink_node_for(file, next);
  }
}

/* Concatenate into a single line all the lines of the paragraph that starts at `line` and
 * consists of `count` lines, skipping the quoting and indentation of all lines after the first. */
/* static */ void concat_paragraph(linestruct *const line, Ulong count) {
  concat_paragraph_for(CTX_OF, line, count);
}

/* ----------------------------- Rewrap paragraph ----------------------------- */

/* Rewrap the given line (that starts with the given lead string which is of
 * the given length), into lines that fit within the target width (wrap_at). */
/* static */ void rewrap_paragraph_for(openfilestruct *const file, int rows,
  linestruct **const line, const char *const restrict lead_str, Ulong lead_len)
{
  ASSERT(line);
  ASSERT(lead_str);
  /* The x-cordinate where the current line is to be broken. */
  long break_pos;
  Ulong line_len;
  /* Do the loop... */
  while (breadth((*line)->data) > wrap_at) {
    line_len = strlen((*line)->data);
    /* Find a point in the line where it can be broken. */
    break_pos = break_line(((*line)->data + lead_len), (wrap_at - wideness((*line)->data, lead_len)), FALSE);
    /* If we can't break the line, or don't need to, we're done. */
    if (break_pos < 0 || (lead_len + break_pos) == lead_len) {
      break;
    }
    /* Adjust the breaking position for the leading part and move it beyond the found whitespace character. */
    break_pos += (lead_len + 1);
    /* Insert a new line after the current one, and copy the leading part plus the text after the breaking point into it. */
    splice_node_for(file, *line, make_new_node(*line));
    /* Construct the new line. */
    (*line)->next->data = xmalloc(lead_len + line_len - break_pos + 1);
    memcpy((*line)->next->data, lead_str, lead_len);
    strcpy(((*line)->next->data + lead_len), ((*line)->data + break_pos));
    /* When requested, snip all trailing spaces. */
    if (ISSET(TRIM_BLANKS)) {
      while (break_pos > 0 && (*line)->data[break_pos - 1] == ' ') {
        --break_pos;
      }
    }
    /* Now actualy break the current line, and go to the next. */
    (*line)->data[break_pos] = '\0';
    DLIST_ADV_NEXT(*line);
  }
  /* If the new paragraph exceeds the viewport, recalculate the multidata. */
  if ((*line)->lineno >= rows) {
    recook = TRUE;
  }
  /* When possible, go to the line after the rewrapped paragraph. */
  if ((*line)->next) {
    DLIST_ADV_NEXT(*line);
  }
}

/* Rewrap the given line (that starts with the given lead string which is of the given length),
 * into lines that fit within the target width (wrap_at).  Note that this is `context-safe`. */
/* static */ void rewrap_paragraph(linestruct **const line, const char *const restrict lead_str, Ulong lead_len) {
  if (IN_GUI_CTX) {
    rewrap_paragraph_for(GUI_OF, GUI_ROWS, line, lead_str, lead_len);
  }
  else {
    rewrap_paragraph_for(TUI_OF, TUI_ROWS, line, lead_str, lead_len);
  }
}

/* ----------------------------- Justify paragraph ----------------------------- */

/* Justify the lines of the given paragraph (that starts at `*line`, and consitis of `count` lines)
 * so they all fit within the target width (`wrap at`) and have their whitespaces normalized. */
/* static */ void justify_paragraph_for(openfilestruct *const file, int rows, linestruct **const line, Ulong count) {
  ASSERT(line);
  /* The line from which the indentation is copied. */
  linestruct *sample;
  /* Length of the quote part. */
  Ulong quot_len;
  /* Length of the quote part plus the indentation part. */
  Ulong lead_len;
  /* The quote+indent struff that is copied from the sample line. */
  char *lead_str;
  /* The sample line is either the only line or the second line. */
  sample = ((count == 1) ? *line : (*line)->next);
  /* Copy the leading part (quoting + indentation) of the sample line. */
  quot_len = quote_length(sample->data);
  lead_len = (quot_len + indent_length(sample->data + quot_len));
  lead_str = measured_copy(sample->data, lead_len);
  /* Concatenate all lines of the paragraph into a single line. */
  concat_paragraph_for(file, *line, count);
  /* Change all blank characters to spaces and remove excess spaces. */
  squeeze(*line, (quot_len + indent_length((*line)->data + quot_len)));
  /* Rewrap the line into multiple lines, accounting for the leading part. */
  rewrap_paragraph_for(file, rows, line, lead_str, lead_len);
  free(lead_str);
}

/* Justify the lines of the given paragraph (that starts at `*line`, and consitis of `count` lines) so they all
 * fit within the target width (`wrap at`) and have their whitespaces normalized.  Note that this is `context-safe`. */
/* static */ void justify_paragraph(linestruct **const line, Ulong count) {
  if (IN_GUI_CTX) {
    justify_paragraph_for(GUI_OF, GUI_ROWS, line, count);
  }
  else {
    justify_paragraph_for(TUI_OF, TUI_ROWS, line, count);
  }
}

/* ----------------------------- Construct argument list ----------------------------- */

/* Set up an argument list for executing the given command. */
/* static */ void construct_argument_list(char ***arguments, char *command, char *filename) {
  ASSERT(arguments);
  char *copy_of_command = copy_of(command);
  char *element = strtok(copy_of_command, " ");
  int count = 2;
  while (element) {
    *arguments = xrealloc(*arguments, (++count * _PTRSIZE));
    (*arguments)[count - 3] = element;
    element = strtok(NULL, " ");
  }
  (*arguments)[count - 2] = filename;
  (*arguments)[count - 1] = NULL;
}

/* ----------------------------- Replace buffer ----------------------------- */

/* Open the specified file, and if that succeeds, remove the text of the marked
 * region or of the entire buffer and read the file contents into it's place.
 * TODO: Split this up, or make subfuctionality from this available, so that
 * we can implement replace marked region in a mush better way and also more things. */
/* static */ bool replace_buffer_for(CTX_ARGS, const char *const restrict filename, undo_type action, const char *const restrict operation) {
  ASSERT(file);
  ASSERT(filename);
  linestruct *was_cutbuffer = cutbuffer;
  int fd;
  FILE *stream;
  /* Open the file-descriptor. */
  if ((fd = open_file(filename, FALSE, &stream)) < 0) {
    return FALSE;
  }
  add_undo_for(file, COUPLE_BEGIN, operation);
  /* When replacing the whole buffer, start cutting at the top. */
  if (action == CUT_TO_EOF) {
    file->current   = file->filetop;
    file->current_x = 0;
  }
  cutbuffer = NULL;
  /* Cut either the marked region or the whole buffer. */
  add_undo_for(file, action, NULL);
  do_snip_for(STACK_CTX, file->mark, !file->mark, FALSE);
  update_undo_for(file, action);
  /* Discard what was cut and restore the cutbuffer. */
  free_lines(cutbuffer);
  cutbuffer = was_cutbuffer;
  /* Insert the spell-checked file into the cleared area...? */
  read_file_into(STACK_CTX, stream, fd, filename, TRUE);
  add_undo_for(file, COUPLE_END, operation);
  return TRUE;
}

/* Open the specified file, and if that succeeds, remove the text of the marked
 * region or of the entire buffer and read the file contents into it's place.
 * TODO: Split this up, or make subfuctionality from this available, so that
 * we can implement replace marked region in a mush better way and also more things. */
/* static */ bool replace_buffer(const char *const restrict filename, undo_type action, const char *const restrict operation) {
  RET_CTX_CALL_WARGS(replace_buffer_for, filename, action, operation);
}

/* ----------------------------- Treat ----------------------------- */

/* Execute the given program, with the given temp-file as the last argument. */
/* static */ void treat_for(CTX_ARGS, char *tempfile, char *program, bool spelling) {
  ASSERT(file);
  char **arguments = NULL;
  struct stat info;
  long was_lineno = file->current->lineno;
  long was_mark_lineno;
  long time_sec   = 0;
  long time_nsec  = 0;
  Ulong was_pww = file->placewewant;
  Ulong was_x   = file->current_x;
  bool was_at_eol = !file->current->data[was_x];
  bool replaced   = FALSE;
  bool upright;
  pid_t pid;
  int status;
  int error;
  /* Stat the temporary file. */
  if (stat(tempfile, &info) == 0) {
    /* The temporary file's size is zero, there is nothing to do. */
    if (info.st_size == 0) {
      statusline(AHEM, ((spelling && file->mark) ? _("Selection is empty") : _("Buffer is empty")));
      return;
    }
    /* Otherwise, save the modification time of the temporary file. */
    else {
      time_sec  = info.st_mtim.tv_sec;
      time_nsec = info.st_mtim.tv_nsec;
    }
  }
  /* The spell checker needs the screen...?, so exit from curses-mode. */
  if (spelling) {
    if (IN_CURSES_CTX) {
      endwin();
    }
  }
  else {
    statusbar_all(_("Invoking formatter..."));
  }
  construct_argument_list(&arguments, program, tempfile);
  /* Fork a child process and run the given program in it. */
  if ((pid = fork()) == 0) {
    execvp(arguments[0], arguments);
    /* Terminate the child if the program is not found. */
    exit(9);
  }
  else if (pid > 0) {
    /* Block SIGWINCH'es while waiting for the forked program to end, so nanox doesn't get pushed past `wait()`. */
    block_sigwinch(TRUE);
    wait(&status);
    block_sigwinch(FALSE);
  }
  error = errno;
  /* When in curses-mode. */
  if (IN_CURSES_CTX) {
    /* After spell checking, restore the terminal state and re-enter curses-mode. */
    if (spelling) {
      terminal_init();
      doupdate();
    }
    /* After formating, make sure that any formatted output is wiped. */
    else {
      full_refresh();
    }
  }
  /* We failed to fork. */
  if (pid < 0) {
    statusline(ALERT, _("Could not fork: %s"), strerror(error));
    free(arguments[0]);
    free(arguments);
    return;
  }
  else if (!WIFEXITED(status) || WEXITSTATUS(status) > 2) {
    statusline(ALERT, _("Error invoking '%s'"), arguments[0]);
    free(arguments[0]);
    free(arguments);
    return;
  }
  else if (WEXITSTATUS(status) != 0) {
    statusline(ALERT, _("Program '%s' complained"), arguments[0]);
  }
  free(arguments[0]);
  free(arguments);
  /* When the temporary file wasn't touched, say so and leave. */
  if (time_sec > 0 && stat(tempfile, &info) == 0 && info.st_mtim.tv_sec == time_sec && info.st_mtim.tv_nsec == time_nsec) {
    statusline(REMARK, _("Nothing changed"));
    return;
  }
  /* Replace the marked text (or entire text) with the corrected text. */
  if (spelling && file->mark) {
    was_mark_lineno = file->mark->lineno;
    upright         = mark_is_before_cursor_for(file);
    replaced        = replace_buffer_for(STACK_CTX, tempfile, CUT, "spelling correction");
    /* Adjust the end point of the marked region for any change in length of the region's last line. */
    if (upright) {
      was_x = file->current_x;
    }
    else {
      file->mark_x = file->current_x;
    }
    /* Restore the mark. */
    file->mark = line_from_number_for(file, was_mark_lineno);
  }
  else {
    /* TRANSLATORS: The next two go with Undid/Redid messages. */
    replaced = replace_buffer_for(STACK_CTX, tempfile, CUT_TO_EOF, (spelling ? N_("spelling correction") : N_("formatting")));
  }
  /* Go back to the old position. */
  goto_line_posx_for(file, rows, was_lineno, was_x);
  if (was_at_eol || file->current_x > strlen(file->current->data)) {
    file->current_x = strlen(file->current->data);
  }
  if (replaced) {
    file->filetop->has_anchor = FALSE;
    update_undo_for(file, COUPLE_END);
  }
  file->placewewant = was_pww;
  adjust_viewport_for(STACK_CTX, STATIONARY);
  if (spelling) {
    statusline(REMARK, _("Finished checking spelling"));
  }
  else {
    statusline(REMARK, _("Buffer has been processed"));
  }
}

/* Execute the given program, with the given temp-file as the last argument.  Note that this is `context-safe`. */
/* static */ void treat(char *tempfile, char *program, bool spelling) {
  CTX_CALL_WARGS(treat_for, tempfile, program, spelling);
}

/* ----------------------------- Do int speller ----------------------------- */

/* Run a spell-check on the given temp-file, using `spell` to preduce a list of all
 * misspelled words, then feeding those through `sort` and `uniq` to obtain a alphabetical
 * non-duplicate list, which words are then offered one by one to the user for correction. */
/* static */ void do_int_speller_for(CTX_ARGS, const char *const restrict tempfile) {
  ASSERT(file);
  ASSERT(tempfile);
  char *buf;
  char *pointer;
  char *oneword;
  long pipesize;
  Ulong bufsize;
  Ulong bytesread;
  Ulong totalread = 0;
  Ulong stash[ARRAY_SIZE(flags)];
  int fd_spell[2];
  int fd_sort[2];
  int fd_uniq[2];
  int fd_temp = -1;
  int status_spell;
  int status_sort;
  int status_uniq;
  pid_t pid_spell;
  pid_t pid_sort;
  pid_t pid_uniq;
  /* Parent: Create all pipes up front. */
  if (pipe(fd_spell) == -1 || pipe(fd_sort) == -1 || pipe(fd_uniq) == -1) {
    statusline(ALERT, _("Could not create pipe: %s"), strerror(errno));
    return;
  }
  statusbar_all(_("Invoking spell checker..."));
  /* Parent: Fork a process to run the spell in. */
  if ((pid_spell = fork()) == 0) {
    /* Child: Open the temporary file that holds the text to be checked. */
    if ((fd_temp = open(tempfile, O_RDONLY)) == -1) {
      exit(6);
    }
    /* Child: Connect standard input to the temporary file. */
    if (dup2(fd_temp, STDIN_FILENO) < 0) {
      exit(7);
    }
    /* Child: Connect standard output to the temporary file. */
    if (dup2(fd_spell[1], STDOUT_FILENO) < 0) {
      exit(8);
    }
    close(fd_temp);
    close(fd_spell[0]);
    close(fd_spell[1]);
    /* Child: Try to run `hunspell`.  If that fails, fall back to `spell`. */
    execlp("hunspell", "hunspell", "-l", NULL);
    execlp("spell", "spell", NULL);
    /* Child: Indecate failure when neither speller was found. */
    exit(9);
  }
  /* Parent: Close the unused write end of the spell pipe. */
  close(fd_spell[1]);
  /* Parent: Fork a process to run sort in. */
  if ((pid_sort = fork()) == 0) {
    /* Child: Connect standard input to the read end of the first pipe. */
    if (dup2(fd_spell[0], STDIN_FILENO) < 0) {
      exit(7);
    }
    /* Child: Connect standard output to the write end of the second pipe. */
    if (dup2(fd_sort[1], STDOUT_FILENO) < 0) {
      exit(8);
    }
    close(fd_spell[0]);
    close(fd_sort[0]);
    close(fd_sort[1]);
    /* Child: Now run the sort program.  Use -f to mix upper and lower case. */
    execlp("sort", "sort", "-f", NULL);
    exit(9);
  }
  close(fd_spell[0]);
  close(fd_sort[1]);
  /* Fork a process to run uniq in. */
  if ((pid_uniq = fork()) == 0) {
    /* Child: Connect standard input to the read end of the sort pipe. */
    if (dup2(fd_sort[0], STDIN_FILENO) < 0) {
      exit(7);
    }
    /* Child: Connect standard output to the write end of the uniq pipe. */
    if (dup2(fd_uniq[1], STDOUT_FILENO) < 0) {
      exit(8);
    }
    close(fd_sort[0]);
    close(fd_uniq[0]);
    close(fd_uniq[1]);
    /* Child: Run uniq. */
    execlp("uniq", "uniq", NULL);
    exit(9);
  }
  close(fd_sort[0]);
  close(fd_uniq[1]);
  /* Parent: When some child process was not forked successfully... */
  if (pid_spell < 0 || pid_sort < 0 || pid_uniq < 0) {
    statusline(ALERT, _("Could not fork: %s"), strerror(errno));
    close(fd_uniq[0]);
    return;
  }
  /* Parent: Get the system pipe buffer size. */
  pipesize = fpathconf(fd_uniq[0], _PC_PIPE_BUF);
  if (pipesize < 0) {
    statusline(ALERT, _("Could not get the size of the pipe buffer"));
    close(fd_uniq[0]);
    return;
  }
  /* Parent: When in curses-mode. */
  if (IN_CURSES_CTX) {
    /* Parent: Leave curses so that error messages go to the original screen... */
    endwin();
    /* Parent: Block SIGWINCH'es while reading misspelled words from the third pipe. */
    block_sigwinch(TRUE);
  }
  bufsize = (pipesize + 1);
  buf     = xmalloc(bufsize);
  pointer = buf;
  /* TODO: Fix this shitty always reallocing shit, and do a exponential growth, with powers of 2.  Like one should. */
  while ((bytesread = read(fd_uniq[0], pointer, pipesize)) > 0) {
    totalread += bytesread;
    bufsize   += pipesize;
    buf     = xrealloc(buf, bufsize);
    pointer = (buf + totalread);
  }
  *pointer = '\0';
  close(fd_uniq[0]);
  /* Parent: Re-enter curses when in curses-mode. */
  if (IN_CURSES_CTX) {
    /* Unblock SIGWINCH'es. */
    block_sigwinch(FALSE);
    terminal_init();
    doupdate();
  }
  /* Parent: Save the settings of the global flags. */
  memcpy(stash, flags, sizeof(flags));
  /* Parent: Do any replacement case-sensitivly, forward, and without regexes. */
  SET(CASE_SENSITIVE);
  UNSET(BACKWARDS_SEARCH);
  UNSET(USE_REGEXP);
  pointer = buf;
  oneword = buf;
  /* Parent: Process each of the misspelled words. */
  while (*pointer) {
    if (*pointer == CR || *pointer == LF) {
      *pointer = NUL;
      if (oneword != pointer) {
        if (!fix_spello_for(STACK_CTX, oneword)) {
          oneword = pointer;
          break;
        }
      }
      oneword = (pointer + 1);
    }
    ++pointer;
  }
  /* Parent: Special case: The last word dosen't end with CR of LF. */
  if (oneword != pointer) {
    fix_spello_for(STACK_CTX, oneword);
  }
  free(buf);
  refresh_needed = TRUE;
  /* Parent: Restore the settings of the global flags. */
  memcpy(flags, stash, sizeof(flags));
  /* Parent: Wait for the three processes to end. */
  waitpid(pid_spell, &status_spell, 0);
  waitpid(pid_sort,  &status_sort,  0);
  waitpid(pid_uniq,  &status_uniq,  0);
  /* Uniq exited abnormaly. */
  if (!WIFEXITED(status_uniq) || WEXITSTATUS(status_uniq)) {
    statusline(ALERT, _("Error invoking \"uniq\""));
  }
  /* Sort exited abnormaly. */
  else if (!WIFEXITED(status_sort) || WEXITSTATUS(status_sort)) {
    statusline(ALERT, _("Error invoking \"sort\""));
  }
  /* Spell exited abnormaly. */
  else if (!WIFEXITED(status_spell) || WEXITSTATUS(status_spell)) {
    statusline(ALERT, _("Error invoking \"spell\""));
  }
  /* Everything exited correctly with no errors. */
  else {
    statusline(ALERT, _("Finished checking spelling"));
  }
}

/* Run a spell-check on the given temp-file, using `spell` to preduce a list of all
 * misspelled words, then feeding those through `sort` and `uniq` to obtain a alphabetical
 * non-duplicate list, which words are then offered one by one to the user for correction. */
/* static */ void do_int_speller(const char *const restrict tempfile) {
  CTX_CALL_WARGS(do_int_speller_for, tempfile);
}

/* ----------------------------- Justify text ----------------------------- */

/* Justify the current paragraph in `file`, or the entire buffer when whole_buffer
 * is `TRUE`.  But if the mark is on, justify only the marked text instead. */
/* static */ void justify_text_for(CTX_ARGS, bool whole_buffer) {
  ASSERT(file);
  /* The leading part (quoting + indentation) of the first line of the paragraph where the marked region begins. */
  char *primary_lead = NULL;
  /* The leading part for lines after the first one. */
  char *secondary_lead = NULL;
  /* The old cutbuffer, so we can justify in the current cutbuffer. */
  linestruct *was_cutbuffer = cutbuffer;
  linestruct *line;
  /* The line that we're justifying in the current cutbuffer. */
  linestruct *jusline;
  /* The line where the paragraph or region starts. */
  linestruct *start;
  /* The line where the paragraph or region ends. */
  linestruct *end;
  linestruct *sample;
  /* The line to return to after a full justification...?  Would that not change? */
  long was_lineno = file->current->lineno;
  /* The length (in bytes) of the above first-line leading part. */
  Ulong primary_len = 0;
  /* The length (in bytes) of that later lead. */
  Ulong secondary_len = 0;
  /* The number of lines in the original paragraph. */
  Ulong linecount;
  /* The x position where the paragraph or region starts. */
  Ulong start_x;
  /* The x position where the paragraph or region ends. */
  Ulong end_x;
  Ulong quot_len;
  Ulong fore_len;
  Ulong other_qout_len;
  Ulong other_white_len;
  /* Whether the end of the marked region is before the end of its line. */
  bool before_eol      = FALSE;
  bool marked_backward = (file->mark && !mark_is_before_cursor_for(file));
  /* TRANSLATORS: This one goes with Undid/Redid messages. */
  add_undo_for(file, COUPLE_BEGIN, _("justification"));
  /* If the mark is on, do as pico.  Treat all marked text as one paragraph. */
  if (file->mark) {
    get_region_for(file, &start, &start_x, &end, &end_x);
    /* When the marked-region is empty, do nothing. */
    if (start == end && start_x == end_x) {
      statusline(AHEM, _("Selection is empty"));
      discard_until(file->undotop->next);
      return;
    }
    quot_len = quote_length(start->data);
    fore_len = (quot_len + indent_length(start->data + quot_len));
    /* When the region start in the lead, take the whole lead. */
    if (start_x <= fore_len) {
      start_x = 0;
    }
    /* Recede over blanks before the region.  This effectivly snips
     * trailing blanks from what will become the preceeding paragraph. */
    while (start_x && is_blank_char(start->data + start_x - 1)) {
      start_x = step_left(start->data, start_x);
    }
    quot_len = quote_length(end->data);
    fore_len = (quot_len + indent_length(end->data + quot_len));
    /* When the region ends in the lead, take the whole lead. */
    if (end_x && end_x < fore_len) {
      end_x = fore_len;
    }
    /* When not at the left edge, advance over blanks after that region. */
    if (end_x) {
      while (is_blank_char(end->data + end_x)) {
        end_x = step_right(end->data, end_x);
      }
    }
    sample = start;
    /* Find the first line of the paragraph in which the region starts. */
    while (sample->prev && inpar(sample) && !begpar(sample, 0)) {
      DLIST_ADV_PREV(sample);
    }
    /* Ignore lines that contain no text. */
    while (sample->next && !inpar(sample)) {
      DLIST_ADV_NEXT(sample);
    }
    /* Store the leading part that is to be used for the new paragraph. */
    quot_len     = quote_length(sample->data);
    primary_len  = (quot_len + indent_length(sample->data + quot_len));
    primary_lead = measured_copy(sample->data, primary_len);
    if (sample->next && start != end) {
      DLIST_ADV_NEXT(sample);
    }
    /* Copy the leading part that is to be used for the new paragraph after its first line
     * (if any):  the quoting of the first line, plus the indentation of the second line. */
    other_qout_len  = quote_length(sample->data);
    other_white_len = indent_length(sample->data + other_qout_len);
    secondary_len   = (quot_len + other_white_len);
    secondary_lead  = fmtstr("%*.s%*.s", (int)quot_len, start->data, (int)other_white_len, (sample->data + other_qout_len));
    /* Include preceding and succeeding leads into the marked region. */
    file->mark      = start;
    file->mark_x    = start_x;
    file->current   = end;
    file->current_x = end_x;
    /* Remember wherer the end of the region was before the end-of-line. */
    before_eol = !!end->data[end_x];
  }
  else {
    /* When justifying the entire buffer, start at the top. */
    if (whole_buffer) {
      file->current = file->filetop;
    }
    /* Otherwise, when in a paragraoh but not at it's beginning, move back to it's first line. */
    else if (inpar(file->current) && !begpar(file->current, 0)) {
      do_para_begin(&file->current);
    }
    /* Find the first line of the paragraph(s) to be justified.  If the search fails, there is nothing
     * to justify, and we will be on the last line of the file, so put the cursor at the end of it. */
    if (!find_paragraph(&file->current, &linecount)) {
      file->current_x = strlen(file->filebot->data);
      discard_until_for(file, file->undotop->next);
      refresh_needed = TRUE;
      return;
    }
    /* Set the starting point of the paragraph. */
    start_x = file->current_x = 0;
    start   = file->current;
    /* Set the end point of the paragraph. */
    if (whole_buffer) {
      end = file->filebot;
    }
    else {
      end = start;
      for (Ulong count=linecount; count>1; --count) {
        DLIST_ADV_NEXT(end);
      }
    }
    /* When possible, step one line further. */
    if (end->next) {
      DLIST_ADV_NEXT(end);
      end_x = 0;
    }
    /* Otherwise, move the x position to the line's end. */
    else {
      end_x = strlen(end->data);
    }
  }
  add_undo_for(file, CUT, NULL);
  /* Do the equivalent of the marked cut into an empty cutbuffer. */
  cutbuffer = NULL;
  extract_segment_for(STACK_CTX, start, start_x, end, end_x);
  update_undo_for(file, CUT);
  /* This is done on a marked region. */
  if (file->mark) {
    line     = cutbuffer;
    quot_len = quote_length(line->data);
    fore_len = (quot_len + indent_length(line->data + quot_len));
    /* If the extracted region begins with any leading part, trim it. */
    if (fore_len) {
      xstr_erase_norealloc(line->data, 0, fore_len);
    }
    /* Then copy back in the leading part that it should have. */
    if (primary_len) {
      line->data = xstrninj(line->data, primary_lead, primary_len, 0);
    }
    /* Now justify the extracted region. */
    concat_paragraph_for(NULL, cutbuffer, linecount);
    squeeze(cutbuffer, primary_len);
    rewrap_paragraph_for(NULL, rows, &line, secondary_lead, secondary_len);
    /* If the marked region started in the middle of a line, insert a newline before the new paragraph. */
    if (start_x) {
      cutbuffer->prev       = make_new_node(NULL);
      cutbuffer->prev->data = COPY_OF("");
      cutbuffer->prev->next = cutbuffer;
      DLIST_ADV_PREV(cutbuffer);
    }
    /* If the marked region ended in the middle of a line, insert a newline after the new paragraph. */
    if (end_x && before_eol) {
      line->next       = make_new_node(line);
      line->next->data = copy_of(primary_lead);
    }
    free(primary_lead);
    free(secondary_lead);
    /* Keep as mush of the marked region onscreen as possible. */
    focusing = FALSE;
  }
  else {
    /* Prepare to justify the text we just put in the cutbuffer. */
    jusline = cutbuffer;
    /* Justify the current paragraph. */
    justify_paragraph_for(NULL, rows, &jusline, linecount);
    /* When justifying the entire buffer, font and justify all paragraphs. */
    if (whole_buffer) {
      while (find_paragraph(&jusline, &linecount)) {
        justify_paragraph_for(NULL, rows, &jusline, linecount);
        if (!jusline->next) {
          break;
        }
      }
    }
  }
  /* Wipe an anchor on the first paragraph if it was only inherited. */
  if (whole_buffer && !file->mark && !cutbuffer->has_anchor) {
    file->current->has_anchor = FALSE;
  }
  add_undo_for(file, PASTE, NULL);
  /* Do the equvilent of a paste of the justified text. */
  ingraft_buffer_into(file, cutbuffer, NULL);
  update_undo_for(file, PASTE);
  /* After justifying a backwards-marked text, swap mark and cursor. */
  if (marked_backward) {
    ATOMIC_SWAP(file->current_x, file->mark_x);
    ATOMIC_SWAP(file->current,   file->mark);
  }
  else if (whole_buffer && !file->mark) {
    goto_line_posx_for(file, rows, was_lineno, 0);
  }
  add_undo_for(file, COUPLE_END, _("justification"));
  /* Report on the status-bar what we justified. */
  if (file->mark) {
    statusline(REMARK, _("Justified selection"));
  }
  else if (whole_buffer && !file->mark) {
    statusline(REMARK, _("Justified file"));
  }
  else {
    statusline(REMARK, _("Justified paragraph"));
  }
  /* We're done justifying.  Restore the cutbuffer. */
  cutbuffer = was_cutbuffer;
  /* Set the desired screen column (always zero, except at EOF). */
  SET_PWW(file);
  set_modified_for(file);
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Justify the current paragraph in the `currently open buffer`, or the entire buffer when `whole_buffer`
 * is `TRUE`.  But if the mark is on, justify only the marked text instead.  Note that this is `context-safe`. */
/* static */ void justify_text(bool whole_buffer) {
  CTX_CALL_WARGS(justify_text_for, whole_buffer);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Line indent plus tab ----------------------------- */

/* Return the given indent of data, plus either (when `TABS_TO_SPACES` is
 * set) a string containg spaces to the next tabstop, or one tabulator. */
char *line_indent_plus_tab(const char *const restrict data, Ulong *const outlen) {
  ASSERT(data);
  char *ret;
  int tablen;
  int len = indent_length(data);
  /* When tabs are represented by a number of spaces. */
  if (ISSET(TABS_TO_SPACES)) {
    tablen = tabstop_length(data, len);
    ret = fmtstr("%.*s%*s", len, data, tablen, " ");
  }
  /* Otherwise, just use a single tabulator. */
  else {
    tablen = 1;
    ret = fmtstr("%.*s\t", len, data);
  }
  len += tablen;
  ASSIGN_IF_VALID(outlen, len);
  return ret;
}

/* ----------------------------- Set marked region ----------------------------- */

/* Set the cursor and mark of file, using ptrs. */
void set_marked_region_for(openfilestruct *const file, linestruct *const top,
  Ulong top_x, linestruct *const bot, Ulong bot_x, bool cursor_at_head)
{
  ASSERT(file);
  ASSERT(top);
  ASSERT(bot);
  /* Cursor is `top`. */
  if (cursor_at_head) {
    file->current   = top;
    file->mark      = bot;
    file->current_x = top_x;
    file->mark_x    = bot_x;
  }
  /* Cursor is `bot`. */
  else {
    file->current   = bot;
    file->mark      = top;
    file->current_x = bot_x;
    file->mark_x    = top_x;
  }
}

/* ----------------------------- Do tab ----------------------------- */
/* TODO: Here we need to add the functionality where if on a line with
 * only blanks and not at the end of the line, we put the cursor at eol.
 * Or maybe when tab is done on a empty line, match the indent with the
 * prev line, and when the prev char is a bracket open char, match the
 * indent plus one indent.
 */

/* Insert a tab.  Or if `TABS_TO_SPACES/--tabstospaces` is in effect, insert
 * the number of spaces that a tab would normally take up at this position. */
void do_tab_for(CTX_ARGS) {
  ASSERT(file);
  Ulong len;
  char *spaces;
  /* When <Tab> is pressed while a region is marked, indent that region. */
  if (file->mark && file->mark != file->current) {
    do_indent_for(file, cols);
  }
  else if (file->syntax && file->syntax->tabstring) {
    inject_into_buffer(STACK_CTX, file->syntax->tabstring, strlen(file->syntax->tabstring));
  }
  else if (tab_helper(file)) {
    ;
  }
  else if (ISSET(TABS_TO_SPACES)) {
    spaces = tab_space_string_for(file, &len);
    inject_into_buffer(STACK_CTX, spaces, len);
    free(spaces);
  }
  else {
    inject_into_buffer(STACK_CTX, "\t", 1);
  }
}

/* Insert a tab.  Or if `TABS_TO_SPACES/--tabstospaces` is in effect, insert
 * the number of spaces that a tab would normally take up at this position. */
void do_tab(void) {
  CTX_CALL(do_tab_for);
}

/* ----------------------------- Indentlen ----------------------------- */

/* Return's the length of whilespace until first non blank char in `string`. */
Ulong indentlen(const char *const restrict string) {
  ASSERT(string);
  const char *ptr = string;
  while (is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return (ptr - string);
}

/* ----------------------------- Quote length ----------------------------- */

/* Return the length of the quote part of the given line.  The 'quote part'
 * of a line is the largest initial substring matching the quoting regex. */
Ulong quote_length(const char *const restrict line) {
  regmatch_t matches;
  int rc = regexec(&quotereg, line, 1, &matches, 0);
  if (rc == REG_NOMATCH || matches.rm_so == (regoff_t)-1) {
    return 0;
  }
  return matches.rm_eo;
}

/* ----------------------------- Add undo ----------------------------- */

/* Add a new undo item of the given type to the top of the current pile for `file`. */
void add_undo_for(openfilestruct *const file, undo_type action, const char *const restrict message) {
  ASSERT(file);
  linestruct *thisline = file->current;
  undostruct *u = undostruct_create_for(file, &action);
  // undostruct *u = xmalloc(sizeof(*u));
  // /* Initialize the newly allocated undo item. */
  // u->type        = action;
  // u->strdata     = NULL;
  // u->cutbuffer   = NULL;
  // u->head_lineno = thisline->lineno;
  // u->head_x      = file->current_x;
  // u->tail_lineno = thisline->lineno;
  // u->tail_x      = file->current_x;
  // u->wassize     = file->totsize;
  // u->newsize     = file->totsize;
  // u->grouping    = NULL;
  // u->xflags      = 0;
  // /* Blow away any undone items. */
  // discard_until_for(file, file->current_undo);
  // /* If some action caused automatic long-line wrapping, insert the SPLIT_BEGIN item underneath
  //  * that action's undo item.  Otherwise, just add the new item to the top of the undo stack. */
  // if (u->type == SPLIT_BEGIN) {
  //   action     = file->undotop->type;
  //   u->wassize = file->undotop->wassize;
  //   u->next    = file->undotop->next;
  //   file->undotop->next = u;
  // }
  // else {
  //   u->next = file->undotop;
  //   file->undotop      = u;
  //   file->current_undo = u;
  // }
  /* Record the info needed to be able to undo each possible action. */
  switch (u->type) {
    case ADD: {
      /* If a new magic line will be added, an undo should remove it. */
      if (thisline == file->filebot) {
        u->xflags |= INCLUDED_LAST_LINE;
      }
      break;
    }
    case ENTER: {
      break;
    }
    case BACK: {
      /* If the next line is the magic line, don't ever undo this backspace, as it won't actually have deleted anything. */
      if (thisline->next == file->filebot && thisline->data[0]) {
        u->xflags |= WAS_BACKSPACE_AT_EOF;
      }
      _FALLTHROUGH;
    }
    case DEL: {
      /* When not at the end of a line, store the deleted character. */
      if (thisline->data[file->current_x]) {
        int charlen = char_length(thisline->data + u->head_x);
        u->strdata  = measured_copy((thisline->data + u->head_x), charlen);
        if (u->type == BACK) {
          u->tail_x += charlen;
        }
        break;
      }
      /* Otherwise, morph the undo item into a line join. */
      action = JOIN;
      if (thisline->next) {
        if (u->type == BACK) {
          u->head_lineno = thisline->next->lineno;
          u->head_x      = 0;
        }
        u->strdata = copy_of(thisline->next->data);
      }
      u->type = JOIN;
      break;
    }
    case REPLACE: {
      u->strdata = copy_of(thisline->data);
      if (thisline == file->filebot && answer[0]) {
        u->xflags |= INCLUDED_LAST_LINE;
      }
      break;
    }
    case SPLIT_BEGIN:
    case SPLIT_END: {
      break;
    }
    case CUT_TO_EOF: {
      u->xflags |= (INCLUDED_LAST_LINE | CURSOR_WAS_AT_HEAD);
      if (file->current->has_anchor) {
        u->xflags |= HAD_ANCHOR_AT_START;
      }
      break;
    }
    case ZAP:
    case CUT: {
      if (file->mark) {
        if (mark_is_before_cursor_for(file)) {
          u->head_lineno = file->mark->lineno;
          u->head_x      = file->mark_x;
          u->xflags |= MARK_WAS_SET;
        }
        else {
          u->tail_lineno = file->mark->lineno;
          u->tail_x      = file->mark_x;
          u->xflags |= (MARK_WAS_SET | CURSOR_WAS_AT_HEAD);
        }
        if (u->tail_lineno == file->filebot->lineno) {
          u->xflags |= INCLUDED_LAST_LINE;
        }
      }
      else if (!ISSET(CUT_FROM_CURSOR)) {
        /* The entire line is being cut regardless of the cursor position. */
        u->xflags |= (WAS_WHOLE_LINE | CURSOR_WAS_AT_HEAD);
        u->tail_x = 0;
      }
      else {
        u->xflags |= CURSOR_WAS_AT_HEAD;
      }
      if ((file->mark && mark_is_before_cursor_for(file) && file->mark->has_anchor)
       || ((!file->mark || !mark_is_before_cursor_for(file)) && file->current->has_anchor)) {
        u->xflags |= HAD_ANCHOR_AT_START;
      }
      break;
    }
    case PASTE: {
      u->cutbuffer = copy_buffer(cutbuffer);
      _FALLTHROUGH;
    }
    case INSERT: {
      if (thisline == file->filebot) {
        u->xflags |= INCLUDED_LAST_LINE;
      }
      break;
    }
    case COUPLE_BEGIN: {
      u->tail_lineno = file->cursor_row;
      _FALLTHROUGH;
    }
    case COUPLE_END: {
      u->strdata = copy_of(_(message));
      break;
    }
    case ZAP_REPLACE: {
      if (!file->mark) {
        die("ZAP_REPLACE should only be done when there is a marked region.\n");
      }
      u->strdata = copy_of(message);
      _FALLTHROUGH;
    }
    case INDENT:
    case UNINDENT:
    case COMMENT:
    case UNCOMMENT:
    case MOVE_LINE_UP:
    case MOVE_LINE_DOWN:
    case INSERT_EMPTY_LINE: {
      if (file->mark) {
        if (mark_is_before_cursor_for(file)) {
          u->head_lineno = file->mark->lineno;
          u->head_x      = file->mark_x;
          u->xflags |= MARK_WAS_SET;
        }
        else {
          u->tail_lineno = file->mark->lineno;
          u->tail_x      = file->mark_x;
          u->xflags |= (MARK_WAS_SET | CURSOR_WAS_AT_HEAD);
        }
      }
      break;
    }
    case ENCLOSE: {
      if (mark_is_before_cursor_for(file)) {
        u->head_lineno = file->mark->lineno;
        u->head_x      = file->mark_x;
        u->xflags |= MARK_WAS_SET;
        if (file->current == file->filebot) {
          u->xflags |= INCLUDED_LAST_LINE;
        }
      }
      else {
        u->tail_lineno = file->mark->lineno;
        u->tail_x      = file->mark_x;
        u->xflags |= (MARK_WAS_SET | CURSOR_WAS_AT_HEAD);
        if (file->mark == file->filebot) {
          u->xflags |= INCLUDED_LAST_LINE;
        }
      }
      u->strdata = copy_of(message);
      break;
    }
    case AUTO_BRACKET: {
      u->strdata = copy_of(file->current->data + file->current_x);
      break;
    }
    case TAB_AUTO_INDENT: {
      /* Save the entire current line. */
      u->strdata = copy_of(thisline->data);
      break;
    }
    default: {
      die("Bad undo type -- please report a bug\n");
    }
  }
  file->last_action = action;
}

/* Add a new undo item of the given type to the top of the current pile for the currently open file.  Works for gui and tui context. */
void add_undo(undo_type action, const char *const restrict message) {
  add_undo_for(CTX_OF, action, message);
}

/* ----------------------------- Update undo ----------------------------- */

/* Update an undo item with (among other things) the file size and cursor position after the given action. */
void update_undo_for(openfilestruct *const restrict file, undo_type action) {
  ASSERT(file);
  undostruct *u = file->undotop;
  Ulong datalen;
  Ulong newlen;
  char *textposition;
  int charlen;
  /* There should never be a missmatch as to what is beeing updated, and thus that should always terminate the execution. */
  if (u->type != action) {
    die("Mismatching undo type -- please report a bug\n");
  }
  u->newsize = file->totsize;
  switch (u->type) {
    case ADD: {
      newlen = (file->current_x - u->head_x);
      u->strdata = xrealloc(u->strdata, (newlen + 1));
      strncpy(u->strdata, (file->current->data + u->head_x), newlen);
      u->strdata[newlen] = '\0';
      u->tail_x = file->current_x;
      break;
    }
    case ENTER: {
      u->strdata = copy_of(file->current->data);
      u->tail_x  = file->current_x;
      break;
    }
    case BACK:
    case DEL: {
      textposition = (file->current->data + file->current_x);
      charlen = char_length(textposition);
      datalen = strlen(u->strdata);
      /* They deleted more: add removed character after earlier stuff. */
      if (file->current_x == u->head_x) {
        u->strdata = xrealloc(u->strdata, (datalen + charlen + 1));
        strncpy((u->strdata + datalen), textposition, charlen);
        u->strdata[datalen + charlen] = '\0';
        u->tail_x = file->current_x;
      }
      /* They backspaced further: add removed character before earlier. */
      else if (file->current_x == (u->head_x - charlen)) {
        u->strdata = xrealloc(u->strdata, (datalen + charlen + 1));
        memmove((u->strdata + charlen), u->strdata, (datalen + 1));
        strncpy(u->strdata, textposition, charlen);
        u->head_x = file->current_x;
      }
      /* They deleted *elsewhere* on the line: start a new undo item. */
      else {
        add_undo_for(file, u->type, NULL);
      }
      break;
    }
    case REPLACE: {
      break;
    }
    case SPLIT_BEGIN:
    case SPLIT_END: {
      break;
    }
    case ZAP:
    case CUT_TO_EOF:
    case CUT: {
      if (u->type == ZAP) {
        u->cutbuffer = cutbuffer;
      }
      else if (cutbuffer) {
        free_lines(u->cutbuffer);
        u->cutbuffer = copy_buffer(cutbuffer);
      }
      else {
        break;
      }
      if (!(u->xflags & MARK_WAS_SET)) {
        linestruct *bottomline = u->cutbuffer;
        Ulong count = 0;
        /* Find the end of the cut for the undo/redo, using our copy. */
        while (bottomline->next) {
          bottomline = bottomline->next;
          ++count;
        }
        u->tail_lineno = (u->head_lineno + count);
        if (ISSET(CUT_FROM_CURSOR) || u->type == CUT_TO_EOF) {
          u->tail_x = strlen(bottomline->data);
          if (!count) {
            u->tail_x += u->head_x;
          }
        }
        else if (file->current == file->filebot && ISSET(NO_NEWLINES)) {
          u->tail_x = strlen(bottomline->data);
        }
      }
      break;
    }
    case COUPLE_BEGIN: {
      break;
    }
    case COUPLE_END:
    case PASTE:
    case INSERT: {
      u->tail_lineno = file->current->lineno;
      u->tail_x      = file->current_x;
      break;
    }
    case INSERT_EMPTY_LINE: {
      /* If the mark was set then check the one that represents where the cursor was.  When inserting a line above cursor. */
      if (((u->xflags & MARK_WAS_SET) && file->current->lineno == ((u->xflags & CURSOR_WAS_AT_HEAD) ? u->head_lineno : u->tail_lineno))
       /* Otherwise, just check the head. */
       || (file->current->lineno == u->head_lineno)) {
        u->xflags |= INSERT_WAS_ABOVE;
        /* If the mark was set. */
        if ((u->xflags & MARK_WAS_SET)) {
          /* And cursor was at head.  Then the mark must be after, so increment it to, to compensate for the newly inserted line above. */
          if (u->xflags & CURSOR_WAS_AT_HEAD) {
            ++u->head_lineno;
            ++u->tail_lineno;
          }
          /* Otherwise, if the mark is before the cursor.  Check if its on the same line as cursor.
           * If so, increment the mark to to compensate for the newly inserted line above. */
          else {
            if (u->head_lineno == u->tail_lineno) {
              ++u->head_lineno;
            }
            ++u->tail_lineno;
          }
        }
        /* Otherwise just increment the cursor line, to compensate for the inserted line above. */
        else {
          ++u->head_lineno;
        }
      }
      /* If the mark was set then check if the cursor is below where the cursor was, this means this was a insertion below the cursor. */
      else if ((u->xflags & MARK_WAS_SET) && (u->xflags & CURSOR_WAS_AT_HEAD) && file->current->lineno <= u->tail_lineno) {
        ++u->tail_lineno;
      }
      break;
    }
    case ZAP_REPLACE: {
      u->cutbuffer = cutbuffer;
      break;
    }
    default: {
      die("Bad undo type -- please report a bug\n");
    }
  }
}

/* Update an undo item with (among other things) the file size and cursor position after the given action. */
void update_undo(undo_type action) {
  update_undo_for(CTX_OF, action);
}

/* ----------------------------- Update multiline undo ----------------------------- */

/* Update a multiline undo item.  This should be called once for each line, affected by a multiple-line-altering
 * feature.  The indentation that is added or removed is saved, separately for each line in the undo item. */
void update_multiline_undo_for(openfilestruct *const file, long lineno, const char *const restrict indentation) {
  ASSERT(file);
  ASSERT(indentation);
  undostruct *u = file->current_undo;
  groupstruct *born;
  /* The number of lines. */
  Ulong nol;
  /* If there already is a group and the current line is contiguous with it, extend the group; otherwise, create a new group. */
  if (u->grouping && (u->grouping->bottom_line + 1) == lineno) {
    nol                                = (lineno - u->grouping->top_line + 1);
    u->grouping->bottom_line           = lineno;
    u->grouping->indentations          = xrealloc(u->grouping->indentations, (nol * _PTRSIZE));
    u->grouping->indentations[nol - 1] = copy_of(indentation);
  }
  else {
    born                  = xmalloc(sizeof(*born));
    born->top_line        = lineno;
    born->bottom_line     = lineno;
    born->indentations    = xmalloc(_PTRSIZE);
    born->indentations[0] = copy_of(indentation);
    born->next            = u->grouping;
    u->grouping           = born;
  }
  /* Store the file size after the change, to be used when redoing. */
  u->newsize = file->totsize;
}

/* Update a multiline undo item.  This should be called once for each line, affected by a multiple-line-altering
 * feature.  The indentation that is added or removed is saved, separately for each line in the undo item. */
void update_multiline_undo(long lineno, const char *const restrict indentation) {
  update_multiline_undo_for(CTX_OF, lineno, indentation);
}

/* ----------------------------- Break line ----------------------------- */

/* Find the last blank in the given piece of text such that the display width to that point is at most
 * (goal + 1).  When there is no such blank, then find the first blank.  Return the index of the last
 * blank in that group of blanks. When snap_at_nl is TRUE, a newline character counts as a blank too. */
long break_line(const char *textstart, long goal, bool snap_at_nl) {
  /* The point where the last blank was found, if any. */
  const char *lastblank = NULL;
  /* An iterator through the given line of text. */
  const char *pointer = textstart;
  /* The column number that corresponds to the position of the pointer. */
  Ulong column = 0;
  /* Skip over leading whitespace, where a line should never be broken. */
  while (*pointer && is_blank_char(pointer)) {
    pointer += advance_over(pointer, &column);
  }
  /* Find the last blank that does not overshoot the target column.
   * When treating a help text, do not break in the keystrokes area. */
  while (*pointer && (long)column <= goal) {
    if (is_blank_char(pointer) && (!inhelp || column > 17 || goal < 40)) {
      lastblank = pointer;
    }
    else if (snap_at_nl && *pointer == '\n') {
      lastblank = pointer;
      break;
    }
    pointer += advance_over(pointer, &column);
  }
  /* If the whole line displays shorter than goal, we're done. */
  if ((long)column <= goal) {
    return (long)(pointer - textstart);
  }
  /* When wrapping a help text and no blank was found, force a line break. */
  if (snap_at_nl && !lastblank) {
    return (long)step_left(textstart, (pointer - textstart));
  }
  /* If no blank was found within the goal width, seek one after it. */
  while (!lastblank) {
    if (!*pointer) {
      return -1;
    }
    if (is_blank_char(pointer)) {
      lastblank = pointer;
    }
    else {
      pointer += char_length(pointer);
    }
  }
  pointer = (lastblank + char_length(lastblank));
  /* Skip any consecutive blanks after the last blank. */
  while (*pointer && is_blank_char(pointer)) {
    lastblank = pointer;
    pointer += char_length(pointer);
  }
  return (long)(lastblank - textstart);
}

/* ----------------------------- Do wrap ----------------------------- */

/* When the current line is overlong, hard-wrap it at the furthest possible whitespace character,
 * and prepend the excess part to an "overflow" line (when it already exists, otherwise create one). */
void do_wrap_for(openfilestruct *const file, int cols) {
  ASSERT(file);
  /* The line to be wrapped, if needed and possible. */
  linestruct *line = file->current;
  /* The length of this line. */
  Ulong line_len = strlen(line->data);
  /* The length of the quoting part of this line. */
  Ulong quot_len = quote_length(line->data);
  /* The length of the quoting part plus subsequent whitespace. */
  Ulong lead_len = (quot_len + indent_length(line->data + quot_len));
  /* The current cursor position, for comparison with the wrap point. */
  Ulong cursor_x = file->current_x;
  /* The position in the line's text where we wrap. */
  long wrap_loc;
  /* The text after the wrap point. */
  const char *remainder;
  /* The length of the remainder. */
  Ulong rest_length;
  /* If `AUTOINDENT` is set or not. */
  bool autowhite;
  Ulong rear_x;
  Ulong typed_x;
  /* First find the last blank character where we can break the line. */
  wrap_loc = break_line((line->data + lead_len), (wrap_at - wideness(line->data, lead_len)), FALSE);
  /* If no wrapping point was found before end-of-line, we don't wrap. */
  if (wrap_loc < 0 || (lead_len + wrap_loc) == line_len) {
    return;
  }
  /* Adjust the wrap location to its position in the full line, and step forward to the character just after the blank. */
  wrap_loc = (lead_len + step_right((line->data + lead_len), wrap_loc));
  /* When now at end-of-line, no need to wrap. */
  if (!line->data[wrap_loc]) {
    return;
  }
  add_undo_for(file, SPLIT_BEGIN, NULL);
  autowhite = ISSET(AUTOINDENT);
  if (quot_len > 0) {
    UNSET(AUTOINDENT);
  }
  /* The remainder is the text that will be wrapped to the next line. */
  remainder   = (line->data + wrap_loc);
  rest_length = (line_len - wrap_loc);
  /* When prepending and the remainder of this line will not make the next line too long, then join the two
   * lines, so that, after the line wrap, the remainder will effectively have been prefixed to the next line. */
  if (file->spillage_line && file->spillage_line == line->next && (rest_length + breadth(line->next->data)) <= wrap_at) {
    /* Go to the end of this line. */
    file->current_x = line_len;
    /* If the remainder doesn't end in a blank, add a space. */
    if (!is_blank_char(remainder + step_left(remainder, rest_length))) {
      add_undo_for(file, ADD, NULL);
      line->data = xrealloc(line->data, (line_len + 2));
      line->data[line_len] = ' ';
      line->data[line_len + 1] = '\0';
      ++rest_length;
      ++file->totsize;
      ++file->current_x;
      update_undo_for(file, ADD);
    }
    /* Join the next line to this one. */
    expunge_for(file, cols, DEL);
    /* If the leading part of the current line equals the leading part of what was the next line, then strip this second leading part. */
    if (strncmp(line->data, (line->data + file->current_x), lead_len) == 0) {
      for (Ulong i=lead_len; i>0; --i) {
        expunge_for(file, cols, DEL);
      }
    }
    /* Remove any extra blanks. */
    while (is_blank_char(&line->data[file->current_x])) {
      expunge_for(file, cols, DEL);
    }
  }
  /* Go to the wrap location. */
  file->current_x = wrap_loc;
  /* When requested, snip trailing blanks off the wrapped line. */
  if (ISSET(TRIM_BLANKS)) {
    rear_x  = step_left(line->data, wrap_loc);
    typed_x = step_left(line->data, cursor_x);
    while ((rear_x != typed_x || (long)cursor_x >= wrap_loc) && is_blank_char(line->data + rear_x)) {
      file->current_x = rear_x;
      expunge_for(file, cols, DEL);
      rear_x = step_left(line->data, rear_x);
    }
  }
  /* Now split the line. */
  do_enter_for(file);
  /* When wrapping a partially visible line, adjust start-of-screen. */
  if (file->edittop == line && file->firstcolumn > 0 && (long)cursor_x >= wrap_loc) {
    go_forward_chunks_for(file, cols, 1, &file->edittop, &file->firstcolumn);
  }
  /* If the original line has quoting, copy it to the spillage line. */
  if (quot_len > 0) {
    line       = line->next;
    line_len   = strlen(line->data);
    line->data = xrealloc(line->data, (lead_len + line_len + 1));
    memmove((line->data + lead_len), line->data, (line_len + 1));
    strncpy(line->data, line->prev->data, lead_len);
    file->current_x += lead_len;
    file->totsize   += lead_len;
    free(file->undotop->strdata);
    update_undo_for(file, ENTER);
    if (autowhite) {
      SET(AUTOINDENT);
    }
  }
  file->spillage_line = file->current;
  if ((long)cursor_x < wrap_loc) {
    file->current   = file->current->prev;
    file->current_x = cursor_x;
  }
  else {
    file->current_x += (cursor_x - wrap_loc);
  }
  SET_PWW(file);
  add_undo_for(file, SPLIT_END, NULL);
  refresh_needed = TRUE;
}

/* When the current line is overlong, hard-wrap it at the furthest possible whitespace character,
 * and prepend the excess part to an "overflow" line (when it already exists, otherwise create one). */
void do_wrap(void) {
  if (IN_GUI_CTX) {
    do_wrap_for(GUI_OF, GUI_COLS);
  }
  else {
    do_wrap_for(TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- Do mark ----------------------------- */

/* Toggle the mark for `file`. */
void do_mark_for(openfilestruct *const file) {
  ASSERT(file);
  if (!file->mark) {
    file->mark     = file->current;
    file->mark_x   = file->current_x;
    file->softmark = FALSE;
    statusbar_all(_("Mark Set"));
  }
  else {
    file->mark = NULL;
    statusbar_all(_("Mark Unset"));
    refresh_needed = TRUE;
  }
}

/* Toggle the mark for the currently open file. */
void do_mark(void) {
  do_mark_for(CTX_OF);
}

/* ----------------------------- Discard until ----------------------------- */

/* Discard `undo-items` that are newer then `thisitem` in `buffer`, or all if `thisitem` is `NULL`. */
void discard_until_for(openfilestruct *const file, const undostruct *const thisitem) {
  ASSERT(file);
  undostruct  *dropit = file->undotop;
  groupstruct *group, *next;
  while (dropit && dropit != thisitem) {
    file->undotop = dropit->next;
    free(dropit->strdata);
    free(dropit->cutbuffer);
    group = dropit->grouping;
    while (group) {
      next = group->next;
      free_chararray(group->indentations, (group->bottom_line - group->top_line + 1));
      free(group);
      group = next;
    }
    free(dropit);
    dropit = file->undotop;
  }
  /* Adjust the pointer to the top of the undo struct. */
  file->current_undo = (undostruct *)thisitem;
  /* Prevent a chain of edition actions from continuing. */
  file->last_action = OTHER;
}

/* Discard undo items that are newer than the given one, or all if NULL. */
void discard_until(const undostruct *thisitem) {
  discard_until_for(CTX_OF, thisitem);
}

/* ----------------------------- Begpar ----------------------------- */

/* Return TRUE when the given line is the beginning of a paragraph (BOP). */
bool begpar(const linestruct *const line, int depth) {
  Ulong quot_len      = 0;
  Ulong indent_len    = 0;
  Ulong prev_dent_len = 0;
  /* The very first line counts as a BOP, even when it contains no text. */
  if (!line->prev) {
    return TRUE;
  }
  /* If recursion is going too deep, just say it's not a BOP. */
  if (depth > RECURSION_LIMIT) {
    return FALSE;
  }
  quot_len   = quote_length(line->data);
  indent_len = indent_length(line->data + quot_len);
  /* If this line contains no text, it is not a BOP. */
  if (!line->data[quot_len + indent_len]) {
    return FALSE;
  }
  /* When requested, treat a line that starts with whitespace as a BOP. */
  if (ISSET(BOOKSTYLE) && !ISSET(AUTOINDENT) && is_blank_char(line->data)) {
    return TRUE;
  }
  /* If the quote part of the preceding line differs, this is a BOP. */
  if (quot_len != quote_length(line->prev->data) || strncmp(line->data, line->prev->data, quot_len) != 0) {
    return TRUE;
  }
  prev_dent_len = indent_length(line->prev->data + quot_len);
  /* If the preceding line contains no text, this is a BOP. */
  if (!line->prev->data[quot_len + prev_dent_len]) {
    return TRUE;
  }
  /* If indentation of this and preceding line are equal, this is not a BOP. */
  if (wideness(line->prev->data, (quot_len + prev_dent_len)) == wideness(line->data, (quot_len + indent_len))) {
    return FALSE;
  }
  /* Otherwise, this is a BOP if the preceding line is not. */
  return !begpar(line->prev, (depth + 1));
}

/* ----------------------------- Inpar ----------------------------- */

/* Return TRUE when the given line is part of a paragraph.  A line is part of a
 * paragraph if it contains something more than quoting and leading whitespace. */
bool inpar(const linestruct *const line) {
  Ulong quot_len   = quote_length(line->data);
  Ulong indent_len = indent_length(line->data + quot_len);
  return (line->data[quot_len + indent_len]);
}

/* ----------------------------- Do block comment ----------------------------- */

/* This is a shortcut to make marked area a block comment in `file`. */
void do_block_comment_for(openfilestruct *const file) {
  ASSERT(file);
  enclose_marked_region_for(file, "/* ", " */");
  keep_mark = TRUE;
}

/* This is a shortcut to make marked area a block comment in the `currently open buffer`. */
void do_block_comment(void) {
  // enclose_marked_region("/* ", " */");
  // keep_mark = TRUE;
  do_block_comment_for(CTX_OF);
}

/* ----------------------------- Length of white ----------------------------- */

/* Return the number of bytes of whitespace at the start of the given text, but at most a tab's worth. */
Ulong length_of_white_for(openfilestruct *const file, const char *text) {
  ASSERT(file);
  ASSERT(text);
  Ulong white_count = 0;
  Ulong thelength;
  if (file->syntax && file->syntax->tabstring) {
    thelength = strlen(file->syntax->tabstring);
    while (text[white_count] == file->syntax->tabstring[white_count]) {
      if (++white_count == thelength) {
        return thelength;
      }
    }
    white_count = 0;
  }
  while (TRUE) {
    if (*text == '\t') {
      return (white_count + 1);
    }
    else if (*text != ' ') {
      return white_count;
    }
    else {
      ++white_count;
      if (EQ(white_count, tabsize)) {
        return tabsize;
      }
    }
    ++text;
  }
}

/* Return the number of bytes of whitespace at the start of the given text, but at most a tab's worth. */
Ulong length_of_white(const char *text) {
  return length_of_white_for(CTX_OF, text);
}

/* ----------------------------- Compensate leftward ----------------------------- */

/* Adjust the positions of mark and cursor of `file` when they are on the given `line`. */
void compensate_leftward_for(openfilestruct *const file, linestruct *const line, Ulong leftshift) {
  ASSERT(file);
  ASSERT(line);
  /* The file's mark is on line. */
  if (line == file->mark) {
    if (file->mark_x < leftshift) {
      file->mark_x = 0;
    }
    else {
      file->mark_x -= leftshift;
    }
  }
  /* The file's cursor is on line. */
  if (line == file->current) {
    if (file->current_x < leftshift) {
      file->current_x = 0;
    }
    else {
      file->current_x -= leftshift;
    }
    /* Set placewewant for file. */
    set_pww_for(file);
  }
}

/* Adjust the positions of mark and cursor when they are on the given line. */
void compensate_leftward(linestruct *const line, Ulong leftshift) {
  compensate_leftward_for(CTX_OF, line, leftshift);
}

/* ----------------------------- Unindent a line ----------------------------- */

/* Remove an indent from the given line, in `file`. */
void unindent_a_line_for(openfilestruct *const file, linestruct *const line, Ulong indent_len) {
  ASSERT(file);
  ASSERT(line); 
  /* If the indent is empty, don't change the line. */
  if (!indent_len) {
    return;
  }
  /* Remove the first tab's worth of whitespace from this line. */
  line->data = xstr_erase_norealloc(line->data, 0, indent_len);
  file->totsize -= indent_len;
  /* Adjust the positions of mark and cursor, when they are affected. */
  compensate_leftward_for(file, line, indent_len);
}

/* Remove an indent from the given line, in `file`. */
void unindent_a_line(linestruct *const line, Ulong indent_len) {
  unindent_a_line_for(CTX_OF, line, indent_len);
}

/* ----------------------------- Do unindent ----------------------------- */

/* Unindent the current line (or the marked lines) by tabsize columns.  The removed indent can be a mixture of spaces plus at most one tab. */
void do_unindent_for(openfilestruct *const file, int cols) {
  ASSERT(file);
  linestruct *top;
  linestruct *bot;
  Ulong indent_len;
  char *indentation;
  /* Use either all the marked lines or just the current line. */
  get_range_for(file, &top, &bot);
  /* Skip any leading lines that cannot be unindented. */
  while (top != bot->next && !length_of_white_for(file, top->data)) {
    top = top->next;
  }
  /* If none of the lines can be unindented, there is nothing to do. */
  if (top == bot->next) {
    return;
  }
  add_undo_for(file, UNINDENT, NULL);
  /* Go through each of the lines, removing their leading indent where possible, and saving the removed whitespace in the undo item. */
  DLIST_FOR_NEXT_END(top, bot->next, line) {
    indent_len  = length_of_white_for(file, line->data);
    indentation = measured_copy(line->data, indent_len);
    unindent_a_line_for(file, line, indent_len);
    update_multiline_undo_for(file, line->lineno, indentation);
    free(indentation);
  }
  set_modified_for(file);
  ensure_firstcolumn_is_aligned_for(file, cols);
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Unindent the current line (or the marked lines) by tabsize columns.  The removed indent can be a mixture of spaces plus at most one tab. */
void do_unindent(void) {
  if (IN_GUI_CTX) {
    do_unindent_for(GUI_OF, GUI_COLS);
  }
  else {
    do_unindent_for(TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- Restore undo posx and mark ----------------------------- */

/* Restore the cursor and mark in `file`, from a undostruct. */
void restore_undo_posx_and_mark_for(openfilestruct *const file, int rows, undostruct *const u) {
  ASSERT(file);
  ASSERT(u);
  /* Restore the mark if it was set. */
  if (u->xflags & MARK_WAS_SET) {
    if (u->xflags & CURSOR_WAS_AT_HEAD) {
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      set_mark_for(file, u->tail_lineno, u->tail_x);
    }
    else {
      goto_line_posx_for(file, rows, u->tail_lineno, u->tail_x);
      set_mark_for(file, u->head_lineno, u->head_x);
    }
    keep_mark = TRUE;
  }
  /* Otherwise just restore the cursor. */
  else {
    goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
  }
}

/* Restore the cursor and mark in the currently open file, from a undostruct. */
void restore_undo_posx_and_mark(undostruct *const u) {
  if (IN_GUI_CTX) {
    restore_undo_posx_and_mark_for(GUI_OF, GUI_ROWS, u);
  }
  else {
    restore_undo_posx_and_mark_for(TUI_OF, TUI_ROWS, u);
  }
}

/* ----------------------------- Insert empty line ----------------------------- */

/* Insert a new empty line, either `above` or `below` `line`.  */
void insert_empty_line_for(openfilestruct *const file, linestruct *const line, bool above, bool autoindent) {
  ASSERT(file);
  ASSERT(line);
  linestruct *topline;
  linestruct *newline;
  Ulong indent_len;
  /* Inserting a empty line above. */
  if (above) {
    if (!line->prev) {
      newline = make_new_node(NULL);
      newline->next = line;
      line->prev    = newline;
      if (line == file->filetop) {
        file->filetop = newline;
      }
      if (line == file->edittop) {
        file->edittop = newline;
      }
    }
    else {
      topline = line->prev;
      newline = make_new_node(topline);
      splice_node_for(file, topline, newline);
    }
  }
  /* Inserting an empty line below. */
  else {
    topline = line;
    newline = make_new_node(topline);
    splice_node_for(file, topline, newline);
  }
  renumber_from(newline);
  if (!autoindent) {
    newline->data = COPY_OF("");
  }
  else {
    indent_len    = indent_length(line->data);
    newline->data = measured_copy(line->data, indent_len);
    /* If the last char of the line we were on was a opening char, then we sould add another indent. */
    if (!above && is_end_char_one_of(newline->prev->data, "{:")) {
      if (ISSET(TABS_TO_SPACES)) {
        newline->data = fmtstrcat(newline->data, "%*s", (int)tabstop_length(newline->data, indent_len), " ");
      }
      else {
        newline->data = xstrncat(newline->data, S__LEN("\t"));
      }
    }
  }
}

/* Insert a new empty line, either `above` or `below` `line`.  */
void insert_empty_line(linestruct *const line, bool above, bool autoindent) {
  insert_empty_line_for(CTX_OF, line, above, autoindent);
}

/* ----------------------------- Do insert empty line above ----------------------------- */

/* Insert a new empty line above `file->current`, and add an undo-item to the undo-stack. */
void do_insert_empty_line_above_for(openfilestruct *const file) {
  ASSERT(file);
  add_undo_for(file, INSERT_EMPTY_LINE, NULL);
  insert_empty_line_for(file, file->current, TRUE, TRUE);
  DLIST_ADV_PREV(file->current);
  update_undo_for(file, INSERT_EMPTY_LINE);
  SET_CURSOR_TO_EOL(file);
  SET_PWW(file);
  refresh_needed = TRUE;
}

/* Insert a new empty line above `openfile->current`, and add an undo-item to the
 * `undo-stack`. Note that this is context safe and works in both the `gui` and `tui`. */
void do_insert_empty_line_above(void) {
  do_insert_empty_line_above_for(CTX_OF);
}

/* ----------------------------- Do insert empty line below ----------------------------- */

/* Insert a new empty line below `file->current`, and add an undo-item to the `undo-stack`. */
void do_insert_empty_line_below_for(openfilestruct *const file) {
  ASSERT(file);
  add_undo_for(file, INSERT_EMPTY_LINE, NULL);
  insert_empty_line_for(file, file->current, FALSE, TRUE);
  DLIST_ADV_NEXT(file->current);
  update_undo_for(file, INSERT_EMPTY_LINE);
  set_cursor_to_eol_for(file);
  refresh_needed = TRUE;
}

/* Insert a new empty line below `openfile->current`, and add an undo-item to the
 * `undo-stack`.  Note that this is context safe and works for both the `gui` and `tui`. */
void do_insert_empty_line_below(void) {
  do_insert_empty_line_below_for(CTX_OF);
}

/* ----------------------------- Cursor is between brackets ----------------------------- */

/* Returns `TRUE` when `file->current->data[file->current_x]` is between a bracket pair, `{}`, `[]` or `()`. */
bool cursor_is_between_brackets_for(openfilestruct *const file) {
  return is_curs_between_any_pair_for(file, (const char *[]){"{}", "[]", "()", NULL}, NULL);
}

/* Returns `TRUE` when `openfile->current->data[openfile->current_x]` is between a bracket pair,
 * `{}`, `[]` or `()`.  Note that this is context safe and can be called from the `tui` or `gui`. */
bool cursor_is_between_brackets(void) {
  return cursor_is_between_brackets_for(CTX_OF);
}

/* ----------------------------- Indent length ----------------------------- */

/* Return the length of the indentation part of the given line.  The "indentation" of a line is the leading consecutive whitespace. */
Ulong indent_length(const char *const restrict line) {
  const char *ptr = line;
  while (is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return (ptr - line);
}

/* ----------------------------- Indent a line ----------------------------- */

/* Add an indent to the given line.  TODO: Make the mark and curren x positions move exactly like vs-code. */
void indent_a_line_for(openfilestruct *const file, linestruct *const line, const char *const restrict indentation) {
  ASSERT(file);
  ASSERT(line);
  ASSERT(indentation);
  Ulong length     = strlen(line->data);
  Ulong indent_len = strlen(indentation);
  /* If the requested indentation is empty, don't change the line. */
  if (!indent_len) {
    return;
  }
  /* Inject the indentation at the begining of the line. */
  line->data = xnstrninj(line->data, length, indentation, indent_len, 0);
  /* Then increase the total size of the file by the indent length added. */
  file->totsize += indent_len;
  /* When `line` is the mark as well ensure the mark only  */
  if (line == file->mark && file->mark_x > 0) {
    file->mark_x += indent_len;
  }
  if (line == file->current && file->current_x > 0) {
    file->current_x  += indent_len;
    file->placewewant = xplustabs_for(file);
  }
}

/* Add an indent to the given line. */
void indent_a_line(linestruct *const line, const char *const restrict indentation) { 
  indent_a_line_for(CTX_OF, line, indentation);
}

/* ----------------------------- Do indent ----------------------------- */

/* Indent the current line (or the marked lines) of `file` by tabsize columns.  This inserts either tab
 * character or a tab's worth of spaces, depending on whether the `TABS_TO_SPACES` flag is in effect. */
void do_indent_for(openfilestruct *const file, int cols) {
  ASSERT(file);
  linestruct *top; 
  linestruct *bot;
  char *indentation;
  char *real_indent;
  /* Use either all the marked lines or just the current line. */
  get_range_for(file, &top, &bot);
  /* Skip any leading empty lines. */
  while (top != bot->next && !*top->data) {
    top = top->next;
  }
  /* If all lines are empty, there is nothing to do. */
  if (top == bot->next) {
    return;
  }
  /* Allocate the tabsize plus the 'NULL-Terminator', as that is the maximum we will use. */
  if (file->syntax && file->syntax->tabstring) {
    indentation = copy_of(file->syntax->tabstring);
  }
  else {
    indentation = construct_full_tab_string(NULL);
  }
  add_undo_for(file, INDENT, NULL);
  /* Go through each of the lines, adding an indent to the non-empty ones, and recording whatever was added in the undo item. */
  DLIST_FOR_NEXT_END(top, bot->next, line) {
    real_indent = (!*line->data ? "" : indentation);
    indent_a_line_for(file, line, real_indent);
    update_multiline_undo_for(file, line->lineno, real_indent);
  }
  free(indentation);
  set_modified_for(file);
  ensure_firstcolumn_is_aligned_for(file, cols);
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Indent the current line (or the marked lines) of `openfile` by tabsize columns.  This inserts
 * either a tab character or a tab's worth of spaces, depending on whether the `TABS_TO_SPACES`
 * flag is in effect.  Note that this is context safe and works for both the gui and tui. */
void do_indent(void) {
  if (IN_GUI_CTX) {
    do_indent_for(GUI_OF, GUI_COLS);
  }
  else {
    do_indent_for(TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- Handle indent action ----------------------------- */

/* Perform an undo or redo for an indent or unindent action. */
void handle_indent_action_for(openfilestruct *const file, int rows, undostruct *const u, bool undoing, bool add_indent) {
  ASSERT(file);
  char *blanks;
  groupstruct *group = u->grouping;
  linestruct  *line  = line_from_number_for(file, group->top_line);
  /* When redoing, reposition the cursor and let the indenter adjust it. */
  if (!undoing) {
    restore_undo_posx_and_mark_for(file, rows, u);
  }
  /* For each line in the group, add or remove the individual indent. */
  while (line && line->lineno <= group->bottom_line) {
    blanks = group->indentations[line->lineno - group->top_line];
    if (undoing ^ add_indent) {
      indent_a_line_for(file, line, blanks);
    }
    else {
      unindent_a_line_for(file, line, strlen(blanks));
    }
    line = line->next;
  }
  /* When undoing, reposition the cursor to the recorded location. */
  if (undoing) {
    restore_undo_posx_and_mark_for(file, rows, u);
  }
  refresh_needed = TRUE;
}

/* Perform an undo or redo for an indent or unindent action. */
void handle_indent_action(undostruct *const u, bool undoing, bool add_indent) {
  if (IN_GUI_CTX) {
    handle_indent_action_for(GUI_OF, GUI_ROWS, u, undoing, add_indent);
  }
  else {
    handle_indent_action_for(TUI_OF, TUI_ROWS, u, undoing, add_indent);
  }
}

/* ----------------------------- Enclose str encode ----------------------------- */

/* Returns an allocated string containg `p1` `ENCLOSE_DELIM` `p2`. */
char *enclose_str_encode(const char *const restrict p1, const char *const restrict p2) {
  ASSERT(p1);
  ASSERT(p2);
  return fmtstr("%s" ENCLOSE_DELIM "%s", p1, p2);
}

/* ----------------------------- Enclose str decode ----------------------------- */

/* Decode `str` and retrieve assign the start of the enclose string to `*p1` and the end to `*p2`. */
void enclose_str_decode(const char *const restrict str, char **const p1, char **const p2) {
  ASSERT(str);
  ASSERT(p1);
  ASSERT(p2);
  const char *enclose_delim = strstr(str, ENCLOSE_DELIM);
  if (!enclose_delim) {
    die("%s: str ('%s') was not encoded properly.  Could not find ENCLOSE_DELIM '%s'.\n", __func__, str, ENCLOSE_DELIM);
  }
  *p1 = measured_copy(str, (enclose_delim - str));
  *p2 = copy_of(enclose_delim + SLTLEN(ENCLOSE_DELIM));
}

/* If `file` currently has a marked region, enclose that region where `p1` will
 * be placed on the top/start and `p2` at the bottom/end of the marked region. */
// void enclose_marked_region_for(openfilestruct *const file, const char *const restrict p1, const char *const restrict p2) {
//   ASSERT(file);
//   ASSERT(p1);
//   ASSERT(p2);
//   char *encoded_str;
//   Ulong len;
//   /* If there is no mark, just return. */
//   if (!file->mark) {
//     return;
//   }
//   /* Encode a string contaning both `p1` and `p2`, so that the undo
//    * knows how mush it should remove on the top and bottom line. */
//   encoded_str = enclose_str_encode(p1, p2);
//   add_undo_for(file, ENCLOSE, encoded_str);
//   free(encoded_str);
//   len = strlen(p1);
//   /* Mark is the `top`. */
//   if (mark_is_before_cursor_for(file)) {
//     /* Inject the  */
//     file->mark->data = xstrninj(file->mark->data, p1, len, file->mark_x);
//     file->mark_x    += len;
//     if (file->mark == file->current) {
//       file->current_x += len; 
//     }
//     file->current->data = xstrinj(file->current->data, p2, file->current_x);
//     if (file->current == file->filebot && !ISSET(NO_NEWLINES)) {
//       new_magicline_for(file);
//     }
//   }
//   /* Current is the `top`. */
//   else {
//     file->current->data = xstrninj(file->current->data, p1, len, file->current_x);
//     file->current_x    += len;
//     if (file->current == file->mark) {
//       file->mark_x += len;
//     }
//     file->mark->data = xstrinj(file->mark->data, p2, file->mark_x);
//     if (file->mark == file->filebot && !ISSET(NO_NEWLINES)) {
//       new_magicline_for(file);
//     }
//   }
//   /* Calculate the new total number of chars. */
//   file->totsize += (mbstrlen(p1) + mbstrlen(p2));
//   file->undotop->newsize = file->totsize;
//   /* Set the `modified` flag in `file`. */
//   set_modified_for(file);
//   refresh_needed = TRUE;
// }

/* ----------------------------- Enclose marked region ----------------------------- */

/* If `file` currently has a marked region, enclose that region where `p1` will
 * be placed on the top/start and `p2` at the bottom/end of the marked region. */
void enclose_marked_region_for(openfilestruct *const file, const char *const restrict p1, const char *const restrict p2) {
  ASSERT(file);
  ASSERT(p1);
  ASSERT(p2);
  char *encoded_str;
  Ulong len;
  Ulong top_x;
  Ulong bot_x;
  linestruct *top;
  linestruct *bot;
  /* If there is no mark, just return. */
  if (!file->mark) {
    return;
  }
  /* Encode a string contaning both `p1` and `p2`, so that the undo
   * knows how mush it should remove on the top and bottom line. */
  encoded_str = enclose_str_encode(p1, p2);
  add_undo_for(file, ENCLOSE, encoded_str);
  free(encoded_str);
  len = strlen(p1);
  /* Get the top and bottom lines and x positions from the mark. */
  get_region_for(file, &top, &top_x, &bot, &bot_x);
  /* Inject the first part into `top->data` at `top_x`. */
  top->data = xstrninj(top->data, p1, len, top_x);
  /* If both the region is contained to one line, advance both the cursor x and the mark x positions. */
  if (top == bot) {
    file->current_x += len;
    file->mark_x    += len;
    bot_x           += len;
  }
  /* Otherwise, when on seperate lines, and the top line is the mark, just advace the mark x position. */
  else if (file->mark == top) {
    file->mark_x += len;
  }
  /* Likewise, when its the cursor that is the top line, just advance the cursor x position. */
  else {
    file->current_x += len;
  }
  /* Inject the last part into `bot->data` at `bot_x`. */
  bot->data = xstrinj(bot->data, p2, bot_x);
  /* If the bottom line of the marked region is the bottom line of the
   * file, then make a new bottom line for the file, when requested. */
  if (bot == file->filebot && !ISSET(NO_NEWLINES)) {
    new_magicline_for(file);
    /* Also set `INCLUDED_LAST_LINE` in the undo object, so that this can be reversed. */
    file->undotop->xflags |= INCLUDED_LAST_LINE;
  }
  /* Calculate the new total number of chars. */
  file->totsize += (mbstrlen(p1) + mbstrlen(p2));
  file->undotop->newsize = file->totsize;
  /* Set the `modified` flag in `file`. */
  set_modified_for(file);
  refresh_needed = TRUE;
}

/* If the currently open file currently has a marked region, enclose that region where
 * `p1` will be placed on the top/start and `p2` at the bottom/end of the marked region. */
void enclose_marked_region(const char *const restrict p1, const char *const restrict p2) {
  enclose_marked_region_for(CTX_OF, p1, p2);
}

/* ----------------------------- Auto bracket ----------------------------- */

/* Auto insert a empty line between '{' and '}', as well as indenting the line once and setting openfile->current to it. */
void auto_bracket_for(openfilestruct *const file, linestruct *const line, Ulong posx) {
  ASSERT(file);
  ASSERT(line);
  Ulong indentlen;
  Ulong lenleft;
  linestruct *middle = make_new_node(line);
  linestruct *end    = make_new_node(middle);
  splice_node_for(file, line, middle);
  splice_node_for(file, middle, end);
  renumber_from(middle);
  indentlen = indent_length(line->data);
  lenleft   = strlen(line->data + posx);
  /* Set up the middle line. */
  middle->data   = line_indent_plus_tab(line->data, &file->current_x);
  file->totsize += file->current_x;
  /* Set up end line. */
  end->data = xmalloc(indentlen + lenleft + 1);
  memcpy((end->data + indentlen), (line->data + posx), (lenleft + 1));
  memcpy(end->data, line->data, indentlen);
  /* Set up start line. */
  line->data       = xrealloc(line->data, (posx + 1));
  line->data[posx] = '\0';
  /* Set the cursor line and x pos to the middle line. */
  file->current   = middle; 
  SET_PWW(file);
  refresh_needed = TRUE;
  /* Number of chars to next tabstop when `TABS_TO_SPACES` is set. */
  // Ulong tablen;
  // tablen    = tabstop_length(line->data, indentlen);
  // middle->data = xmalloc(indentlen + tablen + 1);
  // memcpy(middle->data, line->data, indentlen);
  // if (ISSET(TABS_TO_SPACES)) {
  //   memset((middle->data + indentlen), ' ', tablen);
  //   middle->data[indentlen + tablen] = '\0';
  //   file->totsize += tablen;
  // }
  // else {
  //   middle->data[indentlen]     = '\t';
  //   middle->data[indentlen + 1] = '\0';
  //   ++file->totsize;
  // }
  // file->current_x = (indentlen + tablen);
}

/* Auto insert a empty line between '{' and '}', as well as indenting the line once and setting openfile->current to it. */
void auto_bracket(linestruct *const line, Ulong posx) {
  auto_bracket_for(CTX_OF, line, posx);
}

/* ----------------------------- Do auto bracket for ----------------------------- */

/* Do auto bracket at current position. */
void do_auto_bracket_for(openfilestruct *const file) {
  ASSERT(file);
  Ulong indentlen;
  add_undo_for(file, AUTO_BRACKET, NULL);
  indentlen = indent_length(file->current->data);
  auto_bracket_for(file, file->current, file->current_x);
  file->totsize += ((indentlen * 2) + TAB_BYTE_LEN + 2);
  file->undotop->newsize = file->totsize;
  set_modified_for(file);
}

/* Do auto bracket at current position.  TODO: Implement this so that it correctly indents to the next `tab-stop` when using `TABS_TO_SPACES` and not. */
void do_auto_bracket(void) {
  do_auto_bracket_for(CTX_OF);
}

/* ----------------------------- Comment line ----------------------------- */

/* Test whether the given line can be uncommented, or add or remove a comment, depending on action.
 * Return TRUE if the line is uncommentable, or when anything was added or removed; FALSE otherwise.
 * ADDED: Also takes indentation into account. */
bool comment_line_for(openfilestruct *const file, undo_type action, linestruct *const line, const char *const restrict comment_seq) {
  ASSERT(file);
  ASSERT(line);
  ASSERT(comment_seq);
  bool space_existed;
  Ulong comment_seq_len = strlen(comment_seq);
  /* The postfix, if this is a bracketing type comment sequence. */
  const char *post_seq = strchr(comment_seq, '|');
  /* Length of prefix. */
  Ulong pre_len = (post_seq ? (post_seq++ - comment_seq) : comment_seq_len);
  /* Length of postfix. */
  Ulong  post_len   = (post_seq ? (comment_seq_len - pre_len - 1) : 0);
  Ulong  line_len   = strlen(line->data);
  Ushort indent_len = indent_length(line->data);
  if (!ISSET(NO_NEWLINES) && line == file->filebot) {
    return FALSE;
  }
  if (action == COMMENT) {
    /* Make room for the comment sequence(s) plus the `null-terminator` and also either one space or two. */
    line->data = xrealloc(line->data, (line_len + pre_len + post_len + ((post_len > 0) ? 3 : 2)));
    /* Inject the comment sequence at `indent_len`. */
    xnstrninj_norealloc(line->data, line_len, comment_seq, pre_len, indent_len);
    /* Then inject a space after it. */
    xstrninj_norealloc(line->data, S__LEN(" "), (indent_len + pre_len));
    /* When there is a post comment sequence, inject it at the end. */
    if (post_len > 0) {
      xnstrncat_norealloc(line->data, (line_len + pre_len), post_seq, post_len);
    }
    file->totsize += (pre_len + post_len);
    /* If needed, adjust the position of the mark. */
    if (line == file->mark && file->mark_x >= indent_len) {
      file->mark_x += (pre_len + 1);
    }
    /* Also, check if the cursor needs adjustment. */
    if (line == file->current && file->current_x >= indent_len) {
      file->current_x += (pre_len + 1);
      set_pww_for(file);
    }
    return TRUE;
  }
  /* If the line is commented, report it as uncommentable, or uncomment it. */
  if (strncmp((line->data + indent_len), comment_seq, pre_len) == 0 && (!post_len || strcmp((line->data + line_len - post_len), post_seq) == 0)) {
    /* If this was just a checking call, then return `TRUE` directly. */
    if (action == PREFLIGHT) {
      return TRUE;
    }
    /* There was a space after the comment sequence in line. */
    if (*(line->data + indent_len + pre_len) == ' ') {
      space_existed = TRUE;
      /* Erase the comment prefix by moving the non-comment part. */
      xstrn_erase_norealloc(line->data, line_len, indent_len, (pre_len + 1));
    }
    else {
      space_existed = FALSE;
      /* Erase the comment prefix by moving the non-comment part. */
      xstrn_erase_norealloc(line->data, line_len, indent_len, pre_len);
    }
    /* Truncate the postfix if there was one. */
    file->totsize -= (pre_len + post_len);
    /* Adjust the positions of the cursor, when it is behind the comment. */
    if (line == file->current && file->current_x > indent_len) {
      file->current_x -= (pre_len + space_existed);
      SET_PWW(file);
    }
    /* Adjust the position of the mark when it is behind the comment. */
    if (line == file->mark && file->mark_x > indent_len) {
      file->mark_x -= (pre_len + space_existed);
    }
    return TRUE;
  }
  return FALSE;
}

/* Test whether the given line can be uncommented, or add or remove a comment, depending on action.
 * Return TRUE if the line is uncommentable, or when anything was added or removed; FALSE otherwise.
 * ADDED: Also takes indentation into account. */
bool comment_line(undo_type action, linestruct *const line, const char *const restrict comment_seq) {
  return comment_line_for(CTX_OF, action, line, comment_seq);
}

/* ----------------------------- Get comment seq ----------------------------- */

/* Returns an allocated string containing the correct comment sequence based on `file`. */
char *get_comment_seq_for(openfilestruct *const file) {
  ASSERT(file);
  char *ret;
  /* If the file has a set comment string. */
  if (file->syntax) {
    ret = copy_of(file->syntax->comment);
  }
  /* C derived file. */
  else if (file->is_c_file || file->is_cxx_file || file->is_nanox_file) {
    ret = COPY_OF("//");
  }
  else if (file->is_nasm_file) {
    ret = COPY_OF(";");
  }
  else {
    ret = COPY_OF(GENERAL_COMMENT_CHARACTER);
  }
  return ret;
}

/* Returns an allocated string containing the correct comment sequence based on the currently
 * open file.  Note that this is context safe and works in both the `gui` and `tui`. */
char *get_comment_seq(void) {
  return get_comment_seq_for(CTX_OF);
}

/* ----------------------------- Do comment ----------------------------- */

/* Comment or uncomment the current line or the marked lines. */
void do_comment_for(openfilestruct *const file, int cols) {
  ASSERT(file);
  char *comment_seq = get_comment_seq_for(file);
  undo_type action = UNCOMMENT;
  /* The top and bottom lines of the marked range. */
  linestruct *top;
  linestruct *bot;
  /* If a line if fully empty. */
  bool empty;
  /* If all lines are empty. */
  bool all_empty = TRUE;
  /* This is when 'file->syntax' says comments are foridden. */
  if (!*comment_seq) {
    statusline(AHEM, _("Commenting is not supported for this file type"));
    free(comment_seq);
    return;
  }
  /* Determine which lines to work on. */
  get_range_for(file, &top, &bot);
  /* If only the magic line is selected, don't do anything. */
  if (top == bot && bot == file->filebot && !ISSET(NO_NEWLINES)) {
    statusline(AHEM, _("Cannot comment past end of file"));
    free(comment_seq);
    return;
  }
  /* Figure out whether to comment or uncomment the selected line or lines. */
  DLIST_FOR_NEXT_END(top, bot->next, line) {
    PREFETCH(line->next, 0, 3);
    empty = white_string(line->data);
    /* If this line is not blank and not commented, we comment all. */
    if (!empty && !comment_line_for(file, PREFLIGHT, line, comment_seq)) {
      action = COMMENT;
      break;
    }
    all_empty = (all_empty && empty);
  }
  /* If all selected lines are blank, we comment them. */
  action = (all_empty ? COMMENT : action);
  add_undo_for(file, action, NULL);
  /* Store the comment sequence used for the operation, because it could change when the file name changes; we need to know what it was. */
  file->current_undo->strdata = comment_seq;
  /* Comment/uncomment each of the selected lines when possible, and store undo data when a line changed. */
  DLIST_FOR_NEXT_END(top, bot->next, line) {
    PREFETCH(line->next, 0, 3);
    if (comment_line_for(file, action, line, comment_seq)) {
      update_multiline_undo_for(file, line->lineno, _(""));
    }
  }
  set_modified_for(file);
  ensure_firstcolumn_is_aligned_for(file, cols);
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Comment or uncomment the current line or the marked lines. */
void do_comment(void) {
  if (IN_GUI_CTX) {
    do_comment_for(GUI_OF, GUI_COLS);
  }
  else {
    do_comment_for(TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- Handle comment action ----------------------------- */

/* Perform an undo or redo for a comment or uncomment action. */
void handle_comment_action_for(openfilestruct *const file, int rows, undostruct *const u, bool undoing, bool add_comment) {
  ASSERT(file);
  ASSERT(u);
  groupstruct *group = u->grouping;
  linestruct *line;
  /* When redoing, reposition the cursor and let the commenter adjust it. */
  if (!undoing) {
    restore_undo_posx_and_mark_for(file, rows, u);
  }
  while (group) {
    line = line_from_number_for(file, group->top_line);
    while (line && line->lineno <= group->bottom_line) {
      comment_line_for(file, ((undoing ^ add_comment) ? COMMENT : UNCOMMENT), line, u->strdata);
      line = line->next;
    }
    group = group->next;
  }
  /* When undoing, reposition the cursor to the recorded location. */
  if (undoing) {
    restore_undo_posx_and_mark_for(file, rows, u);
  }
  refresh_needed = TRUE;
}

/* Perform an undo or redo for a comment or uncomment action. */
void handle_comment_action(undostruct *const u, bool undoing, bool add_comment) {
  if (IN_GUI_CTX) {
    handle_comment_action_for(GUI_OF, GUI_ROWS, u, undoing, add_comment);
  }
  else {
    handle_comment_action_for(TUI_OF, TUI_ROWS, u, undoing, add_comment);
  }
}

/* ----------------------------- Copy completion ----------------------------- */

/* Return a copy of the found completion candidate. */
char *copy_completion(const char *restrict text) {
  char *word;
  Ulong length = 0, index = 0;
  /* Find the end of the candidate word to get its length. */
  while (is_word_char((text + length), FALSE)) {
    length = step_right(text, length);
  }
  /* Now copy this candidate to a new string. */
  word = xmalloc(length + 1);
  while (index < length) {
    word[index++] = *(text++);
  }
  word[index] = '\0';
  return word;
}

/* ----------------------------- Do enter ----------------------------- */

/* Break the current line at the cursor position. */
void do_enter_for(openfilestruct *const file) {
  ASSERT(file);
  bool do_another_indent;
  bool allblanks = FALSE;
  linestruct *newnode;
  linestruct *sampleline = file->current;
  linestruct *another_indent_line;
  Ulong extra = 0;
  /* Check if cursor is between two brackets.  TODO: Create get_next_char() to perform the
   * same operation as get_previous_char() and then remake this to perform a mush better check. */
  if (cursor_is_between_brackets_for(file)) {
    do_auto_bracket_for(file);
    return;
  }
  /* If the previous char is an opening bracket, or a lable, and we are not on the same line, we should indent once more. */
  do_another_indent = (
    is_previous_char_one_of(file->current, file->current_x, "{[(:", &another_indent_line, NULL)
    /* For now when on the same line only allow doing another indent when at the end of the line. */
    && (another_indent_line != file->current || !file->current->data[file->current_x])
    && white_string(file->current->data + file->current_x)
  );
  newnode = make_new_node(file->current);
  if (ISSET(AUTOINDENT)) {
    /* When doing automatic long-line wrapping and the next line is in this same paragraph, use its indentation as the model. */
    if (ISSET(BREAK_LONG_LINES) && sampleline->next && inpar(sampleline->next) && !begpar(sampleline->next, 0)) {
      DLIST_ADV_NEXT(sampleline);
    }
    extra = indent_length(sampleline->data);
    /* When breaking in the indentation, limit the automatic one. */
    if (extra > file->current_x) {
      extra = file->current_x;
    }
    else if (extra == file->current_x) {
      allblanks = (indent_length(file->current->data) == extra);
    }
  }
  newnode->data = xmalloc(strlen(file->current->data + file->current_x) + extra + 1);
  strcpy((newnode->data + extra), (file->current->data + file->current_x));
  /* Adjust the mark if it is on the current line after the cursor. */
  if (file->mark == file->current && file->mark_x > file->current_x) {
    file->mark    = newnode;
    file->mark_x += (extra - file->current_x);
  }
  if (ISSET(AUTOINDENT)) {
    /* Copy the whitespace from the sample line to the new one. */
    memcpy(newnode->data, sampleline->data, extra);
    /* If there were only blanks before the cursor, trim them. */
    if (allblanks) {
      file->current_x = 0;
    }
  }
  /* Make the current line end at the cursor position. */
  file->current->data[file->current_x] = '\0';
  add_undo_for(file, ENTER, NULL);
  /* Insert the newly created line after the current one and renumber. */
  splice_node_for(file, file->current, newnode);
  renumber_from(newnode);
  /* Put the cursor on the new line, after any automatic whitespace. */
  file->current     = newnode;
  file->current_x   = extra;
  ++file->totsize;
  set_modified_for(file);
  if (ISSET(AUTOINDENT) && !allblanks) {
    file->totsize += extra;
  }
  /* When approptiet, add another indent. */
  if (do_another_indent) {
    file->current->data = free_and_assign(file->current->data, line_indent_plus_tab(another_indent_line->data, &file->current_x));
    /* Add only the diffrence between the current cursor position minus where the cursor was at. */
    file->totsize += (file->current_x - extra);
    /* If the indent of the new line is the same as the line we came from.  I.E: Autoindent is turned on. */
    // if (line_indent(file->current) == line_indent(file->current->prev)) {
    //   file->current->data = fmtstrcat(file->current->data, "%*s", (int)TAB_BYTE_LEN, (ISSET(TABS_TO_SPACES) ? " " : "\t"));
    //   file->current_x    += TAB_BYTE_LEN;
    //   file->totsize      += TAB_BYTE_LEN;
    // }
  }
  SET_PWW(file);
  update_undo_for(file, ENTER);
  refresh_needed = TRUE;
  focusing       = FALSE;
}

/* Break the current line at the cursor position. */
void do_enter(void) {
  do_enter_for(CTX_OF);
}

/* ----------------------------- Do undo ----------------------------- */

/* Undo the last thing(s) we did in `file`. */
void do_undo_for(CTX_ARGS) {
  ASSERT(file);
  undostruct *u = file->current_undo;
  linestruct *was_cutbuffer;
  linestruct *intruder;
  linestruct *line = NULL;
  Ulong data_len;
  Ulong original_x;
  Ulong regain_from_x;
  char *undidmsg = NULL;
  char *data;
  /* When there exists no undo stack, tell the user and return. */
  if (!u) {
    statusline(AHEM, _("Nothing to undo"));
    return;
  }
  if (u->type <= REPLACE) {
    line = line_from_number_for(file, u->tail_lineno);
  }
  /* Handle the undo.  TRANSLATORS: The next thirteen strings describe actions that are undone or redone.  They are all nouns, not verbs. */
  switch (u->type) {
    case ADD: {
      undidmsg = _("addition");
      /* When this undo event included the last line, and `NO_NEWLINES` is not
       * set, remove the magicline that was created when this action was done. */
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
        remove_magicline_for(file);
      }
      /* Erase the length of all the added data, at the starting position. */
      xstr_erase_norealloc(line->data, u->head_x, strlen(u->strdata));
      /* Move the file current line and x to that starting position. */
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      break;
    }
    case ENTER: {
      undidmsg = _("line break");
      /* An <Enter> at the end of leading whitespace while autoindenting has deleted the whitespace, and stored
       * an x position of zero.  In that case, adjust the position to return to, and to snoop data from. */
      original_x    = (!u->head_x ? u->tail_x : u->head_x);
      regain_from_x = (!u->head_x ? u->head_x : u->tail_x);
      /* Append the data to the line. */
      line->data = xstrcat(line->data, (u->strdata + regain_from_x));
      /* Make line inheret the anchor of the absorbed line. */
      line->has_anchor |= line->next->has_anchor;
      /* Remove the absorbed line from the line list. */
      unlink_node_for(file, line->next);
      /* Restore the state of the file. */
      renumber_from(line);
      file->current = line;
      goto_line_posx_for(file, rows, u->head_lineno, original_x);
      break;
    }
    case BACK:
    case DEL: {
      undidmsg = _("deletion");
      /* Save the length of the undo string data. */
      data_len = strlen(u->strdata);
      data     = xmalloc(strlen(line->data) + data_len + 1);
      memcpy(data, line->data, u->head_x);
      memcpy((data + u->head_x), u->strdata, data_len);
      strcpy((data + u->head_x + data_len), (line->data + u->head_x));
      free(line->data);
      line->data = data;
      goto_line_posx_for(file, rows, u->tail_lineno, u->tail_x);
      break;
    }
    case JOIN: {
      undidmsg = _("line join");
      /* When the join was done by a <Backspace> at the tail of the file, and the `NO_NEWLINES` flag
       * isn't set, do not re-add a newline that wasn't actually deleted.  Just position the cursor. */
      if ((u->xflags & WAS_BACKSPACE_AT_EOF) && !ISSET(NO_NEWLINES)) {
        goto_line_posx_for(file, rows, file->filebot->lineno, 0);
        focusing = FALSE;
      }
      else {
        line->data[u->tail_x] = '\0';
        intruder       = make_new_node(line);
        intruder->data = copy_of(u->strdata);
        splice_node_for(file, line, intruder);
        renumber_from(intruder);
        goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      }
      break;
    }
    case REPLACE: {
      undidmsg = _("replacement");
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
        remove_magicline_for(file);
      }
      /* Store the undo string data ptr. */
      data = u->strdata;
      /* Swap `u->strdata` and `line->data`. */
      u->strdata = line->data;
      line->data = data;
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      break;
    }
    case SPLIT_BEGIN: {
      undidmsg = _("addition");
      break;
    }
    case SPLIT_END: {
      /* Advance the current file undo-object once. */
      DLIST_ADV_NEXT(file->current_undo);
      /* Then perform all actions until we reach the beginning of this split. */
      while (file->current_undo->type != SPLIT_BEGIN) {
        do_undo_for(STACK_CTX);
      }
      /* Now to ensure correct undo stack structure, set `u` to the current file undo-object. */
      u = file->current_undo;
      break;
    }
    case ZAP:
    case CUT_TO_EOF:
    case CUT: {
      undidmsg = _((u->type == ZAP) ? "erasure" : "cut");
      undo_cut_for(file, rows, u);
      break;
    }
    case PASTE: {
      undidmsg = _("paste");
      undo_paste_for(STACK_CTX, u);
      break;
    }
    case INSERT: {
      undidmsg = _("insertion");
      /* Save the cutbuffer. */
      was_cutbuffer = cutbuffer;
      cutbuffer     = NULL;
      /* Set the cursor at the head, and the mark at the tail. */
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      set_mark_for(file, u->tail_lineno, u->tail_x);
      cut_marked_region_for(STACK_CTX);
      u->cutbuffer = cutbuffer;
      /* Restore the cutbuffer. */
      cutbuffer = was_cutbuffer;
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES) && file->current->next) {
        remove_magicline_for(file);
      }
      break;
    }
    case COUPLE_BEGIN: {
      undidmsg = u->strdata;
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      file->cursor_row = u->tail_lineno;
      adjust_viewport_for(STACK_CTX, STATIONARY);
      break;
    }
    case COUPLE_END: {
      /* Remember the row of the cursor for a possible redo. */
      file->current_undo->head_lineno = file->cursor_row;
      /* Advance to the next undo-object in file. */
      DLIST_ADV_NEXT(file->current_undo);
      do_undo_for(STACK_CTX);
      do_undo_for(STACK_CTX);
      do_undo_for(STACK_CTX);
      return;
    }
    case INDENT: {
      handle_indent_action_for(file, rows, u, TRUE, TRUE);
      undidmsg = _("indent");
      break;
    }
    case UNINDENT: {
      handle_indent_action_for(file, rows, u, TRUE, FALSE);
      undidmsg = _("unindent");
      break;
    }
    case COMMENT: {
      handle_comment_action_for(file, rows, u, TRUE, TRUE);
      undidmsg = _("comment");
      break;
    }
    case UNCOMMENT: {
      handle_comment_action_for(file, rows, u, TRUE, TRUE);
      undidmsg = _("uncomment");
      break;
    }
    case MOVE_LINE_UP:
    case MOVE_LINE_DOWN: {
      handle_move_line_action_for(file, u, TRUE);
      break;
    }
    case ENCLOSE: {
      undo_enclose_for(file, u);
      break;
    }
    case AUTO_BRACKET: {
      undo_auto_bracket_for(file, u);
      break;
    }
    case ZAP_REPLACE: {
      handle_zap_replace_action(STACK_CTX, u, TRUE);
      break;
    }
    case INSERT_EMPTY_LINE: {
      undo_insert_empty_line_for(file, rows, u);
      break;
    }
    case TAB_AUTO_INDENT: {
      handle_tab_auto_indent(file, u, TRUE);
      break;
    }
    default: {
      break;
    }
  }
  if (undidmsg && !ISSET(ZERO) && !pletion_line) {
    statusline(HUSH, _("Undid %s"), undidmsg);
  }
  DLIST_ADV_NEXT(file->current_undo);
  file->last_action = OTHER;
  /* If `keep_mark` has not been explicitly set, or when it
   * has been set but this is an exception, remove the mark. */
  if (!keep_mark || (u->xflags & SHOULD_NOT_KEEP_MARK)) {
    file->mark = NULL;
    keep_mark  = FALSE;
  }
  SET_PWW(file);
  file->totsize = u->wassize;
  if (u->type <= REPLACE) {
    check_the_multis_for(file, file->current);
  }
  else if (u->type == INSERT || u->type == COUPLE_BEGIN) {
    recook = TRUE;
  }
  /* When at the point where the `file` was last saved, unset `file->modified`. */
  if (file->current_undo == file->last_saved) {
    file->modified = FALSE;
    if (IN_CURSES_CTX) {
      titlebar(NULL);
    }
  }
  else {
    set_modified_for(file);
  }
}

/* Undo the last thing(s) we did in the currently open buffer.  Note that this is `context-safe`. */
void do_undo(void) {
  CTX_CALL(do_undo_for);
}

/* ----------------------------- Do redo ----------------------------- */

/* Redo the last thing(s) we undid in `file`. */
void do_redo_for(CTX_ARGS) {
  ASSERT(file);
  undostruct *u = file->undotop;
  linestruct *line = NULL;
  linestruct *intruder;
  bool suppress_modification = FALSE;
  char *redidmsg = NULL;
  char *data;
  Ulong data_len;
  int offset;
  /* If there has been no undo(s) done, or we have reached the end. */
  if (!u || u == file->current_undo) {
    statusline(AHEM, _("Nothing to redo"));
    return;
  }
  /* Find the item before the current one in the undo-stack. */
  while (u->next != file->current_undo) {
    DLIST_ADV_NEXT(u);
  }
  if (u->type <= REPLACE) {
    line = line_from_number_for(file, u->tail_lineno);
  }
  /* Handle the redo-action. */
  switch (u->type) {
    case ADD: {
      redidmsg = _("addition");
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
        new_magicline_for(file);
      }
      data_len = strlen(u->strdata);
      data = xmalloc(strlen(line->data) + data_len + 1);
      memcpy(data, line->data, u->head_x);
      memcpy((data + u->head_x), u->strdata, data_len);
      strcpy((data + u->head_x + data_len), (line->data + u->head_x));
      free(line->data);
      line->data = data;
      goto_line_posx_for(file, rows, u->tail_lineno, u->tail_x);
      break;
    }
    case ENTER: {
      redidmsg              = _("line break");
      line->data[u->head_x] = '\0';
      intruder              = make_new_node(line);
      intruder->data        = copy_of(u->strdata);
      splice_node_for(file, line, intruder);
      renumber_from(intruder);
      goto_line_posx_for(file, rows, (u->head_lineno + 1), u->tail_x);
      break;
    }
    case BACK:
    case DEL: {
      redidmsg = _("deletion");
      data_len = strlen(u->strdata);
      memmove((line->data + u->head_x), (line->data + u->head_x + data_len), (strlen(line->data + u->head_x) + data_len + 1));
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      break;
    }
    case JOIN: {
      redidmsg = _("line join");
      /* When the join was done by a <Backspace> at the tail of the file, and the `NO_NEWLINES` flag
       * isn't set, do not join anything, as nothing was actually deleted.  Just position the cursor. */
      if ((u->xflags & WAS_BACKSPACE_AT_EOF) && !ISSET(NO_NEWLINES)) {
        goto_line_posx_for(file, rows, u->tail_lineno, u->tail_x);
        break;
      }
      line->data = xstrcat(line->data, u->strdata);
      unlink_node_for(file, line->next);
      renumber_from(line);
      file->current = line;
      goto_line_posx_for(file, rows, u->tail_lineno, u->tail_x);
      break;
    }
    case REPLACE: {
      redidmsg = _("replacement");
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
        new_magicline_for(file);
      }
      SWAP(u->strdata, line->data);
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      break;
    }
    case SPLIT_BEGIN: {
      file->current_undo = u;
      while (file->current_undo->type != SPLIT_END) {
        do_redo_for(STACK_CTX);
      }
      u = file->current_undo;
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      ensure_firstcolumn_is_aligned_for(file, cols);
      break;
    }
    case SPLIT_END: {
      redidmsg = _("addition");
      break;
    }
    case ZAP:
    case CUT_TO_EOF:
    case CUT: {
      redidmsg = _((u->type == ZAP) ? "erasure" : "cut");
      redo_cut_for(STACK_CTX, u);
      break;
    }
    case PASTE: {
      redidmsg = _("paste");
      redo_paste_for(file, rows, u);
      break;
    }
    case INSERT: {
      redidmsg = _("insertion");
      goto_line_posx_for(file, rows, u->head_lineno, u->head_x);
      if (u->cutbuffer) {
        copy_from_buffer_for(file, rows, u->cutbuffer);
      }
      else {
        suppress_modification = TRUE;
      }
      free_lines_for(file, u->cutbuffer);
      u->cutbuffer = NULL;
      break;
    }
    case COUPLE_BEGIN: {
      file->current_undo = u;
      do_redo_for(STACK_CTX);
      do_redo_for(STACK_CTX);
      do_redo_for(STACK_CTX);
      return;
    }
    case COUPLE_END: {
      redidmsg = u->strdata;
      goto_line_posx_for(file, rows, u->tail_lineno, u->tail_x);
      file->cursor_row = u->head_lineno;
      adjust_viewport_for(STACK_CTX, STATIONARY);
      break;
    }
    case INDENT: {
      handle_indent_action_for(file, rows, u, FALSE, TRUE);
      redidmsg = _("indent");
      break;
    }
    case UNINDENT: {
      handle_indent_action_for(file, rows, u, FALSE, FALSE);
      redidmsg = _("unindent");
      break;
    }
    case COMMENT: {
      handle_comment_action_for(file, rows, u, FALSE, TRUE);
      redidmsg = _("comment");
      break;
    }
    case UNCOMMENT: {
      handle_comment_action_for(file, rows, u, FALSE, FALSE);
      redidmsg = _("uncomment");
      break;
    }
    case MOVE_LINE_UP:
    case MOVE_LINE_DOWN: {
      handle_move_line_action_for(file, u, FALSE);
      break;
    }
    case ENCLOSE: {
      redo_enclose_for(file, u);
      break;
    }
    case AUTO_BRACKET: {
      file->current   = line_from_number_for(file, u->head_lineno);
      file->current_x = u->head_x;
      /* Ensure the undo-redo correctness is enforced. */
      ALWAYS_ASSERT_MSG(cursor_is_between_brackets_for(file), "Undo-redo stack is not correct");
      auto_bracket_for(file, file->current, file->current_x);
      break;
    }
    case INSERT_EMPTY_LINE: {
      offset = ((u->xflags & INSERT_WAS_ABOVE) ? -1 : 0);
      file->current = line_from_number_for(file, ((((u->xflags & MARK_WAS_SET) && !(u->xflags & CURSOR_WAS_AT_HEAD)) ? u->tail_lineno : u->head_lineno) + offset));
      insert_empty_line_for(file, file->current, !!offset, TRUE);
      if (!offset) {
        DLIST_ADV_NEXT(file->current);
      }
      else {
        DLIST_ADV_PREV(file->current);
      }
      refresh_needed = TRUE;
      break;
    }
    case ZAP_REPLACE: {
      handle_zap_replace_action(STACK_CTX, u, FALSE);
      break;
    }
    case TAB_AUTO_INDENT: {
      handle_tab_auto_indent(file, u, FALSE);
      break;
    }
    default: {
      break;
    }
  }
  if (redidmsg && !ISSET(ZERO)) {
    statusline(HUSH, _("Redid %s"), redidmsg);
  }
  file->current_undo = u;
  file->last_action  = OTHER;
  if (!keep_mark || (u->xflags & SHOULD_NOT_KEEP_MARK)) {
    file->mark = NULL;
    keep_mark  = FALSE;
  }
  set_pww_for(file);
  file->totsize = u->newsize;
  if (u->type <= REPLACE) {
    check_the_multis_for(file, file->current);
  }
  else if (u->type == INSERT || u->type == COUPLE_END) {
    recook = TRUE;
  }
  /* When at the point where the `file` was last saved, unset `file->modified`. */
  if (file->current_undo == file->last_saved) {
    file->modified = FALSE;
    if (IN_CURSES_CTX) {
      titlebar(NULL);
    }
  }
  else if (!suppress_modification) {
    set_modified_for(file);
  }
}

/* Redo the last thing(s) we undid in the currently open buffer.  Note that this is `context-safe`. */
void do_redo(void) {
  CTX_CALL(do_redo_for);
}

/* ----------------------------- Count lines words and characters ----------------------------- */

/* Our own version of `wc`.  Note that the character count is in
 * multibyte characters instead of signle byte ascii characters. */
void count_lines_words_and_characters_for(openfilestruct *const file) {
  ASSERT(file);
  linestruct *was_current = file->current;
  linestruct *top;
  linestruct *bot;
  Ulong was_x = file->current_x;
  Ulong words = 0;
  Ulong chars = 0;
  Ulong top_x;
  Ulong bot_x;
  long lines = 0;
  /* The file has a marked region. */
  if (file->mark) {
    get_region_for(file, &top, &top_x, &bot, &bot_x);
    /* When the top and bottom lines are not the same, count the chars from the line after the top line. */
    if (top != bot) {
      chars = (number_of_characters_in(top->next, bot) + 1);
    }
    /* Now add the chars at the top line after top_x and remove all chars on the bottom line after bot_x. */
    chars += (mbstrlen(top->data + top_x) - mbstrlen(bot->data + bot_x));
  }
  /* Otherwise, use our saved values to determine the total number of chars. */
  else {
    top   = file->filetop;
    top_x = 0;
    bot   = file->filebot;
    bot_x = strlen(file->filebot->data);
    chars = file->totsize;
  }
  /* Compute the number of lines. */
  lines = ((bot->lineno - top->lineno) + !(!bot_x || (top == bot && top_x == bot_x)));
  /* Set the current cursor line and position in file. */
  file->current   = top;
  file->current_x = top_x;
  /* Keep stepping to the next word (considering punctuation as part of a word, as "wc -w" does...?),
   * until we reach the end of the relevent area, increamentng the word cound for each successful step. */
  while (file->current->lineno < bot->lineno || (file->current == bot && file->current_x < bot_x)) {
    if (do_next_word_for(file, FALSE, ISSET(WORD_BOUNDS))) {
      ++words;
    }
  }
  /* Restore where we were. */
  file->current   = was_current;
  file->current_x = was_x;
  /* Report on the status-bar the number of lines, words and characters. */
  statusline(
    INFO, _("%s%ld %s,  %lu %s,  %lu %s"),
    (file->mark ? _("In Selection:  ") : ""),
    lines, P_("line", "lines", lines),
    words, P_("word", "words", words),
    chars, P_("character", "characters", chars)
  );
}

/* Our own version of `wc`.  Note that the character count is in multibyte characters
 * instead of signle byte ascii characters.  And also that this function is `context-safe`. */
void count_lines_words_and_characters(void) {
  count_lines_words_and_characters_for(CTX_OF);
}

/* ----------------------------- Do formatter ----------------------------- */

/* Run a formatter on the contents of `file`. */
void do_formatter_for(CTX_ARGS, char *formatter) {
  ASSERT(file);
  FILE *stream;
  char *temp_name;
  bool okay = FALSE;
  ran_a_tool = TRUE;
  /* Only perform any action when not in restricted mode. */
  if (!in_restricted_mode()) {
    if (!formatter || !*formatter) {
      statusline(AHEM, _("No formatter is defined for this type of file"));
      return;
    }
    file->mark = NULL;
    temp_name = safe_tempfile_for(file, &stream);
    if (temp_name) {
      okay = write_file_for(file, temp_name, stream, TEMPORARY, OVERWRITE, NONOTES);
    }
    if (!okay) {
      statusline(ALERT, _("Error writing temp file: %s"), strerror(errno));
    }
    else {
      treat_for(STACK_CTX, temp_name, formatter, FALSE);
    }
    unlink(temp_name);
    free(temp_name);
  }
}

/* Run a formatter on the contents of the currently open buffer.  Note that this is `context-safe`, and
 * that this uses the buffers syntax->formatter if it exists, otherwise, will inform the user and return. */
void do_formatter(void) {
  if (IN_GUI_CTX) {
    do_formatter_for(GUI_OF, GUI_RC, PASS_FIELD_IF_VALID(GUI_OF->syntax, formatter, NULL));
  }
  else {
    do_formatter_for(TUI_OF, TUI_RC, PASS_FIELD_IF_VALID(TUI_OF->syntax, formatter, NULL));
  }
}

/* ----------------------------- Do spell ----------------------------- */

/* Spell-check `file`.  If an alternate spell-checker is specified, use it.  Otherwise, use the
 * internal spell checker...?  There is none, do they mean hunspell/spell.  Those are not internal. */
void do_spell_for(CTX_ARGS) {
  ASSERT(file);
  FILE *stream;
  char *tempfile;
  bool okay;
  ran_a_tool = TRUE;
  if (!in_restricted_mode()) {
    tempfile = safe_tempfile_for(file, &stream);
    if (!tempfile) {
      statusline(ALERT, _("Error writing temp-file: %s"), strerror(errno));
      return;
    }
    if (file->mark) {
      okay = write_region_to_file_for(file, tempfile, stream, TEMPORARY, OVERWRITE);
    }
    else {
      okay = write_file_for(file, tempfile, stream, TEMPORARY, OVERWRITE, NONOTES);
    }
    if (!okay) {
      statusline(ALERT, _("Error writing temp-file: %s"), strerror(errno));
      unlink(tempfile);
      free(tempfile);
      return;
    }
    blank_bottombars();
    if (alt_speller && *alt_speller) {
      treat_for(STACK_CTX, tempfile, alt_speller, TRUE);
    }
    else {
      do_int_speller_for(STACK_CTX, tempfile);
    }
    unlink(tempfile);
    free(tempfile);
    /* Ensure the help lines will be redrawn and a selection is retained. */
    currmenu   = MMOST;
    shift_held = TRUE;
  }
}

/* Spell-check the `currently open buffer`.  If an alternate spell-checker is specified, use it.  Otherwise, use the internal
 * spell checker...?  There is none, do they mean hunspell/spell.  Those are not internal.  Note that this is `context-safe`. */
void do_spell(void) {
  CTX_CALL(do_spell_for);
}

/* ----------------------------- Do justify ----------------------------- */

/* Justify the `current paragraph` in `file`. */
void do_justify_for(CTX_ARGS) {
  justify_text_for(STACK_CTX, ONE_PARAGRAPH);
}

/* Justify the `current paragraph` in the `currently open buffer`.  Note that this is `context-safe`. */
void do_justify(void) {
  CTX_CALL(do_justify_for);
}

/* ----------------------------- Do full justify ----------------------------- */

/* Justify the entirty of `file`. */
void do_full_justify_for(CTX_ARGS) {
  justify_text_for(STACK_CTX, WHOLE_BUFFER);
  ran_a_tool = TRUE;
  recook     = TRUE;
}

/* Justify the entirty of the `currently open buffer`.  Note that this is `context-safe`. */
void do_full_justify(void) {
  CTX_CALL(do_full_justify_for);
}

/* ----------------------------- Do linter ----------------------------- */

void do_linter_for(CTX_ARGS_REF_OF, char *const linter) {
  ASSERT(file);
  char *lintings;
  char *pointer;
  char *onelint;
  char *filename;
  char *linestring;
  char *colstring;
  char *complaint;
  char *spacer;
  char *dontwantfile;
  char *msg;
  char **lintargs;
  int choice;
  int kbinput;
  int lint_status;
  int lint_fd[2];
  functionptrtype function;
  struct stat info;
  const openfilestruct *started_at;
  long pipesize;
  long lineno;
  long colno;
  Ulong bufsize;
  Ulong bytesread;
  Ulong totalread = 0;
  pid_t pid_lint;
  lintstruct *lints   = NULL;
  lintstruct *tmplint = NULL;
  lintstruct *curlint = NULL;
  lintstruct *reslint = NULL;
  time_t last_wait  = 0;
  bool parsesuccess = FALSE;
  bool helpless     = ISSET(NO_HELP);
  ran_a_tool = TRUE;
  /* Do not allow this to perform any actions in restricted-mode, or when not in curses-mode (for now). */
  if (in_restricted_mode() || !IN_CURSES_CTX) {
    return;
  }
  /* If the linter passed is not a valid string, or its empty inform the user and return. */
  if (!linter || !*linter) {
    statusline(AHEM, _("No linter is defined for this type of file"));
    return;
  }
  (*file)->mark = NULL;
  edit_refresh_for(STACK_CTX_DF);
  if ((*file)->modified) {
    choice = ask_user(YESORNO, _("Save modified buffer before linting?"));
    if (choice == CANCEL) {
      statusbar_all(_("Cancelled"));
      return;
    }
    else if (choice == YES && write_it_out_for(*file, FALSE, FALSE) != 1) {
      return;
    }
  }
  /* Create a pipe up front...like always before using it... */
  if (pipe(lint_fd) == -1) {
    statusline(ALERT, _("Could not create pipe: %s"), strerror(errno));
    return;
  }
  blank_bottombars();
  currmenu = MLINTER;
  statusbar_all(_("Invoking linter..."));
  /* If we fail to fork, inform the user and return. */
  if ((pid_lint = fork()) == -1) {
    statusline(ALERT, _("Could not fork: %s"), strerror(errno));
    close(lint_fd[0]);
    close(lint_fd[1]);
    return;
  }
  /* Child */
  else if (pid_lint == 0) {
    /* Child: Redirect standard output/error into the pipe. */
    if (dup2(lint_fd[1], STDOUT_FILENO) < 0) {
      exit(7);
    }
    if (dup2(lint_fd[1], STDERR_FILENO) < 0) {
      exit(8);
    }
    close(lint_fd[0]);
    close(lint_fd[1]);
    construct_argument_list(&lintargs, linter, (*file)->filename);
    /* Child: Start the linter program.  Note that we are using $PATH. */
    execvp(lintargs[0], lintargs);
    /* Child: This is only reached when the linter is not found. */
    exit(9);
  }
  /* Parent */
  else {
    /* Parent: Close the unused write end of the pipe. */
    close(lint_fd[1]);
    /* Get the system pipe buffer size. */
    pipesize = fpathconf(lint_fd[0], _PC_PIPE_BUF);
    if (pipesize < 1) {
      statusline(ALERT, _("Could not get size of pipe buffer"));
      close(lint_fd[0]);
      return;
    }
    /* Block resizing signals while reading from a pipe. */
    BLOCK_SIGWINCH_ACTION(
      /* Read in the returned syntax errors. */
      bufsize  = (pipesize + 1);
      lintings = xmalloc(bufsize);
      pointer  = lintings;
      while ((bytesread = read(lint_fd[0], pointer, pipesize)) > 0) {
        totalread += bytesread;
        bufsize   += pipesize;
        lintings   = xrealloc(lintings, bufsize);
        pointer    = (lintings + totalread);
      }
      *pointer = NUL;
      close(lint_fd[0]);
    );
    pointer = lintings;
    onelint = lintings;
    /* Now parse the output of the linter. */
    while (*pointer) {
      if (*pointer == CR || *pointer == LF) {
        *pointer = NUL;
        if (onelint != pointer) {
          complaint = copy_of(onelint);
          /* Why is not strchr used here...? */
          spacer = strstr(complaint, " ");
          /* The recognized format is 'filename:line:column: message',
           * where ':column' may be absent or be ',column' instead. */
          if ((filename = strtok(onelint, ":")) && spacer) {
            if ((linestring = strtok(NULL, ":"))) {
              if ((colstring = strtok(NULL, " "))) {
                lineno = strtol(linestring, NULL, 10);
                colno  = strtol(colstring,  NULL, 10);
                if (lineno <= 0) {
                  free(complaint);
                  ++pointer;
                  return;
                }
                else if (colno <= 0) {
                  colno = 1;
                  strtok(linestring, ",");
                  if ((colstring = strtok(NULL, ","))) {
                    colno = strtol(colstring, NULL, 10);
                  }
                }
                parsesuccess = TRUE;
                tmplint = curlint;
                curlint = xmalloc(sizeof(*curlint));
                curlint->next = NULL;
                curlint->prev = tmplint;
                if (curlint->prev) {
                  curlint->prev->next = curlint;
                }
                curlint->filename = copy_of(filename);
                curlint->lineno   = lineno;
                curlint->colno    = colno;
                curlint->msg      = copy_of(spacer + 1);
                if (!lints) {
                  lints = curlint;
                }
              }
            }
          }
          free(complaint);
        }
        onelint = (pointer + 1);
      }
      ++pointer;
    }
    free(lintings);
    /* Process the end of the linting process. */
    waitpid(pid_lint, &lint_status, 0);
    if (!WIFEXITED(lint_status) || WEXITSTATUS(lint_status) > 2) {
      statusline(ALERT, _("Error invoking '%s'"), linter);
      return;
    }
    else if (!parsesuccess) {
      statusline(REMARK, _("Got 0 parsable lines from command: %s"), linter);
      return;
    }
    if (helpless && LINES > 5) {
      UNSET(NO_HELP);
      window_init();
    }
    /* Show that we are in the linter now. */
    titlebar(NULL);
    bottombars(MLINTER);
    tmplint = NULL;
    curlint = lints;
    while (1) {
      if (stat(curlint->filename, &info) != -1 && (!(*file)->statinfo || (*file)->statinfo->st_ino != info.st_ino)) {
        started_at = (*file);
        DLIST_ADV_NEXT(*file);
        while (*file != started_at && (!(*file)->statinfo || (*file)->statinfo->st_ino != info.st_ino)) {
          DLIST_ADV_NEXT(*file);
        }
        if (!(*file)->statinfo || (*file)->statinfo->st_ino != info.st_ino) {
          msg    = fmtstr(_("This message is for an unopened file %s, open it in a new buffer?"), curlint->filename);
          choice = ask_user(YESORNO, msg);
          free(msg);
          currmenu = MLINTER;
          if (choice == CANCEL) {
            statusbar_all(_("Cancelled"));
            break;
          }
          else if (choice == YES) {
            open_buffer(curlint->filename, TRUE);
          }
          else {
            dontwantfile = copy_of(curlint->filename);
            reslint      = NULL;
            while (curlint) {
              if (strcmp(curlint->filename, dontwantfile) == 0) {
                if (curlint == lints) {
                  lints = curlint->next;
                }
                else {
                  curlint->prev->next = curlint->next;
                }
                if (curlint->next) {
                  curlint->next->prev = curlint->prev;
                }
                tmplint = curlint;
                DLIST_ADV_NEXT(curlint);
                free(tmplint->msg);
                free(tmplint->filename);
                free(tmplint);
              }
              else {
                if (!reslint) {
                  reslint = curlint;
                }
                DLIST_ADV_NEXT(curlint);
              }
            }
            free(dontwantfile);
            if (!reslint) {
              statusline(REMARK, _("No messages for this file"));
              break;
            }
            else {
              curlint = reslint;
              continue;
            }
          }
        }
      }
      if (tmplint != curlint) {
        /* Put the cursor at the repoted position, but don't go beyond EOL
         * when the second number is a column number instead of an index. */
        goto_line_posx_for(*file, rows, curlint->lineno, (curlint->colno - 1));
        (*file)->current_x = actual_x((*file)->current->data, (*file)->placewewant);
        titlebar(NULL);
        adjust_viewport_for(STACK_CTX_DF, CENTERING);
        confirm_margin_for(*file, &TUI_COLS);
        edit_refresh_for(STACK_CTX_DF);
        statusline(NOTICE, "%s", curlint->msg);
        bottombars(MLINTER);
      }
      /* Place the cursor to indecate the affected line. */
      place_the_cursor_for(*file);
      wnoutrefresh(midwin);
      kbinput = get_kbinput(footwin, VISIBLE);
      if (kbinput == KEY_WINCH) {
        continue;
      }
      function = func_from_key(kbinput);
      tmplint  = curlint;
      if (function == do_cancel || function == do_enter) {
        wipe_statusbar();
        break;
      }
      else if (function == do_help) {
        tmplint = NULL;
        do_help();
      }
      else if (function == do_page_up || function == to_prev_block) {
        if (curlint->prev) {
          DLIST_ADV_PREV(curlint);
        }
        else if (last_wait != time(NULL)) {
          statusbar_all(_("At first message"));
          beep();
          napms(600);
          last_wait = time(NULL);
          statusline(NOTICE, "%s", curlint->msg);
        }
      }
      else if (function == do_page_down || function == to_next_block) {
        if (curlint->next) {
          DLIST_ADV_NEXT(curlint);
        }
        else if (last_wait != time(NULL)) {
          statusbar_all(_("At last message"));
          beep();
          napms(600);
          last_wait = time(NULL);
          statusline(NOTICE, "%s", curlint->msg);
        }
      }
      else {
        beep();
      }
    }
    for (curlint=lints; curlint;) {
      tmplint = curlint;
      DLIST_ADV_NEXT(curlint);
      free(tmplint->msg);
      free(tmplint->filename);
      free(tmplint);
    }
    if (helpless) {
      SET(NO_HELP);
      window_init();
      refresh_needed = TRUE;
    }
    lastmessage = VACUUM;
    currmenu    = MMOST;
    titlebar(NULL);
  }
}

/* Run a linting program on the `currently open buffer`.  Note that this only works in curses-mode for now. */
void do_linter(void) {
  if (IN_CURSES_CTX) {
    do_linter_for(&TUI_OF, TUI_RC, PASS_FIELD_IF_VALID(TUI_OF->syntax, linter, NULL));
  }
}

/* ----------------------------- Complete a word ----------------------------- */

/* TODO: This will be broken if ran on a file, then swapping to another file, maybe...?  But
 * probebly not as the `pletion_line` should be set to `NULL` when handleing the file swapping. */
void complete_a_word_for(CTX_ARGS_REF_OF) {
  ASSERT(file);
  ASSERT(*file);
  /* The buffer that is beeing searched for possible completions. */
  static openfilestruct *scouring = NULL;
  /* A linked list of the completions that have been attempted. */
  static completionstruct *list_of_completions;
  /* The x postition in `pletion line` of the last found completion. */
  static int pletion_x = 0;
  completionstruct *dropit;
  completionstruct *some_word;
  /* The point where we can stop searching for shard. */
  long threshold;
  bool was_set_wrapping = ISSET(BREAK_LONG_LINES);
  Ulong shard_length = 0;
  char *shard;
  char *completion;
  long i;
  long j;
  if (!IN_CURSES_CTX) {
    return;
  }
  /* If this is a fresh completion attempt... */
  if (!pletion_line) {
    /* Clear the list of words of a previous completion run. */
    while (list_of_completions) {
      dropit = list_of_completions;
      list_of_completions = dropit->next;
      free(dropit->word);
      free(dropit);
    }
    /* Prevent a completion from being merged with typed text. */
    (*file)->last_action = OTHER;
    /* Initalize the starting point for searching. */
    scouring     = (*file);
    pletion_line = (*file)->filetop;
    pletion_x    = 0;
    /* Wipe the `No further matches` message. */
    wipe_statusbar();
  }
  else {
    /* Remove the attempted completion from the buffer. */
    do_undo_for(STACK_CTX_DF);
  }
  /* Find the start of the fragment that the user typed. */
  shard = prev_word_get((*file)->current->data, (*file)->current_x, &shard_length, TRUE);
  if (!shard) {
    /* TRANSLATORS: Shown when no text is directly to the left of the cursor. */
    statusline(AHEM, _("No word fragment"));
    pletion_line = NULL;
    return;
  }
  /* Run through all of the lines in the buffer, looking for shard. */
  while (pletion_line) {
    threshold = (strlen(pletion_line->data) - shard_length - 1);
    /* Traverse the whole line, looking for shard. */
    for (i=pletion_x; i<threshold; ++i) {
      /* If the first byte doesn't match, run on. */
      if (pletion_line->data[i] != *shard) {
        continue;
      }
      /* Compare the rest of the bytes in shard. */
      for (j=1; LT(j,shard_length) && pletion_line->data[i+j]==shard[j]; ++j);
      /* If not all bytes matched, continue searching. */
      if (LT(j, shard_length)) {
        continue;
      }
      /* If the found match is not /longer/ then shard, skip it. */
      if (!is_word_char((pletion_line->data + i + j), FALSE)) {
        continue;
      }
      /* If the match is not a seperate word, skip it. */
      if (i > 0 && is_word_char((pletion_line->data + step_left(pletion_line->data, i)), FALSE)) {
        continue;
      }
      /* If this match is the shard itself, ignore it. */
      if (pletion_line == (*file)->current && EQ(i, ((*file)->current_x - shard_length))) {
        continue;
      }
      completion = copy_completion(pletion_line->data + i);
      /* Look among earlier attempted completions for a duplicate. */
      some_word = list_of_completions;
      while (some_word && strcmp(some_word->word, completion) != 0) {
        DLIST_ADV_NEXT(some_word);
      }
      /* If we're already tried this word, skip it. */
      if (some_word) {
        free(completion);
        continue;
      }
      /* Add the found word to the list of completions. */
      some_word           = xmalloc(sizeof(*some_word));
      some_word->word     = completion;
      some_word->next     = list_of_completions;
      list_of_completions = some_word;
      /* Temporarily disable line wrapping so only one undo item is added. */
      UNSET(BREAK_LONG_LINES);
      /* Inject the completion into the buffer. */
      inject_into_buffer(STACK_CTX_DF, (completion + shard_length), (strlen(completion) - shard_length));
      /* If needed, reenable wrapping and wrap the current line. */
      if (was_set_wrapping) {
        SET(BREAK_LONG_LINES);
        do_wrap_for(*file, cols);
      }
      /* Set the position for a possible next search attempt. */
      pletion_x = ++i;
      free(shard);
      return;
    }
    DLIST_ADV_NEXT(pletion_line);
    pletion_x = 0;
    /* When at end off buffer and there is another, search that one. */
    if (!pletion_line && scouring->next != (*file)) {
      DLIST_ADV_NEXT(scouring);
      pletion_line = scouring->filetop;
    }
  }
  /* The search has gone through all buffers. */
  if (list_of_completions) {
    edit_refresh_for(STACK_CTX_DF);
    statusline(AHEM, _("No further matches"));
  }
  else {
    /* TRANSLATORS: Shown when there are zero possible completions. */
    statusline(AHEM, _("No matches"));
  }
  free(shard);
}

void complete_a_word(void) {
  if (IN_GUI_CTX) {
    complete_a_word_for(&GUI_OF, GUI_RC);
  }
  else {
    complete_a_word_for(&TUI_OF, TUI_RC);
  }
}

/* ----------------------------- Find paragraph ----------------------------- */

/* Find the first occurring paragraph in the forward direction.  Return `TRUE` when a paragraph was found,
 * and `FALSE` otherwise.  Furthermore, return the first line and the number of lines of the paragraph. */
bool find_paragraph(linestruct **const first, Ulong *const count) {
  ASSERT(first);
  ASSERT(count);
  linestruct *line = (*first);
  /* When not currently in a paragraph, move forward until that is. */
  while (!inpar(line) && line->next) {
    PREFETCH(line->next, 0, 3);
    PREFETCH(line->next->next, 0, 3);
    DLIST_ADV_NEXT(line);
  }
  (*first) = line;
  /* Move down to the last line of the paragraph (if any). */
  do_para_end(&line);
  /* When not in a paragraph now, there aren't any paragraphs left. */
  if (!inpar(line)) {
    return FALSE;
  }
  /* We found a paragraph.  Now we determine it's number of lines. */
  else {
    (*count) = (line->lineno - (*first)->lineno + 1);
    return TRUE;
  }
}

/* ----------------------------- Do verbatim input ----------------------------- */

/* Get verbatim input.  This is used to insert a Unicode character by it's hexadecimal code, which is typed
 * in by the user.  The function returns the butes that were typed in, and the number of bytes that were read.  No..? */
void do_verbatim_input(void) {
  Ulong count = 1;
  char *bytes;
  /* Only perform any action when in curses mode. */
  if (IN_CURSES_CTX) {
    /* When barless and with cursor on the bottom row, make room for the feedback. */
    if (ISSET(ZERO) && openfile->cursor_row == (editwinrows - 1) && LINES > 1) {
      edit_scroll_for(openfile, FORWARD);
      edit_refresh();
    }
    /* TRANSLATORS: Shown when the next keystroke will be inserted verbatim. */
    statusline_curses(INFO, _("Verbatim Input"));
    place_the_cursor_curses_for(openfile);
    /* Read in the first one or two bytes of the next keystroke. */
    bytes = get_verbatim_kbinput(midwin, &count);
    /* When something valid was obtained, unsuppress cursor-position display,
     * insert the bytes into the edit buffer, and blank the status-bar. */
    if (count > 0) {
      if (ISSET(CONSTANT_SHOW) || ISSET(MINIBAR)) {
        lastmessage = VACUUM;
      }
      if (count < 999) {
        inject(bytes, count);
      }
      /* Ensure that the feedback will be overwritten, or clear it. */
      if (ISSET(ZERO) && currmenu == MMAIN) {
        wredrawln(midwin, (editwinrows - 1), 1);
      }
      else {
        wipe_statusbar();
      }
    }
    else {
      /* TRANSLATORS: An invalid verbatim Unicode code was typed. */
      statusline_curses(AHEM, _("Invalid code"));
    }
    free(bytes);
  }
}

/* ----------------------------- Get previous char ----------------------------- */

/* Returns a pointer to the previous character (left of `xpos` in `line`) that is neither blank nor
 * zero-width.  If `xpos` is at the start of `line`, move to the previous line and continue searching
 * from its end.  Continue across lines until a non-blank, non-zero-width character is found, or return
 * `NULL` at buffer start. The function advances `line` and `xpos` as needed to traverse backward. */
char *get_previous_char(linestruct *line, Ulong xpos, linestruct **const outline, Ulong *const outxpos) {
  ASSERT(line);
  while (1) {
    /* We are at the start of the line. */
    if (!xpos) {
      /* If we are at the very top of the linked list of lines, return NULL, there are no previous chars. */
      if (!line->prev) {
        return NULL;
      }
      /* Otherwise, Move to the line above, and place the position at the end of the line. */
      else {
        PREFETCH(line->prev, 0, 3);
        DLIST_ADV_PREV(line);
        PREFETCH(line->data, 0, 3);
        xpos = strlen(line->data);
      }
    }
    /* Otherwise, we move once to the left. */
    else {
      xpos = step_left(line->data, xpos);
      /* When we find the first non blank and non zerowidth char, return it. */
      if (!is_blank_char(line->data + xpos) && !is_zerowidth(line->data + xpos)) {
        /* If the caller want to know where we ended up, assign out final line and x position. */
        ASSIGN_IF_VALID(outline, line);
        ASSIGN_IF_VALID(outxpos, xpos);
        return (line->data + xpos);
      }
    }
  }
}

/* ----------------------------- Get next char ----------------------------- */

/* Returns a pointer to the next character (at or right of `xpos` in `line`) that is neither a blank nor a
 * zero-width.  If `xpos` is at the end of the `line`, move to the next line and continue searching from it's
 * start.  Continue across lines until a non-blank, non-zero-width character is found, or return `NULL` at
 * buffer end.  The function advances `line` and `xpos` as needed to traverse forward.  Also note that both
 * `outline` and `outxpos` can be `NULL`, if they are not, the position we ended up in will be assigned to them. */
char *get_next_char(linestruct *line, Ulong xpos, linestruct **const outline, Ulong *const outxpos) {
  ASSERT(line);
  while (1) {
    /* We are at the end of the line. */
    if (!line->data[xpos]) {
      /* If there is no more lines, return NULL. */
      if (!line->next) {
        return NULL;
      }
      /* Otherwise, move to the next line, and place the position at the start of the line. */
      else {
        PREFETCH(line->next, 0, 3);
        DLIST_ADV_NEXT(line);
        PREFETCH(line->data, 0, 3);
        xpos = 0;
      }
    }
    /* Otherwise */
    else {
      /* Check if we are at a non-blank, non-zero-width char.  If so, we stop here. */
      if (!is_blank_char(line->data + xpos) && !is_zerowidth(line->data + xpos)) {
        /* If the caller wants to know what line, and where in the line we ended up, tell them. */
        ASSIGN_IF_VALID(outline, line);
        ASSIGN_IF_VALID(outxpos, xpos);
        return (line->data + xpos);
      }
      /* Otherwise, we move to the right. */
      else {
        xpos = step_right(line->data, xpos);
      }
    }
  }
}

/* ----------------------------- Is previous char one of ----------------------------- */

/* Returns `TRUE` when the previous char before the current `line,xpos` is one of `mathces`, this retrieves the
 * true previous char, skipping over all blanks and zero-width chars, and then compares that char to `matches`. */
bool is_previous_char_one_of(linestruct *const line, Ulong xpos,
  const char *const restrict matches, linestruct **const outline, Ulong *const outxpos)
{
  ASSERT(matches);
  ASSERT(*matches);
  char *ch = get_previous_char(line, xpos, outline, outxpos);
  return !(!ch || !mbstrchr(matches, ch));
}

/* ----------------------------- Is next char one of ----------------------------- */

/* Returns `TRUE` when the next char at or after the current `line,xpos` is one of `matches`, this retrieves the
 * true next char, skipping over all blanks and zero-width chars, and then compares that char to `marches`. */
bool is_next_char_one_of(linestruct *const line, Ulong xpos,
  const char *const restrict matches, linestruct **const outline, Ulong *const outxpos)
{
  ASSERT(matches);
  ASSERT(*matches);
  char *ch = get_next_char(line, xpos, outline, outxpos);
  return !(!ch || !mbstrchr(matches, ch));
}

/* ----------------------------- Get previous char match ----------------------------- */

char *get_previous_char_match(linestruct *line, Ulong xpos, const char *const restrict matches,
  bool allow_literals, linestruct **const outline, Ulong *const outxpos)
{
  ASSERT(line);
  char *data;
  int escapes;
  /* Get the previous char, until, we reach the very top of the file. */
  while ((data = get_previous_char(line, xpos, &line, &xpos))) {
    /* We found a literal. */
    if (!allow_literals && mbstrchr("\"'", data) && (!xpos || *(data - 1) != '\\')) {
      /* Now go searching for the next one. */
      while ((data = get_previous_char_match(line, xpos, ((*data == '"') ? "\"" : "'"), TRUE, &line, &xpos))) {
        /* The one we found is escaped. */
        if (xpos && *(data - 1) == '\\') {
          escapes = 0;
          /* Count consecutive escapes. */
          while (xpos && *(data - 1) == '\\') {
            ++escapes;
            --xpos;
            --data;
          }
          /* If we have a uneven number of escapes, then there is something wrong. */
          if (!(escapes % 2)) {
            return NULL;
          }
        }
        /* And if we end up at the very start of the first line, while not allowing literals, we will not find a match. */
        else if (!xpos && !line->prev) {
          return NULL;
        }
        /* Otherwise, we have found our literal match, so break here, and continue searching. */
        else {
          break;
        }
      }
    }
    /* We found a match. */
    else if (mbstrchr(matches, data)) {
      /* Assign where we ended up to outline and outxpos, if the user wants. */
      ASSIGN_IF_VALID(outline, line);
      ASSIGN_IF_VALID(outxpos, xpos);
      return (line->data + xpos);
    }
  }
  return NULL;
}

/* ----------------------------- Tab helper ----------------------------- */

/* Improve the experience of the tabulator. */
bool tab_helper(openfilestruct *const file) {
  ASSERT(file);
  linestruct *line;
  Ulong indent;
  Ulong cur_indent;
  Ulong full_length;
  char *data;
  /* If the previous char is a open bracket char, and we are not on the same line as that bracket. */
  if (is_previous_char_one_of(file->current, file->current_x, "{[(:", &line, NULL) && line != file->current) {
    indent = indent_length(line->data);
    data   = line_indent_plus_tab(line->data, &full_length);
    /* If there is no indent for the given line, or we are already past the full length, just return false. */
    if (file->current_x >= full_length) {
      free(data);
      return FALSE;
    }
    /* The the indent of the current line. */
    cur_indent = indent_length(file->current->data);
    /* If the current line is an empty or blank only line. */
    if (white_string(file->current->data)) {
      /* Add the undo-action. */
      add_undo_for(file, TAB_AUTO_INDENT, NULL);
      file->current_undo->tail_x      = indent;
      file->current_undo->tail_lineno = line->lineno;
      /* Now we remake the line. */
      file->current->data = free_and_assign(file->current->data, data);
      file->current_x     = full_length;
    }
    /* Otherwise, only allow any operation when the cursor is at the indent part of the current line. */
    else if (file->current_x <= cur_indent && cur_indent < full_length) {
      add_undo_for(file, TAB_AUTO_INDENT, NULL);
      file->current_undo->tail_x      = indent;
      file->current_undo->tail_lineno = line->lineno;
      /* Now construct the line. */
      xstr_erase_norealloc(file->current->data, 0, cur_indent);
      file->current->data = free_and_assign(data, xstrninj(file->current->data, data, full_length, 0));
      file->current_x = full_length;
    }
    /* Otherwise, we did nothing, so return FALSE. */
    else {
      free(data);
      return FALSE;
    }
    SET_PWW(file);
    refresh_needed = TRUE;
    return TRUE;
  }
  /* Otherwise, if there is a previous line, and the current line is blank. */
  else if (file->current->prev && white_string(file->current->data)) {
    /* Get the indent lenght of the previous and current line. */
    indent = indent_length(file->current->prev->data);
    /* If the previous line has indent, and the current cursor position
     * is less then that indent, add the indent to the current line. */
    if (indent && file->current_x < indent) {
      /* Create the undo-redo action. */
      add_undo_for(file, TAB_AUTO_INDENT, NULL);
      file->current_undo->tail_x      = indent;
      file->current_undo->tail_lineno = file->current->prev->lineno;
      /* Construct the new line, and position the cursor correctly. */
      file->current->data = xstrncpy(file->current->data, file->current->prev->data, indent);
      file->current_x     = indent;
      SET_PWW(file);
      refresh_needed = TRUE;
      return TRUE;
    }
  }
  return FALSE;
}

/* ----------------------------- Lower case word ----------------------------- */

/* Returns an allocated string containing only lower case chars.
 * TODO: Remake this as what i wanted this to be...  A convert where
 * when all characters are uppercase, then return the lower case word. */
char *lower_case_word(const char *const restrict word) {/*  */
  Ulong len = strlen(word);
  char *ret = xmalloc(len + 1);
  for (Ulong i=0; i<len; ++i) {
    ret[i] = ASCII_TOLOWER(word[i]);
  }
  ret[len] = NUL;
  return ret; 
}

/* ----------------------------- Mark whole file ----------------------------- */

/* Set the `mark` at the very start of `file` and `current` at the very end. */
void mark_whole_file_for(openfilestruct *const file) {
  ASSERT(file);
  file->mark      = file->filetop;
  file->mark_x    = 0;
  file->current   = file->filebot;
  file->current_x = strlen(file->filebot->data);
  file->softmark  = TRUE;
  refresh_needed  = TRUE;
}

void mark_whole_file(void) {
  mark_whole_file_for(CTX_OF);
}
