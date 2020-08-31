#ifndef ADDR_RANGE_H
#define ADDR_RANGE_H
#include <support/types.h>
#include "interval_set.h"

typedef struct addr_range_t {
  src_ulong start;
  src_ulong end;

  struct interval_set_elem iset_elem;
} addr_range_t;

int addr_range_cmp(struct interval_set_elem const *a,
    struct interval_set_elem const *b, void *aux);
void addr_range_free(struct interval_set_elem *a, void *aux);

void addr_range_print(struct interval_set_elem const *a, void *aux);

#endif
