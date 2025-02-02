/** @file nfdreader.h

  @author  Melwin Svensson.
  @date    1-13-2025.

  A fully async fd reader that sends the read data to the a nevhandler to perform the passed callback.

 */
#pragma once

#include "../../include/c_defs.h"
#include "nevhandler.h"

_BEGIN_C_LINKAGE

/* `Opaque`  Structure that reads from a fd and safely sends the data to the event-handler. */
typedef struct nfdreader nfdreader;

typedef void (*nfdreader_cb)(nevhandler *handler, const Uchar *data, long len);

extern nfdreader *nfdreader_stdin;

/* Create a async fd-reader that sends the data to the event-handler, using the cb to execute the wanted command. */
nfdreader *nfdreader_create(nevhandler *handler, int fd, nfdreader_cb callback);

/* Stop the thread from reading then join it to the calling thread. */
void nfdreader_stop(nfdreader *reader);

_END_C_LINKAGE
