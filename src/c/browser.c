/** @file browser.c

  @author  Melwin Svensson.
  @date    1-6-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The list of files to display in the file browser. */
static char **filelist = NULL;
/* The number of files in the list. */
static Ulong list_length = 0;
/* The number of screen rows we can use to display the list. */
static Ulong usable_rows = 0;
/* The number of files that we can display per screen row. */
static int piles = 0;
/* The width of a 'pile' -- the widest filename plus ten. */
static int gauge = 0;
/* The currently selected filename in the list; zero-based. */
static Ulong selected = 0;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Read the list ----------------------------- */

/* Fill 'filelist' with the names of the files in the given directory, set 'list_length' to the
 * number of names in that list, set 'gauge' to the width of the widest filename plus ten, and
 * set 'piles' to the number of files that can be displayed per screen row.  And sort the list too. */
static void read_the_list(const char *path, DIR *dir) {
  Ulong path_len = strlen(path);
  Ulong widest = 0;
  Ulong index  = 0;
  Ulong span;
  const struct dirent *entry;
  /* Find the width of the widest filename in the current folder. */
  while ((entry = readdir(dir))) {
    span = breadth(entry->d_name);
    if (span > widest) {
      widest = span;
    }
    ++index;
  }
  /* Reserve ten columns for blanks plus file size. */
  gauge = (widest + 10);
  /* If needed, make room for ".. (parent dir)". */
  if (gauge < 15) {
    gauge = 15;
  }
  /* Make sure we're not wider than the window. */
  if (gauge > COLS) {
    gauge = COLS;
  }
  rewinddir(dir);
  free_chararray(filelist, list_length);
  list_length = index;
  index = 0;
  filelist = xmalloc(list_length * _PTRSIZE);
  while ((entry = readdir(dir)) && index < list_length) {
    /* Don't show the useless dot item. */
    if (strcmp(entry->d_name, ".") == 0) {
      continue;
    }
    filelist[index] = xmalloc(path_len + _D_ALLOC_NAMLEN(entry));
    sprintf(filelist[index], "%s%s", path, entry->d_name);
    ++index;
  }
  /* Maybe the number of files in the directory decreased between the first time we scanned and the second time. */
  list_length = index;
  /* Sort the list of names. */
  qsort(filelist, list_length, _PTRSIZE, diralphasort);
  /* Calculate how many files fit on a line -- feigning room for two spaces
   * beyond the right edge, and adding two spaces of padding between columns. */
  piles = ((COLS + 2) / (gauge + 2));
  usable_rows = (editwinrows - (ISSET(ZERO) && (LINES > 1) ? 1 : 0));
}

/* ----------------------------- Reselect ----------------------------- */

/* Reselect the given file or directory name, if it still exists.  Note: Make static later. */
static void reselect(const char *const name) {
  Ulong looking_at = 0;
  while (looking_at < list_length && strcmp(filelist[looking_at], name) != 0) {
    ++looking_at;
  }
  /* If the sought name was found, select it; otherwise, just move the highlight so that the
   * changed selection will be noticed, but make sure to stay within the current available range. */
  if (looking_at < list_length) {
    selected = looking_at;
  }
  else if (selected > list_length) {
    selected = (list_length - 1);
  }
  else {
    --selected;
  }
}

/* ----------------------------- Findfile ----------------------------- */

/* Look for the given needle in the list of files, forwards or backwards. Note: Make static later.  */
static void findfile(const char *needle, bool forwards) {
  Ulong began_at = selected;
  /* Iterate through the list of filenames, until a match is found or
   * we've come back to the point where we started. */
  while (TRUE) {
    if (forwards) {
      if (selected++ == (list_length - 1)) {
        selected = 0;
        statusbar_all(_("Search Wrapped"));
      }
    }
    else {
      if (selected-- == 0) {
        selected = (list_length - 1);
        statusbar_all(_("Search Wrapped"));
      }
    }
    /* When the needle occurs in the basename of the file, we have a match. */
    if (mbstrcasestr(tail(filelist[selected]), needle)) {
      if (selected == began_at) {
        statusbar_all(_("This is the only occurrence"));
      }
      return;
    }
    /* When we're back at the starting point without any match... */
    if (selected == began_at) {
      not_found_msg(needle);
      return;
    }
  }
}

