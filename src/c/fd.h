#pragma once

#include "../../config.h"
#include "../include/c_defs.h"

_BEGIN_C_LINKAGE

bool lock_fd(int fd, short lock_type);
bool unlock_fd(int fd);

int opennetfd(const char *address, Ushort port);

_END_C_LINKAGE
