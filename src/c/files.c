/** @file files.c

  @author  Melwin Svensson.
  @date    10-2-2025.

 */
#include "../include/c_proto.h"

/* Return's `TRUE` when path exists, and can be accessed by calling user. */
bool file_exists(const char *const __restrict path) {
  ASSERT(path);
  struct stat st;
  if (access(path, R_OK) != 0) {
    return FALSE;
  }
  return (stat(path, &st) != -1 && !(S_ISDIR(st.st_mode) || S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)));
}

/* Return's `TRUE` when path exists is not an executable, and can be accessed by calling user. */
bool non_exec_file_exists(const char *const __restrict path) {
  ASSERT(path);
  struct stat st;
  if (access(path, R_OK) != 0) {
    return FALSE;
  }
  return (stat(path, &st) != -1 && !(S_ISDIR(st.st_mode) || S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) && !(st.st_mode & S_IXUSR));
}

/* Allocate a given stat struct if not already allocated. */
void statalloc(const char *const __restrict path, struct stat **ptr) {
  ASSERT(path);
  ASSERT(ptr);
  /* If path does not exist, or is something other the a file, free it if its valid and return. */
  if (!file_exists(path)) {
    *ptr ? (free(*ptr), *ptr = NULL) : 0;
    return;
  }
  !*ptr ? (*ptr = xmalloc(sizeof(**ptr))) : 0;
  ALWAYS_ASSERT(stat(path, *ptr) != -1);
}
