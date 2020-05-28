#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "server.h"
#include "util.h"

static void onSignal(int s);

int gMainMq;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "error: need listeners count\n");
    return 1;
  }
  signal(SIGINT, onSignal);
  signal(SIGSEGV, onSignal);
  signal(SIGABRT, onSignal);
  const char mainMqName[] = "/epolls_mq_main";
  gMainMq = createMq(mainMqName, 1);
  if (gMainMq < 0) {
    return 1;
  }
  Server s;
  server_new(&s, atoi(argv[1]));
  if (server_start(&s) < 0) {
    return 1;
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
  server_stop(&s);
  server_delete(&s);
  int rc = mq_unlink(mainMqName);
  if (rc != 0) {
    perror("mq_unlink");
  }
  return 0;
}

static void onSignal(int s) {
  (void)s;
  const char buf[] = "die";
  int rc = mq_send(gMainMq, buf, sizeof(buf), 0);
  if (rc < 0) {
    perror("mq_send");
    return;
  }
}
