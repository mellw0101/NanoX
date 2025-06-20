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

/* ----------------------------- Replace regexp ----------------------------- */

/* Calculate the size of the replacement text, taking possible subexpressions \1 to \9 into
 * account. Returns the replacement text in the passed string only when `create` is `TRUE`. */
/* static */ int replace_regexp_for(openfilestruct *const file, char *string, bool create) {
  ASSERT(file);
  Ulong replacement_size = 0;
  Ulong i;
  const char *c = answer;
  int num;
  /* Iterate through the replacement text to handle expressions replacement using \1, \2, \3, etc. */
  while (*c) {
    num = (*(c + 1) - '0');
    if (*c != '\\' || num < 1 || num > 9 || GT(num, search_regexp.re_nsub)) {
      if (create) {
        *(string++) = *c;
      }
      ++c;
      ++replacement_size;
    }
    else {
      i = (regmatches[num].rm_eo - regmatches[num].rm_so);
      /* Skip over the replacement expression. */
      c += 2;
      /* But add the length of the subexpression to `new_size`. */
      replacement_size += i;
      /* And if create is `TRUE`, append the result of the subexpression match to the new line. */
      if (create) {
        memcpy(string, (file->current->data + regmatches[num].rm_so), i);
        string += i;
      }
    }
  }
  if (create) {
    *string = '\0';
  }
  return replacement_size;
}

/* Calculate the size of the replacement text, taking possible subexpressions \1 to \9 into
 * account. Returns the replacement text in the passed string only when `create` is `TRUE`. */
/* static */ int replace_regexp(char *string, bool create) {
  return replace_regexp_for(CTX_OF, string, create);
}

/* ----------------------------- Replace line ----------------------------- */

/* Returns a copy of the current line with one needle replaced. */
/* static */ char *replace_line_for(openfilestruct *const file, const char *const restrict needle) {
  ASSERT(file);
  ASSERT(needle);
  Ulong new_size = (strlen(file->current->data) + 1);
  Ulong match_len;
  char *copy;
  /* First adjust the size of the new line for the change. */
  if (ISSET(USE_REGEXP)) {
    match_len = (regmatches[0].rm_eo - regmatches[0].rm_so);
    new_size += (replace_regexp(NULL, FALSE) - match_len);
  }
  else {
    match_len = strlen(needle);
    new_size += (strlen(answer) - match_len);
  }
  copy = xmalloc(new_size);
  /* Copy the head of the original line. */
  memcpy(copy, file->current->data, file->current_x);
  /* Add the replacement text. */
  if (ISSET(USE_REGEXP)) {
    replace_regexp((copy + file->current_x), TRUE);
  }
  else {
    strcpy((copy + file->current_x), answer);
  }
  /* Copy the tail of the original line. */
  xstrcat_norealloc(copy, (file->current->data + file->current_x + match_len));
  return copy;
}

