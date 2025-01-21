#pragma once

#include <stdbool.h>
#include "../../config.h"

#define CTRL(x) (x&037)

#define NUL     '\000'
#define BELL    '\007'
#define BS      '\010'
#define TAB     '\011'
#define NL      '\012'
#define NL_STR  "\012"
#define FF      '\014'
#define CAR     '\015'
#define CAN     0x18
#define ESC     '\033'
#define ESC_STR "\033"
#define DEL     0x7f
#define DEL_STR "\177"
#define CSI     0x9b    /* Control string introducer. */
#define CSI_STR "\233"
#define DCS     0x90    /* Device control string. */
#define STERM   0x9c    /* String terminator. */

#define POUND   0xA3

#define META(x)   ((x) | 0x80)

static inline bool _CONST _ALWAYS_INLINE ascii_isdigit(int c) {
  return (c >= '0' && c <= '9');
}

#define ASCII_TOUPPER(c)  (((c) < 'a' || (c) > 'z') ? (c) : ((c) - ('a' - 'A')))
#define ASCII_TOLOWER(c)  (((c) < 'A' || (c) > 'Z') ? (c) : ((c) + ('a' - 'A')))
#define ASCII_ISLOWER(c)  ((unsigned)(c) >= 'a' && (unsigned)(c) <= 'z')
#define ASCII_ISUPPER(c)  ((unsigned)(c) >= 'A' && (unsigned)(c) <= 'Z')
#define ASCII_ISALPHA(c)  (ASCII_ISUPPER(c) || ASCII_ISLOWER(c))
#define ASCII_ISALNUM(c)  (ASCII_ISALPHA(c) || ascii_isdigit(c))

static inline bool _CONST _ALWAYS_INLINE ascii_iswhite(int c) {
  return (c == ' ' || c == '\t');
}

static inline bool _CONST _ALWAYS_INLINE ascii_iswhite_or_nul(int c) {
  return (ascii_iswhite(c) || c == NUL);
}


static inline bool _CONST _ALWAYS_INLINE ascii_ishexdigit(int c) {
  return (ascii_isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

static inline bool _CONST _ALWAYS_INLINE ascii_isident(int c) {
  return (ASCII_ISALNUM(c) || c == '_');
}

/// @returns `TRUE` when `c` is a binary digit.
static inline bool _CONST _ALWAYS_INLINE ascii_isbdigit(int c) {
  return (c == '0' || c == '1');
}

static inline bool _CONST _ALWAYS_INLINE ascii_isodigit(int c) {
  return (c >= '0' && c <= '7');
}

/// @returns `TRUE` when `c` is one of \f, \n, \r, \t, \v. 
static inline bool _CONST _ALWAYS_INLINE ascii_isspace(int c) {
  return ((c >= 9 && c <= 23) || c == ' ');
}
