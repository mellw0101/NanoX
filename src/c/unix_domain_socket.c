#include "../include/c_proto.h"

int unix_socket_fd = -1;

/* Connect to a unix domain socket and assign the fd to 'unix_socket_fd'.
 * Apon failure we assign '-1' to 'unix_socket_fd'. */
void unix_socket_connect(const char *path) {
  struct sockaddr_un sock;
  if ((unix_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    return;
  }
  memset(&sock, 0, sizeof(sock));
  sock.sun_family   = AF_UNIX;
  Ulong len = strlen(path);
  if (len >= sizeof(sock.sun_path)) {
    close(unix_socket_fd);
    unix_socket_fd = -1;
    return;
  }
  memcpy(sock.sun_path, path, len);
  sock.sun_path[len] = '\0';
  if (connect(unix_socket_fd, (struct sockaddr *)&sock, sizeof(sock)) < 0) {
    close(unix_socket_fd);
    unix_socket_fd = -1;
  }
}

/* Send a debug msg to the unix domain socket.  If 'unix_socket_connect'
 * has not been called or has failed this function will do nothing. */
void unix_socket_debug(const char *format, ...) {
  static char buf[BUF_SIZE];
  va_list ap;
  Ulong len;
  long written;
  Ulong total = 0;
  if (unix_socket_fd < 0) {
    return;
  }
  va_start(ap, format);
  vsnprintf(buf, BUF_SIZE, format, ap);
  va_end(ap);
  len = strlen(buf);
  while (total != len && (written = write(unix_socket_fd, buf, len)) > 0) {
    total += written;
  }
  if (written < 0) {
    unix_socket_fd = -1;
  }
}
