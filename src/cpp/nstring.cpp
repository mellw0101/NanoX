#include "../include/prototypes.h"

/* Reallocate and then inject a src into dst at position at.  Return`s the new length of dst. */
Ulong inject_in(char **dst, Ulong dstlen, const char *src, Ulong srclen, Ulong at) _NO_EXCEPT {
  *dst = (char *)nrealloc(*dst, (dstlen + srclen + 1));
  memmove((*dst + at + srclen), (*dst + at), (dstlen - at + 1));
  memmove((*dst + at), src, srclen);
  return (dstlen + srclen);
}

/* Reallocate and then inject a src into dst at position at.  Return`s the new length of dst. */
Ulong inject_in(char **dst, const char *src, Ulong at) _NO_EXCEPT {
  return inject_in(dst, strlen(*dst), src, strlen(src), at);
}

/* Erase 'eraselen' of 'str' at position 'at', if 'do_realloc' is TRUE, reallocates string so its exactly the new len.  Also return`s the new len. */
Ulong erase_in(char **str, Ulong slen, Ulong at, Ulong eraselen, bool do_realloc) _NO_EXCEPT {
  memmove((*str + at), (*str + at + eraselen), (slen - at - eraselen + 1));
  if (do_realloc) {
    *str = (char *)nrealloc(*str, (slen - eraselen + 1));
  }
  return (slen - eraselen);
}

/* Erase 'eraselen' of 'str' at position 'at', if 'do_realloc' is TRUE, reallocates string so its exactly the new len.  Also return`s the new len. */
Ulong erase_in(char **str, Ulong at, Ulong eraselen, bool do_realloc) _NO_EXCEPT {
  return erase_in(str, strlen(*str), at, eraselen, do_realloc);
}