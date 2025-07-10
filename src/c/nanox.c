/** @file nanox.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Used to store the user's original mouse click interval. */
static int oldinterval = -1;

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

/* ----------------------------- Disable mouse support ----------------------------- */

/* Disable mouse support for `curses-mode`. */
static void disable_mouse_support(void) {
  if (IN_CURSES_CTX) {
    mousemask(0, NULL);
    mouseinterval(oldinterval);
  }
}

/* Enable mouse support for `curses-mode`. */
static void enable_mouse_support(void) {
  if (IN_CURSES_CTX) {
    mousemask(ALL_MOUSE_EVENTS, NULL);
    oldinterval = mouseinterval(50);
  }
}

/* Switch mouse support on or off, as needed. */
/* static */ void mouse_init(void) {
  /* Only perform any action when in curses mode. */
  if (IN_CURSES_CTX) {
    /* Enable */
    if (ISSET(USE_MOUSE)) {
      enable_mouse_support();
    }
    /* Disable */
    else {
      disable_mouse_support();
    }
  }
}

/* ----------------------------- Restore terminal ----------------------------- */

/* Make sure the cursor is visible, then exit from curses mode, disable
 * bracketed-paste mode, and restore the original terminal settings. */
static void restore_terminal(void) {
  /* For now we only perform any action when in curses mode, as we will probebly also
   * take control of the terminal later when in tui mode to create a debugging interface. */
  if (IN_CURSES_CTX) {
    curs_set(TRUE);
    endwin();
    /* End bracketed paste. */
    printf("\x1B[?2004l");
    tcsetattr(STDIN_FILENO, TCSANOW, &original_state);
  }
}

/* ----------------------------- Emergency save ----------------------------- */

/* Save `file` under the given name (or `nanox.<pid>` when nameless) with
 * suffix `.save`.  If needed, the name is further suffixed to be unique. */
/* static */ void emergency_save_for(openfilestruct *const file, const char *const restrict name) {
  ASSERT(file);
  ASSERT(name);
  char *plain  = (*name ? copy_of(name) : fmtstr("nanox.%u", getpid()));
  char *target = get_next_filename(plain, ".save");
  if (!*target) {
    fprintf(stderr, _("\nTo meny .save files\n"));
  }
  else if (write_file_for(file, target, NULL, SPECIAL, EMERGENCY, NONOTES)) {
    fprintf(stderr, _("\nBuffer written to %s\n"), target);
  }
  free(plain);
  free(target);
}

/* Save the currently open buffer under the given name (or `nanox.<pid>` when nameless) with suffix
 * `.save`.  If needed, the name is further suffixed to be unique.  Note that this is `context-safe`. */
/* static */ void emergency_save(const char *const restrict name) {
  emergency_save_for(CTX_OF, name);
}

/* ----------------------------- Print opt ----------------------------- */

/* Print the usage line for the given option to the screen. */
/* static */ void print_opt(const char *const restrict sflag, const char *const restrict lflag, const char *const restrict description) {
  ASSERT(sflag);
  ASSERT(lflag);
  ASSERT(description);
  int w1 = breadth(sflag);
  int w2 = breadth(lflag);
  writef(" %s", sflag);
  if (w1 < 14) {
    writef("%*s", (14 - w1), " ");
  }
  writef(" %s", lflag);
  if (w2 < 24) {
    writef("%*s", (24 - w2), " ");
  }
  writef("%s\n", _(description));
}

/* ----------------------------- Signal init ----------------------------- */

