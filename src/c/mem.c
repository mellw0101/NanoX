#include "mem.h"

void *xmalloc(Ulong howmush) {
  void *ret = malloc(howmush);
  if (!ret) {
    die("%s: NanoX is out of memory.\n", __func__);
  }
  return ret;
}

void *xrealloc(void *ptr, Ulong howmush) {
  ptr = realloc(ptr, howmush);
  if (!ptr) {
    die("%s: NanoX is out of memory.\n", __func__);
  }
  return ptr;
}
