#include "server.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "intvector.h"
#include "net.h"

static void* onAccept(void* args);

void server_new(size_t n) {
  const size_t listenersSize = sizeof(ServerListener) * n;
  ServerListener* listeners = (ServerListener*)malloc(listenersSize);
  memset(listeners, 0, listenersSize);
  int rc;
  for (int i = 0; i < n; ++i) {
    int mq = createMq(i, NULL, listeners[i].mqName, 0);
    if (mq < 0) {
      continue;
    }
    listeners[i].args = (ServerListenerArgs*)malloc(sizeof(ServerListenerArgs));
    listeners[i].args->mq = mq;
    listeners[i].args->i = i;
    rc = pthread_create(&listeners[i].id, NULL, &onAccept, listeners[i].args);
    if (rc != 0) {
      perror("phread_create");
      continue;
    }
  }
}

static void* onAccept(void* args) {
  ServerListenerArgs* largs = (ServerListenerArgs*)args;
  NetListener l;
  int rc = net_listen(&l, "tcp", NULL, "55443", 100);
  if (rc < 0) {
    fprintf(stderr, "server: socket_open failed\n");
    return NULL;
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
    return NULL;
  }
  ev.events = EPOLLIN;
  ev.data.fd = largs->mq;
  rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, largs->mq, &ev);
  if (rc < 0) {
    perror("epoll_ctl: mq");
    return NULL;
  }
  IntVector connections;
  intvector_new(&connections, 0);
  struct epoll_event events[kMaxEvents];
  for (;;) {
    int nfds = epoll_wait(epollfd, events, kMaxEvents, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      goto error;
    }
    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == l.fd) {
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
        /* printf("[%d] new conn: %d\n", largs->i, c.fd); */
        int* it = intvector_find(&connections, -1);
        if (it == NULL) {
          intvector_insert(&connections, c.fd);
        } else {
          *it = c.fd;
        }
      } else if (events[n].data.fd == largs->mq) {
        goto error;
      } else {
        rc = onRead(events[n].data.fd);
        if (rc == 0) {
          continue;
        }
        rc = epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
        if (rc < 0) {
          perror("epoll_ctl del conn socket");
          goto error;
        }
        close(events[n].data.fd);
        int* it = intvector_find(&connections, events[n].data.fd);
        if (it == NULL) {
          continue;
        }
        *it = -1;
      }
    }
  }
error:
  for (uint64_t i = 0; i < connections.len; ++i) {
    int v = intvector_at(&connections, i);
    if (v == -1) {
      continue;
    }
    close(v);
  }
  intvector_delete(&connections);
  close(l.fd);
  close(epollfd);
  return NULL;
}
