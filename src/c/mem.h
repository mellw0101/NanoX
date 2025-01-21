#pragma once

#include "../../config.h"
#include "../include/c_proto.h"

_BEGIN_C_LINKAGE

#define CLEAR_PTR(p) memset((p), 0, sizeof(*(p)))

/* Return's a allocated ptr with the size of `howmush`.  Note that this can never return `NULL`,
 * this is because it terminates apon error.  To set the die function call `set_c_die_callback()`
 * with the first parameter, a function ptr with signatue `void (*)(const char *, ...)`. */
void *xmalloc(Ulong howmush)
  _RETURNS_NONNULL _NODISCARD;

void *xrealloc(void *ptr, Ulong howmush)
  _RETURNS_NONNULL _NODISCARD;

_END_C_LINKAGE
