/** @file wchars.h

  @author Melwin Svensson.
  @date   17-1-2025.

 */
#pragma once

#include "ascii_defs.h"


#include "../../../config.h"
#include "../c_defs.h"


#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>


_BEGIN_C_LINKAGE


#define wchar wchar_t


bool ctrl_char(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
int  ctowc(wchar *const wc, const char *const c) __THROW _NODISCARD _CONST _NONNULL(1, 2);
int  charlen(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
bool doublewidth(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
char ctrl_mbrep(const char *const c, bool isdata) __THROW _NODISCARD _CONST _NONNULL(1);
bool zerowidth(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
bool isblankc(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);


_END_C_LINKAGE
