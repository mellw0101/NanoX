/** @file

  @author  Melwin Svensson.
  @date    9-2-2025.

 */
#include "../include/c_proto.h"

void die(const char *format, ...) {
  char *msg;
  va_list ap;
  va_start(ap, format);
  msg = valstr(format, ap);
  va_end(ap);
  if (die_cb) {
    die_cb("%s", msg);
  }
  else {
    fprintf(stderr, "%s", msg);
    exit(1);
  }
}

/* Return a ptr to the last part of a path, if there are no '/' in the path, just return `path`. */
const char *tail(const char *const path) {
  ASSERT(path);
  const char *slash = strrchr(path, '/');
  if (slash) {
    return (slash + 1);
  }
  else {
    return path;
  }
}

/* Return the extention of `path`, if any.  Otherwise, return NULL. */
const char *ext(const char *const path) {
  ASSERT(path);
  const char *pathtail = tail(path);
  /* If the tail of the path starts with '.', then this is not a extention. */
  if (*pathtail == '.') {
    return NULL;
  }
  return (strrchr(pathtail, '.'));
}

/* Return's a allocated ptr of `howmeny` threads. */
thread *get_nthreads(Ulong howmeny) {
  return xmalloc(sizeof(thread) * howmeny);
}

