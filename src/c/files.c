/** @file files.c

  @author  Melwin Svensson.
  @date    10-2-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#ifdef LOCKSIZE
# undef LOCKSIZE
#endif
#ifdef RW_FOR_ALL
# undef RW_FOR_ALL
#endif
#ifdef LOCKING_PREFIX
# undef LOCKING_PREFIX
#endif
#ifdef LOCKING_SUFFIX
# undef LOCKING_SUFFIX
#endif

#define LOCKSIZE    (1024)
#define RW_FOR_ALL  (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define LOCKING_PREFIX  "."
#define LOCKING_SUFFIX  ".swp"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Add an item to the circular list of openfile structs. */
void make_new_buffer(void) {
  openfilestruct *newnode;
  MALLOC_STRUCT(newnode);
  if (!openfile) {
    /* Make the first buffer the only element in the list. */
    CLIST_INIT(newnode);
    startfile = newnode;
  }
  else {
    /* Add the new buffer after the current one in the list. */
    CLIST_INSERT_AFTER(newnode, openfile);
    /* There is more than one buffer: show "Close" in help lines. */
    exitfunc->tag = close_tag;
    more_than_one = (!inhelp || more_than_one);
  }
  /* Make the new buffer the current one, and start initializing it */
  openfile                = newnode;
  openfile->filename      = COPY_OF("");
  openfile->filetop       = make_new_node(NULL);
  openfile->filetop->data = COPY_OF("");
  openfile->filebot       = openfile->filetop;
  openfile->current       = openfile->filetop;
  openfile->current_x     = 0;
  openfile->placewewant   = 0;
  openfile->cursor_row    = 0;
  openfile->edittop       = openfile->filetop;
  openfile->firstcolumn   = 0;
  openfile->totsize       = 0;
  openfile->modified      = FALSE;
  openfile->spillage_line = NULL;
  openfile->mark          = NULL;
  openfile->softmark      = FALSE;
  openfile->fmt           = UNSPECIFIED;
  openfile->undotop       = NULL;
  openfile->current_undo  = NULL;
  openfile->last_saved    = NULL;
  openfile->last_action   = OTHER;
  openfile->statinfo      = NULL;
  openfile->lock_filename = NULL;
  openfile->errormessage  = NULL;
  openfile->syntax        = NULL;
}

/* Return the given file name in a way that fits within the given space. */
char *crop_to_fit(const char *const restrict name, Ulong room) {
  char *clipped;
  if (breadth(name) <= room) {
    return display_string(name, 0, room, FALSE, FALSE);
  }
  if (room < 4) {
    return copy_of("_");
  }
  clipped = display_string(name, (breadth(name) - room + 3), room, FALSE, FALSE);
  clipped = xrealloc(clipped, (strlen(clipped) + 4));
  memmove((clipped + 3), clipped, (strlen(clipped) + 1));
  clipped[0] = '.';
  clipped[1] = '.';
  clipped[2] = '.';
  return clipped;
}

/* Perform a stat call on the given filename, allocating a stat struct if necessary. On success,
 * '*pstat' points to the stat's result.  On failure, '*pstat' is freed and made 'NULL'. */
void stat_with_alloc(const char *filename, struct stat **pstat) {
  !*pstat ? (*pstat = malloc(sizeof(**pstat))) : 0;
  if (stat(filename, *pstat)) {
    free(*pstat);
    *pstat = NULL;
  }
}

/* Update the title bar and the multiline cache to match the current buffer. */
void prepare_for_display(void) {
  /* When using the gui make this function a `No-op` function. */
  if (ISSET(USING_GUI)) {
    return;
  }
  /* Update the title bar, since the filename may have changed. */
  if (!inhelp) {
    titlebar_curses(NULL);
  }
  /* Precalculate the data for any multiline coloring regexes. */
  if (!openfile->filetop->multidata) {
    precalc_multicolorinfo();
  }
  have_palette   = FALSE;
  refresh_needed = TRUE;
}

