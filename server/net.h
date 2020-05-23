#ifndef __EPOLLSERVER_SERVER_NET_H__
#define __EPOLLSERVER_SERVER_NET_H__

typedef struct {
  int fd;
} NetConn;

typedef struct {
  int fd;
} NetListener;

int net_listen(NetListener* l, char* protocol, char* addr, char* port, int backlog);
int net_accept(const NetListener* l, NetConn* c);

#endif
