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

#define LOCKSIZE      (1024)
#define SKIPTHISFILE  ((char *)-1)

#define LOCKING_PREFIX  "."
#define LOCKING_SUFFIX  ".swp"

/* The number of bytes by which we expand the line buffer while reading. */
#define LUMPSIZE  (120)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The PID of a forked process -- needed when wanting to abort it. */
/* static */ pid_t pid_of_command = -1;
/* The PID of the process that pipes data to the above process. */
/* static */ pid_t pid_of_sender = -1;
/* Whether we are pipeing data to the external command. */
/* static */ bool should_pipe = FALSE;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Do lockfile ----------------------------- */

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

/* ----------------------------- Filename completion ----------------------------- */

/* Try to complete the given fragment to an existing filename.  Note remember to set `present_path` for relative paths. */
/* static */ char **filename_completion(const char *const restrict morsel, Ulong *const num_matches) {
  ASSERT(morsel);
  char *dirname = copy_of(morsel);
  char *slash   = strrchr(dirname, '/');
  char *filename;
  char *wasdirname;
  char **matches = NULL;
  Ulong size = 0;
  Ulong cap  = 10;
  Ulong filenamelen;
  directory_t dir;
  /* If there's a '/' in the name, split out the filename and directory parts. */
  if (slash) {
    wasdirname = dirname;
    filename   = copy_of(++slash);
    /* Cut of the the filename part after the slash. */
    *slash = '\0';
    dirname = real_dir_from_tilde(dirname);
    /* A non-absolute path is relative to the current browser directory. */
    if (*dirname != '/') {
      dirname = xrealloc(dirname, (strlen(present_path) + strlen(wasdirname) + 1));
      sprintf(dirname, "%s%s", present_path, wasdirname);
    }
    free(wasdirname);
  }
  else {
    filename = dirname;
    dirname  = copy_of(present_path);
  }
  directory_data_init(&dir);
  if (directory_get(dirname, &dir) == -1) {
    if (IN_CURSES_CONTEXT) {
      beep();
    }
    directory_data_free(&dir);
    free(filename);
    free(dirname);
    return NULL;
  }
  filenamelen = strlen(filename);
  /* Allocate the array. */
  matches = xmalloc(_PTRSIZE * cap);
  /* Iterate through the filenames in the directory, and add each fitting one to the list of matches. */
  DIRECTORY_ITER(dir, i, entry,
    if (strncmp(entry->name, filename, filenamelen) == 0) {
      if (outside_of_confinement(entry->path, TRUE) || (currmenu == MGOTODIR && entry->type != DT_DIR)) {
        continue;
      }
      ENSURE_PTR_ARRAY_SIZE(matches, cap, size);
      matches[size++] = measured_copy(entry->name, entry->namelen);
    }
  );
  directory_data_free(&dir);
  free(dirname);
  free(filename);
  if (!size) {
    free(matches);
    matches = NULL;
  }
  else {
    TRIM_PTR_ARRAY(matches, cap, size);
    matches[size] = NULL;
    ASSIGN_IF_VALID(num_matches, size);
  }
  return matches;
}

/* ----------------------------- Make backup of ----------------------------- */

/* Create a backup of an existing file.  If the user did not request backups,
 * make a temporary one.  (trying first in the directory of the original file,
 * then in the user's home directory).  Return 'TRUE' if the save can proceed. */
/* static */ bool make_backup_of_for(openfilestruct *const file, char *realname) {
  ASSERT(file);
  ASSERT(realname);
  struct timespec filetime[2];
  FILE *original    = NULL;
  FILE *backup_file = NULL;
  int creation_flags;
  int descriptor;
  int verdict;
  bool second_attempt = FALSE;
  char *backupname = NULL;
  char *thename;
  /* Remember the original file's access and modification times. */
  filetime[0].tv_sec = file->statinfo->st_atime;
  filetime[1].tv_sec = file->statinfo->st_mtime;
  statusbar_all(_("Making backup..."));
  /* If no backup directory was specified, we make a simple backup by appending a tilde to the original file name. */
  if (!backup_dir) {
    backupname = fmtstr("%s~", realname);
  }
  /* Otherwise, we create a numbered backup in the specified directory. */
  else {
    thename = get_full_path(realname);
    /* If we have a valid absolute path, replace each slash in this full path with an exclamation mark. */
    if (thename) {
      for (Ulong i=0; thename[i]; ++i) {
        if (thename[i] == '/') {
          thename[i] = '!';
        }
      }
    }
    /* Otherwise, just use the file-name portion of the given path. */
    else {
      thename = copy_of(tail(realname));
    }
    backupname = free_and_assign(thename, fmtstr("%s%s", backup_dir, thename));
    backupname = free_and_assign(backupname, get_next_filename(backupname, "~"));
    /* If all numbered backup names are taken, the use must be fond of backups.  Thus, without one, fo not go on. */
    if (!*backupname) {
      statusline(ALERT, _("To meny existing backup files"));
      free(backupname);
      return FALSE;
    }
  }
  /* Now first try to delete an existing backup file. */
  if (unlink(backupname) < 0 && errno != ENOENT && !ISSET(INSECURE_BACKUP)) {
    goto problem;
  }
  creation_flags = (O_WRONLY | O_CREAT | (ISSET(INSECURE_BACKUP) ? O_TRUNC : O_EXCL));
  /* Create the backup file (or truncate the existing one). */
  descriptor = open(backupname, creation_flags, (S_IRUSR | S_IWUSR));
  retry: {
    if (descriptor >= 0) {
      backup_file = fdopen(descriptor, "wb");
    }
    if (!backup_file) {
      goto problem;
    }
    /* Try to change owner and group to those of the original file.  Ignore
     * permission errors, as a normal user cannot change the owner.  What...? */
    if (fchown(descriptor, file->statinfo->st_uid, file->statinfo->st_gid) < 0 && errno != EPERM) {
      fclose(backup_file);
      goto problem;
    }
    /* Set the backup's permissions to those of the original file.  It's not a security issue if
     * this fails, as we have created the file with just read and write permission for the owner. */
    if (fchmod(descriptor, file->statinfo->st_mode) < 0 && errno != EPERM) {
      fclose(backup_file);
      goto problem;
    }
    original = fopen(realname, "rb");
    /* If opening succeeded, copy the existion file to the backup. */
    if (original) {
      verdict = copy_file(original, backup_file, FALSE);
    }
    /* We failed read the original file. */
    if (!original || verdict < 0) {
      /* We are in curses-mode. */
      if (IN_CURSES_CTX) {
        warn_and_briefly_pause_curses(_("Cannot read the original file"));
      }
      /* We are in gui-mode. */
      else if (IN_GUI_CTX) {
        statusline_gui(ALERT, _("Cannot read the original file"));
      }
      fclose(backup_file);
      goto failure;
    }
    /* We failed to write to the backup file. */
    else if (verdict > 0) {
      fclose(backup_file);
      goto problem;
    }
    /* Since this backup is a newly created file, explicitly sync it to
     * permanent storage before starting to write out the actual file. */
    if (fflush(backup_file) != 0 || fsync(fileno(backup_file)) != 0) {
      fclose(backup_file);
      goto problem;
    }
    /* Set the backup's timestamps to those of the original file.
     * Failure is unimportant.  Saving the file apparently worked. */
    IGNORE_CALL_RESULT(futimens(descriptor, filetime));
    if (fclose(backup_file) == 0) {
      free(backupname);
      return TRUE;
    }
  }
  problem: {
    get_homedir();
    /* If the first attempt of copying the file failed, try again to $HOME. */
    if (!second_attempt && homedir) {
      unlink(backupname);
      free(backupname);
      if (IN_CURSES_CTX) {
        warn_and_briefly_pause_curses(_("Cannot make regular backup"));
        warn_and_briefly_pause_curses(_("Trying again in your home directory"));
      }
      else if (IN_GUI_CTX) {
        statusline_gui(ALERT, _("Cannot make regular backup  Trying again in your home directory"));
      }
      currmenu       = MMOST;
      backupname     = fmtstr("%s/%s~XXXXXX", homedir, tail(realname));
      descriptor     = mkstemp(backupname);
      backup_file    = NULL;
      second_attempt = TRUE;
      goto retry;
    }
    /* Otherwise, just inform the user of our failure. */
    else {
      if (IN_GUI_CTX) {
        warn_and_briefly_pause_curses(_("Cannot make backup"));
      }
      else if (IN_GUI_CTX) {
        statusline_gui(ALERT, _("Cannot make backup"));
      }
    }
  }
  failure: {
    if (IN_CURSES_CTX) {
      warn_and_briefly_pause_curses(strerror(errno));
    }
    else if (IN_GUI_CTX) {
      statusline_gui(ALERT, "%s", strerror(errno));
    }
    currmenu = MMOST;
    free(backupname);
    /* If both attempts failed, and it isn't because of lack of disk space, ask the user what to do
     * because if something goes wrong during the save of the file itself, its contents may be lost.
     * TRANSLATORS: Try to keep this message at most 76 characters.  TODO: Figure out how to handle
     * this when in gui-mode, as `ask_user()` will always return NO in gui mode as we cannot block. */
    if (errno != ENOSPC && ask_user(YESORNO, _("Cannot make backup; continue and save actual file? ")) == YES) {
      return TRUE;
    }
    /* TRANSLATORS: The %s is the reason of failure. */
    statusline(HUSH, _("Cannot make backup: %s"), strerror(errno));
    return FALSE;
  }
}

/* Create a backup of an existing file.  If the user did not request backups,
 * make a temporary one.  (trying first in the directory of the original file,
 * then in the user's home directory).  Return 'TRUE' if the save can proceed. */