/* Show name of current buffer and its number of lines on the status bar. */
void mention_name_and_linecount_for(openfilestruct *const file) {
  ASSERT(file);
  Ulong count = (file->filebot->lineno - !*file->filebot->data);
  if (ISSET(MINIBAR)) {
    report_size = TRUE;
    return;
  }
  else if (ISSET(ZERO)) {
    return;
  }
  if (file->fmt > NIX_FILE) {
    /* TRANSLATORS: First %s is file name, second %s is file format. */
    statusline_all(
      HUSH, P_("%s -- %zu line (%s)", "%s -- %zu lines (%s)", count),
      ((!*file->filename) ? _("New Buffer") : tail(file->filename)),
      count, ((file->fmt == DOS_FILE) ? _("DOS") : _("Mac")));
  }
  else {
    statusline_all(HUSH, P_("%s -- %zu line", "%s -- %zu lines", count), ((file->filename[0] == '\0') ? _("New Buffer") : tail(file->filename)), count);
  }
}

/* Show name of the current buffer and its number of lines on the statusbar. */
void mention_name_and_linecount(void) {
  mention_name_and_linecount_for(ISSET(USING_GUI) ? openeditor->openfile : openfile);
}

/* Delete the lock file.  Return TRUE on success, and FALSE otherwise. */
bool delete_lockfile(const char *const restrict lockfilename) {
  if (unlink(lockfilename) < 0 && errno != ENOENT) {
    statusline_all(MILD, _("Error deleting lock file %s: %s"), lockfilename, strerror(errno));
    return FALSE;
  }
  return TRUE;
}

/* Write a lock file, under the given lockfilename.  This always annihilates an existing version of that file.  Return TRUE on success; FALSE otherwise. */
bool write_lockfile(const char *const restrict lockfilename, const char *const restrict filename, bool modified) {
  ASSERT(lockfilename);
  ASSERT(filename);
  int  pid = getpid();
  Uint uid = getuid();
  struct passwd *pwuid = getpwuid(uid);
  char hostname[32];
  int fd;
  char *lockdata;
  long len;
  long written = 0;
  /* We failed to get the current user data. */
  if (!pwuid) {
    if (ISSET(USING_GUI)) {
      statusline_gui(MILD, _("Couldn't determine user for lock file"));
    }
    else if (!ISSET(NO_NCURSES)) {
      statusline_curses(MILD, _("Couldn't determine user for lock file"));
    }
    return FALSE;
  }
  else if (gethostname(hostname, 31) < 0 && errno != ENAMETOOLONG) {
    statusline_all(MILD, _("Couldn't determin hostname: %s"), strerror(errno));
    return FALSE;
  }
  else {
    hostname[31] = '\0';
  }
  /* First make sure to remove an existing lock file. */
  if (!delete_lockfile(lockfilename)) {
    return FALSE;
  }
  /* Open the lockfile file-descriptor. */
  if ((fd = open(lockfilename, (O_WRONLY | O_CREAT | O_EXCL), RW_FOR_ALL)) == -1) {
    statusline_all(MILD, _("Error opening file-descriptor: %s: %s"), lockfilename, strerror(errno));
    return FALSE;
  }
  /* Create the lock data we will write. */
  lockdata = xmalloc(LOCKSIZE);
  /* And fully clear it. */
  memset(lockdata, 0, LOCKSIZE);
  /*
   * This is the lock data we will store (other bytes remain 0x00):
   *
   *     bytes 0-1     - 0x62 0x30
   *     bytes 2-11    - name of program that created the lock
   *     bytes 24-27   - PID (little endian) of creator process
   *     bytes 28-43   - username of the user who created the lock
   *     bytes 68-99   - hostname of machine from where the lock was created
   *     bytes 108-876 - filename that the lock is for
   *     byte 1007     - 0x55 if file is modified
   *
   * Nano does not write the page size (bytes 12-15), nor the modification
   * time (bytes 16-19), nor the inode of the relevant file (bytes 20-23).
   * Nano also does not use all available space for user name (40 bytes),
   * host name (40 bytes), and file name (890 bytes).  Nor does nano write
   * some byte-order-checking numbers (bytes 1008-1022).
  */
  lockdata[0] = 0x62;
  lockdata[1] = 0x30;
  /* It's fine to overwrite byte 12 with the \0 as it is 0x00 anyway. */
  snprintf(&lockdata[2], 11, "nanox %s", VERSION);
  lockdata[24] = (pid % 256);
  lockdata[25] = ((pid / 256) % 256);
  lockdata[26] = ((pid / (256 * 256)) % 256);
  lockdata[27] = (pid / (256 * 256 * 256));
  strncpy(&lockdata[28], pwuid->pw_name, 16);
  strncpy(&lockdata[68], hostname, 32);
  strncpy(&lockdata[108], filename, 768);
  lockdata[1007] = (modified ? 0x55 : 0x00);
  /* Write to the file-descriptor while we hold a write-lock for it.  */
  fdlock_action(fd, F_WRLCK,
    /* Write until  */
    while ((len = write(fd, (lockdata + written), (LOCKSIZE - written))) > 0) {
      written += len;
    }
  );
  free(lockdata);
  close(fd);
  if (len == -1 || written < LOCKSIZE) {
    statusline_all(MILD, _("Error writing lock file %s: %s"), lockfilename, strerror(errno));
    return FALSE;
  }
  return TRUE;
}

