#ifndef LIB_INTERVAL_SET_H
#define LIB_INTERVAL_SET_H
#include "cutils/rbtree.h"

struct interval_set_elem {
  struct rbtree_elem rb;
};

typedef int interval_set_cmp_func(struct interval_set_elem const *a,
    struct interval_set_elem const *b, void *aux);
typedef void interval_set_print_func(struct interval_set_elem const *a, void *aux);
typedef void interval_set_action_func(struct interval_set_elem *a,
    void *aux);

struct interval_set {
  struct rbtree tree;
  interval_set_cmp_func *cmp;   /* returns 0 if overlapping intervals. -1 or +1 ow. */
  interval_set_action_func *free;
  void *aux;
};

/* Converts pointer to interval set element INTERVAL_SET_ELEM into a pointer to
 * the structure that INTERVAL_SET_ELEM is embedded inside.  Supply the
 * name of the outer structure STRUCT and the member name MEMBER
 * of the hash element.  See list.h for an example. */
#define interval_set_entry(INTERVAL_SET_ELEM, STRUCT, MEMBER)               \
  ((STRUCT *) ((uint8_t *) &(INTERVAL_SET_ELEM)->rb.parent                  \
    - offsetof (STRUCT, MEMBER.rb.parent)))

void interval_set_init(struct interval_set *set, interval_set_cmp_func *cmp, void *aux);
void interval_set_insert(struct interval_set *set, struct interval_set_elem *elem);
void interval_set_delete(struct interval_set *set, struct interval_set_elem *elem);
void interval_set_destroy(struct interval_set *set,
    interval_set_action_func *destructor);

struct interval_set_elem *interval_set_find(struct interval_set const *set,
    struct interval_set_elem const *elem);
struct interval_set_elem *interval_set_lower_bound(struct interval_set const *set,
    struct interval_set_elem const *elem);
struct interval_set_elem *interval_set_upper_bound(struct interval_set const *set,
    struct interval_set_elem const *elem);

/* interval_set traversal. */
struct interval_set_elem *interval_set_begin (struct interval_set const *);
struct interval_set_elem *interval_set_next (struct interval_set_elem const *);
struct interval_set_elem *interval_set_end (struct interval_set const *);

struct interval_set_elem *interval_set_rbegin (struct interval_set const *);
struct interval_set_elem *interval_set_rnext (struct interval_set_elem const *);
struct interval_set_elem *interval_set_rend (struct interval_set const *);

void interval_set_iterate_unordered(struct interval_set *,
    void (*func)(struct interval_set_elem *, void *));


#endif /* lib/interval_set.h */
