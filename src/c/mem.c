#include "../include/c_proto.h"

/* Return's a allocated ptr with the size of `howmush`.  Note that this can never return `NULL`,
 * this is because it terminates apon error.  To set the die function call `set_c_die_callback()`
 * with the first parameter, a function ptr with signatue `void (*)(const char *, ...)`. */
void *xmalloc(Ulong howmush) {
  void *ptr = malloc(howmush);
  ALWAYS_ASSERT_MSG(ptr, "NanoX is out of memory.");
  return ptr;
}

void *xrealloc(void *ptr, Ulong howmush) {
  ptr = realloc(ptr, howmush);
  ALWAYS_ASSERT_MSG(ptr, "NanoX is out of memory.");
  return ptr;
}

void *xcalloc(Ulong elemno, Ulong elemsize) {
  void *ptr = calloc(elemno, elemsize);
  ALWAYS_ASSERT_MSG(ptr, "NanoX is out of memory");
  return ptr;
}
