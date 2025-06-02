/** @file nanox.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Containers for the original and the temporary handler for SIGINT. */
static struct sigaction oldaction;
static struct sigaction newaction;

/* The original settings of the user's terminal. */
struct termios original_state;


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

/* ----------------------------- Splice node ----------------------------- */

/* Splice a new node into an existing linked list of linestructs for `file`, or `NULL` when lines are not related to an `openfilestruct *`. */
void splice_node_for(openfilestruct *const file, linestruct *const after, linestruct *const node) {
  ASSERT(after);
  ASSERT(node);
  DLIST_INSERT_AFTER(after, node);
  /* Update filebot when inserting a node at the end of `file`. */
  if (file && file->filebot == after) {
    file->filebot = node;
  }
}

/* Splice a new node into an existing linked list of linestructs. */
void splice_node(linestruct *const after, linestruct *const node) {
  splice_node_for(CONTEXT_OPENFILE, after, node);
}

/* ----------------------------- Delete node ----------------------------- */

/* Free the data structures in the given node, that is part of `file`.  TODO: Make sure always moving the edittop
 * up is wise, as what happens when its the top of the file then its `NULL` when we could just move it down. */
void delete_node_for(openfilestruct *const file, linestruct *const node) {
  ASSERT(node);
  /* Make this function safe for lines not tied to a file. */
  if (file) {
    /* If the file's first line on the screen gets deleted, step one back. */
    if (node == file->edittop) {
      DLIST_ADV_PREV(file->edittop);
    }
    /* If the file's `spill-over` line for hard-wrapping is deleted... */
    if (node == file->spillage_line) {
      file->spillage_line = NULL;
    }
  }
  /* Free the node's internal data. */
  free(node->data);
  free(node->multidata);
  /* Free the node itself. */
  free(node);
}

/* Free the data structures in the given node. */
void delete_node(linestruct *const line) {
  delete_node_for(CONTEXT_OPENFILE, line);
}

/* ----------------------------- Unlink node ----------------------------- */

/* Disconnect a node from a linked list of linestructs and delete it. */
void unlink_node_for(openfilestruct *const file, linestruct *const node) {
  ASSERT(node);
  DLIST_UNLINK(node);
  /* Update filebot when removing a node at the end of file. */
  if (file && file->filebot == node) {
    DLIST_ADV_PREV(file->filebot);
  }
  delete_node_for(file, node);
}

/* Disconnect a node from a linked list of linestructs and delete it. */
void unlink_node(linestruct *const node) {
  unlink_node_for(CONTEXT_OPENFILE, node);
}

/* ---------------------------------------------------------- */

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
  statusline(AHEM, _("Key is invalid in view mode"));
}

/* When in restricted mode, show a warning and return 'TRUE'. */
bool in_restricted_mode(void) {
  if (ISSET(RESTRICTED)) {
    statusline(AHEM, _("This function is disabled in restricted mode"));
    /* Only use `beep()`, when using the ncurses context. */
    if (!ISSET(NO_NCURSES)) {
      beep();
    }
    return TRUE;
  }
  return FALSE;
}

/* Disable the terminal's XON/XOFF flow-control characters. */
void disable_flow_control(void) {
  struct termios settings;
  tcgetattr(0, &settings);
  settings.c_iflag &= ~IXON;
  tcsetattr(0, TCSANOW, &settings);
}

/* Enable the terminal's XON/XOFF flow-control characters. */
void enable_flow_control(void) {
  struct termios settings;
  tcgetattr(0, &settings);
  settings.c_iflag |= IXON;
  tcsetattr(0, TCSANOW, &settings);
}

/* Disable extended input and output processing in our terminal settings. */
void disable_extended_io(void) {
  struct termios settings = {0};
  tcgetattr(0, &settings);
  settings.c_lflag &= ~IEXTEN;
  settings.c_oflag &= ~OPOST;
  tcsetattr(0, TCSANOW, &settings);
}

// void confirm_margin_for(openfilestruct *const file, int *const out_margin, int total_cols) {
//   ASSERT(file);
//   bool keep_focus;
//   int needed_margin = (digits(file->filebot->lineno) + 1);
//   /* When not requested, supress line numbers. */
//   if (!ISSET(LINE_NUMBERS)) {
//     needed_margin = 0;
//   }
//   if (needed_margin != (*out_margin)) {
//     keep_focus    = (((*out_margin) > 0) && focusing);
//     (*out_margin) = needed_margin;
//     /* Ensure a proper starting column for the first screen row. */
//     ensure_firstcolumn_is_aligned_for(file, total_cols);
//     focusing = keep_focus;
//     refresh_needed = TRUE;
//   }
// }

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

/* Set up the terminal state.  Put the terminal in raw mode
 * (read one character at a time, disable the special control keys, and disable
 * the flow control characters), disable translation of carriage return (^M)
 * into newline (^J), so that we can tell the difference between the Enter key
 * and Ctrl-J, and disable echoing of characters as they're typed. Finally,
 * disable extended input and output processing, and, if we're not in preserve
 * mode, reenable interpretation of the flow control characters. */
void terminal_init(void) {
  /* Running in ncurses context. */
  if (!ISSET(NO_NCURSES)) {
    raw();
    nonl();
    noecho();
    disable_extended_io();
    if (ISSET(PRESERVE)) {
      enable_flow_control();
    }
    disable_kb_interrupt();
    /* Tell the terminal to enable bracketed pastes. */
    printf("\x1B[?2004h");
    fflush(stdout);
  }
  /* Running using our tui, this is relevent after we have rebuilt the tui. */
  else {
    // struct termios raw;
    // raw = original_state;
    // raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    // raw.c_oflag &= ~ONLCR;
    // raw.c_iflag &= ~(IXON | ICRNL | INLCR);
    // raw.c_iflag |= IGNBRK;
    // tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    // disable_extended_io();
    // tui_enable_bracketed_pastes();
  }
}

/* Initialize the three window portions nano uses.  For the tui. */
void window_init(void) {
  int min;
  int toprows;
  int bottomrows;
  /* When inside curses mode. */
  if (!ISSET(NO_NCURSES)) {
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
  /* Add our tui here once we have redone it. */
}

/* Reinitialize and redraw the screen completely. */
void regenerate_screen(void) {
  /* Reset the trigger. */
  the_window_resized = FALSE;
  /* Leave and immediately reenter curses mode, so that ncurses notices the new screen dimensions and sets LINES and COLS accordingly. */
  endwin();
  refresh();
  sidebar     = ((ISSET(INDICATOR) && LINES > 5 && COLS > 9) ? 1 : 0);
  bardata     = xrealloc(bardata, (LINES * sizeof(int)));
  editwincols = (COLS - margin - sidebar);
  /* Put the terminal in the desired state again, and recreate the subwindows with their (new) sizes. */
  terminal_init();
  window_init();
  /* If we have an open buffer, redraw the contents of the subwindows. */
  if (openfile) {
    ensure_firstcolumn_is_aligned();
    draw_all_subwindows();
  }
}