/* Verify that the containing directory of the given filename exists. */
bool has_valid_path(const char *const restrict filename) {
  char *namecopy  = copy_of(filename);
  char *parentdir = dirname(namecopy);
  bool  validity  = FALSE;
  bool  gone      = FALSE;
  struct stat parentinfo;
  char *currentdir;
  if (strcmp(parentdir, ".") == 0) {
    currentdir = realpath(".", NULL);
    gone = (currentdir == NULL && errno == ENOENT);
    free(currentdir);
  }
  if (gone) {
    statusline_all(ALERT, _("The working directory has disappeared"));
  }
  else if (stat(parentdir, &parentinfo) == -1) {
    if (errno == ENOENT) {
      /* TRANSLATORS: Keep the next ten messages at most 76 characters. */
      statusline_all(ALERT, _("Directory '%s' does not exist"), parentdir);
    }
    else {
      statusline_all(ALERT, _("Path '%s': %s"), parentdir, strerror(errno));
    }
  }
  else if (!S_ISDIR(parentinfo.st_mode)) {
    statusline_all(ALERT, _("Path '%s' is not a directory"), parentdir);
  }
  else if (access(parentdir, X_OK) == -1) {
    statusline_all(ALERT, _("Path '%s' is not accessible"), parentdir);
  }
  else if (ISSET(LOCKING) && !ISSET(VIEW_MODE) && access(parentdir, W_OK) < 0) {
    statusline_all(MILD, _("Directory '%s' is not writable"), parentdir);
  }
  else {
    validity = TRUE;
  }
  free(namecopy);
  return validity;
}

void free_one_buffer(openfilestruct *orphan, openfilestruct **open, openfilestruct **start) {
  /* If the buffer to free is the start buffer, advance the start buffer. */
  if (orphan == *start) {
    *start = (*start)->next;
  }
  CLIST_UNLINK(orphan);
  // if (/* orphan->type.is_set<C_CPP>() || orphan->type.is_set<BASH>() */ orphan->is_c_file || orphan->is_cxx_file || orphan->is_bash_file) {
  //   file_listener.stop_listener(orphan->filename);
  // }
  free(orphan->filename);
  free_lines(orphan->filetop);
  free(orphan->statinfo);
  free(orphan->lock_filename);
  /* Free the undo stack for the orphan file. */
  discard_until_for(orphan, NULL);
  free(orphan->errormessage);
  /* If the buffer to free is the open buffer, decrament it once. */
  if (orphan == *open) {
    CLIST_ADV_PREV(*open);
    /* If the buffer to free was the singular and only buffer in the list, set open and start to NULL. */
    if (orphan == *open) {
      *open  = NULL;
      *start = NULL;
    }
  }
  free(orphan);
  /* When just one buffer ramains, set the legacy help bar text for the exit function. */
  if (*open && *open == (*open)->next) {
    exitfunc->tag = exit_tag;
  }
}

