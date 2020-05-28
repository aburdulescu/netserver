#include <errno.h>
#include <mqueue.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "net.h"

static const int MAX_EVENTS = 10;

typedef struct {
  int* data;
  size_t len;
  size_t cap;
} IntList;

static void intlist_new(IntList* l, size_t cap) {
  l->cap = (cap == 0) ? 1 : cap;
  l->len = 0;
  l->data = (int*)malloc(sizeof(int) * cap);
}

static void intlist_delete(const IntList* l) {
  free(l->data);
}

static void intlist_insert(IntList* l, int v) {
  if (l->len == l->cap) {
    l->cap *= 2;
    int* newData = (int*)malloc(sizeof(int) * l->cap);
    memcpy(newData, l->data, l->len * sizeof(int));
    free(l->data);
    l->data = newData;
    return;
  }
  l->data[l->len] = v;
  ++l->len;
}

static int intlist_at(const IntList* l, int i) {
  if (i < 0 && i >= l->len) {
    return -1;
  }
  return l->data[i];
}

static int* intlist_find(const IntList* l, int v) {
  for (int i = 0; i < l->len; ++i) {
    if (l->data[i] == v) {
      return l->data + i;
    }
  }
  return NULL;
}

// 1 - Connection closed
// 0 - Ok
static int onRead(int fd) {
  const size_t bufSize = 8192;
  uint8_t buf[bufSize];
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

typedef struct {
  int i;
  int mq;
} ListenerArgs;

static void* onAccept(void* args) {
  ListenerArgs* largs = (ListenerArgs*)args;
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
  IntList connections;
  intlist_new(&connections, 0);
  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
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
        int* it = intlist_find(&connections, -1);
        if (it == NULL) {
          intlist_insert(&connections, c.fd);
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
        int* it = intlist_find(&connections, events[n].data.fd);
        if (it == NULL) {
          continue;
        }
        *it = -1;
      }
    }
  }
error:
  for (int i = 0; i < connections.len; ++i) {
    int v = intlist_at(&connections, i);
    if (v == -1) {
      continue;
    }
    close(v);
  }
  intlist_delete(&connections);
  close(l.fd);
  close(epollfd);
  return NULL;
}

const size_t MQ_MAX_NAME = 128;
const char gMqNamePrefix[] = "/epolls_mq_";
const size_t gMqNamePrefixSize = sizeof(gMqNamePrefix) - 1;

int createMq(int i, char* mqSuffix, char* mqName, int block) {
  memset(mqName, 0, MQ_MAX_NAME);
  strncpy(mqName, gMqNamePrefix, gMqNamePrefixSize);
  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = 256;
  attr.mq_curmsgs = 0;
  int prefixEnd = gMqNamePrefixSize;
  if (mqSuffix != NULL) {
    size_t mqSuffixSize = strlen(mqSuffix);
    strncpy(mqName + prefixEnd, mqSuffix, mqSuffixSize);
    prefixEnd += mqSuffixSize;
  }
  if (i != -1) {
    char istr[3];
    memset(istr, 0, 3);
    sprintf(istr, "%d", i);
    strncpy(mqName + prefixEnd, istr, 2);
  }
  int rc;
  if (block)
    rc = mq_open(mqName, O_RDWR | O_CREAT, 0660, &attr);
  else
    rc = mq_open(mqName, O_RDWR | O_CREAT | O_NONBLOCK, 0660, &attr);
  if (rc < 0) {
    perror("mq_open");
    return -1;
  }
  return rc;
}

int gMainMq;

void onSignal(int s) {
  (void)s;
  const char buf[] = "die";
  int rc = mq_send(gMainMq, buf, sizeof(buf), 0);
  if (rc < 0) {
    perror("mq_send");
    return;
  }
}

typedef struct {
  ListenerArgs* args;
  pthread_t id;
  char mqName[128];
} ListenerInfo;

// TODO: use multiple threads for request
// handling(https://idea.popcount.org/2017-02-20-epoll-is-fundamentally-broken-12/)
int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "error: need listeners count\n");
    return 1;
  }
  signal(SIGINT, onSignal);
  signal(SIGSEGV, onSignal);
  signal(SIGABRT, onSignal);
  char mainMqName[MQ_MAX_NAME];
  gMainMq = createMq(-1, "main", mainMqName, 1);
  if (gMainMq < 0) {
    return 1;
  }
  const int listenersSize = atoi(argv[1]);
  ListenerInfo listeners[listenersSize];
  memset(listeners, 0, sizeof(listeners));
  int rc;
  for (int i = 0; i < listenersSize; ++i) {
    int mq = createMq(i, NULL, listeners[i].mqName, 0);
    if (mq < 0) {
      continue;
    }
    listeners[i].args = (ListenerArgs*)malloc(sizeof(ListenerArgs));
    listeners[i].args->mq = mq;
    listeners[i].args->i = i;
    rc = pthread_create(&listeners[i].id, NULL, &onAccept, listeners[i].args);
    if (rc != 0) {
      perror("phread_create");
      continue;
    }
  }
  char recvBuf[256];
  int exit = 0;
  while (!exit) {
    int rc = mq_receive(gMainMq, recvBuf, sizeof(recvBuf), 0);
    if (rc < 0) {
      perror("mq_receive");
      return 1;
    }
    exit = 1;
  }
  const char buf[] = "die";
  for (int i = 0; i < listenersSize; ++i) {
    rc = mq_send(listeners[i].args->mq, buf, sizeof(buf), 0);
    if (rc < 0) {
      perror("mq_send");
      continue;
    }
    rc = pthread_join(listeners[i].id, NULL);
    if (rc != 0) {
      perror("phread_join");
    }
    close(listeners[i].args->mq);
    rc = mq_unlink(listeners[i].mqName);
    if (rc != 0) {
      perror("mq_unlink");
    }
    free(listeners[i].args);
  }
  rc = mq_unlink(mainMqName);
  if (rc != 0) {
    perror("mq_unlink");
  }
  return 0;
}
