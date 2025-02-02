#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../../config.h"

#ifndef Ulong
#define Ulong unsigned long
#endif
#ifndef Uint
#define Uint unsigned int
#endif
#ifndef Ushort
#define Ushort unsigned short
#endif
#ifndef Uchar
#define Uchar unsigned char
#endif
#ifndef BOOL
#define BOOL Uchar
#endif

#ifndef __cplusplus
  #define TRUE  1
  #define FALSE 0
#endif

#define SOCKLOG(...) unix_socket_debug(__VA_ARGS__)

#define UNIX_DOMAIN_SOCKET_PATH "/tmp/test"
#define BUF_SIZE 16384

#define MACRO_DO_WHILE(...) do { __VA_ARGS__ } while(0)

#define S__LEN(s) (s), (sizeof(s) -1)

_BEGIN_C_LINKAGE

/* --------------------------------------------- nevhandler --------------------------------------------- */

/* Opaque structure that represents a event loop. */
typedef struct nevhandler nevhandler;

/* --------------------------------------------- nfdlistener --------------------------------------------- */

/* `Opaque`  Structure to listen to file events. */
typedef struct nfdlistener nfdlistener;

/* Structure that represents the event that the callback gets. */
typedef struct {
  Uint  mask;
  Uint  cookie;
  const char *file; /* The file this event is from. */
} nfdlistener_event;

typedef void (*nfdlistener_cb)(nfdlistener_event *);

_END_C_LINKAGE
