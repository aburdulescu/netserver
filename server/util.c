#include "util.h"

#include <mqueue.h>
#include <stdio.h>

int util_createMq(const char* mqName, int isBlocking) {
  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = 256;
  attr.mq_curmsgs = 0;
  int rc;
  if (isBlocking) {
    rc = mq_open(mqName, O_RDWR | O_CREAT, 0660, &attr);
  } else {
    rc = mq_open(mqName, O_RDWR | O_CREAT | O_NONBLOCK, 0660, &attr);
  }
  if (rc < 0) {
    perror("mq_open");
    return -1;
  }
  return rc;
}
