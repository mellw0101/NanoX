/** @file chars.cpp */
#include "../include/prototypes.h"

/* Whether we've enabled UTF-8 support.  Initially set to 'FALSE', and then set to 'TRUE' by utf8_init(). */
static bool use_utf8 = FALSE;

/* Enable UTF-8 support.  Set the 'use_utf8' variable to 'TRUE'. */
void utf8_init(void) {
  use_utf8 = TRUE;
}

/* Checks if UTF-8 support has been enabled. */
bool using_utf8(void) {
  return use_utf8;
}

/* Return 'TRUE' when the given character is some kind of letter. */
bool is_alpha_char(const char *const c) {
  wchar_t wc;
  if (mbtowide(wc, c) < 0) {
    return FALSE;
  }
  return iswalpha(wc);
}

/* Return 'TRUE' when the given character is some kind of letter or a digit. */
bool is_alnum_char(const char *const c) {
  wchar_t wc;
  if (mbtowide(wc, c) < 0) {
    return FALSE;
  }
  return iswalnum(wc);
}

/* Return 'TRUE' when the given character is space or tab or other whitespace. */
bool is_blank_char(const char *const c) {
  wchar_t wc;
  if ((signed char)*c >= 0) {
    return (*c == ' ' || *c == '\t');
  }
  if (mbtowide(wc, c) < 0) {
    return FALSE;
  }
  return iswblank(wc);
}

/* Return 'TRUE' when the given character is a control character. */
bool is_cntrl_char(const char *const c) {
  if (use_utf8) {
    return (!(c[0] & 0xE0) || c[0] == DEL_CODE || ((signed char)c[0] == -62 && (signed char)c[1] < -96));
  }
  else {
    return (!(*c & 0x60) || *c == DEL_CODE);
  }
}

/* Return 'TRUE' when the given character is a punctuation character. */
bool is_punct_char(const char *const c) {
  wchar_t wc;
  if (mbtowide(wc, c) < 0) {
    return FALSE;
  }
  return iswpunct(wc);
}

// Return 'TRUE' when the given character is word-forming (it is alphanumeric or
// specified in 'wordchars', or it is punctuation when allow_punct is TRUE).
bool is_word_char(const char *const c, bool allow_punct) {
  if (!*c) {
    return FALSE;
  }
  if (is_alnum_char(c)) {
    return TRUE;
  }
  if (allow_punct && is_punct_char(c)) {
    return TRUE;
  }
  if (word_chars && *word_chars) {
    char symbol[MAXCHARLEN + 1];
    const int symlen = collect_char(c, symbol);
    symbol[symlen] = '\0';
    return constexpr_strstr(word_chars, symbol);
  }
  else {
    return FALSE;
  }
}

