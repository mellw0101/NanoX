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
#ifdef LUMPSIZE
# undef LUMPSIZE
#endif

#define RW_FOR_ALL  (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define LOCKSIZE    (1024)
#define SKIPTHISFILE ((char *)-1)

#define LOCKING_PREFIX  "."
#define LOCKING_SUFFIX  ".swp"

/* The number of bytes by which we expand the line buffer while reading. */
#define LUMPSIZE  (120)


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* First check if a lock file already exists.  If so, and ask_the_user is TRUE, then ask whether to open the corresponding file
 * anyway.  Return SKIPTHISFILE when the user answers "No", return the name of the lock file on success, and return NULL on failure. */
/* static */ char *do_lockfile(const char *const restrict filename, bool ask_the_user) {
  char *namecopy     = copy_of(filename);
  char *secondcopy   = copy_of(filename);
  Ulong locknamelen  = (strlen(filename) + STRLEN(LOCKING_PREFIX) + STRLEN(LOCKING_SUFFIX));
  char *lockfilename = xmalloc(locknamelen);
  char lockprog[11];
  char lockuser[17];
  char *lockbuf;
  char *question;
  char *pidstring;
  char *postedname;
  char *promptstr;
  int lockfd;
  int lockpid;
  int choice;
  long readamt;
  struct stat info;
  snprintf(lockfilename, locknamelen, "%s/%s%s%s", dirname(namecopy), LOCKING_PREFIX, basename(secondcopy), LOCKING_SUFFIX);
  free(secondcopy);
  free(namecopy);
  if ((ISSET(USING_GUI) || !ask_the_user) && stat(lockfilename, &info) != -1) {
    if (!ISSET(USING_GUI) && !ISSET(NO_NCURSES)) {
      blank_bottombars();
      statusline(ALERT, _("Someone else is also editing this file"));
      napms(1200);
    }
    else {
      statusline(ALERT, _("Someone else is also editing this file"));
    }
  }
  else if (stat(lockfilename, &info) != -1) {
    if ((lockfd = open(lockfilename, O_RDONLY)) < 0) {
      statusline(ALERT, _("Error opening lock file %s: %s"), lockfilename, strerror(errno));
      free(lockfilename);
      return NULL;
    }
    lockbuf = xmalloc(LOCKSIZE);
    readamt = read(lockfd, lockbuf, LOCKSIZE);
    close(lockfd);
    /* If not enough data has been read to show the needed things, or the two magic bytes are not there, skip the lock file. */
    if (readamt < 68 || lockbuf[0] != 0x62 || lockbuf[1] != 0x30) {
      statusline(ALERT, _("Bad lock file is ignored: %s"), lockfilename);
      free(lockfilename);
      free(lockbuf);
      return NULL;
    }
    memcpy(lockprog, &lockbuf[2], 10);
    lockprog[10] = '\0';
    lockpid = ((((Uchar)lockbuf[27] * 256 + (Uchar)lockbuf[26]) * 256 + (Uchar)lockbuf[25]) * 256 + (Uchar)lockbuf[24]);
    memcpy(lockuser, &lockbuf[28], 16);
    lockuser[16] = '\0';
    free(lockbuf);
    pidstring = xmalloc(11);
    snprintf(pidstring, 11, "%u", (Uint)lockpid);
    /* Display newlines in filenames as ^J. */
    as_an_at = FALSE;
    /* TRANSLATORS: The second %s is the name of the user, the third that of the editor. */
    question   = _("File %s is beeing edited by %s (with %s, PID %s); open anyway?");
    postedname = crop_to_fit(filename, (COLS - breadth(question) - breadth(lockuser) - breadth(lockprog) - breadth(pidstring) + 7));
    /* Allow extra space for username (14), program name (8), PID (8), and terminating '\0' (1), minus the %s (2) for the filename.  Total (29). */
    promptstr = xmalloc(strlen(question) + 29 + strlen(postedname));
    /* TODO: Change this to snprintf. */
    sprintf(promptstr, question, postedname, lockuser, lockprog, pidstring);
    free(postedname);
    free(pidstring);
    choice = ask_user(YESORNO, promptstr);
    free(promptstr);
    /* When the user cancelled while we're still starting up, quit. */
    if (choice == CANCEL && !we_are_running) {
      finish();
    }
    if (choice != YES) {
      free(lockfilename);
      wipe_statusbar();
      return SKIPTHISFILE;
    }
  }
  if (write_lockfile(lockfilename, filename, FALSE)) {
    return lockfilename;
  }
  free(lockfilename);
  return NULL;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Add an item to the circular list of openfile structs ensuring correctness by passing a ptr to the start ptr in the list and the currently open one. */
void make_new_buffer_for(openfilestruct **const start, openfilestruct **const open) {
  ASSERT(start);
  ASSERT(open);
  openfilestruct *node = xmalloc(sizeof(*node));
  if (!*open) {
    /* Make the first buffer the only element in the list. */
    CLIST_INIT(node);
    (*start) = node;
  }
  else {
    /* Add the new buffer after the current one in the list. */
    CLIST_INSERT_AFTER(node, *open);
    /* There is more than one buffer: show "Close" in help lines. */
    ASSIGN_FIELD_IF_VALID(exitfunc, tag, close_tag);
    more_than_one = (!inhelp || more_than_one);
  }
  /* Make the new buffer the current one, and start initializing it */
  (*open)                = node;
  (*open)->filename      = COPY_OF("");
  (*open)->filetop       = make_new_node(NULL);
  (*open)->filetop->data = COPY_OF("");
  (*open)->filebot       = (*open)->filetop;
  (*open)->current       = (*open)->filetop;
  (*open)->current_x     = 0;
  (*open)->placewewant   = 0;
  (*open)->cursor_row    = 0;
  (*open)->edittop       = (*open)->filetop;
  (*open)->firstcolumn   = 0;
  (*open)->totsize       = 0;
  (*open)->modified      = FALSE;
  (*open)->spillage_line = NULL;
  (*open)->mark          = NULL;
  (*open)->softmark      = FALSE;
  (*open)->fmt           = UNSPECIFIED;
  (*open)->undotop       = NULL;
  (*open)->current_undo  = NULL;
  (*open)->last_saved    = NULL;
  (*open)->last_action   = OTHER;
  (*open)->statinfo      = NULL;
  (*open)->lock_filename = NULL;
  (*open)->errormessage  = NULL;
  (*open)->syntax        = NULL;
}

/* Add an item to the circular list of openfile structs. */
void make_new_buffer(void) {
  if (IN_GUI_CONTEXT) {
    make_new_buffer_for(&openeditor->startfile, &openeditor->openfile);
  }
  else {
    make_new_buffer_for(&startfile, &openfile);
  }
  // openfilestruct *newnode = xmalloc(sizeof(*newnode));
  // if (!openfile) {
  //   /* Make the first buffer the only element in the list. */
  //   CLIST_INIT(newnode);
  //   startfile = newnode;
  // }
  // else {
  //   writef("%s: hello\n", __func__);
  //   /* Add the new buffer after the current one in the list. */
  //   CLIST_INSERT_AFTER(newnode, openfile);
  //   /* There is more than one buffer: show "Close" in help lines. */
  //   exitfunc->tag = close_tag;
  //   more_than_one = (!inhelp || more_than_one);
  // }
  // /* Make the new buffer the current one, and start initializing it */
  // openfile                = newnode;
  // openfile->filename      = COPY_OF("");
  // openfile->filetop       = make_new_node(NULL);
  // openfile->filetop->data = COPY_OF("");
  // openfile->filebot       = openfile->filetop;
  // openfile->current       = openfile->filetop;
  // openfile->current_x     = 0;
  // openfile->placewewant   = 0;
  // openfile->cursor_row    = 0;
  // openfile->edittop       = openfile->filetop;
  // openfile->firstcolumn   = 0;
  // openfile->totsize       = 0;
  // openfile->modified      = FALSE;
  // openfile->spillage_line = NULL;
  // openfile->mark          = NULL;
  // openfile->softmark      = FALSE;
  // openfile->fmt           = UNSPECIFIED;
  // openfile->undotop       = NULL;
  // openfile->current_undo  = NULL;
  // openfile->last_saved    = NULL;
  // openfile->last_action   = OTHER;
  // openfile->statinfo      = NULL;
  // openfile->lock_filename = NULL;
  // openfile->errormessage  = NULL;
  // openfile->syntax        = NULL;
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
    titlebar(NULL);
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
    statusline(
      HUSH, P_("%s -- %zu line (%s)", "%s -- %zu lines (%s)", count),
      ((!*file->filename) ? _("New Buffer") : tail(file->filename)),
      count, ((file->fmt == DOS_FILE) ? _("DOS") : _("Mac")));
  }
  else {
    statusline(HUSH, P_("%s -- %zu line", "%s -- %zu lines", count), ((file->filename[0] == '\0') ? _("New Buffer") : tail(file->filename)), count);
  }
}

