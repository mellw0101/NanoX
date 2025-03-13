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

#define bob(arg1, \
  arg2) ss


int   ctowc(wchar *const wc, const char *const c) __THROW _NODISCARD _CONST _NONNULL(1, 2);
int   charlen(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
int   collectc(const char *const __restrict string, char *const __restrict c) __THROW _NODISCARD _CONST _NONNULL(1, 2);
Ulong wstrlen(const char *const __restrict string) __THROW _NODISCARD _CONST _NONNULL(1);
char *wstrchr(const char *const __restrict string, const char *const __restrict ch);
char  ctrl_mbrep(const char *const c, bool isdata) __THROW _NODISCARD _CONST _NONNULL(1);
char *stripleadblanks(char *string, Ulong *const moveno);

#ifndef __cplusplus
Ulong step_left(const char *const restrict string, Ulong pos) __THROW _NODISCARD _CONST _NONNULL(1);
Ulong step_nleft(const char *const restrict string, Ulong pos, Ulong steps) __THROW _NODISCARD _CONST _NONNULL(1);
Ulong step_right(const char *const restrict string, Ulong pos) __THROW _NODISCARD _CONST _NONNULL(1);
Ulong step_nright(const char *const restrict string, Ulong pos, Ulong steps) __THROW _NODISCARD _CONST _NONNULL(1);
#endif

bool  isctrlc(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
bool  doublewidth(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
bool  zerowidth(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
bool  isblankc(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
bool  isblankornulc(const char *const c) __THROW _NODISCARD _CONST _NONNULL(1);
bool  isalphac(const char *const __restrict c) __THROW _NODISCARD _CONST _NONNULL(1);
bool  isalnumc(const char *const __restrict c) __THROW _NODISCARD _CONST _NONNULL(1);
bool  ispunctc(const char *const __restrict c) __THROW _NODISCARD _CONST _NONNULL(1);
bool  iswordc(const char *const __restrict c, bool allow_punct, const char *const __restrict allowedchars) __THROW _NODISCARD _CONST _NONNULL(1);
// bool  isconeof(const char c, const char *const __restrict string) __THROW _NODISCARD _CONST _NONNULL(2);

_END_C_LINKAGE
