/** @file fd.c

  @author  Melwin Svensson.
  @date    14-1-2025.

  This file is part of NanoX, a fork of nano.

  Making fd handling easier.

 */
#include "../include/c_proto.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>

int opennetfd(const char *address, Ushort port) {
  struct sockaddr_in addr;
  int sock = socket(AF_INET,SOCK_STREAM,0);
  if (sock == -1) {
    return -1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);
  addr.sin_addr.s_addr = inet_addr(address);
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    return -1;
  }
  return sock;
}

/* Return's a local `UNIX` fd. */
int unixfd(void) {
  int ret;
  /* Ensure this never fails, as if it does we have big problems. */
  ALWAYS_ASSERT_MSG(((ret = socket(AF_UNIX, SOCK_STREAM, 0)) != -1), strerror(errno));
  return ret;
}
