/** @file browser.c

  @author  Melwin Svensson.
  @date    1-6-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* These will be made static later. */

/* The list of files to display in the file browser. */
char **filelist = NULL;
/* The number of files in the list. */
Ulong list_length = 0;
/* The number of screen rows we can use to display the list. */
Ulong usable_rows = 0;
/* The number of files that we can display per screen row. */
int piles = 0;
/* The width of a 'pile' -- the widest filename plus ten. */
int gauge = 0;
/* The currently selected filename in the list; zero-based. */
Ulong selected = 0;


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Fill 'filelist' with the names of the files in the given directory, set 'list_length' to the
 * number of names in that list, set 'gauge' to the width of the widest filename plus ten, and
 * set 'piles' to the number of files that can be displayed per screen row.  And sort the list too. 
 * Note: Make static later. */
void read_the_list(const char *path, DIR *dir) {
  Ulong path_len = strlen(path);
  Ulong widest = 0;
  Ulong index  = 0;
  const struct dirent *entry;
  /* Find the width of the widest filename in the current folder. */
  while ((entry = readdir(dir))) {
    Ulong span = breadth(entry->d_name);
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
  filelist = xmalloc(list_length * sizeof(char *));
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
  qsort(filelist, list_length, sizeof(char *), diralphasort);
  /* Calculate how many files fit on a line -- feigning room for two spaces
   * beyond the right edge, and adding two spaces of padding between columns. */
  piles = ((COLS + 2) / (gauge + 2));
  usable_rows = (editwinrows - (ISSET(ZERO) && (LINES > 1) ? 1 : 0));
}

/* Reselect the given file or directory name, if it still exists.  Note: Make static later. */
void reselect(const char *const name) {
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

/* Display at most a screenful of filenames from the gleaned filelist. */
void browser_refresh(void) {
  /* The current row and column while the list is getting displayed. */
  int row=0, col=0;
  /* The row and column of the selected item. */
  int the_row=0, the_column=0;
  /* The additional information that we'll display about a file. */
  char *info;
  titlebar(present_path);
  blank_edit();
  for (Ulong index = (selected - selected % (usable_rows * piles)); (index < list_length) && (row < (long)usable_rows); ++index) {
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
          if (strcmp(file_ext, "h") == 0 || strcmp(file_ext, "hpp") == 0) {
            apply_color = FG_VS_CODE_MAGENTA;
            did_apply_color = TRUE;
          }
          if (strcmp(file_ext, "tar") == 0 || strcmp(file_ext, "gz") == 0) {
            apply_color = FG_VS_CODE_RED;
            did_apply_color = TRUE;
          }
          if (strcmp(file_ext, "conf") == 0 || strcmp(file_ext, "cfg") == 0) {
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
    curs_set(1);
  }
  wnoutrefresh(midwin);
}

/* Look for the given needle in the list of files, forwards or backwards. Note: Make static later.  */
void findfile(const char *needle, bool forwards) {
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

/* Select the first file in the list -- called by ^W^Y. */
void to_first_file(void) {
  selected = 0;
}

/* Select the last file in the list -- called by ^W^V. */
void to_last_file(void) {
  selected = (list_length - 1);
}

/* Search again without prompting for the last given search string, either forwards or backwards. */
void research_filename(bool forwards) {
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

/* Strip one element from the end of path, and return the stripped path.
 * The returned string is dynamically allocated, and should be freed. */
char *strip_last_component(const char *const restrict path) {
  char *copy = copy_of(path);
  char *last_slash = strrchr(copy, '/');
  if (last_slash) {
    *last_slash = '\0';
  }
  return copy;
}
