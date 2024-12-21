#include "../include/prototypes.h"

#define CONFIGFILE_EXT ".nxcfg"
#define CONFIGDIR      ".config/nanox/"
#define COLORFILE_NAME "color" CONFIGFILE_EXT

#define MINIBAR_OPT                   "minibar_color="
#define SELECTED_TEXT_OPT             "selectedtext_color="
#define CONFIGFILE_DEFAULT_TEXT \
  "linenumber:color="             "\n"  \
  "linenumber:barcolor="          "\n"  \
  "// Options: TRUE, FALSE, FULL" "\n"  \
  "linenumber:bar="               "\n"  \
  MINIBAR_OPT                     "\n"  \
  SELECTED_TEXT_OPT               "\n"

#define TERMINATE TRUE
#define DO_NOT_TERMINATE FALSE

static char *configdir = NULL;
/* Holds data gathered from the config file. */
static configfilestruct *configfile = NULL;
/* Global config to store data retrieved from config file. */
configstruct *config = NULL;

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
  configfilestruct *file = (configfilestruct *)arg;
  /* Line configuration. */
  config->linenumber.color           = file->data.linenumber.color;
  config->linenumber.barcolor        = file->data.linenumber.barcolor;
  config->linenumber.verticalbar     = file->data.linenumber.verticalbar;
  config->linenumber.fullverticalbar = file->data.linenumber.fullverticalbar;
  config->minibar_color       = file->data.minibar_color;
  config->selectedtext_color  = file->data.selectedtext_color;
  refresh_needed = TRUE;
}

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
    if (len == coloropt_lookup_table[i].name_len
     && strncmp(color, coloropt_lookup_table[i].name, len) == 0) {
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
    /* If not a valid color str, log an warning. */
    logW("%s: Invalid color option: '%.*s'.", option, (int)(end - opt), opt);
  }
  return FALSE;
}

/* Fetch a binary opt, return`s value from file.  Or when not in file, then default_opt. */
bool get_binary_option(const char *data, const char *option, bool default_opt) {
  /* Fetch binary option str. */
  const char *opt = strstr(data, option);
  if (opt) {
    opt += const_strlen(option);
    const char *end = opt;
    ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '\n'));
    /* If there is no str after option, return early. */
    if (opt == end) {
      return default_opt;
    }
    /* Otherwise, check if the str is either "TRUE/true" or "FALSE/false". */
    if ((end - opt) == STRLTRLEN("TRUE") || (end - opt) == STRLTRLEN("FALSE")) {
      /* Set *binary_opt to TRUE. */
      if (strncasecmp(opt, "TRUE", STRLTRLEN("TRUE")) == 0) {
        return TRUE;
      }
      /* Set *binary_opt to FALSE. */
      else if (strncasecmp(opt, "FALSE", STRLTRLEN("FALSE")) == 0) {
        return FALSE;
      }
    }
    /* Log a warning if the str was invalid. */
    logW("%s: Invalid binary option: '%.*s'.  Valid values: 'TRUE/true', 'FALSE/false'.", option, (end - opt), opt);
  }
  return default_opt;
}
#define SET_BINARY_OPT(option) get_binary_option(data, option##_OPT, DEFAULT_CONFIG(option)) ? SETCONFIGFILE(option) : void()

void get_linenumber_bar_option(const char *data) {
  configfile->data.linenumber.verticalbar     = FALSE;
  configfile->data.linenumber.fullverticalbar = FALSE;
  const char *opt = strstr(data, "linenumber:bar=");
  if (opt) {
    opt += STRLTRLEN("linenumber:bar=");
    const char *end = opt;
    ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '\n'));
    if (end == opt) {
      return;
    }
    else {
      if (strncasecmp(opt, "TRUE", STRLTRLEN("TRUE")) == 0) {
        configfile->data.linenumber.verticalbar = TRUE;
      }
      else if (strncasecmp(opt, "FULL", STRLTRLEN("FULL")) == 0) {
        configfile->data.linenumber.fullverticalbar = TRUE;
      }
    }
  }
}

/* Load configfile with values from disk. */
void load_colorfile(void) {
  /* Make sure the file always exists. */
  if (!is_file_and_exists(configfile->filepath)) {
    lock_and_write(configfile->filepath, CONFIGFILE_DEFAULT_TEXT, STRLTRLEN(CONFIGFILE_DEFAULT_TEXT), OVERWRITE);
  }
  /* Read the configfile. */
  Ulong file_size;
  char *data = lock_and_read(configfile->filepath, &file_size);
  int color;
  /* Get color opts, if any.  Otherwise, fall back to the default color. */
  configfile->data.linenumber.color    = (get_color_option(data, "linenumber:color=",    &color) ? color : LINE_NUMBER);
  configfile->data.linenumber.barcolor = (get_color_option(data, "linenumber:barcolor=", &color) ? color : LINE_NUMBER);
  configfile->data.minibar_color       = (get_color_option(data, MINIBAR_OPT,            &color) ? color : MINI_INFOBAR);
  configfile->data.selectedtext_color  = (get_color_option(data, SELECTED_TEXT_OPT,      &color) ? color : SELECTED_TEXT);
  get_linenumber_bar_option(data);
  free(data);
  enqueue_callback(update_colorfile, configfile);
}

/* Init NanoX config. */
void init_cfg(void) {
  get_homedir();
  if (!homedir) {
    return;
  }
  config = (configstruct *)nmalloc(sizeof(*config));
  configdir = concatenate_path(homedir, CONFIGDIR);
  if (!is_dir(configdir)) {
    mkdir(configdir, 0755);
  }
  configfile = (configfilestruct *)nmalloc(sizeof(*configfile));
  configfile->filepath = concatenate_path(configdir, COLORFILE_NAME);
  load_colorfile();
  file_listener_t *colorfile_listener = file_listener.add_listener(configfile->filepath);
  colorfile_listener->set_event_callback(IN_CLOSE_WRITE, NULL, FL_ACTION(
    load_colorfile();
  ));
  colorfile_listener->start_listening();
}

void cleanup_cfg(void) {
  free(configdir);
  free(configfile->filepath);
  free(configfile);
  free(config);
}