/** @file text.cpp */
#include "../include/prototypes.h"

/* Toggle the mark. */
void do_mark(void) {
  if (!openfile->mark) {
    openfile->mark     = openfile->current;
    openfile->mark_x   = openfile->current_x;
    openfile->softmark = FALSE;
    statusbar(_("Mark Set"));
  }
  else {
    openfile->mark = NULL;
    statusbar(_("Mark Unset"));
    refresh_needed = TRUE;
  }
}

/* Insert a tab.  Or, if --tabstospaces is in effect, insert the number of spaces that a tab would normally take up at this position. */
void do_tab(void) {
  /* When <Tab> is pressed while a region is marked, indent the region. */
  if (openfile->mark && openfile->mark != openfile->current) {
    do_indent();
  }
  else if (openfile->syntax && openfile->syntax->tabstring) {
    if (suggest_on) {
      accept_suggestion();
      return;
    }
    inject(openfile->syntax->tabstring, strlen(openfile->syntax->tabstring));
  }
  else if (ISSET(TABS_TO_SPACES)) {
    char *spaces = (char *)nmalloc(tabsize + 1);
    Ulong length = (tabsize - (xplustabs() % tabsize));
    memset(spaces, ' ', length);
    spaces[length] = '\0';
    inject(spaces, length);
    free(spaces);
  }
  else {
    if (suggest_on && suggest_str) {
      accept_suggestion();
      return;
    }
    inject((char *)"\t", 1);
  }
}

/* Restore the cursor and mark from a undostruct. */
void restore_undo_posx(undostruct *u) {
  /* Restore the mark if it was set. */
  if (u->xflags & MARK_WAS_SET) {
    if (u->xflags & CURSOR_WAS_AT_HEAD) {
      goto_line_posx(u->head_lineno, u->head_x);
      set_mark(u->tail_lineno, u->tail_x);
    }
    else {
      goto_line_posx(u->tail_lineno, u->tail_x);
      set_mark(u->head_lineno, u->head_x);
    }
    keep_mark = TRUE;
  }
  /* Otherwise just restore the cursor. */
  else {
    goto_line_posx(u->head_lineno, u->head_x);
  }
}

/* Add an indent to the given line. */
void indent_a_line(linestruct *line, char *indentation) {
  Ulong length, indent_len;
  length     = strlen(line->data);
  indent_len = strlen(indentation);
  /* If the requested indentation is empty, don't change the line. */
  if (!indent_len) {
    return;
  }
  /* Add the fabricated indentation to the beginning of the line. */
  line->data = arealloc(line->data, (length + indent_len + 1));
  memmove((line->data + indent_len), line->data, (length + 1));
  memcpy(line->data, indentation, indent_len);
  openfile->totsize += indent_len;
  /* Compensate for the change in the current line. */
  if (line == openfile->mark && openfile->mark_x > 0) {
    openfile->mark_x += indent_len;
  }
  if (line == openfile->current && openfile->current_x > 0) {
    openfile->current_x += indent_len;
    openfile->placewewant = xplustabs();
  }
}

/* Indent the current line (or the marked lines) by tabsize columns.  This inserts either a
 * tab character or a tab's worth of spaces, depending on whether --tabstospaces is in effect. */
