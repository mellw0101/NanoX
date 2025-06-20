/** @file search.c

  @author  Melwin Svensson.
  @date    21-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Whether we have compiled a regular expression for the search. */
static bool have_compiled_regexp = FALSE;
/* Have we reached the starting line again while searching? */
/* static */ bool came_full_circle = FALSE;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Do research ----------------------------- */

/* Search for the last string without prompting in `file`. */
/* static */ void do_research_for(CTX_ARGS) {
  ASSERT(file);
  /* If nothing was searched for yet during this run of nanox, but there is a search history, take the most recent item. */
  if (!*last_search && searchbot->prev) {
    last_search = xstrcpy(last_search, searchbot->prev->data);
  }
  if (!*last_search) {
    statusline(AHEM, _("No current search pattern"));
    return;
  }
  if (ISSET(USE_REGEXP) && !regexp_init(last_search)) {
    return;
  }
  /* Use the search-menu key bindings, to allow cancelling. */
  currmenu = MWHEREIS;
  if (LINES > 1) {
    wipe_statusbar();
  }
  go_looking_for(STACK_CTX);
  if (!inhelp) {
    tidy_up_after_search_for(file);
  }
}

/* Search for the last string without prompting in the currently open buffer. */
/* static */ void do_research(void) {
  CTX_CALL(do_research_for);
}

/* ----------------------------- Search init ----------------------------- */

/* Prepare the prompt and ask the user what to search for.  Keep looping
 * as long as the user presses a toggle, and only take action and exit
 * when <Enter> is pressed or a non-toggle shortcut was executed. */
// /* static */ void search_init_for(CTX_ARGS, bool replacing, bool retain_answer) {
//   ASSERT(file);
//   functionptrtype function;
//   char *disp;
//   Ulong disp_breadth;
//   int response;
//   /* What will be searched for when the user types just <Enter>. */
//   char *the_default;
//   /* If something was searched for earlier, include it in the prompt. */
//   if (*last_search) {
//     disp         = display_string(last_search, 0, (COLS / 3), FALSE, FALSE);
//     disp_breadth = breadth(disp);
//     the_default  = free_and_assign(disp, fmtstr(" [%s%s]", disp, (GT(disp_breadth, (COLS / 3)) ? "..." : "")));
//   }
//   else {
//     the_default = COPY_OF("");
//   }
//   while (TRUE) {
//     /* Ask the user what to search for (or replace). */
//     response = do_prompt(
//       (inhelp ? MFINDINHELP : (replacing ? MREPLACE : MWHEREIS)),
//       (retain_answer ? answer : ""),
//       &search_history,
//       edit_refresh,
//       /* TRANSLATORS: This is the main search prompt. */
//     )
//   }
// }


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Regular expression init ----------------------------- */

/* Compile the given regular expression and store it in search_regexp.
 * Returns `TRUE` if the expression is valid, and `FALSE` otherwise. */
bool regexp_init(const char *regexp) {
  Ulong len;
  char *str;
  int value = regcomp(&search_regexp, regexp, (NANO_REG_EXTENDED | (ISSET(CASE_SENSITIVE) ? 0 : REG_ICASE)));
  /* If regex compilation failed, show the error message. */
  if (value != 0) {
    len = regerror(value, &search_regexp, NULL, 0);
    str = xmalloc(len);
    regerror(value, &search_regexp, str, len);
    statusline(AHEM, _("Bad regex \"%s\": %s"), regexp, str);
    free(str);
    return FALSE;
  }
  have_compiled_regexp = TRUE;
  return TRUE;
}

/* ----------------------------- Tidy up after search ----------------------------- */

/* Free a compiled regular expression, if one was compiled; and schedule a full
 * screen refresh when the mark is on, in case the cursor in `file` has moved. */
void tidy_up_after_search_for(openfilestruct *const file) {
  ASSERT(file);
  if (have_compiled_regexp) {
    regfree(&search_regexp);
    have_compiled_regexp = FALSE;
  }
  if (file->mark) {
    refresh_needed = TRUE;
  }
  recook |= perturbed;
}

/* Free a compiled regular expression, if one was compiled; and schedule a full screen refresh when the
 * mark is on, in case the cursor has moved in the currently open buffer.  Note that this is `context-safe`. */
void tidy_up_after_search(void) {
  if (have_compiled_regexp) {
    regfree(&search_regexp);
    have_compiled_regexp = FALSE;
  }
  if (openfile->mark) {
    refresh_needed = TRUE;
  }
  recook |= perturbed;
}

/* ----------------------------- Goto line posx ----------------------------- */

/* Go to the specified `line-number` and `x` position, in `file`. */
void goto_line_posx_for(openfilestruct *const file, int rows, long lineno, Ulong x) {
  ASSERT(file);
  if (lineno > (file->edittop->lineno + rows) || (ISSET(SOFTWRAP) && lineno > file->current->lineno)) {
    recook |= perturbed;
  }
  file->current     = line_from_number_for(file, lineno);
  file->current_x   = x;
  set_pww_for(file);
  refresh_needed    = TRUE;
}