/* Remove the current buffer from the circular list of buffers.  When just one buffer remains open, show "Exit" in the help lines. */
void close_buffer(void) {
  openfilestruct *orphan = openfile;
  if (orphan == startfile) {
    startfile = startfile->next;
  }
  CLIST_UNLINK(orphan);
  free(orphan->filename);
  free_lines(orphan->filetop);
  free(orphan->statinfo);
  free(orphan->lock_filename);
  /* Free the undo stack. */
  discard_until(NULL);
  free(orphan->errormessage);
  openfile = orphan->prev;
  if (openfile == orphan) {
    openfile = NULL;
  }
  free(orphan);
  /* When just one buffer remains open, show "Exit" in the help lines. */
  if (openfile && CLIST_SINGLE(openfile)) {
    exitfunc->tag = exit_tag;
  }
}

/* Convert the tilde notation when the given path begins with ~/ or ~user/. Return an allocated string containing the expanded path. */
char *real_dir_from_tilde(const char *const restrict path) {
  char *tilded;
  char *ret;
  Ulong i = 1;
  const struct passwd *userdata;
  if (*path != '~') {
    return copy_of(path);
  }
  while (path[i] != '/' && path[i]) {
    ++i;
  }
  if (i == 1) {
    get_homedir();
    tilded = copy_of(homedir);
  }
  else {
    tilded = measured_copy(path, i);
    do {
      userdata = getpwent();
    } while (userdata && strcmp(userdata->pw_name, (tilded + 1)) != 0);
    endpwent();
    if (userdata) {
      tilded = realloc_strcpy(tilded, userdata->pw_dir);
    }
  }
  ret = xmalloc(strlen(tilded) + strlen(path + i) + 1);
  sprintf(ret, "%s%s", tilded, (path + i));
  free(tilded);
  return ret;
}

/* Return 'TRUE' when the given path is a directory. */
bool is_dir(const char *const path) {
  char *thepath = real_dir_from_tilde(path);
  struct stat fileinfo;
  bool retval = (stat(thepath, &fileinfo) != -1 && S_ISDIR(fileinfo.st_mode));
  free(thepath);
  return retval;
}

/* For the given bare path (or path plus filename), return the canonical,
 * absolute path (plus filename) when the path exists, and 'NULL' when not. */
char *get_full_path(const char *const restrict origpath) {
  char *untilded;
  char *target;
  char *slash;
  struct stat fileinfo;
  if (!origpath) {
    return NULL;
  }
  untilded = real_dir_from_tilde(origpath);
  target   = realpath(untilded, NULL);
  slash    = strrchr(untilded, '/');
  /* If realpath() returned NULL, try without the last component, as this can be a file that does not exist yet. */
  if (!target && slash && slash[1]) {
    *slash = '\0';
    target = realpath(untilded, NULL);
    /* Upon success, re-add the last component of the original path. */
    if (target) {
      target = xrealloc(target, (strlen(target) + strlen(slash + 1) + 1));
      strcat(target, (slash + 1));
    }
  }
  /* Ensure that a non-apex directory path ends with a slash. */
  if (target && target[1] && stat(target, &fileinfo) == 0 && S_ISDIR(fileinfo.st_mode)) {
    target = xrealloc(target, (strlen(target) + 2));
    strcat(target, "/");
  }
  free(untilded);
  return target;
}

/* Check whether the given path refers to a directory that is writable.
 * Return the absolute form of the path on success, and 'NULL' on failure. */
