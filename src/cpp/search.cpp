/** @file search.cpp */
#include "../include/prototypes.h"


/* Have we reached the starting line again while searching? */
static bool came_full_circle = FALSE;
/* Whether we have compiled a regular expression for the search. */
// static bool have_compiled_regexp = FALSE;

/* Compile the given regular expression and store it in search_regexp.
 * Return TRUE if the expression is valid, and FALSE otherwise. */
// bool regexp_init(const char *regexp) {
//   int value = regcomp(&search_regexp, regexp, NANO_REG_EXTENDED | (ISSET(CASE_SENSITIVE) ? 0 : REG_ICASE));
//   /* If regex compilation failed, show the error message. */
//   if (value != 0) {
//     Ulong len = regerror(value, &search_regexp, NULL, 0);
//     char *str = (char *)nmalloc(len);
//     regerror(value, &search_regexp, str, len);
//     statusline(AHEM, _("Bad regex \"%s\": %s"), regexp, str);
//     free(str);
//     return FALSE;
//   }
//   have_compiled_regexp = TRUE;
//   return TRUE;
// }

/* Free a compiled regular expression, if one was compiled; and schedule a
 * full screen refresh when the mark is on, in case the cursor has moved. */
// void tidy_up_after_search(void) {
//   if (have_compiled_regexp) {
//     regfree(&search_regexp);
//     have_compiled_regexp = FALSE;
//   }
//   if (openfile->mark) {
//     refresh_needed = TRUE;
//   }
//   recook |= perturbed;
// }

/* Prepare the prompt and ask the user what to search for.  Keep looping
 * as long as the user presses a toggle, and only take action and exit
 * when <Enter> is pressed or a non-toggle shortcut was executed. */
static void search_init(bool replacing, bool retain_answer) {
  functionptrtype function;
  char *disp;
  int response;
  /* What will be searched for when the user types just <Enter>. */
  char *thedefault;
  /* If something was searched for earlier, include it in the prompt. */
  if (*last_search) {
    disp = display_string(last_search, 0, (COLS / 3), FALSE, FALSE);
    thedefault = (char *)nmalloc(strlen(disp) + 7);
    /* We use (COLS / 3) here because we need to see more on the line. */
    sprintf(thedefault, " [%s%s]", disp, ((long)breadth(last_search) > (COLS / 3)) ? "..." : "");
    free(disp);
  }
  else {
    thedefault = STRLTR_COPY_OF("");
  }
  while (TRUE) {
    /* Ask the user what to search for (or replace). */
    response = do_prompt(
      (inhelp ? MFINDINHELP : (replacing ? MREPLACE : MWHEREIS)),
      (retain_answer ? answer : ""),
      &search_history,
      edit_refresh,
      /* TRANSLATORS: This is the main search prompt. */
      "%s%s%s%s%s%s", _("Search"),
      /* TRANSLATORS: The next four modify the search prompt. */
      (ISSET(CASE_SENSITIVE) ? _(" [Case Sensitive]") : ""),
      (ISSET(USE_REGEXP) ? _(" [Regexp]") : ""),
      (ISSET(BACKWARDS_SEARCH) ? _(" [Backwards]") : ""),
      (replacing ? openfile->mark ? _(" (to replace) in selection") : _(" (to replace)") : ""),
      thedefault
    );
    /* If the search was cancelled, or we have a blank answer and nothing was searched for yet during this session, get out. */
    if (response == -1 || (response == -2 && !*last_search)) {
      statusbar(_("Cancelled"));
      break;
    }
    /* If Enter was pressed, prepare to do a replace or a search. */
    if (response == 0 || response == -2) {
      /* If an actual answer was typed, remember it. */
      if (*answer) {
        last_search = realloc_strcpy(last_search, answer);
        update_history(&search_history, answer, PRUNE_DUPLICATE);
      }
      if (ISSET(USE_REGEXP) && !regexp_init(last_search)) {
        break;
      }
      if (replacing) {
        ask_for_and_do_replacements();
      }
      else {
        go_looking();
      }
      break;
    }
    retain_answer = TRUE;
    function      = func_from_key(response);
    /* If we're here, one of the five toggles was pressed, or a shortcut was executed. */
    if (function == case_sens_void) {
      TOGGLE(CASE_SENSITIVE);
    }
    else if (function == backwards_void) {
      TOGGLE(BACKWARDS_SEARCH);
    }
    else if (function == regexp_void) {
      TOGGLE(USE_REGEXP);
    }
    else if (function == flip_replace) {
      if (ISSET(VIEW_MODE)) {
        print_view_warning();
        napms(600);
      }
      else {
        replacing = !replacing;
      }
    }
    else if (function == flip_goto) {
      goto_line_and_column(openfile->current->lineno, (openfile->placewewant + 1), TRUE, TRUE);
      break;
    }
    else {
      break;
    }
  }
  if (!inhelp) {
    tidy_up_after_search();
  }
  free(thedefault);
}

