#include "../include/c_proto.h"

char *xstrl_copy(const char *string, Ulong len) {
  ASSERT(string);
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

char *valstr(const char *format, va_list ap) {
  char *ret;
  int len;
  va_list copy;
  va_copy(copy, ap);
  len = vsnprintf(NULL, 0, format, copy);
  va_end(copy);
  if (len < 0) {
    return NULL;
  }
  ret = xmalloc(len + 1);
  vsnprintf(ret, (len + 1), format, ap);
  return ret;
}

/* Return a allocated formatted string, like sprintf, just better and safe. */
char *fmtstr(const char *format, ...) {
  ASSERT(format);
  char   *ret;
  int     len;
  va_list ap;
  va_list dummy;
  /* Get the lengt we need to allocate. */
  va_start(dummy, format);
  ALWAYS_ASSERT((len = vsnprintf(NULL, 0, format, dummy)) > 0);
  va_end(dummy);
  /* Allocate the return ptr to the correct len. */
  ret = xmalloc(len + 1);
  /* Format the string into ret. */
  va_start(ap, format);
  ALWAYS_ASSERT(vsnprintf(ret, (len + 1), format, ap) > 0);
  va_end(ap);
  return ret;
}

/* Concatate a path, taking into account trailing and leading '/' for a proper path. */
char *concatpath(const char *const __restrict s1, const char *const __restrict s2) {
  Ulong s1len = strlen(s1);
  /* If either s1 end with '/' or s2 starts with '/'. */
  if ((s1[s1len - 1] == '/' && *s2 != '/') || (s1[s1len - 1] != '/' && *s2 == '/')) {
    return fmtstr("%s%s", s1, s2);
  }
  /* When both s1 and s2 starts with '/'. */
  else if (s1[s1len - 1] == '/' && *s2 == '/') {
    return fmtstr("%s%s", s1, (s2 + 1));
  }
  /* And when niether s1 end with '/' or s2 starts with '/'. */
  else {
    return fmtstr("%s/%s", s1, s2);
  }
}

/* Return's a allocated string of `len` chars of `string`. */
char *measured_copy(const char *const __restrict string, Ulong len) {
  ASSERT(string);
  char *ret = xmalloc(len + 1);
  memcpy(ret, string, len);
  ret[len] = '\0';
  return ret;
}

/* Return's a allocated copy of `string`. */
char *copy_of(const char *const __restrict string) {
  return measured_copy(string, strlen(string));
}

#define SPLIT_STRING_TIMER

/* Split a string by demiliter. */
char **split_string(const char *const string, const char delim, bool allow_empty, Ulong *n) {
#ifdef SPLIT_STRING_TIMER
  TIMER_START(timer);
#endif
  ASSERT(string);
  ASSERT(delim);
  /* Initilaze both start and end to string. */
  const char *start = string;
  const char *end   = string;
  /* Set up the return array. */
  Ulong  cap = 10;
  Ulong  len = 0;
  char **result = xmalloc(sizeof(void *) * cap);
  /* Iterate until the end of the string. */
  while (*end) {
    /* Advance end until we se the delimiter. */
    while (*end && *end != delim) {
      ++end;
    }
    ENSURE_PTR_ARRAY_SIZE(result, cap, len);
    result[len++] = measured_copy(start, (end - start));
    /* Break if we reached the end of the string. */
    if (!*end) {
      break;
    }
    /* If allow empty is false, advance end past all delim, for instance
     * if delim is ' ' then double space is just advanced past. */
    if (!allow_empty) {
      while (*end && *end == delim) {
        ++end;
      }
    }
    /* Otherwise, just advance past the delim. */
    else {
      ++end;
    }
    /* Set start to the first char that is not delim. */
    start = end;
  }
  /* Trim the array before returning it, saving memory where we can. */
  TRIM_PTR_ARRAY(result, cap, len);
  result[len] = NULL;
  ASSIGN_IF_VALID(n, len);
#ifdef SPLIT_STRING_TIMER
  TIMER_END(timer, ms);
  printf("%s: time: %.5f ms\n", __func__, (double)ms);
#endif
  return result;
}
