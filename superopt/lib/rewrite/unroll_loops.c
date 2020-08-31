#include "unroll_loops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include "cpu-defs.h"
#include "support/debug.h"
#include "valtag/elf/elf.h"
#include "rewrite/bbl.h"
#include "rewrite/cfg.h"
//#include "exec-all.h"
#include "rewrite/translation_instance.h"
#include "cutils/rbtree.h"
#include "cutils/pc_elem.h"
#include "cutils/imap_elem.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "valtag/regcons.h"
#include "rewrite/transmap.h"
#include "rewrite/static_translate.h"
#include "support/c_utils.h"
#include "rewrite/jumptable.h"
#include "rewrite/rdefs.h"

int num_loop_unrolls = 8;
static char as1[4096];

typedef struct uloop_t {
  src_ulong induction_reg1, induction_reg2;
  int induction_val1, induction_val2;
  char cmp_operator[32];
  long num_bbls;
  src_ulong head, tail, exit;
  src_ulong *bbl_pcs;
  bool contains_nested_loop;
  bool contains_function_call;
  bool contains_external_branch;
  bool no_induction_variables;
  bool unroll_me;
} uloop_t;

static bool
is_unrollable(uloop_t const *loop)
{
  if (loop->tail == (src_ulong)-1) {
    return false;
  }
  if (loop->contains_external_branch) {
     DBG2(UNROLL_LOOPS, "Loop %lx-%lx contains external branch\n", (long)loop->head,
         (long)loop->tail);
    return false;
  }
  if (loop->contains_nested_loop) {
     DBG2(UNROLL_LOOPS, "Loop %lx-%lx contains nested loop\n", (long)loop->head,
         (long)loop->tail);
    return false;
  }
  if (loop->contains_function_call) {
     DBG2(UNROLL_LOOPS, "Loop %lx-%lx contains function call\n", (long)loop->head,
        (long)loop->tail);
    return false;
  }
  if (loop->no_induction_variables) {
     DBG2(UNROLL_LOOPS, "Loop %lx-%lx has no induction vars\n", (long)loop->head,
        (long)loop->tail);
    return false;
  }
  if (loop->num_bbls == 0) {
    DBG2(UNROLL_LOOPS, "Loop %lx-%lx has no bbls\n", (long)loop->head,
        (long)loop->tail);
    return false;
  }
  return true;
}

static long
identify_reachable_bbls_dfs(input_exec_t const *in, src_ulong *reachable_bbl_pcs,
    long head, src_ulong tailpc, bool *visited)
{
  long i, r, nextbbl;
  bbl_t const *bbl;

  r = 0;
  if (visited[head]) {
    return 0;
  }
  visited[head] = true;
  bbl = &in->bbls[head];
  reachable_bbl_pcs[r++] = bbl->pc;
  if (bbl->pc == tailpc) {
    return r;
  }
  for (i = 0; i < bbl->num_nextpcs; i++) {
    if (   bbl->nextpcs[i] != PC_JUMPTABLE
        && bbl->nextpcs[i] != PC_UNDEFINED
        && bbl->nextpcs[i] != tailpc
        && (nextbbl = pc2bbl(&in->bbl_map, bbl->nextpcs[i])) != (unsigned long)-1) {
      ASSERT(nextbbl >= 0 && nextbbl < in->num_bbls);
      r += identify_reachable_bbls_dfs(in, &reachable_bbl_pcs[r], nextbbl, tailpc,
          visited);
    }
  }
  return r;
}

static src_ulong
decide_inner_loop(input_exec_t const *in, src_ulong head, src_ulong curtail,
    src_ulong newtail)
{
  /* XXX If newtail can reach curtail without going through head, return newtail,
     else curtail. */
  return newtail;
}

static long
identify_reachable_bbls(input_exec_t const *in, src_ulong *reachable_bbl_pcs, long head,
    src_ulong tailpc)
{
  bool visited[in->num_bbls];
  long i;

  for (i = 0; i < in->num_bbls; i++) {
    visited[i] = false;
  }
  return identify_reachable_bbls_dfs(in, reachable_bbl_pcs, head, tailpc, visited);
}

