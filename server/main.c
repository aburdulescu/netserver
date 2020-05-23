#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "net.h"

static const int MAX_EVENTS = 10;

void onConnect(int fd) {
}

int main(int argc, char* argv[]) {
  NetListener l;
  int rc = net_listen(&l, "tcp", NULL, "55443", 100);
  if (rc < 0) {
    fprintf(stderr, "server: socket_open failed\n");
    return 1;
  }
  int epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("epoll_create1");
    exit(1);
  }
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = l.fd;
  rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, l.fd, &ev);
  if (rc < 0) {
    perror("epoll_ctl: listen_sock");
    exit(1);
  }
  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      goto error;
    }
    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd != l.fd) {
        onConnect(events[n].data.fd);
      }
      NetConn c;
      rc = net_accept(&l, &c);
      if (rc == -1) {
        goto error;
      }
      rc = fcntl(c.fd, F_SETFL, fcntl(c.fd, F_GETFL, 0) | O_NONBLOCK);
      if (rc < 0) {
        perror("fcntl");
        goto error;
      }
      ev.events = EPOLLIN | EPOLLET;
      ev.data.fd = c.fd;
      rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, c.fd, &ev);
      if (rc < 0) {
        perror("epoll_ctl add conn socket");
        goto error;
      }
    }
  }
  return 0;
error:
  close(l.fd);
  return 1;
}
