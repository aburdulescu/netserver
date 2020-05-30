#ifndef __EPOLLSERVER_SERVER_SERVER_H__
#define __EPOLLSERVER_SERVER_SERVER_H__

#include <pthread.h>
#include <stdint.h>

// Returns:
//  0 - Ok
//  1 - Connection closed
typedef int (*ServerRequestHandler)(int fd);

typedef struct {
  ServerRequestHandler onRequest;
  uint64_t i;
  int mq;
} ServerListenerArgs;

typedef struct {
  ServerListenerArgs* args;
  pthread_t id;
  char mqName[128];
} ServerListener;

typedef struct {
  ServerListener* listeners;
  size_t listenersLen;
} Server;

void server_new(Server* s, size_t n, ServerRequestHandler onRequest);
void server_delete(const Server* s);
int server_start(const Server* s);
void server_stop(const Server* s);

#endif
