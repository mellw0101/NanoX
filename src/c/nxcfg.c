/** @file nxcfg.c
 *
 *  @author Melwin Svensson.
 *  @date   25/03/2026.
 *
 *  NOTE:
 *
 *    For now we simply migrate cpp/cfg.cpp, so we can complete the cpp -> c migration.
 *
 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


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
  "prompt:color="                 "\n"  \
  MINIBAR_OPT                     "\n"  \
  SELECTED_TEXT_OPT               "\n"

#define GET_COLOR_OPTION(data, opt, var, def)  \
  (nxcfg_get_color_option((data), S__LEN(opt), &(var)) ? (var) : (def))


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static char             *configdir  = NULL;
static configfilestruct *configfile = NULL;
static HASHMAP           colors     = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static void nxcfg_colors_init(void) {
  ASSERT(!colors);
  colors = hashmap_create();
  hashmap_insert(colors, "red",            (uintptr_t)FG_VS_CODE_RED);
  hashmap_insert(colors, "green",          (uintptr_t)FG_VS_CODE_GREEN);
  hashmap_insert(colors, "yellow",         (uintptr_t)FG_VS_CODE_YELLOW);
  hashmap_insert(colors, "blue",           (uintptr_t)FG_VS_CODE_BLUE);
  hashmap_insert(colors, "magenta",        (uintptr_t)FG_VS_CODE_MAGENTA);
  hashmap_insert(colors, "cyan",           (uintptr_t)FG_VS_CODE_CYAN);
  hashmap_insert(colors, "white",          (uintptr_t)FG_VS_CODE_WHITE);
  hashmap_insert(colors, "bright-red",     (uintptr_t)FG_VS_CODE_BRIGHT_RED);
  hashmap_insert(colors, "bright-green",   (uintptr_t)FG_VS_CODE_BRIGHT_GREEN);
  hashmap_insert(colors, "bright-yellow",  (uintptr_t)FG_VS_CODE_BRIGHT_YELLOW);
  hashmap_insert(colors, "bright-blue",    (uintptr_t)FG_VS_CODE_BRIGHT_BLUE);
  hashmap_insert(colors, "bright-magenta", (uintptr_t)FG_VS_CODE_BRIGHT_MAGENTA);
  hashmap_insert(colors, "bright-cyan",    (uintptr_t)FG_VS_CODE_BRIGHT_CYAN);
  hashmap_insert(colors, "grey",           (uintptr_t)FG_SUGGEST_GRAY);
  hashmap_insert(colors, "bg-red",         (uintptr_t)BG_VS_CODE_RED);
  hashmap_insert(colors, "bg-blue",        (uintptr_t)BG_VS_CODE_BLUE);
  hashmap_insert(colors, "bg-green",       (uintptr_t)BG_VS_CODE_GREEN);
  hashmap_insert(colors, "bg-grey",        (uintptr_t)BG_GREY_80);
  if (hashmap_get(colors, "blue")) {
    unix_socket_debug("blue EXISTS: %zu\n", (uintptr_t)hashmap_get(colors, "blue"));
  }
}

static void nxcfg_colors_free(void) {
  hashmap_free(colors);
  colors = NULL;
}

static void nxcfg_write(const char *data, Ulong len) {
  ASSERT(data);
  int fd = open(configfile->filepath, (O_WRONLY | O_CREAT | O_TRUNC), 0666);
  if (fd < 0) {
    log_ERR_FA("Failed to open: '%s': %s", configfile->filepath, strerror(errno));
  }
  FDLOCK_FULL_WR(fd, data, (long)len, TRUE);
  close(fd);
}

static char *nxcfg_read(Ulong *size) {
  int   fd = open(configfile->filepath, O_RDONLY);
  Ulong ret_len = 120;
  char *ret;
  long  total_read = 0;
  long  len;
  char  buf[4096];
  if (fd < 0) {
    log_ERR_FA("Failed to open file '%s': %s", configfile->filepath, strerror(errno));
  }
  ret = xmalloc(ret_len);
  FDLOCK_ACTION(fd, F_RDLCK,
    while ((len = read(fd, ARRAY__LEN(buf))) > 0) {
      if (len == -1) {
        log_ERR_FA("Read returned -1");
      }
      if (GE((total_read + len), ret_len)) {
        ret_len = ((total_read + len) * 2);
        ret     = xrealloc(ret, ret_len);
      }
      memcpy((ret + total_read), buf, len);
      total_read += len;
    }
  );
  close(fd);
  ret = xrealloc(ret, (total_read + 1));
  ret[total_read] = '\0';
  ASSIGN_IF_VALID(size, total_read);
  return ret;
}

static bool nxcfg_get_color_option(char *data, const char *option, Ulong option_len, int *color_opt) {
  ASSERT(colors);
  ASSERT(data);
  ASSERT(option);
  char        ch;
  uintptr_t   value;
  const char *end;
  const char *optval = strstr(data, option);
  if (optval) {
    /* Find the end of the value for this option. */
    for (optval+=option_len, end=optval; *end && *end != ' ' && *end != '\t' && *end != '\n'; ++end);
    /* If there is no value for option. */
    if (optval == end) {
      return FALSE;
    }
    /* Insert a nul-terminator, then restore it. */
    ch = *(optval + (end - optval));
    *(data + (end - data)) = '\0';
    FCIO_LOG(INFO, "%s", optval);
    value = (uintptr_t)hashmap_get(colors, optval);
    FCIO_LOG(INFO, "%d", (int)value);
    *(data + (end - data)) = ch;
    /* If the value is not a valid color, inform the user. */
    if (value) {
      *color_opt = value;
      return TRUE;
    }
    FCIO_LOG(
      INFO,
      "Invalid value '%.*s' for option '%.*s'",
      (int)(end - optval), optval, (int)(option_len - 1), option
    );
    // jot_error(
    //   "Invalid value '%.*s' for option '%.*s'",
    //   (int)(end - optval), optval, (int)(option_len - 1), option
    // );
  }
  return FALSE;
}

