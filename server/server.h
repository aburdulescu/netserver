#ifndef __EPOLLSERVER_SERVER_SERVER_H__
#define __EPOLLSERVER_SERVER_SERVER_H__

#include <pthread.h>
#include <stdint.h>

typedef struct {
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

void server_new(Server* s, size_t n);
void server_delete();
int server_start();
void server_stop();

#endif
