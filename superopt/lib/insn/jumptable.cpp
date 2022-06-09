#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "config-host.h"
//#include "cpu-all.h"
#include "cutils/pc_elem.h"
#include "insn/jumptable.h"
#include "support/debug.h"
#include <elf.h>
//#include "exec-all.h"
#include "rewrite/translation_instance.h"

typedef struct jumptable_t
{
  struct rbtree jumptarget_set;
} jumptable_t;

void
jumptable_init(input_exec_t *in)
{
  int i;
  in->jumptable = (jumptable_t *)malloc(sizeof(jumptable_t));
  ASSERT(in->jumptable);
  rbtree_init(&in->jumptable->jumptarget_set, pc_less, NULL);
}

static void
jumptarget_set_elem_free(struct rbtree_elem *a, void *aux)
{
  struct pc_elem_t *elem = rbtree_entry(a, struct pc_elem_t, rb_elem);
  free(elem);
}

void jumptable_free(jumptable_t *jtable)
{
  ASSERT(jtable);
  rbtree_destroy(&jtable->jumptarget_set, jumptarget_set_elem_free);
  free(jtable);
}

void
jumptable_add(jumptable_t *jtable, long src_addr)
{
  struct pc_elem_t *elem, needle;
  needle.pc = src_addr;
  if (!rbtree_find(&jtable->jumptarget_set, &needle.rb_elem)) {
    elem = (struct pc_elem_t *)malloc(sizeof *elem);
    elem->pc = src_addr;
    rbtree_insert(&jtable->jumptarget_set, &elem->rb_elem);
  }
}

bool
jumptable_belongs(jumptable_t const *jtable, long src_addr)
{
  struct pc_elem_t elem;
  elem.pc = src_addr;

  if (rbtree_find(&jtable->jumptarget_set, &elem.rb_elem)) {
    return true;
  } else {
    return false;
  }
}

void *
jumptable_iter_begin(jumptable_t const *jtable, src_ulong *addr)
{
  struct rbtree_elem *iter;
  iter = rbtree_begin(&jtable->jumptarget_set);
  if (iter) {
    struct pc_elem_t *elem = rbtree_entry(iter, struct pc_elem_t, rb_elem);
    *addr = elem->pc;
  }
  return iter;
}

void *
jumptable_iter_next(jumptable_t const *jtable, void *iter, src_ulong *addr)
{
  struct rbtree_elem *next;
  next = (struct rbtree_elem *)rbtree_next((struct rbtree_elem const *)iter);
  if (next) {
    struct pc_elem_t *elem = rbtree_entry(next, struct pc_elem_t, rb_elem);
    *addr = elem->pc;
  }
  return next;
}

int
jumptable_count(struct jumptable_t const *jtable)
{
  void *iter;
  src_ulong src_addr;
  int ret = 0;
  for (iter = jumptable_iter_begin(jtable, &src_addr); iter;
      iter = jumptable_iter_next(jtable, iter, &src_addr)) {
    ret++;
  }
  return ret;
}
