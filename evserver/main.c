#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "ev.h"
#include "net.h"

static void onRead(struct ev_loop* loop, struct ev_io* w, int revents) {
  if (EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }

  const size_t bSize = 1024;
  char b[bSize];

  int rc = recv(w->fd, b, bSize, 0);
  if (rc < 0) {
    perror("read error");
    return;
  }

  if (rc == 0) {
    ev_io_stop(loop, w);
    free(w);
    printf("%d: disconnected!\n", w->fd);
    return;
  }

  b[rc] = '\0';

  printf("%d says: %s\n", w->fd, b);

  send(w->fd, b, rc, 0);
}

static void onAccept(struct ev_loop* loop, struct ev_io* w, int revents) {
  if (EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }

  NetListener l = {w->fd};
  NetConn conn;
  if (net_accept(&l, &conn) == -1) {
    perror("accept error");
    return;
  }

  printf("%d: connected!\n", conn.fd);

  struct ev_io* w_client = (struct ev_io*)malloc(sizeof(struct ev_io));
  ev_io_init(w_client, onRead, conn.fd, EV_READ);
  ev_io_start(loop, w_client);
}

int main(void) {
  NetListener l;
  int rc = net_listen(&l, "tcp", NULL, "55443", 100);
  if (rc < 0) {
    fprintf(stderr, "net_listen: socket_open failed\n");
    return 1;
  }

  printf("listen on port 55443\n");

  struct ev_loop* loop = EV_DEFAULT;

  ev_io w;
  ev_io_init(&w, onAccept, l.fd, EV_READ);
  ev_io_start(loop, &w);

  ev_loop(loop, 0);

  return 0;
}
