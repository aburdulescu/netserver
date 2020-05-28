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

#include "intvector.h"
#include "net.h"
#include "server.h"

static const size_t kMqMaxName = 128;
static const char kMqNamePrefix[] = "/epolls_mq_";
static const size_t kMqNamePrefixSize = sizeof(kMqNamePrefix) - 1;

int gMainMq;
static int createMq(int i, char* mqSuffix, char* mqName, int block);
static void onSignal(int s);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "error: need listeners count\n");
    return 1;
  }
  signal(SIGINT, onSignal);
  signal(SIGSEGV, onSignal);
  signal(SIGABRT, onSignal);
  char mainMqName[kMqMaxName];
  gMainMq = createMq(-1, "main", mainMqName, 1);
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

static int createMq(int i, char* mqSuffix, char* mqName, int block) {
  memset(mqName, 0, kMqMaxName);
  strncpy(mqName, kMqNamePrefix, kMqNamePrefixSize);
  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = 256;
  attr.mq_curmsgs = 0;
  int prefixEnd = kMqNamePrefixSize;
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

static void onSignal(int s) {
  (void)s;
  const char buf[] = "die";
  int rc = mq_send(gMainMq, buf, sizeof(buf), 0);
  if (rc < 0) {
    perror("mq_send");
    return;
  }
}