void do_indent(void) {
  linestruct *top, *bot, *line;
  char       *indentation;
  /* Use either all the marked lines or just the current line. */
  get_range(&top, &bot);
  /* Skip any leading empty lines. */
  while (top != bot->next && !top->data[0]) {
    top = top->next;
  }
  /* If all lines are empty, there is nothing to do. */
  if (top == bot->next) {
    return;
  }
  indentation = (char *)nmalloc(tabsize + 1);
  if (openfile->syntax && openfile->syntax->tabstring) {
    indentation = mallocstrcpy(indentation, openfile->syntax->tabstring);
  }
  else {
    /* Set the indentation to either a bunch of spaces or a single tab. */
    if (ISSET(TABS_TO_SPACES)) {
      memset(indentation, ' ', tabsize);
      indentation[tabsize] = '\0';
    }
    else {
      indentation[0] = '\t';
      indentation[1] = '\0';
    }
  }
  add_undo(INDENT, NULL);
  /* Go through each of the lines, adding an indent to the non-empty ones,
   * and recording whatever was added in the undo item. */
  for (line = top; line != bot->next; line = line->next) {
    char *real_indent = !line->data[0] ? (char *)"" : indentation;
    indent_a_line(line, real_indent);
    update_multiline_undo(line->lineno, real_indent);
  }
  free(indentation);
  set_modified();
  ensure_firstcolumn_is_aligned();
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Return the number of bytes of whitespace at the start of the given text, but at most a tab's worth. */
Ulong length_of_white(const char *text) {
  Ulong white_count = 0;
  if (openfile->syntax && openfile->syntax->tabstring) {
    Ulong thelength = strlen(openfile->syntax->tabstring);
    while (text[white_count] == openfile->syntax->tabstring[white_count]) {
      if (++white_count == thelength) {
        return thelength;
      }
    }
    white_count = 0;
  }
  while (TRUE) {
    if (*text == '\t') {
      return white_count + 1;
    }
    if (*text != ' ') {
      return white_count;
    }
    if (++white_count == tabsize) {
      return tabsize;
    }
    text++;
  }
}

/* Adjust the positions of mark and cursor when they are on the given line. */
void compensate_leftward(linestruct *line, Ulong leftshift) {
  if (line == openfile->mark) {
    if (openfile->mark_x < leftshift) {
      openfile->mark_x = 0;
    }
    else {
      openfile->mark_x -= leftshift;
    }
  }
  if (line == openfile->current) {
    if (openfile->current_x < leftshift) {
      openfile->current_x = 0;
    }
    else {
      openfile->current_x -= leftshift;
    }
    openfile->placewewant = xplustabs();
  }
}

/* Remove an indent from the given line. */
void unindent_a_line(linestruct *line, Ulong indent_len) {
  Ulong length = strlen(line->data);
  /* If the indent is empty, don't change the line. */
  if (!indent_len) {
    return;
  }
  /* Remove the first tab's worth of whitespace from this line. */
  memmove(line->data, (line->data + indent_len), (length - indent_len + 1));
  openfile->totsize -= indent_len;
  /* Adjust the positions of mark and cursor, when they are affected. */
  compensate_leftward(line, indent_len);
}

/* Unindent the current line (or the marked lines) by tabsize columns.
 * The removed indent can be a mixture of spaces plus at most one tab. */
void do_unindent(void) {
  linestruct *top, *bot, *line;
  /* Use either all the marked lines or just the current line. */
  get_range(&top, &bot);
  /* Skip any leading lines that cannot be unindented. */
  while (top != bot->next && length_of_white(top->data) == 0) {
    top = top->next;
  }
  /* If none of the lines can be unindented, there is nothing to do. */
  if (top == bot->next) {
    return;
  }
  add_undo(UNINDENT, NULL);
  /* Go through each of the lines, removing their leading indent where
   * possible, and saving the removed whitespace in the undo item. */
  for (line = top; line != bot->next; line = line->next) {
    Ulong indent_len  = length_of_white(line->data);
    char *indentation = measured_copy(line->data, indent_len);
    unindent_a_line(line, indent_len);
    update_multiline_undo(line->lineno, indentation);
    free(indentation);
  }
  set_modified();
  ensure_firstcolumn_is_aligned();
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Perform an undo or redo for an indent or unindent action. */
void handle_indent_action(undostruct *u, bool undoing, bool add_indent) {
  groupstruct *group = u->grouping;
  linestruct  *line  = line_from_number(group->top_line);
  /* When redoing, reposition the cursor and let the indenter adjust it. */
  if (!undoing) {
    restore_undo_posx(u);
  }
  /* For each line in the group, add or remove the individual indent. */
  while (line && line->lineno <= group->bottom_line) {
    char *blanks = group->indentations[line->lineno - group->top_line];
    if (undoing ^ add_indent) {
      indent_a_line(line, blanks);
    }
    else {
      unindent_a_line(line, strlen(blanks));
    }
    line = line->next;
  }
  /* When undoing, reposition the cursor to the recorded location. */
  if (undoing) {
    restore_undo_posx(u);
  }
  refresh_needed = TRUE;
}

// Test whether the given line can be uncommented, or add or remove a comment, depending on action.
// Return TRUE if the line is uncommentable, or when anything was added or removed; FALSE otherwise.
// ADDED: Also takes indentation into account.
bool comment_line(undo_type action, linestruct *line, const char *comment_seq) {
  Ulong comment_seq_len = strlen(comment_seq);
  /* The postfix, if this is a bracketing type comment sequence. */
  const char *post_seq = strchr(comment_seq, '|');
  /* Length of prefix. */
  Ulong pre_len = (post_seq ? (post_seq++ - comment_seq) : comment_seq_len);
  /* Length of postfix. */
  Ulong  post_len   = (post_seq ? comment_seq_len - pre_len - 1 : 0);
  Ulong  line_len   = strlen(line->data);
  Ushort indent_len = indent_length(line->data);
  if (!ISSET(NO_NEWLINES) && line == openfile->filebot) {
    return FALSE;
  }
  if (action == COMMENT) {
    /* Make room for the comment sequence(s), move the text right and copy them in. */
    line->data = (char *)nrealloc(line->data, line_len + pre_len + post_len + 1);
    memmove((line->data + pre_len + indent_len), (line->data + indent_len), (line_len - indent_len + 1));
    memmove((line->data + indent_len), comment_seq, pre_len);
    inject_in_line(line, " ", (indent_len + comment_seq_len));
    if (post_len > 0) {
      memmove(line->data + pre_len + line_len, post_seq, post_len + 1);
    }
    openfile->totsize += (pre_len + post_len);
    /* If needed, adjust the position of the mark and of the cursor. */
    if (line == openfile->mark && openfile->mark_x > 0) {
      openfile->mark_x += (pre_len + 1);
    }
    if (line == openfile->current && openfile->current_x > 0) {
      openfile->current_x += (pre_len + 1);
      openfile->placewewant = xplustabs();
    }
    return TRUE;
  }
  /* If the line is commented, report it as uncommentable, or uncomment it. */
  if (strncmp((line->data + indent_len), comment_seq, pre_len) == 0
   && (!post_len || strcmp((line->data + line_len - post_len), post_seq) == 0)) {
    if (action == PREFLIGHT) {
      return TRUE;
    }
    /* Erase the comment prefix by moving the non-comment part. */
    memmove((line->data + indent_len), (line->data + indent_len + pre_len + 1), (line_len - pre_len - indent_len));
    /* Truncate the postfix if there was one. */
    line->data[line_len - pre_len - post_len] = '\0';
    openfile->totsize -= (pre_len + post_len);
    /* Adjust the positions of mark and cursor, when needed. */
    compensate_leftward(line, (pre_len + 1));
    return TRUE;
  }
  return FALSE;
}

/* Comment or uncomment the current line or the marked lines. */
void do_comment(void) {
  const char *comment_seq = GENERAL_COMMENT_CHARACTER;
  undo_type action = UNCOMMENT;
  linestruct *top, *bot, *line;
  bool empty, all_empty = TRUE;
  if (openfile->syntax) {
    comment_seq = openfile->syntax->comment;
  }
  else if (openfile->type.is_set<C_CPP>()) {
    comment_seq = "//";
  }
  else if (openfile->type.is_set<ASM>()) {
    comment_seq = ";";
  }
  /* This is when 'openfile->syntax' says comments are foridden. */
  if (!*comment_seq) {
    statusline(AHEM, _("Commenting is not supported for this file type"));
    return;
  }
  /* Determine which lines to work on. */
  get_range(&top, &bot);
  /* If only the magic line is selected, don't do anything. */
  if (top == bot && bot == openfile->filebot && !ISSET(NO_NEWLINES)) {
    statusline(AHEM, _("Cannot comment past end of file"));
    return;
  }
  /* Figure out whether to comment or uncomment the selected line or lines. */
  for (line = top; line != bot->next; line = line->next) {
    empty = white_string(line->data);
    /* If this line is not blank and not commented, we comment all. */
    if (!empty && !comment_line(PREFLIGHT, line, comment_seq)) {
      action = COMMENT;
      break;
    }
    all_empty = (all_empty && empty);
  }
  /* If all selected lines are blank, we comment them. */
  action = all_empty ? COMMENT : action;
  add_undo(action, NULL);
  /* Store the comment sequence used for the operation, because it could change when the file name changes; we need to know what it was. */
  openfile->current_undo->strdata = copy_of(comment_seq);
  /* Comment/uncomment each of the selected lines when possible, and store undo data when a line changed. */
  for (line = top; line != bot->next; line = line->next) {
    if (comment_line(action, line, comment_seq)) {
      update_multiline_undo(line->lineno, _(""));
    }
  }
  set_modified();
  ensure_firstcolumn_is_aligned();
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Perform an undo or redo for a comment or uncomment action. */
void handle_comment_action(undostruct *u, bool undoing, bool add_comment) {
  groupstruct *group = u->grouping;
  /* When redoing, reposition the cursor and let the commenter adjust it. */
  if (!undoing) {
    restore_undo_posx(u);
  }
  while (group) {
    linestruct *line = line_from_number(group->top_line);
    while (line && line->lineno <= group->bottom_line) {
      comment_line(((undoing ^ add_comment) ? COMMENT : UNCOMMENT), line, u->strdata);
      line = line->next;
    }
    group = group->next;
  }
  /* When undoing, reposition the cursor to the recorded location. */
  if (undoing) {
    restore_undo_posx(u);
  }
  refresh_needed = TRUE;
}

#define redo_paste undo_cut
#define undo_paste redo_cut

/* Undo a cut, or redo a paste. */
void undo_cut(undostruct *u) {
  goto_line_posx(u->head_lineno, ((u->xflags & WAS_WHOLE_LINE) ? 0 : u->head_x));
  /* Clear an inherited anchor but not a user-placed one. */
  if (!(u->xflags & HAD_ANCHOR_AT_START)) {
    openfile->current->has_anchor = FALSE;
  }
  if (u->cutbuffer) {
    copy_from_buffer(u->cutbuffer);
  }
  /* If originally the last line was cut too, remove an extra magic line. */
  if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)
   && openfile->filebot != openfile->current && !openfile->filebot->prev->data[0]) {
    remove_magicline();
  }
  if (u->xflags & CURSOR_WAS_AT_HEAD) {
    goto_line_posx(u->head_lineno, u->head_x);
  }
}

/* Redo a cut, or undo a paste. */
void redo_cut(undostruct *u) {
  linestruct *oldcutbuffer = cutbuffer;
  cutbuffer = NULL;
  openfile->mark   = line_from_number(u->head_lineno);
  openfile->mark_x = ((u->xflags & WAS_WHOLE_LINE) ? 0 : u->head_x);
  goto_line_posx(u->tail_lineno, u->tail_x);
  do_snip(TRUE, FALSE, (u->type == ZAP));
  free_lines(cutbuffer);
  cutbuffer = oldcutbuffer;
}

/* Return`s a malloc`ed str encoded with enclose delimiter. */
char *encode_enclose_str(const char *s1, const char *s2) {
  char *part = (char *)nmalloc(strlen(s1) + strlen(ENCLOSE_DELIM) + strlen(s2) + 1);
  sprintf(part, "%s" ENCLOSE_DELIM "%s", s1, s2);
  return part;
}

/* Decode s1 and s2 from a encoded enclose str. */
void decode_enclose_str(const char *str, char **s1, char **s2) {
  const char *enclose_delim = strstr(str, ENCLOSE_DELIM);
  if (!enclose_delim) {
    die("%s: str ('%s') was not encoded properly.  Could not find ENCLOSE_DELIM '%s'.\n", __func__, str, ENCLOSE_DELIM);
  }
  *s1 = measured_copy(str, (enclose_delim - str));
  *s2 = copy_of(enclose_delim + strlen(ENCLOSE_DELIM));
}

/* If an area is marked then plase the 'str' at mark and current_x, thereby enclosing the marked area. */
void enclose_marked_region(const char *s1, const char *s2) {
  /* Return early if there is no mark. */
  if (!openfile->mark) {
    return;
  }
  char *part = encode_enclose_str(s1, s2);
  add_undo(ENCLOSE, part);
  free(part);
  const Ulong s1_len = strlen(s1);
  if (mark_is_before_cursor()) {
    inject_in_line(openfile->mark, s1, openfile->mark_x);
    openfile->mark_x += s1_len;
    if (openfile->mark == openfile->current) {
      openfile->current_x += s1_len;
    }
    inject_in_line(openfile->current, s2, openfile->current_x);
  }
  else {
    inject_in_line(openfile->current, s1, openfile->current_x);
    openfile->current_x += s1_len;
    if (openfile->current == openfile->mark) {
      openfile->mark_x += s1_len;
    }
    inject_in_line(openfile->mark, s2, openfile->mark_x);
  }
  set_modified();
  refresh_needed = TRUE;
}

/* This is a shortcut to make marked area a block comment. */
void do_block_comment(void) {
  enclose_marked_region("/* ", " */");
  keep_mark = TRUE;
}

/* Undo the last thing(s) we did. */
void do_undo(void) {
  undostruct *u = openfile->current_undo;
  linestruct *oldcutbuffer, *intruder;
  linestruct *line = NULL;
  Ulong original_x, regain_from_x;
  char  *undidmsg = NULL;
  char  *data;
  if (!u) {
    statusline(AHEM, _("Nothing to undo"));
    return;
  }
  if (u->type <= REPLACE) {
    line = line_from_number(u->tail_lineno);
  }
  switch (u->type) {
    case ADD : {
      /* TRANSLATORS: The next thirteen strings describe actions that are undone or redone.  They are all nouns, not verbs. */
      undidmsg = _("addition");
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
        remove_magicline();
      }
      memmove((line->data + u->head_x), (line->data + u->head_x + strlen(u->strdata)), (strlen(line->data + u->head_x) - strlen(u->strdata) + 1));
      goto_line_posx(u->head_lineno, u->head_x);
      break;
    }
    case ENTER : {
      undidmsg = _("line break");
      /* An <Enter> at the end of leading whitespace while autoindenting has deleted the whitespace, and
       * stored an x position of zero. In that case, adjust the positions to return to and to scoop data from. */
      original_x    = ((u->head_x == 0) ? u->tail_x : u->head_x);
      regain_from_x = ((u->head_x == 0) ? 0 : u->tail_x);
      line->data    = arealloc(line->data, (strlen(line->data) + strlen(&u->strdata[regain_from_x]) + 1));
      strcat(line->data, &u->strdata[regain_from_x]);
      line->has_anchor |= line->next->has_anchor;
      unlink_node(line->next);
      renumber_from(line);
      openfile->current = line;
      goto_line_posx(u->head_lineno, original_x);
      break;
    }
    case BACK :
    case DEL : {
      undidmsg = (char *)_("deletion");
      data     = (char *)nmalloc(strlen(line->data) + strlen(u->strdata) + 1);
      strncpy(data, line->data, u->head_x);
      strcpy(&data[u->head_x], u->strdata);
      strcpy(&data[u->head_x + strlen(u->strdata)], &line->data[u->head_x]);
      free(line->data);
      line->data = data;
      goto_line_posx(u->tail_lineno, u->tail_x);
      break;
    }
    case JOIN : {
      undidmsg = _("line join");
      /**
        When the join was done by a Backspace at the tail of the file,
        and the nonewlines flag isn't set, do not re-add a newline that
        wasn't actually deleted; just position the cursor.
       */
      if ((u->xflags & WAS_BACKSPACE_AT_EOF) && !ISSET(NO_NEWLINES)) {
        goto_line_posx(openfile->filebot->lineno, 0);
        focusing = FALSE;
        break;
      }
      line->data[u->tail_x] = '\0';
      intruder              = make_new_node(line);
      intruder->data        = copy_of(u->strdata);
      splice_node(line, intruder);
      renumber_from(intruder);
      goto_line_posx(u->head_lineno, u->head_x);
      break;
    }
    case REPLACE : {
      undidmsg = _("replacement");
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
        remove_magicline();
      }
      data       = u->strdata;
      u->strdata = line->data;
      line->data = data;
      goto_line_posx(u->head_lineno, u->head_x);
      break;
    }
    case SPLIT_BEGIN : {
      undidmsg = _("addition");
      break;
    }
    case SPLIT_END : {
      openfile->current_undo = openfile->current_undo->next;
      while (openfile->current_undo->type != SPLIT_BEGIN) {
        do_undo();
      }
      u = openfile->current_undo;
      break;
    }
    case ZAP : {
      undidmsg = _("erasure");
      undo_cut(u);
      break;
    }
    case CUT_TO_EOF :
    case CUT : {
      /* TRANSLATORS: Remember: these are nouns, NOT verbs. */
      undidmsg = _("cut");
      undo_cut(u);
      break;
    }
    case PASTE : {
      undidmsg = _("paste");
      undo_paste(u);
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES) && openfile->filebot != openfile->current) {
        remove_magicline();
      }
      break;
    }
    case INSERT : {
      undidmsg     = _("insertion");
      oldcutbuffer = cutbuffer;
      cutbuffer    = NULL;
      goto_line_posx(u->head_lineno, u->head_x);
      openfile->mark   = line_from_number(u->tail_lineno);
      openfile->mark_x = u->tail_x;
      cut_marked_region();
      u->cutbuffer = cutbuffer;
      cutbuffer    = oldcutbuffer;
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES) && openfile->filebot != openfile->current) {
        remove_magicline();
      }
      break;
    }
    case COUPLE_BEGIN : {
      undidmsg = u->strdata;
      goto_line_posx(u->head_lineno, u->head_x);
      openfile->cursor_row = u->tail_lineno;
      adjust_viewport(STATIONARY);
      break;
    }
    case COUPLE_END : {
      /* Remember the row of the cursor for a possible redo. */
      openfile->current_undo->head_lineno = openfile->cursor_row;
      openfile->current_undo              = openfile->current_undo->next;
      do_undo();
      do_undo();
      do_undo();
      return;
    }
    case INDENT : {
      handle_indent_action(u, TRUE, TRUE);
      undidmsg = _("indent");
      break;
    }
    case UNINDENT : {
      handle_indent_action(u, TRUE, FALSE);
      undidmsg = _("unindent");
      break;
    }
    case COMMENT : {
      handle_comment_action(u, TRUE, TRUE);
      undidmsg = _("comment");
      break;
    }
    case UNCOMMENT : {
      handle_comment_action(u, TRUE, FALSE);
      undidmsg = _("uncomment");
      break;
    }
    case MOVE_LINE_UP : {
      /* Single line move. */
      if (u->head_lineno == u->tail_lineno) {
        openfile->current = line_from_number(u->head_lineno - 1);
        move_line(openfile->current, FALSE);
        openfile->current = openfile->current->next;
        /* Restore the mark if it was set. */
        if (u->xflags & MARK_WAS_SET) {
          openfile->mark = openfile->current;
          if (u->xflags & CURSOR_WAS_AT_HEAD) {
            openfile->current_x = u->head_x;
            openfile->mark_x    = u->tail_x;
          }
          else {
            openfile->current_x = u->tail_x;
            openfile->mark_x    = u->head_x;
          }
          keep_mark = TRUE;
        }
        /* Otherwise just restore the cursor pos. */
        else {
          openfile->current_x = u->head_x;
        }
      }
      /* Multi-line move. */
      else {
        linestruct *top = line_from_number(u->head_lineno);
        linestruct *bot = line_from_number(u->tail_lineno);
        for (linestruct *line = bot; line->lineno != (u->head_lineno - 1); line = line->prev) {
          move_line(line, TRUE);
        }
        /* Restore mark. */
        if (u->xflags & CURSOR_WAS_AT_HEAD) {
          openfile->current   = top;
          openfile->current_x = u->head_x;
          openfile->mark      = bot;
          openfile->mark_x    = u->tail_x;
        }
        else {
          openfile->current   = bot;
          openfile->current_x = u->tail_x;
          openfile->mark      = top;
          openfile->mark_x    = u->head_x;
        }
        keep_mark = TRUE;
      }
      break;
    }
    case MOVE_LINE_DOWN : {
      /* Single line move. */
      if (u->head_lineno == u->tail_lineno) {
        openfile->current = line_from_number(u->head_lineno + 1);
        move_line(openfile->current, TRUE);
        openfile->current = openfile->current->prev;
        /* Restore the mark if it was set. */
        if (u->xflags & MARK_WAS_SET) {
          openfile->mark = openfile->current;
          if (u->xflags & CURSOR_WAS_AT_HEAD) {
            openfile->current_x = u->head_x;
            openfile->mark_x    = u->tail_x;
          }
          else {
            openfile->current_x = u->tail_x;
            openfile->mark_x    = u->head_x;
          }
          keep_mark = TRUE;
        }
        /* Otherwise just restore the cursor pos. */
        else {
          openfile->current_x = u->head_x;
        }
      }
      /* Multi-line move. */
      else {
        linestruct *top = line_from_number(u->head_lineno);
        linestruct *bot = line_from_number(u->tail_lineno);
        for (linestruct *line = top; line->lineno != (u->tail_lineno + 1); line = line->next) {
          move_line(line, FALSE);
        }
        /* Restore mark. */
        if (u->xflags & CURSOR_WAS_AT_HEAD) {
          openfile->current   = top;
          openfile->current_x = u->head_x;
          openfile->mark      = bot;
          openfile->mark_x    = u->tail_x;
        }
        else {
          openfile->current   = bot;
          openfile->current_x = u->tail_x;
          openfile->mark      = top;
          openfile->mark_x    = u->head_x;
        }
        keep_mark = TRUE;
      }
      break;
    }
    case ENCLOSE : {
      char *s1, *s2;
      decode_enclose_str(u->strdata, &s1, &s2);
      linestruct *head = line_from_number(u->head_lineno);
      linestruct *tail = line_from_number(u->tail_lineno);
      erase_in_line(head, u->head_x, strlen(s1));
      erase_in_line(tail, u->tail_x, strlen(s2));
      free(s1);
      free(s2);
      if (u->xflags & CURSOR_WAS_AT_HEAD) {
        goto_line_posx(u->head_lineno, u->head_x);
        openfile->mark   = tail;
        openfile->mark_x = u->tail_x;
      }
      else {
        goto_line_posx(u->tail_lineno, u->tail_x);
        openfile->mark   = head;
        openfile->mark_x = u->head_x;
      }
      refresh_needed = TRUE;
      keep_mark      = TRUE;
      break;
    }
    default : {
      break;
    }
  }
  if (undidmsg && !ISSET(ZERO) && !pletion_line) {
    statusline(HUSH, _("Undid %s"), undidmsg);
  }
  openfile->current_undo = openfile->current_undo->next;
  openfile->last_action  = OTHER;
  if (!keep_mark) {
    openfile->mark = NULL;
  }
  openfile->placewewant  = xplustabs();
  openfile->totsize      = u->wassize;
  if (u->type <= REPLACE) {
    check_the_multis(openfile->current);
  }
  else if (u->type == INSERT || u->type == COUPLE_BEGIN) {
    recook = TRUE;
  }
  /* When at the point where the buffer was last saved, unset "Modified". */
  if (openfile->current_undo == openfile->last_saved) {
    openfile->modified = FALSE;
    titlebar(NULL);
  }
  else {
    set_modified();
  }
}

