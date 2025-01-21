#include "../include/c_proto.h"
#include "xstring.h"
#include "mem.h"

char *xstrl_copy(const char *string, Ulong len) {
  char *ret = xmalloc(len + 1);
  memcpy(ret, string, len);
  ret[len] = '\0';
  return ret;
}

char *xstr_copy(const char *string) {
  return xstrl_copy(string, strlen(string));
}

void *xmeml_copy(const void *data, Ulong len) {
  void *ret = xmalloc(len);
  memcpy(ret, data, len);
  return ret;
}