/* Go to the specified `line-number` and `x` position, in the currently open buffer. */
void goto_line_posx(long lineno, Ulong x) {
  if (IN_GUI_CTX) {
    goto_line_posx_for(GUI_OF, GUI_ROWS, lineno, x);
  }
  else {
    goto_line_posx_for(TUI_OF, TUI_ROWS, lineno, x);
  }
}

/* ----------------------------- Not found msg ----------------------------- */

/* Report on the status bar that the given string was not found. */
void not_found_msg(const char *const restrict str) {
  char *disp     = display_string(str, 0, ((COLS / 2) + 1), FALSE, FALSE);
  Ulong numchars = actual_x(disp, wideness(disp, (COLS / 2)));
  statusline(AHEM, _("\"%.*s%s\" not found"), numchars, disp, (disp[numchars] == '\0') ? "" : "...");
  free(disp);
}

/* ----------------------------- Find next string ----------------------------- */

/* Look for `needle`, starting at (`file->current`, `file->current_x`).  Begin is the line where we
 * first started searching, at columnb `begin_x`.  Returns `1` when we found something, `0` when nothing
 * and `-2` on cancel.  When `match_len` is not NULL, set it to the length of the found string, if any. */
int findnextstr_for(openfilestruct *const file, const char *const restrict needle, bool whole_word_only,
  int modus, Ulong *const match_len, bool skipone, const linestruct *const begin, Ulong begin_x)
{
  ASSERT(file);
  /* The length of a match -- will be recomputed for a regex. */
  Ulong found_len = strlen(needle);
  /* When bigger then zero, show and wipe the "Searching..." message. */
  int feedback = 0;
  int input;
  /* The line we will be searching through now. */
  linestruct *line = file->current;
  /* The point in the line from where we start searching. */
  const char *from = (line->data + file->current_x);
  /* A pointer to the location of the match, if any. */
  const char *found = NULL;
  /* The x coordinate of a found occurrence. */
  Ulong found_x;
  /* The time we last looked at the keyboard. */
  time_t lastkbcheck = time(NULL);
  /* Set non-blocking input so that we can just peek for a <Cancel>. */
  if (IN_CURSES_CTX) {
    nodelay(midwin, TRUE);
  }
  if (!begin) {
    came_full_circle = FALSE;
  }
  while (TRUE) {
    /* When starting a new search, skip the first character, then (in either case) search for the needle in the current line */
    if (skipone) {
      skipone = FALSE;
      /* Backward */
      if (ISSET(BACKWARDS_SEARCH) && from != line->data) {
        from  = (line->data + step_left(line->data, (from - line->data)));
        found = strstrwrapper(line->data, needle, from);
      }
      /* Forward */
      else if (!ISSET(BACKWARDS_SEARCH) && *from) {
        from += char_length(from);
        found = strstrwrapper(line->data, needle, from);
      }
    }
    else {
      found = strstrwrapper(line->data, needle, from);
    }
    if (found) {
      /* When doing a regex search, compute the length of the match. */
      if (ISSET(USE_REGEXP)) {
        found_len = (regmatches[0].rm_eo - regmatches[0].rm_so);
      }
      /* When we're not spell checking, a match should be a seperate word,
       * if it's not, continue looking in the rest of the line.  TODO: add
       * a flag that toggles whether or not to look for the partial matches. */
      if (whole_word_only && !is_separate_word((found - line->data), found_len, line->data)) {
        from = (found + char_length(found));
        continue;
      }
      /* When not on the magic line, the match is valid. */
      if (line->next || *line->data) {
        break;
      }
    }
    if (IN_CURSES_CTX && the_window_resized) {
      regenerate_screen();
      nodelay(midwin, TRUE);
      statusbar_all(_("Searching..."));
      feedback = 1;
    }
    /* If we're back at the beginning, then there is no needle. */
    if (came_full_circle) {
      if (IN_CURSES_CTX) {
        nodelay(midwin, TRUE);
      }
      return 0;
    }
    /* Move to the previous line. */
    if (ISSET(BACKWARDS_SEARCH)) {
      DLIST_ADV_PREV(line);
    }
    /* Move to the next line. */
    else {
      DLIST_ADV_NEXT(line);
    }
    /* We have reached the bottom or top of the file. */
    if (!line) {
      /* When spell-checking or replacing in a region, this is where we stop. */
      if (whole_word_only || modus == INREGION) {
        if (IN_CURSES_CTX) {
          nodelay(midwin, FALSE);
        }
        return 0;
      }
      /* Otherwise, we wrap around to... */
      line = (ISSET(BACKWARDS_SEARCH) ? file->filebot : file->filetop);
      /* When simply searching for the next occurrence. */
      if (modus == JUSTFIND) {
        statusline(REMARK, _("Search Wrapped"));
        /* Delay the "Searching..." message for at least two seconds. */
        feedback = 2;
      }
    }
    /* If we've reached the original starting line, take note. */
    if (line == begin) {
      came_full_circle = TRUE;
    }
    /* Set the starting x to the start or end of the line. */
    from = (ISSET(BACKWARDS_SEARCH) ? (line->data + strlen(line->data)) : line->data);
    /* Glance at the keyboard once every second, to check for <Cancel>.  Currently only in curses mode. */
    if ((time(NULL) - lastkbcheck) > 0) {
      if (IN_CURSES_CTX) {
        input       = wgetch(midwin);
        lastkbcheck = time(NULL);
        /* Consume any queued-up keystrokes, until a <Cancel> or nothing. */
        while (input != ERR) {
          if (input == ESC_CODE) {
            napms(20);
            input = wgetch(midwin);
            meta_key = TRUE;
          }
          else {
            meta_key = FALSE;
          }
          if (func_from_key(input) == do_cancel) {
            if (the_window_resized) {
              regenerate_screen();
            }
            statusbar_all(_("Cancelled"));
            /* Clear out the key buffer (in case a macro is running). */
            while (input != ERR) {
              input = get_input(NULL);
            }
            nodelay(midwin, FALSE);
            return -2;
          }
          input = wgetch(midwin);
        }
      }
      else {
        lastkbcheck = time(NULL);
      }
      if (++feedback > 0) {
        /* TRANSLATORS: This is shown when searching takes more then half a second. */
        statusbar_all(_("Searching..."));
      }
    }
  }
  found_x = (found - line->data);
  if (IN_CURSES_CTX) {
    nodelay(midwin, FALSE);
  }
  /* Ensure that the found occurrence is not beyond the starting x. */
  if (came_full_circle && (ISSET(BACKWARDS_SEARCH) ? (found_x < begin_x) : (found_x > begin_x || (modus == REPLACING && found_x == begin_x)))) {
    return 0;
  }
  /* Set the current position in file to point at what we found. */
  file->current   = line;
  file->current_x = found_x;
  /* When requested, pass back the length of the match. */
  ASSIGN_IF_VALID(match_len, found_len);
  if (modus == JUSTFIND && (!file->mark || file->softmark)) {
    spotlighted    = TRUE;
    light_from_col = xplustabs_for(file);
    light_to_col   = wideness(line->data, (found_x + found_len));
    refresh_needed = TRUE;
  }
  if (feedback > 0) {
    wipe_statusbar();
  }
  return 1;
}

