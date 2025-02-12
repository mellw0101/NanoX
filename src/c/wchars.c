/** @file wchars.c

  @author  Melwin Svensson.
  @date    17-1-2025.

*/
#include "../include/c/wchars.h"

#include "../include/c_proto.h"


static bool utf8_enabled = FALSE;


bool ctrl_char(const char *const c) {
  if (utf8_enabled) {
    return (!(c[0] & 0xe0) || c[0] == DELC || ((signed char)c[0] == -62 && (signed char)c[1] < -96));
  }
  else {
    return (!(*c & 0x60) || *c == DELC);
  }
}

int ctowc(wchar *const wc, const char *const c) {
  /* Insure valid params. */
  ASSERT(wc);
  ASSERT(c);
  Uchar v0, v1, v2, v3;
  /* When utf8 is enabled. */
  if ((Schar)*c < 0 && utf8_enabled) {
    v0 = (Uchar)c[0];
    v1 = (Uchar)c[1];
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
  else {
    *wc = (Uint)*c;
    return 1;
  }
}

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
    return (*c == ' ' || *c == '\t');
  }
  else if (ctowc(&wc, c) < 0) {
    return FALSE;
  }
  return iswblank(wc);
}