/* Register half a dozen signal handlers. */
/* static */ void signal_init(void) {
  struct sigaction deed = {0};
  /* Trap SIGINT and SIGQUIT because we want them to do useful things. */
  deed.sa_handler = SIG_IGN;
  sigaction(SIGINT, &deed, NULL);
  sigaction(SIGQUIT, &deed, NULL);
  /* Trap SIGHUP and SIGTERM because we want to write the file out...? */
  deed.sa_handler = handle_hupterm;
  sigaction(SIGHUP, &deed, NULL);
  sigaction(SIGTERM, &deed, NULL);
  /* Trap SIGWINCH because we want to handle resizes. */
  deed.sa_handler = handle_sigwinch;
  sigaction(SIGWINCH, &deed, NULL);
  /* Prevent the suspend handler from getting interrupted. */
  sigfillset(&deed.sa_mask);
  deed.sa_handler = suspend_nano;
  sigaction(SIGTSTP, &deed, NULL);
  sigfillset(&deed.sa_mask);
  deed.sa_handler = continue_nano;
  sigaction(SIGCONT, &deed, NULL);
# if !defined(DEBUG)
  /* For now we keep this as NANO_NOCATCH. */
  if (!getenv("NANO_NOCATCH")) {
    /* Trap SIGSEGV and SIGABRT to save any changed buffers and reset the terminal to a
     * usable state.  Reset these handlers to their defaults as soon as their signal fires. */
    deed.sa_handler = handle_crash;
    deed.sa_flags |= SA_RESETHAND;
    sigaction(SIGSEGV, &deed, NULL);
    sigaction(SIGABRT, &deed, NULL);
  }
# endif
}

/* ----------------------------- Die gui ----------------------------- */

/* Save all modified files in all editors.  Note that this should only ever be used when in `gui-mode`. */
static void die_gui(void) {
  Editor *first_editor = openeditor;
  Editor *open_editor  = first_editor;
  openfilestruct *first_file;
  openfilestruct *open_file;
  /* Go through all editors. */
  while (open_editor) {
    /* Start from the currently open buffer in the current editor. */
    first_file = open_editor->openfile;
    open_file  = first_file;
    /* Now iterate over all files in the current editor. */
    while (open_file) {
      /* If the current buffer has a lock-file, remove it. */
      if (open_file->lock_filename) {
        delete_lockfile(open_file->lock_filename);
      }
      /* When not in restricted mode, and the current buffer has been modified, we perform an emergency save. */
      if (!ISSET(RESTRICTED) && open_file->modified)  {
        emergency_save_for(open_file, open_file->filename);
      }
      /* Move to the next buffer. */
      CLIST_ADV_NEXT(open_file);
      /* If we have reached the starting file, we stop here. */
      if (open_file == first_file) {
        break;
      }
    }
    /* Move to the next editor. */
    CLIST_ADV_NEXT(open_editor);
    /* If we have reached the starting editor, we stop here. */
    if (open_editor == first_editor) {
      break;
    }
  }
}

/* ----------------------------- Die curses ----------------------------- */

/* Save all modified buffers.  Note that this should only ever be used when in `curses-mode`. */
static void die_curses(void) {
  openfilestruct *first_file = openfile;
  openfilestruct *open_file  = first_file;
  /* Go through all files. */
  while (open_file) {
    /* If the current buffer has a lock-file, remove it. */
    if (open_file->lock_filename) {
      delete_lockfile(open_file->lock_filename);
    }
    /* When not in restricted mode, and the current buffer has been modified, we perform an emergency save. */
    if (!ISSET(RESTRICTED) && open_file->modified) {
      emergency_save_for(open_file, open_file->filename);
    } 
    /* Move to the next file in the circulare list. */
    CLIST_ADV_NEXT(open_file);
    /* If we have reached the first file, we stop here. */
    if (open_file == first_file) {
      break;
    }
  }
}

/* ----------------------------- Suck up input and paste it ----------------------------- */

