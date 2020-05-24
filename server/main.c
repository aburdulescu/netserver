#include <errno.h>
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

// 1 - Connection closed
// 0 - Ok
int onRequest(int fd) {
  const size_t bufSize = 8192;
  uint8_t buf[bufSize];
  memset(buf, 0, bufSize);
  int rc;
  rc = recv(fd, buf, bufSize, 0);
  if (rc == EAGAIN || rc == EWOULDBLOCK) {
    printf("%s: reading from %d would block\n", __FUNCTION__, fd);
    return 0;
  }
  if (rc < 0) {
    perror("recv");
    return 0;
  }
  if (rc == 0) {
    return 1;
  }
  printf("%s: %d: %d %s\n", __FUNCTION__, fd, rc, (char*)buf);
  rc = send(fd, buf, rc, 0);
  if (rc == EAGAIN || rc == EWOULDBLOCK) {
    printf("%s: writing to %d would block\n", __FUNCTION__, fd);
    return 0;
  }
  if (rc < 0) {
    perror("send");
    return 0;
  }
  return 0;
}

// TODO: use SO_REUSEPORT to load balance accepts(https://lwn.net/Articles/542629/)
// TODO: use multiple threads for request
// handling(https://idea.popcount.org/2017-02-20-epoll-is-fundamentally-broken-12/)
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
        rc = onRequest(events[n].data.fd);
        if (rc == 0) {
          continue;
        }
        rc = epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
        if (rc < 0) {
          perror("epoll_ctl del conn socket");
          goto error;
        }
        close(events[n].data.fd);
        printf("%s: %d closed\n", __FUNCTION__, events[n].data.fd);
        continue;
      }
      NetConn c;
      rc = net_accept(&l, &c);
      if (rc == -1) {
        goto error;
      }
      ev.events = EPOLLIN;
      ev.data.fd = c.fd;
      rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, c.fd, &ev);
      if (rc < 0) {
        perror("epoll_ctl add conn socket");
        goto error;
      }
      printf("%d added\n", c.fd);
    }
  }
  return 0;
error:
  close(l.fd);
  return 1;
}
