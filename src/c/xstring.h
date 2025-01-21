#pragma once

#include "../../config.h"
#include "../include/c_defs.h"

_BEGIN_C_LINKAGE

char *xstrl_copy(const char *string, Ulong len);
char *xstr_copy(const char *string);
void *xmeml_copy(const void *data, Ulong len);

_END_C_LINKAGE