/* static */ bool make_backup_of(char *realname) {
  return make_backup_of_for(CTX_OF, realname);
}

/* ----------------------------- Cancel the command ----------------------------- */

/* Send an unconditional kill signal to the running external command. */
/* static */ void cancel_the_command(int _UNUSED signal) {
  if (pid_of_command > 0) {
    kill(pid_of_command, SIGKILL);
  }
  if (should_pipe && pid_of_sender > 0) {
    kill(pid_of_sender, SIGKILL);
  }
}

/* ----------------------------- Send data ----------------------------- */

/* Send the text that starts at `head` to the file-desctiptor `fd`.  TODO: Make this fully fd based. */
/* static */ void send_data(const linestruct *head, int fd) {
  ASSERT(head);
  ASSERT(fd >= 0);
  FILE *pipe = fdopen(fd, "w");
  Ulong length;
  if (!pipe) {
    exit(4);
  }
  /* Send each line, except a final empty line. */
  while (head && (head->next || *head->data)) {
    length = recode_LF_to_NUL(head->data);
    if (fwrite(head->data, 1, length, pipe) < length) {
      exit(5);
    }
    if (head->next && putc('\n', pipe) == EOF) {
      exit(6);
    }
    DLIST_ADV_NEXT(head);
  }
  fclose(pipe);
}

/* ----------------------------- Execute command ----------------------------- */

/* Execute the given command in a shell. */
/* static */ void execute_command_for(CTX_ARGS_REF_OF, const char *const restrict command) {
  ASSERT(file);
  ASSERT(*file);
  ASSERT(command);
  /* The pipes through which text will be written and read. */
  int pipe_from[2];
  int pipe_to[2];
  /* Original and temporary handlers for SIGINT. */
  struct sigaction old_action = {0};
  struct sigaction new_action = {0};
  long was_lineno = ((*file)->mark ? 0 : (*file)->current->lineno);
  int command_status;
  int sender_status;
  FILE *stream;
  const char *shell;
  linestruct *was_cutbuffer;
  bool whole_buffer;
  should_pipe = (*command == '|');
  /* Create a pipe to read the command's output from, and if needed, a pipe to feed the command's input through. */
  if (pipe(pipe_from) == -1 || (should_pipe && pipe(pipe_to) == -1)) {
    statusline(ALERT, _("Could not create pipe: %s"), strerror(errno));
    return;
  }
  /* Fork a child process to run the command in. */
  if ((pid_of_command = fork()) == -1) {
    /* If we fail to fork a child process, inform the user and make a clean getaway. */
    statusline(ALERT, _("Failed to fork a child process: %s"), strerror(errno));
    close(pipe_from[0]);
    close(pipe_from[1]);
    if (should_pipe) {
      close(pipe_to[0]);
      close(pipe_to[1]);
    }
    return;
  }
  /* Child */
  else if (pid_of_command == 0) {
    /* Child: If we fail to get the shell from the env varaible, default to sh. */
    if (!(shell = getenv("SHELL"))) {
      shell = "/bin/sh";
    }
    /* Child: Close the unused read end of the output pipe. */
    close(pipe_from[0]);
    /* Child: Connect the write end of the output pipe to the process's output streams. */
    if (dup2(pipe_from[1], STDOUT_FILENO) < 0) {
      exit(3);
    }
    if (dup2(pipe_from[1], STDERR_FILENO) < 0) {
      exit(4);
    }
    /* Child: If we should pipe the child's input. */
    if (should_pipe) {
      /* Child: If the parent sends text, connect the read end of the feeding pipe to the child's input stream. */
      if (dup2(pipe_to[0], STDIN_FILENO) < 0) {
        exit(5);
      }
      close(pipe_from[1]);
      close(pipe_to[1]);
    }
    /* Child: Run the given command inside the preferred shell. */
    execl(shell, tail(shell), "-c", (command + !!should_pipe), NULL);
    /* If the exec call returns, there has been some error. */
    exit(6);
  }
  /* Parent */
  else {
    /* Parent: Close the unused write end of the pipe. */
    close(pipe_from[1]);
    statusbar_all(_("Executing..."));
    /* Parent: If the command starts with '|', pipe buffer region to the command. */
    if (should_pipe) {
      was_cutbuffer = cutbuffer;
      whole_buffer  = FALSE;
      cutbuffer     = NULL;
      if (ISSET(MULTIBUFFER)) {
        CLIST_ADV_PREV(*file);
        if ((*file)->mark) {
          copy_marked_region_for(*file);
        }
        else {
          whole_buffer = TRUE;
        }
      }
      else {
        /* TRANSLATORS: This one goes with Undid/Redid messages. */
        add_undo_for(*file, COUPLE_BEGIN, N_("filtering"));
        if (!(*file)->mark) {
          (*file)->current   = (*file)->filetop;
          (*file)->current_x = 0;
        }
        add_undo_for(*file, CUT, NULL);
        do_snip_for(STACK_CTX_DF, (*file)->mark, !(*file)->mark, FALSE);
        /* Parent: If there is only a single line in the (*file). */
        if (!(*file)->filetop->next) {
          (*file)->filetop->has_anchor = FALSE;
        }
        update_undo_for(*file, CUT);
      }
      /* Parent: Create a seperate process for piping the data to the command.*/
      if ((pid_of_sender = fork()) == 0) {
        send_data((whole_buffer ? (*file)->filetop : cutbuffer), pipe_to[1]);
        exit(0);
      }
      if (pid_of_sender == -1) {
        statusline(ALERT, _("Could not fork: %s"), strerror(errno));
      }
      close(pipe_to[0]);
      close(pipe_to[1]);
      if (ISSET(MULTIBUFFER)) {
        CLIST_ADV_NEXT(*file);
      }
      free_lines(cutbuffer);
      cutbuffer = was_cutbuffer;
    }
    /* Parent: Re-enable interpretation of the special control keys so that we can get SIGINT when Ctrl-C is pressed. */
    enable_kb_interrupt();
    /* Set up a signal handler so that ^C will terminate the forked process. */
    new_action.sa_handler = cancel_the_command;
    new_action.sa_flags   = 0;
    sigaction(SIGINT, &new_action, &old_action);
    stream = fdopen(pipe_from[0], "rb");
    if (!stream) {
      statusline(ALERT, _("Failed to open pipe: %s"), strerror(errno));
    }
    else {
      read_file_into(STACK_CTX_DF, stream, 0, "pipe", TRUE);
    }
    if (should_pipe && !ISSET(MULTIBUFFER)) {
      if (was_lineno) {
        goto_line_posx_for(*file, rows, was_lineno, 0);
      }
      add_undo_for(*file, COUPLE_END, N_("filtering"));
    }
    /* Wait for the external command (and possible data sender) to terminate. */
    waitpid(pid_of_command, &command_status, 0);
    if (should_pipe && pid_of_sender > 0) {
      waitpid(pid_of_sender, &sender_status, 0);
    }
    /* If the command failed, show what the shell reported. */
    if (!WIFEXITED(command_status) || WEXITSTATUS(command_status)) {
      statusline(ALERT, WIFSIGNALED(command_status) ? _("Cancelled") : _("Error %s"),
        (((*file)->current->prev) && strstr((*file)->current->prev->data, ": "))
          ? (strstr((*file)->current->prev->data, ": ") + 2) : "---");
    }
    else if (should_pipe && pid_of_sender > 0 && (!WIFEXITED(sender_status) || WEXITSTATUS(sender_status))) {
      statusline(ALERT, _("Piping failed"));
    }
    /* If there was and error, undo and discard what the command did. */
    if (lastmessage == ALERT) {
      do_undo_for(STACK_CTX_DF);
      discard_until_for(*file, (*file)->current_undo);
    }
    /* Restore the original handler for SIGINT. */
    sigaction(SIGINT, &old_action, NULL);
    /* Restore the terminal to it's desired state, and disable interpretation of the special control keys again. */
    terminal_init();
  }
}

/* Execute the given command in a shell.  Note that this is `context-safe`. */
/* static */ void execute_command(const char *const restrict command) {
  if (IN_GUI_CTX) {
    execute_command_for(&GUI_OF, GUI_RC, command);
  }
  else {
    execute_command_for(&TUI_OF, TUI_RC, command);
  }
}

/* ----------------------------- Insert a file or ----------------------------- */

/* Insert a file into `*open` (or into a `new buffer`).  But when `execute` is `TRUE`, run a command in
 * the shell and insert it's output into the buffer, or just run one or the tools listed in the help lines. */
