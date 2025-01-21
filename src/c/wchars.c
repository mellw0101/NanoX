#include "wchars.h"

static bool utf8_enabled = FALSE;

bool ctrl_char(const char *const c) {
  if (utf8_enabled) {
    return (!(c[0] & 0xe0) || c[0] == DEL || ((signed char)c[0] == -62 && (signed char)c[1] < -96));
  }
  else {
    return (!(*c & 0x60) || *c == DEL);
  }
}

int ctowc(wchar *wc, const char *const c) {
  if ((signed char)*c < 0 && utf8_enabled) {
    Uchar v0 = (Uchar)c[0];
    Uchar v1 = (Uchar)c[1];
    if (v1 > 0x3f || v0 < 0xc2) {
      return -1;
    }
    else if (v0 < 0xe0) {
      *wc = (((Uint)(v0 & 0x1f) << 6) | (Uint)v1);
      return 2;
    }
    Uchar v2 = ((Uchar)c[2] ^ 0x80);
    if (v2 > 0x3f) {
      return -1;
    }
    else if (v0 < 0xf0) {
      if ((v0 > 0xe0 || v1 >= 0x20) && (v0 != 0xed || v1 < 0x20)) {
        *wc = (((Uint)(v0 & 0x0f) << 12) | ((Uint)v1 << 6) | (Uint)v2);
        return 3;
      }
      else {
        return -1;
      }
    }
    Uchar v3 = ((Uchar)c[3] ^ 0x80);
    if (v3 > 0x3f || v0 > 0xf4) {
      return -1;
    }
    else if ((v0 > 0xf0 || v1 >= 0x10) && (v0 != 0xf4 || v1 < 0x10)) {
      *wc = (((Uint)(v0 & 0x07) << 18) | ((Uint)v1 << 12) | ((Uint)v2 << 6) | (Uint)v3);
      return 4;
    }
    else {
      return -1;
    }
  }
  *wc = (Uint)*c;
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

static char _NODISCARD ctrl_rep(const signed char c) {
  if (c == DEL) {
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