/* Redo the last thing(s) we undid. */
void do_redo(void) {
  undostruct *u    = openfile->undotop;
  linestruct *line = NULL;
  linestruct *intruder;
  bool  suppress_modification = FALSE;
  char *redidmsg = NULL;
  char *data;
  if (!u || u == openfile->current_undo) {
    statusline(AHEM, _("Nothing to redo"));
    return;
  }
  /* Find the item before the current one in the undo stack. */
  while (u->next != openfile->current_undo) {
    u = u->next;
  }
  if (u->type <= REPLACE) {
    line = line_from_number(u->tail_lineno);
  }
  switch (u->type) {
    case ADD: {
      redidmsg = _("addition");
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
        new_magicline();
      }
      data = (char *)nmalloc(strlen(line->data) + strlen(u->strdata) + 1);
      strncpy(data, line->data, u->head_x);
      strcpy(&data[u->head_x], u->strdata);
      strcpy(&data[u->head_x + strlen(u->strdata)], &line->data[u->head_x]);
      free(line->data);
      line->data = data;
      goto_line_posx(u->tail_lineno, u->tail_x);
      break;
    }
    case ENTER: {
      redidmsg              = _("line break");
      line->data[u->head_x] = '\0';
      intruder              = make_new_node(line);
      intruder->data        = copy_of(u->strdata);
      splice_node(line, intruder);
      renumber_from(intruder);
      goto_line_posx(u->head_lineno + 1, u->tail_x);
      break;
    }
    case BACK:
    case DEL: {
      redidmsg = _("deletion");
      memmove(line->data + u->head_x, line->data + u->head_x + strlen(u->strdata),
        (strlen(line->data + u->head_x) - strlen(u->strdata) + 1));
      goto_line_posx(u->head_lineno, u->head_x);
      break;
    }
    case JOIN: {
      redidmsg = _("line join");
      /**
        When the join was done by a Backspace at the tail of the file,
        and the nonewlines flag isn't set, do not join anything, as
        nothing was actually deleted; just position the cursor.
       */
      if ((u->xflags & WAS_BACKSPACE_AT_EOF) && !ISSET(NO_NEWLINES)) {
        goto_line_posx(u->tail_lineno, u->tail_x);
        break;
      }
      line->data = arealloc(line->data, (strlen(line->data) + strlen(u->strdata) + 1));
      strcat(line->data, u->strdata);
      unlink_node(line->next);
      renumber_from(line);
      openfile->current = line;
      goto_line_posx(u->tail_lineno, u->tail_x);
      break;
    }
    case REPLACE: {
      redidmsg = _("replacement");
      if ((u->xflags & INCLUDED_LAST_LINE) && !ISSET(NO_NEWLINES)) {
        new_magicline();
      }
      data       = u->strdata;
      u->strdata = line->data;
      line->data = data;
      goto_line_posx(u->head_lineno, u->head_x);
      break;
    }
    case SPLIT_BEGIN: {
      openfile->current_undo = u;
      while (openfile->current_undo->type != SPLIT_END) {
        do_redo();
      }
      u = openfile->current_undo;
      goto_line_posx(u->head_lineno, u->head_x);
      ensure_firstcolumn_is_aligned();
      break;
    }
    case SPLIT_END: {
      redidmsg = _("addition");
      break;
    }
    case ZAP: {
      redidmsg = _("erasure");
      redo_cut(u);
      break;
    }
    case CUT_TO_EOF:
    case CUT: {
      redidmsg = _("cut");
      redo_cut(u);
      break;
    }
    case PASTE: {
      redidmsg = _("paste");
      redo_paste(u);
      break;
    }
    case INSERT: {
      redidmsg = _("insertion");
      goto_line_posx(u->head_lineno, u->head_x);
      if (u->cutbuffer) {
        copy_from_buffer(u->cutbuffer);
      }
      else {
        suppress_modification = TRUE;
      }
      free_lines(u->cutbuffer);
      u->cutbuffer = NULL;
      break;
    }
    case COUPLE_BEGIN: {
      openfile->current_undo = u;
      do_redo();
      do_redo();
      do_redo();
      return;
    }
    case COUPLE_END: {
      redidmsg = u->strdata;
      goto_line_posx(u->tail_lineno, u->tail_x);
      openfile->cursor_row = u->head_lineno;
      adjust_viewport(STATIONARY);
      break;
    }
    case INDENT: {
      handle_indent_action(u, FALSE, TRUE);
      redidmsg = _("indent");
      break;
    }
    case UNINDENT: {
      handle_indent_action(u, FALSE, FALSE);
      redidmsg = _("unindent");
      break;
    }
    case COMMENT: {
      handle_comment_action(u, FALSE, TRUE);
      redidmsg = _("comment");
      break;
    }
    case UNCOMMENT: {
      handle_comment_action(u, FALSE, FALSE);
      redidmsg = _("uncomment");
      break;
    }
    case MOVE_LINE_UP: {
      /* Single line move. */
      if (u->head_lineno == u->tail_lineno) {
        openfile->current = line_from_number(u->head_lineno);
        move_line(openfile->current, TRUE);
        openfile->current = openfile->current->prev;
        /* Restore the mark if it was set. */
        if (u->xflags & MARK_WAS_SET) {
          openfile->mark = openfile->current;
          keep_mark = TRUE;
          if (u->xflags & CURSOR_WAS_AT_HEAD) {
            openfile->current_x = u->head_x;
            openfile->mark_x    = u->tail_x;
          }
          else {
            openfile->current_x = u->tail_x;
            openfile->mark_x    = u->head_x;
          }
        }
        /* Otherwise just restore the cursor pos. */
        else {
          openfile->current_x = u->head_x;
        }
      }
      /* Multi-line move. */
      else {
        linestruct *top = line_from_number(u->head_lineno - 1);
        linestruct *bot = line_from_number(u->tail_lineno - 1);
        for (linestruct *line = top; line->lineno != u->tail_lineno; line = line->next) {
          move_line(line, FALSE);
        }
        /* Restore mark. */
        if (u->xflags & CURSOR_WAS_AT_HEAD) {
          openfile->current   = top;
          openfile->current_x = u->head_x;
          openfile->mark      = bot;
          openfile->mark_x    = u->tail_x;
        }
        else {
          openfile->current   = bot;
          openfile->current_x = u->tail_x;
          openfile->mark      = top;
          openfile->mark_x    = u->head_x;
        }
        keep_mark = TRUE;
      }
      break;
    }
    case MOVE_LINE_DOWN: {
      /* Single line move. */
      if (u->head_lineno == u->tail_lineno) {
        openfile->current = line_from_number(u->head_lineno);
        move_line(openfile->current, FALSE);
        openfile->current = openfile->current->next;
        /* Restore the mark if it was set. */
        if (u->xflags & MARK_WAS_SET) {
          openfile->mark = openfile->current;
          keep_mark = TRUE;
          if (u->xflags & CURSOR_WAS_AT_HEAD) {
            openfile->current_x = u->head_x;
            openfile->mark_x    = u->tail_x;
          }
          else {
            openfile->current_x = u->tail_x;
            openfile->mark_x    = u->head_x;
          }
        }
        /* Otherwise just restore the cursor pos. */
        else {
          openfile->current_x = u->head_x;
        }
      }
      /* Multi-line move. */
      else {
        linestruct *top = line_from_number(u->head_lineno + 1);
        linestruct *bot = line_from_number(u->tail_lineno + 1);
        for (linestruct *line = bot; line->lineno != u->head_lineno; line = line->prev) {
          move_line(line, TRUE);
        }
        /* Restore mark. */
        if (u->xflags & CURSOR_WAS_AT_HEAD) {
          openfile->current   = top;
          openfile->current_x = u->head_x;
          openfile->mark      = bot;
          openfile->mark_x    = u->tail_x;
        }
        else {
          openfile->current   = bot;
          openfile->current_x = u->tail_x;
          openfile->mark      = top;
          openfile->mark_x    = u->head_x;
        }
        keep_mark = TRUE;
      }
      break;
    }
    case ENCLOSE: {
      char *s1, *s2;
      decode_enclose_str(u->strdata, &s1, &s2);
      const Ulong s1_len = strlen(s1);
      linestruct *head = line_from_number(u->head_lineno);
      linestruct *tail = line_from_number(u->tail_lineno);
      const Ulong head_x = (u->head_x + s1_len);
      const Ulong tail_x = (head == tail) ? (u->tail_x + s1_len) : u->tail_x;
      inject_in_line(head, s1, u->head_x);
      inject_in_line(tail, s2, tail_x);
      free(s1);
      free(s2);
      if (u->xflags & CURSOR_WAS_AT_HEAD) {
        goto_line_posx(u->head_lineno, head_x);
        openfile->mark   = tail;
        openfile->mark_x = tail_x;
      }
      else {
        goto_line_posx(u->tail_lineno, tail_x);
        openfile->mark   = head;
        openfile->mark_x = head_x;
      }
      refresh_needed = TRUE;
      keep_mark      = TRUE;
      break;
    }
    default : {
      break;
    }
  }
  if (redidmsg && !ISSET(ZERO)) {
    statusline(HUSH, _("Redid %s"), redidmsg);
  }
  openfile->current_undo = u;
  openfile->last_action  = OTHER;
  if (!keep_mark) {
    openfile->mark = NULL;
  }
  openfile->placewewant  = xplustabs();
  openfile->totsize      = u->newsize;
  if (u->type <= REPLACE) {
    check_the_multis(openfile->current);
  }
  else if (u->type == INSERT || u->type == COUPLE_END) {
    recook = TRUE;
  }
  /* When at the point where the buffer was last saved, unset "Modified". */
  if (openfile->current_undo == openfile->last_saved) {
    openfile->modified = FALSE;
    titlebar(NULL);
  }
  else if (!suppress_modification) {
    set_modified();
  }
}

/* Break the current line at the cursor position. */
void do_enter(void) {
  if (suggest_on && suggest_str) {
    accept_suggestion();
    return;
  }
  /* Check if cursor is between two brackets. */
  if (enter_with_bracket()) {
    return;
  }
  char c_prev = '\0';
  if (openfile->current->data[openfile->current_x - 1]) {
    c_prev = openfile->current->data[openfile->current_x - 1];
  }
  linestruct *newnode    = make_new_node(openfile->current);
  linestruct *sampleline = openfile->current;
  Ulong       extra      = 0;
  bool        allblanks  = FALSE;
  if (ISSET(AUTOINDENT)) {
    /* When doing automatic long-line wrapping and the next line is in this same paragraph, use its indentation as the model. */
    if (ISSET(BREAK_LONG_LINES) && sampleline->next && inpar(sampleline->next) && !begpar(sampleline->next, 0)) {
      sampleline = sampleline->next;
    }
    extra = indent_length(sampleline->data);
    /* When breaking in the indentation, limit the automatic one. */
    if (extra > openfile->current_x) {
      extra = openfile->current_x;
    }
    else if (extra == openfile->current_x) {
      allblanks = (indent_length(openfile->current->data) == extra);
    }
  }
  newnode->data = (char *)nmalloc(strlen(openfile->current->data + openfile->current_x) + extra + 1);
  strcpy(&newnode->data[extra], (openfile->current->data + openfile->current_x));
  /* Adjust the mark if it is on the current line after the cursor. */
  if (openfile->mark == openfile->current && openfile->mark_x > openfile->current_x) {
    openfile->mark = newnode;
    openfile->mark_x += extra - openfile->current_x;
  }
  if (ISSET(AUTOINDENT)) {
    /* Copy the whitespace from the sample line to the new one. */
    strncpy(newnode->data, sampleline->data, extra);
    /* If there were only blanks before the cursor, trim them. */
    if (allblanks) {
      openfile->current_x = 0;
    }
  }
  /* Make the current line end at the cursor position. */
  openfile->current->data[openfile->current_x] = '\0';
  add_undo(ENTER, NULL);
  /* Insert the newly created line after the current one and renumber. */
  splice_node(openfile->current, newnode);
  renumber_from(newnode);
  /* Put the cursor on the new line, after any automatic whitespace. */
  openfile->current     = newnode;
  openfile->current_x   = extra;
  openfile->placewewant = xplustabs();
  ++openfile->totsize;
  set_modified();
  if (ISSET(AUTOINDENT) && !allblanks) {
    openfile->totsize += extra;
  }
  update_undo(ENTER);
  if (c_prev == '{' || c_prev == ':') {
    do_tab();
  }
  refresh_needed = TRUE;
  focusing       = FALSE;
}

/* Discard undo items that are newer than the given one, or all if NULL. */
void discard_until(const undostruct *thisitem) {
  undostruct  *dropit = openfile->undotop;
  groupstruct *group;
  while (dropit && dropit != thisitem) {
    openfile->undotop = dropit->next;
    free(dropit->strdata);
    free_lines(dropit->cutbuffer);
    group = dropit->grouping;
    while (group) {
      groupstruct *next = group->next;
      free_chararray(group->indentations, (group->bottom_line - group->top_line + 1));
      free(group);
      group = next;
    }
    free(dropit);
    dropit = openfile->undotop;
  }
  /* Adjust the pointer to the top of the undo stack. */
  openfile->current_undo = (undostruct *)thisitem;
  /* Prevent a chain of editing actions from continuing. */
  openfile->last_action = OTHER;
}