static void
identify_loop_attributes(input_exec_t const *in, uloop_t *loops)
{
  long i;

  for (i = 0; i < in->num_bbls; i++) {
    src_ulong reachable_bbl_pcs[in->num_bbls];
    bbl_t *head;
    long j;

    if (loops[i].tail == (src_ulong)-1) {
      continue;
    }
    DBG2(UNROLL_LOOPS, "identifying reachable bbls for %lx, exit=%lx\n",
        (long)loops[i].head, (long)loops[i].exit);
    loops[i].num_bbls = identify_reachable_bbls(in, reachable_bbl_pcs, i, loops[i].exit);
    for (j = 0; j < loops[i].num_bbls; j++) {
      src_ulong r_pc;
      bbl_t *bbl_r;
      long r;

      r_pc = reachable_bbl_pcs[j];
      if (r_pc == PC_JUMPTABLE) {
        DBG2(UNROLL_LOOPS, "%lx-%lx: contains external branch to indir\n",
            (long)in->bbls[i].pc, (long)loops[i].tail);
        loops[i].contains_external_branch = true;
        break;
      }
      r = pc2bbl(&in->bbl_map, r_pc);
      ASSERT(r >= 0 && r < in->num_bbls);
      bbl_r = &in->bbls[r];
      ASSERT(r >= 0 && r < in->num_bbls);
      if (!pc_dominates(in, in->bbls[i].pc, r_pc)) {
        DBG2(UNROLL_LOOPS, "%lx-%lx: contains external branch to %lx\n",
            (long)in->bbls[i].pc, (long)loops[i].tail, (long)r_pc);
        loops[i].contains_external_branch = true;
        break;
      }
      if (r != i && loops[r].tail != -1) {
        DBG2(UNROLL_LOOPS, "%lx-%lx: contains nested loop at %lx-%lx\n",
            (long)in->bbls[i].pc, (long)loops[i].tail, (long)r_pc,
            (long)loops[r].tail);
        loops[i].contains_nested_loop = true;
        break;
      }
      if (bbl_r->num_fcalls) {
        DBG2(UNROLL_LOOPS, "%lx-%lx: contains function call at %lx\n",
            (long)in->bbls[i].pc, (long)loops[i].tail, (long)r_pc);
        loops[i].contains_function_call = true;
        break;
      }
    }
    if (!is_unrollable(&loops[i])) {
      continue;
    }
    for (j = 0; j < loops[i].num_bbls; j++) {
      src_ulong nreachable_bbl_pcs[in->num_bbls];
      bool can_reach_tail;
      long nreach, nr, r;
      src_ulong r_pc;
      bbl_t *tb_r;

      r_pc = reachable_bbl_pcs[j];
      ASSERT(r_pc != PC_JUMPTABLE);
      r = pc2bbl(&in->bbl_map, r_pc);
      ASSERT(r >= 0 && r < in->num_bbls);
 
      if (r == i) {
        continue;
      }
      can_reach_tail = false;
      nreach = identify_reachable_bbls(in, nreachable_bbl_pcs, r, in->bbls[i].pc);
      for (nr = 0; nr < nreach; nr++) {
        if (nreachable_bbl_pcs[nr] == loops[i].exit) {
          can_reach_tail = true;
          break;
        }
      }
      if (!can_reach_tail) {
        DBG2(UNROLL_LOOPS, "%lx-%lx: contains external branch to %lx "
            "(can't reach tail)\n", (long)in->bbls[i].pc, (long)loops[i].tail,
            (long)r_pc);
        loops[i].contains_external_branch = true;
        break;
      }
    }
  }
}

