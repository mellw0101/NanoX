#ifndef _C_PROTO__H
#define _C_PROTO__H

#include "c_defs.h"

#ifdef __cplusplus
__BEGIN_DECLS
#endif

extern int unix_socket_fd;

void unix_socket_connect(const char *path);
void unix_socket_debug(const char *format, ...);

#ifdef __cplusplus
__END_DECLS
#endif

#endif /* _C_PROTO__H */