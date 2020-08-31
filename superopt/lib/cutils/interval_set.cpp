#include "cutils/interval_set.h"
#include <stdint.h>
#include "cutils/rbtree.h"


static bool
rbtree_less(struct rbtree_elem const *_a, struct rbtree_elem const *_b, void *aux)
{
  struct interval_set *set = (struct interval_set *)aux;
  struct interval_set_elem *a, *b;
  int cmp;
  a = rbtree_entry(_a, struct interval_set_elem, rb);
  b = rbtree_entry(_b, struct interval_set_elem, rb);

  cmp = (*set->cmp)(a, b, set->aux);
  if (cmp < 0) {
    return true;
  }
  return false;
}

static void
rbtree_free(struct rbtree_elem *_a, void *aux)
{
  struct interval_set *set = (struct interval_set *)aux;
  struct interval_set_elem *a;
  a = rbtree_entry(_a, struct interval_set_elem, rb);
  (*set->free)(a, set->aux);
}

void
interval_set_init(struct interval_set *set, interval_set_cmp_func *cmp, void *aux)
{
  set->cmp = cmp;
  set->aux = aux;
  rbtree_init(&set->tree, rbtree_less, set);
}

void
interval_set_destroy(struct interval_set *set, interval_set_action_func *destructor)
{
  set->free = destructor;
  rbtree_destroy(&set->tree, rbtree_free);
}

#define VOID_FUNC(func)                                                            \
void interval_set_##func(struct interval_set *set, struct interval_set_elem *elem) \
{                                                                                  \
  rbtree_##func(&set->tree, &elem->rb);                                            \
}

VOID_FUNC(insert)
VOID_FUNC(delete)

#define RET_ELEM_FUNC(func)                                                        \
struct interval_set_elem *                                                         \
interval_set_##func(struct interval_set const *set,                                \
    struct interval_set_elem const *elem)                                          \
{                                                                                  \
  struct rbtree_elem *found;                                                       \
  found = rbtree_##func(&set->tree, &elem->rb);                                    \
  if (!found) {                                                                    \
    return NULL;                                                                   \
  }                                                                                \
  return rbtree_entry(found, struct interval_set_elem, rb);                        \
}

RET_ELEM_FUNC(find)
RET_ELEM_FUNC(lower_bound)
RET_ELEM_FUNC(upper_bound)

#if 0
/* interval_set traversal. */
struct interval_set_elem *interval_set_begin (struct interval_set const *);
struct interval_set_elem *interval_set_next (struct interval_set_elem const *);
struct interval_set_elem *interval_set_end (struct interval_set const *);

struct interval_set_elem *interval_set_rbegin (struct interval_set const *);
struct interval_set_elem *interval_set_rnext (struct interval_set_elem const *);
struct interval_set_elem *interval_set_rend (struct interval_set const *);

void interval_set_iterate_unordered(struct interval_set *,
    void (*func)(struct interval_set_elem *, void *));

#endif

