#include <map>
#include <iostream>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "config-host.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "ppc/regs.h"

#include "rewrite/peephole.h"
//#include "regset.h"

#include "ppc/insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "valtag/memset.h"
#include "valtag/transmap.h"
#include "rewrite/transmap_set.h"
#include "rewrite/transmap_genset.h"
#include "support/c_utils.h"
#include "support/debug.h"

static int
int_compare(const void *_a, const void *_b)
{
  int a = *(int *)_a;
  int b = *(int *)_b;

  return (b - a); /* sort in descending order */
}

/*
void
transmap_set_row_get_fullrep_regset (transmap_set_elem_t const *head,
    ppc_regset_t *fullrep)
{
  ppc_regset_clear(fullrep);

  transmap_set_elem_t const *cur;

  int num_zeros[PPC_NUM_REGS];
  int num_ones[PPC_NUM_REGS];
  memset(num_zeros, 0x0, sizeof num_zeros);
  memset(num_ones, 0x0, sizeof num_ones);

  for (cur = head; cur; cur=cur->next)
  {
    int i;
    for (i = 0; i < PPC_NUM_REGS; i++)
    {
      if (!cur->tmap->table[i].in_memory)
      {
        num_ones[i]++;
      }
      else
      {
        num_zeros[i]++;
      }
    }
  }

  int i;
  int num_occurrences[PPC_NUM_REGS];
  for (i = 0; i < PPC_NUM_REGS; i++)
  {
    //dbg ("num_zeros[%d]=%d, num_ones[%d]=%d\n",i,num_zeros[i],i,num_ones[i]);
    num_occurrences[i] = MIN(num_zeros[i], num_ones[i]);
  }

  int sorted_num_occurrences[PPC_NUM_REGS];
  memcpy(sorted_num_occurrences, num_occurrences, sizeof num_occurrences);

  qsort(sorted_num_occurrences, PPC_NUM_REGS, sizeof(int), &int_compare);

  if (sorted_num_occurrences[0] == 0)
    return;

  int threshold = -1;
  int num_regs = 0;
  for (i = 0; i < PPC_NUM_REGS; i++)
  {
    int n = log_base2(sorted_num_occurrences[i]);
    if (sorted_num_occurrences[n] >= (1<<n))
    {
      threshold = sorted_num_occurrences[n];
      num_regs = n + 1;
    }
  }
  assert (threshold != -1);

  int num_marks = 0;
  for (i = 0; i < PPC_NUM_REGS && num_marks < num_regs; i++);
  {
    if (num_occurrences[i] >= threshold)
    {
      ppc_regset_mark(fullrep, i);
      num_marks++;
    }
  }
}

int transmap_set_dirtyness_clear_winner (transmap_set_elem_t * const *row,
    int regno, int *dirty)
{
  int i;
  int min_cost = COST_INFINITY;
  *dirty = 0;

  for (i = 0; i < i386_num_available_regs; i++)
  {
    transmap_set_elem_t *cur;
    for (cur=row[i]; cur; cur=cur->next)
    {
      if (min_cost > cur->cost && !cur->tmap->table[regno].in_memory)
      {
	min_cost = cur->cost;
	if (cur->tmap->table[regno].reg.dirty)
	{
	  *dirty = 1;
	}
	else
	{
	  *dirty = 0;
	}
      }
    }
  }
  return 1;
}
*/

void
transmap_set_elem_free(transmap_set_elem_t *cur)
{
  transmap_free(cur->in_tmap);
  if (cur->out_tmaps) {
    //transmap_free(cur->out_tmaps);
    delete [] cur->out_tmaps;
    cur->out_tmaps = NULL;
  }
  if (cur->decisions) {
    free(cur->decisions);
    cur->decisions = NULL;
  }
  /*if (cur->in_tcons) {
    free(cur->in_tcons);
  }*/
  /* to avoid dangling pointers */
  cur->in_tmap = NULL;
  cur->out_tmaps = NULL;
  cur->decisions = NULL;
  cur->next = NULL;
  //cur->in_tcons = NULL;

  free(cur);
}

void
free_transmap_set_row(transmap_set_elem_t *head)
{
  transmap_set_elem_t *cur, *next;

  if (!head) {
    return;
	}
  for (cur = head, next = cur->next; cur; cur = next, next = cur?cur->next:NULL) {
    transmap_set_elem_free(cur);
  }
}

/*
bool
transmap_set_dirty_bit_appears(pred_transmap_set_t const *pred_set, int regno)
{
  pred_transmap_set_t const *pred;

  for (pred = pred_set; pred; pred = pred->next) {
    transmap_set_elem_t const *cur;
    for (cur = pred->list; cur; cur = cur->next) {
      if (   cur->in_tmap->table[regno].loc == TMAP_LOC_REG
          && cur->in_tmap->table[regno].reg.dirty) {
        return true;
      }
    }
  }
  return false;
}

bool
transmap_set_clean_bit_appears(pred_transmap_set_t const *pred_set, int regno)
{
  pred_transmap_set_t const *pred;

  for (pred = pred_set; pred; pred = pred->next) {
    transmap_set_elem_t const *cur;
    for (cur = pred->list; cur; cur = cur->next) {
      if (   cur->in_tmap->table[regno].loc == TMAP_LOC_MEM
          || !cur->in_tmap->table[regno].reg.dirty)
        return true;
    }
  }
  return false;
}
*/

