#ifndef __EPOLLSERVER_SERVER_SERVER_H__
#define __EPOLLSERVER_SERVER_SERVER_H__

#include <pthread.h>

typedef struct {
  int i;
  int mq;
} ServerListenerArgs;

typedef struct {
  ServerListenerArgs* args;
  pthread_t id;
  char mqName[128];
} ServerListener;

typedef struct {
  ServerListener* listeners;
} Server;

void server_new(size_t n);
void server_delete();
void server_start();
void server_stop();

#endif
