#include "net.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int net_listen(NetListener* l, char* protocol, char* addr, char* port, int backlog) {
  int rc;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  if (strcmp(protocol, "tcp") == 0) {
    hints.ai_socktype = SOCK_STREAM;
  } else if (strcmp(protocol, "udp") == 0) {
    hints.ai_socktype = SOCK_DGRAM;
  } else {
    fprintf(stderr, "unknown protocol %s\n", protocol);
    return -1;
  }
  hints.ai_flags = AI_PASSIVE;
  struct addrinfo* servinfo;
  rc = getaddrinfo(addr, port, &hints, &servinfo);
  if (rc != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
    return 1;
  }
  struct addrinfo* p;
  for (p = servinfo; p != NULL; p = p->ai_next) {
    l->fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (l->fd == -1) {
      perror("socket");
      continue;
    }
    int yes = 1;
    rc = setsockopt(l->fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int));
    if (rc < 0) {
      perror("setsockopt");
      continue;
    }
    rc = bind(l->fd, p->ai_addr, p->ai_addrlen);
    if (rc < 0) {
      close(l->fd);
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
  if (listen(l->fd, backlog) == -1) {
    perror("listen");
    return -1;
  }
  return 0;
}

int net_accept(const NetListener* l, NetConn* c) {
  struct sockaddr_storage addr;
  socklen_t addrlen = sizeof(addr);
  c->fd = accept(l->fd, (struct sockaddr*)&addr, &addrlen);
  if (c->fd < 0) {
    perror("accept");
    return -1;
  }
  int rc = fcntl(c->fd, F_SETFL, fcntl(c->fd, F_GETFL, 0) | O_NONBLOCK);
  if (rc < 0) {
    perror("fcntl");
    return -1;
  }
  return 0;
}
