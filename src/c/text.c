/** @file text.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


/* The maximum depth of recursion.  Note that this MUST be an even number. */
#define RECURSION_LIMIT 222


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


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Insert a tab.  Or if `TABS_TO_SPACES/--tabstospaces` is in effect, insert the number of spaces that a tab would normally take up at this position. */
void do_tab_for(openfilestruct *const file, int rows, int cols) {
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
  else if (ISSET(TABS_TO_SPACES)) {
    spaces = tab_space_string_for(file, &len);
    inject_into_buffer(STACK_CTX, spaces, len);
    free(spaces);
  }
  else {
    inject_into_buffer(STACK_CTX, "\t", 1);
  }
}

/* Insert a tab.  Or if `TABS_TO_SPACES/--tabstospaces` is in effect, insert the number of spaces that a tab would normally take up at this position. */
void do_tab(void) {
  if (IN_GUI_CTX) {
    do_tab_for(GUI_CTX);
  }
  else {
    do_tab_for(TUI_CTX);
  }
}

/* Return's the length of whilespace until first non blank char in `string`. */
Ulong indentlen(const char *const restrict string) {
  ASSERT(string);
  const char *ptr = string;
  while (is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return (ptr - string);
}

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
    go_forward_chunks_for(file, 1, &file->edittop, &file->firstcolumn, cols);
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
  // file->placewewant = xplustabs();
  set_pww_for(file);
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

/* Return TRUE when the given line is part of a paragraph.  A line is part of a
 * paragraph if it contains something more than quoting and leading whitespace. */
bool inpar(const linestruct *const line) {
  Ulong quot_len   = quote_length(line->data);
  Ulong indent_len = indent_length(line->data + quot_len);
  return (line->data[quot_len + indent_len]);
}

/* This is a shortcut to make marked area a block comment. */
void do_block_comment(void) {
  enclose_marked_region("/* ", " */");
  keep_mark = TRUE;
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
    else if ((long)(++white_count) == tabsize) {
      return tabsize;
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

/* ----------------------------- Unindent ----------------------------- */

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

/* Insert a new empty line above `file->current`, and add an undo-item to the undo-stack. */
void do_insert_empty_line_above_for(openfilestruct *const file) {
  ASSERT(file);
  add_undo_for(file, INSERT_EMPTY_LINE, NULL);
  insert_empty_line_for(file, file->current, TRUE, TRUE);
  DLIST_ADV_PREV(file->current);
  update_undo_for(file, INSERT_EMPTY_LINE);
  set_cursor_to_eol_for(file);
  refresh_needed = TRUE;
}

/* Insert a new empty line above `openfile->current`, and add an undo-item to the
 * `undo-stack`. Note that this is context safe and works in both the `gui` and `tui`. */
void do_insert_empty_line_above(void) {
  do_insert_empty_line_above_for(CTX_OF);
}

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

/* ----------------------------- Indent ----------------------------- */

/* Return the length of the indentation part of the given line.  The "indentation" of a line is the leading consecutive whitespace. */
Ulong indent_length(const char *const restrict line) {
  const char *ptr = line;
  while (is_blank_char(ptr)) {
    ptr += char_length(ptr);
  }
  return (ptr - line);
}

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
  while (top != bot->next && !top->data[0]) {
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

/* ----------------------------- Enclose marked region ----------------------------- */

/* Returns an allocated string containg `p1` `ENCLOSE_DELIM` `p2`. */
char *enclose_str_encode(const char *const restrict p1, const char *const restrict p2) {
  ASSERT(p1);
  ASSERT(p2);
  return fmtstr("%s" ENCLOSE_DELIM "%s", p1, p2);
}

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
  *p2 = copy_of(enclose_delim + STRLEN(ENCLOSE_DELIM));
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
  /* Number of chars to next tabstop when `TABS_TO_SPACES` is set. */
  Ulong tablen;
  Ulong lenleft;
  linestruct *middle = make_new_node(line);
  linestruct *end    = make_new_node(middle);
  splice_node_for(file, line, middle);
  splice_node_for(file, middle, end);
  renumber_from(middle);
  indentlen = indent_length(line->data);
  tablen    = tabstop_length(line->data, indentlen);
  lenleft   = strlen(line->data + posx);
  middle->data = xmalloc(indentlen + tablen + 1);
  end->data    = xmalloc(indentlen + lenleft + 1);
  /* Set up the middle line. */
  memcpy(middle->data, line->data, indentlen);
  if (ISSET(TABS_TO_SPACES)) {
    memset((middle->data + indentlen), ' ', tablen);
    middle->data[indentlen + tablen] = '\0';
    file->totsize += tablen;
  }
  else {
    middle->data[indentlen]     = '\t';
    middle->data[indentlen + 1] = '\0';
    ++file->totsize;
  }
  /* Set up end line. */
  memcpy((end->data + indentlen), (line->data + posx), (lenleft + 1));
  memcpy(end->data, line->data, indentlen);
  /* Set up start line. */
  line->data       = xrealloc(line->data, (posx + 1));
  line->data[posx] = '\0';
  /* Set the cursor line and x pos to the middle line. */
  file->current   = middle; 
  file->current_x = (indentlen + tablen);
  set_pww_for(file);
  refresh_needed = TRUE;
}

/* Auto insert a empty line between '{' and '}', as well as indenting the line once and setting openfile->current to it. */
void auto_bracket(linestruct *const line, Ulong posx) {
  auto_bracket_for(CTX_OF, line, posx);
}

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

/* ----------------------------- Comment ----------------------------- */

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
      set_pww_for(file);
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

/* Comment or uncomment the current line or the marked lines. */
void do_comment_for(openfilestruct *const file, int total_cols) {
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
    if (comment_line_for(file, action, line, comment_seq)) {
      update_multiline_undo_for(file, line->lineno, _(""));
    }
  }
  set_modified_for(file);
  ensure_firstcolumn_is_aligned_for(file, total_cols);
  refresh_needed = TRUE;
  shift_held     = TRUE;
}

/* Comment or uncomment the current line or the marked lines. */
void do_comment(void) {
  if (IN_GUI_CTX) {
    do_comment_for(openeditor->openfile, openeditor->cols);
  }
  else {
    do_comment_for(openfile, editwincols);
  }
}

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
  Ulong extra = 0;
  /* Check if cursor is between two brackets. */
  if (cursor_is_between_brackets_for(file)) {
    do_auto_bracket_for(file);
    return;
  }
  /* If the prev char is one of the usual section start chars, or a lable, we should indent once more. */
  do_another_indent = is_prev_cursor_char_one_of_for(file, "{:");
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
  set_pww_for(file);
  set_modified_for(file);
  if (ISSET(AUTOINDENT) && !allblanks) {
    file->totsize += extra;
  }
  /* When approptiet, add another indent. */
  if (do_another_indent) {
    /* If the indent of the new line is the same as the line we came from.  I.E: Autoindent is turned on. */
    if (line_indent(file->current) == line_indent(file->current->prev)) {
      file->current->data = fmtstrcat(file->current->data, "%*s", (int)TAB_BYTE_LEN, (ISSET(TABS_TO_SPACES) ? " " : "\t"));
      file->current_x    += TAB_BYTE_LEN;
      file->totsize      += TAB_BYTE_LEN;
    }
  }
  update_undo_for(file, ENTER);
  refresh_needed = TRUE;
  focusing       = FALSE;
}

/* Break the current line at the cursor position. */
void do_enter(void) {
  do_enter_for(CTX_OF);
}
