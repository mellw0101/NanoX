/** @file socket.c

  @author  Melwin Svensson.
  @date    14-2-2025.

 */
#include "../include/c_proto.h"

#include "../include/c/wchars.h"

#define TEST_SOCKET_PATH  "/tmp/test" /* "/tmp/nanox_test_epoll_socket" */
#define MAX_EVENTS        10
#define BUFFER_SIZE       128

int serverfd = -1;
int globclientfd = -1;

/* Fork then start the nanox socket inside the child. */
void nanox_fork_socket(void) {
  int pid;
  ALWAYS_ASSERT_MSG(((pid = fork()) != -1), strerror(errno));
  /* Child. */
  if (pid == 0) {
    exit(nanox_socketrun());
  }
  /* Parent. */
}

/* Start our socket. */
int nanox_socketrun(void) {
  int clientfd;
  int epollfd;
  int eventcount;
  long bytesread;
  Ulong total = 0;
  struct sockaddr_un addr;
  struct epoll_event event, events[MAX_EVENTS];
  char buffer[BUFFER_SIZE] = {0};
  /* Create UNIX socket. */
  serverfd = unixfd();
  /* For now remove the socket if it exists. */
  unlink(TEST_SOCKET_PATH);
  /* Set up socket. */
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, S__LEN(TEST_SOCKET_PATH));
  /* Bind socket to path. */
  ALWAYS_ASSERT(bind(serverfd, (struct sockaddr_un *)&addr, sizeof(addr)) != -1);
  ALWAYS_ASSERT(listen(serverfd, 5) != -1);
  ALWAYS_ASSERT((epollfd = epoll_create1(0)) != -1);
  /* Add server to epoll. */
  event.events = EPOLLIN;
  event.data.fd = serverfd;
  ALWAYS_ASSERT(epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &event) != -1);
  printf("Socket listening on: %s\n", TEST_SOCKET_PATH);
  while (1) {
    eventcount = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    for (int i=0; i<eventcount; ++i) {
      if (events[i].data.fd == serverfd) {
        /* Accept new client. */
        ALWAYS_ASSERT((clientfd = accept(serverfd, NULL, NULL)) != -1);
        /* Add client to epoll. */
        event.events  = (EPOLLIN | EPOLLET);
        event.data.fd = clientfd;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &event);
      }
      else {
        /* Read the incomming data. */
        while ((bytesread = read(events[i].data.fd, BUF__LEN(buffer))) > 0) {
          total += bytesread;
          write(STDOUT_FILENO, buffer, bytesread);
        }
        /* Handle client message.  Terminating apon any errors from 'read()'. */
        ALWAYS_ASSERT_MSG((bytesread != -1), strerror(errno));
        /* Client disconnected. */
        if (total == 0) {
          close(events[i].data.fd);
          epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
        }
        else {
          /* Check for hello. */
          if (strncmp(buffer, S__LEN("hello")) == 0 && isblankornulc(buffer + SLTLEN("hello"))) {
            write(events[i].data.fd, S__LEN("Hello client.\n"));
          }
          /* Check for termination. */
          else if (strncmp(buffer, S__LEN("kill")) == 0 && isblankornulc(buffer + SLTLEN("kill"))) {
            break;
          }
        }
      }
    }
  }
  printf("Socket: %s: Terminating.\n", TEST_SOCKET_PATH);
  close(serverfd);
  unlink(TEST_SOCKET_PATH);
  return 0;
}

/* Create and connect a client to the socket, and return the fd. */
int nanox_socket_client(void) {
  int fd;
  struct sockaddr_un addr;  
  ALWAYS_ASSERT_MSG(((fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1), strerror(errno));
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, S__LEN(TEST_SOCKET_PATH));
  ALWAYS_ASSERT_MSG((connect(fd, (struct sockaddr_un *)&addr, sizeof(addr)) != -1), strerror(errno));
  return fd;
}
