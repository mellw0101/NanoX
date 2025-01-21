#include "../../include/prototypes.h"

/* Open a file.  Return`s the fd and assigns the stream to 'f'. */
int IndexFile::open_file(FILE **f) {
  int fd;
  char *full_filename = get_full_path(filename);
  struct stat fileinfo;
  if (!full_filename || stat(full_filename, &fileinfo) == -1) {
    full_filename = mallocstrcpy(full_filename, filename);
  }
  if (stat(full_filename, &fileinfo) == -1) {
    free(full_filename);
    logI("File \"%s\" not found.", filename);
    return -1;
  }
  block_sigwinch(TRUE);
  install_handler_for_Ctrl_C();
  fd = open(full_filename, O_RDONLY);
  restore_handler_for_Ctrl_C();
  block_sigwinch(FALSE);
  if (fd == -1) {
    if (errno == EINTR || !errno) {
      logI("Interupted.");
    }
    else {
      logE("Error reading %s: %s", filename, strerror(errno));
    }
  }
  else {
    *f = fdopen(fd, "rb");
    if (!*f) {
      logE("Error reading %s: %s", filename, strerror(errno));
      close(fd);
      fd = -1;
    }
  }
  free(full_filename);
  return fd;
}

#define LUMP 120

void IndexFile::read_lines(FILE *f, int fd) {
  Ulong       bufsize   = LUMP;
  Ulong       len       = 0;
  Ulong       num_lines = 0;
  char       *buf       = (char *)nmalloc(bufsize);
  int         onevalue;
  int         errorcode;
  format_type format;
  filetop = make_new_node(NULL);
  filebot = filetop;
  block_sigwinch(TRUE);
  flockfile(f);
  control_C_was_pressed = FALSE;
  while ((onevalue = getc_unlocked(f)) != EOF) {
    char input = (char)onevalue;
    if (control_C_was_pressed) {
      break;
    }
    if (input == '\n') {
      if (len > 0 && buf[len - 1] == '\r' && !ISSET(NO_CONVERT)) {
        if (num_lines > 0) {
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
      buf[len] = input;
      ++len;
      if (len == bufsize) {
        bufsize += LUMP;
        buf = arealloc(buf, bufsize);
      }
      continue;
    }
    filebot->data = encode_data(buf, len);
    filebot->next = make_new_node(filebot);
    filebot       = filebot->next;
    ++num_lines;
    len = 0;
    if (input != '\n') {
      buf[len++] = input;
    }
  }
  errorcode = errno;
  funlockfile(f);
  block_sigwinch(FALSE);
  if (!ISSET(USING_GUI) && isendwin()) {
    if (!isatty(STDIN_FILENO)) {
      reconnect_and_store_state();
    }
    terminal_init();
    doupdate();
  }
  if (ferror(f) && errorcode != EINTR && errorcode) {
    logE(strerror(errorcode));
  }
  if (control_C_was_pressed) {
    logI("Interupted.");
  }
  fclose(f);
  if (!len) {
    filebot->data = STRLTR_COPY_OF("");
  }
  else {
    bool mac_line_needs_newline = FALSE;
    if (buf[len - 1] == '\r' && !ISSET(NO_CONVERT)) {
      if (!num_lines) {
        format = MAC_FILE;
      }
      buf[--len] = '\0';
      mac_line_needs_newline = TRUE;
    }
    filebot->data = encode_data(buf, len);
    ++num_lines;
    if (mac_line_needs_newline) {
      filebot->next = make_new_node(filebot);
      filebot       = filebot->next;
      filebot->data = STRLTR_COPY_OF("");
    }
  }
  free(buf);
}

void IndexFile::get_last_time_changed(void) {
  last_time_changed = 0;
  struct stat fileinfo;
  if (stat(filename, &fileinfo) == -1) {
    logE("'stat' failed for path: '%s'.", filename);
    return;
  }
  last_time_changed = fileinfo.st_atime;
}

const char *const &IndexFile::name(void) const noexcept {
  return filename;
}

linestruct *const &IndexFile::top(void) const noexcept {
  return filetop;
}

bool IndexFile::has_changed(void) noexcept {
  time_t was_ltc = last_time_changed;
  get_last_time_changed();
  if (was_ltc != last_time_changed) {
    return TRUE;
  }
  return FALSE;
}

void IndexFile::delete_data(void) noexcept {
  free(filename);
  filename = NULL;
  free_lines(filetop);
  filetop = NULL;
  filebot = NULL;
}

void IndexFile::read_file(const char *path) {
  filename = copy_of(path);
  FILE *f;
  int   fd = open_file(&f);
  if (fd < 0) {
    return;
  }
  read_lines(f, fd);
  close(fd);
  get_last_time_changed();
}
