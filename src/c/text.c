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
      /* Fall-through. */
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
    default : {
      die("Bad undo type -- please report a bug\n");
    }
  }
  file->last_action = action;
}

/* Add a new undo item of the given type to the top of the current pile for the currently open file.  Works for gui and tui context. */
void add_undo(undo_type action, const char *const restrict message) {
  add_undo_for((ISSET(USING_GUI) ? openeditor->openfile : openfile), action, message);
}

/* Update an undo item with (among other things) the file size and cursor position after the given action. */
void update_undo_for(openfilestruct *const restrict file, undo_type action) {
  ASSERT(file);
  undostruct *u = file->undotop;
  Ulong datalen;
  Ulong newlen;
  char *textposition;
  int charlen;
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
  update_undo_for(CONTEXT_OPENFILE, action);
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
  update_multiline_undo_for(CONTEXT_OPENFILE, lineno, indentation);
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
  do_mark_for(ISSET(USING_GUI) ? openeditor->openfile : openfile);
}

/* Discard `undo-items` that are newer then `thisitem` in `buffer`, or all if `thisitem` is `NULL`. */
void discard_until_for(openfilestruct *const buffer, const undostruct *const thisitem) {
  undostruct  *dropit = buffer->undotop;
  groupstruct *group, *next;
  while (dropit && dropit != thisitem) {
    buffer->undotop = dropit->next;
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
    dropit = buffer->undotop;
  }
  /* Adjust the pointer to the top of the undo struct. */
  buffer->current_undo = (undostruct *)thisitem;
  /* Prevent a chain of edition actions from continuing. */
  buffer->last_action = OTHER;
}

/* Discard undo items that are newer than the given one, or all if NULL. */
void discard_until(const undostruct *thisitem) {
  discard_until_for(openfile, thisitem);
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

/* Insert a new empty line, either `above` or `below` `line`.  */
void insert_empty_line(linestruct *line, bool above, bool autoindent) {
  linestruct *topline, *newline;
  if (above) {
    if (!line->prev) {
      newline = make_new_node(NULL);
      newline->next = line;
      line->prev = newline;
      if (line == openfile->filetop) {
        openfile->filetop = newline;
      }
      if (line == openfile->edittop) {
        openfile->edittop = newline;
      }
    }
    else {
      topline = line->prev;
      newline = make_new_node(topline);
      splice_node(topline, newline);
    }
  }
  else {
    topline = line;
    newline = make_new_node(topline);
    splice_node(topline, newline);
  }
  renumber_from(newline);
  if (!autoindent) {
    newline->data = COPY_OF("");
  }
  else {
    newline->data = measured_copy(line->data, indent_length(line->data));
  }
}

/* This is a shortcut to make marked area a block comment. */
void do_block_comment(void) {
  enclose_marked_region("/* ", " */");
  keep_mark = TRUE;
}

/* ----------------------------- Cursor is between brackets ----------------------------- */

/* Returns `TRUE` when `file->current->data[file->current_x]` is between a bracket pair, `{}`, `[]` or `()`. */
bool cursor_is_between_brackets_for(openfilestruct *const file) {
  return is_curs_between_any_pair_for(file, (const char *[]){"{}", "[]", "()", NULL}, NULL);
}

/* Returns `TRUE` when `openfile->current->data[openfile->current_x]` is between a bracket pair,
 * `{}`, `[]` or `()`.  Note that this is context safe and can be called from the `tui` or `gui`. */
bool cursor_is_between_brackets(void) {
  return cursor_is_between_brackets_for(ISSET(USING_GUI) ? openeditor->openfile : openfile);
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
  indent_a_line_for((ISSET(USING_GUI) ? openeditor->openfile : openfile), line, indentation);
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
  }
  /* Calculate the new total number of chars. */
  file->totsize += (mbstrlen(p1) + mbstrlen(p2));
  file->undotop->newsize = file->totsize;
  /* Set the `modified` flag in `file`. */
  set_modified_for(file);
  refresh_needed = TRUE;
}

/* If the currently open file currently has a marked region, enclose that region where `p1` will
 * be placed on the top/start and `p2` at the bottom/end of the marked region. */
void enclose_marked_region(const char *const restrict p1, const char *const restrict p2) {
  enclose_marked_region_for((ISSET(USING_GUI) ? openeditor->openfile : openfile), p1, p2);
}