char *check_writable_directory(const char *path) {
  char *full_path = get_full_path(path);
  if (!full_path) {
    return NULL;
  }
  if (full_path[strlen(full_path) - 1] != '/' || access(full_path, W_OK) != 0) {
    free(full_path);
    return NULL;
  }
  return full_path;
}

/* Our sort routine for file listings.  Sort alphabetically and case-insensitively, and sort directories before filenames. */
int diralphasort(const void *va, const void *vb) {
  struct stat fileinfo;
  const char *a = *(const char *const *)va;
  const char *b = *(const char *const *)vb;
  bool aisdir = (stat(a, &fileinfo) != -1 && S_ISDIR(fileinfo.st_mode));
  bool bisdir = (stat(b, &fileinfo) != -1 && S_ISDIR(fileinfo.st_mode));
  if (aisdir && !bisdir) {
    return -1;
  }
  if (!aisdir && bisdir) {
    return 1;
  }
  int difference = mbstrcasecmp(a, b);
  /* If two names are equivalent when ignoring case, compare them bytewise. */
  if (!difference) {
    return strcmp(a, b);
  }
  else {
    return difference;
  }
}

/* Mark `file` as modified if it isn't already, and then update the title bar to display the buffer's new status.  As well as re-writing the lockfile it there is one. */
void set_modified_for(openfilestruct *const file) {
  ASSERT(file);
  if (file->modified) {
    return;
  }
  file->modified = TRUE;
  if (!ISSET(NO_NCURSES)) {
    titlebar_curses(NULL);
  }
  if (file->lock_filename) {
    write_lockfile(file->lock_filename, file->filename, TRUE);
  }
}

/* Mark the `openfile` buffer as modified if it isn't already, and then update the title bar to display the buffer's new status. */
void set_modified(void) {
  set_modified_for(openfile);
}

/* Encode any NUL bytes in the given line of text (of the given length), and return a dynamically allocated copy of the resultant string. */
char *encode_data(char *text, Ulong length) {
  recode_NUL_to_LF(text, length);
  text[length] = '\0';
  return copy_of(text);
}

/* Change to the specified operating directory, when its valid. */
void init_operating_dir(void) {
  char *target = get_full_path(operating_dir);
  if (!target || chdir(target) == -1) {
    die(_("Invalid operating directory: %s\n"), operating_dir);
  }
}

/* Check whether the given path is outside of the operating directory.
 * Return TRUE if it is, and 'FALSE' otherwise.  If tabbing is TRUE,
 * incomplete names that can grow into matches for the operating directory
 * are considered to be inside, so that tab completion will work. */
bool outside_of_confinement(const char *const restrict somepath, bool tabbing) {
  bool is_inside;
  bool begins_to_be;
  char *fullpath;
  if (!operating_dir) {
    return FALSE;
  }
  fullpath = get_full_path(somepath);
  /* When we can't get an absolute path, it means some directory in the path doesn't exist or is unreadable.  When
   * not doing tab completion, somepath is what the user typed somewhere.  We don't want to report a non-existent
   * directory as being outside the operating directory, so we return FALSE.  When the user is doing tab
   * completion, then somepath exists but is not executable.  So we say it is outside the operating directory. */
  if (!fullpath) {
    return tabbing;
  }
  is_inside    = (strstr(fullpath, operating_dir) == fullpath);
  begins_to_be = (tabbing && (strstr(operating_dir, fullpath) == operating_dir));
  free(fullpath);
  return (!is_inside && !begins_to_be);
}

/* Transform the specified backup directory to an absolute path, and verify that it is usable. */
void init_backup_dir(void) {
  char *target = get_full_path(backup_dir);
  /* If we can't get an absolute path (which means it doesn't exist or isn't accessible), or it's not a directory, fail. */
  if (!target || target[strlen(target) - 1] != '/') {
    die(_("Invalid backup directory: %s\n"), backup_dir);
  }
  free(backup_dir);
  backup_dir = xrealloc(target, (strlen(target) + 1));
}