/* Look for `needle`, starting at (`current`, `current_x`) in the currently open buffer.  Begin is the line
 * where we first started searching, at columnb `begin_x`.  Returns `1` when we found something, `0` when
 * nothing and `-2` on cancel.  When `match_len` is not NULL, set it to the length of the found string, if any. */
int findnextstr(const char *const restrict needle, bool whole_word_only, int modus,
  Ulong *const match_len, bool skipone, const linestruct *const begin, Ulong begin_x)
{
  if (IN_GUI_CTX) {
    return findnextstr_for(GUI_OF, needle, whole_word_only, modus, match_len, skipone, begin, begin_x);
  }
  else {
    return findnextstr_for(TUI_OF, needle, whole_word_only, modus, match_len, skipone, begin, begin_x);
  }
}

/* ----------------------------- Go locking ----------------------------- */

/* Search for the global string 'last_search' in `file`.  Inform the user when the string occurs only once. */
void go_looking_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *was_current = file->current;
  Ulong was_x = file->current_x;
/* # define TIMEIT 12 */
# ifdef TIMEIT
#   include <time.h>
  clock_t start = clock();
# endif
  came_full_circle = FALSE;
  /* TODO: Add a flag to chose whole words or not. */
  didfind = findnextstr_for(file, last_search, FALSE, JUSTFIND, NULL, TRUE, file->current, file->current_x);
  /* If we found something, and we're back at the exact same spot where we started searching, then this is the only occurence. */
  if (didfind == 1 && file->current == was_current && file->current_x == was_x) {
    statusline(REMARK, _("This is the only occurrence"));
  }
  else if (!didfind) {
    not_found_msg(last_search);
  }
# ifdef TIMEIT
  statusline(INFO, "Took: %.2f", (double)(clock() - start) / CLOCKS_PER_SEC);
# endif
  edit_redraw_for(STACK_CTX, was_current, CENTERING);
}

/* Search for the global string 'last_search' in the currently open buffer.  Inform the user when the string occurs only once. */
void go_looking(void) {
  CTX_CALL(go_looking_for);
}

/* ----------------------------- Do findprevious ----------------------------- */

/* Search in the backward direction for the next occurrence in `file`. */
void do_findprevious_for(CTX_ARGS) {
  ASSERT(file);
  SET(BACKWARDS_SEARCH);
  do_research_for(STACK_CTX);
}

/* Search in the backward direction for the next occurrence in the currently open buffer.  Note that this is `context-safe` */
void do_findprevious(void) {
  CTX_CALL(do_findprevious_for);
}

/* ----------------------------- Do findnext ----------------------------- */

/* Search in the forward direction for the next occurrence in `file`. */
void do_findnext_for(CTX_ARGS) {
  ASSERT(file);
  UNSET(BACKWARDS_SEARCH);
  do_research_for(STACK_CTX);
}

/* Search in the forward direction for the next occurrence in the currently open buffer.  Note that this is `context-safe` */
void do_findnext(void) {
  CTX_CALL(do_findnext_for);
}
