#include "intvector.h"

#include <stdlib.h>
#include <string.h>

void intvector_new(IntVector* l, size_t cap) {
  l->cap = (cap == 0) ? 1 : cap;
  l->len = 0;
  l->data = (int*)malloc(sizeof(int) * cap);
}

void intvector_delete(const IntVector* l) {
  free(l->data);
}

void intvector_insert(IntVector* l, int v) {
  if (l->len == l->cap) {
    l->cap *= 2;
    int* newData = (int*)malloc(sizeof(int) * l->cap);
    memcpy(newData, l->data, l->len * sizeof(int));
    free(l->data);
    l->data = newData;
    return;
  }
  l->data[l->len] = v;
  ++l->len;
}

int intvector_at(const IntVector* l, int i) {
  if (i < 0 && i >= l->len) {
    return -1;
  }
  return l->data[i];
}

int* intvector_find(const IntVector* l, int v) {
  for (int i = 0; i < l->len; ++i) {
    if (l->data[i] == v) {
      return l->data + i;
    }
  }
  return NULL;
}
