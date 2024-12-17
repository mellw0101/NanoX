#include "prototypes.h"

#define CONFIGFILE_EXT ".nxcfg"
#define CONFIGDIR      ".config/nanox/"
#define COLORFILE_NAME "color" CONFIGFILE_EXT

static char *configdir = NULL;
static colorfilestruct *colorfile = NULL;

#define TERMINATE TRUE
#define DO_NOT_TERMINATE FALSE

/* Open a fd for file_path. */
int open_fd(const char *file_path, int flags, bool on_failure, mode_t permissions)  {
  block_sigwinch(TRUE);
  int fd;
  if (!permissions) {
    fd = open(file_path, flags);
  }
  else {
    fd = open(file_path, flags, permissions);
  }
  block_sigwinch(FALSE);
  if (fd < 0) {
    /* Operation interupted. */
    if (errno == EINTR || !errno) {
      logI("Opening fd: '%s' was interupted.", file_path);
      if (on_failure) {
        exit(EINTR);
      }
    }
    else {
      int status = errno;
      logE("Error opening fd: '%s': %s", file_path, strerror(errno));
      if (on_failure) {
        exit(status);
      }
    }
  }
  return fd;
}

/* Lock a file.  Return`s FALSE on failure. */
bool lock_file(int fd, short type, long start, long len, flock *lock) {
  lock->l_type   = type;     /* Lock type, i.e: F_WRLCK, F_RDLCK. */
  lock->l_whence = SEEK_SET; /* Where start relates to. */
  lock->l_start  = start;    /* Start offset. */
  lock->l_len    = len;      /* Length of offset.  Zero means whole file. */
  if (fcntl(fd, F_SETLKW, lock) == -1) {
    logE("Failed to lock fd: %s", strerror(errno));
    return FALSE;
  }
  return TRUE;
}

/* Unlock a file using lock struct that was used to lock. */
bool unlock_file(int fd, flock *lock) {
  lock->l_type = F_UNLCK;
  if (fcntl(fd, F_SETLK, lock) == -1) {
    logE("Failed to unlock fd: %s", strerror(errno));
    return FALSE;
  }
  return TRUE;
  nmalloc(2);
}

/* Lock a file, then write data to it. */
void lock_and_write(const char *file, const void *data, Ulong len, kind_of_writing_type method) {
  int fd =  open_fd(file, (O_WRONLY | O_CREAT | (method == APPEND ? O_APPEND : O_TRUNC)), TERMINATE, 0666);
  flock lock;
  if (!lock_file(fd, F_WRLCK, 0,  0, &lock)) {
    close(fd);
    exit(errno);
  }
  /* If there is any data then write it to the fd. */
  if (data && write(fd, data, len) == -1) {
    logE("Failed to write to fd: '%s': %s", file, strerror(errno));
  }
  unlock_file(fd, &lock);
  close(fd);
}


/* Lock a file, then read it. */
char *lock_and_read(const char *file_path, Ulong *file_size) {
  int fd = open_fd(file_path, O_RDONLY, TERMINATE, 0);
  flock lock;
  if (!lock_file(fd, F_RDLCK, 0, 0, &lock)) {
    close(fd);
    exit(errno);
  }
  Ulong ret_len = 120;
  char *ret = (char *)malloc(ret_len);
  long byread = 0;
  long len = 0;
  char buffer[4096];
  while ((len = read(fd, buffer, sizeof(buffer))) > 0) {
    if (len == -1) {
      logE("read failed.");
      close(fd);
      free(ret);
      exit(errno);
    }
    if ((byread + len) >= ret_len) {
      ret_len = ((byread + len) * 2);
      ret = arealloc(ret, ret_len);
    }
    memcpy((ret + byread), buffer, len);
    byread += len;
  }
  ret = arealloc(ret, (byread + 1));
  ret[byread] = '\0';
  unlock_file(fd, &lock);
  close(fd);
  *file_size = byread;
  return ret;
}

/* Callback that the main thread runs to update colors in a thread-safe manner. */
static void update_colorfile(void *arg) {
  colorfilestruct *file = (colorfilestruct *)arg;
  line_number_color = file->linenumber;
}

/* Load colorfile with values from disk. */
void load_colorfile(void) {
  if (!is_file_and_exists(colorfile->filepath)) {
    lock_and_write(colorfile->filepath, "linenumber_color=\n", strlen("linenumber_color=\n"), OVERWRITE);
  }
  Ulong file_size;
  char *data = lock_and_read(colorfile->filepath, &file_size);
  const char *line_color = strstr(data, "linenumber_color=");
  if (line_color) {
    /* Define a callback for the main thread to set the line color. */
    line_color += strlen("linenumber_color=");
    const char *end = line_color;
    ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '\n'));
    /* There is no string after '='. */
    if (line_color == end) {
      colorfile->linenumber = LINE_NUMBER;
    }
    else {
      char *color = measured_copy(line_color, (end - line_color));
      if (strcmp(color, "red") == 0) {
        colorfile->linenumber = FG_VS_CODE_RED;
      }
      else if (strcmp(color, "green") == 0) {
        colorfile->linenumber = FG_VS_CODE_GREEN;
      }
      else if (strcmp(color, "blue") == 0) {
        colorfile->linenumber = FG_VS_CODE_BLUE;
      }
      else if (strcmp(color, "bg-red") == 0) {
        colorfile->linenumber = BG_VS_CODE_RED;
      }
      else {
        colorfile->linenumber = LINE_NUMBER;
      }
      free(color);
    }
  }
  enqueue_callback(update_colorfile, colorfile);
}

/* Init NanoX config. */
void init_cfg(void) {
  get_homedir();
  if (!homedir) {
    return;
  }
  configdir = concatenate_path(homedir, CONFIGDIR);
  if (!is_dir(configdir)) {
    mkdir(configdir, 0755);
  }
  colorfile = (colorfilestruct *)nmalloc(sizeof(*colorfile));
  colorfile->filepath = concatenate_path(configdir, COLORFILE_NAME);
  load_colorfile();
  file_listener_t *colorfile_listener = file_listener.add_listener(colorfile->filepath);
  colorfile_listener->set_event_callback(IN_CLOSE_WRITE, NULL, FL_ACTION(
    load_colorfile();
  ));
  colorfile_listener->start_listening();
}

void cleanup_cfg(void) {
  free(configdir);
  free(colorfile->filepath);
  free(colorfile);
}
