#ifndef _C_PROTO__H
#define _C_PROTO__H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifndef Ulong
#define Ulong unsigned long
#endif
#ifndef Uint
#define Uint unsigned int
#endif
#ifndef Ushort
#define Ushort unsigned short
#endif
#ifndef Uchar
#define Uchar unsigned char
#endif

__BEGIN_DECLS

#define UNIX_DOMAIN_SOCKET_PATH "/tmp/test"
#define BUF_SIZE 16384

extern int unix_socket_fd;

void unix_socket_connect(const char *path);
void unix_socket_debug(const char *format, ...);

__END_DECLS

#endif /* _C_PROTO__H */