static char *
induction_regs_to_string(induction_reg_t const *regs, int num_regs, char *buf,
    size_t size)
{
  char *ptr = buf, *end = buf + size;
  int i;
  for (i = 0; i < num_regs; i++) {
    if (regs[i].valid) {
      ptr += snprintf(ptr, end - ptr, "%d:%d,", regs[i].name, regs[i].value);
    } else {
      ptr += snprintf(ptr, end - ptr, "!%d!,", regs[i].name);
    }
    ASSERT(ptr < end);
  }
  return buf;
}

static void
induction_regs_merge(induction_reg_t *regs, long *num_regs, induction_reg_t const *sregs,
    long num_sregs)
{
  long i;
  for (i = 0; i < num_sregs; i++) {
    long j;
    for (j = 0; j < *num_regs; j++) {
      if (regs[j].name == sregs[i].name) {
        regs[j].value = sregs[i].value;
        regs[j].valid = sregs[i].valid;
        break;
      }
    }
    if (j == *num_regs) {
      regs[j].name = sregs[i].name;
      regs[j].value = sregs[i].value;
      regs[j].valid = sregs[i].valid;
      (*num_regs)++;
    }
  }
}

static bool
induction_regs_match(induction_reg_t const *regs, int num_regs,
    induction_reg_t const *sregs, int num_sregs, int reg1, int reg2)
{
  int i;
  for (i = 0; i < num_regs; i++) {
    if (regs[i].name != reg1 && regs[i].name != reg2) {
      continue;
    }
    int j;
    for (j = 0; j < num_sregs; j++) {
      if (sregs[j].name == regs[i].name) {
        ASSERT(sregs[j].valid);
        ASSERT(regs[i].valid);
        if (regs[i].value != sregs[j].value) {
          DBG2(UNROLL_LOOPS, "Mismatch on %d: %d <-> %d\n", regs[i].name,
              regs[i].value, sregs[j].value);
          return false;
        }
      }
    }
  }
  return true;
}

