#include "../include/prototypes.h"

/* Reallocate and then inject a src into dst at position at.  Return`s the new length of dst.  This overload
 * provides the most optimized way to inject into a str where both destination and source length are provided. */
Ulong inject_in(char **dst, Ulong dstlen, const char *src, Ulong srclen, Ulong at) _NO_EXCEPT {
  *dst = (char *)nrealloc(*dst, (dstlen + srclen + 1));
  memmove((*dst + at + srclen), (*dst + at), (dstlen - at + 1));
  memmove((*dst + at), src, srclen);
  return (dstlen + srclen);
}

/* Reallocate and then inject a src into dst at position at.  Return`s the new length of dst.
 * This overload allows for some optimization by only calling strlen on the destination string. */
Ulong inject_in(char **dst, const char *src, Ulong srclen, Ulong at) _NO_EXCEPT {
  return inject_in(dst, strlen(*dst), src, srclen, at);
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

/* Append source string to destination str.  Return`s the new len of the string. */
Ulong append_to(char **dst, Ulong dstlen, const char *src, Ulong srclen) _NO_EXCEPT {
  Ulong newlen = (dstlen + srclen);
  *dst = (char *)nrealloc(*dst, (newlen + 1));
  memmove((*dst + dstlen), src, srclen);
  (*dst)[newlen] = '\0';
  return newlen;
}

/* Append source string to destination str.  Return`s the new len of the string. */
Ulong append_to(char **dst, const char *src) _NO_EXCEPT {
  return append_to(dst, strlen(*dst), src, strlen(src));
}