/* Read in all waiting input bytes and paste them into the buffer in one go. */
/* static */ void suck_up_input_and_paste_it(void) {
  linestruct *was_cutbuffer;
  linestruct *line;
  Ulong index;
  int input;
  /* Only perform any action when in `curses-mode`. */
  if (IN_CURSES_CTX) {
    was_cutbuffer = cutbuffer;
    line          = make_new_node(NULL);
    line->data    = COPY_OF("");
    index         = 0;
    cutbuffer     = line;
    while (bracketed_paste) {
      input = get_kbinput(midwin, BLIND);
      if (input == '\r' || input == '\n') {
        line->next = make_new_node(line);
        DLIST_ADV_NEXT(line);
        line->data = COPY_OF("");
        index = 0;
      }
      else if ((0x20 <= input && input <= 0xFF && input != DEL_CODE) || input == '\t') {
        line->data = xrealloc(line->data, (index + 2));
        line->data[index++] = input;
        line->data[index]   = '\0';
      }
      else if (input != BRACKETED_PASTE_MARKER) {
        beep();
      }
    }
    if (ISSET(VIEW_MODE)) {
      print_view_warning();
    }
    else {
      paste_text();
    }
    free_lines(cutbuffer);
    cutbuffer = was_cutbuffer;
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Make new node ----------------------------- */

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
  splice_node_for(CTX_OF, after, node);
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
  delete_node_for(CTX_OF, line);
}

/* ----------------------------- Unlink node ----------------------------- */

/* Disconnect a node from a linked list of linestructs and delete it. */
void unlink_node_for(CTX_ARG_OF, linestruct *const node) {
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
  unlink_node_for(CTX_OF, node);
}

/* ----------------------------- Free lines ----------------------------- */

/* Free an entire linked list of linestructs. */
void free_lines_for(openfilestruct *const file, linestruct *src) {
  /* Make this function a `no-op` function passed `NULL` list head. */
  if (!src) {
    return;
  }
  while (src->next) {
    src = src->next;
    delete_node_for(file, src->prev);
  }
  delete_node_for(file, src);
}

/* Free an entire linked list of linestructs. */
void free_lines(linestruct *const head) {
  free_lines_for(CTX_OF, head);
}

/* ----------------------------- Copy node ----------------------------- */

/* Make a copy of a linestruct node. */
linestruct *copy_node(const linestruct *const src) {
  ASSERT(src);
  linestruct *dst = xmalloc(sizeof(*dst));
  dst->data       = copy_of(src->data);
  dst->multidata  = NULL;
  dst->lineno     = src->lineno;
  dst->has_anchor = src->has_anchor;
  return dst;
}

/* ----------------------------- Copy buffer top bot ----------------------------- */

/* Duplicate an entire linked list of linestructs, and assign the head to `*top` and when the caller wants the tail to `*bot`. */
void copy_buffer_top_bot(const linestruct *src, linestruct **const top, linestruct **const bot) {
  ASSERT(src);
  ASSERT(top);
  linestruct *item;
  (*top)       = copy_node(src);
  (*top)->prev = NULL;
  item         = (*top);
  DLIST_ADV_NEXT(src);
  while (src) {
    item->next       = copy_node(src);
    item->next->prev = item;
    DLIST_ADV_NEXT(item);
    DLIST_ADV_NEXT(src);
  }
  item->next = NULL;
  ASSIGN_IF_VALID(bot, item);
}

/* ----------------------------- Copy buffer ----------------------------- */

/* Duplicate an entire linked list of linestructs. */
linestruct *copy_buffer(const linestruct *src) {
  linestruct *head;
  linestruct *item;
  head       = copy_node(src);
  head->prev = NULL;
  item       = head;
  DLIST_ADV_NEXT(src);
  while (src) {
    item->next       = copy_node(src);
    item->next->prev = item;
    DLIST_ADV_NEXT(item);
    DLIST_ADV_NEXT(src);
  }
  item->next = NULL;
  return head;
}

/* ----------------------------- Renumber from ----------------------------- */

/* Renumber the lines in a buffer, from the given line onwards. */
void renumber_from(linestruct *line) {
  long number = (!line->prev ? 0 : line->prev->lineno);
  while (line) {
    line->lineno = ++number;
    line = line->next;
  }
}

/* ----------------------------- Print view warning ----------------------------- */

/* Display a warning about a key disabled in view mode. */
void print_view_warning(void) {
  statusline(AHEM, _("Key is invalid in view mode"));
}

/* ----------------------------- In restricted mode ----------------------------- */

/* When in restricted mode, show a warning and return 'TRUE'. */
bool in_restricted_mode(void) {
  if (ISSET(RESTRICTED)) {
    statusline(AHEM, _("This function is disabled in restricted mode"));
    /* Only use `beep()`, when using the ncurses context. */
    if (IN_CURSES_CTX) {
      beep();
    }
    return TRUE;
  }
  return FALSE;
}

/* ----------------------------- Disable flow control ----------------------------- */

/* Disable the terminal's XON/XOFF flow-control characters. */
void disable_flow_control(void) {
  struct termios settings;
  tcgetattr(0, &settings);
  settings.c_iflag &= ~IXON;
  tcsetattr(0, TCSANOW, &settings);
}

/* ----------------------------- Enable flow control ----------------------------- */

/* Enable the terminal's XON/XOFF flow-control characters. */
void enable_flow_control(void) {
  struct termios settings;
  tcgetattr(0, &settings);
  settings.c_iflag |= IXON;
  tcsetattr(0, TCSANOW, &settings);
}

/* ----------------------------- Disable extended io ----------------------------- */

/* Disable extended input and output processing in our terminal settings. */
void disable_extended_io(void) {
  struct termios settings = {0};
  tcgetattr(0, &settings);
  settings.c_lflag &= ~IEXTEN;
  settings.c_oflag &= ~OPOST;
  tcsetattr(0, TCSANOW, &settings);
}

/* ----------------------------- Confirm margin ----------------------------- */

void confirm_margin_for(openfilestruct *const file, int *const cols) {
  ASSERT(file);
  ASSERT(cols);
  bool keep_focus;
  int needed_margin;
  /* Only perform any action when in curses-mode. */
  if (IN_CURSES_CTX) {
    needed_margin = (digits(file->filebot->lineno) + 1);
    /* When not requested or space is to tight, suppress line numbers. */
    if (!ISSET(LINE_NUMBERS) || needed_margin > (COLS - 4)) {
      needed_margin = 0;
    }
    if (needed_margin != margin) {
      keep_focus = ((margin > 0) && focusing);
      margin     = needed_margin;
      *cols      = (COLS - margin - sidebar);
      /* Ensure a proper starting column for the first screen row. */
      ensure_firstcolumn_is_aligned_for(file, *cols);
      focusing = keep_focus;
      /* The margin has changed -- schedual a full refresh. */
      refresh_needed = TRUE;
    }
  }
}

/* Ensure that the margin can accommodate the buffer's highest line number. */
void confirm_margin(void) {
  if (IN_CURSES_CTX) {
    confirm_margin_for(TUI_OF, &TUI_COLS);
  }
  // bool keep_focus;
  // int needed_margin;
  // /* Only perform any action when in curses-mode. */
  // if (IN_CURSES_CTX) {
  //   needed_margin = (digits(openfile->filebot->lineno) + 1);
  //   /* When not requested or space is too tight, suppress line numbers. */
  //   if (!ISSET(LINE_NUMBERS) || needed_margin > (COLS - 4)) {
  //     needed_margin = 0;
  //   }
  //   if (needed_margin != margin) {
  //     keep_focus  = ((margin > 0) && focusing);
  //     margin      = needed_margin;
  //     editwincols = (COLS - margin - sidebar);
  //     /* Ensure a proper starting column for the first screen row. */
  //     ensure_firstcolumn_is_aligned();
  //     focusing = keep_focus;
  //     /* The margin has changed -- schedule a full refresh. */
  //     refresh_needed = TRUE;
  //   }
  // }
}

/* ----------------------------- Disable kb interrupt ----------------------------- */

/* Stop ^C from generating a SIGINT. */
void disable_kb_interrupt(void) {
  struct termios settings = {0};
  /* Only perform any action when in curses-mode.  TODO: Check this... */
  if (IN_CURSES_CTX) {
    tcgetattr(0, &settings);
    settings.c_lflag &= ~ISIG;
    tcsetattr(0, TCSANOW, &settings);
  }
}

/* ----------------------------- Enable kb interrupt ----------------------------- */

/* Make ^C generate a SIGINT. */
void enable_kb_interrupt(void) {
  struct termios settings = {0};
  /* Only perform any action when in curses-mode.  TODO: Check this... */
  if (IN_CURSES_CTX) {
    tcgetattr(0, &settings);
    settings.c_lflag |= ISIG;
    tcsetattr(0, TCSANOW, &settings);
  }
}

/* ----------------------------- Install handler for Ctrl C ----------------------------- */

/* Make ^C interrupt a system call and set a flag. */
void install_handler_for_Ctrl_C(void) {
  /* Enable the generation of a SIGINT when ^C is pressed. */
  enable_kb_interrupt();
  /* Set up a signal handler so that pressing ^C will set a flag. */
  newaction.sa_handler = make_a_note;
  newaction.sa_flags   = 0;
  sigaction(SIGINT, &newaction, &oldaction);
}

/* ----------------------------- Restore handler for Ctrl C ----------------------------- */

/* Go back to ignoring ^C. */
void restore_handler_for_Ctrl_C(void) {
  sigaction(SIGINT, &oldaction, NULL);
  disable_kb_interrupt();
}

/* ----------------------------- Terminal init ----------------------------- */

/* Set up the terminal state.  Put the terminal in raw mode
 * (read one character at a time, disable the special control keys, and disable
 * the flow control characters), disable translation of carriage return (^M)
 * into newline (^J), so that we can tell the difference between the Enter key
 * and Ctrl-J, and disable echoing of characters as they're typed. Finally,
 * disable extended input and output processing, and, if we're not in preserve
 * mode, reenable interpretation of the flow control characters. */
void terminal_init(void) {
  /* Running in ncurses context. */
  if (IN_CURSES_CTX) {
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
  // else {
  //   struct termios raw;
  //   raw = original_state;
  //   raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  //   raw.c_oflag &= ~ONLCR;
  //   raw.c_iflag &= ~(IXON | ICRNL | INLCR);
  //   raw.c_iflag |= IGNBRK;
  //   tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  //   disable_extended_io();
  //   tui_enable_bracketed_pastes();
  // }
}

/* ----------------------------- Window init ----------------------------- */

/* Initialize the three window portions nano uses.  For the tui. */
void window_init(void) {
  int min;
  int toprows;
  int bottomrows;
  /* When inside curses mode. */
  if (IN_CURSES_CTX) {
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

/* ----------------------------- Regenerate screen ----------------------------- */

/* Reinitialize and redraw the screen completely. */
void regenerate_screen(void) {
  if (!IN_CURSES_CTX) {
    return;
  }
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

/* ----------------------------- Block sigwinch ----------------------------- */

/* Block or unblock the SIGWINCH signal, depending on the blockit parameter. */
void block_sigwinch(bool blockit) {
  sigset_t winch;
  sigemptyset(&winch);
  sigaddset(&winch, SIGWINCH);
  sigprocmask((blockit ? SIG_BLOCK : SIG_UNBLOCK), &winch, NULL);
  if (the_window_resized) {
    regenerate_screen();
  }
}

/* ----------------------------- Handle sigwinch ----------------------------- */

/* Handler for SIGWINCH (window size change). */
void handle_sigwinch(int _UNUSED signal) {
  /* Let the input routine know that a SIGWINCH has occurred. */
  the_window_resized = TRUE;
}

/* ----------------------------- Suspend nano ----------------------------- */

/* Handler for SIGTSTP (suspend). */
void suspend_nano(int _UNUSED signal) {
  disable_mouse_support();
  restore_terminal();
  printf("\n\n");
  /* Display our helpful message. */
  printf(_("Use \"fg\" to return to nano.\n"));
  fflush(stdout);
  /* The suspend keystroke must not elicit cursor-position display. */
  lastmessage = HUSH;
  /* Do what mutt does: send ourselves a SIGSTOP. */
  kill(0, SIGSTOP);
}

/* ----------------------------- Continue nano ----------------------------- */

/* Handler for SIGCONT (continue after suspend). */
void continue_nano(int _UNUSED signal) {
  if (ISSET(USE_MOUSE)) {
    enable_mouse_support();
  }
  /* Seams wierd to me that we assume the window was resized
   * instead of checking, but it's the original code.
   * COMMENT: -> // Perhaps the user resized the window while we slept.
   * TODO: Check if the window was resized instead. */
  the_window_resized = TRUE;
  /* Insert a fake keystroke, to neutralize a key-eating issue. */
  ungetch(KEY_FRESH);
}

/* ----------------------------- Do suspend ----------------------------- */

/* When permitted, put nano to sleep. */
void do_suspend(void) {
  if (in_restricted_mode()) {
    return;
  }
  suspend_nano(0);
  ran_a_tool = TRUE;
}

/* ----------------------------- Reconnect and store state ----------------------------- */

/* Reconnect standard input to the tty, and store its state. */
void reconnect_and_store_state(void) {
  int tty;
  /* Only perform this when in ncurses mode. */
  if (IN_CURSES_CTX) {
    tty = open("/dev/tty", O_RDONLY);
    if (tty < 0 || dup2(tty, STDIN_FILENO) < 0) {
      die(_("Could not reconnect stdin to keyboard"));
    }
    close(tty);
    /* If input was not cut short, store the current state of the terminal. */
    if (!control_C_was_pressed) {
      tcgetattr(STDIN_FILENO, &original_state);
    }
  }
}

/* ----------------------------- Handle hupterm ----------------------------- */

/* Handler for SIGHUP (hangup) and SIGTERM (terminate). */
void handle_hupterm(int _UNUSED signal) {
  die(_("Received SIGHUP or SIGTERM\n"));
}

/* ----------------------------- Handle crash ----------------------------- */

/* Handler for SIGSEGV (segfault) and SIGABRT (abort). */
void handle_crash(int _UNUSED_IN_DEBUG signal) {
# if !defined(DEBUG)
  void *buffer[256];
  int size = backtrace(buffer, ARRAY_SIZE(buffer));
  char **symbols = backtrace_symbols(buffer, size);
  /* When we are dying from a signal, try to print the last ran functions. */
  for (int i=0; i<size; ++i) {
    fprintf(stderr, "[%d]: %s\n", i, symbols[i]);
  }
  switch (signal) {
    case SIGABRT: {
      die(_("Sorry! Nano crashed! Code: '%d/SIGABRT' (abort).  Please report a bug.\n"), signal);
      break;
    }
    case SIGSEGV: {
      die(_("Sorry! Nano crashed! Code: '%d/SIGSEGV' (segfault).  Please report a bug.\n"), signal);
      break;
    }
    default: {
      die(_("Sorry! Nano crashed! Code: %d.  Please report a bug.\n"), signal);
      break;
    }
  }
# else
  ;
# endif
}

/* ----------------------------- Inject ----------------------------- */

/* Insert the given `burst` of `count` bytes into `file`. */
void inject_into_buffer(CTX_ARGS, char *burst, Ulong count) {
  ASSERT(file);
  linestruct *line   = file->current;
  Ulong datalen      = strlen(line->data);
  Ulong original_row = 0;
  Ulong old_amount   = 0;
  if (ISSET(SOFTWRAP)) {
    if (file->cursor_row == (rows - 1)) {
      original_row = chunk_for(cols, xplustabs_for(file), line);
    }
    old_amount = extra_chunks_in(cols, line);
  }
  /* Encode an embedded `NUL` byte as `0x0A`. */
  RECODE_NUL_TO_LF(burst, count);
  /* Only add a new undo item when the current item is not an `ADD` or when the current typing is not contignous with the previous typing. */
  if (file->last_action != ADD || file->current_undo->tail_lineno != line->lineno || file->current_undo->tail_x != file->current_x) {
    add_undo_for(file, ADD, NULL);
  }
  /* Inject the burst into the cursor line of `file`. */
  file->current->data = xnstrninj(file->current->data, datalen, burst, count, file->current_x);
  /* When the cursor is on the top row and not on the first chunk of a line, adding text
   * there might change the preceding chunk and thus require an adjustment of firstcolumn. */
  if (line == file->edittop && file->firstcolumn > 0) {
    ensure_firstcolumn_is_aligned_for(file, cols);
    refresh_needed = TRUE;
  }
  /* When the mark is to the right of the cursor compensate its position. */
  if (line == file->mark && file->current_x < file->mark_x) {
    file->mark_x += count;
  }
  file->current_x += count;
  file->totsize += mbstrlen(burst);
  set_modified_for(file);
  /* If text was added to the magic line, create a new magic line. */
  if (line == file->filebot && !ISSET(NO_NEWLINES)) {
    new_magicline_for(file);
    /* Original code from nano, this is stupid and there is no reason to have the first if statement. 
     *
     * if (margin || (openfile->syntax && openfile->syntax->multiscore)) {
     *   if (margin && openfile->cursor_row < (editwinrows - 1)) {
     *     update_line_curses(thisline->next, 0);
     *   }
     * }
     *
     * I will just use the second statement. */
    if (margin && file->cursor_row < (rows - 1)) {
      update_line_curses_for(file, line->next, 0);
    }
  }
  update_undo_for(file, ADD);
  if (ISSET(BREAK_LONG_LINES)) {
    do_wrap_for(file, cols);
  }
  SET_PWW(file);
  /* When softwrapping and the number of chunks in the current line changed, or we were
   * on the last row of the edit window and moved to a new chunk, we need a full refresh. */
  if (ISSET(SOFTWRAP) && (extra_chunks_in(cols, file->current) != old_amount
  || (file->cursor_row == (rows - 1) && chunk_for(cols, file->placewewant, file->current) > original_row))) {
    refresh_needed = TRUE;
    focusing       = FALSE;
  }
  if (!refresh_needed) {
    check_the_multis_for(file, file->current);
  }
  if (!refresh_needed) {
    update_line_curses_for(file, file->current, file->current_x);
  }
}

/* Insert the given `burst` of `count` bytes into the currently open file.  Note that this is `context-safe`. */
void inject(char *burst, Ulong count) {
  CTX_CALL_WARGS(inject_into_buffer, burst, count);
}

/* ----------------------------- Unbound key ----------------------------- */

/* Say that an unbound key was struck, and if possible which one. */
void unbound_key(int code) {
  /* Only perform any action when in `curses-mode`. */
  if (IN_CURSES_CTX) {
    if (code == FOREIGN_SEQUENCE) {
      /* TRANSLATORS: This refers to a sequnce of escape codes (from the keyboard) that nano does not recognize. */
      statusline(AHEM, _("Unknown sequence"));
    }
    else if (code == NO_SUCH_FUNCTION) {
      statusline(AHEM, _("Unknown function: %s"), commandname);
    }
    else if (code == MISSING_BRACE) {
      statusline(AHEM, _("Missing }"));
    }
    else if (code > KEY_F0 && code < (KEY_F0 + 25)) {
      /* TRANSLATORS: this refers to an unbound function key. */
      statusline(AHEM, _("Unbound key: F%i"), (code - KEY_F0));
    }
    else if (code > 0x7F) {
      statusline(AHEM, _("Unbound key"));
    }
    else if (meta_key) {
      if (code < 0x20) {
        statusline(AHEM, _("Unbindable key: M-^%c"), (code + 0x40));
      }
      else if (shifted_metas && 'A' <= code && code <= 'Z') {
        statusline(AHEM, _("Unbound key: Sh-M-%c"), code);
      }
      else {
        statusline(AHEM, _("Unbound key: M-%c"), toupper(code));
      }
    }
    else if (code == ESC_CODE) {
      statusline(AHEM, _("Unbindable key: ^["));
    }
    else if (code < 0x20) {
      statusline(AHEM, _("Unbound key: ^%c"), (code + 0x40));
    }
    else {
      statusline(AHEM, _("Unbound key: %c"), code);
    }
    set_blankdelay_to_one();
  }
}

/* ----------------------------- Close and go ----------------------------- */

/* Close `*open`, freeing its memory.  Note that this is a context-less
 * function, meaning it needs the given context's buffer pointers. */
void close_and_go_for(openfilestruct **const start, openfilestruct **const open, int cols) {
  ASSERT(start);
  ASSERT(open);
  ASSERT(*start);
  ASSERT(*open);
  if ((*open)->lock_filename) {
    delete_lockfile((*open)->lock_filename);
    (*open)->lock_filename = NULL;
  }
  /* When position history is enabled, save it. */
  if (ISSET(POSITIONLOG)) {
    update_poshistory_for(*open);
  }
  /* If there is another buffer, close this one. */
  if (!CLIST_SINGLE(*open)) {
    switch_to_next_buffer_for(open, cols);
    DLIST_ADV_PREV(*open);
    close_buffer_for(*open, start, open);
    DLIST_ADV_NEXT(*open);
    /* When in curses-mode, adjust the count in the top-bar. */
    if (IN_CURSES_CTX) {
      titlebar(NULL);
    }
  }
  /* Otherwise, just terminate. */
  else {
    if (ISSET(HISTORYLOG)) {
      save_history();
    }
    /* TODO: Implement finish as a context-less solution. */
    finish();
  }
}

/* Close the `currently open buffer`, freeing it's memory.  Note that this is `context-safe`. */
void close_and_go(void) {
  if (IN_GUI_CTX) {
    close_and_go_for(&GUI_SF, &GUI_OF, GUI_COLS);
  }
  else {
    close_and_go_for(&TUI_SF, &TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- Die ----------------------------- */

/* Die gracefully, by restoring the terminal state (when in `curses-mode`) and, saving any buffers that were modified. */
void die(const char *const restrict format, ...) {
  static int stabs = 0;
  va_list ap;
  /* When dying for a second time, just pull the plug. */
  if (++stabs > 1) {
    exit(11);
  }
  if (IN_CURSES_CTX) {
    restore_terminal();
  }
  display_rcfile_errors();
  va_start(ap, format);
  /* Print the dying message to stderr. */
  vfprintf(stderr, format, ap);
  va_end(ap);
  if (IN_GUI_CTX) {
    die_gui();
  }
  else if (IN_CURSES_CTX) {
    die_curses();
  }
  /* Abandon the building. */
  exit(1);
}

/* ----------------------------- Version ----------------------------- */

/* Display the verison number of NanoX, a copyright notice for the original creators
 * of the forked version of `GNU nano`.  And also some information about this fork. */
void version(void) {
  writef(_("NanoX (nx), version %s\n"), VERSION);
  writef("  'NanoX %s' is a fork of `GNU nano v8.0-44-gef1c9b9f` from git source code.\n", REVISION);
  writef("  First converted from 'C' into 'C++', and modernized to include mush more\n");
  writef("  modern features, sush as (whole line moving / selection region moving),\n");
  writef("  mush better mark state tracking, and also a new openGL based gui and mush more.\n");
  writef("  Then converted back into 'C' once again.\n");
  writef(_("  (C) %s the Free Software Foundation and various contributors\n"), "2024");
# ifdef DEBUG
  writef(_("  Compiled options:"));
  writef("    --enable-dubug");
# endif
  writef("\n");
  exit(0);
}

/* ----------------------------- Usage ----------------------------- */

void usage(void) {
  writef(_("Usage: %s [OPTIONS] [[+LINE[,COLUMN]] FILE]...\n\n"));
  /* TRANSLARORS: The next two strings are part of the --help
   * output.  It's best to keep it's lines within 80 characters. */
  writef(_("To place the cursor on a specific line of a file, put the line number with\n"
           "a '+' before the filename.  The column number can be added after a comma.\n"));
  /* TRANSLATORS: Then next  */
}

























