#include "../include/c_proto.h"

void *xmeml_copy(const void *data, Ulong len) {
  void *ret = xmalloc(len);
  memcpy(ret, data, len);
  return ret;
}

#define SPLIT_STRING_TIMER

/* Split a string by demiliter. */
char **split_string_nano(const char *const string, const char delim, bool allow_empty, Ulong *n) {
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
