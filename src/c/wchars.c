/** @file wchars.c

  @author  Melwin Svensson.
  @date    17-1-2025.

*/
#include "../include/c/wchars.h"

#include "../include/c_proto.h"


static bool utf8_enabled = TRUE;


int ctowc(wchar *const wc, const char *const c) {
  /* Insure valid params. */
  ASSERT(wc);
  ASSERT(c);
  Uchar v0, v1, v2, v3;
  /* When utf8 is enabled. */
  if ((Schar)*c < 0 && utf8_enabled) {
    v0 = (Uchar)c[0];
    v1 = ((Uchar)c[1] ^ 0x80);
    if (v1 > 0x3F || v0 < 0xC2) {
      return -1;
    }
    else if (v0 < 0xE0) {
      *wc = (((Uint)(v0 & 0x1F) << 6) | (Uint)v1);
      return 2;
    }
    v2 = ((Uchar)c[2] ^ 0x80);
    if (v2 > 0x3F) {
      return -1;
    }
    else if (v0 < 0xF0) {
      if ((v0 > 0xE0 || v1 >= 0x20) && (v0 != 0xED || v1 < 0x20)) {
        *wc = (((Uint)(v0 & 0x0F) << 12) | ((Uint)v1 << 6) | (Uint)v2);
        return 3;
      }
      else {
        return -1;
      }
    }
    v3 = ((Uchar)c[3] ^ 0x80);
    if (v3 > 0x3f || v0 > 0xf4) {
      return -1;
    }
    else if ((v0 > 0xF0 || v1 >= 0x10) && (v0 != 0xF4 || v1 < 0x10)) {
      *wc = (((Uint)(v0 & 0x07) << 18) | ((Uint)v1 << 12) | ((Uint)v2 << 6) | (Uint)v3);
      return 4;
    }
    else {
      return -1;
    }
  }
  /* Otherwise, just return one. */
  *wc = (Uint)*c;
  return 1;
}

/* Return's the number of bytes in the char that starts at `*c`. */
int charlen(const char *const c) {
  ASSERT(c);
  Uchar c0, c1, c2, c3;
  if ((Uchar)*c > 0xC1 && utf8_enabled) {
    c0 = (Uchar)c[0];
    c1 = (Uchar)c[1];
    if ((c1 ^ 0x80) > 0x3F) {
      return 1;
    }
    else if (c0 < 0xE0) {
      return 2;
    }
    c2 = (Uchar)c[2];
    if ((c2 ^ 0x80) > 0x3F) {
      return 1;
    }
    else if (c0 < 0xF0) {
      if ((c0 > 0xE0 || c1 >= 0xA0) && (c0 != 0xED || c1 < 0xA0)) {
        return 3;
      }
      else {
        return 1;
      }
    }
    c3 = (Uchar)c[3];
    if ((c3 ^ 0x80) > 0x3F || c0 > 0xF4) {
      return 1;
    }
    else if ((c0 > 0xF0 || c1 >= 0x90) && (c0 != 0xF4 || c2 < 0x90)) {
      return 4;
    }
  }
  return 1;
}

/* Return the length `in bytes` of the character at the start of the given `string`, and return a copy of this character in `*c`. */
int collectc(const char *const restrict string, char *const restrict c) {
  ASSERT(string);
  ASSERT(c);
  int clen = charlen(string);
  for (int i = 0; i < clen; ++i) {
    c[i] = string[i];
  }
  return clen;
}

/* Return's the number of multibyte char's in `string`. */
Ulong wstrlen(const char *const restrict string) {
  ASSERT(string);
  const char *ptr = string;
  Ulong count = 0;
  while (*ptr) {
    ptr += charlen(ptr);
    ++count;
  }
  return count;
}

/* Like `strchr()` but for wide chars. */
char *wstrchr(const char *const restrict string, const char *const restrict ch) {
  ASSERT(string);
  ASSERT(ch);
  /* These are 'TRUE' when we failed to make the code points into wide chars. */
  bool badc=FALSE, bads=FALSE;
  /* The wide chars we will compare. */
  wchar wc, ws;
  /* A copy of the data we want to search. */
  const char *data = string;
  /* The length of wide char we are currently comparing to in data. */
  int symlen;
  /* When utf8 is enabled, use our routine. */
  if (utf8_enabled) {
    if (ctowc(&wc, ch) < 0) {
      wc = (Uchar)*ch;
      badc = TRUE;
    }
    while (*data) {
      if ((symlen = ctowc(&ws, data)) < 0) {
        ws = (Uchar)*data;
        bads = TRUE;
      }
      if (wc == ws && badc == bads) {
        break;
      }
      data += symlen;
    }
    if (*data) {
      return NULL;
    }
    return (char *)data;
  }
  /* Otherwise, just use 'strchr()'. */
  else {
    return strchr(data, *ch);
  }
}

static char _NODISCARD ctrl_rep(const Schar c) {
  if (c == DELC) {
    return '?';
  }
  else if (c == -97) {
    return '=';
  }
  else if (c < 0) {
    return (c + 224);
  }
  else {
    return (c + 64);
  }
}

char ctrl_mbrep(const char *const c, bool isdata) {
  /* An embedded newline is an encoded NUL if 'isdata' is TRUE. */
  if (*c == NL && isdata) {
    return '@';
  }
  else if (utf8_enabled) {
    if ((Uchar)c[0] < 128) {
      return ctrl_rep(c[0]);
    }
    else {
      return ctrl_rep(c[1]);
    }
  }
  else {
    return ctrl_rep(*c);
  }
}

