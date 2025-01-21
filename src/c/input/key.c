/** @file key.c

  @author  Melwin Svensson.
  @date    18-1-2025.

 */
#include "key.h"

#include "../../../config.h"
#include "../../include/c_defs.h"

struct keybind {
  int code;     /* The `key-code` that is bound to the `action`. */
  int menus;    /* Where this `key-binding` should be usable. */
  keybindaction action; /* The `action` this key should execute. */
};

#define BINDINGS_SIZE 255

static keybind bindings[BINDINGS_SIZE] = {0};
static Uchar bindings_size = 0;

void keybind_add(int keycode, int menus, keybindaction action) {
  if (bindings_size == BINDINGS_SIZE) {
    return;
  }
  keybind *const key = &bindings[bindings_size++];
  key->code   = keycode;
  key->menus  = menus;
  key->action = action;
}

keybindaction keybind_get(int keycode) {
  for (Uchar i = 0; i < bindings_size; ++i) {
    if (bindings[i].code == keycode) {
      return bindings[i].action;
    }
  }
  return NULL;
}
