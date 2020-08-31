#include <stdlib.h>
#include <string.h>
#include "cutils/pc_elem.h"
#include "cutils/rbtree.h"

bool
pc_less(struct rbtree_elem const *a, struct rbtree_elem const *b, void *aux)
{
  struct pc_elem_t *e1, *e2;
  //int ret;

  e1 = rbtree_entry(a, struct pc_elem_t, rb_elem);
  e2 = rbtree_entry(b, struct pc_elem_t, rb_elem);

  if (e1->pc < e2->pc) {
    return true;
  }
  return false;
}

void
pc_elem_free(struct rbtree_elem *e, void *aux)
{
  struct pc_elem_t *te;
  te = rbtree_entry(e, struct pc_elem_t, rb_elem);
  free(te);
}