/* Look for needle, starting at (current, current_x).  Begin is the line where we first started searching,
 * at column begin_x.  Return 1 when we found something, 0 when nothing, and -2 on cancel.  When match_len is
 * not NULL, set it to the length of the found string, if any.  TODO: (findnextstr) - This can be usefull. */
int findnextstr(const char *needle, bool whole_word_only, int modus, Ulong *match_len, bool skipone, const linestruct *begin, Ulong begin_x) {
  /* The length of a match -- will be recomputed for a regex. */
  Ulong found_len = strlen(needle);
  /* When bigger than zero, show and wipe the "Searching..." message. */
  int feedback = 0;
  /* The line that we will search through now. */
  linestruct *line = openfile->current;
  /* The point in the line from where we start searching. */
  const char *from = (line->data + openfile->current_x);
  /* A pointer to the location of the match, if any. */
  const char *found = NULL;
  /* The x coordinate of a found occurrence. */
  Ulong found_x;
  /* The time we last looked at the keyboard. */
  time_t lastkbcheck = time(NULL);
  /* Set non-blocking input so that we can just peek for a Cancel. */
  nodelay(midwin, TRUE);
  !begin ? (came_full_circle = FALSE) : 0;
  while (TRUE) {
    /* When starting a new search, skip the first character, then (in either case) search for the needle in the current line. */
    if (skipone) {
      skipone = FALSE;
      if (ISSET(BACKWARDS_SEARCH) && from != line->data) {
        from  = (line->data + step_left(line->data, (from - line->data)));
        found = strstrwrapper(line->data, needle, from);
      }
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
      /* When we're spell checking, a match should be a separate word;
       * if it's not, continue looking in the rest of the line. */
      if (whole_word_only && !is_separate_word((found - line->data), found_len, line->data)) {
        from = (found + char_length(found));
        continue;
      }
      /* When not on the magic line, the match is valid. */
      if (line->next || line->data[0]) {
        break;
      }
    }
    if (the_window_resized) {
      regenerate_screen();
      nodelay(midwin, TRUE);
      statusbar(_("Searching..."));
      feedback = 1;
    }
    /* If we're back at the beginning, then there is no needle. */
    if (came_full_circle) {
      nodelay(midwin, FALSE);
      return 0;
    }
    /* Move to the previous or next line in the file. */
    line = (ISSET(BACKWARDS_SEARCH) ? line->prev : line->next);
    /* If we've reached the start or end of the buffer, wrap around; but stop when spell-checking or replacing in a region. */
    if (!line) {
      if (whole_word_only || modus == INREGION) {
        nodelay(midwin, FALSE);
        return 0;
      }
      line = (ISSET(BACKWARDS_SEARCH) ? openfile->filebot : openfile->filetop);
      if (modus == JUSTFIND) {
        statusline(REMARK, _("Search Wrapped"));
        /* Delay the "Searching..." message for at least two seconds. */
        feedback = -2;
      }
    }
    /* If we've reached the original starting line, take note. */
    (line == begin) ? (came_full_circle = TRUE) : 0;
    /* Set the starting x to the start or end of the line. */
    from = line->data;
    if (ISSET(BACKWARDS_SEARCH)) {
      from += strlen(line->data);
    }
    /* Glance at the keyboard once every second, to check for a Cancel. */
    if ((time(NULL) - lastkbcheck) > 0) {
      int input   = wgetch(midwin);
      lastkbcheck = time(NULL);
      /* Consume any queued-up keystrokes, until a Cancel or nothing. */
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
          statusbar(_("Cancelled"));
          /* Clear out the key buffer (in case a macro is running). */
          while (input != ERR) {
            input = get_input(NULL);
          }
          nodelay(midwin, FALSE);
          return -2;
        }
        input = wgetch(midwin);
      }
      if (++feedback > 0) {
        /* TRANSLATORS: This is shown when searching takes more than half a second. */
        statusbar(_("Searching..."));
      }
    }
  }
  found_x = (found - line->data);
  nodelay(midwin, FALSE);
  /* Ensure that the found occurrence is not beyond the starting x. */
  if (came_full_circle && ((!ISSET(BACKWARDS_SEARCH) && (found_x > begin_x || (modus == REPLACING && found_x == begin_x))) || (ISSET(BACKWARDS_SEARCH) && found_x < begin_x))) {
    return 0;
  }
  /* Set the current position to point at what we found. */
  openfile->current   = line;
  openfile->current_x = found_x;
  /* When requested, pass back the length of the match. */
  match_len ? (*match_len = found_len) : 0;
  if (modus == JUSTFIND && (!openfile->mark || openfile->softmark)) {
    spotlighted    = TRUE;
    light_from_col = xplustabs();
    light_to_col   = wideness(line->data, (found_x + found_len));
    refresh_needed = TRUE;
  }
  if (feedback > 0) {
    wipe_statusbar();
  }
  return 1;
}

