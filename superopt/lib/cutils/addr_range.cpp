#include <stdio.h>
#include <stdlib.h>
#include "cutils/addr_range.h"
#include "cutils/interval_set.h"

int
addr_range_cmp(struct interval_set_elem const *a,
    struct interval_set_elem const *b, void *aux)
{
  struct addr_range_t *e1, *e2;

  e1 = interval_set_entry(a, struct addr_range_t, iset_elem);
  e2 = interval_set_entry(b, struct addr_range_t, iset_elem);

  //XXX: shouldn't <= be replaced by <
  if (e1->end <= e2->start) {
    return -1;
  } else if (e2->end <= e1->start) {
    return 1;
  }
  return 0;
}

void
addr_range_free(struct interval_set_elem *a, void *aux)
{
  struct addr_range_t *e;
  e = interval_set_entry(a, struct addr_range_t, iset_elem);
  free(e);
}

void
addr_range_print(struct interval_set_elem const *a, void *aux)
{
  struct addr_range_t *e1;

  e1 = interval_set_entry(a, struct addr_range_t, iset_elem);
  printf(" (0x%x,0x%x)", e1->start, e1->end);
}