/* This function returns:
   -1: if regno is never flag-allocated in prev-row
    0: if regno is always flag-allocated in signed form (i.e. regnum == 0)
    1: if regno is always flag-allocated in unsigned form (i.e. regnum == 1)
    7: if regno is flag-allocated sometimes in signed and sometimes in unsigned
       form
 */
int
transmap_set_get_crf_sign_ov(pred_transmap_set_t const *pred_set, int regno,
    int sign_ov)
{
  NOT_IMPLEMENTED();
#if 0
  pred_transmap_set_t const *pred;
  long i;

#if ARCH_SRC == ARCH_PPC
  //ASSERT (regno >= PPC_REG_CRFN(0) && regno < PPC_REG_CRFN(8));
#endif

  for (pred = pred_set; pred; pred = pred->next) {
    transmap_set_elem_t const *cur;
    for (cur = pred->list; cur; cur = cur->next) {
      for (i = 0; i < cur->row->num_nextpcs; i++) {
        if (cur->out_tmaps[i].table[regno].loc == TMAP_LOC_FLAGS) {
          ASSERT(cur->out_tmaps[i].table[regno].reg.rf.sign_ov != -1);
          int new_sign_ov = cur->out_tmaps[i].table[regno].reg.rf.sign_ov;

          DBG (ENUM_TRANS, "sign_ov = %d, new_sign_ov = %d\n", sign_ov,
              new_sign_ov);
          if (sign_ov == -1) {
            sign_ov = new_sign_ov;
          } else if (sign_ov != new_sign_ov)
            return 2;//XXX 7;
        }
      }
    }
  }
  return sign_ov;
#endif
}

/*
transmap_set_elem_t *
transmap_set_union(transmap_set_elem_t *set, transmap_set_elem_t *new_set)
{
  transmap_set_elem_t *cur, *prev, *next, *tail, *ret;
  transmap_genset_t *tmaps_genset;

  transmap_genset_init(tmaps_genset);
  for (prev = NULL, cur = set; cur; prev = cur, cur = cur->next) {
    transmap_genset_add(tmaps_genset, cur->in_tmap);
  }

  tail = prev;
  ret = set;

  for (cur = new_set; cur; cur = next) {
    ASSERT(!cur->out_tmaps);
    next = cur->next;
    if (!transmap_genset_belongs(tmaps_genset, cur->in_tmap)) {
      if (!tail) {
        ASSERT(!ret);
        ret = cur;
        tail = cur;
        tail->next = NULL;
      } else {
        tail->next = cur;
        tail = tail->next;
        tail->next = NULL;
      }
    } else {
      transmap_set_elem_free (cur);
    }
  }

  transmap_genset_free(tmaps_genset);
  return ret;
}

void transmap_set_row_tmaps_copy (transmap_set_elem_t **out,
    transmap_set_elem_t const *in)
{
  transmap_set_elem_t *cur = NULL;
  if (in)
  {
    cur = (transmap_set_elem_t *)malloc(sizeof(transmap_set_elem_t));
    assert(cur);
    transmap_init (&cur->tmap);
    transmap_copy (cur->tmap, in->tmap);
    cur->decisions = NULL;
    cur->next = NULL;
    *out = cur;
  }
  else
  {
    *out = NULL;
    return;
  }

  for (in = in->next; in; in = in->next)
  {
    cur->next = (transmap_set_elem_t *)malloc(sizeof(transmap_set_elem_t));
    assert(cur->next);
    cur = cur->next;
    transmap_init(&cur->tmap);
    transmap_copy(cur->tmap, in->tmap);
    cur->decisions = NULL;
    cur->next = NULL;
  }
}

void transmap_set_tmaps_copy (transmap_set_t *out, transmap_set_t const *in)
{
  int i;
  for (i = 0; i < SRC_MAX_ILEN; i++) {
    transmap_set_row_tmaps_copy (&out->head[i], in->head[i]);
  }
}

void transmap_set_cartesian_product (transmap_set_t *out,
    transmap_set_t const *tmap_sets, int num_tmap_sets)
{
  if (num_tmap_sets == 0) {
    int i;
    for (i = 0; i < SRC_MAX_ILEN; i++) {
      out->head[i] = NULL;
    }
    return;
  }

  transmap_set_tmaps_copy (out, &tmap_sets[0]);
  //XXX: do this efficiently and precisely

  int i;
  for (i = 1; i < num_tmap_sets; i++)
  {
    //transmap_set_cartesian_product2 (out, &tmap_sets[i]);
  }
}
*/

long
transmap_set_count(transmap_set_elem_t const *set)
{
  int count = 0;
  transmap_set_elem_t const *cur;
  for (cur = set; cur; cur=cur->next) {
    count++;
  }
  return count;
}
