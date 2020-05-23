#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static const int MAX_EVENTS = 10;
static const int LISTEN_BACKLOG = 100;

void setnonblocking(int fd) {
}

void do_use_fd(int fd) {
}

typedef struct {
  int fd;
} TCPConn;

int tcp_listen(TCPConn* c, char* addr, char* port) {
  int yes = 1;
  int rc;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  struct addrinfo* servinfo;
  rc = getaddrinfo(addr, port, &hints, &servinfo);
  if (rc != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
    return 1;
  }
  struct addrinfo* p;
  for (p = servinfo; p != NULL; p = p->ai_next) {
    c->fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (c->fd == -1) {
      perror("socket");
      continue;
    }
    rc = setsockopt(c->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (rc < 0) {
      perror("setsockopt");
      continue;
    }
    rc = bind(c->fd, p->ai_addr, p->ai_addrlen);
    if (rc < 0) {
      close(c->fd);
      perror("server: bind");
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);
  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return -1;
  }
  if (listen(c->fd, LISTEN_BACKLOG) == -1) {
    perror("listen");
    return -1;
  }
  return 0;
}

int tcp_accept(const TCPConn* conn, TCPConn* newConn) {
  struct sockaddr_storage addr;
  socklen_t addrlen = sizeof(addr);
  newConn->fd = accept(conn->fd, (struct sockaddr*)&addr, &addrlen);
  if (newConn->fd < 0) {
    perror("accept");
    return -1;
  }
  return 0;
}

int tcp_close(const TCPConn* conn) {
  return close(conn->fd);
}

int main(int argc, char* argv[]) {
  TCPConn conn;
  int rc = tcp_listen(&conn, NULL, "55443");
  if (rc < 0) {
    fprintf(stderr, "server: tcp_listen failed\n");
    return 1;
  }
  int epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("epoll_create1");
    exit(1);
  }
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = conn.fd;
  rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, conn.fd, &ev);
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
      if (events[n].data.fd != conn.fd) {
        do_use_fd(events[n].data.fd);
      }
      TCPConn newConn;
      rc = tcp_accept(&conn, &newConn);
      if (rc == -1) {
        goto error;
      }
      //      setnonblocking(conn_sock);
      ev.events = EPOLLIN | EPOLLET;
      ev.data.fd = newConn.fd;
      if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newConn.fd, &ev) == -1) {
        perror("epoll_ctl: conn_sock");
        goto error;
      }
    }
  }
  return 0;
error:
  tcp_close(&conn);
  return 1;
}
