/** @file text.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../include/c_proto.h"

#include "../include/c/wchars.h"

/* Return's the length of whilespace until first non blank char in `string`. */
Ulong indentlen(const char *const string) {
  ASSERT(string);
  const char *ptr = string;
  while (*ptr && isblankc(ptr)) {
    ptr += charlen(ptr);
  }
  return (ptr - string);
}