/* Show name of the current buffer and its number of lines on the statusbar. */
void mention_name_and_linecount(void) {
  mention_name_and_linecount_for(ISSET(USING_GUI) ? openeditor->openfile : openfile);
}

/* Delete the lock file.  Return TRUE on success, and FALSE otherwise. */
bool delete_lockfile(const char *const restrict lockfilename) {
  if (unlink(lockfilename) < 0 && errno != ENOENT) {
    statusline(MILD, _("Error deleting lock file %s: %s"), lockfilename, strerror(errno));
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
    statusline(MILD, _("Couldn't determin hostname: %s"), strerror(errno));
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
    statusline(MILD, _("Error opening file-descriptor: %s: %s"), lockfilename, strerror(errno));
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
    statusline(MILD, _("Error writing lock file %s: %s"), lockfilename, strerror(errno));
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
    statusline(ALERT, _("The working directory has disappeared"));
  }
  else if (stat(parentdir, &parentinfo) == -1) {
    if (errno == ENOENT) {
      /* TRANSLATORS: Keep the next ten messages at most 76 characters. */
      statusline(ALERT, _("Directory '%s' does not exist"), parentdir);
    }
    else {
      statusline(ALERT, _("Path '%s': %s"), parentdir, strerror(errno));
    }
  }
  else if (!S_ISDIR(parentinfo.st_mode)) {
    statusline(ALERT, _("Path '%s' is not a directory"), parentdir);
  }
  else if (access(parentdir, X_OK) == -1) {
    statusline(ALERT, _("Path '%s' is not accessible"), parentdir);
  }
  else if (ISSET(LOCKING) && !ISSET(VIEW_MODE) && access(parentdir, W_OK) < 0) {
    statusline(MILD, _("Directory '%s' is not writable"), parentdir);
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
    CLIST_ADV_NEXT(*start);
  }
  CLIST_UNLINK(orphan);
  free(orphan->filename);
  free_lines_for(NULL, orphan->filetop);
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
  if (*open && CLIST_SINGLE(*open)) {
    ASSIGN_FIELD_IF_VALID(exitfunc, tag, exit_tag);
  }
}