/* Read all data from inn, and write it to out.  File inn must be open for
 * reading, and out for writing.  Return 0 on success, a negative number on
 * read error, and a positive number on write error.  File inn is always
 * closed by this function, out is closed  only if close_out is TRUE. */
int copy_file(FILE *inn, FILE *out, bool close_out) {
  int   retval = 0;
  char  buf[BUFSIZ];
  Ulong charsread;
  int (*flush_out_fnc)(FILE *) = ((close_out) ? fclose : fflush);
  do {
    charsread = fread(buf, 1, BUFSIZ, inn);
    if (charsread == 0 && ferror(inn)) {
      retval = -1;
      break;
    }
    if (fwrite(buf, 1, charsread, out) < charsread) {
      retval = 2;
      break;
    }
  } while (charsread > 0);
  if (fclose(inn) == EOF) {
    retval = -3;
  }
  if (flush_out_fnc(out) == EOF) {
    retval = 4;
  }
  return retval;
}

/* Create, safely, a temporary file in the standard temp directory.
 * On success, return the malloc()ed filename, plus the corresponding
 * file stream opened in read-write mode.  On error, return 'NULL'. */
char *safe_tempfile(FILE **stream) {
  const char *env_dir = getenv("TMPDIR");
  char *tempdir = NULL, *tempfile_name = NULL;
  char *extension;
  int descriptor;
  /* Get the absolute path for the first directory among $TMPDIR and P_tmpdir that is writable, otherwise use /tmp/. */
  if (env_dir) {
    tempdir = check_writable_directory(env_dir);
  }
  if (!tempdir) {
    tempdir = check_writable_directory(P_tmpdir);
  }
  if (!tempdir) {
    tempdir = COPY_OF("/tmp/");
  }
  extension = strrchr(openfile->filename, '.');
  if (!extension || strchr(extension, '/')) {
    extension = openfile->filename + strlen(openfile->filename);
  }
  tempfile_name = xrealloc(tempdir, (strlen(tempdir) + 12 + strlen(extension)));
  strcat(tempfile_name, "nano.XXXXXX");
  strcat(tempfile_name, extension);
  descriptor = mkstemps(tempfile_name, strlen(extension));
  *stream = ((descriptor > 0) ? fdopen(descriptor, "r+b") : NULL);
  if (!(*stream)) {
    if (descriptor > 0) {
      close(descriptor);
    }
    free(tempfile_name);
    return NULL;
  }
  return tempfile_name;
}

/* Update title bar and such after switching to another buffer. */
void redecorate_after_switch(void) {
  /* If only one file buffer is open, there is nothing to update. */
  if (openfile == openfile->next) {
    statusline_curses(AHEM, _("No more open file buffers"));
    return;
  }
  /* While in a different buffer, the width of the screen may have changed,
   * so make sure that the starting column for the first row is fitting. */
  ensure_firstcolumn_is_aligned();
  /* Update title bar and multiline info to match the current buffer. */
  prepare_for_display();
  /* Ensure that the main loop will redraw the help lines. */
  currmenu = MMOST;
  /* Prevent a possible Shift selection from getting cancelled. */
  shift_held = TRUE;
  /* If the switched-to buffer gave an error during opening, show the message
   * once; otherwise, indicate on the status bar which file we switched to. */
  if (openfile->errormessage) {
    statusline_curses(ALERT, "%s", openfile->errormessage);
    free(openfile->errormessage);
    openfile->errormessage = NULL;
  }
  else {
    mention_name_and_linecount();
  }
}

/* Switch to the previous entry in the circular list of buffers. */
void switch_to_prev_buffer(void) {
  openfile = openfile->prev;
  redecorate_after_switch();
}

/* Switch to the next entry in the circular list of buffers. */
void switch_to_next_buffer(void) {
  openfile = openfile->next;
  redecorate_after_switch();
}
