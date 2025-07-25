/** @file chars.c */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Whether we've enabled UTF-8 support.  Initially set to 'FALSE', and then set to 'TRUE' by utf8_init(). */
static bool use_utf8 = FALSE;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Control rep ----------------------------- */

/* Return the visible representation of control character c. */
_NODISCARD
static char control_rep(Schar c) {
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

/* ----------------------------- Is punct char ----------------------------- */

/* Return `TRUE` when the given character is a punctuation character. */
_NODISCARD _NONNULL(1)
static bool is_punct_char(const char *const c) {
  wchar_t wc;
  if (mbtowide(&wc, c) < 0) {
    return FALSE;
  }
  return iswpunct(wc);
}

/* ----------------------------- Revstrcasestr ----------------------------- */

/* This function is equivalent to strcasestr(), except in that it scans the string in reverse, starting at pointer. */
_NODISCARD _NONNULL(1, 2, 3)
static char *revstrcasestr(const char *const haystack, const char *const needle, const char *pointer)  {
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


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Utf8 init ----------------------------- */

/* Enable UTF-8 support.  Set the 'use_utf8' variable to 'TRUE'. */
void utf8_init(void) {
  use_utf8 = TRUE;
}

/* ----------------------------- Using utf8 ----------------------------- */

/* Checks if UTF-8 support has been enabled. */
bool using_utf8(void) {
  return use_utf8;
}

/* ----------------------------- Is lang word char ----------------------------- */

/* TODO: When we have remade the filetype into a enum then remove `file` dependency and add a codepoint and a filetype param. */
bool is_lang_word_char(openfilestruct *const file) {
  ASSERT(file);
  char symbol[MAXCHARLEN + 1];
  int  symlen;
  /* Do not allow checking if the char under the cursor is '\0', as this would always be true. */
  if (file->current->data[file->current_x]) {
    symlen = collect_char((file->current->data + file->current_x), symbol);
    symbol[symlen] = '\0';
    /* C/C++ */
    if ((file->is_c_file || file->is_cxx_file) && strstr("{}=|&!/", symbol)) {
      return TRUE;
    }
    /* AT&T Asm. */
    else if (file->is_atnt_asm_file && strstr("#", symbol)) {
      return TRUE;
    }
  }
  return FALSE;
}

/* Return 'TRUE' when for the openfile language this char represents a stopping point when doing prev/next word. */
bool is_language_word_char(const char *pointer, Ulong index) {
  /* C/C++ */
  if ((openfile->is_c_file || openfile->is_cxx_file) && is_char_one_of(pointer, index, "{}=|&!/")) {
    return TRUE;
  }
  /* AT&T Asm. */
  else if (openfile->is_atnt_asm_file && is_char_one_of(pointer, index, "#")) {
    return TRUE;
  }
  return FALSE;
}

/* Same as is_language_word_char().  But for the char at 'openfile->current->data+openfile->current_x'. */
bool is_cursor_language_word_char(void) {
  return is_language_word_char(openfile->current->data, openfile->current_x);
}

/* Return 'TRUE' when 'ch' is a opening enclose char.  Meaning it`s a char that can be enclosed, for example: '"{(< */
bool is_enclose_char(char ch) {
  return (ch == '"' || ch == '\'' || ch == '(' || ch == '{' || ch == '[' || ch == '<');
}

/* ----------------------------- Is alpha char ----------------------------- */

/* Return 'TRUE' when the given character is some kind of letter. */
bool is_alpha_char(const char *const restrict c) {
  wchar wc;
  if (mbtowide(&wc, c) < 0) {
    return FALSE;
  }
  return iswalpha(wc);
}

/* ----------------------------- Is alnum char ----------------------------- */

/* Return 'TRUE' when the given character is some kind of letter or a digit. */
bool is_alnum_char(const char *const restrict c) {
  wchar wc;
  if (mbtowide(&wc, c) < 0) {
    return FALSE;
  }
  return iswalnum(wc);
}

/* ----------------------------- Is blank char ----------------------------- */

/* Return 'TRUE' when the given character is space or tab or other whitespace. */
bool is_blank_char(const char *const restrict c) {
  wchar wc;
  if ((Schar)*c >= 0) {
    return ASCII_ISWHITE(*c);
  }
  if (mbtowide(&wc, c) < 0) {
    return FALSE;
  }
  return iswblank(wc);
}

/* Return 'TRUE' when prev char is blank. */
bool is_prev_blank_char(const char *pointer, Ulong index) {
  if (index && is_blank_char(pointer + index - 1)) {
    return TRUE;
  }
  return FALSE;
}

/* Return 'TRUE' when prev cursor position is blank char or other whitespace. */
bool is_prev_cursor_blank_char(void) {
  return is_prev_blank_char(openfile->current->data, openfile->current_x);
}

/* Return 'TRUE' when current cursor position is a blank char or other whitespace. */
bool is_cursor_blank_char(void) {
  return is_blank_char(openfile->current->data + openfile->current_x);
}
 
/* Return 'TRUE' when the given character is a control character. */
bool is_cntrl_char(const char *const c) {
  if (use_utf8) {
    return (!(*c & 0xE0) || *c == DEL_CODE || ((Schar)*c == -62 && (Schar)c[1] < -96));
  }
  else {
    return (!(*c & 0x60) || *c == DEL_CODE);
  }
}

/* ----------------------------- Is word char ----------------------------- */

/* Return 'TRUE' when the given character is word-forming (it is alphanumeric or specified in 'wordchars', or it is punctuation when allow_punct is TRUE). */
bool is_word_char(const char *const c, bool allow_punct) {
  char symbol[MAXCHARLEN + 1];
  int symlen;
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
    symlen = collect_char(c, symbol);
    symbol[symlen] = '\0';
    return strstr(word_chars, symbol);
  }
  else {
    return FALSE;
  }
}

/* See is_word_char().  This does the same but for the current char at 'openfile->current->data+openfile->current_x'. */
bool is_cursor_word_char(bool allow_punct) {
  return is_word_char((openfile->current->data + openfile->current_x), allow_punct);
}

/* Return 'TRUE' when char at prev index is a word char. */
bool is_prev_word_char(const char *pointer, Ulong index, bool allow_punct) {
  if (index && is_word_char((pointer + index - 1), allow_punct)) {
    return TRUE;
  }
  return FALSE;
}

/* Return 'TRUE' when char at prev cursor pos is word char. */
bool is_prev_cursor_word_char(bool allow_punct) {
  return is_prev_word_char(openfile->current->data, openfile->current_x, allow_punct);
}

/* Return 'TRUE' when the char before index is 'ch'. */
bool is_prev_char(const char *pointer, Ulong index, const char ch) {
  if (index && *(pointer + index - 1) == ch) {
    return TRUE;
  }
  return FALSE;
}

/* Return 'TRUE' when the char at prev cursor position is 'ch'. */
bool is_prev_cursor_char(const char ch) {
  return is_prev_char(openfile->current->data, openfile->current_x, ch);
}

/* Return 'TRUE' when any of the char`s is 'chars' matches the previus char in 'pointer' at 'index'. */
bool is_prev_char_one_of(const char *pointer, Ulong index, const char *chars) {
  return (index && strchr(chars, *(pointer + index - 1)));
}

/* Return 'TRUE' when any of the char`s is 'chars' matches the previus char at cursor. */
bool is_prev_cursor_char_one_of_for(openfilestruct *const file, const char *chars) {
  ASSERT(file);
  return is_prev_char_one_of(file->current->data, file->current_x, chars);
}

/* Return 'TRUE' when any of the char`s is 'chars' matches the previus char at cursor. */
bool is_prev_cursor_char_one_of(const char *chars) {
  return is_prev_cursor_char_one_of_for(CTX_OF, chars);
}

/* Return 'TRUE' when char at current cursor pos is 'ch'. */
bool is_cursor_char(const char ch) {
  if (*(openfile->current->data + openfile->current_x) == ch) {
    return TRUE;
  }
  return FALSE;
}

/* Return 'TRUE' when one char in 'chars' matches the char at 'pointer+index'. */
bool is_char_one_of(const char *pointer, Ulong index, const char *chars) {
  return (*(pointer + index) && strchr(chars, *(pointer + index)));
}

/* Return 'TRUE' when one char in 'chars' matches the char at 'openfile->current->data+openfile->current_x'. */
bool is_cursor_char_one_of(const char *chars) {
  return is_char_one_of(openfile->current->data, openfile->current_x, chars);
}

/* Returns true is the last char stop in ptr points to any char in chars. */
bool is_end_char_one_of(const char *const restrict ptr, const char *const restrict chars) {
  ASSERT(ptr);
  ASSERT(chars);
  return is_char_one_of(ptr, step_left(ptr, strlen(ptr)), chars);
}

/* ----------------------------- Is between chars ----------------------------- */

/* Return 'TRUE' when pointer+index-1 is equal to 'pre_ch' and 'pointer+index' is equal to 'post_ch'. */
bool is_between_chars(const char *pointer, Ulong index, const char pre_ch, const char post_ch) {
  if (index && *(pointer + index - 1) == pre_ch && *(pointer + index) == post_ch) {
    return TRUE;
  }
  return FALSE;
}

/* Returns `TRUE` when `file->current->data[file->current_x - 1]` == `a` and `file->current->data[file->current_x]` == `b`. */
bool is_curs_between_chars_for(openfilestruct *const restrict file, char a, char b) {
  ASSERT(file);
  return is_between_chars(file->current->data, file->current_x, a, b);
}

/* Returns `TRUE` when `openfile->current->data[openfile->current_x - 1]` == `a` and
 * `openfile->current->data[openfile->current_x]` == `b`.  Note that this correctly
 * handles the current context and is fully safe to use for the tui and the gui. */
bool is_curs_between_chars(char a, char b) {
  return is_curs_between_chars_for(CTX_OF, a, b);
}

/* ----------------------------- Is between any char pair ----------------------------- */

/* Returns `TRUE` if `ptr[index - 1]` and `ptr[index]` are the same as any pair in `pairs`. */
bool is_between_any_char_pair(const char *const restrict ptr, Ulong index, const char **const restrict pairs, Ulong *const restrict out_index) {
  ASSERT(ptr);
  ASSERT(pairs);
  /* If the passed index is zero just return `FALSE`, there is nothing to check. */
  if (!index) {
    return FALSE;
  }
  for (const char **pair=pairs; *pair; ++pair) {
    /* If `ptr[index - 1]` == `(*pair)[0]` and `ptr[index]` == `(*pair)[1]`, return `TRUE`. */
    if (is_between_chars(ptr, index, (*pair)[0], (*pair)[1])) {
      /* If the caller wants the index of the correct pair, then assign it to out_index. */
      ASSIGN_IF_VALID(out_index, (pair - pairs));
      return TRUE;
    }
  }
  return FALSE;
}

/* Returns `TRUE` if `file->current->data[file->current_x - 1]` and `file->current->data[file->current_x]` match any of the pairs first and second char. */
bool is_curs_between_any_pair_for(openfilestruct *const restrict file, const char **const restrict pairs, Ulong *const restrict out_index) {
  ASSERT(file);
  return is_between_any_char_pair(file->current->data, file->current_x, pairs, out_index);
}

/* Returns `TRUE` if `openfile->current->data[openfile->current_x - 1]` and `openfile->current->data[openfile->current_x]`
 * match any of the pairs first and second char.  Note that this is context safe and can be called from the `tui` and `gui`. */
bool is_curs_between_any_pair(const char **const restrict pairs, Ulong *const restrict out_index) {
  return is_curs_between_any_pair_for(CTX_OF, pairs, out_index);
}

/* Return 'TRUE' when char before cursor is equal to 'pre_ch' and char at cursor is equal to 'post_ch'. */
bool is_cursor_between_chars(const char pre_ch, const char post_ch) {
  return is_between_chars(openfile->current->data, openfile->current_x, pre_ch, post_ch);
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

/* ----------------------------- Mbtowide ----------------------------- */

/* Convert the given multibyte sequence c to wide character wc, and return
 * the number of bytes in the sequence, or -1 for an invalid sequence. */
int mbtowide(wchar *const restrict wc, const char *const restrict c) {
  if ((Schar)*c < 0 && use_utf8) {
    Uchar v1 = (Uchar)c[0];
    Uchar v2 = (Uchar)c[1] ^ 0x80;
    if (v2 > 0x3F || v1 < 0xC2) {
      return -1;
    }
    else if (v1 < 0xE0) {
      *wc = (((Uint)(v1 & 0x1F) << 6) | (Uint)v2);
      return 2;
    }
    Uchar v3 = (Uchar)c[2] ^ 0x80;
    if (v3 > 0x3F) {
      return -1;
    }
    else if (v1 < 0xF0) {
      if ((v1 > 0xE0 || v2 >= 0x20) && (v1 != 0xED || v2 < 0x20)) {
        *wc = (((Uint)(v1 & 0x0F) << 12) | ((Uint)v2 << 6) | (Uint)v3);
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
    else if ((v1 > 0xF0 || v2 >= 0x10) && (v1 != 0xF4 || v2 < 0x10)) {
      *wc = (((Uint)(v1 & 0x07) << 18) | ((Uint)v2 << 12) | ((Uint)v3 << 6) | (Uint)v4);
      return 4;
    }
    else {
      return -1;
    }
  }
  else {
    *wc = (Uint)*c;
    return 1;
  }
}

/* ----------------------------- Widetomb ----------------------------- */

/* Returns the length (not including the null-terminator) of wc when reprecented as
 * a multi-byte char string.  Ensure `mb` has a size of at least `MAXCHARLEN + 1`. */
int widetomb(Uint wc, char *const restrict mb) {
  ASSERT(mb);
  /* Ascii */
  if (wc <= 0x7F) {
    mb[0] = wc;
    mb[1] = NUL;
    return 1;
  }
  /* 2-byte */
  else if (wc <= 0x7FF) {
    mb[0] = (0xC0 | (wc >> 6));
    mb[1] = (0x80 | (wc & 0x3F));
    mb[2] = NUL;
    return 2;
  }
  /* 2-byte-pair */
  else if (wc >= 0xD800 && wc <= 0xDFFF) {
    mb[0] = NUL;
    return 0;
  }
  /* 3-byte */
  else if (wc <= 0xFFFF) {
    mb[0] = (0xE0 |  (wc >> 12));
    mb[1] = (0x80 | ((wc >>  6) & 0x3F));
    mb[2] = (0x80 |  (wc & 0x3F));
    mb[3] = NUL;
    return 3;
  }
  /* 4-byte */
  else if (wc <= 0x10FFFF) {
    mb[0] = (0xF0 |  (wc >> 18));
    mb[1] = (0x80 | ((wc >> 12) & 0x3F));
    mb[2] = (0x80 | ((wc >>  6) & 0x3F));
    mb[3] = (0x80 |  (wc & 0x3F));
    mb[4] = NUL;
    return 4;
  }
  /* Not a valid ascii, or utf-8 char. */
  mb[0] = NUL;
  return 0;
}

/* ----------------------------- Is doublewidth ----------------------------- */

/* Return `TRUE` when the given character occupies two cells. */
bool is_doublewidth(const char *const ch) {
  wchar_t wc;
  /* Only from U+1100 can code points have double width. */
  if ((Uchar)*ch < 0xE1 || !use_utf8) {
    return FALSE;
  }
  if (mbtowide(&wc, ch) < 0) {
    return FALSE;
  }
  return (wcwidth(wc) == 2);
}

/* ----------------------------- Is zerowidth ----------------------------- */

/* Return `TRUE` when the given character occupies zero cells. */
bool is_zerowidth(const char *ch) {
  wchar wc;
  /* Only from U+0300 can code points have zero width. */
  if ((Uchar)*ch < 0xCC || !use_utf8) {
    return FALSE;
  }
  if (mbtowide(&wc, ch) < 0) {
    return FALSE;
  }
# if defined(__OpenBSD__)
  /* Work around an OpenBSD bug -- see https://sv.gnu.org/bugs/?60393. */
  if (wc >= 0xF0000) {
    return FALSE;
  }
# endif
  return (wcwidth(wc) == 0);
}

/* Return 'TRUE' when the character at 'openfile->current->data+openfile->current_x' occupies zero cells. */
bool is_cursor_zerowidth(void) {
  return is_zerowidth(openfile->current->data + openfile->current_x);
}

/* ----------------------------- Char length ----------------------------- */

/* Return the number of bytes in the character that starts at *pointer. */
int char_length(const char *const ptr) {
  Uchar c1;
  Uchar c2;
  if ((Uchar)*ptr > 0xC1 && use_utf8) {
    c1 = (Uchar)ptr[0];
    c2 = (Uchar)ptr[1];
    if ((c2 ^ 0x80) > 0x3F) {
      return 1;
    }
    if (c1 < 0xE0) {
      return 2;
    }
    if (((Uchar)ptr[2] ^ 0x80) > 0x3F) {
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
    if (((Uchar)ptr[3] ^ 0x80) > 0x3F) {
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

/* ----------------------------- Mbstrlen ----------------------------- */

/* Return the number of (multibyte and singlebyte) characters in the given string. */
Ulong mbstrlen(const char *pointer) {
  Ulong count = 0;
  while (*pointer) {
    pointer += char_length(pointer);
    ++count;
  }
  return count;
}

/* ----------------------------- Collect char ----------------------------- */

/* Return the length (in bytes) of the character at the start of the given string, and return a copy of this character in *thechar. */
int collect_char(const char *const str, char *c) {
  int charlen = char_length(str);
  for (int i=0; i<charlen; ++i) {
    c[i] = str[i];
  }
  return charlen;
}

/* ----------------------------- Advance over ----------------------------- */

/* Return the length ( in bytes ) of the character at the start of the given string, and add this character's width to '*column'. */
int advance_over(const char *const str, Ulong *column) {
  if (*str < 0 && use_utf8) {
    /* A UTF-8 upper control code has two bytes and takes two columns. */
    if ((Uchar)str[0] == 0xC2 && (Schar)str[1] < -96) {
      (*column) += 2;
      return 2;
    }
    else {
      wchar_t wc;
      const int charlen = mbtowide(&wc, str);
      if (charlen < 0) {
        (*column) += 1;
        return 1;
      }
      const int width = wcwidth(wc);
#     if defined(__OpenBSD__)
      *(*column) += ((width < 0 || wc >= 0xF0000) ? 1 : width);
#     else
      (*column) += ((width < 0) ? 1 : width);
#     endif
      return charlen;
    }
  }
  if ((Uchar)*str < 0x20) {
    if (*str == '\t') {
      (*column) += (tabsize - (*column) % tabsize);
    }
    else {
      (*column) += 2;
    }
  }
  else if (0x7E < (Uchar)*str && (Uchar)*str < 0xA0) {
    (*column) += 2;
  }
  else {
    (*column) += 1;
  }
  return 1;
}

/* ----------------------------- Step left ----------------------------- */

/* Return the index in buf of the beginning of the multibyte character before the one at pos. */
Ulong step_left(const char *const buf, const Ulong pos) {
  Ulong before;
  Ulong charlen;
  if (use_utf8) {
    charlen = 0;
    if (pos < 4) {
      before = 0;
    }
    else {
      const char *ptr = (buf + pos);
      /* Probe for a valid starter byte in the preceding four bytes. */
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

/* Returns the index in `mbstr` of the visual beginning of the multi-byte character
 * before the one at pos, meaning that it will advance past all zerowidth chars. */
Ulong visual_step_left(const char *const restrict mbstr, Ulong pos) {
  ASSERT(mbstr);
  Ulong ret = pos;
  /* Only perform any action when not already at the beginning of the string. */
  if (ret > 0) {
    /* Move one utf-8 char to the left. */
    ret = step_left(mbstr, ret);
    /* Then advance over any zero width chars. */
    while (ret > 0 && is_zerowidth(mbstr + pos)) {
      ret = step_left(mbstr, ret);
    }
  }
  return ret;
}

/* ----------------------------- Step cursor left ----------------------------- */

/* Move `file->current_x` one step to the left, while also ensuring we pass all preceding zerowidth characters. */
void step_cursor_left(openfilestruct *const file) {
  ASSERT(file);
  /* Only perform any action when not already at the start of the current line. */
  if (file->current_x > 0) {
    /* Move one char to the left. */
    STEP_LEFT(file);
    /* Then advance over any zero-width chars. */
    while (file->current_x > 0 && IS_ZEROWIDTH(file)) {
      STEP_LEFT(file);
    }
  }
}

/* ----------------------------- Step right ----------------------------- */

/* Return the index in buf of the beginning of the multibyte character after the one at pos. */
Ulong step_right(const char *const buf, const Ulong pos) {
  return (pos + char_length(buf + pos));
}

/* ----------------------------- Step cursor right ----------------------------- */

/* Move `file->current_x` one step to the right, while also ensuring we pass all following zerowidth characters. */
void step_cursor_right(openfilestruct *const file) {
  ASSERT(file);
  /* Only perform any action when not already at the end of the current line. */
  if (file->current->data[file->current_x]) {
    /* Move one step right. */
    STEP_RIGHT(file);
    /* Then advance over any zero-width chars. */
    while (file->current->data[file->current_x] && IS_ZEROWIDTH(file)) {
      STEP_RIGHT(file);
    }
  }
}

/* ----------------------------- Mbstrcasecmp ----------------------------- */

/* This function is equivalent to strcasecmp() for multibyte strings. */
int mbstrcasecmp(const char *s1, const char *s2) {
  return mbstrncasecmp(s1, s2, HIGHEST_POSITIVE);
}

/* ----------------------------- Mbstrncasecmp ----------------------------- */

/* This function is equivalent to strncasecmp() for multibyte strings. */
int mbstrncasecmp(const char *s1, const char *s2, Ulong n) {
  Wchar wc1;
  Wchar wc2;
  bool bad1;
  bool bad2;
  int difference;
  if (use_utf8) {
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
        ++s1;
        ++s2;
        --n;
        continue;
      }
      bad1 = (mbtowide(&wc1, s1) < 0);
      bad2 = (mbtowide(&wc2, s2) < 0);
      if (bad1 || bad2) {
        if (*s1 != *s2) {
          return (int)((Uchar)*s1 - (Uchar)*s2);
        }
        if (bad1 != bad2) {
          return (bad1 ? 1 : -1);
        }
      }
      else {
        difference = (int)towlower((Wint)wc1) - (int)towlower((Wint)wc2);
        if (difference) {
          return difference;
        }
      }
      s1 += char_length(s1);
      s2 += char_length(s2);
      --n;
    }
    return ((n > 0) ? (int)((Uchar)*s1 - (Uchar)*s2) : 0);
  }
  else {
    return strncasecmp(s1, s2, n);
  }
}

/* ----------------------------- Mbstrcasestr ----------------------------- */

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

/* ----------------------------- Revstrstr ----------------------------- */

/* This function is equivalent to strstr(), except in that it scans the string in reverse, starting at pointer. */
char *revstrstr(const char *const haystack, const char *const needle, const char *pointer) {
  Ulong needle_len = strlen(needle);
  Ulong tail_len   = strlen(pointer);
  if (tail_len < needle_len) {
    pointer -= (needle_len - tail_len);
  }
  while (pointer >= haystack) {
    if (strncmp(pointer, needle, needle_len) == 0) {
      return (char *)pointer;
    }
    --pointer;
  }
  return NULL;
}

/* ----------------------------- Mbrevstrcasestr ----------------------------- */

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
      pointer = (haystack + step_left(haystack, (pointer - haystack)));
    }
  }
  else {
    return revstrcasestr(haystack, needle, pointer);
  }
}

/* ----------------------------- Mbstrchr ----------------------------- */

/* This function is equivalent to strchr() for multibyte strings.  It is used to find the
 * first occurrence of a character in a string.  The character to find is given as a
 * multibyte string.  The function is used in justify.c to find the first space in a line. */
char *mbstrchr(const char *string, const char *const chr) {
  if (use_utf8) {
    bool    bad_s = FALSE;
    bool    bad_c = FALSE;
    wchar_t ws;
    wchar_t wc;
    if (mbtowide(&wc, chr) < 0) {
      wc = (Uchar)*chr;
      bad_c = TRUE;
    }
    while (*string) {
      const int symlen = mbtowide(&ws, string);
      if (symlen < 0) {
        ws = (Uchar)*string;
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

/* ----------------------------- Mbstrpbrk ----------------------------- */

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

/* ----------------------------- Mbrevstrpbrk ----------------------------- */

/* Locate, in the string that starts at head, the first occurrence of any of
 * the characters in accept, starting from pointer and searching backwards. */
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

/* ----------------------------- Has blank char ----------------------------- */

/* Return 'TRUE' if the given string contains at least one blank character. */
bool has_blank_char(const char *restrict str) {
  while (*str && !is_blank_char(str)) {
    str += char_length(str);
  }
  return *str;
}

/* ----------------------------- White string ----------------------------- */

/* Return 'TRUE' when the given string is empty or consists of only blanks. */
bool white_string(const char *restrict str) {
  while (is_blank_char(str) || *str == '\r') {
    str += char_length(str);
  }
  return !*str;
}

/* ----------------------------- Get bracket match ----------------------------- */

char get_bracket_match(const char ch) {
  switch (ch) {
    case '{': {
      return '}';
    }
    case '}': {
      return '{';
    }
    case '[': {
      return ']';
    }
    case ']': {
      return '[';
    }
    case '(': {
      return ')';
    }
    case ')': {
      return '(';
    }
    default: {
      return NUL;
    }
  }
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
void strip_leading_blanks_from(char *const str) {
  const char *start = str;
  for (; start && is_blank_char(start); ++start);
  if (start != str) {
    memmove(str, start, (strlen(start) - 1));
  }
  // (start != str) ? memmove(str, start, (strlen(start) + 1)) : ((void *)0);
}

void strip_leading_chars_from(char *str, const char ch) {
  char *start = str;
  for (; start && *start == ch; start++)
    ;
  (start != str) ? memmove(str, start, (strlen(start) + 1)) : 0;
}

/* Works like 'strchr' except made for c/cpp code so it skips all 'string literals', 'char literals', slash and block comments. */
// const char *nstrchr_ccpp(const char *__s, const char __c) {
//   PROFILE_FUNCTION;
//   const char *end = __s;
//   do {
//     ADV_PTR(end, *end != __c && *end != '"' && *end != '\'' && *end != '/');
//     /* Start of a string literal. */
//     if (*end == '"'){
//       ++end;
//       ADV_PTR(end, *end != '"');
//       if (!*end) {
//         return NULL;
//       }
//       ++end;
//     }
//     /* Start of a char literal.  */
//     else if (*end == '\'') {
//       ++end;
//       ADV_PTR(end, *end != '\'');
//       if (!*end) {
//         return NULL;
//       }
//       ++end;  
//     }
//     /* Check if start of a comment. */
//     else if (*end == '/') {
//       ++end;
//       /* If 'EOL' or start of a slash comment.  Return 'NULL' emidietly. */
//       if (!*end || *end == '/') {
//         return NULL;
//       }
//       /* Start of block comment. */
//       else if (*end == '*') {
//         ++end;
//         if (!*end) {
//           return NULL;
//         }
//         /* Find end of block comment. */
//         do {
//           ADV_PTR(end, *end != '*');
//           if (*end == '*') {
//             ++end;
//             if (!*end) {
//               return NULL;
//             }
//             else if (*end == '/') {
//               ++end;
//               if (!*end) {
//                 return NULL;
//               }
//               break;
//             }
//           }
//         } while (*end);
//       }
//     }
//   } while (*end && *end != __c);
//   if (!*end) {
//     return NULL;
//   }
//   else if (*end == __c) {
//     return end;
//   }
//   return NULL;
// }
