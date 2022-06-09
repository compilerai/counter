#include <map>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include "support/debug.h"
#include "support/c_utils.h"
#include "insn/rdefs.h"
#include "ppc/insn.h"
#include "valtag/regset.h"
#include "config-host.h"

static char as1[4096], as2[4096];

void
rdefs_init (rdefs_t *rdefs)
{
  NOT_IMPLEMENTED();
  /*int i;
  for (i = 0; i < SRC_NUM_REGS; i++) {
    rdefs->table[i] = NULL;
  }*/
}

void
rdefs_free (rdefs_t *rdefs)
{
  NOT_IMPLEMENTED();
  /*int i;
  for (i = 0; i < SRC_NUM_REGS; i++) {
    list_node_t *cur = rdefs->table[i];
    while (cur) {
      list_node_t *prev = cur->next;
      free(cur);
      cur = prev;
    }
    rdefs->table[i] = NULL;
  }*/
}

static int
rdefs_is_empty(rdefs_t const *rdefs)
{
  NOT_IMPLEMENTED();
  /*int i;
  for (i = 0; i < SRC_NUM_REGS; i++)
  {
    if (rdefs->table[i])
      return 0;
  }
  return 1;*/
}

void
rdefs_copy(rdefs_t *dst, rdefs_t const *src)
{
  NOT_IMPLEMENTED();
  /*int i;
  rdefs_free(dst);
  for (i = 0; i < SRC_NUM_REGS; i++) {
    list_node_t const *cur = src->table[i];
    list_node_t *dst_end = dst->table[i];
    while (cur) {
      list_node_t *new_node = (list_node_t *)malloc(sizeof(list_node_t));
      assert(new_node);
      new_node->pc = cur->pc;
      new_node->next = NULL;

      if (!dst_end) {
        dst->table[i] = new_node;
      } else {
        dst_end->next = new_node;
      }

      dst_end = new_node;
      cur = cur->next;
    }
  }*/
}

void
rdefs_union (rdefs_t *dst, rdefs_t const *src)
{
  NOT_IMPLEMENTED();
  /*int i;
  if (!src)
    return;
  for (i = 0; i < SRC_NUM_REGS; i++) {
    list_node_t *cur_dst = dst->table[i];
    list_node_t const *cur_src = src->table[i];
    list_node_t *prev_dst = NULL;
    while (cur_dst || cur_src) {
      if (!cur_dst || (cur_src && cur_src->pc < cur_dst->pc)) {
        list_node_t *new_node = (list_node_t *)malloc(sizeof(list_node_t));
        assert(new_node);
        new_node->pc = cur_src->pc;
        new_node->next = cur_dst;

        if (!prev_dst) {
          dst->table[i] = new_node;
        } else {
          prev_dst->next = new_node;
        }
        prev_dst = new_node;
        cur_src = cur_src->next;
      } else {
        assert(cur_dst);
        if (cur_src && cur_src->pc == cur_dst->pc) {
          cur_src = cur_src->next;
        }
        prev_dst = cur_dst;
        cur_dst = cur_dst->next;
      }
    }
  }*/
}

void
rdefs_diff(rdefs_t *dst, rdefs_t const *src)
{
  NOT_IMPLEMENTED();
  /*int i;
  if (!src)
    return;
  for (i = 0; i < SRC_NUM_REGS; i++) {
    list_node_t *cur_dst = dst->table[i];
    list_node_t const *cur_src = src->table[i];
    list_node_t *prev_dst = NULL;
    while (cur_dst && cur_src) {
      if (cur_dst->pc == cur_src->pc) {
        list_node_t *free_node = cur_dst;
        cur_src = cur_src->next;
        cur_dst = cur_dst->next;
        if (!prev_dst) {
          dst->table[i] = cur_dst;
        } else {
          prev_dst->next = cur_dst;
        }
        free(free_node);
      } else if (cur_src->pc < cur_dst->pc) {
        cur_src = cur_src->next;
      } else if (cur_src->pc > cur_dst->pc) {
        prev_dst = cur_dst;
        cur_dst = cur_dst->next;
      }
    }
  }*/
}

