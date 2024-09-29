#ifndef _C_PROTO__H
#define _C_PROTO__H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/un.h>

__BEGIN_DECLS

#define UNIX_DOMAIN_SOCKET_PATH "/tmp/test"

extern int unix_socket_fd;

void unix_socket_connect(const char *path);
void unix_socket_debug(const char *format, ...);

__END_DECLS

#endif /* _C_PROTO__H */