#define MAX_INDUCTION_REGS 64
static bool
determine_induction_register_dfs(input_exec_t const *in, uloop_t *loop,
    src_ulong cur, induction_reg_t *regs, long *num_regs, bool *visited)
{
  long loop_bbl_index = -1;
  long i, j, bbl_id, bbln;
  i386_insn_t insn;
  src_ulong pc;
  bbl_t *bbl;

  for (i = 0; i < loop->num_bbls; i++) {
    if (loop->bbl_pcs[i] == cur) {
      loop_bbl_index = i;
      break;
    }
  }
  ASSERT(loop_bbl_index != -1);
  /*if (visited[loop_tb_index]) {
    DBG2(UNROLL_LOOPS, "%s() %d\n", __func__, __LINE__);
    return true;
  }*/
  visited[loop_bbl_index] = true;

  bbl_id = pc2bbl(&in->bbl_map, cur);
  ASSERT(bbl_id >= 0 && bbl_id < in->num_bbls);
  bbl = &in->bbls[bbl_id];
  DBG2(UNROLL_LOOPS, "called on bbl %lx\n", (long)bbl->pc);
  bbln = bbl_num_insns(bbl);
  induction_reg_t *bblregs[bbln];
  long num_bblregs[bbln];
  i = 0;
  for (pc = bbl_first_insn(bbl); pc != PC_UNDEFINED; pc = bbl_next_insn(bbl, pc)) {
    bool fetch;
    fetch = i386_insn_fetch(&insn, pc, NULL);
    ASSERT(fetch);

    if (!i386_insn_update_induction_reg_values(&insn, regs, MAX_INDUCTION_REGS,
          num_regs)) {
      DBG2(UNROLL_LOOPS, "%lx: returning false\n", (long)bbl->pc);
      for (j = 0; j < i; j++) {
        if (bblregs[j]) {
          free(bblregs[j]);
        }
      }
      return false;
    }
    bblregs[i] = (induction_reg_t *)malloc(*num_regs * sizeof(induction_reg_t));
    ASSERT(bblregs[i]);
    memcpy(bblregs[i], regs, *num_regs * sizeof(induction_reg_t));
    num_bblregs[i] = *num_regs;
    i++;
  }

  induction_reg_t orig_regs[MAX_INDUCTION_REGS];
  long num_orig_regs = *num_regs;
  memcpy(orig_regs, regs, *num_regs * sizeof(induction_reg_t));
  bool ret = false;
  for (i = 0; i < bbl->num_nextpcs; i++) {
    ASSERT(bbl->nextpcs[i] != PC_JUMPTABLE);
    if (bbl->nextpcs[i] == loop->exit) {
      src_ulong ireg1, ireg2;
      long ival1, ival2;
      i386_insn_t insn;
      bool fetch;

      fetch = i386_insn_fetch(&insn, bbl_insn_address(bbl, bbl_num_insns(bbl) - 1),
          NULL);
      ASSERT(fetch);
      if (!i386_insn_is_conditional_direct_branch(&insn)) {
        ret = false;
        goto done;
      }
      char cmp_operator[32];
      list_node_t *eflags_rdefs;
      i386_insn_t cmp_insn;
      long pc_index;
      src_ulong pc;

      NOT_IMPLEMENTED();
      eflags_rdefs = 0; //XXX: bbl->rdefs[bbl_num_insns(bbl) - 1].table[I386_EFLAGS_REG];
      if (!eflags_rdefs || eflags_rdefs->next) {
        /* if there is none or more than one reaching definitions, return false. */
        DBG2(UNROLL_LOOPS, "%x: eflags_rdefs=%p, next=%p\n",
            bbl_insn_address(bbl, bbl_num_insns(bbl) - 1), eflags_rdefs,
            eflags_rdefs?eflags_rdefs->next:NULL);
        ret = false;
        goto done;
      }
      pc = eflags_rdefs->pc;
      pc_index = bbl_insn_index(bbl, pc);
      if (pc_index == -1) {
        DBG2(UNROLL_LOOPS, "%lx: insn setting flags not in same tb.\n", (long)bbl->pc);
        ret = false;
        goto done;
      }
      fetch = i386_insn_fetch(&cmp_insn, eflags_rdefs->pc, NULL);
      ASSERT(fetch);

      bool branch_target_is_loop_head = (bbl->nextpcs[0] == loop->head);
      DBG2(UNROLL_LOOPS, "Calling i386_insn_comparison_uses_induction_regs on "
          "jmp_insn at %x, cmp_insn at %x, induction regs \"%s\", target_is_head %d\n",
          bbl_insn_address(bbl, bbl_num_insns(bbl) - 1), eflags_rdefs->pc,
          induction_regs_to_string(bblregs[pc_index], num_bblregs[pc_index], as1,
            sizeof as1), branch_target_is_loop_head);

      /* call i386_insn_comparison_uses_induction_regs. */
      if (!i386_insn_comparison_uses_induction_regs(&insn, &cmp_insn, bblregs[pc_index],
            num_bblregs[pc_index], branch_target_is_loop_head, &ireg1, &ival1, &ireg2,
            &ival2, cmp_operator, sizeof cmp_operator)) {
        DBG2(UNROLL_LOOPS, "%lx: comparison does not use induction regs.\n",
            (long)bbl->pc);
        ret = false;
        goto done;
      }

      if (loop->induction_reg1 == -1) {
        ASSERT(loop->induction_reg2 == -1);
        loop->induction_reg1 = ireg1;
        loop->induction_reg2 = ireg2;
        loop->induction_val1 = ival1;
        loop->induction_val2 = ival2;
        strncpy(loop->cmp_operator, cmp_operator, sizeof loop->cmp_operator);
        ret = true;
        goto done;
      }
      if (   ireg1 != loop->induction_reg1
          || ireg2 != loop->induction_reg2
          || ival1 != loop->induction_val1
          || ival2 != loop->induction_val2
          || strcmp(loop->cmp_operator, cmp_operator)) {
        DBG2(UNROLL_LOOPS, "%lx: different conditions have different induction vars.\n",
            (long)bbl->pc);
        ret = false;
        goto done;
      }
      ret = true;
      goto done;
    } else if (bbl->nextpcs[i] != loop->head) {
      induction_reg_t sregs[MAX_INDUCTION_REGS];
      long num_sregs = *num_regs;
      memcpy(sregs, orig_regs, num_orig_regs * sizeof(induction_reg_t));
      if (!determine_induction_register_dfs(in, loop, bbl->nextpcs[i], sregs, &num_sregs,
            visited)) {
        ret = false;
        goto done;
      }
      DBG2(UNROLL_LOOPS, "%lx: induction_regs on %lx path: %s\n", (long)bbl->pc,
          (long)bbl->nextpcs[i],
          induction_regs_to_string(sregs, num_sregs, as1, sizeof as1));
      if (ret) {
        if (!induction_regs_match(regs, *num_regs, sregs, num_sregs,
              loop->induction_reg1, loop->induction_reg2)) {
          DBG2(UNROLL_LOOPS, "%lx: cur=%lx (pc %lx), nextpcs[0]=%lx, nextpcs[1]=%lx, "
              "nsucc %ld. "
              "returning false\n", (long)loop->head, (long)cur,
              (long)cur, (long)bbl->nextpcs[0], (long)bbl->nextpcs[1],
              bbl->num_nextpcs);
          ret = false;
          goto done;
        }
      } else {
        induction_regs_merge(regs, num_regs, sregs, num_sregs);
      }
      DBG2(UNROLL_LOOPS, "%lx: after merge induction_regs on %lx path: %s\n",
          (long)bbl->pc, (long)bbl->nextpcs[i],
          induction_regs_to_string(sregs, num_sregs, as1, sizeof as1));
      ASSERT(loop->induction_reg1 != -1);
      ASSERT(loop->induction_reg2 != -1);
      ret = true;
    }
  }
done:
  for (i = 0; i < bbln; i++) {
    if (bblregs[i]) {
      free(bblregs[i]);
    }
  }
  return ret;
}