/* static */ void insert_a_file_or_for(FULL_CTX_ARGS, bool execute) {
  ASSERT(start);
  ASSERT(open);
  ASSERT(*start);
  ASSERT(*open);
  int response;
  const char *msg;
  /* The last answer the user typed at the status-bar prompt. */
  char *given;
  /* When using the browser, this is the user-chosen answer. */
  char *chosen;
  long priorline;
  long priorcol;
  bool  was_multibuffer = ISSET(MULTIBUFFER);
  long  was_lineno;
  Ulong was_x;
  functionptrtype function;
  /* Display newlines in filenames as ^J. */
  as_an_at = FALSE;
  /* Reset the flag that is set by the Spell Checker and Linter and such. */
  ran_a_tool = FALSE;
  /* For now just allow curses-mode operation. */
  if (IN_CURSES_CTX) {
    given = COPY_OF("");
    while (1) {
      /* TRANSLATORS: The next six messages are prompts. */
      if (execute) {
        if (ISSET(MULTIBUFFER)) {
          msg = _("Command to execute in new buffer");
        }
        else {
          msg = _("Command to execute");
        }
      }
      else {
        if (ISSET(MULTIBUFFER)) {
          if (ISSET(NO_CONVERT)) {
            msg = _("File to read unconverted into new buffer [from %s]");
          }
          else {
            msg = _("File to read into new buffer [from %s]");
          }
        }
        else {
          if (ISSET(NO_CONVERT)) {
            msg = _("File to insert unconverted [from %s]");
          }
          else {
            msg = _("File to insert [from %s]");
          }
        }
      }
      present_path = xstrcpy(present_path, "./");
      response     = do_prompt((execute ? MEXECUTE : MINSERTFILE), given,
        (execute ? &execute_history : NULL), edit_refresh, msg, (operating_dir ? operating_dir : "./"));
      /* If we're in multibuffer-mode and the filename or command is blank, open a new buffer instead of canceling. */
      if (response == -1 || (response == -2 && !ISSET(MULTIBUFFER))) {
        statusbar_all(_("Cancelled"));
        break;
      }
      else {
        was_lineno = (*open)->current->lineno;
        was_x      = (*open)->current_x;
        function   = func_from_key(response);
        given      = xstrcpy(given, answer);
        if (ran_a_tool) {
          break;
        }
        else if (function == flip_newbuffer) {
          /* Allow toggling only when not in view-mode. */
          if (!ISSET(VIEW_MODE)) {
            TOGGLE(MULTIBUFFER);
          }
          else {
            beep();
          }
          continue;
        }
        else if (function == flip_convert) {
          TOGGLE(NO_CONVERT);
          continue;  
        }
        else if (function == flip_execute) {
          execute = !execute;
          continue;
        }
        else if (function == flip_pipe) {
          add_or_remove_pipe_symbol_from_answer();
          given = xstrcpy(given, answer);
          continue;
        }
        else if (function == to_files) {
          chosen = browse_in(answer);
          /* If no file was chosen, go back to the prompt. */
          if (!chosen) {
            continue;
          }
          answer   = free_and_assign(answer, chosen);
          response = 0;
        }
        /* If we dont have a file yet, go back to the prompt. */
        if (response && (!ISSET(MULTIBUFFER) || response != -2)) {
          continue;
        }
        else if (execute) {
          /* When in multibuffer mode, first open a blank buffer. */
          if (ISSET(MULTIBUFFER)) {
            open_buffer_for(FULL_STACK_CTX, "", TRUE);
          }
          /* If the command is not empty, execute it and read it's output
           * into the buffer, and add the command to the history list. */
          if (*answer) {
            execute_command_for(open, rows, cols, answer);
            update_history(&execute_history, answer, PRUNE_DUPLICATE);
          }
          /* If this is a new buffer, put the cursor at the top. */
          if (ISSET(MULTIBUFFER)) {
            (*open)->current     = (*open)->filetop;
            (*open)->current_x   = 0;
            (*open)->placewewant = 0;
            set_modified_for(*open);
          }
        }
        else {
          /* Make sure the specified path is tilde-expanded. */
          answer = free_and_assign(answer, real_dir_from_tilde(answer));
          /* Read the file into a new buffer or into the current buffer. */
          open_buffer_for(FULL_STACK_CTX, answer, ISSET(MULTIBUFFER));
        }
        if (ISSET(MULTIBUFFER)) {
          if (ISSET(POSITIONLOG) && !execute && has_old_position(answer, &priorline, &priorcol)) {
            goto_line_and_column_for(*open, rows, cols, priorline, priorcol, FALSE, FALSE);
          }
          /* Update title-bar and color info for this new buffer. */
          prepare_for_display_for(*open);
        }
        else {
          /* If the buffer actually changed, mark it as modified. */
          if ((*open)->current->lineno != was_lineno || (*open)->current_x != was_x) {
            set_modified_for(*open);
          }
          refresh_needed = TRUE;
        }
        break;
      }
    }
    free(given);
    if (was_multibuffer) {
      SET(MULTIBUFFER);
    }
    else {
      UNSET(MULTIBUFFER);
    }
  }
}

/* Insert a file into the `currently open buffer` (or into a `new buffer`).  But when `execute` is `TRUE`, run a
 * command in the shell and insert it's output into the buffer, or just run one or the tools listed in the help lines. */
