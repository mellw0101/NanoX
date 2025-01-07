#include "../include/prototypes.h"

/* Reallocate and then inject a src into dst at position at.  Return`s the new length of dst.  This overload
 * provides the most optimized way to inject into a str where both destination and source length are provided. */
Ulong inject_in(char **dst, Ulong dstlen, const char *src, Ulong srclen, Ulong at, bool realloc) _NOTHROW {
  if (realloc) {
    *dst = (char *)nrealloc(*dst, (dstlen + srclen + 1));
  }
  memmove((*dst + at + srclen), (*dst + at), (dstlen - at + 1));
  memmove((*dst + at), src, srclen);
  return (dstlen + srclen);
}

/* Reallocate and then inject a src into dst at position at.  Return`s the new length of dst.
 * This overload allows for some optimization by only calling strlen on the destination string. */
Ulong inject_in(char **dst, const char *src, Ulong srclen, Ulong at, bool realloc) _NOTHROW {
  return inject_in(dst, strlen(*dst), src, srclen, at, realloc);
}

/* Reallocate and then inject a src into dst at position at.  Return`s the new length of dst. */
Ulong inject_in(char **dst, const char *src, Ulong at, bool realloc) _NOTHROW {
  return inject_in(dst, strlen(*dst), src, strlen(src), at, realloc);
}

/* Erase 'eraselen' of 'str' at position 'at', if 'do_realloc' is TRUE, reallocates string so its exactly the new len.  Also return`s the new len. */
Ulong erase_in(char **str, Ulong slen, Ulong at, Ulong eraselen, bool do_realloc) _NOTHROW {
  memmove((*str + at), (*str + at + eraselen), (slen - at - eraselen + 1));
  if (do_realloc) {
    *str = (char *)nrealloc(*str, (slen - eraselen + 1));
  }
  return (slen - eraselen);
}

/* Erase 'eraselen' of 'str' at position 'at', if 'do_realloc' is TRUE, reallocates string so its exactly the new len.  Also return`s the new len. */
Ulong erase_in(char **str, Ulong at, Ulong eraselen, bool do_realloc) _NOTHROW {
  return erase_in(str, strlen(*str), at, eraselen, do_realloc);
}

/* Append source string to destination str.  Return`s the new len of the string. */
Ulong append_to(char **dst, Ulong dstlen, const char *src, Ulong srclen) _NOTHROW {
  Ulong newlen = (dstlen + srclen);
  *dst = (char *)nrealloc(*dst, (newlen + 1));
  memmove((*dst + dstlen), src, srclen);
  (*dst)[newlen] = '\0';
  return newlen;
}

/* Append source string to destination str.  Return`s the new len of the string. */
Ulong append_to(char **dst, const char *src) _NOTHROW {
  return append_to(dst, strlen(*dst), src, strlen(src));
}

/* Split a sting into a malloc`ed 'char **' containing malloc`ed 'char *'. */
char **split_string(const char *string, const char delim, Ulong *n) _NOTHROW {
  /* If the string is empty, return NULL. */
  if (!*string) {
    *n = 0;
    return NULL;
  }
  bool last_char_was_delim = FALSE;
  /* Set up the array size and cap.  Start with a cap of 10. */
  Ulong size = 0, cap = 10;
  char **parts = (char **)nmalloc(sizeof(char *) * cap);
  const char *start = string, *end = NULL;
  for (Ulong i = 0; string[i]; ++i) {
    if (string[i] == delim || !string[i + 1]) {
      /* Only true if the last char in the string is the the delim. */
      if (!string[i + 1] && string[i] == delim) {
        end = &string[i];
        /* When this is true we add an empty sting to the end of the array. */
        last_char_was_delim = TRUE;
      }
      else if (!string[i + 1]) {
        end = &string[i + 1];
      }
      else {
        end = &string[i];
      }
      /* Reallocate if needed. */
      ENSURE_CHARARRAY_CAPACITY(parts, cap, size);
      parts[size++] = measured_copy(start, (end - start));
      start = (end + 1);
    }
  }
  if (last_char_was_delim) {
    ENSURE_CHARARRAY_CAPACITY(parts, cap, size);
    parts[size++] = STRLTR_COPY_OF("");
  }
  /* Resize the array to the exact number of parts, to save memory. */
  parts = arealloc(parts, (sizeof(char *) * size));
  *n = size;
  return parts;
}
