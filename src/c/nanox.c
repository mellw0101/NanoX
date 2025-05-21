/** @file nanox.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Containers for the original and the temporary handler for SIGINT. */
static struct sigaction oldaction;
static struct sigaction newaction;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Register that Ctrl+C was pressed during some system call. */
static void make_a_note(int _UNUSED signal) {
  control_C_was_pressed = TRUE;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Create a new linestruct node.  Note that we do NOT set 'prevnode->next'. */
linestruct *make_new_node(linestruct *prevnode)  {
  linestruct *newnode = xmalloc(sizeof(*newnode));
  newnode->prev       = prevnode;
  newnode->next       = NULL;
  newnode->data       = NULL;
  newnode->multidata  = NULL;
  newnode->lineno     = ((prevnode) ? (prevnode->lineno + 1) : 1);
  newnode->has_anchor = FALSE;
  // newnode->flags.clear();
  newnode->is_block_comment_start   = FALSE;
  newnode->is_block_comment_end     = FALSE;
  newnode->is_in_block_comment      = FALSE;
  newnode->is_single_block_comment  = FALSE;
  newnode->is_hidden                = FALSE;
  newnode->is_bracket_start         = FALSE;
  newnode->is_in_bracket            = FALSE;
  newnode->is_bracket_end           = FALSE;
  newnode->is_function_open_bracket = FALSE;
  newnode->is_dont_preprocess_line  = FALSE;
  newnode->is_pp_line               = FALSE;
  if (prevnode) {
    (prevnode->is_in_block_comment || prevnode->is_block_comment_start) ? (newnode->is_in_block_comment = TRUE) : ((int)0);
    (prevnode->is_in_bracket || prevnode->is_bracket_start) ? (newnode->is_in_bracket = TRUE) : ((int)0);
    // (prevnode->flags.is_set(IN_BLOCK_COMMENT) || prevnode->flags.is_set(BLOCK_COMMENT_START))
    //   ? newnode->flags.set(IN_BLOCK_COMMENT) : newnode->flags.unset(IN_BLOCK_COMMENT);
    // (prevnode->flags.is_set(IN_BRACKET) || prevnode->flags.is_set(BRACKET_START))
    //   ? newnode->flags.set(IN_BRACKET) : (void)0;
  }
  return newnode;
}

/* Splice a new node into an existing linked list of linestructs. */
void splice_node(linestruct *afterthis, linestruct *newnode) {
  newnode->next = afterthis->next;
  newnode->prev = afterthis;
  if (afterthis->next) {
    afterthis->next->prev = newnode;
  }
  afterthis->next = newnode;
  /* Update filebot when inserting a node at the end of file. */
  if (openfile && openfile->filebot == afterthis) {
    openfile->filebot = newnode;
  }
}

/* Free the data structures in the given node */
void delete_node(linestruct *line) {
  /* If the first line on the screen gets deleted, step one back. */
  if (line == openfile->edittop) {
    openfile->edittop = line->prev;
  }
  /* If the spill-over line for hard-wrapping is deleted... */
  if (line == openfile->spillage_line) {
    openfile->spillage_line = NULL;
  }
  free(line->data);
  free(line->multidata);
  free(line);
}

/* Disconnect a node from a linked list of linestructs and delete it. */
void unlink_node(linestruct *line) {
  if (line->prev) {
    line->prev->next = line->next;
  }
  if (line->next) {
    line->next->prev = line->prev;
  }
  /* Update filebot when removing a node at the end of file. */
  if (openfile && openfile->filebot == line) {
    openfile->filebot = line->prev;
  }
  delete_node(line);
}

/* Free an entire linked list of linestructs. */
void free_lines(linestruct *src) {
  if (!src) {
    return;
  }
  while (src->next) {
    src = src->next;
    delete_node(src->prev);
  }
  delete_node(src);
}

/* Make a copy of a linestruct node. */
linestruct *copy_node(const linestruct *src) {
  linestruct *dst = xmalloc(sizeof(*dst));
  dst->data       = copy_of(src->data);
  dst->multidata  = NULL;
  dst->lineno     = src->lineno;
  dst->has_anchor = src->has_anchor;
  return dst;
}

/* Duplicate an entire linked list of linestructs. */
linestruct *copy_buffer(const linestruct *src) {
  linestruct *head, *item;
  head       = copy_node(src);
  head->prev = NULL;
  item       = head;
  src        = src->next;
  while (src) {
    item->next       = copy_node(src);
    item->next->prev = item;
    item             = item->next;
    src              = src->next;
  }
  item->next = NULL;
  return head;
}

/* Renumber the lines in a buffer, from the given line onwards. */
void renumber_from(linestruct *line) {
  long number = (!line->prev ? 0 : line->prev->lineno);
  while (line) {
    line->lineno = ++number;
    line = line->next;
  }
}

/* Display a warning about a key disabled in view mode. */
void print_view_warning(void) {
  print_status(AHEM, _("Key is invalid in view mode"));
}

/* When in restricted mode, show a warning and return 'TRUE'. */
bool in_restricted_mode(void) {
  if (ISSET(RESTRICTED)) {
    print_status(AHEM, _("This function is disabled in restricted mode"));
    beep();
    return TRUE;
  }
  return FALSE;
}

void confirm_margin_for(openfilestruct *const file, int *const out_margin) {
  ASSERT(file);
  bool keep_focus;
  int needed_margin = (digits(file->filebot->lineno) + 1);
  /* When not requested, supress line numbers. */
  if (!ISSET(LINE_NUMBERS)) {
    needed_margin = 0;
  }
  if (needed_margin != (*out_margin)) {
    keep_focus    = (((*out_margin) > 0) && focusing);
    (*out_margin) = needed_margin;
    /* Ensure a proper starting column for the first screen row. */
    ensure_firstcolumn_is_aligned_for(file);
    focusing = keep_focus;
    refresh_needed = TRUE;
  }
}

/* Ensure that the margin can accommodate the buffer's highest line number. */
void confirm_margin(void) {
  bool keep_focus;
  int needed_margin = (digits(openfile->filebot->lineno) + 1);
  /* When not requested or space is too tight, suppress line numbers. */
  if (!ISSET(LINE_NUMBERS) || needed_margin > (COLS - 4)) {
    needed_margin = 0;
  }
  if (needed_margin != margin) {
    keep_focus  = ((margin > 0) && focusing);
    margin      = needed_margin;
    editwincols = (COLS - margin - sidebar);
    /* Ensure a proper starting column for the first screen row. */
    ensure_firstcolumn_is_aligned();
    focusing = keep_focus;
    /* The margin has changed -- schedule a full refresh. */
    refresh_needed = TRUE;
  }
}

/* Stop ^C from generating a SIGINT. */
void disable_kb_interrupt(void) {
  struct termios settings = {0};
  tcgetattr(0, &settings);
  settings.c_lflag &= ~ISIG;
  tcsetattr(0, TCSANOW, &settings);
}

/* Make ^C generate a SIGINT. */
void enable_kb_interrupt(void) {
  struct termios settings = {0};
  tcgetattr(0, &settings);
  settings.c_lflag |= ISIG;
  tcsetattr(0, TCSANOW, &settings);
}

/* Make ^C interrupt a system call and set a flag. */
void install_handler_for_Ctrl_C(void) {
  /* Enable the generation of a SIGINT when ^C is pressed. */
  enable_kb_interrupt();
  /* Set up a signal handler so that pressing ^C will set a flag. */
  newaction.sa_handler = make_a_note;
  newaction.sa_flags   = 0;
  sigaction(SIGINT, &newaction, &oldaction);
}

/* Go back to ignoring ^C. */
void restore_handler_for_Ctrl_C(void) {
  sigaction(SIGINT, &oldaction, NULL);
  disable_kb_interrupt();
}

/* ----------------------------- Curses ----------------------------- */

/* Initialize the three window portions nano uses.  Ncurses verion. */
void window_init_curses(void) {
  int min;
  int toprows;
  int bottomrows;
  if (midwin) {
    if (topwin) {
      delwin(topwin);
    }
    delwin(midwin);
    delwin(footwin);
  }
  topwin = NULL;
  /* If the terminal is very flat, don't set up a title bar. */
  if (LINES < 3) {
    editwinrows = (ISSET(ZERO) ? LINES : 1);
    /* Set up two subwindows.  If the terminal is just one line, edit window and status-bar window will cover each other. */
    midwin  = newwin(editwinrows, COLS, 0, 0);
    footwin = newwin(1, COLS, (LINES - 1), 0);
  }
  else {
    min        = (ISSET(ZERO) ? 3 : (ISSET(MINIBAR) ? 4 : 5));
    toprows    = ((ISSET(EMPTY_LINE) && LINES > min) ? 2 : 1);
    bottomrows = ((ISSET(NO_HELP) || LINES < min) ? 1 : 3);
    if (ISSET(MINIBAR) || ISSET(ZERO)) {
      toprows = 0;
    }
    editwinrows = (LINES - toprows - bottomrows + (ISSET(ZERO) ? 1 : 0));
    /* Set up the normal three subwindow's. */
    if (toprows > 0) {
      topwin = newwin(toprows, COLS, 0, 0);
    }
    midwin  = newwin(editwinrows, COLS, toprows, 0);
    footwin = newwin(bottomrows, COLS, (LINES - bottomrows), 0);
  }
  /* In case the terminal shrunk, make sure the status line is clear. */
  wnoutrefresh(footwin);
  /* When not disabled, turn escape-sequence translation on. */
  if (!ISSET(RAW_SEQUENCES)) {
    keypad(midwin, TRUE);
    keypad(footwin, TRUE);
  }
  /* Set up the wrapping point, accounting for the screen width when negative. */
  if ((COLS + fill) < 0) {
    wrap_at = 0;
  }
  else if (fill <= 0) {
    wrap_at = (COLS + fill);
  }
  else {
    wrap_at = fill;
  }
}
