#include "../../include/prototypes.h"

int IndexFile::open_file(FILE **f) {
  int         fd;
  struct stat fileinfo;
  char       *full_filename = get_full_path(filename);
  if (!full_filename || stat(full_filename, &fileinfo) == -1) {
    full_filename = mallocstrcpy(full_filename, filename);
  }
  if (stat(full_filename, &fileinfo) == -1) {
    free(full_filename);
    logI("File \"%s\" not found.", filename);
    return -1;
  }
  block_sigwinch(true);
  install_handler_for_Ctrl_C();
  fd = open(full_filename, O_RDONLY);
  restore_handler_for_Ctrl_C();
  block_sigwinch(false);
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
  filetop = make_new_node(nullptr);
  filebot = filetop;
  block_sigwinch(true);
  flockfile(f);
  control_C_was_pressed = false;
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
        buf = (char *)nrealloc(buf, bufsize);
      }
      continue;
    }
    filebot->data = encode_data(buf, len);
    filebot->next = make_new_node(filebot);
    filebot = filebot->next;
    ++num_lines;
    len = 0;
    if (input != '\n') {
      buf[len++] = input;
    }
  }
  errorcode = errno;
  funlockfile(f);
  block_sigwinch(false);
  if (isendwin()) {
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
    filebot->data = copy_of("");
  }
  else {
    bool mac_line_needs_newline = false;
    if (buf[len - 1] == '\r' && !ISSET(NO_CONVERT)) {
      if (!num_lines) {
        format = MAC_FILE;
      }
      buf[--len] = '\0';
      mac_line_needs_newline = true;
    }
    filebot->data = encode_data(buf, len);
    ++num_lines;
    if (mac_line_needs_newline) {
      filebot->next = make_new_node(filebot);
      filebot = filebot->next;
      filebot->data = copy_of("");
    }
  }
  free(buf);
}

void IndexFile::read_file(const char *path) {
  filename = memmove_copy_of(path);
  FILE *f;
  int fd = open_file(&f);
  if (fd < 0) {
    return;
  }
  read_lines(f, fd);
}
