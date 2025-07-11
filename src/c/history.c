/** @file history.c

  @author  Melwin Svensson.
  @date    29-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#ifndef SEARCH_HISTORY
# define SEARCH_HISTORY  "search_history"
#endif

#ifndef POSITION_HISTORY
# define POSITION_HISTORY  "filepos_history"
#endif


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Whether any of the history lists has changed. */
static bool history_changed = FALSE;

/* The name of the positions-history file. */
static char *poshistname = NULL;

/* The list of filenames with their last cursor positions. */
static poshiststruct *position_history = NULL;

/* The last time the positions-history file was written. */
static time_t latest_timestamp = 942927132;

/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Return from the history list that starts at start and ends at end the first node that contains the first len characters of the given text, or NULL if there is no such node. */
static linestruct *find_in_history(const linestruct *const start, const linestruct *const end, const char *const restrict text, Ulong len) {
  DLIST_FOR_PREV_END(start, end->prev, item) {
    if (strncmp(item->data, text, len) == 0) {
      return (linestruct *)item;
    }
  }
  return NULL;
}

/* Write the lines of a history list, starting at head, from oldest to newest, to the given file.  Return 'TRUE' if writing succeeded, and 'FALSE' otherwise. */
static bool write_list(const linestruct *const head, FILE *const histfile) {
  Ulong length;
  DLIST_FOR_NEXT(head, item) {
    /* Decode 0x0A bytes as embedded NULs. */
    length = recode_LF_to_NUL(item->data);
    if (fwrite(item->data, 1, length, histfile) < length) {
      return FALSE;
    }
    if (putc('\n', histfile) == EOF) {
      return FALSE;
    }
  }
  return TRUE;
}

/* Save the recorded cursor positions for files that were edited. */
static void save_poshistory(void) {
  FILE *histfile = fopen(poshistname, "wb");
  struct stat fileinfo;
  char *path_and_place;
  Ulong length;
  if (!histfile) {
    jot_error(N_("Error writing %s: %s"), poshistname, strerror(errno));
    return;
  }
  /* Don't allow others to read or write the history file. */
  if (chmod(poshistname, (S_IRUSR | S_IWUSR)) < 0) {
    jot_error(N_("Cannot limit permissions on %s: %s"), poshistname, strerror(errno));
  }
  DLIST_FOR_NEXT(position_history, item) {
    /* Assume 20 decimal positions each for line and column number, plus two spaces, plus the line feed, plus the null byte. */
    path_and_place = xmalloc(strlen(item->filename) + 44);
    sprintf(path_and_place, "%s %zd %zd\n", item->filename, item->linenumber, item->columnnumber);
    /* Encode newlines in filenames as NULs. */
    length = recode_LF_to_NUL(path_and_place);
    /* Restore the terminating newline. */
    path_and_place[length - 1] = '\n';
    if (fwrite(path_and_place, 1, length, histfile) < length) {
      jot_error(N_("Error writing %s: %s"), poshistname, strerror(errno));
    }
    free(path_and_place);
  }
  if (fclose(histfile) == EOF) {
    jot_error(N_("Error writing %s: %s"), poshistname, strerror(errno));
  }
  if (stat(poshistname, &fileinfo) == 0) {
    latest_timestamp = fileinfo.st_mtime;
  }
}

