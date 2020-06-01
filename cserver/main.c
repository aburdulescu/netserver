#include <errno.h>
#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.h"
#include "util.h"

static void onSignal(int s);
static int onRequest(int fd);

int gMainMq;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "error: need listeners count\n");
    return 1;
  }
  signal(SIGINT, onSignal);
  signal(SIGSEGV, onSignal);
  signal(SIGABRT, onSignal);
  const char mainMqName[] = "/cserver_mq_main";
  gMainMq = util_createMq(mainMqName, 1);
  if (gMainMq < 0) {
    return 1;
  }
  Server s;
  server_new(&s, atoi(argv[1]), onRequest);
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
  close(gMainMq);
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

static int onRequest(int fd) {
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
