/** @file nfuture.h

  A simple future system built using `pthread.h`.

  @author Melwin Svensson.  01-13-2025.

 */
#pragma once

#include "../../../config.h"

_BEGIN_C_LINKAGE

/* Opaque structure for handling futures. */
typedef struct nfuture nfuture;

/* Fatal error callback to be called when we should terminate. 
 * This is set using `nfuture_set_fatal_error_callback()`. */
extern void (*nfuture_fatal_error_cb)(const char *format, ...);

/* Clean `future` and free its internal memory, then free `future`. */
void nfuture_free(nfuture *future);

/* Retrieve the result from `future`, do keep in mind that this blocks the calling thread until future is available. */
void *nfuture_get(nfuture *future);

/* Check if the result is ready, and only if it is, fetch it.  Return`s `TRUE` upon success. */
bool nfuture_try_get(nfuture *future, void **result);

/* Create a `nfuture`, to get when needed. */
nfuture *nfuture_submit(void *(*task)(void *), void *);

/* Set the callback that should be called upon a fatal error. */
void nfuture_set_fatal_error_callback(void (*cb)(const char *, ...));

_END_C_LINKAGE