/* Strip all leading blank char's in `string`. */
char *stripleadblanks(char *string, Ulong *const moveno) {
  ASSERT(string);
  char *ptr = string;
  while (*ptr && isblankc(ptr)) {
    ++ptr;
  }
  ASSIGN_IF_VALID(moveno, (ptr - string));
  /* When ptr has moved, move the string. */
  if (ptr != string) {
    memmove(string, ptr, (strlen(ptr) + 1));
  }
  return string;
}

/* Returns the index in `string` of the beginning of the multibyte char before the one at pos. */
Ulong step_left(const char *const restrict string, Ulong pos) {
  /* Ensure input string is valid. */
  ASSERT(string);
  const char *ptr;
  Ulong before, clen=0;
  /* When utf8 is enabled, ensure correctness with multibyte chars. */
  if (utf8_enabled) {
    if (pos < 4) {
      before = 0;
    }
    else {
      ptr = (string + pos);
      /* Probe for a valid starter byte in the preciding four bytes. */
      if ((Schar)*--ptr > -65) {
        before = (pos - 1);
      }
      else if ((Schar)*--ptr > -65) {
        before = (pos - 2);
      }
      else if ((Schar)*--ptr > -65) {
        before = (pos - 3);
      }
      else if ((Schar)*--ptr > -65) {
        before = (pos - 4);
      }
      else {
        before = (pos - 1);
      }
    }
    /* Move forward until we reach the original char, so we know the length of its preceding char. */
    while (before < pos) {
      clen = charlen(string + before);
      before += clen;
    }
    return (before - clen);
  }
  /* Otherwise, unless pos is zero return 'pos - 1'. */
  else {
    return (!pos ? 0 : (pos - 1));
  }
}

/* Move `steps` to the left in a utf8 safe mannor. */
Ulong step_nleft(const char *const restrict string, Ulong pos, Ulong steps) {
  ASSERT(string);
  ASSERT(pos);
  ASSERT(steps);
  Ulong ret = pos;
  while (steps--) {
    ret = step_left(string, ret);
  }
  return ret;
}

/* Return's the index in `string` of the beginning of the multibyte char after the one at pos. */
Ulong step_right(const char *const restrict string, Ulong pos) {
  return (pos + charlen(string + pos));
}

/* Move `steps` to the left in a utf8 safe mannor. */
Ulong step_nright(const char *const restrict string, Ulong pos, Ulong steps) {
  ASSERT(string);
  ASSERT(pos);
  ASSERT(steps);
  Ulong ret = pos;
  while (steps--) {
    ret = step_right(string, ret);
  }
  return ret;
}

/* ------------------------------------ Boolian char checks ------------------------------------ */


bool isctrlc(const char *const c) {
  if (utf8_enabled) {
    return (!(c[0] & 0xe0) || c[0] == DELC || ((Schar)c[0] == -62 && (Schar)c[1] < -96));
  }
  else {
    return (!(*c & 0x60) || *c == DELC);
  }
}

bool doublewidth(const char *const c) {
  wchar wc;
  if ((Uchar)*c < 0xe1 || !utf8_enabled) {
    return FALSE;
  }
  else if (ctowc(&wc, c) < 0) {
    return FALSE;
  }
  return (wcwidth(wc) == 2);
}

bool zerowidth(const char *const c) {
  wchar wc;
  /* Only form U+0300 can code points have zero width. */
  if ((Uchar)*c < 0xcc || !utf8_enabled) {
    return FALSE;
  }
  else if (ctowc(&wc, c) < 0) {
    return FALSE;
  }
#if defined(__OpenBSD__)
  else if (wc > 0xf0000) {
    return FALSE;
  }
#endif
  return (wcwidth(wc) == 0);
}

/* Return's `TRUE` when `c` is a blank char. */
bool isblankc(const char *const c) {
  ASSERT(c);
  wchar wc;
  if ((Schar)*c >= 0) {
    return ascii_iswhite(*c);
  }
  return (!(ctowc(&wc, c) < 0) && iswblank(wc));
}

/* Return's `TRUE` when `c` is either a blank char or a `NUL` char. */
bool isblankornulc(const char *const c) {
  return (isblankc(c) || *c == NUL);
}

/* Return's true when `*c` is some kind of letter. */
bool isalphac(const char *const restrict c) {
  ASSERT(c);
  wchar wc;
  return (!(ctowc(&wc, c) < 0) && iswalpha(wc));
}

/* Return's true when `*c` is some kind of letter or number. */
bool isalnumc(const char *const restrict c) {
  ASSERT(c);
  wchar wc;
  return (!(ctowc(&wc, c) < 0) && iswalnum(wc));
}

/* Return's `TRUE` when the given char is a `punctuation` char. */
bool ispunctc(const char *const restrict c) {
  ASSERT(c);
  wchar wc;
  return (!(ctowc(&wc, c) < 0) && iswpunct(wc));
}

/* Return's `TRUE` if `c` is a char used in words or when `allowedchars` are not `NULL`, if it matches one of them. */
bool iswordc(const char *const restrict c, bool allow_punct, const char *const restrict allowedchars) {
  ASSERT(c);
  char symbol[sizeof(wchar) + 1];
  int symlen;
  /* If 'c' points to the end of the sequence return 'FALSE'. */
  if (!*c) {
    return FALSE;
  }
  else if (isalnumc(c)) {
    return TRUE;
  }
  else if (allow_punct && ispunctc(c)) {
    return TRUE;
  }
  else if (allowedchars && *allowedchars) {
    symlen = collectc(c, symbol);
    symbol[symlen] = '\0';
    return strstr(allowedchars, symbol);
  }
  return FALSE;
}

/* Return's `TRUE` if `c` is any char in `string`. */
bool isconeof(const char c, const char *const restrict string) {
  return (strchr(string, c));
}
