#ifndef _C_PROTO__H
#define _C_PROTO__H

#include "../../config.h"

_BEGIN_C_LINKAGE

#include "c_defs.h"

extern int unix_socket_fd;

void unix_socket_connect(const char *path);
void unix_socket_debug(const char *format, ...);

#if !defined(__cplusplus)
#define die(...) MACRO_DO_WHILE(  \
  if (die_cb) {                   \
    die_cb(__VA_ARGS__);          \
  }                               \
  else {                          \
    fprintf(stderr, __VA_ARGS__); \
    exit(1);                      \
  }                               \
)
#endif
/* This callback will be called upon when a fatal error occurs.
 * That means this function should terminate the application. */
static void (*die_cb)(const char *, ...) = NULL;
static inline void set_c_die_callback(void (*cb)(const char *, ...)) {
  die_cb = cb;
}

_END_C_LINKAGE

#endif /* _C_PROTO__H */