static bool
determine_induction_register(input_exec_t const *in, uloop_t *loop)
{
  induction_reg_t regs[MAX_INDUCTION_REGS];
  bool visited[loop->num_bbls];
  long num_regs = 0;
  long i;

  for (i = 0; i < loop->num_bbls; i++) {
    visited[i] = false;
  }
  loop->induction_reg1 = loop->induction_reg2 = (src_ulong)-1;
  loop->induction_val1 = loop->induction_val2 = 0;
  return determine_induction_register_dfs(in, loop, loop->head, regs, &num_regs,
    visited);
}

static int
src_ulong_compare(const void *_a, const void  *_b)
{
  src_ulong a = *(src_ulong *)_a;
  src_ulong b = *(src_ulong *)_b;

  return (a - b);
}

static size_t
loop_unroll(translation_instance_t *ti, uloop_t const *loop, uint8_t *buf, size_t size)
{
  NOT_IMPLEMENTED();
/*
  uint8_t *start_tc_ptr, *end_tc_ptr, *first_jcc_insn_tc_ptr, *ptr, *end;
  size_t loop_body_size, head_distance;
  char inv_cmp_operator[32];
  bbl_t *headbbl, *bbl;
  long u, i, bbl_id;
  input_exec_t *in;

  in = ti->in;
  head_distance = -1;
  i386_insn_invert_cmp_operator(inv_cmp_operator, sizeof inv_cmp_operator,
      loop->cmp_operator);
  loop_body_size = 0;
  for (i = 0; i < loop->num_bbls; i++) {
    if (loop->bbl_pcs[i] == loop->head) {
      head_distance = loop_body_size;
    }
    bbl_id = pc2bbl(&in->bbl_map, loop->bbl_pcs[i]);
    ASSERT(bbl_id >= 0 && bbl_id < in->num_bbls);
    bbl = &in->bbls[bbl_id];
    loop_body_size += bbl->size;
  }

  ptr = buf;
  end = buf + size;
  ptr += i386_add_to_induction_reg_insn(ptr, loop->induction_reg1,
      loop->induction_val1, num_loop_unrolls);
  ptr += i386_add_to_induction_reg_insn(ptr, loop->induction_reg2,
      loop->induction_val2, num_loop_unrolls);
  ptr += i386_cmp_insn_for_induction_regs(ptr,
      loop->induction_reg1, loop->induction_reg2);

  first_jcc_insn_tc_ptr = ptr;
  ptr += i386_unroll_loop_jcc_insn(ptr, loop->cmp_operator, 0);

  start_tc_ptr = ptr;
  ptr += i386_add_to_induction_reg_insn(ptr, loop->induction_reg1,
      -loop->induction_val1, num_loop_unrolls);
  ptr += i386_add_to_induction_reg_insn(ptr, loop->induction_reg2,
      -loop->induction_val2, num_loop_unrolls);
  ptr += i386_unroll_loop_jmp_insn(ptr, head_distance);
  ASSERT(head_distance != -1);
  for (u = 0; u < num_loop_unrolls; u++) {
    long i, cur;

    cur = 0;
    for (i = 0; i < loop->num_bbls; i++) {
      long j, bbl_id;
      src_ulong pc;
      bbl_t *bbl;

      bbl_id = pc2bbl(&in->bbl_map, loop->bbl_pcs[i]);
      ASSERT(bbl_id >= 0 && bbl_id < in->num_bbls);
      bbl = &in->bbls[bbl_id];
      cur += bbl->size;
      for (pc = bbl_first_insn(bbl), j = 0;
          pc != PC_UNDEFINED;
          pc = bbl_next_insn(bbl, pc), j++) {
        i386_insn_t insn;
        long size, i;
        bool fetch;

        fetch = i386_insn_fetch(&insn, pc, &size);
        ASSERT(fetch);
        if (j == bbl_num_insns(bbl) - 1) {
          if (bbl->nextpcs[0] == loop->head || bbl->nextpcs[0] == loop->exit) {
            long k;
            ASSERT(size >= 5);
            for (k = 0; k < size - 5; k++) {
              *(ptr++) = 0x90;
            }
            *(ptr++) = 0xe9;
            *(uint32_t *)ptr = loop_body_size - cur + head_distance;
            ptr += 4;
            break;
          }
        }
        for (i = 0; i < size; i++) {
          *(ptr++) = ldub_input(pc + i);
        }
      }
    }
  }
  for (i = 0; i < head_distance; i++) {
    *(ptr++) = 0x90;
  }

  ptr += i386_add_to_induction_reg_insn(ptr, loop->induction_reg1, loop->induction_val1,
      num_loop_unrolls);
  ptr += i386_add_to_induction_reg_insn(ptr, loop->induction_reg2, loop->induction_val2,
      num_loop_unrolls);
  ptr += i386_cmp_insn_for_induction_regs(ptr, loop->induction_reg1,
      loop->induction_reg2);
  int jcc_insn_size = i386_unroll_loop_jcc_insn(ptr, inv_cmp_operator, 0);
  ptr += i386_unroll_loop_jcc_insn(ptr, inv_cmp_operator,
      start_tc_ptr - (ptr + jcc_insn_size));
  end_tc_ptr = ptr;
  // unrolled loop's tail. 
  ptr += i386_add_to_induction_reg_insn(ptr, loop->induction_reg1,
      -loop->induction_val1, num_loop_unrolls);
  ptr += i386_add_to_induction_reg_insn(ptr, loop->induction_reg2,
      -loop->induction_val2, num_loop_unrolls);
  // patch first jcc insn to jump to unrolled loop's tail. 
  i386_unroll_loop_jcc_insn(first_jcc_insn_tc_ptr, loop->cmp_operator,
      end_tc_ptr - start_tc_ptr);
  return ptr - buf;
*/
}