/* ----------------------------- Search filename ----------------------------- */

/* Prepare the prompt and ask the user what to search for, then search for it.
 * If `forward` is `TRUE`, search forward in the list, otherwise, search backward. */
static void search_filename(bool forward) {
  char *the_default;
  char *disp;
  int response;
  Ulong disp_breadth;
  /* If something was searched before, show it between squere brackets. */
  if (*last_search) {
    disp         = display_string(last_search, 0, (COLS / 3), FALSE, FALSE);
    disp_breadth = breadth(disp);
    /* We use (COLS / 3) here because we need to see more then the line.  What...? */
    the_default = free_and_assign(disp, fmtstr(" [%s%s]", disp, (GT(disp_breadth, (COLS / 3)) ? "..." : "")));
  }
  else {
    the_default = COPY_OF("");
  }
  /* Now ask what to search for. */
  response = do_prompt(MWHEREISFILE, "", &search_history, browser_refresh, "%s%s%s", _("Search"),
    /* TRANSLATORS: A modifier of the search prompt. */ (!forward ? _(" [Backwards]") : ""), the_default);
  free(the_default);
  /* If the user cancelled, or typed <Enter> on a blank answer and nothing was searched for yet during this session, just leave. */
  if (response == -1 || (response == -2 && !*last_search)) {
    statusbar_all(_("Cancelled"));
    return;
  }
  /* If the user typed an answer, remember it. */
  if (*answer) {
    last_search = xstrcpy(last_search, answer);
    update_history(&search_history, answer, PRUNE_DUPLICATE);
  }
  if (!response || response == -2) {
    findfile(last_search, forward);
  }
}

/* ----------------------------- Research filename ----------------------------- */

/* Search again without prompting for the last given search string, either forwards or backwards. */
static void research_filename(bool forwards) {
  /* If nothing was searched for yet, take the last item from history. */
  if (!*last_search && searchbot->prev) {
    last_search = realloc_strcpy(last_search, searchbot->prev->data);
  }
  if (!*last_search) {
    statusbar_all(_("No current search pattern"));
  }
  else {
    wipe_statusbar();
    findfile(last_search, forwards);
  }
}

/* ----------------------------- Strip last component ----------------------------- */

/* Strip one element from the end of path, and return the stripped path.
 * The returned string is dynamically allocated, and should be freed. */
static char *strip_last_component(const char *const restrict path) {
  char *copy = copy_of(path);
  char *last_slash = strrchr(copy, '/');
  if (last_slash) {
    *last_slash = '\0';
  }
  return copy;
}

/* ----------------------------- Browse ----------------------------- */

/* Allow the user to browse through the directories in the filesystem, starting at the
 * given path.  The user can select a file, which will be returned.  The user can also
 * select a directory, which will be entered.  The user can also cancel the browsing. */
