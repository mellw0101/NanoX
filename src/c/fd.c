/** @file fd.c

  @author  Melwin Svensson.
  @date    14-1-2025.

  This file is part of NanoX, a fork of nano.

  Making fd handling easier.

 */
#include "fd.h"

#include "../include/c_proto.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>

bool lock_fd(int fd, short lock_type) {
  if (fd < 0) {
    return FALSE;
  }
  struct flock lock = {
    .l_type   = lock_type,
    .l_whence = SEEK_SET,
    .l_start  = 0,
    .l_len    = 0
  };
  if (fcntl(fd, F_SETLKW, &lock) == -1) {
    return FALSE;
  }
  return TRUE;
}

bool unlock_fd(int fd) {
  struct flock lock = {0};
  lock.l_type = F_UNLCK;
  if (fcntl(fd, F_SETLK, &lock) == -1) {
    return FALSE;
  }
  return TRUE;
}

int opennetfd(const char *address, Ushort port) {
  struct sockaddr_in addr;
  int sock=socket(AF_INET,SOCK_STREAM,0);
  if (sock==-1) {
    return -1;
  }
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);
  addr.sin_addr.s_addr=inet_addr(address);
  if (connect(sock,(struct sockaddr*)&addr,sizeof(addr)) < 0) {
    return -1;
  }
  return sock;
}
