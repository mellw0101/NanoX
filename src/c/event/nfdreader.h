#pragma once

#include "../../include/c_defs.h"
#include "../../../config.h"
#include "nevhandler.h"

_BEGIN_C_LINKAGE

/* `Opaque`  Structure that reads from a fd and safely sends the data to the event-handler. */
typedef struct nfdreader nfdreader;

extern nfdreader *nfdreader_stdin;

/* Create a async fd-reader that sends the data to the event-handler, using the cb to execute the wanted command. */
nfdreader *nfdreader_create(
  nevhandler *handler,
  int fd,
  void (*cb)(nevhandler *handler, const Uchar *data, long len));

/* Stop the thread from reading then join it to the calling thread. */
void nfdreader_stop(nfdreader *reader);

/* Close the fd and free the reader.  Note that this will only do that if `nfdreader_stop()` has been called. */
void nfdreader_free(nfdreader *reader);

_END_C_LINKAGE