/* Ask for a string and then search forward for it. */
void do_search_forward(void) {
  UNSET(BACKWARDS_SEARCH);
  search_init(FALSE, FALSE);
}

/* Ask for a string and then search backwards for it. */
void do_search_backward(void) {
  SET(BACKWARDS_SEARCH);
  search_init(FALSE, FALSE);
}

/* Search for the last string without prompting. */
static void do_research(void) {
  /* If nothing was searched for yet during this run of nano, but there is a search history, take the most recent item. */
  if (!*last_search && searchbot->prev) {
    last_search = realloc_strcpy(last_search, searchbot->prev->data);
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
  go_looking();
  if (!inhelp) {
    tidy_up_after_search();
  }
}

/* Search in the backward direction for the next occurrence. */
void do_findprevious(void) {
  SET(BACKWARDS_SEARCH);
  do_research();
}

/* Search in the forward direction for the next occurrence. */
void do_findnext(void) {
  UNSET(BACKWARDS_SEARCH);
  do_research();
}

/* Report on the status bar that the given string was not found. */
void not_found_msg(const char *str) _NOTHROW {
  char *disp     = display_string(str, 0, ((COLS / 2) + 1), FALSE, FALSE);
  Ulong numchars = actual_x(disp, wideness(disp, (COLS / 2)));
  statusline(AHEM, _("\"%.*s%s\" not found"), numchars, disp, (disp[numchars] == '\0') ? "" : "...");
  free(disp);
}

/* Search for the global string 'last_search'.  Inform the user when the string occurs only once. */
void go_looking(void) {
  linestruct *was_current   = openfile->current;
  Ulong       was_current_x = openfile->current_x;
/* #define TIMEIT 12 */
#ifdef TIMEIT
#  include <ctime>
  clock_t start = clock();
#endif
  came_full_circle = FALSE;
  didfind          = findnextstr(last_search, FALSE, JUSTFIND, NULL, TRUE, openfile->current, openfile->current_x);
  /* If we found something, and we're back at the exact same spot
   * where we started searching, then this is the only occurrence. */
  if (didfind == 1 && openfile->current == was_current && openfile->current_x == was_current_x) {
    statusline(REMARK, _("This is the only occurrence"));
  }
  else if (!didfind) {
    not_found_msg(last_search);
  }
#ifdef TIMEIT
  statusline(INFO, "Took: %.2f", static_cast<f64>(clock() - start) / CLOCKS_PER_SEC);
#endif
  edit_redraw(was_current, CENTERING);
}

/* Calculate the size of the replacement text, taking possible subexpressions \1 to \9 into account.
 * Return the replacement text in the passed string only when create is TRUE. */
static int replace_regexp(char *string, bool create) _NOTHROW {
  Ulong replacement_size=0, i;
  const char *c = answer;
  int num;
  /* Iterate through the replacement text to handle subexpression replacement using \1, \2, \3, etc. */
  while (*c) {
    num = (*(c + 1) - '0');
    if (*c != '\\' || num < 1 || num > 9 || num > (long)search_regexp.re_nsub) {
      create ? *string++ = *c : 0;
      ++c;
      ++replacement_size;
    }
    else {
      i = (regmatches[num].rm_eo - regmatches[num].rm_so);
      /* Skip over the replacement expression. */
      c += 2;
      /* But add the length of the subexpression to new_size. */
      replacement_size += i;
      /* And if create is TRUE, append the result of the subexpression match to the new line. */
      if (create) {
        strncpy(string, (openfile->current->data + regmatches[num].rm_so), i);
        string += i;
      }
    }
  }
  create ? (*string = '\0') : 0;
  return replacement_size;
}

/* Return a copy of the current line with one needle replaced. */
static char *replace_line(const char *needle) {
  Ulong new_size = (strlen(openfile->current->data) + 1);
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
  copy = (char *)xmalloc(new_size);
  /* Copy the head of the original line. */
  strncpy(copy, openfile->current->data, openfile->current_x);
  /* Add the replacement text. */
  if (ISSET(USE_REGEXP)) {
    replace_regexp((copy + openfile->current_x), TRUE);
  }
  else {
    strcpy((copy + openfile->current_x), answer);
  }
  /* Copy the tail of the original line. */
  strcat(copy, (openfile->current->data + openfile->current_x + match_len));
  return copy;
}

/* Step through each occurrence of the search string and prompt the user before replacing it.  We seek for needle,
 * and replace it with answer.  The parameters real_current and real_current_x are needed in order to allow the
 * cursor position to be updated when a word before the cursor is replaced by a shorter word.  Return -1 if needle
 * isn't found, -2 if the seeking is aborted, else the number of replacements performed. */
long do_replace_loop(const char *needle, bool whole_word_only, const linestruct *real_current, Ulong *real_current_x) {
  linestruct *was_mark = openfile->mark;
  linestruct *top, *bot;
  Ulong top_x, bot_x;
  bool  skipone       = ISSET(BACKWARDS_SEARCH);
  bool  replaceall    = FALSE;
  int   modus         = REPLACING;
  bool  right_side_up = (openfile->mark && mark_is_before_cursor());
  long  numreplaced   = -1;
  Ulong match_len;
  /* If the mark is on, frame the region, and turn the mark off. */
  if (openfile->mark) {
    get_region(&top, &top_x, &bot, &bot_x);
    openfile->mark = NULL;
    modus = INREGION;
    /* Start either at the top or the bottom of the marked region. */
    if (!ISSET(BACKWARDS_SEARCH)) {
      openfile->current   = top;
      openfile->current_x = top_x;
    }
    else {
      openfile->current   = bot;
      openfile->current_x = bot_x;
    }
  }
  came_full_circle = FALSE;
  while (TRUE) {
    int choice = NO;
    int result = findnextstr(needle, whole_word_only, modus, &match_len, skipone, real_current, *real_current_x);
    /* If nothing more was found, or the user aborted, stop looping. */
    if (result < 1) {
      if (result < 0) {
        /* It's a Cancel instead of Not found. */
        numreplaced = -2;
      }
      break;
    }
    /* An occurrence outside of the marked region means we're done. */
    if (was_mark && (openfile->current->lineno > bot->lineno || openfile->current->lineno < top->lineno
     || (openfile->current == bot && (openfile->current_x + match_len) > bot_x) || (openfile->current == top && openfile->current_x < top_x))) {
      break;
    }
    /* Indicate that we found the search string. */
    if (numreplaced == -1) {
      numreplaced = 0;
    }
    if (!replaceall) {
      spotlighted    = TRUE;
      light_from_col = xplustabs();
      light_to_col   = wideness(openfile->current->data, (openfile->current_x + match_len));
      /* Refresh the edit window, scrolling it if necessary. */
      edit_refresh();
      /* TRANSLATORS: This is a prompt. */
      choice = ask_user(YESORALLORNO, _("Replace this instance?"));
      spotlighted = FALSE;
      if (choice == CANCEL) {
        break;
      }
      replaceall = (choice == ALL);
      /* When "No" or moving backwards, the search routine should first move one character further before continuing. */
      skipone = (!choice || ISSET(BACKWARDS_SEARCH));
    }
    if (choice == YES || replaceall) {
      Ulong length_change;
      char *altered;
      altered = replace_line(needle);
      length_change = (strlen(altered) - strlen(openfile->current->data));
      add_undo(REPLACE, NULL);
      /* If the mark was on and it was located after the cursor, then adjust its x position for any text length changes. */
      if (was_mark && !right_side_up) {
        if (openfile->current == was_mark && openfile->mark_x > openfile->current_x) {
          if (openfile->mark_x < (openfile->current_x + match_len)) {
            openfile->mark_x = openfile->current_x;
          }
          else {
            openfile->mark_x += length_change;
          }
          bot_x = openfile->mark_x;
        }
      }
      /* If the mark was not on or it was before the cursor, then adjust the cursor's x position for any text length changes. */
      if (!was_mark || right_side_up) {
        if (openfile->current == real_current && openfile->current_x < *real_current_x) {
          if (*real_current_x < (openfile->current_x + match_len)) {
            *real_current_x = (openfile->current_x + match_len);
          }
          *real_current_x += length_change;
          bot_x = *real_current_x;
        }
      }
      /* Don't find the same zero-length or BOL match again. */
      if (!match_len || (*needle == '^' && ISSET(USE_REGEXP))) {
        skipone = TRUE;
      }
      /* When moving forward, put the cursor just after the replacement text, so that searching will continue there. */
      if (!ISSET(BACKWARDS_SEARCH)) {
        openfile->current_x += (match_len + length_change);
      }
      /* Update the file size, and put the changed line into place. */
      openfile->totsize += (mbstrlen(altered) - mbstrlen(openfile->current->data));
      free(openfile->current->data);
      openfile->current->data = altered;
      check_the_multis(openfile->current);
      refresh_needed = FALSE;
      set_modified();
      as_an_at = TRUE;
      ++numreplaced;
    }
  }
  if (numreplaced == -1) {
    not_found_msg(needle);
  }
  openfile->mark = was_mark;
  return numreplaced;
}

/* Replace a string. */
void do_replace(void) {
  if (ISSET(VIEW_MODE)) {
    print_view_warning();
  }
  else {
    UNSET(BACKWARDS_SEARCH);
    search_init(TRUE, FALSE);
  }
}

/* Ask the user what to replace the search string with, and do the replacements. */
void ask_for_and_do_replacements(void) {
  linestruct *was_edittop = openfile->edittop;
  linestruct *beginline   = openfile->current;
  Ulong was_firstcolumn = openfile->firstcolumn;
  Ulong begin_x         = openfile->current_x;
  char *replacee        = copy_of(last_search);
  long  numreplaced;
  int   response = do_prompt(MREPLACEWITH, "", &replace_history, /* TRANSLATORS: This is a prompt. */ edit_refresh, _("Replace with"));
  /* Set the string to be searched, as it might have changed at the prompt. */
  free(last_search);
  last_search = replacee;
  /* When not "", add the replace string to the replace history list. */
  if (response == 0) {
    update_history(&replace_history, answer, PRUNE_DUPLICATE);
  }
  /* When cancelled, or when a function was run, get out. */
  if (response == -1) {
    statusbar(_("Cancelled"));
    return;
  }
  else if (response > 0) {
    return;
  }
  numreplaced = do_replace_loop(last_search, FALSE, beginline, &begin_x);
  /* Restore where we were. */
  openfile->edittop     = was_edittop;
  openfile->firstcolumn = was_firstcolumn;
  openfile->current     = beginline;
  openfile->current_x   = begin_x;
  refresh_needed        = TRUE;
  if (numreplaced >= 0) {
    statusline(REMARK, P_("Replaced %zd occurrence", "Replaced %zd occurrences", numreplaced), numreplaced);
  }
}

/* Go to the specified line and x position. */
// void goto_line_posx(long linenumber, Ulong pos_x) _NOTHROW {
//   if (linenumber > (openfile->edittop->lineno + editwinrows) || (ISSET(SOFTWRAP) && linenumber > openfile->current->lineno)) {
//     recook |= perturbed;
//   }
//   if (linenumber < openfile->filebot->lineno) {
//     openfile->current = line_from_number(linenumber);
//   }
//   else {
//     openfile->current = openfile->filebot;
//   }
//   openfile->current_x   = pos_x;
//   openfile->placewewant = xplustabs();
//   refresh_needed = TRUE;
// }

/* Go to the specified line and column, or ask for them if interactive is TRUE.  In the latter case also
 * update the screen afterwards.  Note that both the line and column number should be one-based. */
void goto_line_and_column(long line, long column, bool retain_answer, bool interactive) {
  int response, rows_from_tail;
  linestruct *currentline;
  Ulong leftedge;
  if (interactive) {
    /* Ask for the line and column.  TRANSLATOR: This is a prompt. */
    response = do_prompt(MGOTOLINE, retain_answer ? answer : "", NULL, edit_refresh, _("Enter line number, column number"));
    /* If the user cancelled or gave a blank answer, get out. */
    if (response < 0) {
      statusbar(_("Cancelled"));
      return;
    }
    if (func_from_key(response) == flip_goto) {
      UNSET(BACKWARDS_SEARCH);
      /* Switch to searching but retain what the user typed so far. */
      search_init(FALSE, TRUE);
      return;
    }
    /* If a function was executed, we're done here. */
    if (response > 0) {
      return;
    }
    /* Try to extract one or two numbers from the user's response. */
    if (!parse_line_column(answer, &line, &column)) {
      statusline(AHEM, _("Invalid line or column number"));
      return;
    }
  }
  else {
    if (!line) {
      line = openfile->current->lineno;
    }
    if (!column) {
      column = (openfile->placewewant + 1);
    }
  }
  /* Take a negative line number to mean: from the end of the file. */
  if (line < 0) {
    line = (openfile->filebot->lineno + line + 1);
  }
  if (line < 1) {
    line = 1;
  }
  if (line > (openfile->edittop->lineno + editwinrows) || (ISSET(SOFTWRAP) && line > openfile->current->lineno)) {
    recook |= perturbed;
  }
  /* Iterate to the requested line. */
  for (openfile->current = openfile->filetop; line > 1 && openfile->current != openfile->filebot; line--) {
    openfile->current = openfile->current->next;
  }
  /* Take a negative column number to mean: from the end of the line. */
  if (column < 0) {
    column = breadth(openfile->current->data) + column + 2;
  }
  if (column < 1) {
    column = 1;
  }
  /* Set the x position that corresponds to the requested column. */
  openfile->current_x   = actual_x(openfile->current->data, (column - 1));
  openfile->placewewant = (column - 1);
  if (ISSET(SOFTWRAP) && (openfile->placewewant / editwincols) > (breadth(openfile->current->data) / editwincols)) {
    openfile->placewewant = breadth(openfile->current->data);
  }
  /* When a line number was manually given, center the target line. */
  if (interactive) {
    adjust_viewport((*answer == ',') ? STATIONARY : CENTERING);
    refresh_needed = TRUE;
  }
  else {
    if (ISSET(SOFTWRAP)) {
      currentline    = openfile->current;
      leftedge       = leftedge_for(xplustabs(), openfile->current, editwincols);
      rows_from_tail = ((editwinrows / 2) - go_forward_chunks((editwinrows / 2), &currentline, &leftedge));
    }
    else {
      rows_from_tail = (openfile->filebot->lineno - openfile->current->lineno);
    }
    /* If the target line is close to the tail of the file, put the last line or chunk
     * on the bottom line of the screen; otherwise, just center the target line. */
    if (rows_from_tail < editwinrows / 2 && !ISSET(JUMPY_SCROLLING)) {
      openfile->cursor_row = (editwinrows - 1 - rows_from_tail);
      adjust_viewport(STATIONARY);
    }
    else {
      adjust_viewport(CENTERING);
    }
  }
}

/* Go to the specified line and column, asking for them beforehand. */
void do_gotolinecolumn(void) {
  goto_line_and_column(openfile->current->lineno, (openfile->placewewant + 1), FALSE, TRUE);
}

/* Search, starting from the current position, for any of the two characters in bracket_pair.  If reverse is TRUE,
 * search backwards, otherwise forwards.  Return TRUE when one of the brackets was found, and FALSE otherwise. */
bool find_a_bracket(bool reverse, const char *bracket_pair) _NOTHROW {
  linestruct *line = openfile->current;
  const char *pointer, *found;
  if (reverse) {
    /* First step away from the current bracket. */
    if (!openfile->current_x) {
      line = line->prev;
      if (!line) {
        return FALSE;
      }
      pointer = (line->data + strlen(line->data));
    }
    else {
      pointer = (line->data + step_left(line->data, openfile->current_x));
    }
    /* Now seek for any of the two brackets we are interested in. */
    while (!(found = mbrevstrpbrk(line->data, bracket_pair, pointer))) {
      line = line->prev;
      if (!line) {
        return FALSE;
      }
      pointer = (line->data + strlen(line->data));
    }
  }
  else {
    pointer = (line->data + step_right(line->data, openfile->current_x));
    while (!(found = mbstrpbrk(pointer, bracket_pair))) {
      line = line->next;
      if (!line) {
        return FALSE;
      }
      pointer = line->data;
    }
  }
  /* Set the current position to the found bracket. */
  openfile->current   = line;
  openfile->current_x = (found - line->data);
  return TRUE;
}

/* Search for a match to the bracket at the current cursor position, if there is one. */
void do_find_bracket(void) _NOTHROW {
  linestruct *was_current = openfile->current;
  /* The current cursor position, in case we don't find a complement. */
  Ulong was_current_x = openfile->current_x;
  /* The location in matchbrackets of the bracket under the cursor. */
  const char *ch;
  /* The length of ch in bytes. */
  int ch_len;
  /* The location in matchbrackets of the complementing bracket. */
  const char *wanted_ch;
  /* The length of wanted_ch in bytes. */
  int wanted_ch_len;
  /* The pair of characters in ch and wanted_ch. */
  char bracket_pair[MAXCHARLEN * 2 + 1];
  /* The index in matchbrackets where the closing brackets start. */
  Ulong halfway = 0;
  /* Half the number of characters in matchbrackets. */
  Ulong charcount = mbstrlen(matchbrackets) / 2;
  /* The initial bracket count. */
  Ulong balance = 1;
  /* The direction we search. */
  bool reverse;
  ch = mbstrchr(matchbrackets, (openfile->current->data + openfile->current_x));
  if (!ch) {
    statusline(AHEM, _("Not a bracket"));
    return;
  }
  /* Find the halfway point in matchbrackets, where the closing ones start. */
  for (Ulong i = 0; i < charcount; ++i) {
    halfway += char_length(matchbrackets + halfway);
  }
  /* When on a closing bracket, we have to search backwards for a matching
   * opening bracket; otherwise, forward for a matching closing bracket. */
  reverse = (ch >= (matchbrackets + halfway));
  /* Step half the number of total characters either backwards or forwards
   * through matchbrackets to find the wanted complementary bracket. */
  wanted_ch = ch;
  while (charcount-- > 0) {
    if (reverse) {
      wanted_ch = (matchbrackets + step_left(matchbrackets, (wanted_ch - matchbrackets)));
    }
    else {
      wanted_ch += char_length(wanted_ch);
    }
  }
  ch_len        = char_length(ch);
  wanted_ch_len = char_length(wanted_ch);
  /* Copy the two complementary brackets into a single string. */
  strncpy(bracket_pair, ch, ch_len);
  strncpy((bracket_pair + ch_len), wanted_ch, wanted_ch_len);
  bracket_pair[ch_len + wanted_ch_len] = '\0';
  while (find_a_bracket(reverse, bracket_pair)) {
    /* Increment/decrement balance for an identical/other bracket. */
    balance += ((strncmp((openfile->current->data + openfile->current_x), ch, ch_len) == 0) ? 1 : -1);
    /* When balance reached zero, we've found the complementary bracket. */
    if (balance == 0) {
      edit_redraw(was_current, FLOWING);
      return;
    }
  }
  statusline(AHEM, _("No matching bracket"));
  /* Restore the cursor position. */
  openfile->current   = was_current;
  openfile->current_x = was_current_x;
}

/* Place an anchor at the current line when none exists, otherwise remove it. */
void put_or_lift_anchor(void) {
  openfile->current->has_anchor = !openfile->current->has_anchor;
  update_line(openfile->current, openfile->current_x);
  if (openfile->current->has_anchor) {
    statusline(REMARK, _("Placed anchor"));
  }
  else {
    statusline(REMARK, _("Removed anchor"));
  }
}

/* Make the given line the current line, or report the anchoredness. */
static void go_to_and_confirm(linestruct *line) {
  linestruct *was_current = openfile->current;
  if (line != openfile->current) {
    openfile->current   = line;
    openfile->current_x = 0;
    if (line->lineno > openfile->edittop->lineno + editwinrows || (ISSET(SOFTWRAP) && line->lineno > was_current->lineno)) {
      recook |= perturbed;
    }
    edit_redraw(was_current, CENTERING);
    statusbar(_("Jumped to anchor"));
  }
  else if (openfile->current->has_anchor) {
    statusline(REMARK, _("This is the only anchor"));
  }
  else {
    statusline(AHEM, _("There are no anchors"));
  }
}

/* Jump to the first anchor before the current line; wrap around at the top. */
void to_prev_anchor(void) {
  linestruct *line = openfile->current;
  do {
    line = (line->prev ? line->prev : openfile->filebot);
  } while (!line->has_anchor && line != openfile->current);
  go_to_and_confirm(line);
}

/* Jump to the first anchor after the current line; wrap around at the bottom. */
void to_next_anchor(void) {
  linestruct *line = openfile->current;
  do {
    line = (line->next ? line->next : openfile->filetop);
  } while (!line->has_anchor && line != openfile->current);
  go_to_and_confirm(line);
}

// static bool search_file_in_dir(const char *file, const char *dir) {
//   DIR *d = opendir(dir);
//   dirent *e;
//   if (!d) {
//     return FALSE;
//   }
//   while ((e = readdir(d))) {
//     if (strcmp(file, e->d_name) == 0) {
//       return TRUE;
//     }
//   }
//   return FALSE;
// }

/* Find a global header and return the full path. */
// char *find_global_header(const char *str) {
//   PROFILE_FUNCTION;
//   const char *end = tail(str);
//   if (end != str) {
//     const char *subpath     = substr(str, (end - str));
//     const char *search_path = concat_path("/usr/include/", subpath);
//     if (search_file_in_dir(end, search_path)) {
//       char *data = memmove_concat(search_path, end);
//       return data;
//     }
//   }
//   else if (search_file_in_dir(str, "/usr/include/")) {
//     Ulong slen = strlen(str);
//     char *data = (char *)nmalloc("/usr/include/"_sllen + slen + 1);
//     memmove(data, "/usr/include/", "/usr/include/"_sllen);
//     memmove(data + "/usr/include/"_sllen, str, slen);
//     data["/usr/include/"_sllen + slen] = '\0';
//     nlog("found %s\n", data);
//     return data;
//   }
//   return NULL;
// }

// char *find_local_header(const char *str) {
//   const char *current_dir = NULL, *tail_end = NULL;
//   char        buf[PATH_MAX];
//   memset(buf, 0, sizeof(buf));
//   current_dir = getenv("PWD");
//   tail_end    = tail(openfile->filename);
//   if (!current_dir || !tail_end) {
//     return NULL;
//   }
//   current_dir = concat_path(current_dir, substr(openfile->filename, (tail_end - openfile->filename) - 1));
//   memmove(buf, current_dir, strlen(current_dir));
//   const char *prev = strstr(str, "..");
//   while (prev != NULL) {
//     buf[(tail(buf) - buf) - 1] = '\0';
//     prev += 3;
//     prev = strstr(prev, "..");
//     str += 3;
//   }
//   strcat(buf, "/");
//   strcat(buf, str);
//   return measured_copy(buf, strlen(buf));
// }
