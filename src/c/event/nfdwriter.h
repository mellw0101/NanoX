/** @file nfdwriter.h

  @author Melwin Svensson.  17-1-2025.

 */
#pragma once

#include "../../../config.h"
#include "../../include/c_defs.h"

_BEGIN_C_LINKAGE

/* `Opaque`  Structure that writes to a fd. */
typedef struct nfdwriter nfdwriter;

extern nfdwriter *nfdwriter_stdout;
extern nfdwriter *nfdwriter_netlog;

/* Create a new `nfdwriter` instance. */
nfdwriter *nfdwriter_create(int fd);

/* Write to a fd in a process safe and thread safe manner, fully async. */
void nfdwriter_write(nfdwriter *writer, const void *data, Ulong len);

int nfdwriter_printf(nfdwriter *writer, const char *fmt, ...) _PRINTFLIKE(2, 3);

/* Stops the thread for `writer`.  This function is `NULL-SAFE`. */
void nfdwriter_stop(nfdwriter *writer);

void nfdwriter_flush(nfdwriter *writer);

/* Frees `writer`s memory, note that this will do nothing if `nfdwriter_stop()` has not been called before. */
void nfdwriter_free(nfdwriter *writer);

/* Write to `stdout` in a fully async manner.  Note that `nfdwriter_stdout` need's to be created using `nfdwriter_create()` for this to work. */
#define nfdwriter_write_stdout(...) nfdwriter_write(nfdwriter_stdout, __VA_ARGS__)

void nfdwriter_netlog_init(const char *addr, Ushort port);

_END_C_LINKAGE
