#include "../include/prototypes.h"

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
  line_number_color  = file->linenumber;
  mini_infobar_color = file->minibar;
}

#define LINENUMBER_OPTION "linenumber_color="
#define MINIBAR_OPTION    "minibar_color="
#define COLORFILE_DEFAULT_TEXT \
  LINENUMBER_OPTION "\n"       \
  MINIBAR_OPTION    "\n"

/* Lookup-table for available color opt`s. */
static const coloroption coloropt_lookup_table[] {
  {           "red", STRLTRLEN("red"),            FG_VS_CODE_RED},
  {         "green", STRLTRLEN("green"),          FG_VS_CODE_GREEN},
  {        "yellow", STRLTRLEN("yellow"),         FG_VS_CODE_YELLOW},
  {          "blue", STRLTRLEN("blue"),           FG_VS_CODE_BLUE},
  {       "magenta", STRLTRLEN("magenta"),        FG_VS_CODE_MAGENTA},
  {          "cyan", STRLTRLEN("cyan"),           FG_VS_CODE_CYAN},
  {         "white", STRLTRLEN("white"),          FG_VS_CODE_WHITE},
  {    "bright-red", STRLTRLEN("bright-red"),     FG_VS_CODE_BRIGHT_RED},
  {  "bright-green", STRLTRLEN("bright-green"),   FG_VS_CODE_BRIGHT_GREEN},
  { "bright-yellow", STRLTRLEN("bright-yellow"),  FG_VS_CODE_BRIGHT_YELLOW},
  {   "bright-blue", STRLTRLEN("bright-blue"),    FG_VS_CODE_BRIGHT_BLUE},
  {"bright-magenta", STRLTRLEN("bright-magenta"), FG_VS_CODE_BRIGHT_MAGENTA},
  {   "bright-cyan", STRLTRLEN("bright-cyan"),    FG_VS_CODE_BRIGHT_CYAN},
  {          "grey", STRLTRLEN("grey"),           FG_SUGGEST_GRAY},
  {        "bg-red", STRLTRLEN("bg-red"),         BG_VS_CODE_RED},
  {       "bg-blue", STRLTRLEN("bg-blue"),        BG_VS_CODE_BLUE},
  {      "bg-green", STRLTRLEN("bg-green"),       BG_VS_CODE_GREEN},
};
constexpr Ulong COLOROPT_LOOKUP_TABLE_SIZE = ARRAY_SIZE(coloropt_lookup_table);
#define COLOROPT_LOOKUP_TABLE_SIZE COLOROPT_LOOKUP_TABLE_SIZE

/* Optimized lookup for color opt.  Uses opt name len to minimize overhead. */
bool lookup_coloropt(const char *color, int len, int *color_opt) {
  for (Ulong i = 0; i < COLOROPT_LOOKUP_TABLE_SIZE; ++i) {
    if (len == coloropt_lookup_table[i].name_len && strncmp(color, coloropt_lookup_table[i].name, len) == 0) {
      *color_opt = coloropt_lookup_table[i].color_index;
      return TRUE;
    }
  }
  return FALSE;
}

/* Fetch the color for an option.  Return`s TRUE on success, otherwise FALSE. */
bool get_color_option(const char *data, const char *option, int *color_opt) {
  /* Fetch the color data, if any. */
  const char *opt = strstr(data, option);
  if (opt) {
    opt += const_strlen(option);
    const char *end = opt;
    ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '\n'));
    /* If there is nothing after opt.  Return early. */
    if (opt == end) {
      return FALSE;
    }
    /* Otherwise, check if the str matches any of the available colors. */
    if (lookup_coloropt(opt, (end - opt), color_opt)) {
      return TRUE;
    }
    logW("%s: Invalid color option: '%.*s'.", option, (int)(end - opt), opt);
  }
  return FALSE;
}

/* Load colorfile with values from disk. */
void load_colorfile(void) {
  /* Make sure the file always exists. */
  if (!is_file_and_exists(colorfile->filepath)) {
    lock_and_write(colorfile->filepath, COLORFILE_DEFAULT_TEXT, STRLTRLEN(COLORFILE_DEFAULT_TEXT), OVERWRITE);
  }
  /* Read the colorfile. */
  Ulong file_size;
  char *data = lock_and_read(colorfile->filepath, &file_size);
  int color;
  /* Get color opts, if any.  Otherwise, fall back to the default color. */
  colorfile->linenumber = (get_color_option(data, LINENUMBER_OPTION, &color) ? color : LINE_NUMBER);
  colorfile->minibar    = (get_color_option(data, MINIBAR_OPTION, &color)    ? color : MINI_INFOBAR);
  free(data);
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