/* Returns a copy of the current line with one needle replaced. */
/* static */ char *replace_line(const char *const restrict needle) {
  return replace_line_for(CTX_OF, needle);
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
//       "%s%s%s%s%s%s", _("Search"),
//       (ISSET(CASE_SENSITIVE) ? _(" [Case Sensitive]") : ""),
//       (ISSET(USE_REGEXP) ? _(" [Regexp]") : ""),
//       (ISSET(BACKWARDS_SEARCH) ? _(" [Backwards]") : ""),
//       (replacing ? (file->mark ? _(" (to replace) in selection") : _(" (to replace)")) : ""),
//       the_default
//     );
//     /* If the search was cancelled, or we have a blank answer and nothing was searched for yet during this session, get out. */
//     if (response == -1 || (response == -2 && !*last_search)) {
//       statusbar_all(_("Cancelled"));
//       break;
//     }
//     /* If <Enter> was pressed, prepare to do a replace or a search. */
//     if (!response || response == -2) {
//       /* If an actual answer was typed, remember it. */
//       if (*answer) {
//         last_search = xstrcpy(last_search, answer);
//         update_history(&search_history, answer, PRUNE_DUPLICATE);
//       }
//       if (ISSET(USE_REGEXP) && !regexp_init(last_search)) {
//         break;
//       }
//       if (replacing) {
//         ask_for_and_do_replacements()
//       }
//     }
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

/* ----------------------------- Do replace loop ----------------------------- */

/* Step through each occurrence of the search string and prompt the user before replacing it.  We seek for needle,
 * and replace it with the answer.  The parameters `real_current` and `real_current_x` are needed in order to allow
 * the cursor position to be updated when a word before the cursor is replaced by a shorter word.  Returns `-1` if
 * needle isn't found, `-2` if the seeking is aborted, else the number of replacements performed. */
// long do_replace_loop_for(CTX_ARGS, const char *const restrict needle, bool whole_word_only, const linestruct *const real_current, Ulong *const real_current_x) {
//   ASSERT(file);
//   ASSERT(real_current);
//   ASSERT(real_current_x);
//   linestruct *was_mark = file->mark;
//   linestruct *top;
//   linestruct *bot;
//   char *altered;
//   Ulong top_x;
//   Ulong bot_x;
//   Ulong match_len;
//   Ulong length_change;
//   bool skipone       = ISSET(BACKWARDS_SEARCH);
//   bool replaceall    = FALSE;
//   bool right_side_up = (file->mark && mark_is_before_cursor_for(file));
//   long numreplaced   = -1;
//   int modus          = REPLACING;
//   int choice;
//   int result;
//   /* The mark in file is set. */
//   if (file->mark) {
//     /* Get the marked region in file, then set the mark in file to NULL. */
//     get_region_for(file, &top, &top_x, &bot, &bot_x);
//     file->mark = NULL;
//     /* Also set the mode to `INREGION`. */
//     modus = INREGION;
//     /* When forwards searching, start at the top of the marked region. */
//     if (!ISSET(BACKWARDS_SEARCH)) {
//       file->current   = top;
//       file->current_x = top_x;
//     }
//     /* Otherwise, when backwards searching, start at the bottom. */
//     else {
//       file->current   = bot;
//       file->current_x = bot_x;
//     }
//   }
//   came_full_circle = FALSE;
//   while (TRUE) {
//     choice = NO;
//     result = findnextstr_for(file, needle, whole_word_only, modus, &match_len, skipone, real_current, *real_current_x);
//     /* If nothing more was found, or the user aborted, stop looping. */
//     if (result < 1) {
//       if (result < 0) {
//         /* It's a <Cancel> instead of <Not found>. */
//         numreplaced = -2;
//       }
//       break;
//     }
//     /* An occurrence outside of the marked region means we're done. */
//     if (was_mark && (file->current->lineno > bot->lineno || file->current->lineno < top->lineno
//     || (file->current == bot && (file->current_x + match_len) > bot_x) || (file->current == top && file->current_x < top_x))) {
//       break;
//     }
//     /* Indecate that we found the search string. */
//     if (numreplaced == -1) {
//       numreplaced = 0;
//     }
//     /* When we are not replacing everything in one go. */
//     if (!replaceall) {
//       spotlighted = TRUE;
//       light_from_col = xplustabs_for(file);
//       light_to_col   = wideness(file->current->data, (file->current_x + match_len));
//       /* Refresh the edit window, scrolling it if nessesary. */
//       edit_refresh_for(STACK_CTX);
//       /* TRANSLATORS: This is a prompt. */
//       choice = ask_user(YESORALLORNO, _("Replace this instance?"));
//       spotlighted = FALSE;
//       if (choice == CANCEL) {
//         break;
//       }
//       replaceall = (choice == ALL);
//       /* When "No" or moving backwards, the search routine should first move one character further before continuing. */
//       skipone = (!choice || ISSET(BACKWARDS_SEARCH));
//     }
//     if (choice == YES || replaceall) {
//       altered = replace_line()
//     }
//   }
// }

/* ----------------------------- Ask for and do replacement ----------------------------- */

/* Ask the user what to replace the search string with, and do the replacements */
// void ask_for_and_do_replacements_for()
