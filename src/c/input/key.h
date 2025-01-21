/** @file key.h

  @author  Melwin Svensson.
  @date    18-1-2025.

  This file is part of NanoX, a fork of Nano.

 */
#pragma once

#include "../../../config.h"

_BEGIN_C_LINKAGE

typedef void (*keybindaction)(void);
typedef struct keybind keybind;

/* Add a keybind to the internal stack. */
void keybind_add(int keycode, int menus, keybindaction action);

keybindaction keybind_get(int keycode);

_END_C_LINKAGE
