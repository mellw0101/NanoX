/** @file wchars.h

  @author Melwin Svensson.    17-1-2025.

 */
#pragma once

#include "ascii_defs.h"

#include "../../config.h"
#include "../include/c_defs.h"

#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>

_BEGIN_C_LINKAGE

#define wchar wchar_t

bool ctrl_char(const char *const c) _NODISCARD _CONST;
int  ctowc(wchar *wc, const char *const c) _NODISCARD _CONST;
bool doublewidth(const char *const c) _NODISCARD _CONST;
char ctrl_mbrep(const char *const c, bool isdata);
bool zerowidth(const char *const c);

_END_C_LINKAGE