#define MAX_UNROLL_LOOP_TBS 3

void
unroll_loops(translation_instance_t *ti)
{
  input_exec_t *in = ti->in;
  uloop_t loops[in->num_bbls];
  long i, j, dst_bbl;
  uint8_t *ptr, *end;
  i386_insn_t insn;
  src_ulong dst;
  bbl_t *bbl;
  bool fetch;

  for (i = 0; i < in->num_bbls; i++) {
    loops[i].head = (src_ulong)-1;
    loops[i].tail = (src_ulong)-1;
    loops[i].exit = (src_ulong)-1;
    loops[i].contains_external_branch = false;
    loops[i].contains_nested_loop = false;
    loops[i].contains_function_call = false;
    loops[i].no_induction_variables = false;
    loops[i].num_bbls = 0;
    loops[i].bbl_pcs = NULL;
    loops[i].induction_reg1 = loops[i].induction_reg2 = (src_ulong)-1;
    loops[i].induction_val1 = loops[i].induction_val2 = 0;
    loops[i].unroll_me = false;
  }
  for (i = 0; i < in->num_bbls; i++) {
    bbl = &in->bbls[i];
    for (j = 0; j < bbl->num_nextpcs; j++) {
      if (bbl->nextpcs[j] != PC_JUMPTABLE) {
        fetch = i386_insn_fetch(&insn, bbl_last_insn(bbl), NULL);
        ASSERT(fetch);
        dst = bbl->nextpcs[j];
        dst_bbl = pc2bbl(&in->bbl_map, dst);
        ASSERT(dst_bbl >= 0 && dst_bbl < in->num_bbls);
        DBG2(UNROLL_LOOPS, "pc_dominates(%lx, %lx) = %d\n", (long)dst, (long)bbl->pc,
            pc_dominates(in, dst, bbl->pc));
        if (   pc_dominates(in, dst, bbl->pc)
            && !i386_insn_is_function_call(&insn)) {
          long k;

          DBG2(UNROLL_LOOPS, "Found loop %lx-%lx\n", (long)dst, (long)bbl->pc);
          loops[dst_bbl].head = dst;
          loops[dst_bbl].tail = decide_inner_loop(in, dst, loops[dst_bbl].tail,
              bbl->pc);
          for (k = 0; k < bbl->num_nextpcs; k++) {
            if (bbl->nextpcs[k] == bbl->nextpcs[j]) {
              continue;
            }
            if (bbl->nextpcs[k] == PC_JUMPTABLE) {
              DBG2(UNROLL_LOOPS, "%lx: contains external branch\n", (long)dst);
              loops[dst_bbl].contains_external_branch = true;
              break;
            }
            if (loops[dst_bbl].exit == (src_ulong)-1) {
              loops[dst_bbl].exit = bbl->nextpcs[k];
            }
            if (loops[dst_bbl].exit != bbl->nextpcs[k]) {
              DBG2(UNROLL_LOOPS, "%lx: loop contains external branch %lx\n",
                  (long)dst, (long)bbl->nextpcs[k]);
              loops[dst_bbl].contains_external_branch = true;
              break;
            }
          }
        }
      }
    }
  }
  identify_loop_attributes(in, loops);
  for (i = 0; i < in->num_bbls; i++) {
    bbl_t *bbl = &in->bbls[i];
    src_ulong dst;
    long j;

    if (!is_unrollable(&loops[i])) {
      continue;
    }

    dst = loops[i].tail;
    loops[i].bbl_pcs = (src_ulong *)malloc(loops[i].num_bbls * sizeof(src_ulong));
    ASSERT(loops[i].bbl_pcs);
    j = identify_reachable_bbls(in, loops[i].bbl_pcs, i, loops[i].exit);
    ASSERT(j == loops[i].num_bbls);
    if (!determine_induction_register(in, &loops[i])) {
      DBG2(UNROLL_LOOPS, "%lx size %ld: induction variables not found.\n",
          (long)bbl->pc, loops[i].num_bbls);
      loops[i].no_induction_variables = true;
      continue;
    }
    DBG(UNROLL_LOOPS, "Found unrollable loop %lx-%lx of size %ld bbls\n",
        (long)bbl->pc, (long)dst, loops[i].num_bbls);
    if (loops[i].num_bbls > MAX_UNROLL_LOOP_TBS) {
      DBG(UNROLL_LOOPS, "%lx: num_bbls %ld > max (%ld)\n", (long)bbl->pc,
          (long)loops[i].num_bbls, (long)MAX_UNROLL_LOOP_TBS);
      continue;
    }
    qsort(loops[i].bbl_pcs, loops[i].num_bbls, sizeof(src_ulong), src_ulong_compare);
    loops[i].unroll_me = true;
    for (j = 0; j < loops[i].num_bbls - 1; j++) {
      long jbbl, kbbl;

      jbbl = pc2bbl(&in->bbl_map, loops[i].bbl_pcs[j]);
      ASSERT(jbbl >= 0 && jbbl < in->num_bbls);
      kbbl = pc2bbl(&in->bbl_map, loops[i].bbl_pcs[j + 1]);
      ASSERT(kbbl >= 0 && kbbl < in->num_bbls);

      if (kbbl != jbbl + 1) {
        DBG2(UNROLL_LOOPS, "%lx: loop bbls %lx -> %lx not contiguous.\n",
            (long)loops[i].head, (long)loops[i].bbl_pcs[j],
            (long)loops[i].bbl_pcs[j + 1]);
        loops[i].unroll_me = false;
      }
    }
  }

  transmap_t dummy_out_tmap_store, *dummy_out_tmap = &dummy_out_tmap_store;
  regcons_t dummy_transcons;
  long num_unrolled = 0;

  ti->bbl_headers = (uint8_t **)malloc(in->num_bbls * sizeof(uint8_t *));
  ASSERT(ti->bbl_headers);
  memset(ti->bbl_headers, 0, in->num_bbls * sizeof(char *));
  ti->bbl_header_sizes = (long *)malloc(in->num_bbls * sizeof(long));
  ASSERT(ti->bbl_header_sizes);

  for (i = 0; i < in->num_bbls; i++) {
    bbl_t *bbl = &in->bbls[i];
    src_ulong pc;
    long b0, b1;
    long n;

    if (loops[i].unroll_me) {
      long j, hdr_size, max_hdr_size;

      max_hdr_size = loops[i].num_bbls * 10240;
      num_unrolled++;
      DBG(UNROLL_LOOPS, "Unrolling %lx-%lx (size %ld): %s %d:%d, %d:%d\n",
          (long)in->bbls[i].pc, (long)loops[i].tail, (long)loops[i].num_bbls,
          loops[i].cmp_operator, loops[i].induction_reg2, loops[i].induction_val2,
          loops[i].induction_reg1, loops[i].induction_val1);

      ti->bbl_headers[i] = (char *)malloc(max_hdr_size * sizeof(char));
      ASSERT(ti->bbl_headers[i]);
      hdr_size = loop_unroll(ti, &loops[i], ti->bbl_headers[i], max_hdr_size);
      ti->bbl_headers[i] = (char *)realloc(ti->bbl_headers[i], hdr_size * sizeof(char));
      ti->bbl_header_sizes[i] = hdr_size;
    }
  }
  DBG_EXEC(UNROLL_LOOPS_INFO,
      long i;
      printf("Unrolled %ld loops.\n", num_unrolled);
      for (i = 0; i < in->num_bbls; i++) {
        if (!loops[i].unroll_me) {
          continue;
        }
        printf(", %lx-%lx", (long)loops[i].head, (long)loops[i].tail);
      }
      printf("\n");
  );

  for (i = 0; i < in->num_bbls; i++) {
    if (loops[i].bbl_pcs) {
      free(loops[i].bbl_pcs);
    }
  }
}