void close_buffer_for(openfilestruct *const orphan, openfilestruct **const start, openfilestruct **const open) {
  /* If the buffer to free is the start buffer, advance the start buffer. */
  if (orphan == *start) {
    CLIST_ADV_NEXT(*start);
  }
  CLIST_UNLINK(orphan);
  free(orphan->filename);
  free_lines_for(NULL, orphan->filetop);
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
  if (*open && CLIST_SINGLE(*open)) {
    ASSIGN_FIELD_IF_VALID(exitfunc, tag, exit_tag);
  }
}

/* Remove the current buffer from the circular list of buffers.  When just one buffer remains open, show "Exit" in the help lines. */
void close_buffer(void) {
  if (IN_GUI_CONTEXT) {
    close_buffer_for(openeditor->openfile, &openeditor->startfile, &openeditor->openfile);
  }
  else {
    close_buffer_for(openfile, &startfile, &openfile);
  }
  // openfilestruct *orphan = openfile;
  // if (orphan == startfile) {
  //   startfile = startfile->next;
  // }
  // CLIST_UNLINK(orphan);
  // free(orphan->filename);
  // free_lines(orphan->filetop);
  // free(orphan->statinfo);
  // free(orphan->lock_filename);
  // /* Free the undo stack. */
  // discard_until(NULL);
  // free(orphan->errormessage);
  // openfile = orphan->prev;
  // if (openfile == orphan) {
  //   openfile = NULL;
  // }
  // free(orphan);
  // /* When just one buffer remains open, show "Exit" in the help lines. */
  // if (openfile && CLIST_SINGLE(openfile)) {
  //   ASSIGN_FIELD_IF_VALID(exitfunc, tag, exit_tag);
  // }
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
  if (!ISSET(USING_GUI) && !ISSET(NO_NCURSES)) {
    titlebar(NULL);
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

/* Change to the specified operating directory, when its valid.  TODO: Make sure this works correctly. */
void init_operating_dir(void) {
  char *target = get_full_path(operating_dir);
  /* If the operating directory is inaccessible, fail. */
  if (!target || chdir(target) == -1) {
    die(_("Invalid operating directory: %s\n"), operating_dir);
  }
  operating_dir = free_and_assign(operating_dir, target);
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
  if (CLIST_SINGLE(openfile)) {
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

/* This function will return the name of the first available extension of a filename
 * (starting with [name][suffix], then [name][suffix].1,etc.).  Memory is allocated
 * or the return value.  If no writable extension exists, we return "". */
char *get_next_filename(const char *const restrict name, const char *const restrict suffix) {
  Ulong wholenamelen = (strlen(name) + strlen(suffix));
  Ulong i = 0;
  char *buf;
  struct stat fs;
  /* Reserve space for, the name plus the suffix plus a dot plus possibly five digits plus a null byte. */
  buf = xmalloc(wholenamelen + 7);
  sprintf(buf, "%s%s", name, suffix);
  while (TRUE) {
    if (stat(buf, &fs) == -1) {
      return buf;
    }
    /* Limit the number of backup files to a hundred thousand. */
    if (++i == 100000) {
      break;
    }
    sprintf((buf + wholenamelen), ".%lu", i);
  }
  /* There is no possible save file: blank out the filename. */
  *buf = '\0';
  return buf;
}

/* Open the file with the given name.  If the file does not exist, display
 * "New File" if `new_one` is `TRUE`, and say "File not found" otherwise.
 * Retrurns 0 if we say "New File", `-1` upon failure, and the obtained file
 * descriptor otherwise.  The opened filestream is returned in `*f`. */
int open_file(const char *const restrict path, bool new_one, FILE **const f) {
  ASSERT(f);
  int fd;
  char *full_path = get_full_path(path);
  struct stat info;
  /* If the absolute path is unusable (due to some component's permissions), try the given path instead (as it's probebly relative). */
  if (!full_path || stat(full_path, &info) == -1) {
    full_path = realloc_strcpy(full_path, path);
  }
  if (stat(full_path, &info) == -1) {
    free(full_path);
    if (new_one) {
      statusline(REMARK, _("New File"));
      return 0;
    }
    else {
      statusline(ALERT, _("File \"%s\" not found"), path);
      return -1;
    }
  }
  if (S_ISFIFO(info.st_mode)) {
    statusbar_all(_("Reading from FIFO..."));
  }
  block_sigwinch(TRUE);
  install_handler_for_Ctrl_C();
  /* Try to open the file. */
  fd = open(full_path, O_RDONLY);
  restore_handler_for_Ctrl_C();
  block_sigwinch(FALSE);
  /* There was an error opening the file-descriptor. */
  if (fd == -1) {
    /* The errno either indecates an interuption caused the failed call to open, or that there is no error (errno == 0)
     * (this shouldn't happen), but if it does, it likely means that the interuption ran something that succeded. */
    if (errno == EINTR || !errno) {
      statusline(ALERT, _("Interrupted"));
    }
    else {
      statusline(ALERT, _("Error reading %s: %s"), path, strerror(errno));
    }
  }
  /* Successfully opened the file-descriptor accisiated with `path`. */
  else {
    *f = fdopen(fd, "rb");
    /* There was an error opening the `FILE *`. */
    if (!*f) {
      statusline(ALERT, _("Error reading %s: %s"), path, strerror(errno));
      close(fd);
      fd = -1;
    }
    else if (!ISSET(ZERO) || we_are_running) {
      statusbar_all(_("Reading..."));
    }
  }
  free(full_path);
  return fd;
}

/* ----------------------------- Read file ----------------------------- */

/* Read the given open file `f` into the buffer `file`.  filename should be
 * set to the name of the file.  `undoable` means that undo records should be
 * created and that the file does not need to be checked for writability. */
void read_file_into(openfilestruct *const file, int rows, int cols, FILE *const f, int fd, const char *const restrict filename, bool undoable) {
  ASSERT(file);
  ASSERT(f);
  ASSERT(filename);
  /* The line number where we start the insertion. */
  long was_lineno = file->current->lineno;
  /* The leftedge where we start the insertion. */
  Ulong was_leftedge = 0;
  /* The number of lines in the file. */
  Ulong num_lines = 0;
  /* The length of the current line of the file. */
  Ulong len = 0;
  /* The size of the line buffer.  Will be increased as needed. */
  Ulong bufsize = LUMPSIZE;
  /* The buffer in which we assemble each line of the file. */
  char *buf = xmalloc(bufsize);
  /* The top of the new buffer where we store the read file. */
  linestruct *top;
  /* The bottom line of the new buffer. */
  linestruct *bot;
  /* The current value we read from the file, either a byte or `EOF`. */
  int value;
  /* The error code, in case an error occured during reading. */
  int errornum;
  /* Whether the file is writable (in case we care (What...?)). */
  bool writable = TRUE;
  bool mac_line_needs_newline = FALSE;
  /* The type of line ending the file uses: Unix, DOS, or Mac. */
  format_type format = NIX_FILE;
  /* The char we are currently processing. */
  char input;
  /* When the caller knows we can write to this file. */
  if (undoable) {
    add_undo_for(file, INSERT, NULL);
  }
  /* If soft-wrapping is enabled. */
  if (ISSET(SOFTWRAP)) {
    was_leftedge = leftedge_for(xplustabs_for(file), file->current, cols);
  }
  /* Create an empty buffer. */
  top = make_new_node(NULL);
  bot = top;
  block_sigwinch(TRUE);
  /* Lock the file before starting to read it, to avoid the overhead of locking it for each single byte we read from it.
   * Note: This way of reading a file is not the most efficient nor safe way, as it perform alot of operations while holding
   * the global lock, and this is not needed i will make a pure file-desctiptor based pipeline to perform these actions. */
  flockfile(f);
  control_C_was_pressed = FALSE;
  /* Read in the entire file, byte by byte, line by line.  Like i said this is not optimal, as we are holding the lock. */
  while ((value = getc_unlocked(f)) != EOF) {
    input = (char)value;
    if (control_C_was_pressed) {
      break;
    }
    /* When the byte before the current one is a `CR` and automatic format conversion has not
     * been switched off, then strip this `CR` when it's before a `LF` or when the file is in
     * Mac format.  Also, when this is the first line break, make note of the format. */
    if (input == '\n') {
      if (len > 0 && buf[len - 1] == '\r' && !ISSET(NO_CONVERT)) {
        if (!num_lines) {
          format = DOS_FILE;
        }
        --len;
      }
    }
    else if ((!num_lines || format == MAC_FILE) && len > 0 && buf[len - 1] == '\r' && !ISSET(NO_CONVERT)) {
      format = MAC_FILE;
      --len;
    }
    else {
      /* Store the byte. */
      buf[len] = input;
      /* Keep track of the total length of the line.  It might have `NUL` bytes in it, so we can't just use
       * `strlen()` later.  Note: Why should one ever use `strlen()` in this case, where we have the length, as
       * this is not a question about can or not, rather, there is no need to perform more operation for no reason. */
      ++len;
      /* When needed, increase the line-buffer size.  Don't bother decreasing it -- it get freed when
       * reading is finished.  Note: Um... yes it does as should all dynamicly allocated memory like what?? */
      if (len == bufsize) {
        bufsize += LUMPSIZE;
        buf = xrealloc(buf, bufsize);
      }
      continue;
    }
    /* Store the data and make a new line. */
    bot->data = encode_data(buf, len);
    bot->next = make_new_node(bot);
    DLIST_ADV_NEXT(bot);
    ++num_lines;
    /* Reset the length in preperation for the next line. */
    len = 0;
    /* If it was a Mac line, then store the byte after the \r as the first byte of the next line. */
    if (input != '\n') {
      buf[len++] = input;
    }
  }
  errornum = errno;
  /* We are done with the file, unlock it. */
  funlockfile(f);
  block_sigwinch(FALSE);
  /* When reading from stdin, restore the terminal and reenter curses mode. */
  if (!ISSET(USING_GUI) && !isendwin()) {
    if (!isatty(STDIN_FILENO)) {
      reconnect_and_store_state();
    }
    terminal_init();
    if (!ISSET(NO_NCURSES)) {
      doupdate();
    }
  }
  /* If there was a real error during the reading, let the user know. */
  if (ferror(f) && errornum != EINTR && errornum) {
    statusline(ALERT, strerror(errornum));
  }
  /* The reading of this file was interupted by the user. */
  if (control_C_was_pressed) {
    statusline(ALERT, _("Interrupted"));
  }
  fclose(f);
  if (fd > 0 && !undoable && !ISSET(VIEW_MODE)) {
    writable = (access(filename, W_OK) == 0);
  }
  /* If the file ended in a newline, or it was entirely empty, make the last line blank. */
  if (!len) {
    bot->data = COPY_OF("");
  }
  /* Otherwise, put the last read data in. */
  else {
    /* If the final character is a `CR` and file conversion isn't disabled,
     * strip this `CR` and indecate that an extra blank line is needed. */
    if (buf[len - 1] == '\r' && !ISSET(NO_CONVERT)) {
      /* There is only this line in the file. */
      if (!num_lines) {
        format = MAC_FILE;
      }
      buf[--len] = '\0';
      mac_line_needs_newline = TRUE;
    }
    /* Store the data of the final line. */
    bot->data = encode_data(buf, len);
    ++num_lines;
    if (mac_line_needs_newline) {
      bot->next = make_new_node(bot);
      DLIST_ADV_NEXT(bot);
      bot->data = COPY_OF("");
    }
  }
  free(buf);
  /* Insert the just read buffer into `file`. */
  ingraft_buffer_into(file, top, bot);
  /* Set the desired x position at the end of what was inserted. */
  set_pww_for(file);
  /* If this file is unwritable, inform the user. */
  if (!writable) {
    statusline(ALERT, _("File '%s' is unwritable"), filename);
  }
  /* No blurb for new buffers with --zero or --mini.  Why? */
  else if ((ISSET(ZERO) || ISSET(MINIBAR)) && !(we_are_running && undoable)) {
    ;
  }
  /* Mac file. */
  else if (format == MAC_FILE) {
    statusline(REMARK, P_("Read %lu line (converted from Mac format)", "Read %lu lines (converted from Mac format)", num_lines), num_lines);
  }
  /* DOS file. */
  else if (format == DOS_FILE) {
    statusline(REMARK, P_("Read %lu line (converted from Mac format)", "Read %lu lines (converted from Mac format)", num_lines), num_lines);
  }
  /* Other... */
  else {
    statusline(REMARK, P_("Read %lu line", "Read %lu lines", num_lines), num_lines);
  }
  report_size = TRUE;
  if (undoable) {
    /* If we inserted less then a screenful, don't center the cursor. */
    if (less_than_a_screenful_for(file, was_lineno, was_leftedge, rows, cols)) {
      focusing = FALSE;
      perturbed = TRUE;
    }
    else {
      recook = TRUE;
    }
    update_undo_for(file, INSERT);
  }
  if (ISSET(MAKE_IT_UNIX)) {
    file->fmt = NIX_FILE;
  }
  else if (file->fmt == UNSPECIFIED) {
    file->fmt = format;
  }
}

/* Read the given open file f into the current buffer.  filename should be
 * set to the name of the file.  undoable means that undo records should be
 * created and that the file does not need to be checked for writability. */
void read_file(FILE *f, int fd, const char *const restrict filename, bool undoable) {
  if (IN_GUI_CONTEXT) {
    read_file_into(GUI_CONTEXT, f, fd, filename, undoable);
  }
  else {
    read_file_into(TUI_CONTEXT, f, fd, filename, undoable);
  }
}

/* ----------------------------- Open buffer ----------------------------- */

/* This does one of three things.  If the filename is "", it just creates a new empty buffer.  When the filename
 * is not empty, it reads that file into a new buffer when requested, otherwise into the existing buffer. */
bool open_buffer_for(openfilestruct **const start, openfilestruct **const open, int rows, int cols, const char *const restrict path, bool new_one) {
  ASSERT(start);
  ASSERT(open);
  /* The filename after tilde expansion. */
  char *full_path;
  /* The filename of the lockfile. */
  char *lock_path;
  struct stat info;
  /* Code 0 means a new file, -1 means failure, and else its the fd. */
  int fd = 0;
  FILE *f;
  /* Display newlines in filenames as ^J. */
  as_an_at = FALSE;
  if (outside_of_confinement(path, FALSE)) {
    statusline(ALERT, _("Can't read file from outside of %s"), operating_dir);
    return FALSE;
  }
  full_path = real_dir_from_tilde(path);
  /* Dont try to open directories, character files, or block files. */
  if (*path && stat(full_path, &info) == 0) {
    /* Directory */
    if (S_ISDIR(info.st_mode)) {
      statusline(ALERT, _("\"%s\" is a directory"), full_path);
      free(full_path);
      return FALSE;
    }
    /* Device */
    else if (S_ISCHR(info.st_mode) || S_ISBLK(info.st_mode)) {
      statusline(ALERT, _("\"%s\" is a device file"), full_path);
      free(full_path);
      return FALSE;
    }
    /* Read-Only */
    else if (new_one && !(info.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) && getuid() == ROOT_UID) {
      statusline(ALERT, _("\"%s\" is meant to be read-only"), full_path);
    }
  }
  /* When loading in a new buffer, first check the file's path is valid, and then (if requested and possible) create a lock file for it. */
  if (new_one) {
    make_new_buffer_for(start, open);
    if (has_valid_path(full_path) && ISSET(LOCKING) && !ISSET(VIEW_MODE) && *path) {
      lock_path = do_lockfile(full_path, TRUE);
      /* When not overriding an existing lock, discard the buffer.  TODO: This needs to be prompted for the gui as well, witch we can do but is
       * a bit more involved, also i will remake the gui prompt pipeline, and create a response function that gets called when a `Y/N` is needed. */
      if (lock_path == SKIPTHISFILE) {
        close_buffer_for(*open, start, open);
        free(full_path);
        return FALSE;
      }
      else {
        (*open)->lock_filename = lock_path;
      }
    }
  }
  /* If we have a path and are not in `NOREAD_MODE`, open the file. */
  if (*path && !ISSET(NOREAD_MODE)) {
    fd = open_file(full_path, new_one, &f);
  }
  /* If we've successfully opened an existing file, read it in. */
  if (fd > 0) {
    install_handler_for_Ctrl_C();
    read_file_into(*open, rows, cols, f, fd, full_path, !new_one);
    restore_handler_for_Ctrl_C();
    /* If the currently open buffer does not have the allocated stat struct for `full_path`, then allocate it. */
    if (!(*open)->statinfo) {
      stat_with_alloc(full_path, &(*open)->statinfo);
    }
  }
  /* For a new buffer, store filename and put cursor at start of buffer. */
  if (fd >= 0 && new_one) {
    (*open)->filename    = realloc_strcpy((*open)->filename, full_path);
    (*open)->current     = (*open)->filetop;
    (*open)->current_x   = 0;
    (*open)->placewewant = 0;
  }
  /* If a new buffer was opened, check wether a syntax can be applied. */
  if (new_one) {
    find_and_prime_applicable_syntax_for(*open);
    syntax_check_file(*open);
  }
  free(full_path);
  return TRUE;
}

/* This does one of three things.  If the filename is "", it just creates a new empty buffer.  When the filename
 * is not empty, it reads that file into a new buffer when requested, otherwise into the existing buffer. */
bool open_buffer(const char *const restrict path, bool new_one) {
  if (IN_GUI_CONTEXT) {
    return open_buffer_for(FULL_GUI_CONTEXT, path, new_one);
  }
  else {
    return open_buffer_for(FULL_TUI_CONTEXT, path, new_one);
  }
}