/* Add a new undo item of the given type to the top of the current pile. */
void add_undo(undo_type action, const char *message) {
  undostruct *u = (undostruct *)nmalloc(sizeof(undostruct));
  linestruct *thisline = openfile->current;
  /* Initialize the newly allocated undo item. */
  u->type        = action;
  u->strdata     = NULL;
  u->cutbuffer   = NULL;
  u->head_lineno = thisline->lineno;
  u->head_x      = openfile->current_x;
  u->tail_lineno = thisline->lineno;
  u->tail_x      = openfile->current_x;
  u->wassize     = openfile->totsize;
  u->newsize     = openfile->totsize;
  u->grouping    = NULL;
  u->xflags      = 0;
  /* Blow away any undone items. */
  discard_until(openfile->current_undo);
  /* If some action caused automatic long-line wrapping, insert the SPLIT_BEGIN item underneath
   * that action's undo item.  Otherwise, just add the new item to the top of the undo stack. */
  if (u->type == SPLIT_BEGIN) {
    action     = openfile->undotop->type;
    u->wassize = openfile->undotop->wassize;
    u->next    = openfile->undotop->next;
    openfile->undotop->next = u;
  }
  else {
    u->next                = openfile->undotop;
    openfile->undotop      = u;
    openfile->current_undo = u;
  }
  /* Record the info needed to be able to undo each possible action. */
  switch (u->type) {
    case ADD : {
      /* If a new magic line will be added, an undo should remove it. */
      if (thisline == openfile->filebot) {
        u->xflags |= INCLUDED_LAST_LINE;
      }
      break;
    }
    case ENTER : {
      break;
    }
    case BACK : {
      /* If the next line is the magic line, don't ever undo this
       * backspace, as it won't actually have deleted anything. */
      if (thisline->next == openfile->filebot && thisline->data[0]) {
        u->xflags |= WAS_BACKSPACE_AT_EOF;
      }
      /* Fall-through. */
    }
    case DEL : {
      /* When not at the end of a line, store the deleted character;
       * otherwise, morph the undo item into a line join. */
      if (thisline->data[openfile->current_x]) {
        int charlen = char_length(thisline->data + u->head_x);
        u->strdata  = measured_copy(thisline->data + u->head_x, charlen);
        if (u->type == BACK) {
          u->tail_x += charlen;
        }
        break;
      }
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
    case REPLACE : {
      u->strdata = copy_of(thisline->data);
      if (thisline == openfile->filebot && answer[0]) {
        u->xflags |= INCLUDED_LAST_LINE;
      }
      break;
    }
    case SPLIT_BEGIN :
    case SPLIT_END : {
      break;
    }
    case CUT_TO_EOF : {
      u->xflags |= (INCLUDED_LAST_LINE | CURSOR_WAS_AT_HEAD);
      if (openfile->current->has_anchor) {
        u->xflags |= HAD_ANCHOR_AT_START;
      }
      break;
    }
    case ZAP :
    case CUT : {
      if (openfile->mark) {
        if (mark_is_before_cursor()) {
          u->head_lineno = openfile->mark->lineno;
          u->head_x      = openfile->mark_x;
          u->xflags |= MARK_WAS_SET;
        }
        else {
          u->tail_lineno = openfile->mark->lineno;
          u->tail_x      = openfile->mark_x;
          u->xflags |= (MARK_WAS_SET | CURSOR_WAS_AT_HEAD);
        }
        if (u->tail_lineno == openfile->filebot->lineno) {
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
      if ((openfile->mark && mark_is_before_cursor() && openfile->mark->has_anchor)
       || ((!openfile->mark || !mark_is_before_cursor()) && openfile->current->has_anchor)) {
        u->xflags |= HAD_ANCHOR_AT_START;
      }
      break;
    }
    case PASTE : {
      u->cutbuffer = copy_buffer(cutbuffer);
      /* Fall-through. */
    }
    case INSERT : {
      if (thisline == openfile->filebot) {
        u->xflags |= INCLUDED_LAST_LINE;
      }
      break;
    }
    case COUPLE_BEGIN : {
      u->tail_lineno = openfile->cursor_row;
      /* Fall-through. */
    }
    case COUPLE_END : {
      u->strdata = copy_of(_(message));
      break;
    }
    case INDENT :
    case UNINDENT :
    case COMMENT :
    case UNCOMMENT :
    case MOVE_LINE_UP :
    case MOVE_LINE_DOWN : {
      if (openfile->mark) {
        if (mark_is_before_cursor()) {
          u->head_lineno = openfile->mark->lineno;
          u->head_x      = openfile->mark_x;
          u->xflags |= MARK_WAS_SET;
        }
        else {
          u->tail_lineno = openfile->mark->lineno;
          u->tail_x      = openfile->mark_x;
          u->xflags |= (MARK_WAS_SET | CURSOR_WAS_AT_HEAD);
        }
      }
      break;
    }
    case ENCLOSE : {
      if (mark_is_before_cursor()) {
        u->head_lineno = openfile->mark->lineno;
        u->head_x      = openfile->mark_x;
        u->xflags |= MARK_WAS_SET;
      }
      else {
        u->tail_lineno = openfile->mark->lineno;
        u->tail_x      = openfile->mark_x;
        u->xflags |= (MARK_WAS_SET | CURSOR_WAS_AT_HEAD);
      }
      u->strdata = copy_of(message);
      break;
    }
    default : {
      die("Bad undo type -- please report a bug\n");
    }
  }
  openfile->last_action = action;
}

// Update a multiline undo item.  This should be called once for each line, affected by a multiple-line-altering
// feature.  The indentation that is added or removed is saved, separately for each line in the undo item. */
void update_multiline_undo(long lineno, char *indentation) {
  undostruct *u = openfile->current_undo;
  /* If there already is a group and the current line is contiguous with it, extend the group; otherwise, create a new group. */
  if (u->grouping && (u->grouping->bottom_line + 1) == lineno) {
    Ulong number_of_lines     = (lineno - u->grouping->top_line + 1);
    u->grouping->bottom_line  = lineno;
    u->grouping->indentations = arealloc(u->grouping->indentations, (number_of_lines * sizeof(char *)));
    u->grouping->indentations[number_of_lines - 1] = copy_of(indentation);
  }
  else {
    groupstruct *born     = (groupstruct *)nmalloc(sizeof(groupstruct));
    born->top_line        = lineno;
    born->bottom_line     = lineno;
    born->indentations    = (char **)nmalloc(sizeof(char *));
    born->indentations[0] = copy_of(indentation);
    born->next            = u->grouping;
    u->grouping           = born;
  }
  /* Store the file size after the change, to be used when redoing. */
  u->newsize = openfile->totsize;
}

/* Update an undo item with (among other things) the file size and cursor position after the given action. */
void update_undo(undo_type action) {
  undostruct *u = openfile->undotop;
  Ulong datalen, newlen;
  char *textposition;
  int   charlen;
  if (u->type != action) {
    die("Mismatching undo type -- please report a bug\n");
  }
  u->newsize = openfile->totsize;
  switch (u->type) {
    case ADD : {
      newlen     = (openfile->current_x - u->head_x);
      u->strdata = arealloc(u->strdata, (newlen + 1));
      strncpy(u->strdata, (openfile->current->data + u->head_x), newlen);
      u->strdata[newlen] = '\0';
      u->tail_x          = openfile->current_x;
      break;
    }
    case ENTER : {
      u->strdata = copy_of(openfile->current->data);
      u->tail_x  = openfile->current_x;
      break;
    }
    case BACK :
    case DEL : {
      textposition = (openfile->current->data + openfile->current_x);
      charlen      = char_length(textposition);
      datalen      = strlen(u->strdata);
      if (openfile->current_x == u->head_x) {
        /* They deleted more: add removed character after earlier stuff. */
        u->strdata = arealloc(u->strdata, (datalen + charlen + 1));
        strncpy((u->strdata + datalen), textposition, charlen);
        u->strdata[datalen + charlen] = '\0';
        u->tail_x                     = openfile->current_x;
      }
      else if (openfile->current_x == u->head_x - charlen) {
        /* They backspaced further: add removed character before earlier. */
        u->strdata = arealloc(u->strdata, (datalen + charlen + 1));
        memmove((u->strdata + charlen), u->strdata, (datalen + 1));
        strncpy(u->strdata, textposition, charlen);
        u->head_x = openfile->current_x;
      }
      else {
        /* They deleted *elsewhere* on the line: start a new undo item. */
        add_undo(u->type, NULL);
      }
      break;
    }
    case REPLACE : {
      break;
    }
    case SPLIT_BEGIN :
    case SPLIT_END : {
      break;
    }
    case ZAP :
    case CUT_TO_EOF :
    case CUT : {
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
        u->tail_lineno = u->head_lineno + count;
        if (ISSET(CUT_FROM_CURSOR) || u->type == CUT_TO_EOF) {
          u->tail_x = strlen(bottomline->data);
          if (!count) {
            u->tail_x += u->head_x;
          }
        }
        else if (openfile->current == openfile->filebot && ISSET(NO_NEWLINES)) {
          u->tail_x = strlen(bottomline->data);
        }
      }
      break;
    }
    case COUPLE_BEGIN : {
      break;
    }
    case COUPLE_END :
    case PASTE :
    case INSERT : {
      u->tail_lineno = openfile->current->lineno;
      u->tail_x      = openfile->current_x;
      break;
    }
    default : {
      die("Bad undo type -- please report a bug\n");
    }
  }
}

// When the current line is overlong, hard-wrap it at the furthest possible whitespace character,
// and prepend the excess part to an "overflow" line (when it already exists, otherwise create one).
void do_wrap(void) {
  /* The line to be wrapped, if needed and possible. */
  linestruct *line = openfile->current;
  /* The length of this line. */
  Ulong line_len = strlen(line->data);
  /* The length of the quoting part of this line. */
  Ulong quot_len = quote_length(line->data);
  /* The length of the quoting part plus subsequent whitespace. */
  Ulong lead_len = quot_len + indent_length(line->data + quot_len);
  /* The current cursor position, for comparison with the wrap point. */
  Ulong cursor_x = openfile->current_x;
  /* The position in the line's text where we wrap. */
  long wrap_loc;
  /* The text after the wrap point. */
  const char *remainder;
  /* The length of the remainder. */
  Ulong rest_length;
  /* First find the last blank character where we can break the line. */
  wrap_loc = break_line((line->data + lead_len), (wrap_at - wideness(line->data, lead_len)), FALSE);
  /* If no wrapping point was found before end-of-line, we don't wrap. */
  if (wrap_loc < 0 || lead_len + wrap_loc == line_len) {
    return;
  }
  /* Adjust the wrap location to its position in the full line,
   * and step forward to the character just after the blank. */
  wrap_loc = (lead_len + step_right(line->data + lead_len, wrap_loc));
  /* When now at end-of-line, no need to wrap. */
  if (!line->data[wrap_loc]) {
    return;
  }
  add_undo(SPLIT_BEGIN, NULL);
  bool autowhite = ISSET(AUTOINDENT);
  if (quot_len > 0) {
    UNSET(AUTOINDENT);
  }
  /* The remainder is the text that will be wrapped to the next line. */
  remainder   = (line->data + wrap_loc);
  rest_length = (line_len - wrap_loc);
  /* When prepending and the remainder of this line will not make the next
   * line too long, then join the two lines, so that, after the line wrap,
   * the remainder will effectively have been prefixed to the next line. */
  if (openfile->spillage_line && openfile->spillage_line == line->next
   && (rest_length + breadth(line->next->data)) <= wrap_at) {
    /* Go to the end of this line. */
    openfile->current_x = line_len;
    /* If the remainder doesn't end in a blank, add a space. */
    if (!is_blank_char(remainder + step_left(remainder, rest_length))) {
      add_undo(ADD, NULL);
      line->data               = arealloc(line->data, (line_len + 2));
      line->data[line_len]     = ' ';
      line->data[line_len + 1] = '\0';
      rest_length++;
      openfile->totsize++;
      openfile->current_x++;
      update_undo(ADD);
    }
    /* Join the next line to this one. */
    expunge(DEL);
    /* If the leading part of the current line equals the leading part of
     * what was the next line, then strip this second leading part. */
    if (strncmp(line->data, line->data + openfile->current_x, lead_len) == 0) {
      for (Ulong i = lead_len; i > 0; i--) {
        expunge(DEL);
      }
    }
    /* Remove any extra blanks. */
    while (is_blank_char(&line->data[openfile->current_x])) {
      expunge(DEL);
    }
  }
  /* Go to the wrap location. */
  openfile->current_x = wrap_loc;
  /* When requested, snip trailing blanks off the wrapped line. */
  if (ISSET(TRIM_BLANKS)) {
    Ulong rear_x  = step_left(line->data, wrap_loc);
    Ulong typed_x = step_left(line->data, cursor_x);
    while ((rear_x != typed_x || cursor_x >= wrap_loc) && is_blank_char(line->data + rear_x)) {
      openfile->current_x = rear_x;
      expunge(DEL);
      rear_x = step_left(line->data, rear_x);
    }
  }
  /* Now split the line. */
  do_enter();
  /* When wrapping a partially visible line, adjust start-of-screen. */
  if (openfile->edittop == line && openfile->firstcolumn > 0 && cursor_x >= wrap_loc) {
    go_forward_chunks(1, &openfile->edittop, &openfile->firstcolumn);
  }
  /* If the original line has quoting, copy it to the spillage line. */
  if (quot_len > 0) {
    line       = line->next;
    line_len   = strlen(line->data);
    line->data = arealloc(line->data, (lead_len + line_len + 1));
    memmove((line->data + lead_len), line->data, (line_len + 1));
    strncpy(line->data, line->prev->data, lead_len);
    openfile->current_x += lead_len;
    openfile->totsize += lead_len;
    free(openfile->undotop->strdata);
    update_undo(ENTER);
    if (autowhite) {
      SET(AUTOINDENT);
    }
  }
  openfile->spillage_line = openfile->current;
  if (cursor_x < wrap_loc) {
    openfile->current   = openfile->current->prev;
    openfile->current_x = cursor_x;
  }
  else {
    openfile->current_x += (cursor_x - wrap_loc);
  }
  openfile->placewewant = xplustabs();
  add_undo(SPLIT_END, NULL);
  refresh_needed = TRUE;
}

// Find the last blank in the given piece of text such that the display width to that point is at most
// (goal + 1).  When there is no such blank, then find the first blank.  Return the index of the last
// blank in that group of blanks. When snap_at_nl is TRUE, a newline character counts as a blank too.
long break_line(const char *textstart, long goal, bool snap_at_nl) {
  /* The point where the last blank was found, if any. */
  const char *lastblank = NULL;
  /* An iterator through the given line of text. */
  const char *pointer = textstart;
  /* The column number that corresponds to the position of the pointer. */
  Ulong column = 0;
  /* Skip over leading whitespace, where a line should never be broken. */
  while (*pointer && is_blank_char(pointer)) {
    pointer += advance_over(pointer, column);
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
    pointer += advance_over(pointer, column);
  }
  /* If the whole line displays shorter than goal, we're done. */
  if ((long)column <= goal) {
    return (long)(pointer - textstart);
  }
  /* When wrapping a help text and no blank was found, force a line break. */
  if (snap_at_nl && !lastblank) {
    return (long)step_left(textstart, pointer - textstart);
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

/* Return the length of the indentation part of the given line.  The "indentation" of a line is the leading consecutive whitespace. */
Ulong indent_length(const char *line) {
  const char *start = line;
  while (*line && is_blank_char(line)) {
    line += char_length(line);
  }
  return (Ulong)(line - start);
}

// Return the length of the quote part of the given line.  The 'quote part'
// of a line is the largest initial substring matching the quoting regex.
Ulong quote_length(const char *line) {
  regmatch_t matches;
  int rc = regexec(&quotereg, line, 1, &matches, 0);
  if (rc == REG_NOMATCH || matches.rm_so == (regoff_t)-1) {
    return 0;
  }
  return matches.rm_eo;
}

/* The maximum depth of recursion.  Note that this MUST be an even number. */
#define RECURSION_LIMIT 222

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
  if (wideness(line->prev->data, quot_len + prev_dent_len) == wideness(line->data, quot_len + indent_len)) {
    return FALSE;
  }
  /* Otherwise, this is a BOP if the preceding line is not. */
  return !begpar(line->prev, depth + 1);
}

/* Return TRUE when the given line is part of a paragraph.  A line is part of a
 * paragraph if it contains something more than quoting and leading whitespace. */
bool inpar(const linestruct *const line) {
  Ulong quot_len   = quote_length(line->data);
  Ulong indent_len = indent_length(line->data + quot_len);
  return (line->data[quot_len + indent_len]);
}

/* Find the first occurring paragraph in the forward direction.  Return 'TRUE' when a paragraph was found,
 * and 'FALSE' otherwise.  Furthermore, return the first line and the number of lines of the paragraph. */
bool find_paragraph(linestruct **firstline, Ulong *linecount) {
  linestruct *line = *firstline;
  /* When not currently in a paragraph, move forward to a line that is. */
  while (!inpar(line) && line->next) {
    line = line->next;
  }
  *firstline = line;
  /* Move down to the last line of the paragraph (if any). */
  do_para_end(&line);
  /* When not in a paragraph now, there aren't any paragraphs left. */
  if (!inpar(line)) {
    return FALSE;
  }
  /* We found a paragraph.  Now we determine its number of lines. */
  *linecount = (line->lineno - (*firstline)->lineno + 1);
  return TRUE;
}

/* Concatenate into a single line all the lines of the paragraph that starts at 'line' and
 * consists of 'count' lines, skipping the quoting and indentation on all lines after the first. */
void concat_paragraph(linestruct *line, Ulong count) {
  while (count > 1) {
    linestruct *next_line     = line->next;
    Ulong       next_line_len = strlen(next_line->data);
    Ulong       next_quot_len = quote_length(next_line->data);
    Ulong       next_lead_len = next_quot_len + indent_length(next_line->data + next_quot_len);
    Ulong       line_len      = strlen(line->data);
    /* We're just about to tack the next line onto this one.  If this line isn't empty, make sure it ends in a space. */
    if (line_len > 0 && line->data[line_len - 1] != ' ') {
      line->data             = arealloc(line->data, (line_len + 2));
      line->data[line_len++] = ' ';
      line->data[line_len]   = '\0';
    }
    line->data = arealloc(line->data, (line_len + next_line_len - next_lead_len + 1));
    strcat(line->data, (next_line->data + next_lead_len));
    line->has_anchor |= next_line->has_anchor;
    unlink_node(next_line);
    count--;
  }
}

/* Copy a character from one place to another. */
void copy_character(char **from, char **to) {
  int charlen = char_length(*from);
  if (*from == *to) {
    *from += charlen;
    *to += charlen;
  }
  else {
    while (--charlen >= 0) {
      *((*to)++) = *((*from)++);
    }
  }
}

/* In the given line, replace any series of blanks with a single space, but keep two
 * spaces (if there are two) after any closing punctuation, and remove all blanks from
 * the end of the line.  Leave the first skip number of characters untreated. */
void squeeze(linestruct *line, Ulong skip) {
  char *start = (line->data + skip);
  char *from = start, *to = start;
  /* For each character, 1) when a blank, change it to a space, and pass over all blanks after it;
   * 2) if it is punctuation, copy it plus a possible tailing bracket, and change at most two subsequent
   * blanks to spaces, and * pass over all blanks after these; 3) leave anything else unchanged. */
  while (*from) {
    if (is_blank_char(from)) {
      from += char_length(from);
      *(to++) = ' ';
      while (*from && is_blank_char(from)) {
        from += char_length(from);
      }
    }
    else if (mbstrchr(punct, from)) {
      copy_character(&from, &to);
      if (*from && mbstrchr(brackets, from)) {
        copy_character(&from, &to);
      }
      if (*from && is_blank_char(from)) {
        from += char_length(from);
        *(to++) = ' ';
      }
      if (*from && is_blank_char(from)) {
        from += char_length(from);
        *(to++) = ' ';
      }
      while (*from && is_blank_char(from)) {
        from += char_length(from);
      }
    }
    else {
      copy_character(&from, &to);
    }
  }
  /* If there are spaces at the end of the line, remove them. */
  while (to > start && *(to - 1) == ' ') {
    to--;
  }
  *to = '\0';
}

/* Rewrap the given line (that starts with the given lead string which is of the given length), into lines that fit within the target width (wrap_at). */
void rewrap_paragraph(linestruct **line, char *lead_string, Ulong lead_len) {
  /* The x-coordinate where the current line is to be broken. */
  long break_pos;
  while (breadth((*line)->data) > wrap_at) {
    Ulong line_len = strlen((*line)->data);
    /* Find a point in the line where it can be broken. */
    break_pos = break_line(((*line)->data + lead_len), (wrap_at - wideness((*line)->data, lead_len)), FALSE);
    /* If we can't break the line, or don't need to, we're done. */
    if (break_pos < 0 || lead_len + break_pos == line_len) {
      break;
    }
    /* Adjust the breaking position for the leading part and move it beyond the found whitespace character. */
    break_pos += (lead_len + 1);
    /* Insert a new line after the current one, and copy the leading part plus the text after the breaking point into it. */
    splice_node(*line, make_new_node(*line));
    (*line)->next->data = (char *)nmalloc(lead_len + line_len - break_pos + 1);
    strncpy((*line)->next->data, lead_string, lead_len);
    strcpy((*line)->next->data + lead_len, (*line)->data + break_pos);
    /* When requested, snip the one or two trailing spaces. */
    if (ISSET(TRIM_BLANKS)) {
      while (break_pos > 0 && (*line)->data[break_pos - 1] == ' ') {
        --break_pos;
      }
    }
    /* Now actually break the current line, and go to the next. */
    (*line)->data[break_pos] = '\0';
    *line                    = (*line)->next;
  }
  /* If the new paragraph exceeds the viewport, recalculate the multidata. */
  if ((*line)->lineno >= editwinrows) {
    recook = TRUE;
  }
  /* When possible, go to the line after the rewrapped paragraph. */
  if ((*line)->next) {
    *line = (*line)->next;
  }
}

/* Justify the lines of the given paragraph (that starts at *line, and consists of 'count' lines)
 * so they all fit within the target width (wrap_at) and have their whitespace normalized. */
void justify_paragraph(linestruct **line, Ulong count) {
  linestruct *sampleline; /* The line from which the indentation is copied. */
  Ulong quot_len;         /* Length of the quote part. */
  Ulong lead_len;         /* Length of the quote part plus the indentation part. */
  char *lead_string;      /* The quote+indent stuff that is copied from the sample line. */
  /* The sample line is either the only line or the second line. */
  sampleline = (count == 1 ? *line : (*line)->next);
  /* Copy the leading part (quoting + indentation) of the sample line. */
  quot_len    = quote_length(sampleline->data);
  lead_len    = quot_len + indent_length(sampleline->data + quot_len);
  lead_string = measured_copy(sampleline->data, lead_len);
  /* Concatenate all lines of the paragraph into a single line. */
  concat_paragraph(*line, count);
  /* Change all blank characters to spaces and remove excess spaces. */
  squeeze(*line, quot_len + indent_length((*line)->data + quot_len));
  /* Rewrap the line into multiple lines, accounting for the leading part. */
  rewrap_paragraph(line, lead_string, lead_len);
  free(lead_string);
}

#define ONE_PARAGRAPH FALSE
#define WHOLE_BUFFER  TRUE

/* Justify the current paragraph, or the entire buffer when whole_buffer is
 * 'TRUE'.  But if the mark is on, justify only the marked text instead. */
void justify_text(bool whole_buffer) {
  /* The number of lines in the original paragraph. */
  Ulong linecount;
  /* The line where the paragraph or region starts. */
  linestruct *startline;
  /* The line where the paragraph or region ends. */
  linestruct *endline;
  /* The x position where the paragraph or region starts. */
  Ulong start_x;
  /* The x position where the paragraph or region ends. */
  Ulong end_x;
  /* The old cutbuffer, so we can justify in the current cutbuffer. */
  linestruct *was_cutbuffer = cutbuffer;
  /* The line that we're justifying in the current cutbuffer. */
  linestruct *jusline;
  /* Whether the end of a marked region is before the end of its line. */
  bool before_eol = FALSE;
  /* The leading part (quoting + indentation) of the first line
   * of the paragraph where the marked region begins. */
  char *primary_lead = NULL;
  /* The length (in bytes) of the above first-line leading part. */
  Ulong primary_len = 0;
  /* The leading part for lines after the first one. */
  char *secondary_lead = NULL;
  /* The length of that later lead. */
  Ulong secondary_len = 0;
  /* The line to return to after a full justification. */
  long was_the_linenumber = openfile->current->lineno;
  bool marked_backward    = (openfile->mark && !mark_is_before_cursor());
  /* TRANSLATORS: This one goes with Undid/Redid messages. */
  add_undo(COUPLE_BEGIN, N_("justification"));
  /* If the mark is on, do as Pico: treat all marked text as one paragraph. */
  if (openfile->mark) {
    Ulong       quot_len, fore_len, other_quot_len, other_white_len;
    linestruct *sampleline;
    get_region(&startline, &start_x, &endline, &end_x);
    /* When the marked region is empty, do nothing. */
    if (startline == endline && start_x == end_x) {
      statusline(AHEM, _("Selection is empty"));
      discard_until(openfile->undotop->next);
      return;
    }
    quot_len = quote_length(startline->data);
    fore_len = quot_len + indent_length(startline->data + quot_len);
    /* When the region starts IN the lead, take the whole lead. */
    if (start_x <= fore_len) {
      start_x = 0;
    }
    /* Recede over blanks before the region.  This effectively snips
     * trailing blanks from what will become the preceding paragraph. */
    while (start_x > 0 && is_blank_char(&startline->data[start_x - 1])) {
      start_x = step_left(startline->data, start_x);
    }
    quot_len = quote_length(endline->data);
    fore_len = quot_len + indent_length(endline->data + quot_len);
    /* When the region ends IN the lead, take the whole lead. */
    if (0 < end_x && end_x < fore_len) {
      end_x = fore_len;
    }
    /* When not at the left edge, advance over blanks after the region. */
    while (end_x > 0 && is_blank_char(&endline->data[end_x])) {
      end_x = step_right(endline->data, end_x);
    }
    sampleline = startline;
    /* Find the first line of the paragraph in which the region starts. */
    while (sampleline->prev && inpar(sampleline) && !begpar(sampleline, 0)) {
      sampleline = sampleline->prev;
    }
    /* Ignore lines that contain no text. */
    while (sampleline->next && !inpar(sampleline)) {
      sampleline = sampleline->next;
    }
    /* Store the leading part that is to be used for the new paragraph. */
    quot_len     = quote_length(sampleline->data);
    primary_len  = quot_len + indent_length(sampleline->data + quot_len);
    primary_lead = measured_copy(sampleline->data, primary_len);
    if (sampleline->next && startline != endline) {
      sampleline = sampleline->next;
    }
    /* Copy the leading part that is to be used for the new paragraph after its first line
     * (if any):  the quoting of the first line, plus the indentation of the second line. */
    other_quot_len  = quote_length(sampleline->data);
    other_white_len = indent_length(sampleline->data + other_quot_len);
    secondary_len   = quot_len + other_white_len;
    secondary_lead  = (char *)nmalloc(secondary_len + 1);
    strncpy(secondary_lead, startline->data, quot_len);
    strncpy(secondary_lead + quot_len, sampleline->data + other_quot_len, other_white_len);
    secondary_lead[secondary_len] = '\0';
    /* Include preceding and succeeding leads into the marked region. */
    openfile->mark      = startline;
    openfile->mark_x    = start_x;
    openfile->current   = endline;
    openfile->current_x = end_x;
    linecount           = endline->lineno - startline->lineno + (end_x > 0 ? 1 : 0);
    /* Remember whether the end of the region was before the end-of-line. */
    before_eol = endline->data[end_x] != '\0';
  }
  else {
    /* When justifying the entire buffer, start at the top.  Otherwise, when
     * in a paragraph but not at its beginning, move back to its first line. */
    if (whole_buffer) {
      openfile->current = openfile->filetop;
    }
    else if (inpar(openfile->current) && !begpar(openfile->current, 0)) {
      do_para_begin(&openfile->current);
    }
    /* Find the first line of the paragraph(s) to be justified.  If the
     * search fails, there is nothing to justify, and we will be on the
     * last line of the file, so put the cursor at the end of it. */
    if (!find_paragraph(&openfile->current, &linecount)) {
      openfile->current_x = strlen(openfile->filebot->data);
      discard_until(openfile->undotop->next);
      refresh_needed = TRUE;
      return;
    }
    else {
      openfile->current_x = 0;
    }
    /* Set the starting point of the paragraph. */
    startline = openfile->current;
    start_x   = 0;
    /* Set the end point of the paragraph. */
    if (whole_buffer) {
      endline = openfile->filebot;
    }
    else {
      endline = startline;
      for (Ulong count = linecount; count > 1; count--) {
        endline = endline->next;
      }
    }
    /* When possible, step one line further; otherwise, to line's end. */
    if (endline->next) {
      endline = endline->next;
      end_x   = 0;
    }
    else {
      end_x = strlen(endline->data);
    }
  }
  add_undo(CUT, NULL);
  /* Do the equivalent of a marked cut into an empty cutbuffer. */
  cutbuffer = NULL;
  extract_segment(startline, start_x, endline, end_x);
  update_undo(CUT);
  if (openfile->mark) {
    linestruct *line     = cutbuffer;
    Ulong       quot_len = quote_length(line->data);
    Ulong       fore_len = quot_len + indent_length(line->data + quot_len);
    Ulong       text_len = strlen(line->data) - fore_len;
    /* If the extracted region begins with any leading part, trim it. */
    if (fore_len > 0) {
      memmove(line->data, line->data + fore_len, text_len + 1);
    }
    /* Then copy back in the leading part that it should have. */
    if (primary_len > 0) {
      line->data = (char *)nrealloc(line->data, primary_len + text_len + 1);
      memmove(line->data + primary_len, line->data, text_len + 1);
      strncpy(line->data, primary_lead, primary_len);
    }
    /* Now justify the extracted region. */
    concat_paragraph(cutbuffer, linecount);
    squeeze(cutbuffer, primary_len);
    rewrap_paragraph(&line, secondary_lead, secondary_len);
    /* If the marked region started in the middle of a line,
     * insert a newline before the new paragraph. */
    if (start_x > 0) {
      cutbuffer->prev       = make_new_node(NULL);
      cutbuffer->prev->data = copy_of("");
      cutbuffer->prev->next = cutbuffer;
      cutbuffer             = cutbuffer->prev;
    }
    /* If the marked region ended in the middle of a line,
     * insert a newline after the new paragraph. */
    if (end_x > 0 && before_eol) {
      line->next       = make_new_node(line);
      line->next->data = copy_of(primary_lead);
    }
    free(secondary_lead);
    free(primary_lead);
    /* Keep as much of the marked region onscreen as possible. */
    focusing = FALSE;
  }
  else {
    /* Prepare to justify the text we just put in the cutbuffer. */
    jusline = cutbuffer;
    /* Justify the current paragraph. */
    justify_paragraph(&jusline, linecount);
    /* When justifying the entire buffer, find and justify all paragraphs. */
    if (whole_buffer) {
      while (find_paragraph(&jusline, &linecount)) {
        justify_paragraph(&jusline, linecount);
        if (!jusline->next) {
          break;
        }
      }
    }
  }
  /* Wipe an anchor on the first paragraph if it was only inherited. */
  if (whole_buffer && !openfile->mark && !cutbuffer->has_anchor) {
    openfile->current->has_anchor = FALSE;
  }
  add_undo(PASTE, NULL);
  /* Do the equivalent of a paste of the justified text. */
  ingraft_buffer(cutbuffer);
  update_undo(PASTE);
  /* After justifying a backward-marked text, swap mark and cursor. */
  if (marked_backward) {
    linestruct *bottom   = openfile->current;
    Ulong       bottom_x = openfile->current_x;
    openfile->current    = openfile->mark;
    openfile->current_x  = openfile->mark_x;
    openfile->mark       = bottom;
    openfile->mark_x     = bottom_x;
  }
  else if (whole_buffer && !openfile->mark) {
    goto_line_posx(was_the_linenumber, 0);
  }
  add_undo(COUPLE_END, N_("justification"));
  /* Report on the status bar what we justified. */
  if (openfile->mark) {
    statusline(REMARK, _("Justified selection"));
  }
  else if (whole_buffer) {
    statusline(REMARK, _("Justified file"));
  }
  else {
    statusbar(_("Justified paragraph"));
  }
  /* We're done justifying.  Restore the cutbuffer. */
  cutbuffer = was_cutbuffer;
  /* Set the desired screen column (always zero, except at EOF). */
  openfile->placewewant = xplustabs();
  set_modified();
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Justify the current paragraph. */
void do_justify(void) {
  justify_text(ONE_PARAGRAPH);
}

/* Justify the entire file. */
void do_full_justify(void) {
  justify_text(WHOLE_BUFFER);
  ran_a_tool = TRUE;
  recook     = TRUE;
}

/* Set up an argument list for executing the given command. */
void construct_argument_list(char ***arguments, char *command, char *filename) {
  char *copy_of_command = copy_of(command);
  char *element = strtok(copy_of_command, " ");
  int count = 2;
  while (element) {
    (*arguments) = anrealloc(*arguments, (++count * sizeof(char *)));
    (*arguments)[count - 3] = element;
    element = strtok(NULL, " ");
  }
  (*arguments)[count - 2] = filename;
  (*arguments)[count - 1] = NULL;
}

/* Open the specified file, and if that succeeds, remove the text of the marked
 * region or of the entire buffer and read the file contents into its place. */
bool replace_buffer(const char *filename, undo_type action, const char *operation) {
  linestruct *was_cutbuffer = cutbuffer;
  int         descriptor;
  FILE       *stream;
  descriptor = open_file(filename, FALSE, &stream);
  if (descriptor < 0) {
    return FALSE;
  }
  add_undo(COUPLE_BEGIN, operation);
  /* When replacing the whole buffer, start cutting at the top. */
  if (action == CUT_TO_EOF) {
    openfile->current   = openfile->filetop;
    openfile->current_x = 0;
  }
  cutbuffer = NULL;
  /* Cut either the marked region or the whole buffer. */
  add_undo(action, NULL);
  do_snip((openfile->mark != NULL), (openfile->mark == NULL), FALSE);
  update_undo(action);
  /* Discard what was cut. */
  free_lines(cutbuffer);
  cutbuffer = was_cutbuffer;
  /* Insert the spell-checked file into the cleared area. */
  read_file(stream, descriptor, filename, TRUE);
  add_undo(COUPLE_END, operation);
  return TRUE;
}

/* Execute the given program, with the given temp file as last argument. */
void treat(char *tempfile_name, char *theprogram, bool spelling) {
  long          was_lineno = openfile->current->lineno;
  Ulong         was_pww    = openfile->placewewant;
  Ulong         was_x      = openfile->current_x;
  bool          was_at_eol = !(openfile->current->data[openfile->current_x]);
  struct stat   fileinfo;
  long          timestamp_sec  = 0;
  long          timestamp_nsec = 0;
  static char **arguments      = NULL;
  pid_t         thepid;
  int           program_status, errornumber;
  bool          replaced = FALSE;
  /* Stat the temporary file.  If that succeeds and its size is zero,
   * there is nothing to do; otherwise, store its time of modification. */
  if (stat(tempfile_name, &fileinfo) == 0) {
    if (fileinfo.st_size == 0) {
      if (spelling && openfile->mark) {
        statusline(AHEM, _("Selection is empty"));
      }
      else {
        statusline(AHEM, _("Buffer is empty"));
      }
      return;
    }
    timestamp_sec  = (long)fileinfo.st_mtim.tv_sec;
    timestamp_nsec = (long)fileinfo.st_mtim.tv_nsec;
  }
  /* The spell checker needs the screen, so exit from curses mode. */
  if (spelling) {
    endwin();
  }
  else {
    statusbar(_("Invoking formatter..."));
  }
  construct_argument_list(&arguments, theprogram, tempfile_name);
  /* Fork a child process and run the given program in it. */
  if ((thepid = fork()) == 0) {
    execvp(arguments[0], arguments);
    /* Terminate the child if the program is not found. */
    exit(9);
  }
  else if (thepid > 0) {
    /* Block SIGWINCHes while waiting for the forked program to end,
     * so nano doesn't get pushed past the wait(). */
    block_sigwinch(TRUE);
    wait(&program_status);
    block_sigwinch(FALSE);
  }
  errornumber = errno;
  /* After spell checking, restore terminal state and reenter curses mode;
   * after formatting, make sure that any formatter output is wiped. */
  if (spelling) {
    terminal_init();
    doupdate();
  }
  else {
    full_refresh();
  }
  if (thepid < 0) {
    statusline(ALERT, _("Could not fork: %s"), strerror(errornumber));
    free(arguments[0]);
    return;
  }
  else if (!WIFEXITED(program_status) || WEXITSTATUS(program_status) > 2) {
    statusline(ALERT, _("Error invoking '%s'"), arguments[0]);
    free(arguments[0]);
    return;
  }
  else if (WEXITSTATUS(program_status) != 0) {
    statusline(ALERT, _("Program '%s' complained"), arguments[0]);
  }
  free(arguments[0]);
  /* When the temporary file wasn't touched, say so and leave. */
  if (timestamp_sec > 0 && stat(tempfile_name, &fileinfo) == 0 && (long)fileinfo.st_mtim.tv_sec == timestamp_sec
   && (long)fileinfo.st_mtim.tv_nsec == timestamp_nsec) {
    statusline(REMARK, _("Nothing changed"));
    return;
  }
  /* Replace the marked text (or entire text) with the corrected text. */
  if (spelling && openfile->mark) {
    long was_mark_lineno = openfile->mark->lineno;
    bool upright         = mark_is_before_cursor();
    replaced             = replace_buffer(tempfile_name, CUT, "spelling correction");
    /* Adjust the end point of the marked region for any change in
     * length of the region's last line. */
    if (upright) {
      was_x = openfile->current_x;
    }
    else {
      openfile->mark_x = openfile->current_x;
    }
    /* Restore the mark. */
    openfile->mark = line_from_number(was_mark_lineno);
  }
  else {
    replaced = replace_buffer(tempfile_name, CUT_TO_EOF,
                              /* TRANSLATORS: The next two go with Undid/Redid messages. */
                              (spelling ? N_("spelling correction") : N_("formatting")));
  }
  /* Go back to the old position. */
  goto_line_posx(was_lineno, was_x);
  if (was_at_eol || openfile->current_x > strlen(openfile->current->data)) {
    openfile->current_x = strlen(openfile->current->data);
  }
  if (replaced) {
    openfile->filetop->has_anchor = FALSE;
    update_undo(COUPLE_END);
  }
  openfile->placewewant = was_pww;
  adjust_viewport(STATIONARY);
  if (spelling) {
    statusline(REMARK, _("Finished checking spelling"));
  }
  else {
    statusline(REMARK, _("Buffer has been processed"));
  }
}

/* Let the user edit the misspelled word.  Return 'FALSE' if the user cancels. */
bool fix_spello(const char *word) {
  linestruct *was_edittop     = openfile->edittop;
  linestruct *was_current     = openfile->current;
  Ulong       was_firstcolumn = openfile->firstcolumn;
  Ulong       was_x           = openfile->current_x;
  bool        proceed         = FALSE;
  int         result;
  bool        right_side_up = (openfile->mark && mark_is_before_cursor());
  linestruct *top, *bot;
  Ulong       top_x, bot_x;
  /* If the mark is on, start at the beginning of the marked region. */
  if (openfile->mark) {
    /* If the region is marked normally, swap the end points, so that
     * (current, current_x) (where searching starts) is at the top. */
    get_region(&top, &top_x, &bot, &bot_x);
    if (right_side_up) {
      openfile->current   = top;
      openfile->current_x = top_x;
      openfile->mark      = bot;
      openfile->mark_x    = bot_x;
    }
  }
  /* Otherwise, start from the top of the file. */
  else {
    openfile->current   = openfile->filetop;
    openfile->current_x = 0;
  }
  /* Find the first whole occurrence of word. */
  result = findnextstr(word, TRUE, INREGION, NULL, FALSE, NULL, 0);
  /* If the word isn't found, alert the user; if it is, allow correction. */
  if (result == 0) {
    statusline(ALERT, _("Unfindable word: %s"), word);
    lastmessage = VACUUM;
    proceed     = TRUE;
    napms(2800);
  }
  else if (result == 1) {
    spotlighted            = TRUE;
    light_from_col         = xplustabs();
    light_to_col           = light_from_col + breadth(word);
    linestruct *saved_mark = openfile->mark;
    openfile->mark         = NULL;
    edit_refresh();
    put_cursor_at_end_of_answer();
    /* Let the user supply a correctly spelled alternative. */
    proceed        = (do_prompt(MSPELL, word, NULL, edit_refresh,
                                /* TRANSLATORS: This is a prompt. */
                                _("Edit a replacement")) != -1);
    spotlighted    = FALSE;
    openfile->mark = saved_mark;
    /* If a replacement was given, go through all occurrences. */
    if (proceed && strcmp(word, answer) != 0) {
      do_replace_loop(word, TRUE, was_current, &was_x);
      /* TRANSLATORS: Shown after fixing misspellings in one word. */
      statusbar(_("Next word..."));
      napms(400);
    }
  }
  if (openfile->mark) {
    /* Restore the (compensated) end points of the marked region. */
    if (right_side_up) {
      openfile->current   = openfile->mark;
      openfile->current_x = openfile->mark_x;
      openfile->mark      = top;
      openfile->mark_x    = top_x;
    }
    else {
      openfile->current   = top;
      openfile->current_x = top_x;
    }
  }
  else {
    /* Restore the (compensated) cursor position. */
    openfile->current   = was_current;
    openfile->current_x = was_x;
  }
  /* Restore the viewport to where it was. */
  openfile->edittop     = was_edittop;
  openfile->firstcolumn = was_firstcolumn;
  return proceed;
}

/* Run a spell-check on the given file, using 'spell' to produce a list of all misspelled words, then feeding those through
 * 'sort' and 'uniq' to obtain an alphabetical list, which words are then offered one by one to the user for correction. */
void do_int_speller(const char *const tempfile_name) {
  char *misspellings, *pointer, *oneword;
  long  pipesize;
  Ulong buffersize, bytesread, totalread;
  int   spell_fd[2], sort_fd[2], uniq_fd[2], tempfile_fd = -1;
  pid_t pid_spell, pid_sort, pid_uniq;
  int   spell_status, sort_status, uniq_status;
  Ulong stash[sizeof(flags) / sizeof(flags[0])];
  /* Create all three pipes up front. */
  if (pipe(spell_fd) == -1 || pipe(sort_fd) == -1 || pipe(uniq_fd) == -1) {
    statusline(ALERT, _("Could not create pipe: %s"), strerror(errno));
    return;
  }
  statusbar(_("Invoking spell checker..."));
  /* Fork a process to run spell in. */
  if ((pid_spell = fork()) == 0) {
    /* Child: open the temporary file that holds the text to be checked. */
    if ((tempfile_fd = open(tempfile_name, O_RDONLY)) == -1) {
      exit(6);
    }
    /* Connect standard input to the temporary file. */
    if (dup2(tempfile_fd, STDIN_FILENO) < 0) {
      exit(7);
    }
    /* Connect standard output to the write end of the first pipe. */
    if (dup2(spell_fd[1], STDOUT_FILENO) < 0) {
      exit(8);
    }
    close(tempfile_fd);
    close(spell_fd[0]);
    close(spell_fd[1]);
    /* Try to run 'hunspell'; if that fails, fall back to 'spell'. */
    execlp("hunspell", "hunspell", "-l", NULL);
    execlp("spell", "spell", NULL);
    /* Indicate failure when neither speller was found. */
    exit(9);
  }
  /* Parent: close the unused write end of the first pipe. */
  close(spell_fd[1]);
  /* Fork a process to run sort in. */
  if ((pid_sort = fork()) == 0) {
    /* Connect standard input to the read end of the first pipe. */
    if (dup2(spell_fd[0], STDIN_FILENO) < 0) {
      exit(7);
    }
    /* Connect standard output to the write end of the second pipe. */
    if (dup2(sort_fd[1], STDOUT_FILENO) < 0) {
      exit(8);
    }
    close(spell_fd[0]);
    close(sort_fd[0]);
    close(sort_fd[1]);
    /* Now run the sort program.  Use -f to mix upper and lower case. */
    execlp("sort", "sort", "-f", NULL);
    exit(9);
  }
  close(spell_fd[0]);
  close(sort_fd[1]);
  /* Fork a process to run uniq in. */
  if ((pid_uniq = fork()) == 0) {
    if (dup2(sort_fd[0], STDIN_FILENO) < 0) {
      exit(7);
    }
    if (dup2(uniq_fd[1], STDOUT_FILENO) < 0) {
      exit(8);
    }
    close(sort_fd[0]);
    close(uniq_fd[0]);
    close(uniq_fd[1]);
    execlp("uniq", "uniq", NULL);
    exit(9);
  }
  close(sort_fd[0]);
  close(uniq_fd[1]);
  /* When some child process was not forked successfully... */
  if (pid_spell < 0 || pid_sort < 0 || pid_uniq < 0) {
    statusline(ALERT, _("Could not fork: %s"), ERRNO_C_STR);
    close(uniq_fd[0]);
    return;
  }
  /* Get the system pipe buffer size. */
  pipesize = fpathconf(uniq_fd[0], _PC_PIPE_BUF);
  if (pipesize < 1) {
    statusline(ALERT, _("Could not get size of pipe buffer"));
    close(uniq_fd[0]);
    return;
  }
  /* Leave curses mode so that error messages go to the original screen. */
  endwin();
  /* Block SIGWINCHes while reading misspelled words from the third pipe. */
  block_sigwinch(TRUE);
  totalread    = 0;
  buffersize   = pipesize + 1;
  misspellings = (char *)nmalloc(buffersize);
  pointer      = misspellings;
  while ((bytesread = read(uniq_fd[0], pointer, pipesize)) > 0) {
    totalread += bytesread;
    buffersize += pipesize;
    misspellings = (char *)nrealloc(misspellings, buffersize);
    pointer      = misspellings + totalread;
  }
  *pointer = '\0';
  close(uniq_fd[0]);
  block_sigwinch(FALSE);
  /* Re-enter curses mode. */
  terminal_init();
  doupdate();
  /* Save the settings of the global flags. */
  memcpy(stash, flags, sizeof(flags));
  /* Do any replacements case-sensitively, forward, and without regexes. */
  SET(CASE_SENSITIVE);
  UNSET(BACKWARDS_SEARCH);
  UNSET(USE_REGEXP);
  pointer = misspellings;
  oneword = misspellings;
  /* Process each of the misspelled words. */
  while (*pointer != '\0') {
    if ((*pointer == '\r') || (*pointer == '\n')) {
      *pointer = '\0';
      if (oneword != pointer) {
        if (!fix_spello(oneword)) {
          oneword = pointer;
          break;
        }
      }
      oneword = pointer + 1;
    }
    pointer++;
  }
  /* Special case: the last word doesn't end with '\r' or '\n'. */
  if (oneword != pointer) {
    fix_spello(oneword);
  }
  free(misspellings);
  refresh_needed = TRUE;
  /* Restore the settings of the global flags. */
  memcpy(flags, stash, sizeof(flags));
  /* Process the end of the three processes. */
  waitpid(pid_spell, &spell_status, 0);
  waitpid(pid_sort, &sort_status, 0);
  waitpid(pid_uniq, &uniq_status, 0);
  if (WIFEXITED(uniq_status) == 0 || WEXITSTATUS(uniq_status)) {
    statusline(ALERT, _("Error invoking \"uniq\""));
  }
  else if (WIFEXITED(sort_status) == 0 || WEXITSTATUS(sort_status)) {
    statusline(ALERT, _("Error invoking \"sort\""));
  }
  else if (WIFEXITED(spell_status) == 0 || WEXITSTATUS(spell_status)) {
    statusline(ALERT, _("Error invoking \"spell\""));
  }
  else {
    statusline(REMARK, _("Finished checking spelling"));
  }
}

/* Spell check the current file.  If an alternate spell checker is specified, use it.  Otherwise, use the internal spell checker. */
void do_spell(void) {
  FILE *stream;
  char *temp_name;
  bool  okay;
  ran_a_tool = TRUE;
  if (in_restricted_mode()) {
    return;
  }
  temp_name = safe_tempfile(&stream);
  if (!temp_name) {
    statusline(ALERT, _("Error writing temp file: %s"), strerror(errno));
    return;
  }
  if (openfile->mark) {
    okay = write_region_to_file(temp_name, stream, TEMPORARY, OVERWRITE);
  }
  else {
    okay = write_file(temp_name, stream, TEMPORARY, OVERWRITE, NONOTES);
  }
  if (!okay) {
    statusline(ALERT, _("Error writing temp file: %s"), strerror(errno));
    unlink(temp_name);
    free(temp_name);
    return;
  }
  blank_bottombars();
  if (alt_speller && *alt_speller) {
    treat(temp_name, alt_speller, TRUE);
  }
  else {
    do_int_speller(temp_name);
  }
  unlink(temp_name);
  free(temp_name);
  /* Ensure the help lines will be redrawn and a selection is retained. */
  currmenu   = MMOST;
  shift_held = TRUE;
}

/* Run a linting program on the current buffer. */
void do_linter(void) {
  char       *lintings, *pointer, *onelint;
  long        pipesize;
  Ulong       buffersize, bytesread, totalread;
  bool        parsesuccess, helpless;
  int         lint_status, lint_fd[2];
  pid_t       pid_lint;
  lintstruct *lints, *tmplint, *curlint;
  time_t      last_wait;
  parsesuccess = FALSE;
  helpless     = ISSET(NO_HELP);
  lints        = NULL;
  tmplint      = NULL;
  curlint      = NULL;
  last_wait    = 0;
  ran_a_tool   = TRUE;
  if (in_restricted_mode()) {
    return;
  }
  if (!openfile->syntax || !openfile->syntax->linter || !*openfile->syntax->linter) {
    statusline(AHEM, _("No linter is defined for this type of file"));
    return;
  }
  openfile->mark = NULL;
  edit_refresh();
  if (openfile->modified) {
    int choice = ask_user(YESORNO, _("Save modified buffer before linting?"));
    if (choice == CANCEL) {
      statusbar(_("Cancelled"));
      return;
    }
    else if (choice == YES && (write_it_out(FALSE, FALSE) != 1)) {
      return;
    }
  }
  /* Create a pipe up front. */
  if (pipe(lint_fd) == -1) {
    statusline(ALERT, _("Could not create pipe: %s"), strerror(errno));
    return;
  }
  blank_bottombars();
  currmenu = MLINTER;
  statusbar(_("Invoking linter..."));
  /* Fork a process to run the linter in. */
  if ((pid_lint = fork()) == 0) {
    char **lintargs = NULL;
    /* Redirect standard output and standard error into the pipe. */
    if (dup2(lint_fd[1], STDOUT_FILENO) < 0) {
      exit(7);
    }
    if (dup2(lint_fd[1], STDERR_FILENO) < 0) {
      exit(8);
    }
    close(lint_fd[0]);
    close(lint_fd[1]);
    construct_argument_list(&lintargs, openfile->syntax->linter, openfile->filename);
    /* Start the linter program; we are using $PATH. */
    execvp(lintargs[0], lintargs);
    /* This is only reached when the linter is not found. */
    exit(9);
  }
  /* Parent continues here. */
  close(lint_fd[1]);
  /* If the child process was not forked successfully... */
  if (pid_lint < 0) {
    statusline(ALERT, _("Could not fork: %s"), strerror(errno));
    close(lint_fd[0]);
    return;
  }
  /* Get the system pipe buffer size. */
  pipesize = fpathconf(lint_fd[0], _PC_PIPE_BUF);
  if (pipesize < 1) {
    statusline(ALERT, _("Could not get size of pipe buffer"));
    close(lint_fd[0]);
    return;
  }
  /* Block resizing signals while reading from the pipe. */
  block_sigwinch(TRUE);
  /* Read in the returned syntax errors. */
  totalread  = 0;
  buffersize = (pipesize + 1);
  lintings   = (char *)nmalloc(buffersize);
  pointer    = lintings;
  while ((bytesread = read(lint_fd[0], pointer, pipesize)) > 0) {
    totalread += bytesread;
    buffersize += pipesize;
    lintings = (char *)nrealloc(lintings, buffersize);
    pointer  = lintings + totalread;
  }
  *pointer = '\0';
  close(lint_fd[0]);
  block_sigwinch(FALSE);
  pointer = lintings;
  onelint = lintings;
  /* Now parse the output of the linter. */
  while (*pointer) {
    if ((*pointer == '\r') || (*pointer == '\n')) {
      *pointer = '\0';
      if (onelint != pointer) {
        char *filename, *linestring, *colstring;
        char *complaint = copy_of(onelint);
        char *spacer    = strstr(complaint, " ");
        /* The recognized format is "filename:line:column: message", where ":column" may be absent or be ",column" instead. */
        if ((filename = strtok(onelint, ":")) && spacer) {
          if ((linestring = strtok(NULL, ":"))) {
            if ((colstring = strtok(NULL, " "))) {
              long linenumber = strtol(linestring, NULL, 10);
              long colnumber  = strtol(colstring, NULL, 10);
              if (linenumber <= 0) {
                free(complaint);
                ++pointer;
                continue;
              }
              if (colnumber <= 0) {
                colnumber = 1;
                strtok(linestring, ",");
                if ((colstring = strtok(NULL, ","))) {
                  colnumber = strtol(colstring, NULL, 10);
                }
              }
              parsesuccess  = TRUE;
              tmplint       = curlint;
              curlint       = (lintstruct *)nmalloc(sizeof(lintstruct));
              curlint->next = NULL;
              curlint->prev = tmplint;
              if (curlint->prev != NULL) {
                curlint->prev->next = curlint;
              }
              curlint->filename = copy_of(filename);
              curlint->lineno   = linenumber;
              curlint->colno    = colnumber;
              curlint->msg      = copy_of(spacer + 1);
              if (lints == NULL) {
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
    statusline(ALERT, _("Error invoking '%s'"), openfile->syntax->linter);
    return;
  }
  if (!parsesuccess) {
    statusline(REMARK, _("Got 0 parsable lines from command: %s"), openfile->syntax->linter);
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
  while (TRUE) {
    int kbinput;
    functionptrtype function;
    struct stat lintfileinfo;
    if (stat(curlint->filename, &lintfileinfo) != -1 && (!openfile->statinfo || openfile->statinfo->st_ino != lintfileinfo.st_ino)) {
      const openfilestruct *started_at = openfile;
      openfile = openfile->next;
      while (openfile != started_at && (!openfile->statinfo || openfile->statinfo->st_ino != lintfileinfo.st_ino)) {
        openfile = openfile->next;
      }
      if (!openfile->statinfo || openfile->statinfo->st_ino != lintfileinfo.st_ino) {
        char *msg = (char *)nmalloc(1024 + strlen(curlint->filename));
        sprintf(msg, _("This message is for unopened file %s, open it in a new buffer?"), curlint->filename);
        int choice = ask_user(YESORNO, msg);
        free(msg);
        currmenu = MLINTER;
        if (choice == CANCEL) {
          statusbar(_("Cancelled"));
          break;
        }
        else if (choice == YES) {
          open_buffer(curlint->filename, TRUE);
        }
        else {
          char *dontwantfile = copy_of(curlint->filename);
          lintstruct *restlint = NULL;
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
              curlint = curlint->next;
              free(tmplint->msg);
              free(tmplint->filename);
              free(tmplint);
            }
            else {
              if (!restlint) {
                restlint = curlint;
              }
              curlint = curlint->next;
            }
          }
          free(dontwantfile);
          if (!restlint) {
            statusline(REMARK, _("No messages for this file"));
            break;
          }
          else {
            curlint = restlint;
            continue;
          }
        }
      }
    }
    if (tmplint != curlint) {
      /* Put the cursor at the reported position, but don't go beyond EOL
       * when the second number is a column number instead of an index. */
      goto_line_posx(curlint->lineno, (curlint->colno - 1));
      openfile->current_x = actual_x(openfile->current->data, openfile->placewewant);
      titlebar(NULL);
      adjust_viewport(CENTERING);
      confirm_margin();
      edit_refresh();
      statusline(NOTICE, "%s", curlint->msg);
      bottombars(MLINTER);
    }
    /* Place the cursor to indicate the affected line. */
    place_the_cursor();
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
      if (curlint->prev != NULL) {
        curlint = curlint->prev;
      }
      else if (last_wait != time(NULL)) {
        statusbar(_("At first message"));
        beep();
        napms(600);
        last_wait = time(NULL);
        statusline(NOTICE, "%s", curlint->msg);
      }
    }
    else if (function == do_page_down || function == to_next_block) {
      if (curlint->next) {
        curlint = curlint->next;
      }
      else if (last_wait != time(NULL)) {
        statusbar(_("At last message"));
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
  for (curlint = lints; curlint;) {
    tmplint = curlint;
    curlint = curlint->next;
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

/* Run a manipulation program on the contents of the buffer. */
void do_formatter(void) {
  FILE *stream;
  char *temp_name;
  bool  okay = FALSE;
  ran_a_tool = TRUE;
  if (in_restricted_mode()) {
    return;
  }
  if (!openfile->syntax || !openfile->syntax->formatter || !*openfile->syntax->formatter) {
    statusline(AHEM, _("No formatter is defined for this type of file"));
    return;
  }
  openfile->mark = NULL;
  temp_name = safe_tempfile(&stream);
  if (temp_name) {
    okay = write_file(temp_name, stream, TEMPORARY, OVERWRITE, NONOTES);
  }
  if (!okay) {
    statusline(ALERT, _("Error writing temp file: %s"), strerror(errno));
  }
  else {
    treat(temp_name, openfile->syntax->formatter, FALSE);
  }
  unlink(temp_name);
  free(temp_name);
}

/* Our own version of "wc".  Note that the character count is in multibyte characters instead of single-byte characters. */
void count_lines_words_and_characters(void) {
  linestruct *was_current = openfile->current;
  Ulong       was_x       = openfile->current_x;
  linestruct *topline, *botline;
  Ulong       top_x, bot_x;
  Ulong       words = 0, chars = 0;
  long        lines = 0;
  /* Set the start and end point of the area to measure: either the marked
   * region or the whole buffer.  Then compute the number of characters. */
  if (openfile->mark) {
    get_region(&topline, &top_x, &botline, &bot_x);
    if (topline != botline) {
      chars = (number_of_characters_in(topline->next, botline) + 1);
    }
    chars += (mbstrlen(topline->data + top_x) - mbstrlen(botline->data + bot_x));
  }
  else {
    topline = openfile->filetop;
    top_x   = 0;
    botline = openfile->filebot;
    bot_x   = strlen(botline->data);
    chars   = openfile->totsize;
  }
  /* Compute the number of lines. */
  lines = botline->lineno - topline->lineno;
  lines += (bot_x == 0 || (topline == botline && top_x == bot_x)) ? 0 : 1;
  openfile->current   = topline;
  openfile->current_x = top_x;
  /* Keep stepping to the next word (considering punctuation as part of a word, as "wc -w" does),
   * until we reach the end of the relevant area, incrementing the word count for each successful step. */
  while (openfile->current->lineno < botline->lineno || (openfile->current == botline && openfile->current_x < bot_x)) {
    if (do_next_word(FALSE)) {
      words++;
    }
  }
  /* Restore where we were. */
  openfile->current   = was_current;
  openfile->current_x = was_x;
  /* Report on the status bar the number of lines, words, and characters. */
  statusline(INFO, _("%s%zd %s,  %zu %s,  %zu %s"), openfile->mark ? _("In Selection:  ") : "", lines,
             P_("line", "lines", lines), words, P_("word", "words", words), chars,
             P_("character", "characters", chars));
}

/* Get verbatim input.  This is used to insert a Unicode character by its hexadecimal code, which is typed
 * in by the user.  The function returns the bytes that were typed in, and the number of bytes that were read. */
void do_verbatim_input(void) {
  Ulong count = 1;
  char *bytes;
  /* When barless and with cursor on bottom row, make room for the feedback. */
  if (ISSET(ZERO) && openfile->cursor_row == (editwinrows - 1) && LINES > 1) {
    edit_scroll(FORWARD);
    edit_refresh();
  }
  /* TRANSLATORS : Shown when the next keystroke will be inserted verbatim. */
  statusline(INFO, _("Verbatim Input"));
  place_the_cursor();
  /* Read in the first one or two bytes of the next keystroke. */
  bytes = get_verbatim_kbinput(midwin, &count);
  /* When something valid was obtained, unsuppress cursor-position display,
   * insert the bytes into the edit buffer, and blank the status bar. */
  if (count > 0) {
    if (ISSET(CONSTANT_SHOW) || ISSET(MINIBAR)) {
      lastmessage = VACUUM;
    }
    if (count < 999) {
      inject(bytes, count);
    }
    /* Ensure that the feedback will be overwritten, or clear it. */
    if (ISSET(ZERO) && currmenu == MMAIN) {
      wredrawln(midwin, editwinrows - 1, 1);
    }
    else {
      wipe_statusbar();
    }
  }
  else {
    /* TRANSLATORS: An invalid verbatim Unicode code was typed. */
    statusline(AHEM, _("Invalid code"));
  }
  free(bytes);
}

/* Return a copy of the found completion candidate. */
char *copy_completion(char *text) {
  char *word;
  Ulong length = 0, index = 0;
  /* Find the end of the candidate word to get its length. */
  while (is_word_char(&text[length], FALSE)) {
    length = step_right(text, length);
  }
  /* Now copy this candidate to a new string. */
  word = (char *)nmalloc(length + 1);
  while (index < length) {
    word[index++] = *(text++);
  }
  word[index] = '\0';
  return word;
}

/* Look at the fragment the user has typed, then search all buffers for the first word that starts with this fragment,
 * and tentatively complete the fragment.  If the user hits 'Complete' again, search and paste the next possible completion. */
void complete_a_word(void) {
  /* The buffer that is being searched for possible completions. */
  static openfilestruct *scouring = NULL;
  /* A linked list of the completions that have been attempted. */
  static completionstruct *list_of_completions;
  /* The x position in `pletion_line` of the last found completion. */
  static int pletion_x        = 0;
  bool       was_set_wrapping = ISSET(BREAK_LONG_LINES);
  Ulong      start_of_shard;
  Ulong      shard_length = 0;
  char      *shard;
  /* If this is a fresh completion attempt... */
  if (!pletion_line) {
    /* Clear the list of words of a previous completion run. */
    while (list_of_completions) {
      completionstruct *dropit = list_of_completions;
      list_of_completions      = list_of_completions->next;
      free(dropit->word);
      free(dropit);
    }
    /* Prevent a completion from being merged with typed text. */
    openfile->last_action = OTHER;
    /* Initialize the starting point for searching. */
    scouring     = openfile;
    pletion_line = openfile->filetop;
    pletion_x    = 0;
    /* Wipe the "No further matches" message. */
    wipe_statusbar();
  }
  else {
    /* Remove the attempted completion from the buffer. */
    do_undo();
  }
  /* Find the start of the fragment that the user typed. */
  start_of_shard = openfile->current_x;
  while (start_of_shard > 0) {
    Ulong oneleft = step_left(openfile->current->data, start_of_shard);
    if (!is_word_char(&openfile->current->data[oneleft], FALSE)) {
      break;
    }
    start_of_shard = oneleft;
  }
  /* If there is no word fragment before the cursor, do nothing. */
  if (start_of_shard == openfile->current_x) {
    /* TRANSLATORS: Shown when no text is directly left of the cursor. */
    statusline(AHEM, _("No word fragment"));
    pletion_line = NULL;
    return;
  }
  shard = (char *)nmalloc(openfile->current_x - start_of_shard + 1);
  /* Copy the fragment that has to be searched for. */
  while (start_of_shard < openfile->current_x) {
    shard[shard_length++] = openfile->current->data[start_of_shard++];
  }
  shard[shard_length] = '\0';
  /* Run through all of the lines in the buffer, looking for shard. */
  while (pletion_line) {
    /* The point where we can stop searching for shard. */
    long              threshold = (strlen(pletion_line->data) - shard_length - 1);
    completionstruct *some_word;
    char             *completion;
    Ulong             i, j;
    /* Traverse the whole line, looking for shard. */
    for (i = pletion_x; (long)i < threshold; i++) {
      /* If the first byte doesn't match, run on. */
      if (pletion_line->data[i] != shard[0]) {
        continue;
      }
      /* Compare the rest of the bytes in shard. */
      for (j = 1; j < shard_length; j++) {
        if (pletion_line->data[i + j] != shard[j]) {
          break;
        }
      }
      /* If not all of the bytes matched, continue searching. */
      if (j < shard_length) {
        continue;
      }
      /* If the found match is not /longer/ than shard, skip it. */
      if (!is_word_char(&pletion_line->data[i + j], FALSE)) {
        continue;
      }
      /* If the match is not a separate word, skip it. */
      if (i > 0 && is_word_char(&pletion_line->data[step_left(pletion_line->data, i)], FALSE)) {
        continue;
      }
      /* If this match is the shard itself, ignore it. */
      if (pletion_line == openfile->current && i == openfile->current_x - shard_length) {
        continue;
      }
      completion = copy_completion(pletion_line->data + i);
      /* Look among earlier attempted completions for a duplicate. */
      some_word = list_of_completions;
      while (some_word && strcmp(some_word->word, completion) != 0) {
        some_word = some_word->next;
      }
      /* If we've already tried this word, skip it. */
      if (some_word != NULL) {
        free(completion);
        continue;
      }
      /* Add the found word to the list of completions. */
      some_word           = (completionstruct *)nmalloc(sizeof(completionstruct));
      some_word->word     = completion;
      some_word->next     = list_of_completions;
      list_of_completions = some_word;
      /* Temporarily disable wrapping so only one undo item is added. */
      UNSET(BREAK_LONG_LINES);
      /* Inject the completion into the buffer. */
      inject(&completion[shard_length], strlen(completion) - shard_length);
      /* If needed, reenable wrapping and wrap the current line. */
      if (was_set_wrapping) {
        SET(BREAK_LONG_LINES);
        do_wrap();
      }
      /* Set the position for a possible next search attempt. */
      pletion_x = ++i;
      free(shard);
      return;
    }
    pletion_line = pletion_line->next;
    pletion_x    = 0;
    /* When at end of buffer and there is another, search that one. */
    if (pletion_line == NULL && scouring->next != openfile) {
      scouring     = scouring->next;
      pletion_line = scouring->filetop;
    }
  }
  /* The search has gone through all buffers. */
  if (list_of_completions) {
    edit_refresh();
    statusline(AHEM, _("No further matches"));
  }
  else {
    /* TRANSLATORS: Shown when there are zero possible completions. */
    statusline(AHEM, _("No matches"));
  }
  free(shard);
}

/* Return`s a all lower case str of 'str'. */
char *lower_case_word(const char *str) {
  char *ret = (char *)nmalloc(strlen(str + 1));
  const char *copy = str;
  Uint i = 0;
  for (; *copy; ++copy, ++i) {
    ret[i] = *copy;
    if (*copy >= 'A' && *copy <= 'Z') {
      ret[i] += 32;
    }
  }
  ret[i] = '\0';
  return ret;
}