/* static */ void insert_a_file_or(bool execute) {
  FULL_CTX_CALL_WARGS(insert_a_file_or_for, execute);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Make new buffer ----------------------------- */

/* Add an item to the circular list of openfile structs ensuring correctness
 * by passing a ptr to the start ptr in the list and the currently open one. */
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

/* Add an item to the circular list of openfile structs.  Note that this is `context-safe`. */
void make_new_buffer(void) {
  if (IN_GUI_CTX) {
    make_new_buffer_for(&GUI_SF, &GUI_OF);
  }
  else {
    make_new_buffer_for(&TUI_SF, &TUI_OF);
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

/* ----------------------------- Crop to fit ----------------------------- */

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

/* ----------------------------- Stat with alloc ----------------------------- */

/* Perform a stat call on the given filename, allocating a stat struct if necessary. On success,
 * '*pstat' points to the stat's result.  On failure, '*pstat' is freed and made 'NULL'. */
void stat_with_alloc(const char *filename, struct stat **pstat) {
  !*pstat ? (*pstat = malloc(sizeof(**pstat))) : 0;
  if (stat(filename, *pstat)) {
    free(*pstat);
    *pstat = NULL;
  }
}

/* ----------------------------- Prepare for display ----------------------------- */

/* Update the title-bar and multiline cache to match `file`. */
void prepare_for_display_for(openfilestruct *const file) {
  ASSERT(file);
  /* Only perform any action when in curses-mode. */
  if (IN_CURSES_CTX) {
    /* Update the title-bar, since the filename may have changed. */
    if (!inhelp) {
      titlebar(NULL);
    }
    /* Precalculate the data for any multiline coloring regexes. */
    if (!file->filetop->multidata) {
      precalc_multicolorinfo_for(file);
    }
    have_palette   = FALSE;
  }
  refresh_needed = TRUE;
}

/* Update the title bar and the multiline cache to match the current buffer. */
void prepare_for_display(void) {
  prepare_for_display_for(CTX_OF);
  // /* When using the gui make this function a `No-op` function. */
  // if (ISSET(USING_GUI)) {
  //   return;
  // }
  // /* Update the title bar, since the filename may have changed. */
  // if (!inhelp) {
  //   titlebar(NULL);
  // }
  // /* Precalculate the data for any multiline coloring regexes. */
  // if (!openfile->filetop->multidata) {
  //   precalc_multicolorinfo();
  // }
  // have_palette   = FALSE;
  // refresh_needed = TRUE;
}

/* ----------------------------- Mention name and linecount ----------------------------- */

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

/* ----------------------------- Delete lockfile ----------------------------- */

/* Delete the lock file.  Return TRUE on success, and FALSE otherwise. */
bool delete_lockfile(const char *const restrict lockfilename) {
  if (unlink(lockfilename) < 0 && errno != ENOENT) {
    statusline(MILD, _("Error deleting lock file %s: %s"), lockfilename, strerror(errno));
    return FALSE;
  }
  return TRUE;
}

/* ----------------------------- Write lockfile ----------------------------- */

/* Write a lock file, under the given lockfilename.  This always annihilates an
 * existing version of that file.  Return TRUE on success; FALSE otherwise. */
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
  /* Create the lock data we will write.  And fully clear it. */
  lockdata = xmalloc(LOCKSIZE);
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

/* ----------------------------- Has valid path ----------------------------- */

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

/* ----------------------------- Close buffer ----------------------------- */

void close_buffer_for(openfilestruct *const orphan, openfilestruct **const start, openfilestruct **const open) {
  ASSERT(orphan);
  ASSERT(start);
  ASSERT(open);
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
  if (IN_GUI_CTX) {
    close_buffer_for(GUI_OF, &GUI_SF, &GUI_OF);
  }
  else {
    close_buffer_for(TUI_OF, &TUI_SF, &TUI_OF);
  }
}

/* ----------------------------- Real dir from tilde ----------------------------- */

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

/* ----------------------------- Is dir ----------------------------- */

/* Return 'TRUE' when the given path is a directory. */
bool is_dir(const char *const path) {
  char *thepath = real_dir_from_tilde(path);
  struct stat fileinfo;
  bool retval = (stat(thepath, &fileinfo) != -1 && S_ISDIR(fileinfo.st_mode));
  free(thepath);
  return retval;
}

/* ----------------------------- Get full path ----------------------------- */

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

/* ----------------------------- Check writable directory ----------------------------- */

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

/* ----------------------------- Diralphasort ----------------------------- */

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

/* ----------------------------- Set modified ----------------------------- */

/* Mark `file` as modified if it isn't already, and then update the title-bar to
 * display the buffer's new status.  As well as re-writing the lockfile it there is one. */
void set_modified_for(openfilestruct *const file) {
  ASSERT(file);
  if (file->modified) {
    return;
  }
  file->modified = TRUE;
  if (IN_CURSES_CTX) {
    titlebar(NULL);
  }
  if (file->lock_filename) {
    write_lockfile(file->lock_filename, file->filename, TRUE);
  }
}

/* Mark the `openfile` buffer as modified if it isn't already, and then update the title bar to display the buffer's new status. */
void set_modified(void) {
  set_modified_for(CTX_OF);
}

/* ----------------------------- Encode data ----------------------------- */

/* Encode any NUL bytes in the given line of text (of the given length),
 * and return a dynamically allocated copy of the resultant string. */
char *encode_data(char *text, Ulong length) {
  recode_NUL_to_LF(text, length);
  text[length] = '\0';
  return copy_of(text);
}

/* ----------------------------- Init operating dir ----------------------------- */

/* Change to the specified operating directory, when its valid.  TODO: Make sure this works correctly. */
void init_operating_dir(void) {
  char *target = get_full_path(operating_dir);
  /* If the operating directory is inaccessible, fail. */
  if (!target || chdir(target) == -1) {
    die(_("Invalid operating directory: %s\n"), operating_dir);
  }
  operating_dir = free_and_assign(operating_dir, target);
}

/* ----------------------------- Outside of confinement ----------------------------- */

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

/* ----------------------------- Init backup dir ----------------------------- */

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

/* ----------------------------- Copy file ----------------------------- */

/* Read all data from inn, and write it to out.  File inn must be open for
 * reading, and out for writing.  Return 0 on success, a negative number on
 * read error, and a positive number on write error.  File inn is always
 * closed by this function, out is closed  only if close_out is TRUE. */
int copy_file(FILE *inn, FILE *out, bool close_out) {
  int retval = 0;
  char buf[BUFSIZ];
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

/* ----------------------------- Safe tempfile ----------------------------- */

/* Create, safely, a temporary file in the standard temp directory.
 * On success, return the malloc()ed filename, plus the corresponding
 * file stream opened in read-write mode.  On error, return 'NULL'. */
char *safe_tempfile_for(openfilestruct *const file, FILE **const stream) {
  ASSERT(file);
  ASSERT(stream);
  const char *envdir = getenv("TMPDIR");
  char *tmpdir  = NULL;
  char *tmpfile = NULL;
  char *ext;
  int fd;
  /* First check if the environment variable `$TMPDIR` is set. */
  if (envdir) {
    /* Now check for the writability of the directory `$TMPDIR` points at. */
    tmpdir = check_writable_directory(envdir);
  }
  /* If we still dont have a directory, check `P_tmpdir`. */
  if (!tmpdir) {
    tmpdir = check_writable_directory(P_tmpdir);
  }
  /* We still do not have a directory.  As a last resort we will use `/tmp/`. */
  if (!tmpdir) {
    tmpdir = COPY_OF("/tmp/");
  }
  /* Get the extention of the filename. */
  ext = strrchr(file->filename, '.');
  if (!ext || strchr(ext, '/')) {
    ext = (file->filename + strlen(file->filename));
  }
  /* Create the temp-file-path based on the temp-dir we got. */
  tmpfile = fmtstrcat(tmpdir, "nano.XXXXXX%s", ext);
  fd = mkstemps(tmpfile, strlen(ext));
  /* If we sucessfully open the file-descriptor. */
  if (fd > 0) {
    *stream = fdopen(fd, "r+b");
    /* And also successfully open the FILE * stream, then return the path of the newly created temp-file. */
    if (*stream) {
      return tmpfile;
    }
    close(fd);
  }
  free(tmpfile);
  return NULL;
}

/* Create, safely, a temporary file in the standard temp directory.
 * On success, return the malloc()ed filename, plus the corresponding
 * file stream opened in read-write mode.  On error, return 'NULL'. */
char *safe_tempfile(FILE **const stream) {
  return safe_tempfile_for(CTX_OF, stream);
  // const char *env_dir = getenv("TMPDIR");
  // char *tempdir       = NULL;
  // char *tempfile_name = NULL;
  // char *extension;
  // int descriptor;
  // /* First check the environment variable `$TMPDIR`. */
  // if (env_dir) {
  //   tempdir = check_writable_directory(env_dir);
  // }
  // /* When no environment variable was set, check `P_tmpdir`. */
  // if (!tempdir) {
  //   tempdir = check_writable_directory(P_tmpdir);
  // }
  // /* As a last resort, use `/tmp/`. */
  // if (!tempdir) {
  //   tempdir = COPY_OF("/tmp/");
  // }
  // extension = strrchr(openfile->filename, '.');
  // if (!extension || strchr(extension, '/')) {
  //   extension = openfile->filename + strlen(openfile->filename);
  // }
  // tempfile_name = xrealloc(tempdir, (strlen(tempdir) + 12 + strlen(extension)));
  // strcat(tempfile_name, "nano.XXXXXX");
  // strcat(tempfile_name, extension);
  // descriptor = mkstemps(tempfile_name, strlen(extension));
  // *stream = ((descriptor > 0) ? fdopen(descriptor, "r+b") : NULL);
  // if (!*stream) {
  //   if (descriptor > 0) {
  //     close(descriptor);
  //   }
  //   free(tempfile_name);
  //   return NULL;
  // }
  // return tempfile_name;
}

/* ----------------------------- Redecorate after switch ----------------------------- */

/* Update title-bar and such after switching to another buffer. */
void redecorate_after_switch_for(openfilestruct *const file, int cols) {
  ASSERT(file);
  /* Only perform any action when in curses-mode. */
  if (IN_CURSES_CTX) {
    /* If only one file buffer is open, there is nothing to update. */
    if (CLIST_SINGLE(file)) {
      statusline_curses(AHEM, _("No more open file buffers"));
      return;
    }
    /* While in a diffrent buffer, the width of the screen may have changed,
     * so make sure that the starting column for the first row is fitting. */
    ensure_firstcolumn_is_aligned_for(file, cols);
    /* Update the title-bar and multiline-info to match the file. */
    prepare_for_display_for(file);
    /* Ensure thet the main loop will redraw the help lines. */
    currmenu = MMOST;
    /* Prevent a possible shift selection from getting cancelled. */
    shift_held = TRUE;
    /* If the switched-to buffer gave an error during opening, show the message once. */
    if (file->errormessage) {
      statusline_curses(ALERT, "%s", file->errormessage);
      free(file->errormessage);
      file->errormessage = NULL;
    }
    /* Otherwise, indecate on the status-bar witch file we switched to. */
    else {
      mention_name_and_linecount_for(file);
    }
  }
}

/* Update title-bar and such after switching to another buffer. */
void redecorate_after_switch(void) {
  if (IN_CURSES_CTX) {
    redecorate_after_switch_for(TUI_OF, TUI_COLS);
  }
  // /* Only perform any action when in curses mode. */
  // if (IN_CURSES_CTX) {
  //   /* If only one file buffer is open, there is nothing to update. */
  //   if (CLIST_SINGLE(TUI_OF)) {
  //     statusline_curses(AHEM, _("No more open file buffers"));
  //     return;
  //   }
  //   /* While in a different buffer, the width of the screen may have changed,
  //    * so make sure that the starting column for the first row is fitting. */
  //   ensure_firstcolumn_is_aligned();
  //   /* Update title bar and multiline info to match the current buffer. */
  //   prepare_for_display();
  //   /* Ensure that the main loop will redraw the help lines. */
  //   currmenu = MMOST;
  //   /* Prevent a possible Shift selection from getting cancelled. */
  //   shift_held = TRUE;
  //   /* If the switched-to buffer gave an error during opening, show the message
  //    * once; otherwise, indicate on the status bar which file we switched to. */
  //   if (TUI_OF->errormessage) {
  //     statusline_curses(ALERT, "%s", TUI_OF->errormessage);
  //     free(TUI_OF->errormessage);
  //     TUI_OF->errormessage = NULL;
  //   }
  //   else {
  //     mention_name_and_linecount();
  //   }
  // }
}

/* ----------------------------- Switch to prev buffer ----------------------------- */

/* Switch `*file` to the previous entry in the circular list of
 * buffers.  TODO: Implement the redecorating for the editor as well. */
void switch_to_prev_buffer_for(openfilestruct **const open, int cols) {
  ASSERT(open);
  ASSERT(*open);
  DLIST_ADV_PREV(*open);
  if (IN_CURSES_CTX) {
    redecorate_after_switch_for(*open, cols);
  }
}

/* Switch to the previous entry in the circular list of buffers. */
void switch_to_prev_buffer(void) {
  if (IN_GUI_CTX) {
    switch_to_prev_buffer_for(&GUI_OF, GUI_COLS);
  }
  else {
    switch_to_prev_buffer_for(&TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- Switch to next buffer ----------------------------- */

/* Switch `*file` to the next entry in the circular list of buffers.  TODO: Implement the redecorating for the editor as well. */
void switch_to_next_buffer_for(openfilestruct **const open, int cols) {
  ASSERT(open);
  ASSERT(*open);
  DLIST_ADV_NEXT(*open);
  if (IN_CURSES_CTX) {
    redecorate_after_switch_for(*open, cols);
  }
}

/* Switch to the next entry in the circular list of buffers. */
void switch_to_next_buffer(void) {
  if (IN_GUI_CTX) {
    switch_to_next_buffer_for(&GUI_OF, GUI_COLS);
  }
  else {
    switch_to_next_buffer_for(&TUI_OF, TUI_COLS);
  }
  // if (IN_CURSES_CTX) {
  //   openfile = openfile->next;
  //   redecorate_after_switch();
  // }
}

/* ----------------------------- Get next filename ----------------------------- */

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

/* ----------------------------- Open file ----------------------------- */

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
void read_file_into(CTX_ARGS, FILE *const f, int fd, const char *const restrict filename, bool undoable) {
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
    was_leftedge = leftedge_for(cols, xplustabs_for(file), file->current);
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
  if (IN_CURSES_CTX && !isendwin()) {
    if (!isatty(STDIN_FILENO)) {
      reconnect_and_store_state();
    }
    terminal_init();
    doupdate();
  }
  /* If there was a real error during the reading, let the user know. */
  if (ferror(f) && errornum != EINTR && errornum) {
    statusline(ALERT, "%s", strerror(errornum));
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
  SET_PWW(file);
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
    if (less_than_a_screenful_for(STACK_CTX, was_lineno, was_leftedge)) {
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
  CTX_CALL_WARGS(read_file_into, f, fd, filename, undoable);
}

/* ----------------------------- Open buffer ----------------------------- */

/* This does one of three things.  If the filename is "", it just creates a new empty buffer.  When the filename
 * is not empty, it reads that file into a new buffer when requested, otherwise into the existing buffer. */
bool open_buffer_for(openfilestruct **const start, openfilestruct **const open,
  int rows, int cols, const char *const restrict path, bool new_one)
{
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
  if (IN_GUI_CTX) {
    return open_buffer_for(FULL_GUI_CTX, path, new_one);
  }
  else {
    return open_buffer_for(FULL_TUI_CTX, path, new_one);
  }
}

/* ----------------------------- Open buffer browser ----------------------------- */

/* Open a file using the browser. */
void open_buffer_browser_for(FULL_CTX_ARGS) {
  ASSERT(start);
  ASSERT(open);
  ASSERT(*start);
  ASSERT(*open);
  char *file;
  openfilestruct *was_open;
  /* Let the user pick a file using the browser. */
  if (IN_CURSES_CTX && ((file = browse_in(*(*open)->filename ? (*open)->filename : "./")))) {
    /* Save the currently open buffer. */
    was_open = (*open);
    open_buffer_for(FULL_STACK_CTX, file, TRUE);
    free(file);
    /* If the file the user was at when opening the new one was fully empty and without a name, then remove it. */
    if (!*was_open->filename && !was_open->totsize) {
      close_buffer_for(was_open, start, open);
    }
    refresh_needed = TRUE;
  }
}

void open_buffer_browser(void) {
  FULL_CTX_CALL(open_buffer_browser_for);
}

/* ----------------------------- Open new empty buffer ----------------------------- */

/* Open a new empty buffer. */
void open_new_empty_buffer_for(openfilestruct **const start, openfilestruct **const open) {
  make_new_buffer_for(start, open);
  refresh_needed = TRUE;
}

void open_new_empty_buffer(void) {
  if (IN_GUI_CTX) {
    open_new_empty_buffer_for(&GUI_SF, &GUI_OF);
  }
  else {
    open_new_empty_buffer_for(&TUI_SF, &TUI_OF);
  }
}

/* ----------------------------- Username completion ----------------------------- */

/* Try to complete the given fragment of given length to a username. */
char **username_completion(const char *const restrict morsel, Ulong length, Ulong *const num_matches) {
  ASSERT(morsel);
  ASSERT(num_matches);
  Ulong cap  = 4;
  Ulong size = 0;
  char **matches = xmalloc(_PTRSIZE * cap);
  struct passwd *user;
  /* Iterate through the entries in the passwd file, and add each fitting username to the list of matches. */
  while ((user = getpwent())) {
    /* Only add matching names of users that are inside the confinement. */
    if (strncmp(user->pw_name, (morsel + 1), (length - 1)) == 0 && !outside_of_confinement(user->pw_dir, TRUE)) {
      ENSURE_PTR_ARRAY_SIZE(matches, cap, size);
      matches[size++] = fmtstr("~%s", user->pw_name);
    }
  }
  endpwent();
  /* When no matches were found, return a NULL ptr. */
  if (!size) {
    free(matches);
    return NULL;
  }
  /* Otherwise, trim and null-terminate the array. */
  else {
    TRIM_PTR_ARRAY(matches, cap, size);
    matches[size] = NULL;
    *num_matches = size;
    return matches;
  }
}

/* ----------------------------- Input tab ----------------------------- */

/* Do tab completion.  `place` is the position of the status-bar cursor, and
 * `refresh_func` is the function to be called to refresh the edit window. */
char *input_tab(char *morsel, Ulong *const place, functionptrtype refresh_func, bool *const listed) {
  ASSERT(morsel);
  Ulong num_matches = 0;
  char **matches = NULL;
  const char *lastslash;
  Ulong length_of_path;
  Ulong match;
  Ulong common_len = 0;
  Ulong longest_name = 0;
  Ulong nrows;
  Ulong ncols;
  Ulong namelen;
  char *shared;
  char *glued;
  char *disp;
  char c1[MAXCHARLEN];
  char c2[MAXCHARLEN];
  int l1;
  int l2;
  int lastrow;
  int row;
  /* If the cursor is not at the end of the fragment, do nothing. */
  if (morsel[*place]) {
    if (IN_CURSES_CTX) {
      beep();
    }
    return morsel;
  }
  /* If the fragment starts with a tilde(~) and contains no slash, then try completing it as a username. */
  if (*morsel == '~' && !strchr((morsel + 1), '/')) { 
    matches = username_completion(morsel, *place, &num_matches);
  }
  /* If there was no username matches found. try matching against filenames. */
  if (!matches) {
    matches = filename_completion(morsel, &num_matches);
  }
  /* If possible completions were listed before buf none will be listed now... */
  if (refresh_func && listed && num_matches < 2) {
    refresh_func();
    *listed = FALSE;
  }
  /* We found no match, just return. */
  if (!matches) {
    if (IN_CURSES_CTX) {
      beep();
    }
    return morsel;
  }
  lastslash = revstrstr(morsel, "/", (morsel + (*place)));
  length_of_path = (!lastslash ? 0 : (lastslash - morsel + 1));
  /* Determine the number of characters that all matches have in common. */
  while (TRUE) {
    l1 = collect_char((*matches + common_len), c1);
    for (match=1; match<num_matches; ++match) {
      l2 = collect_char((matches[match] + common_len), c2);
      if (l1 != l2 || strncmp(c1, c2, l2) != 0) {
        break;
      }
    }
    if (match < num_matches || !(*matches)[common_len]) {
      break;
    }
    common_len += l1;
  }
  shared = xmalloc(length_of_path + common_len + 1);
  memcpy(shared, morsel, length_of_path);
  memcpy((shared + length_of_path), *matches, common_len);
  common_len += length_of_path;
  shared[common_len] = '\0';
  /* Cover also the case of the user specifying a relative path. */
  glued = fmtstr("%s%s", present_path, shared);
  /* When we have a single match, and it is a directory, set the last char as '/'. */
  if (num_matches == 1 && (is_dir(shared) || is_dir(glued))) {
    shared[common_len++] = '/';
  }
  /* If the matches have something in common, copy that path. */
  if (common_len != *place) {
    morsel = xstrncpy(morsel, shared, common_len);
    *place = common_len;
  }
  else if (num_matches == 1 && IN_CURSES_CTX) {
    beep();
  }
  /* If there is more then one possible completion, show a
   * sorted list.  For now we only support this in curses mode. */
  if (num_matches > 1 && IN_CURSES_CTX) {
    lastrow = (editwinrows - 1 - (ISSET(ZERO) && LINES > 1));
    if (!listed) {
      beep();
    }
    qsort(matches, num_matches, _PTRSIZE, diralphasort);
    /* Find the length of the longest name among the matches. */
    for (match=0; match<num_matches; ++match) {
      namelen = breadth(matches[match]);
      if (namelen > longest_name) {
        longest_name = namelen;
      }
    }
    /* Only allow a length of the total terminal columns. */
    CLAMP_MAX(longest_name, (COLS - 1));
    /* The columns of names will be seperated by two spaces, but the last column will have just one space after it. */
    ncols = ((COLS + 1) / (longest_name + 2));
    nrows = ((num_matches + ncols - 1) / ncols);
    row   = (LT(ncols, lastrow) ? (lastrow - nrows) : 0);
    /* Blank the edit window and hide the cursor. */
    blank_edit();
    curs_set(FALSE);
    /* Now print the list of matches out there. */
    for (match=0; match<num_matches; ++match) {
      wmove(midwin, row, ((longest_name + 2) * (match % ncols)));
      if (row == lastrow && ((match + 1) % ncols) == 0 && (match + 1) < num_matches) {
        waddstr(midwin, _("(more)"));
        break;
      }
      disp = display_string(matches[match], 0, longest_name, FALSE, FALSE);
      waddstr(midwin, disp);
      free(disp);
      if (((match + 1) % ncols) == 0) {
        ++row;
      }
    }
    wnoutrefresh(midwin);
    *listed = TRUE;
  }
  free_chararray(matches, num_matches);
  free(glued);
  free(shared);
  return morsel;
}

/* ----------------------------- Write file ----------------------------- */

/* Write `file` to disk.  If `thefile` isn't `NULL`, we write to a temporary file that is already open.  If `normal` is `FALSE`
 * (for a spellcheck or an emergency save, for example), we don't make a backup and don't give feedback.  If `method` is `APPEND`
 * or `PREPEND`, it means we will be appending or prepending instead of owerwriting the given file.  If `annotate` is `TRUE` and
 * when writing a `normal` file, we set the current filename and stat info.  Returns `TRUE` on success, and `FALSE` otherwise. */
bool write_file_for(openfilestruct *const file, const char *const restrict name,
  FILE *thefile, bool normal, kind_of_writing_type method, bool annotate)
{
  ASSERT(file);
  ASSERT(name);
  /* Becomes TRUE when the file is non-temporary and exists. */
  bool is_existing_file;
  /* The status fields filled in by stating the file. */
  struct stat info;
  /* The filename after tilde expansion. */
  char *realname = real_dir_from_tilde(name);
  /* The descriptor that gets assigned when opening the file. */
  int descriptor = 0;
  /* The name of the temprorary file we use when prepending. */
  char *tempname = NULL;
  /* An iterator for moving through the lines of the buffer. */
  linestruct *line = file->filetop;
  /* The number of lines written, for feedback on the status-bar. */
  Ulong lineswritten = 0;
  Ulong data_len;
  Ulong wrote;
  FILE *source;
  FILE *target;
  int verdict;
  mode_t permissions;
  const char *oldname;
  const char *newname;
  /* The flag we will use to open the file.  Use O_EXCL for an emergency file. */
  int open_flag = (O_WRONLY | O_CREAT | ((method == APPEND) ? O_APPEND : (normal ? O_TRUNC : O_EXCL)));
  /* If we're writing a temporary file, we're probebly going outside
   * the operating directory, so skip the operating directory test. */
  if (normal && outside_of_confinement(realname, FALSE)) {
    statusline(ALERT, _("Can't write outside of %s"), operating_dir);
    goto cleanup_and_exit;
  }
  /* Check whether the file (at the end of a symnlink) exists. */
  is_existing_file = (normal && (stat(realname, &info) != -1));
  /* If we haven't stated this file before (say, the user just specified it interactivly), stat and save
   * the value now, or else we will chase NULL-pointers when we do modtime checks and such during backup. */
  if (!file->statinfo && is_existing_file) {
    stat_with_alloc(realname, &file->statinfo);
  }
  /* When the user requested a backup, we do this only if the file exists and isn't temporary and the file has
   * not been modified by someone else since we opened it (or we are appending/prepending or writing a selection). */
  if (ISSET(MAKE_BACKUP) && is_existing_file && !S_ISFIFO(info.st_mode) && file->statinfo
  && (file->statinfo->st_mtime == info.st_mtime || method != OVERWRITE || file->mark)) {
    if (!make_backup_of_for(file, realname)) {
      goto cleanup_and_exit;
    }
  }
  /* When prepending, first copy the existing file to a temporary file. */
  if (method == PREPEND) {
    if (is_existing_file && S_ISFIFO(info.st_mode)) {
      statusline(ALERT, _("Error writing %s: FIFO"), realname);
      goto cleanup_and_exit;
    }
    source = fopen(realname, "rb");
    if (!source) {
      statusline(ALERT, _("Error reading %s: %s"), realname, strerror(errno));
      goto cleanup_and_exit;
    }
    tempname = safe_tempfile_for(file, &target);
    if (!tempname) {
      statusline(ALERT, _("Error writing temp file: %s"), strerror(errno));
      fclose(source);
      goto cleanup_and_exit;
    }
    verdict = copy_file(source, target, TRUE);
    /* We failed to read. */
    if (verdict < 0) {
      statusline(ALERT, _("Error reading %s: %s"), realname, strerror(errno));
      unlink(tempname);
      goto cleanup_and_exit;
    }
    /* We failed to write. */
    else if (verdict > 0) {
      statusline(ALERT, _("Error writing temp file: %s"), strerror(errno));
      unlink(tempname);
      goto cleanup_and_exit;
    }
  }
  /* If the file already exists and is a FIFO, then inform the user of this. */
  if (is_existing_file && S_ISFIFO(info.st_mode)) {
    statusbar_all(_("Writing to FIFO..."));
  }
  /* When it's not a temporary file, this is where we open or create
   * it.  For an emergency file, access is restricted to just the owner. */
  if (!thefile) {
    permissions = (normal ? RW_FOR_ALL : (S_IRUSR | S_IWUSR));
    block_sigwinch(TRUE);
    /* Open the file-desctiptor. */
    if (normal) {
      CTRL_C_HANDLER_ACTION(
        descriptor = open(realname, open_flag, permissions);
      );
    }
    else {
      descriptor = open(realname, open_flag, permissions);
    }
    block_sigwinch(FALSE);
    /* If we couldn't open the file, just give up. */
    if (descriptor < 0) {
      /* We were interupted. */
      if (errno == EINTR || !errno) {
        statusline(ALERT, _("Interrupted"));
      }
      /* Some other problem caused the issue. */
      else {
        statusline(ALERT, _("Error writing %s: %s"), realname, strerror(errno));
      }
      /* If we made a temp-file, delete it. */
      if (tempname) {
        unlink(tempname);
      }
      goto cleanup_and_exit;
    }
    /* Try to open the file. */
    thefile = fdopen(descriptor, ((method == APPEND) ? "ab" : "wb"));
    /* We failed. */
    if (!thefile) {
      statusline(ALERT, _("Error writing %s: %s"), realname, strerror(errno));
      close(descriptor);
      goto cleanup_and_exit;
    }
  }
  /* When normaly writing the file, and the minibar is not active, tell the user we are writing. */
  if (normal && !ISSET(MINIBAR)) {
    statusbar_all(_("Writing"));
  }
  /* Now we do the writing. */
  while (TRUE) {
    /* Decode LF's as the NUL's that they are, before writing to disk. */
    data_len = recode_LF_to_NUL(line->data);
    /* Write the data to disk.  TODO: Either remake this into a fully fd based
     * loop, or make another function that performs this task, that is fd based. */
    wrote = fwrite(line->data, 1, data_len, thefile);
    /* Re-encode any embedded NUL's as LF's. */
    recode_NUL_to_LF(line->data, data_len);
    /* If there was any missmatch when writing, something is very wrong... */
    if (wrote < data_len) {
      statusline(ALERT, _("Error writing %s: %s"), realname, strerror(errno));
      fclose(thefile);
      goto cleanup_and_exit;
    }
    /* If we've reached the last line of the buffer, don't write a new-line character after it.  If this last
     * line is empty, it means zero bytes are written for it, and we don't count it in the number of lines. */
    if (!line->next) {
      if (*line->data) {
        ++lineswritten;
      }
      break;
    }
    if (file->fmt == DOS_FILE || file->fmt == MAC_FILE) {
      if (putc('\r', thefile) == EOF) {
        statusline(ALERT, _("Error writing %s: %s"), realname, strerror(errno));
        fclose(thefile);
        goto cleanup_and_exit;
      }
    }
    if (file->fmt != MAC_FILE) {
      if (putc('\n', thefile) == EOF) {
        statusline(ALERT, _("Error writing %s: %s"), realname, strerror(errno));
        fclose(thefile);
        goto cleanup_and_exit;
      }
    }
    DLIST_ADV_NEXT(line);
    ++lineswritten;
  }
  /* When prepending, append the temporary file to what we wrote above. */
  if (method == PREPEND) {
    source = fopen(tempname, "rb");
    if (!source) {
      statusline(ALERT, _("Error reading temp file: %s"), strerror(errno));
      fclose(thefile);
      goto cleanup_and_exit;
    }
    verdict = copy_file(source, thefile, FALSE);
    /* We failed to read. */
    if (verdict < 0) {
      statusline(ALERT, _("Error reading temp file: %s"), strerror(errno));
      fclose(thefile);
      goto cleanup_and_exit;
    }
    /* We failed to write. */
    else if (verdict > 0) {
      statusline(ALERT, _("Error writing %s: %s"), realname, strerror(errno));
      fclose(thefile);
      goto cleanup_and_exit;
    }
    unlink(tempname);
  }
  if (!is_existing_file || !S_ISFIFO(info.st_mode)) {
    /* Ensure the data has reached the disk before reporting it as written. */
    if (fflush(thefile) != 0 || fsync(fileno(thefile)) != 0) {
      statusline(ALERT, _("Error writing %s to disk: %s"), realname, strerror(errno));
      fclose(thefile);
      goto cleanup_and_exit;
    }
  }
  /* Change permissions and owner of an emergency save file to the values
   * of the original, but ignore any failures as we are in a hurry */
  if (method == EMERGENCY && descriptor && file->statinfo) {
    IGNORE_CALL_RESULT(fchmod(descriptor, file->statinfo->st_mode));
    IGNORE_CALL_RESULT(fchown(descriptor, file->statinfo->st_uid, file->statinfo->st_gid));
  }
  if (fclose(thefile) != 0) {
    statusline(ALERT, _("Error writing %s: %s"), realname, strerror(errno));
    cleanup_and_exit: {
      if (errno == ENOSPC && normal) {
        if (IN_CURSES_CTX) {
          napms(3200);
          lastmessage = VACUUM;
          /* TRANSLATORS: This warns for data loss when the disk is full. */
          statusline(ALERT, _("File on disk has been truncated!"));
          napms(3200);
          lastmessage = VACUUM;
          /* TRANSLATORS: This is a suggestion to the user, where "resume" means
           * resuming from suspension.  Try to keep this at most 76 characters. */
          statusline(ALERT, _("Maybe ^T^Z, make room on disk, then ^S^X"));
          stat_with_alloc(realname, &file->statinfo);
        }
        else {
          statusline(ALERT, _("Not enough space on disk to save!"));
        }
      }
      free(tempname);
      free(realname);
      return FALSE;
    }
  }
  /* When having written an entire buffer, update some administrativa...? */
  if (annotate && method == OVERWRITE) {
    /* The filename has changed. */
    if (strcmp(file->filename, realname) != 0) {
      /* Write a new lockfile, if needed. */
      if (file->lock_filename) {
        delete_lockfile(file->lock_filename);
        free(file->lock_filename);
        file->lock_filename = NULL;
      }
      if (ISSET(LOCKING)) {
        file->lock_filename = do_lockfile(realname, FALSE);
      }
      file->filename = xstrcpy(file->filename, realname);
      oldname = (file->syntax ? file->syntax->name : "");
      find_and_prime_applicable_syntax_for(file);
      newname = (file->syntax ? file->syntax->name : "");
      /* If the syntax changed, discard and recompute the multidata.  TODO: Here we should either rethink
       * how we should format our own line syntax data format or use the multidata in a diffrent way. */
      if (strcmp(oldname, newname) != 0) {
        DLIST_ND_FOR_NEXT(file->filetop, line) {
          free(line->multidata);
          line->multidata = NULL;
        }
        precalc_multicolorinfo_for(file);
        have_palette   = FALSE;
        refresh_needed = TRUE;
      }
    }
    /* Get or update the stat info to reflect the current state. */
    stat_with_alloc(realname, &file->statinfo);
    /* Record at which point in the undo stack the buffer was saved. */
    file->last_saved  = file->current_undo;
    file->last_action = OTHER;
    file->modified    = FALSE;
    if (IN_CURSES_CTX) {
      titlebar(NULL);
    }
  }
  if (ISSET(MINIBAR) && !ISSET(ZERO) && LINES > 1 && annotate) {
    report_size = TRUE;
  }
  else {
    if (normal) {
      statusline(REMARK, P_("Wrote %lu line", "Wrote %lu lines", lineswritten), lineswritten);
    }
    free(tempname);
    free(realname);
  }
  return TRUE;
}

/* Write the currently open buffer to disk.  If `thefile` isn't `NULL`, we write to a temporary file that is already
 * open.  If `normal` is `FALSE` (for a spellcheck or an emergency save, for example), we don't make a backup and
 * don't give feedback.  If `method` is `APPEND` or `PREPEND`, it means we will be appending or prepending instead
 * of owerwriting the given file.  If `annotate` is `TRUE` and when writing a `normal` file, we set the current
 * filename and stat info.  Returns `TRUE` on success, and `FALSE` otherwise.  Note that this is `context-safe`. */
bool write_file(const char *const restrict name, FILE *thefile, bool normal, kind_of_writing_type method, bool annotate) {
  return write_file_for(CTX_OF, name, thefile, normal, method, annotate);
}

/* ----------------------------- Write region to file ----------------------------- */

/* Write the marked region of `file` out to disk.  Returns `TRUE` on success and `FALSE` on error. */
bool write_region_to_file_for(openfilestruct *const file, const char *const restrict name, FILE *stream, bool normal, kind_of_writing_type method) {
  ASSERT(file);
  linestruct *birthline;
  linestruct *top;
  linestruct *bot;
  linestruct *stopper;
  linestruct *after;
  Ulong top_x;
  Ulong bot_x;
  char *was_data;
  char saved_byte;
  bool ret;
  get_region_for(file, &top, &top_x, &bot, &bot_x);
  /* When needed, prepare a magic end line for the region. */
  if (normal && (bot_x > 0) && !ISSET(NO_NEWLINES)) {
    stopper = make_new_node(bot);
    stopper->data = COPY_OF("");
  }
  else {
    stopper = NULL;
  }
  /* Make the marked area look like a seperate buffer. */
  after            = bot->next;
  bot->next        = stopper;
  saved_byte       = bot->data[bot_x];
  bot->data[bot_x] = '\0';
  was_data         = top->data;
  top->data       += top_x;
  birthline        = file->filetop;
  file->filetop    = top;
  ret              = write_file_for(file, name, stream, normal, method, NONOTES);
  /* Restore the proper state of the buffer. */
  file->filetop    = birthline;
  top->data        = was_data;
  bot->data[bot_x] = saved_byte;
  bot->next        = after;
  if (stopper) {
    delete_node_for(file, stopper);
  }
  return ret;
}

/* Write the marked region of the currently open buffer out to disk.  Returns `TRUE` on success and `FALSE` on error. */
bool write_region_to_file(const char *const restrict name, FILE *stream, bool normal, kind_of_writing_type method) {
  return write_region_to_file_for(CTX_OF, name, stream, normal, method);
}

/* ----------------------------- Write it out ----------------------------- */

/* Write `file` (or marked region in `file`) to disk.  `exiting` Indecates whether the program is
 * exiting.  If `exiting` is `TRUE`, write the entire buffer regardless of whether the mark is on.
 * Do not ask for a name when `withprompt` is `FALSE`.  Nor when doing a save-on-exit and the buffer
 * already has a name. If `withprompt` is `TRUE`, then ask for (confirmation of) the filename.  Returns
 * `0` if the operation is cancelled, `1` if the buffer is saved and `2` if the buffer is discarded. */
int write_it_out_for(openfilestruct *const file, bool exiting, bool withprompt) {
  ASSERT(file);
  /* The filename we offer, or what the user typed so far. */
  char *given;
  /* Whether it's okey to save the buffer under a diffrent name. */
  bool maychange = !*file->filename;
  /* How we will write the file, this defaults to `OVERWRITE`. */
  kind_of_writing_type method = OVERWRITE;
  /* TODO: Make this either a global or some other way to check for singleness. */
  static bool did_credits = FALSE;
  functionptrtype function;
  int response;
  int choice;
  struct stat info;
  const char *msg;
  const char *formatstr;
  const char *backupstr;
  char *chosen;
  char *full_answer;
  char *full_filename;
  bool name_exists;
  bool do_warning;
  char *question;
  char *name;
  char *message;
  /* Whether to display newlines in filenames as ^J or not. */
  as_an_at = FALSE;
  /* When in curses-mode.  TODO: As for the gui-mode, implement the same functionality. */
  if (IN_CURSES_CTX) {
    given = copy_of((file->mark && !exiting) ? "" : file->filename);
    /* Do the loop... */
    while (TRUE) {
      response  = 0;
      choice    = NO;
      formatstr = ((file->fmt == DOS_FILE) ? _(" [Dos Format]") : (file->fmt == MAC_FILE) ? _(" [Mac Format]") : "");
      backupstr = (ISSET(MAKE_BACKUP) ? _(" [Backup]") : "");
      /* When the mark is on, offer to write the selection to disk, but not when in restricted
      * mode, because it would allow writing to a file not specified on the command line.  What..? */
      if (file->mark && !exiting && !ISSET(RESTRICTED)) {
        /* TRANSLATORS: The next six strings are prompts. */
        msg = ((method == PREPEND) ? _("Prepend Selection to File")
            :  (method == APPEND)  ? _("Append Selection to File")
                                  : _("Write Selection to File"));
      }
      else if (method != OVERWRITE) {
        msg = ((method == PREPEND) ? _("File Name to Prepend to") : _("File Name to Append to"));
      }
      else {
        msg = _("File Name to Write");
      }
      present_path = xstrcpy(present_path, "./");
      /* When we shouldn't prompt, use the existing filename. */
      if ((!withprompt || (ISSET(SAVE_ON_EXIT) && exiting)) && *file->filename) {
        answer = xstrcpy(answer, file->filename);
      }
      /* Otherwise, ask for (confirmation of) the filename. */
      else {
        response = do_prompt(MWRITEFILE, given, NULL, edit_refresh, "%s%s%s", msg, formatstr, backupstr);
      }
      /* If the user inputs a blank string or cancelles, inform the user the write operation was cancelled and return 0. */
      if (response < 0) {
        statusbar_all(_("Cancelled"));
        free(given);
        return 0;
      }
      function = func_from_key(response);
      /* Upon request, abandon the buffer. */
      if (function == discard_buffer) {
        free(given);
        return 2;
      }
      given = xstrcpy(given, answer);
      /* To files */
      if (function == to_files && !ISSET(RESTRICTED)) {
        chosen = browse_in(answer);
        if (!chosen) {
          continue;
        }
        answer = free_and_assign(answer, chosen);
      }
      /* Dos format */
      else if (function == dos_format) {
        file->fmt = ((file->fmt == DOS_FILE) ? NIX_FILE : DOS_FILE);
        continue;
      }
      /* Mac format */
      else if (function == mac_format) {
        file->fmt = ((file->fmt == MAC_FILE) ? NIX_FILE : MAC_FILE);
        continue;
      }
      /* Back it up */
      else if (function == back_it_up && !ISSET(RESTRICTED)) {
        TOGGLE(MAKE_BACKUP);
        continue;
      }
      /* Prepend it */
      else if (function == prepend_it && !ISSET(RESTRICTED)) {
        method = ((method == PREPEND) ? OVERWRITE : PREPEND);
        if (strcmp(answer, file->filename) == 0) {
          *given = '\0';
        }
        continue;
      }
      /* Append it */
      else if (function == append_it && !ISSET(RESTRICTED)) {
        method = ((method == APPEND) ? OVERWRITE : APPEND);
        if (strcmp(answer, file->filename) == 0) {
          *given = '\0';
        }
        continue;
      }
      /* Do help */
      else if (function == do_help) {
        continue;
      }
      else {
        /* If the user pressed Ctrl-X in the edit window, and answered `Y` at the "Save modified buffer?"
         * prompt, and entered "zzy" as filename, and this is the first time around, show an ester egg. */
        if (exiting && !ISSET(SAVE_ON_EXIT) && !*file->filename && strcmp(answer, "zzy") == 0 && !did_credits) {
          if (LINES > 5 && COLS > 31) {
            do_credits();
            did_credits = TRUE;
          }
          else {
            /* TRANSLATORS: Concisely say the screen is to small. */
            statusline(AHEM, _("Too tiny"));
          }
          free(given);
          return 0;
        }
        if (method == OVERWRITE) {
          full_answer   = get_full_path(answer);
          full_filename = get_full_path(file->filename);
          name_exists   = (stat(PASS_IF_VALID(full_answer, answer), &info) != -1);
          if (!*file->filename) {
            do_warning = name_exists;
          }
          else {
            do_warning = (strcmp(PASS_IF_VALID(full_answer, answer), PASS_IF_VALID(full_filename, file->filename)) != 0);
          }
          free(full_filename);
          free(full_answer);
          if (do_warning) {
            /* When in restricted mode, we aren't allowed to overwrite an existing file with the
             * current buffer, nor to change the name of the current buffer if it alreade has one. */
            if (ISSET(RESTRICTED)) {
              /* TRANSLATORS: Restricted mode forbids overwriting. */
              warn_and_briefly_pause_curses(_("File exists -- cannot overwrite"));
              continue;
            }
            if (!maychange && (exiting || !file->mark)) {
              if (ask_user(YESORNO, _("Save file under DIFFRENT NAME? ")) != YES) {
                continue;
              }
              maychange = TRUE;
            }
            if (name_exists) {
              /* Construct the question. */
              question = _("File \"%s\" exists; OVERWRITE? ");
              name     = crop_to_fit(answer, (COLS - breadth(question) + 1));
              message  = free_and_assign(name, fmtstr(question, name));
              /* Ask the user. */
              choice = ask_user(YESORNO, message);
              free(message);
              /* If the user did not say yes, just continue. */
              if (choice != YES) {
                continue;
              }
            }
          }
          /* Complain if the file exists, the name hasn't changed, and
           * the stat info we had before does not match what we have now. */
          else if (name_exists && file->statinfo
          && (file->statinfo->st_mtime < info.st_mtime
          || file->statinfo->st_dev != info.st_dev
          || file->statinfo->st_ino != info.st_ino)) {
            warn_and_briefly_pause_curses(_("File on disk has changed"));
            /* TRANSLATORS: Try to keep this at most 76 characters. */
            choice = ask_user(YESORNO, _("File was modified since you opened it; continue saving? "));
            wipe_statusbar();
            /* When in tool mode and not called by `savefile`, overwrite the file right here when requested. */
            if (ISSET(SAVE_ON_EXIT) && withprompt) {
              free(given);
              switch (choice) {
                /* Write the file. */
                case YES: {
                  return write_file_for(file, file->filename, NULL, NORMAL, OVERWRITE, NONOTES);
                }
                /* Discard the buffer. */
                case NO: {
                  return 2;
                }
                /* Otherwise, just return 0. */
                default: {
                  return 0;
                }
              }
            }
            else if (choice == CANCEL && exiting) {
              continue;
            }
            else if (choice != YES) {
              free(given);
              return 1;
            }
          }   
        }
        break;
      }
    }
    free(given);
  }
  /* For now, when in gui mode, we set answer to the filename of file. */
  else if (IN_GUI_CTX) {
    answer = xstrcpy(answer, file->filename);
  }
  /* When the mark is on (and we've prompted for a name and we're not exiting
   * and we're not in restricted mode), then write out the marked region. */
  if (file->mark && withprompt && !exiting && !ISSET(RESTRICTED)) {
    return write_region_to_file_for(file, answer, NULL, NORMAL, method);
  }
  /* Otherwise, write out the whole buffer. */
  else {
    return write_file_for(file, answer, NULL, NORMAL, method, ANNOTATE);
  }
}

/* Write the `currently open buffer` (or marked region in the `currently open buffer`) to disk.  `exiting`
 * Indecates whether the program is exiting.  If `exiting` is `TRUE`, write the entire buffer regardless
 * of whether the mark is on. Do not ask for a name when `withprompt` is `FALSE`.  Nor when doing a save-on-exit
 * and the buffer already has a name. If `withprompt` is `TRUE`, then ask for (confirmation of) the filename.
 * Returns `0` if the operation is cancelled, `1` if the buffer is saved and `2` if the buffer is discarded. */
int write_it_out(bool exiting, bool withprompt) {
  return write_it_out_for(CTX_OF, exiting, withprompt);
}

/* ----------------------------- Do writeout ----------------------------- */

/* Write `*open` to disk, or discard it. */
void do_writeout_for(openfilestruct **const start, openfilestruct **const open, int cols) {
  ASSERT(start);
  ASSERT(open);
  ASSERT(*start);
  ASSERT(*open);
  /* If the user chose to discard the buffer, close it. */
  if (write_it_out_for(*open, FALSE, TRUE) == 2) {
    close_and_go_for(start, open, cols);
  }
}

/* Write the `currently open buffer` to disk, or discard it. */
void do_writeout(void) {
  if (IN_GUI_CTX) {
    do_writeout_for(&GUI_SF, &GUI_OF, GUI_COLS);
  }
  else {
    do_writeout_for(&TUI_SF, &TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- Do savefile ----------------------------- */

/* If it has a name, write `*open` to disk without prompting. */
void do_savefile_for(openfilestruct **const start, openfilestruct **const open, int cols) {
  ASSERT(start);
  ASSERT(open);
  ASSERT(*start);
  ASSERT(*open);
  if (write_it_out_for(*open, FALSE, FALSE) == 2) {
    close_and_go_for(start, open, cols);
  }
}

/* If it has a name, write the `currently open buffer` to disk without prompting.  Note that this is `context-safe`. */
void do_savefile(void) {
  if (IN_GUI_CTX) {
    do_savefile_for(&GUI_SF, &GUI_OF, GUI_COLS);
  }
  else {
    do_savefile_for(&TUI_SF, &TUI_OF, TUI_COLS);
  }
}

/* ----------------------------- Do insertfile ----------------------------- */

/* If the current mode of operation allows it, go insert a file. */
void do_insertfile_for(FULL_CTX_ARGS) {
  if (!in_restricted_mode()) {
    insert_a_file_or_for(FULL_STACK_CTX, FALSE);
  }
}

/* If the current mode of operation allows it, go insert a file.  Note that this is `context-safe`. */
void do_insertfile(void) {
  FULL_CTX_CALL(do_insertfile_for);
}

/* ----------------------------- Do execute ----------------------------- */

/* If the current mode of operation allows it, go prompt for a command. */
void do_execute_for(FULL_CTX_ARGS) {
  if (!in_restricted_mode()) {
    insert_a_file_or_for(FULL_STACK_CTX, TRUE);
  }
}

/* If the current mode of operation allows it, go prompt for a command.  Note that this is `context-safe`. */
void do_execute(void) {
  FULL_CTX_CALL(do_execute_for);
}

/* ----------------------------- Norm path ----------------------------- */

/* TODO: Invest mush more time into this and ensure it always correctly normalizes the path. */
char *norm_path(const char *const restrict path) {
  ASSERT(path);
  char *ret = copy_of(path);
  const char *last_slash = ((*ret == '/') ? ret : NULL);
  const char *ptr = ret;
  while (*ptr) {
    /* Double slashes. */
    if (*ptr == '/' && ptr[1] == '/') {
      xstr_erase_norealloc(ret, (ptr - ret), 1);
    }
    /* Unessesary current dir indecator. */
    else if (*ptr == '/' && ptr[1] == '.' && ptr[2] == '/') {
      xstr_erase_norealloc(ret, (ptr - ret), 2);
    }
    /* Trailing '.' character. */
    else if (*ptr == '/' && ptr[1] == '.' && ptr[2] == NUL) {
      xstr_erase_norealloc(ret, (ptr - ret + 1), 1);
      break;
    }
    /* Parent-directory marker. */
    else if (*ptr == '/' && ptr[1] == '.' && ptr[2] == '.' && (ptr[3] == '/' || ptr[3] == NUL)) {
      /* When gooing up from root, we can never have a valid path, so return NULL. */
      if (ptr == ret) {
        free(ret);
        return NULL;
      }
      else if (last_slash) {
        xstr_erase_norealloc(ret, (last_slash - ret), ((ptr - ret) - (last_slash - ret) + 3));
      }
      else {
        xstr_erase_norealloc(ret, 0, ((ptr - ret) + 3));
      }
      /* Start over. */
      ptr = ret;
      last_slash = ((*ret == '/') ? ret : NULL);
    }
    /* Otherwise */
    else {
      /* Indecate the last seen '/' char. */
      if (*ptr == '/') {
        last_slash = ptr;
      }
      ptr += char_length(ptr);
    }
  }
  return ret;
}

void norm_path_test_a_path(const char *const restrict path) {
  char *np = norm_path(path);
  log_INFO_1("%s -> %s", path, PASS_IF_VALID(np, "NULL"));
  free(np);
}

void norm_path_test(void) {
  norm_path_test_a_path("//foo//bar");
  norm_path_test_a_path("/foo/./bar/");
  norm_path_test_a_path("/foo/bar/.");
  norm_path_test_a_path("/foo/bar/../baz");
  norm_path_test_a_path("/foo/../..");
  norm_path_test_a_path("/foo/../");
  norm_path_test_a_path("foo/bar/../baz");
  norm_path_test_a_path("./foo/./bar");
  norm_path_test_a_path("../foo/bar");
  norm_path_test_a_path("/../foo");
  norm_path_test_a_path("/foo/bar///");
  norm_path_test_a_path("foo/bar///");
  norm_path_test_a_path("/foo///bar///baz//");
  norm_path_test_a_path("///");
  norm_path_test_a_path("/");
  norm_path_test_a_path("////foo");
  norm_path_test_a_path("//foo//");
  norm_path_test_a_path("/a/b/c/../../../");
  norm_path_test_a_path("/a/b/../../../../");
  norm_path_test_a_path("a/b/c/../../../");
  norm_path_test_a_path("a/b/c/../../../../");
  norm_path_test_a_path("/foo/./bar/../baz/./");
  norm_path_test_a_path("foo/./bar/../baz/./");
  norm_path_test_a_path("/foo/../bar/./../baz");
  norm_path_test_a_path("/a/./././b/.././c/");
  norm_path_test_a_path("/././.");
  norm_path_test_a_path("././foo/./bar/../baz/");
  norm_path_test_a_path("./../foo/");
  norm_path_test_a_path("/usr/bin");
  norm_path_test_a_path("foo/bar");
  norm_path_test_a_path("/foo/bar");
  norm_path_test_a_path(".");
  norm_path_test_a_path("foo/../../../bar");
  exit(0);
}

/* ----------------------------- Abs path ----------------------------- */

char *abs_path(const char *const restrict path) {
  return ((*path == '/') ? norm_path(path) : get_full_path(path));
}