/* Reload the position history file if it has been modified since last load. */
static void reload_positions_if_needed(void) {
  poshiststruct *item;
  poshiststruct *nextone;
  struct stat fileinfo;
  if (stat(poshistname, &fileinfo) != 0 || fileinfo.st_mtime == latest_timestamp) {
    return;
  }
  for (item=position_history; item; item=nextone) {
    nextone = item->next;
    free(item->filename);
    free(item);
  }
  position_history = NULL;
  load_poshistory();
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Initialize the lists of historical search and replace strings and the list of historical executed commands. */
void history_init(void) {
  search_history        = make_new_node(NULL);
  search_history->data  = COPY_OF("");
  searchtop             = search_history;
  searchbot             = search_history;
  replace_history       = make_new_node(NULL);
  replace_history->data = COPY_OF("");
  replacetop            = replace_history;
  replacebot            = replace_history;
  execute_history       = make_new_node(NULL);
  execute_history->data = COPY_OF("");
  executetop            = execute_history;
  executebot            = execute_history;
}

/* Reset the pointer into the history list that contains item to the bottom. */
void reset_history_pointer_for(const linestruct *const item) {
  if (item == search_history) {
    search_history = searchbot;
  }
  else if (item == replace_history) {
    replace_history = replacebot;
  }
  else if (item == execute_history) {
    execute_history = executebot;
  }
}

/* Update a history list (the one in which item is the current position) 
 * with a fresh string text.  That is: add text, or move it to the end. */
void update_history(linestruct **const item, const char *const restrict text, bool avoid_duplicates) {
  linestruct **htop    = NULL;
  linestruct **hbot    = NULL;
  linestruct  *thesame = NULL;
  if (*item == search_history) {
    htop = &searchtop;
    hbot = &searchbot;
  }
  else if (*item == replace_history) {
    htop = &replacetop;
    hbot = &replacebot;
  }
  else if (*item == execute_history) {
    htop = &executetop;
    hbot = &executebot;
  }
  /* When requested, check if the string is already in the history. */
  if (avoid_duplicates) {
    thesame = find_in_history(*hbot, *htop, text, HIGHEST_POSITIVE);
  }
  /* If an identical string was found, delete that item. */
  if (thesame) {
    linestruct *after = thesame->next;
    /* If the string is at the head of the list, move the head. */
    if (thesame == *htop) {
      *htop = after;
    }
    unlink_node(thesame);
    renumber_from(after);
  }
  /* If the history is full, delete the oldest item (the one at the
   * head of the list), to make room for a new item at the end. */
  if ((*hbot)->lineno == MAX_SEARCH_HISTORY + 1) {
    linestruct *oldest = *htop;
    *htop = (*htop)->next;
    unlink_node(oldest);
    renumber_from(*htop);
  }
  /* Store the fresh string in the last item, then create a new item. */
  (*hbot)->data = realloc_strcpy((*hbot)->data, text);
  splice_node(*hbot, make_new_node(*hbot));
  *hbot = (*hbot)->next;
  (*hbot)->data = COPY_OF("");
  /* Indicate that the history needs to be saved on exit. */
  history_changed = TRUE;
  /* Set the current position in the list to the bottom. */
  *item = *hbot;
}

/* Go backward through one of three history lists, starting at item *here,
 * searching for a string that is a tab completion of the given string,
 * looking at only its first len characters.  When found, make *here point
 * at the item and return its string; otherwise, just return the string. */
char *get_history_completion(linestruct **const here, char *string, Ulong len) {
  linestruct *htop = NULL;
  linestruct *hbot = NULL;
  linestruct *item;
  if (*here == search_history) {
    htop = searchtop;
    hbot = searchbot;
  }
  else if (*here == replace_history) {
    htop = replacetop;
    hbot = replacebot;
  }
  else if (*here == execute_history) {
    htop = executetop;
    hbot = executebot;
  }
  /* First search from the current position to the top of the list
   * for a match of len characters.  Skip over an exact match. */
  item = find_in_history((*here)->prev, htop, string, len);
  while (item && strcmp(item->data, string) == 0) {
    item = find_in_history(item->prev, htop, string, len);
  }
  if (item) {
    *here = item;
    return realloc_strcpy(string, item->data);
  }
  /* Now search from the bottom of the list to the original position. */
  item = find_in_history(hbot, *here, string, len);
  while (item && strcmp(item->data, string) == 0) {
    item = find_in_history(item->prev, *here, string, len);
  }
  if (item) {
    *here = item;
    return realloc_strcpy(string, item->data);
  }
  /* When no useful match was found, simply return the given string. */
  return string;
}

/* Check whether we have or could make a directory for history files. */
bool have_statedir(void) {
  const char *xdgdatadir;
  struct stat dirinfo;
  char *statepath;
  get_homedir();
  if (homedir) {
    statedir = concatenate(homedir, "/.nano/");
    if (stat(statedir, &dirinfo) == 0 && S_ISDIR(dirinfo.st_mode)) {
      poshistname = concatenate(statedir, POSITION_HISTORY);
      return TRUE;
    }
  }
  free(statedir);
  xdgdatadir = getenv("XDG_DATA_HOME");
  if (!homedir && !xdgdatadir) {
    return FALSE;
  }
  if (xdgdatadir) {
    statedir = concatenate(xdgdatadir, "/nano/");
  }
  else {
    statedir = concatenate(homedir, "/.local/share/nano/");
  }
  if (stat(statedir, &dirinfo) == -1) {
    if (!xdgdatadir) {
      statepath = concatenate(homedir, "/.local");
      mkdir(statepath, S_IRWXU | S_IRWXG | S_IRWXO);
      free(statepath);
      statepath = concatenate(homedir, "/.local/share");
      mkdir(statepath, S_IRWXU);
      free(statepath);
    }
    if (mkdir(statedir, S_IRWXU) == -1) {
      jot_error(N_("Unable to create directory %s: %s\nIt is required for 'saving/loading' search history or cursor positions.\n"), statedir, strerror(errno));
      return FALSE;
    }
  }
  else if (!S_ISDIR(dirinfo.st_mode)) {
    jot_error(N_("Path %s is not a directory and needs to be.\nNano will be unable to load or save search history or cursor positions.\n"), statedir);
    return FALSE;
  }
  poshistname = concatenate(statedir, POSITION_HISTORY);
  return TRUE;
}

/* Load the histories for Search, Replace With, and Execute Command. */
void load_history(void) {
  char *histname = concatenate(statedir, SEARCH_HISTORY);
  FILE *histfile = fopen(histname, "rb");
  /* If reading an existing file failed, don't save history when we quit. */
  if (!histfile && errno != ENOENT) {
    jot_error(N_("Error reading %s: %s"), histname, strerror(errno));
    UNSET(HISTORYLOG);
  }
  if (!histfile) {
    free(histname);
    return;
  }
  linestruct **history = &search_history;
  char        *stanza  = NULL;
  Ulong        dummy   = 0;
  long         read;
  /* Load the three history lists (first search, then replace, then execute)
   * from oldest entry to newest.  Between two lists there is an empty line. */
  while ((read = getline(&stanza, &dummy, histfile)) > 0) {
    stanza[--read] = '\0';
    if (read > 0) {
      recode_NUL_to_LF(stanza, read);
      update_history(history, stanza, IGNORE_DUPLICATES);
    }
    else if (history == &search_history) {
      history = &replace_history;
    }
    else {
      history = &execute_history;
    }
  }
  if (fclose(histfile) == EOF) {
    jot_error(N_("Error reading %s: %s"), histname, strerror(errno));
  }
  free(histname);
  free(stanza);
  /* Reading in the lists has marked them as changed; undo this side effect. */
  history_changed = FALSE;
}

/* Save the histories for Search, Replace With, and Execute Command. */
void save_history(void) {
  char *histname;
  FILE *histfile;
  /* If the histories are unchanged, don't bother saving them. */
  if (!history_changed) {
    return;
  }
  histname = concatenate(statedir, SEARCH_HISTORY);
  histfile = fopen(histname, "wb");
  if (!histfile) {
    jot_error(N_("Error writing %s: %s"), histname, strerror(errno));
    free(histname);
    return;
  }
  /* Don't allow others to read or write the history file. */
  if (chmod(histname, (S_IRUSR | S_IWUSR)) < 0) {
    jot_error(N_("Cannot limit permissions on %s: %s"), histname, strerror(errno));
  }
  if (!write_list(searchtop, histfile) || !write_list(replacetop, histfile) || !write_list(executetop, histfile)) {
    jot_error(N_("Error writing %s: %s"), histname, strerror(errno));
  }
  if (fclose(histfile) == EOF) {
    jot_error(N_("Error writing %s: %s"), histname, strerror(errno));
  }
  free(histname);
}

/* Load the recorded cursor positions for files that were edited. */
void load_poshistory(void) {
  FILE *histfile = fopen(poshistname, "rb");
  poshiststruct *drop_record;
  poshiststruct *lastitem = NULL;
  poshiststruct *newitem;
  char *lineptr;
  char *columnptr;
  char *stanza = NULL;
  struct stat    fileinfo;
  Ulong          dummy = 0;
  long           count = 0;
  long           read;
  /* If reading an existing file failed, don't save history when we quit. */
  if (!histfile) {
    if (errno != ENOENT) {
      jot_error(N_("Error reading %s: %s"), poshistname, strerror(errno));
      UNSET(POSITIONLOG);
    }
    return;
  }
  /* Read and parse each line, and store the extracted data. */
  while ((read = getline(&stanza, &dummy, histfile)) > 5) {
    /* Decode NULs as embedded newlines. */
    recode_NUL_to_LF(stanza, read);
    /* Find the spaces before column number and line number. */
    columnptr = revstrstr(stanza, " ", (stanza + read - 3));
    if (!columnptr) {
      continue;
    }
    lineptr = revstrstr(stanza, " ", (columnptr - 2));
    if (!lineptr) {
      continue;
    }
    /* Now separate the three elements of the line. */
    *(columnptr++) = '\0';
    *(lineptr++)   = '\0';
    /* Create a new position record. */
    newitem               = xmalloc(sizeof(*newitem));
    newitem->filename     = copy_of(stanza);
    newitem->linenumber   = atoi(lineptr);
    newitem->columnnumber = atoi(columnptr);
    newitem->next         = NULL;
    /* Add the record to the list. */
    if (!position_history) {
      position_history = newitem;
    }
    else {
      lastitem->next = newitem;
    }
    lastitem = newitem;
    /* Impose a limit, so the file will not grow indefinitely. */
    if (++count > 200) {
      drop_record      = position_history;
      position_history = position_history->next;
      free(drop_record->filename);
      free(drop_record);
    }
  }
  if (fclose(histfile) == EOF) {
    jot_error(N_("Error reading %s: %s"), poshistname, strerror(errno));
  }
  free(stanza);
  if (stat(poshistname, &fileinfo) == 0) {
    latest_timestamp = fileinfo.st_mtime;
  }
}

/* Update the recorded last file positions with the current position in `file`.  If no existing entry is found, add a new one at the end. */
void update_poshistory_for(openfilestruct *const file) {
  ASSERT(file);
  char *fullpath = get_full_path(file->filename);
  poshiststruct *previous = NULL;
  poshiststruct *item;
  poshiststruct *theone;
  if (!fullpath || !file->filename[0]) {
    free(fullpath);
    return;
  }
  reload_positions_if_needed();
  /* Look for a matching filename in the list. */
  DLIST_ND_FOR_NEXT(position_history, item) {
    if (!strcmp(item->filename, fullpath)) {
      break;
    }
    previous = item;
  }
  /* Don't record files that have the default cursor position. */
  if (file->current->lineno == 1 && !file->current_x) {
    if (item) {
      if (!previous) {
        position_history = item->next;
      }
      else {
        previous->next = item->next;
      }
      free(item->filename);
      free(item);
      save_poshistory();
    }
    free(fullpath);
    return;
  }
  theone = item;
  /* If we didn't find it, make a new node; otherwise, if we're not at the end, move the matching one to the end. */
  if (!theone) {
    theone           = xmalloc(sizeof(*theone));
    theone->filename = copy_of(fullpath);
    if (!position_history) {
      position_history = theone;
    }
    else {
      previous->next = theone;
    }
  }
  else if (item->next) {
    if (!previous) {
      position_history = item->next;
    }
    else {
      previous->next = item->next;
    }
    while (item->next) {
      item = item->next;
    }
    item->next = theone;
  }
  /* Store the last cursor position. */
  theone->linenumber   = file->current->lineno;
  theone->columnnumber = (xplustabs_for(file) + 1);
  theone->next         = NULL;
  free(fullpath);
  save_poshistory();
}

/* Update the recorded last file positions with the current position in the
 * current buffer.  If no existing entry is found, add a new one at the end. */
void update_poshistory(void) {
  update_poshistory_for(CONTEXT_OPENFILE);
}

/* Check whether the given file matches an existing entry in the recorded last file positions.
 * If not, return 'FALSE'.  If yes, return 'TRUE' and set line and column to the retrieved values. */
bool has_old_position(const char *const restrict file, long *const line, long *const column) {
  ASSERT(line);
  ASSERT(column);  
  char *fullpath = get_full_path(file);
  poshiststruct *item;
  if (!fullpath) {
    return FALSE;
  }
  reload_positions_if_needed();
  item = position_history;
  while (item && strcmp(item->filename, fullpath)) {
    item = item->next;
  }
  free(fullpath);
  if (!item) {
    return FALSE;
  }
  else {
    *line   = item->linenumber;
    *column = item->columnnumber;
    return TRUE;
  }
}