char *rdefs_to_string(rdefs_t *rdefs, regset_t *use_regs,
    char *buf, int buf_size)
{
  NOT_IMPLEMENTED();
  /*char *ptr = buf, *end = buf+buf_size;
  *ptr = '\0';
  int i;
  int nprints = 0;
  for (i = 0; i < SRC_NUM_REGS; i++) {
    if (regset_belongs(use_regs, i)) {
      char tmp[16];
      src_regname(i,tmp,sizeof tmp);
      ptr += snprintf(ptr, end-ptr, "%c%s", nprints++?',':' ', tmp);
      list_node_t const *cur = rdefs->table[i];
      if (!cur) ptr += snprintf(ptr, end-ptr, "[none");
      int num_pcs = 0;
      while (cur)
      {
        ptr += snprintf(ptr, end-ptr, "%c%x", num_pcs++?',':'[', cur->pc);
        cur=cur->next;
      }
      ptr += snprintf (ptr, end-ptr, "]");
    }
  }
  return buf;*/
}

void
rdefs_register (rdefs_t *rdefs, regset_t const *regs,
    int32_t pc)
{
  NOT_IMPLEMENTED();
  /*int reg;
  for (reg = 0; reg < SRC_NUM_REGS; reg++) {
    if (!regset_belongs(regs, reg)) {
      continue;
    }
    assert(reg >= 0 && reg < SRC_NUM_REGS); 
    list_node_t *cur = rdefs->table[reg];
    list_node_t *prev = NULL;

    while (cur && cur->pc < pc) {
      prev = cur;
      cur = cur->next;
    }

    if (cur && cur->pc == pc) {
      continue;
    }
    assert(!cur || cur->pc > pc);

    list_node_t *new_node = (list_node_t *)malloc(sizeof(list_node_t));
    assert(new_node);
    new_node->pc = pc;
    new_node->next = cur;

    if (prev) {
      prev->next = new_node;
    } else {
      rdefs->table[reg] = new_node;
    }
  }*/
}

void
rdefs_unregister(rdefs_t *rdefs, int reg, int32_t pc)
{
  NOT_IMPLEMENTED();
  /*assert(reg >= 0 && reg < SRC_NUM_REGS); 
  list_node_t *cur = rdefs->table[reg];
  list_node_t *prev = NULL;

  while (cur && cur->pc < pc) {
    prev = cur;
    cur = cur->next;
  }

  if (!cur || cur->pc > pc)
    return;

  assert(cur && cur->pc == pc);

  if (prev)
    prev->next = cur->next;
  else
    rdefs->table[reg] = cur->next;

  free(cur);*/
}

void
rdefs_clear(rdefs_t *rdefs, regset_t const *regs)
{
  NOT_IMPLEMENTED();
  /*int reg;
  for (reg = 0; reg < SRC_NUM_REGS; reg++) {
    if (regset_belongs(regs, reg)) {
      list_node_t *cur = rdefs->table[reg];
      while (cur) {
        list_node_t *next = cur->next;
        free(cur);
        cur = next;
      }
      rdefs->table[reg] = NULL;
    }
  }*/
}

int
rdefs_equal(rdefs_t const *rdefs1, rdefs_t const *rdefs2)
{
  NOT_IMPLEMENTED();
  /*int i;
  for (i = 0; i < SRC_NUM_REGS; i++) {
    list_node_t *cur1 = rdefs1->table[i];
    list_node_t *cur2 = rdefs2->table[i];

    while (cur1 && cur2) {
      if (cur1->pc != cur2->pc)
        return 0;
      cur1=cur1->next;
      cur2=cur2->next;
    }
    if (cur1 || cur2)
      return 0;
  }
  return 1;*/
}

/* for debugging only */
static void
rdefs_traverse(rdefs_t const *rdefs)
{
  NOT_IMPLEMENTED();
  /*int i;
  for (i = 0; i < SRC_NUM_REGS; i++) {
    list_node_t *cur = rdefs->table[i];
    for (cur = rdefs->table[i]; cur; cur=cur->next);
  }*/
}
