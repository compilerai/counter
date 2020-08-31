#ifndef PC_ELEM_H
#define PC_ELEM_H
#include <support/types.h>
#include "rbtree.h"

typedef struct pc_elem_t {
  src_ulong pc;

  struct rbtree_elem rb_elem;
} pc_elem_t;

bool pc_less(struct rbtree_elem const *a, struct rbtree_elem const *b, void *aux);
void pc_elem_free(struct rbtree_elem *e, void *aux);


#endif
