#pragma once

#include "../../../config.h"

_BEGIN_C_LINKAGE

/* Opaque structure that represents a event loop. */
typedef struct nevhandler nevhandler;

/* The main event-handler for the tui. */
extern nevhandler *tui_handler;

/* Create a `nevhandler` instance. */
nevhandler *nevhandler_create(void);

/* Clean up, then free `handler`. */
void nevhandler_free(nevhandler *handler);

/* Submit a task to be performed by `handler`. */
void nevhandler_submit(nevhandler *handler, void (*cb)(void *), void *arg);

/* Stop the execution and listening for new events. */
void nevhandler_stop(nevhandler *handler, int status);

/* Start the listening for events. */
int nevhandler_start(nevhandler *handler, bool spawn_thread);

_END_C_LINKAGE