/* Return the visible representation of control character c. */
char control_rep(const signed char c) {
  if (c == DEL_CODE) {
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

/* Return the visible representation of multibyte control character 'c'. */
char control_mbrep(const char *const c, bool isdata) {
  /* An embedded newline is an encoded NUL if 'isdata' is TRUE. */
  if (*c == '\n' && (isdata || as_an_at)) {
    return '@';
  }
  if (use_utf8) {
    if ((Uchar)c[0] < 128) {
      return control_rep(c[0]);
    }
    else {
      return control_rep(c[1]);
    }
  }
  else {
    return control_rep(*c);
  }
}

// Convert the given multibyte sequence c to wide character wc, and return
// the number of bytes in the sequence, or -1 for an invalid sequence.
int mbtowide(wchar_t &wc, const char *const c) {
  if ((signed char)*c < 0 && use_utf8) {
    Uchar v1 = (Uchar)c[0];
    Uchar v2 = (Uchar)c[1] ^ 0x80;
    if (v2 > 0x3F || v1 < 0xC2) {
      return -1;
    }
    if (v1 < 0xE0) {
      wc = (((Uint)(v1 & 0x1F) << 6) | (Uint)v2);
      return 2;
    }
    Uchar v3 = (Uchar)c[2] ^ 0x80;
    if (v3 > 0x3F) {
      return -1;
    }
    if (v1 < 0xF0) {
      if ((v1 > 0xE0 || v2 >= 0x20) && (v1 != 0xED || v2 < 0x20)) {
        wc = (((Uint)(v1 & 0x0F) << 12) | ((Uint)v2 << 6) | (Uint)v3);
        return 3;
      }
      else {
        return -1;
      }
    }
    Uchar v4 = (Uchar)c[3] ^ 0x80;
    if (v4 > 0x3F || v1 > 0xF4) {
      return -1;
    }
    if ((v1 > 0xF0 || v2 >= 0x10) && (v1 != 0xF4 || v2 < 0x10)) {
      wc = (((Uint)(v1 & 0x07) << 18) | ((Uint)v2 << 12) | ((Uint)v3 << 6) | (Uint)v4);
      return 4;
    }
    else {
      return -1;
    }
  }
  wc = (Uint)*c;
  return 1;
}

/* Return 'TRUE' when the given character occupies two cells. */
bool is_doublewidth(const char *const ch) {
  wchar_t wc;
  /* Only from U+1100 can code points have double width. */
  if ((Uchar)*ch < 0xE1 || !use_utf8) {
    return FALSE;
  }
  if (mbtowide(wc, ch) < 0) {
    return FALSE;
  }
  return (wcwidth(wc) == 2);
}

/* Return 'TRUE' when the given character occupies zero cells. */
bool is_zerowidth(const char *ch) {
  wchar_t wc;
  /* Only from U+0300 can code points have zero width. */
  if ((Uchar)*ch < 0xCC || !use_utf8) {
    return FALSE;
  }
  if (mbtowide(wc, ch) < 0) {
    return FALSE;
  }
#if defined(__OpenBSD__)
  /* Work around an OpenBSD bug -- see https://sv.gnu.org/bugs/?60393. */
  if (wc >= 0xF0000) {
    return FALSE;
  }
#endif
  return (wcwidth(wc) == 0);
}

/* Return the number of bytes in the character that starts at *pointer. */
int char_length(const char *const &pointer) {
  if ((Uchar)*pointer > 0xC1 && use_utf8) {
    const Uchar c1 = (Uchar)pointer[0];
    const Uchar c2 = (Uchar)pointer[1];
    if ((c2 ^ 0x80) > 0x3F) {
      return 1;
    }
    if (c1 < 0xE0) {
      return 2;
    }
    if (((Uchar)pointer[2] ^ 0x80) > 0x3F) {
      return 1;
    }
    if (c1 < 0xF0) {
      if ((c1 > 0xE0 || c2 >= 0xA0) && (c1 != 0xED || c2 < 0xA0)) {
        return 3;
      }
      else {
        return 1;
      }
    }
    if (((Uchar)pointer[3] ^ 0x80) > 0x3F) {
      return 1;
    }
    if (c1 > 0xF4) {
      return 1;
    }
    if ((c1 > 0xF0 || c2 >= 0x90) && (c1 != 0xF4 || c2 < 0x90)) {
      return 4;
    }
  }
  return 1;
}

/* Return the number of (multibyte) characters in the given string. */
Ulong mbstrlen(const char *pointer) {
  Ulong count = 0;
  while (*pointer != '\0') {
    pointer += char_length(pointer);
    count++;
  }
  return count;
}

/* Return the length (in bytes) of the character at the start of the given string, and return a copy of this character in *thechar. */
int collect_char(const char *const str, char *c) {
  const int charlen = char_length(str);
  for (int i = 0; i < charlen; i++) {
    c[i] = str[i];
  }
  return charlen;
}

/* Return the length ( in bytes ) of the character at the start of the given string, and add this character's width to '*column'. */
int advance_over(const char *const str, Ulong &column) {
  if (*str < 0 && use_utf8) {
    /* A UTF-8 upper control code has two bytes and takes two columns. */
    if ((Uchar)str[0] == 0xC2 && (signed char)str[1] < -96) {
      column += 2;
      return 2;
    }
    else {
      wchar_t wc;
      const int charlen = mbtowide(wc, str);
      if (charlen < 0) {
        column += 1;
        return 1;
      }
      const int width = wcwidth(wc);
#if defined(__OpenBSD__)
      *column += ((width < 0 || wc >= 0xF0000) ? 1 : width);
#else
      column += (width < 0) ? 1 : width;
#endif
      return charlen;
    }
  }
  if ((Uchar)*str < 0x20) {
    if (*str == '\t') {
      column += (tabsize - column % tabsize);
    }
    else {
      column += 2;
    }
  }
  else if (0x7E < (Uchar)*str && (Uchar)*str < 0xA0) {
    column += 2;
  }
  else {
    column += 1;
  }
  return 1;
}

/* Return the index in buf of the beginning of the multibyte character before the one at pos. */
Ulong step_left(const char *const buf, const Ulong pos) {
  if (use_utf8) {
    Ulong before, charlen = 0;
    if (pos < 4) {
      before = 0;
    }
    else {
      const char *ptr = (buf + pos);
      /* Probe for a valid starter byte in the preceding four bytes. */
      if ((signed char)*--ptr > -65) {
        before = (pos - 1);
      }
      else if ((signed char)*--ptr > -65) {
        before = (pos - 2);
      }
      else if ((signed char)*--ptr > -65) {
        before = (pos - 3);
      }
      else if ((signed char)*--ptr > -65) {
        before = (pos - 4);
      }
      else {
        before = (pos - 1);
      }
    }
    /* Move forward again until we reach the original character, so we know the length of its preceding character. */
    while (before < pos) {
      charlen = char_length(buf + before);
      before += charlen;
    }
    return (before - charlen);
  }
  else {
    return (!pos ? 0 : (pos - 1));
  }
}

/* Return the index in buf of the beginning of the multibyte character after the one at pos. */
Ulong step_right(const char *const buf, const Ulong pos) {
  return (pos + char_length(buf + pos));
}

/* This function is equivalent to strcasecmp() for multibyte strings. */
int mbstrcasecmp(const char *s1, const char *s2) {
  return mbstrncasecmp(s1, s2, HIGHEST_POSITIVE);
}

/* This function is equivalent to strncasecmp() for multibyte strings. */
int mbstrncasecmp(const char *s1, const char *s2, Ulong n) {
  if (use_utf8) {
    wchar_t wc1, wc2;
    while (*s1 && *s2 && n > 0) {
      if (*s1 >= 0 && *s2 >= 0) {
        if ('A' <= (*s1 & 0x5F) && (*s1 & 0x5F) <= 'Z') {
          if ('A' <= (*s2 & 0x5F) && (*s2 & 0x5F) <= 'Z') {
            if ((*s1 & 0x5F) != (*s2 & 0x5F)) {
              return ((*s1 & 0x5F) - (*s2 & 0x5F));
            }
          }
          else {
            return ((int)(*s1 | 0x20) - (int)*s2);
          }
        }
        else if ('A' <= (*s2 & 0x5F) && (*s2 & 0x5F) <= 'Z') {
          return ((int)*s1 - (int)(*s2 | 0x20));
        }
        else if (*s1 != *s2) {
          return ((int)*s1 - (int)*s2);
        }
        s1++;
        s2++;
        n--;
        continue;
      }
      bool bad1 = (mbtowide(wc1, s1) < 0), bad2 = (mbtowide(wc2, s2) < 0);
      if (bad1 || bad2) {
        if (*s1 != *s2) {
          return (int)((Uchar)*s1 - (Uchar)*s2);
        }
        if (bad1 != bad2) {
          return (bad1 ? 1 : -1);
        }
      }
      else {
        const int difference = (int)towlower((wint_t)wc1) - (int)towlower((wint_t)wc2);
        if (difference) {
          return difference;
        }
      }
      s1 += char_length(s1), s2 += char_length(s2);
      n--;
    }
    return (n > 0) ? (int)((Uchar)*s1 - (Uchar)*s2) : 0;
  }
  else {
    return strncasecmp(s1, s2, n);
  }
}

/* This function is equivalent to strcasestr() for multibyte strings. */
char *mbstrcasestr(const char *haystack, const char *const needle) {
  if (use_utf8) {
    const Ulong needle_len = mbstrlen(needle);
    while (*haystack) {
      if (mbstrncasecmp(haystack, needle, needle_len) == 0) {
        return (char *)haystack;
      }
      haystack += char_length(haystack);
    }
    return NULL;
  }
  else {
    return (char *)strcasestr(haystack, needle);
  }
}

/* This function is equivalent to strstr(), except in that it scans the string in reverse, starting at pointer. */
char *revstrstr(const char *const haystack, const char *const needle, const char *pointer) {
  const Ulong needle_len = strlen(needle), tail_len = strlen(pointer);
  if (tail_len < needle_len) {
    pointer -= (needle_len - tail_len);
  }
  while (pointer >= haystack) {
    if (strncmp(pointer, needle, needle_len) == 0) {
      return (char *)pointer;
    }
    pointer--;
  }
  return NULL;
}

/* This function is equivalent to strcasestr(), except in that it scans the string in reverse, starting at pointer. */
char *revstrcasestr(const char *const haystack, const char *const needle, const char *pointer) {
  const Ulong needle_len = strlen(needle);
  const Ulong tail_len   = strlen(pointer);
  if (tail_len < needle_len) {
    pointer -= (needle_len - tail_len);
  }
  while (pointer >= haystack) {
    if (strncasecmp(pointer, needle, needle_len) == 0) {
      return (char *)pointer;
    }
    pointer--;
  }
  return NULL;
}

/* This function is equivalent to strcasestr() for multibyte strings, except in that it scans the string in reverse, starting at pointer. */
char *mbrevstrcasestr(const char *const haystack, const char *const needle, const char *pointer) {
  if (use_utf8) {
    const Ulong needle_len = mbstrlen(needle);
    const Ulong tail_len   = mbstrlen(pointer);
    if (tail_len < needle_len) {
      pointer -= (needle_len - tail_len);
    }
    if (pointer < haystack) {
      return NULL;
    }
    while (TRUE) {
      if (!mbstrncasecmp(pointer, needle, needle_len)) {
        return (char *)pointer;
      }
      if (pointer == haystack) {
        return NULL;
      }
      pointer = haystack + step_left(haystack, pointer - haystack);
    }
  }
  else {
    return revstrcasestr(haystack, needle, pointer);
  }
}

// This function is equivalent to strchr() for multibyte strings.  It is used to find the
// first occurrence of a character in a string.  The character to find is given as a
// multibyte string.  The function is used in justify.c to find the first space in a line.
char *mbstrchr(const char *string, const char *const chr) {
  if (use_utf8) {
    bool    bad_s = FALSE;
    bool    bad_c = FALSE;
    wchar_t ws;
    wchar_t wc;
    if (mbtowide(wc, chr) < 0) {
      wc    = (Uchar)*chr;
      bad_c = TRUE;
    }
    while (*string) {
      const int symlen = mbtowide(ws, string);
      if (symlen < 0) {
        ws    = (Uchar)*string;
        bad_s = TRUE;
      }
      if (ws == wc && bad_s == bad_c) {
        break;
      }
      string += symlen;
    }
    if (!*string) {
      return NULL;
    }
    return (char *)string;
  }
  else {
    return strchr((char *)string, *chr);
  }
}

/* Locate, in the given string, the first occurrence of any of the characters in accept, searching forward. */
char *mbstrpbrk(const char *str, const char *accept) {
  while (*str) {
    if (mbstrchr(accept, str)) {
      return (char *)str;
    }
    str += char_length(str);
  }
  return NULL;
}

// Locate, in the string that starts at head, the first occurrence of any of
// the characters in accept, starting from pointer and searching backwards.
char *mbrevstrpbrk(const char *const head, const char *const accept, const char *pointer) {
  if (!*pointer) {
    if (pointer == head) {
      return NULL;
    }
    pointer = (head + step_left(head, (pointer - head)));
  }
  while (TRUE) {
    if (mbstrchr(accept, pointer)) {
      return (char *)pointer;
    }
    /* If we've reached the head of the string, we found nothing. */
    if (pointer == head) {
      return NULL;
    }
    pointer = (head + step_left(head, (pointer - head)));
  }
}

/* Return 'TRUE' if the given string contains at least one blank character. */
bool has_blank_char(const char *str) {
  while (*str && !is_blank_char(str)) {
    str += char_length(str);
  }
  return *str;
}

/* Return 'TRUE' when the given string is empty or consists of only blanks. */
bool white_string(const char *str) {
  while (*str && (is_blank_char(str) || *str == '\r')) {
    str += char_length(str);
  }
  return !*str;
}

/**
  This is the original code from 'nano', and I must say, this is fucking horible.
  Honestly why the fuck even do it like this, it is the most inefficient way to do it.

  Remove leading whitespace from the given string.

void strip_leading_blanks_from(char *str) {
  while (str && (*str == ' ' || *str == '\t')) {
    memmove(str, str + 1, strlen(str));
  }
}
 */

/* Remove leading whitespace from a given string */
void strip_leading_blanks_from(char *str) {
  char *start = str;
  for (; start && (*start == '\t' || *start == ' '); start++)
    ;
  (start != str) ? memmove(str, start, (strlen(start) + 1)) : 0;
}

void strip_leading_chars_from(char *str, const char ch) {
  char *start = str;
  for (; start && *start == ch; start++)
    ;
  (start != str) ? memmove(str, start, (strlen(start) + 1)) : 0;
}

/* Works like 'strchr' except made for c/cpp code so it skips all 'string literals', 'char literals', slash and block comments. */
const char *nstrchr_ccpp(const char *__s, const char __c) noexcept {
  PROFILE_FUNCTION;
  const char *end = __s;
  do {
    ADV_PTR(end, *end != __c && *end != '"' && *end != '\'' && *end != '/');
    /* Start of a string literal. */
    if (*end == '"'){
      ++end;
      ADV_PTR(end, *end != '"');
      if (!*end) {
        return NULL;
      }
      ++end;
    }
    /* Start of a char literal.  */
    else if (*end == '\'') {
      ++end;
      ADV_PTR(end, *end != '\'');
      if (!*end) {
        return NULL;
      }
      ++end;  
    }
    /* Check if start of a comment. */
    else if (*end == '/') {
      ++end;
      /* If 'EOL' or start of a slash comment.  Return 'NULL' emidietly. */
      if (!*end || *end == '/') {
        return NULL;
      }
      /* Start of block comment. */
      else if (*end == '*') {
        ++end;
        if (!*end) {
          return NULL;
        }
        /* Find end of block comment. */
        do {
          ADV_PTR(end, *end != '*');
          if (*end == '*') {
            ++end;
            if (!*end) {
              return NULL;
            }
            else if (*end == '/') {
              ++end;
              if (!*end) {
                return NULL;
              }
              break;
            }
          }
        } while (*end);
      }
    }
  } while (*end && *end != __c);
  if (!*end) {
    return NULL;
  }
  else if (*end == __c) {
    return end;
  }
  return NULL;
}
