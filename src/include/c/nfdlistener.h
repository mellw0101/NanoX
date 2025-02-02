/** @file nfdlistener.h

  @author  Melwin Svensson.
  @date    22-1-2025.

 */
#pragma once

#include "../c_defs.h"

_BEGIN_C_LINKAGE

/* Create a `file-listener` that calls `cb` when a event occurs.  Note that this will free
 * itself when it has stopped, so all that needs to be called is `nfdlistener_stop()`. */
nfdlistener *nfdlistener_create(nevhandler *handler, const char *file, Uint mask, nfdlistener_cb cb);

/* Signals to the `file-listener` that it should stop.  Note that the `file-listener` will free itself once it has stopped. */
void nfdlistener_stop(nfdlistener *listener);

_END_C_LINKAGE