static void nxcfg_configfile_update_event(configfilestruct *file) {
  coloridx_linenumber     = file->data.linenumber.color;
  coloridx_linenumber_bar = file->data.linenumber.barcolor;
  verticalbar             = (file->data.linenumber.verticalbar || file->data.linenumber.fullverticalbar);
  coloridx_prompt         = file->data.prompt.color;
  coloridx_minibar        = file->data.minibar_color;
  coloridx_selected_text  = file->data.selectedtext_color;

  // config->linenumber.color           = file->data.linenumber.color;
  // config->linenumber.barcolor        = file->data.linenumber.barcolor;
  // config->linenumber.verticalbar     = file->data.linenumber.verticalbar;
  // config->linenumber.fullverticalbar = file->data.linenumber.fullverticalbar;
  // config->prompt.color               = file->data.prompt.color;
  // config->minibar_color              = file->data.minibar_color;
  // config->selectedtext_color         = file->data.selectedtext_color;
  refresh_needed = TRUE;
}

static void nxcfg_configfile_interupt_signal(void *_UNUSED arg) {
  the_window_resized = TRUE;
  ungetch(KEY_FRESH);
}

static void nxcfg_load(void) {
  int   color;
  char *data;
  /* Make sure the file always exists. */
  if (!file_exists(configfile->filepath)) {
    nxcfg_write(S__LEN(CONFIGFILE_DEFAULT_TEXT));
  }
  /* Read the configfile. */
  data = nxcfg_read(NULL);
  /* Get color options. */
  nxcfg_colors_init();
  configfile->data.linenumber.color    = GET_COLOR_OPTION(data, "linenumber:color=",    color, LINE_NUMBER);
  configfile->data.linenumber.barcolor = GET_COLOR_OPTION(data, "linenumber:barcolor=", color, LINE_NUMBER);
  configfile->data.prompt.color        = GET_COLOR_OPTION(data, "prompt:color=",        color, PROMPT_BAR);
  configfile->data.minibar_color       = GET_COLOR_OPTION(data, MINIBAR_OPT,            color, MINI_INFOBAR);
  configfile->data.selectedtext_color  = GET_COLOR_OPTION(data, SELECTED_TEXT_OPT,      color, SELECTED_TEXT);
  nxcfg_colors_free();
  free(data);
  /** TODO:
   *
   * Now that this works, and we can update live when any changes happen to the
   * color file, from anywhere, we must also implement a way where all open files are
   * also listened to, and we inform the user that the file has changed on disk.
   *
   * But, before we can fully use this, we must ensure that we do not simply
   * always use a signal to interupt, because this will always interupt, and
   * we might not be currently blocked, and because this would mean that all,
   * and any outside changes to the currently open files would also signal.
   *
   * This is why we must create a system that yes, uses a signal when needed to
   * unblock so that we process an event to any change, this way if we are not
   * currently blocking, we can either simply enqueue an event, or set some flag
   * that once we become blocked we should instantly skip blocking, but it would
   * be mush better to simply process events before we even get to the block,
   * and we should do all this after we have streamlined the curses main loop,
   * and also after we have tried to try a gui like main loop, and use a custom
   * 'tick' rating.  As in we tick very slowly when idle, and this way we would
   * achive a non blocking system, or if we should continue with curses fully,
   * but then we WILL not use it as a dependency anympore, rather we must then
   * implement our own functionality direcly into it, and then we could simply
   * implement exactly what we need, rather then go looking for something else,
   * because then I would simply make my own, as i tried and succeded, but without
   * having to remake all things directly to fully test all improvements we want.
   *
   */
  event_enqueue((EVENT_CB)nxcfg_configfile_update_event, configfile);
}

static void nxcfg_configfile_listener(void *_UNUSED data, Uint _UNUSED mask) {
  nxcfg_load();
  if (IN_CURSES_CTX) {
    event_enqueue_on_main_thread(nxcfg_configfile_interupt_signal, NULL);
  }
}

static void nxcfg_init_configdir(void) {
  ASSERT(!configdir);
  get_homedir();
  if (homedir) {
    configdir = concatpath(homedir, CONFIGDIR);
    if (!is_dir(configdir)) {
      mkdir(configdir, 0755);
    }
  }
}

static void nxcfg_init_configfile(void) {
  ASSERT(configdir);
  ASSERT(!configfile);
  configfile           = xmalloc(sizeof(*configfile));
  configfile->filepath = concatpath(configdir, COLORFILE_NAME);
  /* We must load before we set the file listener for the file. */
  nxcfg_load();
  FCIO_LOG(INFO, "Loaded nxcfg");
  file_listener_add_file(file_listener, configfile->filepath, nxcfg_configfile_listener, NULL, IN_CLOSE_WRITE);
}


/* ---------------------------------------------------------- Global-Functions ---------------------------------------------------------- */


void nxcfg_init(void) {
  // config = xmalloc(sizeof(*config));
  nxcfg_init_configdir();
  if (configdir) {
    nxcfg_init_configfile();
  }
}

void nxcfg_free(void) {
  if (configdir) {
    free(configdir);
    free(configfile->filepath);
    free(configfile);
    // free(config);
  }
}
