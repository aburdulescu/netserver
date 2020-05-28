#ifndef __EPOLLSERVER_INTVECTOR_NET_H__
#define __EPOLLSERVER_INTVECTOR_NET_H__

#include <stddef.h>

typedef struct {
  int* data;
  size_t len;
  size_t cap;
} IntVector;

void intvector_new(IntVector* l, size_t cap);
void intvector_delete(const IntVector* l);
void intvector_insert(IntVector* l, int v);
int intvector_at(const IntVector* l, int i);
int* intvector_find(const IntVector* l, int v);

#endif