static char *browse(char *path) {
  /* The name of the currently selected file, or of the directory we were in before backing up to "...". */
  char *present_name = NULL;
  /* The number of the selected file before the current selected file. */
  Ulong old_selected;
  /* The directory whose contents we are showing. */
  DIR *dir;
  /* The name of the file that the user picked, or NULL if none. */
  char *chosen = NULL;
  functionptrtype function;
  int kbinput;
  int mx;
  int my;
  struct stat st;
  /* When not in curses-mode, just return NULL. */
  if (!IN_CURSES_CTX) {
    return NULL;
  }
  /* We come here when the user refreshes or selects a new directory. */
  read_directory_contents: {
    path = free_and_assign(path, get_full_path(path));
    if (path) {
      dir = opendir(path);
    }
    if (!path || !dir) {
      statusline(ALERT, _("Cannot open directory: %s"), strerror(errno));
      /* If we don't have a file list, there is nothing to show. */
      if (!filelist) {
        lastmessage = VACUUM;
        free(present_path);
        free(path);
        napms(1200);
        return NULL;
      }
      path         = xstrcpy(path, present_path);
      present_name = xstrcpy(present_name, filelist[selected]);
    }
    if (dir) {
      /* Get the file list, and set gauge and piles in the process. */
      read_the_list(path, dir);
      closedir(dir);
      dir = NULL;
    }
    /* If something was selected before, reselect it. */
    if (present_name) {
      reselect(present_name);
      free(present_name);
      present_name = NULL;
    }
    /* Otherwise, just select the first item (..). */
    else {
      selected = 0;
    }
    old_selected = (Ulong)-1;
    present_path = xstrcpy(present_path, path);
    titlebar(path);
    if (!list_length) {
      statusline(ALERT, _("No entries"));
      napms(1200);
    }
    else {
      while (TRUE) {
        lastmessage = VACUUM;
        bottombars(MBROWSER);
        /* Display (or redisplay) the file list if the list itself or the selected file has changed. */
        if (old_selected != selected || ISSET(SHOW_CURSOR)) {
          browser_refresh();
        }
        old_selected = selected;
        kbinput      = get_kbinput(midwin, ISSET(SHOW_CURSOR));
        /* Handle mouse events. */
        if (kbinput == KEY_MOUSE) {
          /* When the user clicked in the file list, select a filename. */
          if (get_mouseinput(&my, &mx, TRUE) == 0 && wmouse_trafo(midwin, &my, &mx, FALSE)) {
            selected = (selected - selected % (usable_rows * piles) + (my * piles) + (mx / (gauge + 2)));
            /* When beyond the end-of-row, select the preceding filename.  Well
             * -- this might not make for the most responsive feeling interface. */
            if (mx > (piles * (gauge + 2))) {
              --selected;
            }
            /* When beyond the end-of-list, select the last filename.  Again... */
            if (selected > (list_length - 1)) {
              selected = (list_length - 1);
            }
            /* When a filename is clicked a second time, choose it. */
            if (old_selected == selected) {
              kbinput = KEY_ENTER;
            }
          }
          if (kbinput == KEY_MOUSE) {
            continue;
          }
        }
        while (bracketed_paste) {
          kbinput = get_kbinput(midwin, BLIND);
        }
        if (kbinput == BRACKETED_PASTE_MARKER) {
          beep();
          continue;
        }
        function = interpret(kbinput);
        if (function == full_refresh || function == do_help) {
          function();
          /* Simulate a terminal resize to force a directory reload, or because the terminal dimensions might have changed */
          kbinput = KEY_WINCH;
        }
        else if (function == do_toggle) {
          if (get_shortcut(kbinput)->toggle == NO_HELP) {
            TOGGLE(NO_HELP);
            window_init();
            kbinput = KEY_WINCH;
          }
        }
        else if (function == do_search_backward) {
          search_filename(BACKWARD);
        }
        else if (function == do_search_forward) {
          search_filename(FORWARD);
        }
        else if (function == do_findprevious) {
          research_filename(BACKWARD);
        }
        else if (function == do_findnext) {
          research_filename(FORWARD);
        }
        else if (function == do_left) {
          if (selected > 0) {
            --selected;
          }
        }
        else if (function == do_right) {
          if (selected < (list_length - 1)) {
            ++selected;
          }
        }
        else if (function == to_prev_word) {
          selected -= (selected % piles);
        }
        else if (function == to_next_word) {
          selected += (piles - 1 - (selected % piles));
          if (selected >= list_length) {
            selected = (list_length - 1);
          }
        }
        else if (function == do_up) {
          if (GE(selected, piles)) {
            selected -= piles;
          }
        }
        else if (function == do_down) {
          if ((selected + piles) <= (list_length - 1)) {
            selected += piles;
          }
        }
        else if (function == to_prev_block) {
          selected = (((selected / (usable_rows * piles)) * usable_rows * piles) + selected % piles);
        }
        else if (function == to_next_block) {
          /* TODO: This seams wastfull, figure out what is needed. */
          selected = (((selected / (usable_rows * piles)) * usable_rows * piles) + selected % piles + usable_rows * piles - piles);
          if (selected >= list_length) {
            selected = ((list_length / piles) * piles + selected % piles);
          }
          if (selected >= list_length) {
            selected -= piles;
          }
        }
        else if (function == do_page_up) {
          if (LT(selected, piles)) {
            selected = 0;
          }
          else if (selected < (usable_rows * piles)) {
            selected = (selected % piles);
          }
          else {
            selected -= (usable_rows * piles);
          }
        }
        else if (function == do_page_down) {
          if ((selected + piles) >= (list_length - 1)) {
            selected = (list_length - 1);
          }
          else if ((selected + usable_rows * piles) >= list_length) {
            selected = ((selected + usable_rows * piles - list_length) % piles + list_length - piles);
          }
          else {
            selected += (usable_rows * piles);
          }
        }
        else if (function == to_first_file) {
          selected = 0;
        }
        else if (function == to_last_file) {
          selected = (list_length - 1);
        }
        else if (function == goto_dir) {
          /* Ask for a directory to go to. */
          if (do_prompt(MGOTODIR, "", NULL, /* TRANSLATORS: This is a prompt. */ browser_refresh, _("Go to directory")) < 0) {
            statusbar_all(_("Cancelled"));
            continue;
          }
          path = free_and_assign(path, real_dir_from_tilde(answer));
          /* If the given path is relative, join it with the current path. */
          if (*path != '/') {
            path = xrealloc(path, (strlen(present_path) + strlen(answer) + 1));
            sprintf(path, "%s%s", present_path, answer);
          }
          /* This refers to the confining effects of the option `--operatingdir`, not of `--restricted`. */
          if (outside_of_confinement(path, FALSE)) {
            statusline(ALERT, _("Can't go outside of %s"), operating_dir);
            path = xstrcpy(path, present_path);
            continue;
          }
          /* Snip any trailing slashes, so the name can be compared. */
          while (strlen(path) > 1 && path[strlen(path) - 1] == '/') {
            path[strlen(path) - 1] = '\0';
          }
          /* In case the specified directory cannot be entered, select it
           * (if it's in the current list) so it will be highlighted. */
          for (Ulong j=0; j<list_length; ++j) {
            if (strcmp(filelist[j], path) == 0) {
              selected = j;
            }
          }
          /* Try opening and reading the specified directory. */
          goto read_directory_contents;
        }
        else if (function == do_enter) {
          /* It isn't possible to move up from the root directory. */
          if (strcmp(filelist[selected], "/..") == 0) {
            statusline(ALERT, _("Can't move up a directory"));
            continue;
          }
          /* Note: The selected file can be outside the operating directory if it's ".."
           * or if it's a symlink to a directory outside the operating directory. */
          if (outside_of_confinement(filelist[selected], FALSE)) {
            statusline(ALERT, _("Can't go outside of %s"), operating_dir);
            continue;
          }
          /* If for some reason the file is inaccessible, complain. */
          if (stat(filelist[selected], &st) == -1) {
            statusline(ALERT, _("Error reading %s: %s"), filelist[selected], strerror(errno));
            continue;
          }
          /* If it isn't a directory, a file was selected -- we're done. */
          if (!S_ISDIR(st.st_mode)) {
            chosen = copy_of(filelist[selected]);
            break;
          }
          /* If we are moving up one level, remember where we came from,
           * so this directory can be highlighted and easily reentered. */
          if (strcmp(tail(filelist[selected]), "..") == 0) {
            present_path = strip_last_component(filelist[selected]);
          }
          /* Try opening and reading the selected directory. */
          path = xstrcpy(path, filelist[selected]);
          goto read_directory_contents;
        }
        else if (function == (functionptrtype)implant) {
          implant(first_sc_for(MBROWSER, function)->expansion);
        }
        else if (kbinput == KEY_WINCH) {
          ; /* Gets handled below. */
        }
        else if (function == do_exit) {
          break;
        }
        else {
          unbound_key(kbinput);
        }
        /* If the terminal resized (or might have), refresh the file list. */
        if (kbinput == KEY_WINCH) {
          /* Remember the selected file, to be able to reselect it. */
          present_path = copy_of(filelist[selected]);
          goto read_directory_contents;
        }
      }
    }
  }
  titlebar(NULL);
  edit_refresh();
  free(path);
  free_chararray(filelist, list_length);
  filelist    = NULL;
  list_length = 0;
  return chosen;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Browser refresh ----------------------------- */

/* Display at most a screenful of filenames from the gleaned filelist. */
void browser_refresh(void) {
  /* The current row and column while the list is getting displayed. */
  int row = 0;
  int col = 0;
  /* The row and column of the selected item. */
  int the_row    = 0;
  int the_column = 0;
  /* The additional information that we'll display about a file. */
  char *info;
  titlebar(present_path);
  blank_edit();
  for (Ulong index=(selected - selected % (usable_rows * piles)); (index < list_length) && LT(row, usable_rows); ++index) {
    /* The filename we display, minus the path. */
    const char *thename = tail(filelist[index]);
    /* The file extention. */
    const char *file_ext = ext(thename);
    /* The color to apply. */
    int apply_color = 0;
    /* Any mod to the color like bold or italic. */
    int apply_color_mod = 0;
    /* 'TRUE' if we apply color, otherwise 'FALSE'. */
    bool did_apply_color = FALSE;
    /* The length of the filename in columns. */
    Ulong namelen = breadth(thename);
    /* The length of the file information in columns. */
    Ulong infolen;
    /* The maximum length of the file information in columns: normally seven, but will be twelve for "(parent dir)". */
    Ulong infomaxlen = 7;
    /* Whether to put an ellipsis before the filename?  We don't waste space on dots when there are fewer than 15 columns. */
    bool dots = (COLS >= 15 && namelen >= (gauge - infomaxlen));
    /* The filename (or a fragment of it) in displayable format.  When a fragment, account for dots plus one space padding. */
    char *disp = display_string(thename, (dots ? (namelen + infomaxlen + 4 - gauge) : 0), gauge, FALSE, FALSE);
    struct stat state;
    /* Show information about the file: "--" for symlinks (except when they point to a directory)
     * and for files that have disappeared, '(dir)' for directories, and the file size for normal files. */
    if (lstat(filelist[index], &state) == -1 || S_ISLNK(state.st_mode)) {
      if (stat(filelist[index], &state) == -1 || !S_ISDIR(state.st_mode)) {
        info = copy_of("--");
        apply_color = FG_COMMENT_GREEN;
        did_apply_color = TRUE;
      }
      else {
        /* TRANSLATORS: Anything more than 7 cells gets clipped. */
        info = copy_of(_("(dir)"));
        apply_color = FG_VS_CODE_BLUE;
        apply_color_mod = A_BOLD;
        did_apply_color = TRUE;
      }
    }
    else if (S_ISDIR(state.st_mode)) {
      if (strcmp(thename, "..") == 0) {
        /* TRANSLATORS: Anything more than 12 cells gets clipped. */
        info = copy_of(_("(parent dir)"));
        infomaxlen = 12;
      }
      else {
        info = copy_of(_("(dir)"));
        apply_color = FG_VS_CODE_BLUE;
        apply_color_mod = A_BOLD;
        did_apply_color = TRUE;
      }
    }
    else {
      off_t result = state.st_size;
      char  modifier;
      info = xmalloc(infomaxlen + 1);
      /* Massage the file size into a human-readable form. */
      if (state.st_size < (1 << 10)) {
        /* Bytes. */
        modifier = ' ';
      }
      else if (state.st_size < (1 << 20)) {
        result >>= 10;
        /* Kilobytes. */
        modifier = 'K';
      }
      else if (state.st_size < (1 << 30)) {
        result >>= 20;
        /* Megabytes. */
        modifier = 'M';
      }
      else {
        result >>= 30;
        /* Gigabytes. */
        modifier = 'G';
      }
      /* Show the size if less than a terabyte, else show "(huge)". */
      if (result < (1 << 10)) {
        sprintf(info, "%4ju %cB", (intmax_t)result, modifier);
      }
      else {
        /* TRANSLATORS: Anything more than 7 cells gets clipped.  If necessary, you can leave out the parentheses. */
        info = realloc_strcpy(info, _("(huge)"));
      }
      /* Only check further color coding when not selected. */
      if (index != selected) {
        /* Apply extention based colors when not selected. */
        if (file_ext) {
          if (strcmp(file_ext, "c") == 0 || strcmp(file_ext, "cpp") == 0) {
            apply_color = FG_VS_CODE_BRIGHT_GREEN;
            did_apply_color = TRUE;
          }
          else if (strcmp(file_ext, "h") == 0 || strcmp(file_ext, "hpp") == 0) {
            apply_color = FG_VS_CODE_MAGENTA;
            did_apply_color = TRUE;
          }
          else if (strcmp(file_ext, "tar") == 0 || strcmp(file_ext, "gz") == 0) {
            apply_color = FG_VS_CODE_RED;
            did_apply_color = TRUE;
          }
          else if (strcmp(file_ext, "conf") == 0 || strcmp(file_ext, "cfg") == 0) {
            apply_color = FG_VS_CODE_BRIGHT_MAGENTA;
            did_apply_color = TRUE;
          }
        }
        /* If the file is executeble. */
        if (state.st_mode & S_IXUSR) {
          apply_color = FG_VS_CODE_GREEN;
          apply_color_mod = A_BOLD;
          did_apply_color = TRUE;
        }
      }
    }
    /* If this is the selected item, draw its highlighted bar upfront, and remember its location to be able to place the cursor on it. */
    if (index == selected) {
      wattron(midwin, interface_color_pair[SELECTED_TEXT]);
      mvwprintw(midwin, row, col, "%*s", gauge, " ");
      the_row    = row;
      the_column = col;
    }
    /* If color was selected, turn it on. */
    else if (did_apply_color) {
      wattron(midwin, (interface_color_pair[apply_color] | apply_color_mod));
    }
    /* If the name is too long, we display something like "...ename". */
    if (dots) {
      mvwaddstr(midwin, row, col, "...");
    }
    mvwaddstr(midwin, row, (dots ? (col + 3) : col), disp);
    col += gauge;
    /* Make sure info takes up no more than infomaxlen columns. */
    infolen = breadth(info);
    if (infolen > infomaxlen) {
      info[actual_x(info, infomaxlen)] = '\0';
      infolen = infomaxlen;
    }
    mvwaddstr(midwin, row, (col - infolen), info);
    /* If this is the selected item, finish its highlighting. */
    if (index == selected) {
      wattroff(midwin, interface_color_pair[SELECTED_TEXT]);
    }
    /* If color was selected, turn it off after we have printed the text. */
    else if (did_apply_color) {
      wattroff(midwin, (interface_color_pair[apply_color] | apply_color_mod));
    }
    free(disp);
    free(info);
    /* Add some space between the columns. */
    col += 2;
    /* If the next item will not fit on this row, move to next row. */
    if (col > (COLS - gauge)) {
      ++row;
      col = 0;
    }
  }
  /* If requested, put the cursor on the selected item and switch it on. */
  if (ISSET(SHOW_CURSOR)) {
    wmove(midwin, the_row, the_column);
    curs_set(TRUE);
  }
  wnoutrefresh(midwin);
}

/* ----------------------------- To first file ----------------------------- */

/* Select the first file in the list -- called by ^W^Y. */
void to_first_file(void) {
  selected = 0;
}

/* ----------------------------- To last file ----------------------------- */

/* Select the last file in the list -- called by ^W^V. */
void to_last_file(void) {
  selected = (list_length - 1);
}

/* ----------------------------- Browse in ----------------------------- */

/* Prepare to start browsing.  If the given path has a directory part, start browsing in that
 * directory, otherwise in the current directory.  If the path is not a directory, try to strip
 * a filename from it.  If then still not a directory, use the current working directory instead.
 * If the resulting path isn't in the operating directory, use the operating directory instead. */
char *browse_in(const char *const restrict inpath) {
  ASSERT(inpath);
  char *path;
  struct stat info;
  /* When not in `curses-mode`, always return NULL. */
  if (!IN_CURSES_CTX) {
    return NULL;
  }
  path = real_dir_from_tilde(inpath);
  /* If the path is not a directory. */
  if (stat(path, &info) == -1 || !S_ISDIR(info.st_mode)) {
    /* Try to strip a filename from the path. */
    path = free_and_assign(path, strip_last_component(path));
    /* Still not a directory. */
    if (stat(path, &info) == -1 || !S_ISDIR(info.st_mode)) {
      /* Try to use the current working directory. */
      path = free_and_assign(path, realpath(".", NULL));
      /* If this also fails, tell the user and return NULL. */
      if (!path) {
        statusline(ALERT, _("The working directory has disappeared"));
        napms(1200);
        return NULL;
      }
    }
  }
  /* If the resulting path isn't in the operating directory, use the operating directory instead. */
  if (outside_of_confinement(path, FALSE)) {
    path = xstrcpy(path, operating_dir);
  }
  return browse(path);
}
