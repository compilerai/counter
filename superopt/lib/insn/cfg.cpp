#include <map>
#include <iostream>
#include "insn/cfg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "support/src-defs.h"
#include "support/debug.h"
#include "support/c_utils.h"
#include "support/globals.h"

#include "cutils/pc_elem.h"
#include "cutils/imap_elem.h"
#include "cutils/rbtree.h"
#include "cutils/chash.h"
#include "cutils/interval_set.h"
#include "cutils/addr_range.h"

#include "expr/symbol_or_section_id.h"

#include "rewrite/translation_instance.h"
#include "insn/edge_table.h"
#include "insn/live_ranges.h"
#include "insn/rdefs.h"
#include "rewrite/bbl.h"
#include "insn/src-insn.h"
#include "valtag/ldst_input.h"
#include "insn/fun_type_info.h"
#include "insn/jumptable.h"
#include "valtag/nextpc.h"

/* arch specific includes */
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "ppc/insn.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "ppc/regs.h"
#include "valtag/regset.h"
#include <algorithm>
#include "valtag/pc_with_copy_id.h"

using namespace std;

bool indir_link_flag = false;

#define MAX_LINK_TARGETS 4

static char as1[4096];

static void
bbl_print(FILE *file, input_exec_t const *in, long bbl_index, bbl_t const *bbl, char const *name)
{
  long i;

  ASSERT(bbl_index >= 0 && bbl_index < in->num_bbls);
  fprintf(file, "\n\nbbl [%s]: pc 0x%lx, size %ld, num_insns %ld, in_flow %lld, reachable %d\n",
      name, (unsigned long)bbl->pc, bbl->size, bbl->num_insns,
      bbl->in_flow, bbl->reachable);
  fprintf(file, "preds:");
  for (i = 0; i < bbl->num_preds; i++) {
    fprintf(file, "%c", i?',':' ');
    if (bbl->preds[i] == PC_JUMPTABLE) {
      fprintf(file, "indir [%f]", bbl->pred_weights[i]);
    } else {
      fprintf(file, "%lx [%f]", (long)bbl->preds[i],
          bbl->pred_weights?bbl->pred_weights[i]:0);
    }
  }
  fprintf(file, "\n");
  if (bbl->live_regs) {
    fprintf(file, "live_in: %s\n",
        regset_to_string(&bbl->live_regs[0], as1, sizeof as1));
    fprintf(file, "live_ranges_in: %s\n",
        live_ranges_to_string(&bbl->live_ranges[0], &bbl->live_regs[0], as1, sizeof as1));
    long imnl = in->innermost_natural_loop[bbl_index];
    fprintf(file, "innermost_natural_loop: %ld [pc %lx]\n", imnl, (imnl == -1) ? -1 : (long)in->bbls[imnl].pc);
  }
  for (i = 0; i < bbl->num_insns; i++) {
    src_insn_t insn;

    if (src_insn_fetch(&insn, in, bbl->pcs[i], NULL)) {
      fprintf(file, "%lx: %s", (long)bbl->pcs[i],
          src_insn_to_string(&insn, as1, sizeof as1));
    } else {
      char *ptr = as1, *end = ptr + sizeof as1;
      src_ulong next, j;
      ptr += snprintf(ptr, end - ptr, ".byte ");
      if (i == bbl->num_insns -1) {
        next = bbl->pc + bbl->size;
      } else {
        next = bbl->pcs[i + 1];
      }
      for (j = bbl->pcs[i]; j < next; j++) {
        ptr += snprintf(ptr, end - ptr, "0x%hhx", ldub_input(in, j));
      }
      fprintf(file, "%lx: %s\n", (long)bbl->pcs[i], as1);
    }
  }
  fprintf(file, "nextpcs:");
  for (i = 0; i < bbl->num_nextpcs; i++) {
    fprintf(file, "%c", i?',':' ');
    if (bbl->nextpcs[i].get_tag() == tag_const && bbl->nextpcs[i].get_val() == PC_JUMPTABLE) {
      fprintf(file, "indir");
    } else {
      fprintf(file, "%s [%f]", bbl->nextpcs[i].nextpc_to_string().c_str(),
          bbl->nextpc_probs?bbl->nextpc_probs[i]:0);
    }
  }
  if (bbl->loop) {
    fprintf(file, "\nloop:\npcs:");
    for (i = 0; i < bbl->loop->num_pcs; i++) {
      fprintf(file, " %lx", (long)bbl->loop->pcs[i]);
    }
    fprintf(file, "\nout_edges:");
    for (i = 0; i < bbl->loop->num_out_edges; i++) {
      fprintf(file, " 0x%lx->%s [%.2f]", (long)bbl->loop->out_edges[i].src,
           bbl->loop->out_edges[i].dst.nextpc_to_string().c_str(), bbl->loop->out_edge_weights[i]);
    }
  }
  fprintf(file, "\nfcalls:");
  for (i = 0; i < bbl->num_fcalls; i++) {
    fprintf(file, " %s [returns to %lx],", bbl->fcalls[i].nextpc_to_string().c_str(),
        (long)bbl->fcall_returns[i]);
  }
  fprintf(file, "\n");
}



static void
read_edge_tables(edge_table_t *src_table, edge_table_t *dst_table)
{
  if (profile_name) {
    char edges_fn[1024];
    snprintf (edges_fn, sizeof edges_fn, "%s", profile_name);

    FILE *fp = fopen (edges_fn, "r");
    if (fp) {
      int n;
      printf("Reading edge-info from file %s\n", edges_fn);
      n = fscanf(fp, "IN_EDGES <--\ndst src\n");
      edge_table_read (fp, dst_table);
      n = fscanf(fp, "OUT_EDGES -->\nsrc dst\n");
      edge_table_read (fp, src_table);

      fclose(fp);
    } else {
      printf ("Warning: could not read profile name %s (file %s). error=%s\n",
          profile_name, edges_fn, strerror(errno));
    }
  }
}

typedef struct dst_sort_struct_t {
  src_ulong dst;
  long long num_occur;
} dst_sort_struct_t;

static int
dst_sort_struct_compare (const void *_a, const void *_b)
{
  dst_sort_struct_t *a = (dst_sort_struct_t *)_a;
  dst_sort_struct_t *b = (dst_sort_struct_t *)_b;
  return (b->num_occur - a->num_occur);
}



static int
find_likely_indir_targets(input_exec_t *in, bbl_t *bbl, src_ulong *targets,
    double *target_probs, unsigned long const *linked_targets,
    int num_linked_targets, int MAX_TARGETS)
{
  if (!indir_link_flag) {
    return 0;
  }
  edge_table_t *src_table;
  edge_table_init (&src_table, 65536);
  read_edge_tables (src_table, NULL);

#define MAX_OUT_EDGES (100*MAX_TARGETS)
  void *iter = NULL;
  int num_dsts = 0;
  dst_sort_struct_t dst_sort_struct[MAX_OUT_EDGES];
  while ((iter = edge_table_find_next(src_table, iter, bbl_last_insn(bbl),
          &(dst_sort_struct[num_dsts].dst),
          &(dst_sort_struct[num_dsts].num_occur)))
      && ++num_dsts < MAX_OUT_EDGES);

  /* if (num_dsts == 0 && is_function_return(bbl_last_insn(bbl))) {
    //src_ulong fret = bbl->pc + bbl->size - 4;
    src_ulong fret = bbl_last_insn(bbl);
    struct pc_elem_t elem;
    struct rbtree_elem *iter;
    elem.pc = fret;
    for (iter = rbtree_lower_bound(&in->function_calls, &elem.rb_elem);
         iter && num_dsts < MAX_OUT_EDGES;
         iter = rbtree_next(iter)) {
      int i;
      int dst_exists = 0;
      struct pc_elem_t *ret = rbtree_entry(iter, struct pc_elem_t, rb_elem);
      for (i = 0; i < num_dsts; i++) {
        if (dst_sort_struct[i].dst == ret->pc) {
          dst_exists = 1;
        }
      }
      if (!dst_exists) {
        dst_sort_struct[num_dsts].dst = ret->pc;
        long bbl_index;
        struct hash_elem *found;
        struct imap_elem_t needle;
        needle.pc = ret->pc;
        found = hash_find(&in->bbl_map, &needle.h_elem);
        ASSERT(found);
        bbl_index = hash_entry(found, imap_elem_t, h_elem)->val;
        dst_sort_struct[num_dsts].num_occur = in->bbls[bbl_index].in_flow + 1;
        num_dsts++;
      }
    }
  }*/

  qsort(dst_sort_struct, num_dsts, sizeof(dst_sort_struct_t),
      dst_sort_struct_compare);

  int i;
  unsigned long new_target_pcs[MAX_TARGETS];
  long long new_target_occur[MAX_TARGETS];
  unsigned long long sum_occur = 0;
  int num_new_targets = 0;
  for (i = 0; i < num_dsts && num_new_targets < MAX_TARGETS; i++) {
    int j;
    int already_occurs = 0;
    for (j = 0; j < num_linked_targets; j++) {
      ASSERT(linked_targets[j] >= 0 && linked_targets[j] < in->num_bbls);
      if (dst_sort_struct[i].dst == in->bbls[linked_targets[j]].pc) {
        already_occurs = 1;
        break;
      }
    }
    if (!already_occurs) {
      new_target_pcs[num_new_targets] = dst_sort_struct[i].dst;
      new_target_occur[num_new_targets] = dst_sort_struct[i].num_occur;
      num_new_targets++;
      sum_occur += dst_sort_struct[i].num_occur;
    }
  }

  for (i = 0; i < num_new_targets; i++) {
    /*long bbl_index;
    struct hash_elem *found;
    struct imap_elem_t needle;

    needle.pc = new_target_pcs[i];
    found = hash_find(&in->bbl_map, &needle.h_elem);
    ASSERT(found);
    bbl_index = hash_entry(found, imap_elem_t, h_elem)->val;*/
    targets[i] = new_target_pcs[i];
    target_probs[i] = (double)new_target_occur[i]/sum_occur;
  }

  edge_table_free(src_table);

  return num_new_targets;
}

static src_ulong
got_addr(input_exec_t const *in)
{
  unsigned i;
  //for (i = 0; i < in->num_symbols; i++)
  for (auto s : in->symbol_map) {
    //!strcmp(in->symbols[i].name, "_GLOBAL_OFFSET_TABLE_");
    if (!strcmp(s.second.name, "_GLOBAL_OFFSET_TABLE_")) {
      //return in->symbols[i].value;
      return s.second.value;
    }
  }
  //fprintf(stderr, "_GLOBAL_OFFSET_TABLE_ not found\n");
  return (src_ulong)-1;
}

bool
is_code_section(input_section_t const *section)
{
  if (strcmp(section->name, ".dbg_functions") && (section->flags & SHF_EXECINSTR)) {
    return true;
  }
  return false;
}



static bool
is_global_offset_table(input_exec_t const *in, char const *sec_name,src_ulong nip)
{
  if (strcmp(sec_name, ".got")) {
    return false;
  }
  if (nip >= got_addr(in)) {
    return true;
  } else {
    return false;
  }
}

static bool
input_code_section_pc_is_executable(input_section_t const *isec, src_ulong pc)
{
/*#if ARCH_SRC == ARCH_ETFG
  return etfg_input_section_pc_is_executable(isec, pc);
#endif*/
  return true;
}

static bool
is_executable(input_exec_t const *in, src_ulong nip)
{
  int i;
  for (i = 0; i < in->num_input_sections; i++) {
    DBG3(SRC_INSN, "in->input_sections[%d]: %s. addr=%x, size=%x\n", i,
        in->input_sections[i].name, (unsigned)in->input_sections[i].addr,
        (unsigned)in->input_sections[i].size);
    if (   nip >= in->input_sections[i].addr
        && nip < in->input_sections[i].addr + in->input_sections[i].size
        //&& strcmp(in->input_sections[i].name, ".plt")
        && !is_global_offset_table(in, in->input_sections[i].name, nip)) {
      if (   is_code_section(&in->input_sections[i])
          && input_code_section_pc_is_executable(&in->input_sections[i], nip)) {
        DBG3(SRC_INSN, "returning true for %lx\n", (long)nip);
        return true;
      } else {
        /*
           DBG (STAT_TRANS, "returning FALSE for nip %x. name %s. flags %x\n",
           nip, in->input_sections[i].name.c_str(),
           in->input_sections[i].flags);
        //return false;
        */
      }
    }
  }
  DBG(SRC_INSN, "returning FALSE for nip %lx\n", (long)nip);
  return false;
}


static bool
visited(struct interval_set *translated_addrs, src_ulong pc)
{
  addr_range_t lb = {pc, pc + 1};
  struct interval_set_elem *found;
  struct addr_range_t *range;

  found = interval_set_lower_bound (translated_addrs, &lb.iset_elem);

  /* if (!found || rbtree_entry(found, bbl_leader_t, rb_elem)->start > pc) {
    if (found != rbtree_begin(translated_addrs)) {
      found = rbtree_prev (found);
    }
  } */
  if (!found) {
    DBG2(CFG, "%lx: returning false\n", (unsigned long)pc);
    return false;
  }
  range = interval_set_entry(found, struct addr_range_t, iset_elem);

  if (range->start <= pc && range->end > pc) {
    return true;
  }

  DBG2(CFG, "%lx: start=%lx, end=%lx, returning false\n", (unsigned long)pc,
      (unsigned long)range->start, (unsigned long)range->end);
  return false;
}

void
bbl_leaders_add(struct rbtree *tree, src_ulong pc)
{
  struct pc_elem_t *bbl_leader, needle;
  struct rbtree_elem *found;
  needle.pc = pc;

  if (found = rbtree_find(tree, &needle.rb_elem)) {
    return;
  }
  bbl_leader = (struct pc_elem_t *)malloc(sizeof *bbl_leader);
  ASSERT(bbl_leader);
  bbl_leader->pc = pc;
  rbtree_insert(tree, &bbl_leader->rb_elem);
}

static void
bbl_scan(input_exec_t const *in, src_ulong pc, src_ulong end, unsigned long *size,
    unsigned long *num_insns, src_ulong *pcs, vector<nextpc_t> &nextpcs, vector<nextpc_t>* fcalls,
    src_ulong *fcall_returns, long *num_fcalls)
{
  unsigned long n;
  //int num_nextpcs;
  src_insn_t insn;
  src_ulong cur;
  bool fetch;

  cur = pc;
  //num_nextpcs = 0;
  nextpcs.clear();
  if (num_fcalls) {
    *num_fcalls = 0;
  }
  n = 0;

  DBG(CFG, "pc = %lx\n", (long)pc);
  do {
    unsigned long insn_size;

    DBG(CFG, "cur = %lx, is_executable=%d, jumptable_belongs=%d\n", (long)cur, is_executable(in, cur), jumptable_belongs(in->jumptable, cur));
    if (!is_executable(in, cur)) {
      break;
    }
    if (jumptable_belongs(in->jumptable, cur) && cur != pc) {
      break;
    }
    fetch = src_insn_fetch(&insn, in, cur, &insn_size);
    ASSERT(n < BBL_MAX_NUM_INSNS);
    pcs[n] = cur;
    DBG(CFG, "src_insn_fetch(%lx) returned %d\n", (long)cur, fetch);
    cur += insn_size;
    n++;
    if (!fetch) {
      break;
    }
    CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": fetched insn =\n" << src_iseq_to_string(&insn, 1, as1, sizeof as1) << endl);
    if (fcalls) {
      if (src_insn_is_direct_function_call(&insn)) {
        ASSERT(*num_fcalls == fcalls->size());
        //fcalls[*num_fcalls] = src_insn_fcall_target(&insn, cur, in);
        fcalls->push_back(src_insn_fcall_target(&insn, cur, in));
        if (fcall_returns) {
          fcall_returns[*num_fcalls] = cur;
        }
        if (fcalls->at(*num_fcalls).get_tag() != tag_const) {
          DBG(CFG, "Adding fcall to %s (return %lx)\n", fcalls->at(*num_fcalls).nextpc_to_string().c_str(),
              fcall_returns?(long)fcall_returns[*num_fcalls]:(long)-1);
          (*num_fcalls)++;
        } else if (   fcalls->at(*num_fcalls).get_val() != (src_ulong)-1
                 && is_possible_nip(in, fcalls->at(*num_fcalls).get_val())) {
          DBG(CFG, "Adding fcall to %lx (return %lx)\n", (long)fcalls->at(*num_fcalls).get_val(),
              fcall_returns?(long)fcall_returns[*num_fcalls]:(long)-1);
          (*num_fcalls)++;
        } else {
          fcalls->pop_back();
        }
      } else if (src_insn_is_indirect_function_call(&insn)) {
        ASSERT(*num_fcalls == fcalls->size());
        //fcalls[*num_fcalls] = PC_JUMPTABLE;
        fcalls->push_back(PC_JUMPTABLE);
        if (fcall_returns) {
          fcall_returns[*num_fcalls] = cur;
        }
        (*num_fcalls)++;
      }
    }
    if (end && cur >= end) {
      break;
    }
  } while (!src_insn_terminates_bbl(&insn));

  ASSERT(size);
  *size = cur - pc;
  *num_insns = n;
  DBG(CFG, "%lx: size=%lx, num_insns=%lx\n", (unsigned long)pc, *size, *num_insns);

  //num_nextpcs = 0;
  if (!fetch) {
    //nextpcs[0] = PC_UNDEFINED;
    //num_nextpcs = 1;
    nextpcs.push_back(PC_UNDEFINED);
    DBG(CFG, "%lx: num_nextpcs = %lu\n", (unsigned long)pc, (unsigned long)nextpcs.size());
  } else if (src_insn_is_unconditional_direct_branch_not_fcall(&insn)) {
    //unsigned long target;
    vector<nextpc_t> targets = src_insn_branch_targets(&insn, cur);
    if (targets.size() != 1) {
      cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << ", targets.size() = " << targets.size() << endl;
      for (auto target : targets) {
        cout << __func__ << " " << __LINE__ << ": target = " << target.nextpc_to_string() << endl;
      }
    }
    ASSERT(targets.size() == 1);
    nextpc_t const& target = *targets.begin();
    //DBG(CFG, "target=%lx\n", (unsigned long)target);
    if (target.get_tag() != tag_const || is_possible_nip(in, target.get_val())) {
      //nextpcs[0] = target;
      //num_nextpcs = 1;
      nextpcs.push_back(target);
    }
    DBG(CFG, "%lx: num_nextpcs = %lu\n", (unsigned long)pc, (unsigned long)nextpcs.size());
  } else if (   src_insn_is_conditional_direct_branch(&insn)
             && !src_insn_is_function_call(&insn)) {
    //unsigned long target;
    //target = src_insn_branch_target(&insn, cur);
    vector<nextpc_t> targets = src_insn_branch_targets(&insn, cur);
    if (targets.size() != 2) {
      cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << ", targets.size() = " << targets.size() << endl;
      cout << __func__ << " " << __LINE__ << ": insn = " << src_insn_to_string(&insn, as1, sizeof as1) << endl;
      for (auto target : targets) {
        cout << __func__ << " " << __LINE__ << ": target = " << target.nextpc_to_string() << endl;
      }
    }
    ASSERT(targets.size() >= 2);
    /*auto iter = targets.begin();
    src_ulong target0 = *iter;
    iter++;
    src_ulong target1 = *iter;
    DBG(CFG, "target=%lx\n", (unsigned long)target);*/
    for (auto target : targets) {
      if (target.get_tag() != tag_const || (is_executable(in, target.get_val()) && is_possible_nip(in, target.get_val()))) {
        //nextpcs[num_nextpcs] = target;
        //num_nextpcs++;
        nextpcs.push_back(target.get_val());
      }
    }
    /*if (is_executable(in, cur) && is_possible_nip(in, cur)) {
      nextpcs[num_nextpcs] = cur;
      num_nextpcs++;
    }
    DBG(CFG, "%lx: num_nextpcs = %lu\n", (unsigned long)pc, (unsigned long)num_nextpcs);*/
  } else if (   src_insn_is_conditional_indirect_branch(&insn)
             && !src_insn_is_function_call(&insn)) {
#if ARCH_SRC == ARCH_ETFG
    NOT_REACHED();
#endif
    //nextpcs[num_nextpcs] = PC_JUMPTABLE;
    //num_nextpcs++;
    nextpcs.push_back(PC_JUMPTABLE);
    if (is_executable(in, cur) && is_possible_nip(in, cur)) {
      nextpcs.push_back(cur);
      //nextpcs[num_nextpcs] = cur;
      //num_nextpcs++;
    }
    DBG(CFG, "%lx: num_nextpcs = %lu\n", (unsigned long)pc, (unsigned long)nextpcs.size());
  } else if (src_insn_is_indirect_branch_not_fcall(&insn)) {
    //nextpcs[0] = PC_JUMPTABLE;
    //num_nextpcs = 1;
    nextpcs.push_back(PC_JUMPTABLE);
    DBG(CFG, "%lx: num_nextpcs = %lu\n", (unsigned long)pc, (unsigned long)nextpcs.size());
  } else {
    //if (is_executable(in, cur) && is_possible_nip(in, cur)) {
      //nextpcs[0] = cur;
      //num_nextpcs = 1;
      nextpcs.push_back(cur);
    /*} else {
      nextpcs[0] = PC_UNDEFINED;
      num_nextpcs = 1;
    }*/
    DBG(CFG, "%lx: num_nextpcs = %lu\n", (unsigned long)pc, (unsigned long)nextpcs.size());
  }
  //return num_nextpcs;
}

static void
translated_addrs_add(struct interval_set *translated_addrs, src_ulong pc,
    src_ulong bbl_end)
{
  addr_range_t range = {pc , bbl_end};
  struct interval_set_elem *found;

  if (found = interval_set_find(translated_addrs, &range.iset_elem)) {
    addr_range_t *frange;
    frange = interval_set_entry(found, addr_range_t, iset_elem);
    ASSERT(frange->end >= pc || frange->start < bbl_end);
    /*DBG(CFG, "Expanding translated_addrs entry from (%x,%x) to (%x,%x)\n",
        frange->start, frange->end, min(frange->start, pc), max(frange->end, bbl_end));*/
    //frange->start = min(frange->start, pc);
    if (frange->start > pc) {
      frange->start = pc;
    }
    //frange->end = max(frange->end, bbl_end);
    if (frange->end < bbl_end) {
      frange->end = bbl_end;
    }
  } else {
    addr_range_t *frange;
    frange = (addr_range_t *)malloc(sizeof *frange);
    ASSERT(frange);
    frange->start = pc;
    frange->end = bbl_end;
    DBG(CFG, "Inserting translated_addrs entry (%lx,%lx)\n", (long)frange->start,
        (long)frange->end);
    interval_set_insert(translated_addrs, &frange->iset_elem);
  }
}

static void
dfs_find_bbl_leaders(input_exec_t *in, src_ulong pc,
    struct interval_set *translated_addrs)
{
  long num_nextpcs, num_fcalls;
  unsigned long num_insns, size;
  src_ulong *pcs;//, *fcalls;
  //nextpc_t *nextpcs;
  src_ulong bbl_end;
  int i;

  DBG2(CFG, "pc=%lx\n", (long)pc);
  if (   pc == PC_JUMPTABLE
      || pc == PC_UNDEFINED
      || !is_executable(in, pc)
      || !is_possible_nip(in, pc)) {
    return;
  }
  DBG(CFG, "Adding %lx to bbl_leaders\n", (long)pc);
  bbl_leaders_add(&in->bbl_leaders, pc);

  if (visited(translated_addrs, pc)) {
    return;
  }

  //nextpcs = (nextpc_t *)malloc(BBL_MAX_NUM_NEXTPCS * sizeof(nextpc_t));
  vector<nextpc_t> nextpcs;
  pcs = (src_ulong *)malloc(BBL_MAX_NUM_INSNS * sizeof(src_ulong));
  ASSERT(pcs);
  //fcalls = (src_ulong *)malloc(BBL_MAX_NUM_INSNS * sizeof(src_ulong));
  //ASSERT(fcalls);
  vector<nextpc_t> fcalls_vec;

  bbl_scan(in, pc, 0, &size, &num_insns, pcs, nextpcs, &fcalls_vec, NULL,
      &num_fcalls);
  bbl_end = pc + size;

  num_nextpcs = nextpcs.size();
  ASSERT(num_fcalls == fcalls_vec.size());

  //ASSERT(num_nextpcs < BBL_MAX_NUM_NEXTPCS);
  //nextpcs = (src_ulong *)realloc(nextpcs, num_nextpcs * sizeof(src_ulong));
  ASSERT(num_insns < BBL_MAX_NUM_INSNS);
  pcs = (src_ulong *)realloc(pcs, num_insns * sizeof(src_ulong));
  ASSERT(num_fcalls < BBL_MAX_NUM_INSNS);
  //fcalls = (src_ulong *)realloc(fcalls, num_fcalls * sizeof(src_ulong));
  nextpc_t *fcalls = new nextpc_t[num_fcalls];
  ASSERT(fcalls);
  for (size_t i = 0; i < num_fcalls; i++) {
    fcalls[i] = fcalls_vec.at(i);
  }

  DBG(CFG, "bbl at %lx : num_nextpcs = %ld\n", (long)pc, (long)num_nextpcs);

  while (!is_possible_nip(in, bbl_end) && is_executable(in, bbl_end)) {
    bbl_end++;
  }

  translated_addrs_add(translated_addrs, pc, bbl_end);
  ASSERT(visited(translated_addrs, pc));

  for (i = 0; i < num_nextpcs; i++) {
    DBG(CFG, "following: %lx -> %s\n", (long)pc, nextpcs.at(i).nextpc_to_string().c_str());
    if (nextpcs.at(i).get_tag() == tag_const) {
      dfs_find_bbl_leaders(in, nextpcs.at(i).get_val(), translated_addrs);
    }
  }
  for (i = 0; i < num_fcalls; i++) {
    DBG(CFG, "following: %lx -> %s\n", (long)pc, fcalls[i].nextpc_to_string().c_str());
    if (fcalls[i].get_tag() == tag_const) {
      dfs_find_bbl_leaders(in, fcalls[i].get_val(), translated_addrs);
    }
  }
  dfs_find_bbl_leaders(in, bbl_end, translated_addrs);
  //free(nextpcs);
  free(pcs);
  //free(fcalls);
  delete[] fcalls;
}

static void
scan_data_section(input_exec_t *in, char const *secname,
    struct interval_set *translated_addrs)
{
  input_section_t *rodata = NULL;
  unsigned i;

  if (in->type == ET_REL) {
    return;
  }

  for (i = 0; i < in->num_input_sections; i++) {
    if (!strcmp(in->input_sections[i].name, secname)) {
      rodata = &in->input_sections[i];
    }
  }
  if (!rodata) {
    return;
  }
  DBG(CFG, "rodata->addr=%lx, end=%lx\n", (unsigned long)rodata->addr,
      (unsigned long)rodata->addr + rodata->size);

  src_long *ldata = (src_long *)rodata->data;

  for (i = 0; i < rodata->size/(sizeof(src_long)); i++) {
    src_long curloc = rodata->addr + i*sizeof(src_long);
    src_long val = tswap32(*(uint32_t *)(ldata + i));

    src_ulong candidate = (curloc + val);
    while (is_possible_nip(in, candidate)) {
      DBG(CFG, "curloc %lx. candidate code address: %lx\n", (long)curloc,
          (long)candidate);
      bbl_leaders_add(&in->bbl_leaders, candidate);
      DBG(CFG, "Adding %lx to jumptable\n", (long)candidate);
      jumptable_add(in->jumptable, candidate);

      DBG(CFG, "Added %lx to bbl_leaders and jumptable\n", (long)candidate);
      if (!visited(translated_addrs, candidate)) {
        DBG(CFG, "calling dfs_find_bbl_leaders(%lx)\n", (long)candidate);
        dfs_find_bbl_leaders(in, candidate, translated_addrs);
      }
      i++;
      val = tswap32(*(uint32_t *)(ldata + i));
      candidate = (curloc + val);
    }

    src_ulong candidate2 = val;
    if (is_possible_nip(in, candidate2)) {
      DBG(CFG, "curloc %lx. candidate2 code address: %lx\n", (long)curloc,
          (long)candidate2);
      bbl_leaders_add(&in->bbl_leaders, candidate2);
      DBG(CFG, "Adding %lx to jumptable\n", (long)candidate2);
      jumptable_add(in->jumptable, candidate2);

      DBG(CFG, "Added %x to bbl_leaders and jumptable\n", (unsigned)candidate2);

      if (!visited(translated_addrs, candidate2)) {
        DBG(CFG, "calling dfs_find_bbl_leaders(%x)\n", (unsigned)candidate2);
        dfs_find_bbl_leaders(in, candidate2, translated_addrs);
      }
    }
  }
}

static void
scan_code_section(input_exec_t *in, char const *secname,
    struct interval_set *translated_addrs,
    unsigned char input_exec_type)
{
  src_ulong pc, next, start, end;
  input_section_t *text = NULL;
  unsigned long insn_size;
  long i;

  CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": entry\n");

  for (i = 0; i < in->num_input_sections; i++) {
    if (!strcmp(in->input_sections[i].name, secname)) {
      text = &in->input_sections[i];
    }
  }
  start = text->addr, end = text->addr + text->size;


  for (pc = start; pc < end; pc = next) {
    uint32_t candidate;
    src_insn_t insn;
    bool fetch;
    int i;

    CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": scanning pc address 0x" << hex << pc << dec << endl);
    fetch = src_insn_fetch(&insn, in, pc, &insn_size);
    next = pc + insn_size;
    if (!fetch || !is_possible_nip(in, pc)) {
      continue;
    }
    fetch = src_insn_get_constant_operand(&insn, &candidate);
    if (fetch && is_possible_nip(in, candidate) && candidate != next && input_exec_type == ET_EXEC) {
      bbl_leaders_add(&in->bbl_leaders, candidate);
      DBG(CFG, "Adding %lx to jumptable\n", (long)candidate);
      jumptable_add(in->jumptable, candidate);
      if (!visited(translated_addrs, candidate)) {
        DBG(CFG, "calling dfs_find_bbl_leaders(%lx)\n", (long)candidate);
        dfs_find_bbl_leaders(in, candidate, translated_addrs);
      }
    }
    CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": calling pc2function_name on pc 0x" << hex << pc << dec << endl);
    symbol_t const *symbol;
    if (symbol = pc_has_associated_symbol_of_func_or_notype(in, pc)) {
      DBG(CFG, "adding pc %lx to jumptable because it has associated symbol %s.\n",
          (long)pc, symbol->name);
      bbl_leaders_add(&in->bbl_leaders, pc);
      jumptable_add(in->jumptable, pc);
      if (!visited(translated_addrs, pc)) {
        DBG(CFG, "calling dfs_find_bbl_leaders(%lx)\n", (long)pc);
        dfs_find_bbl_leaders(in, pc, translated_addrs);
      }
    } else {
      CPP_DBG_EXEC2(CFG, cout << __func__ << " " << __LINE__ << ": pc 0x" << hex << pc << dec << " is not a function symbol\n");
    }
  }
}

static void
scan_code_sections(input_exec_t *in, struct interval_set *translated_addrs, unsigned char input_exec_type)
{
  /* keep fetching instructions. If the instruction encodes a constant that lies
   * between the code sections, mark it as a jumptable address. */
  DBG (CFG, "%s","calling scan_code_section(.srctext)\n");
  scan_code_section(in, ".srctext", translated_addrs, input_exec_type);
}

static void
mark_section_boundaries_as_bbls(input_exec_t *in, struct interval_set *translated_addrs)
{
  unsigned i;
  for (i = 0; i < in->num_input_sections; i++) {
    if (is_code_section(&in->input_sections[i])) {
      src_ulong start, end;

      start = in->input_sections[i].addr;
      end = strcmp(in->input_sections[i].name, ".got")?
        in->input_sections[i].addr + in->input_sections[i].size:
        got_addr(in);

      bbl_leaders_add(&in->bbl_leaders, start);
      bbl_leaders_add(&in->bbl_leaders, end);

      if (!visited(translated_addrs, start) && is_executable(in, start)) {
        DBG(CFG, "calling dfs_find_bbl_leaders(%lx)\n", (long)start);
        dfs_find_bbl_leaders(in, start, translated_addrs);
      }

      DBG (CFG, "Inserted %x to bbl_leaders\n", (unsigned)start);
      DBG (CFG, "Inserted %x to bbl_leaders\n", (unsigned)end);
    }
  }
}



static void
determine_bbl_leaders_and_jumptable_targets(input_exec_t *in)
{
  struct interval_set translated_addrs;
  interval_set_init(&translated_addrs, &addr_range_cmp, NULL);

  rbtree_init(&in->bbl_leaders, &pc_less, NULL);

  jumptable_init(in);

  DBG(CFG, "calling dfs_find_bbl_leaders(%lx)\n", (unsigned long)in->entry);
#if ARCH_SRC == ARCH_ARM
  bbl_leaders_add(&in->bbl_leaders,in->entry);
#endif
  dfs_find_bbl_leaders(in, in->entry, &translated_addrs);
  DBG(CFG, "%s", "calling mark_section_boundaries_as_bbls()\n");
  mark_section_boundaries_as_bbls(in, &translated_addrs);

  if (in->fresh) {
    DBG (CFG, "%s","scanning data and code sections\n");
    scan_data_section(in, ".rodata", &translated_addrs);
    scan_data_section(in, ".sdata", &translated_addrs);
    scan_data_section(in, ".data", &translated_addrs);
    scan_data_section(in, ".got", &translated_addrs);
    scan_data_section(in, ".got.plt", &translated_addrs);
    scan_data_section(in, ".ctors", &translated_addrs);
    scan_data_section(in, ".dtors", &translated_addrs);
    scan_data_section(in, "__libc_atexit", &translated_addrs);
    scan_data_section(in, "__libc_subfreeres", &translated_addrs);
    scan_code_sections(in, &translated_addrs, in->type);
  } else {
    int i;
    for (i = 0; i < in->num_input_sections; i++) {
      if (strstart(in->input_sections[i].name, ".jumptable.", NULL)) {
        Elf32_Addr addr;
        char *ldata = (char *)in->input_sections[i].data;
        for (addr = 0; addr < in->input_sections[i].size; addr += 4) {
          src_ulong val = tswap32(*(uint32_t *)(ldata + addr));
          if (!val) {
            continue;
          }
          DBG(CFG, "Added %x to bbl_leaders and jumptable at loc 0x%x due "
              "to '%s'\n", (unsigned)val,
              (unsigned)(in->input_sections[i].addr + addr), in->input_sections[i].name);
          ASSERT(is_possible_nip(in, val));
          bbl_leaders_add(&in->bbl_leaders, val);
          DBG(CFG, "Adding %lx to jumptable\n", (long)val);
          jumptable_add(in->jumptable, val);
          if (!visited(&translated_addrs, val)) {
            DBG(CFG, "calling dfs_find_bbl_leaders(%lx)\n", (long)val);
            dfs_find_bbl_leaders(in, val, &translated_addrs);
          }
        }
      }
    }
  }
  interval_set_destroy(&translated_addrs, &addr_range_free);
}

static void
compute_postorder(input_exec_t const *in, long num_bbls, long bbl_index,
    cfg_edge_t * const *fwd_edges,
    long *postorder_indices, long *postorder_indices_pos, char *visited)
{
  ASSERT(bbl_index >= 0 && bbl_index < num_bbls);

  if (visited[bbl_index]) {
    return;
  }
  visited[bbl_index] = 1;

  cfg_edge_t const *iter;
  for (iter = fwd_edges[bbl_index]; iter; iter = iter->next) {
    DBG_EXEC(DOMINATORS,
        if (in) {
          printf("following edge %lx -> %lx\n", (long)in->bbls[bbl_index].pc,
              (long)in->bbls[iter->dst].pc);
        }
    );
    compute_postorder(in, num_bbls, iter->dst, fwd_edges, postorder_indices,
        postorder_indices_pos, visited);
  }
  postorder_indices[*postorder_indices_pos] = bbl_index;
  (*postorder_indices_pos)++;
}

static int 
dominators_intersect(long const *dominators, long num_bbls, long const *postorder,
    long i, long j)
{
  long finger1 = postorder[i];
  long finger2 = postorder[j];

  while (finger1 != finger2) {
    ASSERT(i >= 0 && i < num_bbls);
    ASSERT(j >= 0 && j < num_bbls);

    while (finger1 < finger2) {
      /*DBG(DOMINATORS, "finger1 at %lx: %ld, finger2 at %lx: %ld\n",
          (long)in->bbls[i].pc, finger1, (long)in->bbls[j].pc, finger2);*/
      i = dominators[i];
      finger1 = postorder[i];
    }

    while (finger2 < finger1) {
      /*DBG(DOMINATORS, "finger1 at %lx: %ld, finger2 at %lx: %ld\n", (long)in->bbls[i].pc,
          finger1, (long)in->bbls[j].pc, finger2);*/
      j = dominators[j];
      finger2 = postorder[j];
    }
  }
  return i;
}

static bool
bbl_has_jumptable_or_undefined_successor(bbl_t const *bbl)
{
  long i;
  for (i = 0; i < bbl->num_nextpcs; i++) {
    if (bbl->nextpcs[i].get_val() == PC_JUMPTABLE || bbl->nextpcs[i].get_val() == PC_UNDEFINED) {
      return true;
    }
  }
  return false;
}

static void
bbl_build_edges(cfg_edge_t **fwd_edges, cfg_edge_t **bwd_edges, input_exec_t const *in,
    char *visited, long bbl_id, nextpc_t const *nextpcs, long num_nextpcs)
{
  long j;

  for (j = 0; j < num_nextpcs; j++) {
    long nextbbl;
    if (   nextpcs[j].get_tag() != tag_const
        || nextpcs[j].get_val() == PC_UNDEFINED
        || nextpcs[j].get_val() == PC_JUMPTABLE) {
      continue;
    }
    nextbbl = pc2bbl(&in->bbl_map, nextpcs[j].get_val());
    if (nextbbl == (unsigned long)-1) {
      continue;
    }
    ASSERT(nextbbl >= 0 && nextbbl < in->num_bbls);

    visited[nextbbl] = 1;
    add_cfg_edge(fwd_edges, bbl_id, nextbbl);
    DBG(DOMINATORS, "adding fwd_edge: %lx->%lx\n", (long)in->bbls[bbl_id].pc,
        (long)in->bbls[nextbbl].pc);
    add_cfg_edge(bwd_edges, nextbbl, bbl_id);
  }
}

static void
build_edges(cfg_edge_t **fwd_edges, cfg_edge_t **bwd_edges, input_exec_t const *in)
{
  long num_bbls_aug = in->num_bbls + 2;
  char visited[in->num_bbls];
  long i;

  memset(visited, 0x0, sizeof visited);

  for (i = 0; i < in->num_bbls; i++) {
    bbl_t *bbl;
    long j;

    bbl = &in->bbls[i];
    DBG(DOMINATORS, "%s() %d: bbl %ld %lx: reachable = %d\n", __func__, __LINE__, (long)i, (unsigned long)bbl->pc, (int)bbl->reachable);
    if (!bbl->reachable) {
      continue;
    }
    bbl_build_edges(fwd_edges, bwd_edges, in, visited, i, bbl->nextpcs,
        bbl->num_nextpcs);
    bbl_build_edges(fwd_edges, bwd_edges, in, visited, i, bbl->fcalls,
        bbl->num_fcalls);
    /* if (tb->next_possible_eip[2]) {
      struct imap_elem_t needle;
      struct hash_elem *found;
      long fret;

      needle.pc = tb->next_possible_eip[2];
      found = hash_find(&in->tb_map, &needle.h_elem);
      if (found) {
        fret = hash_entry(found, imap_elem_t, h_elem)->val;
        visited[fret] = 1;
        add_edge(fwd_edges, i, fret);
        DBG(DOMINATORS, "adding fwd_edge: %lx->%lx\n", in->bbls[i].pc,
            tb->next_possible_eip[2]);
        add_edge(bwd_edges, fret, i);
      }
    } */
  }

  long entrybbl = pc2bbl(&in->bbl_map, in->entry);
  ASSERT(entrybbl >= 0 && entrybbl < in->num_bbls);
  add_cfg_edge(fwd_edges, SOURCE_NODE(num_bbls_aug), entrybbl);
  add_cfg_edge(bwd_edges, entrybbl, SOURCE_NODE(num_bbls_aug));
  visited[entrybbl] = 1;
  for (i = 0; i < in->num_bbls; i++) {
    bbl_t *bbl;

    bbl = &in->bbls[i];
    if (/*   !visited[i]
        || */jumptable_belongs(in->jumptable, bbl->pc)) {
      add_cfg_edge(fwd_edges, SOURCE_NODE(num_bbls_aug), i);
      add_cfg_edge(bwd_edges, i, SOURCE_NODE(num_bbls_aug));
    }
    if (bbl_has_jumptable_or_undefined_successor(bbl)) {
      add_cfg_edge(fwd_edges, i, SINK_NODE(num_bbls_aug));
      add_cfg_edge(bwd_edges, SINK_NODE(num_bbls_aug), i);
    }
  }
}

void
free_cfg_edges(cfg_edge_t **edges, long n)
{
  long i;
  for (i = 0; i < n; i++) {
    cfg_edge_t *iter, *prev;
    for (iter = edges[i]; iter; prev=iter, iter=iter->next, free(prev));
    edges[i] = NULL;
  }
}

static src_ulong
get_bbl_leader(struct rbtree const *bbl_leaders, src_ulong pc)
{
  struct rbtree_elem *found;
  struct pc_elem_t lb;

  lb.pc = pc;
  found = rbtree_lower_bound(bbl_leaders, &lb.rb_elem);

  if (!found) {
    return PC_UNDEFINED;
  }

  return rbtree_entry(found, struct pc_elem_t, rb_elem)->pc;
}

static bool
__pc_dominates(input_exec_t const *in, long const *dominators, src_ulong pc1,
    src_ulong pc2, bool less_dominates)
{
  src_ulong bbl1_pc, bbl2_pc;
  int bbl1_id, bbl2_id;

  if (pc1 == PC_JUMPTABLE || pc2 == PC_JUMPTABLE) {
    return false;
  }
  if (pc1 == PC_UNDEFINED || pc2 == PC_UNDEFINED) {
    return false;
  }
  if (!dominators) {
    return false;
  }
  if (pc1 == pc2) {
    return true;
  }

  bbl1_pc = get_bbl_leader(&in->bbl_leaders, pc1);
  bbl2_pc = get_bbl_leader(&in->bbl_leaders, pc2);

  if (bbl1_pc == PC_UNDEFINED || bbl2_pc == PC_UNDEFINED) {
#if ARCH_SRC == ARCH_ETFG
    NOT_REACHED();
#else
    return false;
#endif
  }
  ASSERT(bbl1_pc != PC_UNDEFINED);
  ASSERT(bbl2_pc != PC_UNDEFINED);
  if ((bbl1_pc == bbl2_pc) && (less_dominates == (pc1 < pc2))) {
    return true;
  }

  bbl1_id = pc2bbl(&in->bbl_map, bbl1_pc);
  if (bbl1_id == (unsigned long)-1) {
    return false;
  }
  ASSERT(bbl1_id >= 0 && bbl1_id < in->num_bbls);
  bbl2_id = pc2bbl(&in->bbl_map, bbl2_pc);
  if (bbl2_id < 0 || bbl2_id >= in->num_bbls) {
    printf("%s() %d: bbl2_id = %ld, bbl2_pc = %lx\n", __func__, __LINE__, (long)bbl2_id, (unsigned long)bbl2_pc);
  }
  ASSERT(bbl2_id >= 0 && bbl2_id < in->num_bbls);

  int i = bbl2_id;
  ASSERT(i >= 0 && i < in->num_bbls + 2);
  while (dominators[i] != -1) {
    ASSERT(dominators[i] >= 0 && dominators[i] < in->num_bbls + 2);
    if (dominators[i] == bbl1_id) {
      //printf("pc_dominates(%lx, %lx) returning true\n", (long)pc1, (long)pc2);
      return true;
    }
    if (i == dominators[i]) {
      break;
    }
    i = dominators[i];
  }
  return false;
}

bool
pc_dominates(input_exec_t const *in, src_ulong pc1, src_ulong pc2)
{
  return __pc_dominates(in, in->dominators, pc1, pc2, true);
}

bool
pc_postdominates(input_exec_t const *in, src_ulong pc1, src_ulong pc2)
{
  return __pc_dominates(in, in->post_dominators, pc1, pc2, false);
}

unsigned long
pc2bbl(struct chash const *map, src_ulong pc)
{
  struct myhash_elem *found;
  imap_elem_t needle;

  needle.pc = pc;
  found = myhash_find(map, &needle.h_elem);
  if (!found) {
    return (unsigned long)-1;
  }
  return chash_entry(found, imap_elem_t, h_elem)->val;
}

unsigned long
pc_find_bbl(input_exec_t const *in, src_ulong pc)
{
  src_ulong bbl_leader;
  unsigned long ret;

  bbl_leader = get_bbl_leader(&in->bbl_leaders, pc);
  ASSERT(bbl_leader != PC_UNDEFINED);
  ret = pc2bbl(&in->bbl_map, bbl_leader);
  return ret;
  //if (!(ret >= 0 && ret < in->num_bbls)) {
  //  //cout << __func__ << " " << __LINE__ << ": pc = 0x" << hex << pc << ", bbl_leader = 0x" << bbl_leader << dec << ", ret = " << ret << endl;
  //}
  //ASSERT(ret >= 0 && ret < in->num_bbls);
  //return ret;
}

void
compute_dominators_helper(input_exec_t const *in, long *dominators, long num_bbls_aug,
    cfg_edge_t * const *fwd_edges, cfg_edge_t * const *bwd_edges, long start)
{
  /* Algorithm taken from "A Simple, Fast Dominance Algorithm" by
     Keith Cooper, Timothy Harvey and Ken Kennedy */
  long postorder_indices[num_bbls_aug];
  char visited[num_bbls_aug];
  long postorder_indices_pos;
  struct imap_elem_t needle;
  struct myhash_elem *found;
  long i;

  for (i = 0; i < num_bbls_aug; i++) {
    dominators[i] = -1;
  }
  dominators[start] = start;

  memset(visited, 0x0, sizeof visited);
  postorder_indices_pos = 0;

  compute_postorder(in, num_bbls_aug, start, fwd_edges, postorder_indices,
      &postorder_indices_pos, visited);
  DBG(DOMINATORS, "postorder_indices_pos=%ld, num_bbls_aug=%ld\n",
      postorder_indices_pos, num_bbls_aug);
  DBG_EXEC(DOMINATORS,
      if (in) {
        long i;
        printf("postorder:");
        for (i = 0; i < postorder_indices_pos; i++) {
          if (postorder_indices[i] < in->num_bbls) {
            printf (" %lx", (long)in->bbls[postorder_indices[i]].pc);
          } else {
            printf(" %s", (postorder_indices[i] == SOURCE_NODE(num_bbls_aug))?" <src>":" <sink>");
          }
        }
        printf ("\n");
      }
  );
  if (postorder_indices_pos != num_bbls_aug) {
    long n = postorder_indices_pos;
    for (i =0; i < num_bbls_aug; i++) {
      long j;
      for (j = 0; j < postorder_indices_pos; j++) {
        if (postorder_indices[j] == i) break;
      }
      if (j == postorder_indices_pos) {
        postorder_indices[n++] = i;
      }
    }
    ASSERT(n == num_bbls_aug);

    DBG_EXEC(DOMINATORS,
        if (in) {
          long i;
          printf("Unreachable postorder bbls (%ld):",
              num_bbls_aug - postorder_indices_pos);
          for (i = postorder_indices_pos; i < num_bbls_aug; i++) {
            if (postorder_indices[i] < in->num_bbls) {
              printf(" %lx", (long)in->bbls[postorder_indices[i]].pc);
            }
          }
          printf("\n");
        }
    );
    postorder_indices_pos = n;
  }

  long *postorder;

  postorder = (long *)malloc(num_bbls_aug * sizeof(long));
  ASSERT(postorder);

  for (i = 0; i < num_bbls_aug; i++) {
    ASSERT(postorder_indices[i] >= 0 && postorder_indices[i] < num_bbls_aug);
    postorder[postorder_indices[i]] = i;
  }

  long iter = 0;
  bool some_change = true;
  while (some_change) {
    some_change = false;
    long j;
    for (j = num_bbls_aug - 1; j >= 0; j--) {
      long i = postorder[j];
      //printf("bwd_edges[%ld]=%p\n", i, bwd_edges[i]);
      if (dominators[i] == i) {
        ASSERT(i == start);
        continue;
      }
      long new_idom = -1;
      cfg_edge_t const *iter;
      for (iter = bwd_edges[i]; iter; iter = iter->next) {
        //printf("iter->dst=%ld\n", (long)iter->dst);
        if (new_idom == -1 && dominators[iter->dst] != -1) {
          new_idom = iter->dst;
          DBG_EXEC(DOMINATORS,
              if (in) {
                printf("new_idom=%lx\n", (long)in->bbls[new_idom].pc);
              }
          );
        } else if (dominators[iter->dst] != -1) {
          long idom = dominators_intersect(dominators, num_bbls_aug, postorder,
              iter->dst, new_idom);
          DBG_EXEC(DOMINATORS,
              if (in) {
                printf("intersect(%lx,%lx) returned %lx\n",
                    (long)in->bbls[iter->dst].pc, (long)in->bbls[new_idom].pc,
                    (long)in->bbls[idom].pc);
              }
          );
          new_idom = idom;
        }
      }

      if (new_idom != -1 && dominators[i] != new_idom) {
        DBG_EXEC(DOMINATORS,
            if (in) {
              printf("changing dominators[%lx] from %lx to %lx\n",
                  (long)in->bbls[i].pc,
                  ((int)dominators[i] == -1)?0:(long)in->bbls[dominators[i]].pc,
                  (long)in->bbls[new_idom].pc);
            }
        );
        ASSERT(new_idom >= 0 && new_idom < num_bbls_aug);
        //ASSERT(postorder[new_idom] > postorder[i]);
        dominators[i] = new_idom;
        some_change = true;
      }
    }
    iter++;
  }
  free(postorder);
}

static void
compute_dominators_and_postdominators(input_exec_t *in)
{
  long num_bbls_aug = in->num_bbls + 2;
  cfg_edge_t *fwd_edges[num_bbls_aug], *bwd_edges[num_bbls_aug];
  long i;

  ASSERT(!in->dominators);
  in->dominators = (long *)malloc(num_bbls_aug * sizeof(long));
  ASSERT(in->dominators);

  ASSERT(!in->post_dominators);
  in->post_dominators = (long *)malloc(num_bbls_aug * sizeof(long));
  ASSERT(in->post_dominators);

  memset(fwd_edges, 0x0, sizeof fwd_edges);
  memset(bwd_edges, 0x0, sizeof bwd_edges);
  build_edges(fwd_edges, bwd_edges, in);

  /* Compute dominators. */
  compute_dominators_helper(in, in->dominators, num_bbls_aug, fwd_edges, bwd_edges, SOURCE_NODE(num_bbls_aug));
  DBG_EXEC(DOMINATORS,
      long i;
      printf("printing dominators (num_bbls %ld):\n", num_bbls_aug);
      for (i = 0; i < num_bbls_aug; i++) {
        printf(" %lx", in->dominators[i]);
      }
      printf("\n");
  );

  /* Compute postdominators. */
  /*num_start_bbls = 0;
  for (i = 0; i < in->num_bbls; i++) {
    if (bbl_has_jumptable_successor(&in->bbls[i])) {
      start_bbls[num_start_bbls] = i;
      num_start_bbls++;
    }
  }*/

  compute_dominators_helper(in, in->post_dominators, num_bbls_aug, bwd_edges, fwd_edges, SINK_NODE(num_bbls_aug));
  DBG_EXEC(DOMINATORS,
      long i;
      printf("printing postdominators (num_bbls %ld):\n",
      num_bbls_aug);
      for (i = 0; i < num_bbls_aug; i++) {
        printf(" %lx", in->post_dominators[i]);
      }
      printf("\n");
  );

  free_cfg_edges(fwd_edges, num_bbls_aug);
  free_cfg_edges(bwd_edges, num_bbls_aug);
}

set<pair<long, long>>
input_exec_t::identify_backedges() const
{
  input_exec_t const *in = this;
  set<pair<long, long>> ret;
  for (long i = 0; i < in->num_bbls; i++) {
    bbl_t const *bbl = &in->bbls[i];
    for (long n = 0; n < bbl->num_nextpcs; n++) {
      if (bbl->nextpcs[n].get_tag() != tag_const) {
        continue;
      }
      src_ulong nextpc = bbl->nextpcs[n].get_val();
      if (nextpc == PC_UNDEFINED || nextpc == PC_JUMPTABLE) {
        continue;
      }
      long idx = pc_find_bbl(this, nextpc);
      if (idx == -1) {
        continue;
      }
      ASSERT(idx >= 0 && idx < in->num_bbls);
      if (pc_dominates(in, idx, i)) {
        ret.insert(make_pair(i, idx));
      }
    }
  }
  return ret;
}

set<long>
input_exec_t::identify_bbls_that_reach_tail_without_going_through_stop_bbls(long tail, set<long> const &stop_bbls) const
{
  if (set_belongs(stop_bbls, tail)) {
    return set<long>();
  }
  input_exec_t const *in = this;
  bbl_t const *bbl_tail = &in->bbls[tail];
  set<long> nstop_bbls = stop_bbls;
  nstop_bbls.insert(tail);
  set<long> ret;
  ret.insert(tail);
  for (unsigned long i = 0; i < bbl_tail->num_preds; i++) {
    src_ulong pred = bbl_tail->preds[i];
    if (pred == PC_UNDEFINED || pred == PC_JUMPTABLE) {
      continue;
    }
    long idx = pc_find_bbl(in, pred);
    ASSERT(idx >= 0 && idx < in->num_bbls);
    set<long> s = this->identify_bbls_that_reach_tail_without_going_through_stop_bbls(idx, nstop_bbls);
    set_union(ret, s);
  }
  return ret;
}

set<long>
input_exec_t::identify_bbls_that_reach_tail_without_going_through_head(long tail, long head) const
{
  input_exec_t const *in = this;
  //bbl_t const *bbl_tail = &in->bbls[tail];
  set<long> stop_bbls;
  stop_bbls.insert(head);
  return identify_bbls_that_reach_tail_without_going_through_stop_bbls(tail, stop_bbls);
}

map<long, set<long>>
input_exec_t::identify_natural_loops() const
{
  input_exec_t const *in = this;
  set<pair<long, long>> backedges = this->identify_backedges();
  map<long, set<long>> ret;
  for (const auto &backedge : backedges) {
    long tail = backedge.first;
    long head = backedge.second;
    set<long> loop = this->identify_bbls_that_reach_tail_without_going_through_head(tail, head);
    //cout << __func__ << " " << __LINE__ << ": looking at backedge " << hex << in->bbls[tail].pc << "->" << in->bbls[head].pc << ": loop size = " << loop.size() << ":";
    //for (const auto &l : loop) {
    //  cout << " " << hex << in->bbls[l].pc << dec;
    //}
    //cout << endl;
    if (ret.count(head)) {
      set<long> const &existing_loop = ret.at(head);
      set_union(loop, existing_loop);
      ret[head] = loop;
    } else {
      ret.insert(make_pair(head, loop));
    }
  }
  return ret;
}

//size_t
//input_exec_t::get_symbol_size(char const* symbol_name) const
//{
//  for (const auto &s : this->symbol_map) {
//    if (!strcmp(s.second.name, symbol_name)) {
//      return s.second.size;
//    }
//  }
//  NOT_REACHED();
//}

align_t
input_exec_t::get_symbol_alignment(symbol_id_t symbol_id) const
{
  const auto &s = this->symbol_map.at(symbol_id);
  int shndx = s.shndx;
  for (size_t i = 0; i < this->num_input_sections; i++) {
    if (this->input_sections[i].orig_index == shndx) {
      return this->input_sections[i].align;
    }
  }
  return 1;
}

bool
input_exec_t::symbol_is_rodata(symbol_t const& s) const
{
  Elf_Half section_index = s.shndx;
  //cout << __func__ << " " << __LINE__ << ": checking symbol " << s.name << endl;

  for (int i =  0; i < this->num_input_sections; i++) {
    if (this->input_sections[i].orig_index == section_index) {
      //cout << __func__ << " " << __LINE__ << ": checking section number " << i << " name " << this->input_sections[i].name << endl;
      if (strstart(this->input_sections[i].name, RODATA_SECTION_NAME, nullptr)) {
        return true;
      } else {
        return false;
      }
    }
  }
  return false;
}

graph_symbol_t
input_exec_t::get_graph_symbol_for_symbol_or_section_id(symbol_or_section_id_t const& ssid) const
{
  if (ssid.is_symbol()) {
    symbol_id_t symbol_id = ssid.get_index();
    ASSERT(this->symbol_map.count(symbol_id));
    symbol_t const& sym = this->symbol_map.at(symbol_id);
    align_t align = this->get_symbol_alignment(symbol_id);
    return graph_symbol_t(mk_string_ref(sym.name), sym.size, align, false);
  } else if (ssid.is_section()) {
    int section_idx = ssid.get_index();
    input_section_t const& isec = this->input_sections[section_idx];
    //cout << __func__ << " " << __LINE__ << ": isec.name = " << isec.name << endl;
    return graph_symbol_t(mk_string_ref(isec.name), isec.size, isec.align, strstart(isec.name, RODATA_SECTION_NAME, nullptr));
  } else NOT_REACHED();
}

int
input_exec_t::get_section_index_for_orig_index(int orig_index) const
{
  for (int i = 0; i < this->num_input_sections; i++) {
    if (this->input_sections[i].orig_index == orig_index) {
      return i;
    }
  }
  NOT_REACHED();
}

static long
get_innermost_natural_loop_index(map<long, set<long>> const &nloops, long bbl_index)
{
  long ret = -1;
  long min_setsize;
  for (const auto &nloop : nloops) {
    if (set_belongs(nloop.second, bbl_index)) {
      if (ret == -1 || min_setsize > nloop.second.size()) {
        ret = nloop.first;
        min_setsize = nloop.second.size();
      }
    }
  }
  return ret;
}

static void
compute_innermost_natural_loops(input_exec_t *in)
{
  ASSERT(!in->innermost_natural_loop);
  in->innermost_natural_loop = new long[in->num_bbls];
  ASSERT(in->innermost_natural_loop);
  //vector<pair<src_ulong, src_ulong>> back_edges = in->identify_back_edges();
  map<long, set<long>> nloops = in->identify_natural_loops();
  for (long i = 0; i < in->num_bbls; i++) {
    in->innermost_natural_loop[i] = get_innermost_natural_loop_index(nloops, i);
  }
}

static bool
pc_is_function_return(input_exec_t const *in, src_ulong pc)
{
  unsigned long insn_size;
  src_insn_t insn;
  bool fetch;

  fetch = src_insn_fetch(&insn, in, pc, &insn_size);
  if (!fetch) {
    return false;
  }
  //uint32_t opcode = ldl_input(pc);
  //ppc_insn_init(&insn);
  //int disas = ppc_disassemble(&insn, (char *)&opcode);
  //if (disas!=4) { return 0; }
  //return (ppc_insn_is_function_return(&insn));
  return (src_insn_is_function_return(&insn));
}

static regset_t const *
live_jumptable()
{
  static regset_t ret;
  static bool init = false;
  if (!init) {
#if ARCH_SRC == ARCH_ETFG
    regset_copy(&ret, regset_empty());
    /*for (int i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
      reg_identifier_t reg_id(LLVM_CALLEE_SAVE_REGNAME);
      ret.excregs[ETFG_EXREG_GROUP_GPRS][reg_id] = MAKE_MASK(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS));
    }*/
#else
    regset_copy(&ret, regset_empty());
#endif
    init = true;
  }
  return &ret;
}

static live_ranges_t const *
live_ranges_jumptable()
{
  static live_ranges_t ret;
  static bool init = false;
  if (!init) {
#if ARCH_SRC == ARCH_ETFG
    live_ranges_copy(&ret, live_ranges_zero());
    /*for (int i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
      reg_identifier_t reg_id(LLVM_CALLEE_SAVE_REGNAME);
      ret.lr_extable[ETFG_EXREG_GROUP_GPRS][reg_id] = 0;
    }*/
#else
    live_ranges_copy(&ret, live_ranges_zero());
#endif
    init = true;
  }
  return &ret;

}

static void
compute_liveness(input_exec_t *in, int max_iter)
{
  //cout << __func__ << " " << __LINE__ << ": entry" << endl;
  live_ranges_t jumptable_live_ranges;
  regset_t jumptable_live_regs;
  bool some_change;
  int num_iter = 0;

  /* Initialize jumptable's live_regs set to empty set */
  //regset_copy(&jumptable_live_regs, regset_empty());
  regset_copy(&jumptable_live_regs, live_jumptable());
  live_ranges_copy(&jumptable_live_ranges, live_ranges_jumptable());
  some_change = true;
  long niter = 0;

  while (some_change || num_iter < max_iter) {
    long i;

    CPP_DBG_EXEC(COMPUTE_LIVENESS, cout << __func__ << " " << __LINE__ << ": niter = " << niter << endl);
    niter++;
    if (!some_change) {
      num_iter++;
    }
    some_change = false;

    for (i = in->num_bbls - 1; i >= 0; i--) {
      live_ranges_t cur_live_ranges;
      regset_t orig_in, orig_out;
      regset_t cur_live_set;
      long j, n_insns;
      bbl_t *bbl;

      regset_clear(&orig_in);
      regset_clear(&orig_out);
      regset_clear(&cur_live_set);

      bbl = &in->bbls[i];
      regset_copy(&cur_live_set, regset_empty());
      live_ranges_copy(&cur_live_ranges, live_ranges_zero());

      CPP_DBG_EXEC(COMPUTE_LIVENESS, cout << __func__ << " " << __LINE__ << ": looking at bbl " << hex << bbl->pc << dec << endl);

      for (j = 0; j < bbl->num_nextpcs; j++) {
        unsigned long nextbbl_num;
        bbl_t *nextbbl;

        double wj;
        if (bbl->nextpcs[j].get_tag() != tag_const) {
          continue;
        }

        CPP_DBG_EXEC(COMPUTE_LIVENESS, cout << __func__ << " " << __LINE__ << ": bbl->nextpc[" << j << "] = " << bbl->nextpcs[j].nextpc_to_string() << endl);

        wj = bbl->nextpc_probs[j];
        if (bbl->nextpcs[j].get_val() == PC_UNDEFINED) {
          continue;
        }

        if (bbl->nextpcs[j].get_val() == PC_JUMPTABLE) {
          regset_t succ_live_set;
          live_ranges_t succ_live_ranges;

          regset_copy(&succ_live_set, &jumptable_live_regs);
          live_ranges_copy(&succ_live_ranges, &jumptable_live_ranges);
          if (use_calling_conventions) {
#if ARCH_SRC == ARCH_I386 || ARCH_SRC == ARCH_X64
            if (pc_is_function_return(in, bbl_last_insn(bbl))) {
              regset_t retregs;
              regset_clear(&retregs);
              regset_mark_ex_mask(&retregs, SRC_EXREG_GROUP_GPRS, R_EAX,
                  MAKE_MASK(SRC_EXREG_LEN(SRC_EXREG_GROUP_GPRS)));
              regset_union(&succ_live_set, &retregs);
              regset_diff(&succ_live_set, src_volatile_regs());
              live_ranges_set(&succ_live_ranges, &retregs);
            }
#elif ARCH_SRC == ARCH_PPC || ARCH_SRC == ARCH_ARM
            if ((   bbl->nextpcs[j].get_val() == PC_JUMPTABLE
                 && pc_is_function_return(in, bbl_last_insn(bbl)))
                /*check if this works --> || is_function_call(tb->pc + tb->size - 4)*/) {
              regset_diff(&succ_live_set, src_volatile_regs());
            }
#elif ARCH_SRC == ARCH_ETFG
            if (pc_is_function_return(in, bbl_last_insn(bbl))) {
              /*regset_t callee_save;
              regset_clear(&callee_save);
              for (size_t i = 0; i < regset_count_exregs(&dst_callee_save_regs, ETFG_EXREG_GROUP_CALLEE_SAVE_REGS); i++) {
                stringstream ss;
                ss << G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
                reg_identifier_t regnum = get_reg_identifier_for_regname(ss.str());
                regset_mark_ex_mask(&succ_live_set, ETFG_EXREG_GROUP_CALLEE_SAVE_REGS, regnum,
                    MAKE_MASK(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_CALLEE_SAVE_REGS)));
                regset_mark_ex_mask(&callee_save, ETFG_EXREG_GROUP_CALLEE_SAVE_REGS, regnum,
                    MAKE_MASK(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_CALLEE_SAVE_REGS)));
              }
              live_ranges_set(&cur_live_ranges, &callee_save);*/
              regset_t retregs = etfg_get_retregs_at_pc(in, bbl_last_insn(bbl));
              regset_union(&succ_live_set, &retregs);
              live_ranges_set(&succ_live_ranges, &retregs);
              //cout << __func__ << " " << __LINE__ << ": retregs = " << regset_to_string(&retregs, as1, sizeof as1) << endl;
              //cout << __func__ << " " << __LINE__ << ": succ_live_set = " << regset_to_string(&succ_live_set, as1, sizeof as1) << endl;
              //cout << __func__ << " " << __LINE__ << ": cur_live_ranges = " << live_ranges_to_string(&cur_live_ranges, &succ_live_set, as1, sizeof as1) << endl;
            }
#else
#error "unknown src arch"
#endif
          }
          CPP_DBG_EXEC(COMPUTE_LIVENESS, cout << __func__ << " " << __LINE__ << ": bbl " << hex << bbl->pc << dec << ": succ_live_set = " << regset_to_string(&succ_live_set, as1, sizeof as1) << endl);
          regset_union(&cur_live_set, &succ_live_set);
          live_ranges_combine(&cur_live_ranges, &succ_live_ranges, wj);
          continue;
        }

        nextbbl_num = pc2bbl(&in->bbl_map, bbl->nextpcs[j].get_val());
        if (nextbbl_num == (unsigned long)-1) {
          continue;
        }
        ASSERT(nextbbl_num >= 0 && nextbbl_num < in->num_bbls);
        nextbbl = &in->bbls[nextbbl_num];
        if (nextbbl->live_regs) {
          regset_union(&cur_live_set, &nextbbl->live_regs[0]);
          live_ranges_combine(&cur_live_ranges, &nextbbl->live_ranges[0], wj);
        } else {
          regset_union(&cur_live_set, regset_empty());
          live_ranges_combine(&cur_live_ranges, live_ranges_zero(), wj);
        }
      }

      live_ranges_t lr_orig_in, lr_orig_out;
      n_insns = bbl_num_insns(bbl);
      if (!bbl->live_regs) {
        //bbl->live_regs = (regset_t *)malloc((n_insns + 1)*sizeof(regset_t));
        bbl->live_regs = new regset_t[n_insns + 1];
        ASSERT(bbl->live_regs);
        for (int i = 0; i < n_insns + 1; i++) {
          regset_clear(&bbl->live_regs[i]);
        }
        regset_copy(&orig_in, regset_empty());
        regset_copy(&orig_out, regset_empty());
      } else {
        regset_copy(&orig_in, &bbl->live_regs[0]);
        regset_copy(&orig_out, &bbl->live_regs[n_insns]);
      }
      if (!bbl->live_ranges) {
        //bbl->live_ranges = (live_ranges_t*)malloc((n_insns + 1)*sizeof(live_ranges_t));
        bbl->live_ranges = new live_ranges_t[n_insns + 1];
        ASSERT(bbl->live_ranges);
        live_ranges_copy(&bbl->live_ranges[n_insns], live_ranges_zero());
        live_ranges_copy(&lr_orig_in, live_ranges_zero());
        live_ranges_copy(&lr_orig_out, live_ranges_zero());
      } else {
        live_ranges_copy(&lr_orig_in, &bbl->live_ranges[0]);
        live_ranges_copy(&lr_orig_out, &bbl->live_ranges[n_insns]);
      }


      //cout << __func__ << " " << __LINE__ << ": cur_live_set = " << regset_to_string(&cur_live_set, as1, sizeof as1) << endl;

      regset_copy(&bbl->live_regs[n_insns], &cur_live_set);
      live_ranges_copy(&bbl->live_ranges[n_insns], &cur_live_ranges);

      src_ulong pc, succ_pc;
      long index;
      for (succ_pc = bbl->pc + bbl->size, pc = bbl_last_insn(bbl);
          pc != PC_UNDEFINED;
          succ_pc = pc, pc = bbl_prev_insn(bbl, pc)) {
        regset_t use, def;
        src_insn_t insn;

        if (src_insn_fetch(&insn, in, pc, NULL)) {
          src_insn_get_usedef_regs(&insn, &use, &def);

          regset_diff(&cur_live_set, &def);
          regset_union(&cur_live_set, &use);
          live_ranges_reset(&cur_live_ranges, &def);
          live_ranges_set(&cur_live_ranges, &use);
#if ARCH_SRC != ARCH_ETFG
          if ((index = array_search(bbl->fcall_returns, bbl->num_fcalls, succ_pc))
              != -1) {
            regset_t live_at_target;
            nextpc_t fcall_target;
            src_insn_t insn;
            bool ret;

            fcall_target = bbl->fcalls[index];
            if (fcall_target.get_tag() != tag_const || fcall_target.get_val() == PC_JUMPTABLE) {
              regset_copy(&live_at_target, &jumptable_live_regs);
            } else {
              ASSERT(fcall_target.get_val() != PC_UNDEFINED);
              long fcall_target_bbl;
              fcall_target_bbl = pc2bbl(&in->bbl_map, fcall_target.get_val());
              ASSERT(fcall_target_bbl >= 0 && fcall_target_bbl < in->num_bbls);
              ASSERT(bbl_first_insn(&in->bbls[fcall_target_bbl]) == fcall_target.get_val());
              if (in->bbls[fcall_target_bbl].live_regs) {
                regset_copy(&live_at_target, &in->bbls[fcall_target_bbl].live_regs[0]);
              } else {
                regset_copy(&live_at_target, regset_empty());
              }
            }
            regset_diff(&live_at_target, &def);
            regset_union(&cur_live_set, &live_at_target);
          }
#endif
        }
        //cout << __func__ << " " << __LINE__ << ": pc = " << hex << pc << dec << ": " << regset_to_string(&cur_live_set, as1, sizeof as1) << endl;
        regset_copy(&bbl->live_regs[bbl_insn_index(bbl, pc)], &cur_live_set);

        live_ranges_decrement(&cur_live_ranges);
        live_ranges_copy(&bbl->live_ranges[bbl_insn_index(bbl, pc)], &cur_live_ranges);
      }

      if (   !regset_equal(&bbl->live_regs[n_insns], &orig_out)
          || !regset_equal(&bbl->live_regs[0], &orig_in)) {
        some_change = true;

#if ARCH_SRC != ARCH_ETFG
        if (jumptable_belongs(in->jumptable, in->bbls[i].pc)) {
          regset_union(&jumptable_live_regs, &bbl->live_regs[0]);
        }
#endif

        if (use_calling_conventions) {
#if 0
          if (is_function_return_target(in->bbls[i].pc)) {
            regset_diff (&in->bbls[in->global_sink_tb].live_regs[0],
                src_volatile_regs());
          }
#endif
          /*
             int i;
             for (i = 0; i < 8; i++) {
             regset_unmark (&in->bbls[in->global_sink_tb].live_regs[0],
             PPC_REG_CRFN(i));
             }
             */
        }
      }
    }
  }
  //cout << __func__ << " " << __LINE__ << ": exit" << endl;
}

/*static void
compute_live_ranges(input_exec_t *in, int max_iter)
{
  live_ranges_t jumptable_live_ranges;
  bool some_change = true;
  int num_iter = 0;

  //live_ranges_copy(&jumptable_live_ranges, live_ranges_zero());
  live_ranges_copy(&jumptable_live_ranges, live_ranges_jumptable());

  while (some_change && num_iter < max_iter) {
    long i;

    some_change = false;
    num_iter++;

    for (i = in->num_bbls - 1; i >= 0; i--) {
      live_ranges_t cur_live_ranges;
      bbl_t *bbl;
      long j;

      bbl = &in->bbls[i];
      live_ranges_copy(&cur_live_ranges, live_ranges_zero());

      for (j = 0; j < bbl->num_nextpcs; j++) {
        unsigned long nextbbl_num;
        bbl_t *nextbbl;
        double wj;

        wj = bbl->nextpc_probs[j];
        if (bbl->nextpcs[j] == PC_UNDEFINED) {
          continue;
        }
        if (bbl->nextpcs[j] == PC_JUMPTABLE) {
          if (use_calling_conventions) {
#if ARCH_SRC == ARCH_ETFG
            if (pc_is_function_return(in, bbl_last_insn(bbl))) {
              for (size_t i = 0; i < regset_count_exregs(&dst_callee_save_regs, ETFG_EXREG_GROUP_CALLEE_SAVE_REGS); i++) {
                stringstream ss;
                ss << G_SRC_KEYWORD "." LLVM_CALLEE_SAVE_REGNAME << "." << i;
                regset_mark_ex_mask(&succ_live_set, ETFG_EXREG_GROUP_CALLEE_SAVE_REGS, ss.str(),
                    MAKE_MASK(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_CALLEE_SAVE_REGS)));
              }
            }
#else
            NOT_IMPLEMENTED();
#endif
          }
          live_ranges_combine(&cur_live_ranges, &jumptable_live_ranges, wj);
          continue;
        }
        nextbbl_num = pc2bbl(&in->bbl_map, bbl->nextpcs[j]);
        if (nextbbl_num == (unsigned long)-1) {
          continue;
        }
        ASSERT(nextbbl_num >= 0 && nextbbl_num < in->num_bbls);
        nextbbl = &in->bbls[nextbbl_num];
        if (nextbbl->live_ranges) {
          live_ranges_combine(&cur_live_ranges, &nextbbl->live_ranges[0], wj);
        } else {
          live_ranges_combine(&cur_live_ranges, live_ranges_zero(), wj);
        }
      }

      live_ranges_t orig_in, orig_out;
      int n_insns;

      n_insns = bbl_num_insns(bbl);
      if (!bbl->live_ranges) {
        //bbl->live_ranges = (live_ranges_t*)malloc((n_insns + 1)*sizeof(live_ranges_t));
        bbl->live_ranges = new live_ranges_t[n_insns + 1];
        ASSERT(bbl->live_ranges);
        live_ranges_copy(&bbl->live_ranges[n_insns], live_ranges_zero());
        live_ranges_copy(&orig_in, live_ranges_zero());
        live_ranges_copy(&orig_out, live_ranges_zero());
      } else {
        live_ranges_copy(&orig_in, &bbl->live_ranges[0]);
        live_ranges_copy(&orig_out, &bbl->live_ranges[n_insns]);
      }

      live_ranges_copy (&bbl->live_ranges[n_insns], &cur_live_ranges);

      src_ulong pc;
      for (pc = bbl_last_insn(bbl); pc != PC_UNDEFINED; pc = bbl_prev_insn(bbl, pc)) {
        regset_t use, def;
        src_insn_t insn;

        if (src_insn_fetch(&insn, in, pc, NULL)) {
          src_insn_get_usedef_regs(&insn, &use, &def);

          live_ranges_reset (&cur_live_ranges, &def);
          live_ranges_set (&cur_live_ranges, &use);
        }

        live_ranges_decrement(&cur_live_ranges);
        live_ranges_copy(&bbl->live_ranges[bbl_insn_index(bbl, pc)], &cur_live_ranges);
      }

      if (   !live_ranges_equal(&bbl->live_ranges[n_insns], &orig_out)
          || !live_ranges_equal(&bbl->live_ranges[0], &orig_in)) {
        some_change = 1;
      }
    }
  }

  TIME ("computed live_ranges using %d iterations\n", num_iter);
}*/




#if ARCH_SRC == ARCH_I386 || ARCH_SRC == ARCH_X64
static void
compute_insn_boundaries_for_section(input_exec_t const *in, input_section_t const *text,
    struct chash *boundaries)
{
  src_ulong start = text->addr, end = text->addr + text->size;
  unsigned long insn_size;
  src_ulong pc, next;

  for (pc = start; pc < end; pc = next) {
    struct imap_elem_t *imap_elem;
    src_insn_t insn;
    bool fetch;

    fetch = src_insn_fetch(&insn, in, pc, &insn_size);
    next = pc + insn_size;
    if (!fetch) {
      continue;
    }
    if (fetch && src_insn_is_unconditional_branch_not_fcall(&insn)) {
      while (ldub_input(in, next) == 0 && next < end) {
        next++;
      }
    }
    if (is_executable(in, pc)) {
      imap_elem = (struct imap_elem_t *)malloc(sizeof *imap_elem);
      ASSERT(imap_elem);
      imap_elem->pc = pc;
      imap_elem->val = 0;
      DBG(INSN_BOUNDARIES, "Inserting 0x%x into insn_boundaries\n", pc);
      myhash_insert(boundaries, &imap_elem->h_elem);
    }
  }
}

static void
compute_insn_boundaries(input_exec_t const *in, struct chash *boundaries)
{
  int i;
  myhash_init(boundaries, imap_hash_func, imap_equal_func, NULL);
  for (i = 0; i < in->num_input_sections; i++) {
    if (is_code_section(&in->input_sections[i])) {
      compute_insn_boundaries_for_section(in, &in->input_sections[i], boundaries);
    }
  }
}
#endif

bool
pc_is_executable(input_exec_t const *in, src_ulong pc)
{
#if ARCH_SRC == ARCH_ETFG
  ASSERT(in->get_executable_pcs().size() != 0);
  return set_belongs(in->get_executable_pcs(), pc);
#else
  src_ulong bbl_pc = get_bbl_leader(&in->bbl_leaders, pc);
  if (bbl_pc == PC_UNDEFINED) {
    return false;
  }
  unsigned long bbl_id = pc2bbl(&in->bbl_map, bbl_pc);
  if (bbl_id == (unsigned long)-1) {
    return false;
  }
  return true;
#endif
}

bool
is_possible_nip(input_exec_t const *in, src_ulong pc)
{
  src_ulong nip = pc_get_orig_pc(pc);
#if ARCH_SRC == ARCH_PPC
  bool ret = (((nip % 4) == 0) && is_executable(in, nip));
  DBG(SRC_INSN, "returning %d\n", ret);
  return ret;
#elif ARCH_SRC == ARCH_I386 || ARCH_SRC == ARCH_X64
  struct myhash_elem *found;
  struct imap_elem_t needle;
  needle.pc = nip;
  if (myhash_find(&in->insn_boundaries, &needle.h_elem)) {
    return true;
  }
  return false;
#elif ARCH_SRC == ARCH_ARM
  return ((nip % 4) == 0) && is_executable(in, nip);
#elif ARCH_SRC == ARCH_ETFG
  return etfg_pc_is_valid_pc(in, pc);
  //return set_belongs(in->get_executable_pcs(), pc);
#else
#error "src arch not recognized"
#endif
}

static struct pc_elem_t const *
find_next_bbl(struct rbtree const *tree, src_ulong pc)
{
  struct rbtree_elem *found;
  struct pc_elem_t ub;

  ub.pc = pc;
  found = rbtree_upper_bound(tree, &ub.rb_elem);

  if (!found) {
    return NULL;
  }
  return rbtree_entry(found, struct pc_elem_t, rb_elem);
}

static bool
is_dynrel_addr(input_exec_t *in, src_ulong nip)
{
  unsigned i;

  if (nip == UNRESOLVED_SYMBOL) {
    return true;
  }
  for (i = 0; i < in->num_relocs; i++) {
    if (in->relocs[i].type == R_PPC_JMP_SLOT) {
      if (in->relocs[i].symbolValue == nip) {
        return true;
      }
    }
  }
  return false;
}


static void
compute_rdefs(input_exec_t *in)
{
  rdefs_t global_sink_rdefs;
  rdefs_init(&global_sink_rdefs);
  rdefs_register(&global_sink_rdefs, regset_all(), -1);

  bool some_change = true;
  int num_iter=0;
  while (some_change) {
    long i;

    DBG(STAT_TRANS, "iter %d\n", num_iter);
    some_change = false;
    for (i = 0; i < in->num_bbls; i++) {
      bbl_t *bbl = &in->bbls[i];
      DBG(STAT_TRANS, "looking at bbl %lx\n", (long)bbl->pc);
      long n_insns = bbl_num_insns(bbl);
      if (!bbl->rdefs) {
        bbl->rdefs = (rdefs_t *)malloc((n_insns + 1) * sizeof(rdefs_t));
        ASSERT(bbl->rdefs);
        int i;
        for (i = 0; i < n_insns + 1; i++) {
          rdefs_init(&bbl->rdefs[i]);
        }
      }

      rdefs_t cur_rdefs, orig_rdefs;
      long i;

      rdefs_init(&cur_rdefs);
      rdefs_init(&orig_rdefs);

      for (i = 0; i < bbl->num_preds; i++) {
        if (bbl->preds[i] == PC_JUMPTABLE) {
          rdefs_union(&cur_rdefs, &global_sink_rdefs);
        } else {
          bbl_t *bblpred;

          bblpred = &in->bbls[pc_find_bbl(in, bbl->preds[i])];
          if (bblpred->rdefs) {
            rdefs_union(&cur_rdefs, &bblpred->rdefs[bbl_num_insns(bblpred)]);
          }
        }
      }

      rdefs_copy(&orig_rdefs, &bbl->rdefs[n_insns]);

      rdefs_copy(&bbl->rdefs[0], &cur_rdefs);

      src_ulong pc;
      for (pc = bbl_first_insn(bbl), i = 0;
          pc != PC_UNDEFINED;
          pc = bbl_next_insn(bbl, pc), i++) {
        regset_t use, def;
        rdefs_t gen, kill;
        src_insn_t insn;

        /* printf("tb->pc=%x, tb->size=0x%x, tb_num_insns=%d, pc=%x, i=%d\n", tb->pc,
            tb->size, tb_num_insns(tb), pc, i); */
        if (!src_insn_fetch(&insn, in, pc, NULL)) {
          continue;
        }

        src_insn_get_usedef_regs(&insn, &use, &def);

        rdefs_init(&gen);
        rdefs_init(&kill);

        rdefs_register(&gen, &def, pc);
        rdefs_clear(&cur_rdefs, &def);
        rdefs_union(&cur_rdefs, &gen);

        ASSERT(i < bbl_num_insns(bbl));
        rdefs_copy(&bbl->rdefs[i + 1], &cur_rdefs);

        rdefs_free(&gen);
        rdefs_free(&kill);
      }

      if (!rdefs_equal(&bbl->rdefs[n_insns], &orig_rdefs)) {
        some_change = true;
      }
      rdefs_free(&cur_rdefs);
      rdefs_free(&orig_rdefs);
    }
    num_iter++;
  }
  DBG(STAT_TRANS, "done. iter %d\n", num_iter);
  rdefs_free(&global_sink_rdefs);
}


static void
bbl_init(bbl_t *bbl)
{
  bbl->pc = (src_ulong)-1;
  bbl->size = 0;
  bbl->num_insns = 0;
  bbl->pcs = NULL;
  bbl->preds = NULL;
  bbl->pred_weights = NULL;
  bbl->num_preds = 0;
  bbl->nextpcs = NULL;
  bbl->fcalls = NULL;
  bbl->fcall_returns = NULL;
  bbl->num_fcalls = 0;
  bbl->nextpc_probs = NULL;
  bbl->num_nextpcs = 0;
  bbl->indir_targets_start = 0;
  bbl->indir_targets_stop = 0;
  bbl->live_regs = NULL;
  bbl->live_ranges = NULL;
  bbl->rdefs = NULL;
  bbl->loop = NULL;
  bbl->reachable = false;
  bbl->in_flow = 1;
}

static void
add_fcall_return_addrs_to_jumptable(input_exec_t *in)
{
  unsigned long insn_size;
  src_insn_t insn;
  bbl_t *bbl;
  long i, j;

  DBG2(CFG, "%s", "entered add_fcall_return_addrs_to_jumptable()\n");
  for (i = 0; i < in->num_bbls; i++) {
    bbl = &in->bbls[i];
    DBG2(CFG, "Looking at bbl %lx: num_fcalls = %ld\n", (long)bbl->pc,
        (long)bbl->num_fcalls);
    for (j = 0; j < bbl->num_fcalls; j++) {
      DBG(CFG, "%lx: adding fcall_return %lx to jumptable\n", (long)bbl->pc,
          (long)bbl->fcall_returns[j]);
      jumptable_add(in->jumptable, bbl->fcall_returns[j]);
      if (   src_insn_fetch(&insn, in, bbl->fcall_returns[j], &insn_size)
          && src_insn_is_unconditional_branch_not_fcall(&insn)) {
        vector<nextpc_t> targets = src_insn_branch_targets(&insn, bbl->fcall_returns[j] + insn_size);
        ASSERT(targets.size() == 1);
        src_ulong target = targets.begin()->get_val();
        jumptable_add(in->jumptable, target);
      }
    }
  }
}

bool
src_pc_belongs_to_plt_section(input_exec_t const *in, src_ulong pc)
{
  int section_idx;

  section_idx = get_input_section_by_name(in, ".plt");

  if (section_idx != -1) {
    if (   pc >= in->input_sections[section_idx].addr
        && pc < in->input_sections[section_idx].addr + in->input_sections[section_idx].size) {
      return true;
    }
  }
  DBG(CFG, "returning false for pc %lx\n", (long)pc);
  return false;
}

static void
jumptable_add_plt_bbls(input_exec_t *in)
{
  long i;

  for (i = 0; i < in->num_bbls; i++) {
    src_ulong pc;
    pc = in->bbls[i].pc;
    if (in->bbls[i].reachable) {
      continue;
    }
    if (!src_pc_belongs_to_plt_section(in, pc)) {
      continue;
    }
    jumptable_add(in->jumptable, pc);
    in->bbls[i].reachable = true;
  }
}

static void
jumptable_add_all_bbls(input_exec_t *in)
{
  NOT_IMPLEMENTED();
/*
  long i;

  // add all pc's to jumptable
  for (i = 0; i < in->num_bbls; i++) {
    unsigned long pc;
    for (pc = in->bbls[i].pc; pc < in->bbls[i].pc + in->bbls[i].size; pc+=4) {
      jumptable_add(in->jumptable, pc);
    }
    in->bbls[i].reachable = true;
  }
*/
}

static void
jumptable_add_unreachable_bbls(input_exec_t *in)
{
  long i;

  /* Add to jumptable those pcs that are unreachable through
     any direct jump */
  for (i = 0; i < in->num_bbls; i++) {
    if (!in->bbls[i].reachable) {
      DBG(CFG, "Adding pc %lx to jumptable\n", (long)in->bbls[i].pc);
      jumptable_add(in->jumptable, in->bbls[i].pc);
      in->bbls[i].reachable = true;
    }
  }
}

/* Allocate a new translation block. Flush the translation buffer if
   too many translation blocks or too much generated code. */
static bbl_t *
bbl_alloc(input_exec_t *in)
{
  bbl_t *bbl;

  bbl = &in->bbls[in->num_bbls++];
  bbl_init(bbl);

  return bbl;
}

void
compute_bbl_map(input_exec_t *in)
{
  unsigned long i;
  myhash_init(&in->bbl_map, imap_hash_func, imap_equal_func, NULL);
  for (i = 0; i < in->num_bbls; i++) {
    struct imap_elem_t *elem;
    struct myhash_elem *ins;

    elem = (struct imap_elem_t *)malloc(sizeof *elem);
    ASSERT(elem);
    elem->pc = in->bbls[i].pc;
    elem->val = i;
    DBG(CFG, "Adding %lx->%ld to bbl_map\n", (long)elem->pc, i);
    ins = myhash_insert(&in->bbl_map, &elem->h_elem);
    ASSERT(!ins);
  }
}

static void
bbl_pred_add(bbl_t *bbl, src_ulong predpc)
{
  bbl->num_preds++;
  bbl->preds = (src_ulong *)realloc(bbl->preds, bbl->num_preds * sizeof(src_ulong));
  ASSERT(bbl->preds);
  bbl->preds[bbl->num_preds - 1] = predpc;

  bbl->pred_weights = (double *)realloc(bbl->pred_weights,
      bbl->num_preds * sizeof(double));
  ASSERT(bbl->pred_weights);
  bbl->pred_weights[bbl->num_preds - 1] = 1;
}

static void
bbl_compute_predecessors(input_exec_t *in)
{
  unsigned long i;

  /* Make a pass to make a list of predecessors. */
  for (i = 0; i < in->num_bbls; i++) {
    bbl_t const *bbl;
    unsigned long j;
    src_ulong pc;

    bbl = &in->bbls[i];
    pc = bbl->pc;
    for (j = 0; j < bbl->num_nextpcs; j++) {
      unsigned long next;
      src_ulong nextpc;
      bbl_t *nextbbl;

      if (bbl->nextpcs[j].get_tag() != tag_const) {
        continue;
      }
      nextpc = bbl->nextpcs[j].get_val();
      if (   nextpc != PC_UNDEFINED
          && nextpc != PC_JUMPTABLE
          && (next = pc2bbl(&in->bbl_map, nextpc)) != (unsigned long)-1) {
        ASSERT(next >= 0 && next < in->num_bbls);
        nextbbl = &in->bbls[next];
        DBG(CFG, "Adding pred %lx to %lx\n", (long)pc, (long)nextbbl->pc);
        bbl_pred_add(nextbbl, bbl->pcs[bbl->num_insns - 1]);
      }
    }
  }

  for (i = 0; i < in->num_bbls; i++) {
    bbl_t *bbl;

    bbl = &in->bbls[i];
    if (jumptable_belongs(in->jumptable, bbl->pc)) {
      DBG(CFG, "Adding pred PC_JUMPTABLE to %lx\n", (long)bbl->pc);
      bbl_pred_add(bbl, PC_JUMPTABLE);
    }
  }
}

static void
bbls_link_indir_jumps(input_exec_t *in)
{
  int i;
  for (i = 0; i < in->num_bbls; i++) {
    unsigned long linked_targets[MAX_LINK_TARGETS];
    double target_probs[MAX_LINK_TARGETS];
    src_ulong targets[MAX_LINK_TARGETS];
    int num_linked_targets = 0;
    long num_targets = 0;
    bbl_t *bbl;
    int j;

    bbl = &in->bbls[i];
    for (j = 0; j < bbl->num_nextpcs; j++) {
      if (   bbl->nextpcs[j].get_tag() == tag_const
          && bbl->nextpcs[j].get_val() != PC_JUMPTABLE
          && bbl->nextpcs[j].get_val() != PC_UNDEFINED) {
        linked_targets[num_linked_targets++] = pc2bbl(&in->bbl_map, bbl->nextpcs[j].get_val());
      }
    }

    for (j = 0; j < bbl->num_nextpcs; j++) {
      if (   bbl->nextpcs[j].get_tag() == tag_const
          && bbl->nextpcs[j].get_val() == PC_JUMPTABLE) {
        num_targets = find_likely_indir_targets(in, bbl, targets, target_probs,
            linked_targets, num_linked_targets, MAX_LINK_TARGETS);
        num_targets = 0;

        DBG_EXEC(LINK_INDIR,
            dbg("bbl %lx links:", (unsigned long)bbl->pc);
            int i;
            for (i = 0; i < num_targets; i++)
            {
            printf("%c%lx", i?',':' ', (unsigned long)targets[i]);
            }
            printf("\n");
            );

        bbl->nextpcs = nextpc_t::nextpcs_realloc(bbl->nextpcs, bbl->num_nextpcs,
            (bbl->num_nextpcs + num_targets));

        ASSERT(bbl->nextpcs);

        bbl->indir_targets_start = j;
        bbl->indir_targets_stop = j + num_targets;

        int i;
        //initialize the new entries in the arrays
        for (i = bbl->num_nextpcs - 1; i >= j; i--) {
          bbl->nextpcs[i + num_targets] = bbl->nextpcs[i];
        }

        for (i = 0; i < num_targets; i++) {
          bbl->nextpcs[j + i] = targets[i];
        }

        bbl->num_nextpcs += num_targets;

        DBG_EXEC(LINK_INDIR,
            printf("%lx: num_indir_link_targets=%ld, num_nextpcs=%ld, nextpcs:",
              (unsigned long)bbl->pc, num_targets, bbl->num_nextpcs);
            for (i = 0; i < bbl->num_nextpcs; i++)
            printf (" %s", bbl->nextpcs[i].nextpc_to_string().c_str());
            printf("\n");
            );
        break;
      }
    }
  }
}

typedef struct {
  src_ulong pc;
  double weight;
} preds_sort_struct_t;

static int
preds_sort_invcompare(const void *_a, const void *_b)
{
  preds_sort_struct_t *a = (preds_sort_struct_t *)_a;
  preds_sort_struct_t *b = (preds_sort_struct_t *)_b;

  if (a->weight > b->weight) {
    return -1;
  } else if (a->weight < b->weight) {
    return 1;
  } else {
    return 0;
  }
}

static bbl_t *
tb_most_frequent_pred (input_exec_t *in, int n)
{
  NOT_IMPLEMENTED();
#if 0
  pred_succ_node_t *cur;
  tb_t *tb = &in->bbls[n];
  tb_t *mfp = NULL;
  for (cur = tb->preds; cur; cur = cur->next) {
    int i = cur->tb_index;

    if (i == in->global_sink_tb) {
      continue;
    }
    tb_t *pred = &in->bbls[i];
    if (   !mfp
        || (pred->pc >= tb->pc && mfp->pc > pred->pc)
        || (pred->pc < tb->pc && mfp->pc < pred->pc)) {
      mfp = pred;
    }
  }
  return mfp;
#endif
}

#if 0
static void
compute_flow_statically (void)
{
  int i;
  for (i = 0; i < ti.num_bbls; i++)
  {
    tb_t *tb = &ti.bbls[i];
    tb_t *mfp = tb_most_frequent_pred(i);
    
    pred_succ_node_t *cur;
    int num_tb_preds = 0;
    for (cur = tb->preds; cur; cur = cur->next)
    {
      if (mfp == &ti.bbls[cur->tb_index])
      {
	cur->weight = 1.0;
      }
      else
      {
	cur->weight = 0.0;
      }
      num_tb_preds++;
    }

    if (num_tb_preds == 0)
      continue;

    /* sort the preds based on their weights. */
    preds_sort_struct_t preds_sort[num_tb_preds];
    int j = 0;
    for (cur = tb->preds; cur; cur=cur->next)
    {
      preds_sort[j].ptr = cur;
      preds_sort[j].weight = cur->weight;
      j++;
    }
    qsort (preds_sort, num_tb_preds, sizeof(preds_sort_struct_t),
	preds_sort_invcompare);

    tb->preds = preds_sort[0].ptr;
    cur = tb->preds;
    cur->next = NULL;
    for (j = 1; j < num_tb_preds; j++)
    {
      cur->next = preds_sort[j].ptr;
      cur = cur->next;
      cur->next = NULL;
    }
  }
}
#endif

static bool
check_bbl_preds(input_exec_t const *in, bbl_t const *bbl)
{
  long j;
  if (!jumptable_belongs(in->jumptable, bbl->pc)) {
    return true;
  }
  for (j = 0; j < bbl->num_preds; j++) {
    if (bbl->preds[j] == PC_JUMPTABLE) {
      return true;
    }
  }
  return false;
}

static void
compute_flow_from_edge_frequencies(input_exec_t *in,
    edge_table_t const *src_table, edge_table_t const *dst_table)
{
  int i;
  for (i = 0; i < in->num_bbls; i++) {
    long long num_occur, in_flow;
    src_ulong last_nip, nip2;
    void *iter;
    bbl_t *bbl;

    bbl = &in->bbls[i];
    long long flow[bbl->num_nextpcs];

    last_nip = bbl_last_insn(bbl);
    in_flow = 0;

    int j;
    for (j = 0; j < bbl->num_nextpcs; j++) {
      flow[j] = 0;
    }
    for (iter = edge_table_find_next(src_table, NULL, last_nip, &nip2, &num_occur);
        iter;
        iter = edge_table_find_next(src_table,iter,last_nip,&nip2,&num_occur)) {
      in_flow += num_occur;

      for (j = 0; j < bbl->num_nextpcs; j++) {
        int duplicate_successor = 0;
        int k;
        /* this loop handles the case where two successors of a bbl point
           to the same address. In this case, assign probability to only
           one branch */
        for (k = 0; k < j; k++) {
          if (bbl->nextpcs[k].get_val() == bbl->nextpcs[j].get_val()) {
            duplicate_successor = 1;
            flow[j] = 0;
            break;
          }
        }

        if (duplicate_successor) {
          continue;
        }
        if (   bbl->nextpcs[j].get_val() != PC_JUMPTABLE
            && bbl->nextpcs[j].get_val() == nip2) {
          flow[j] = num_occur;
        }
      }
    }

    if (in_flow == 0) {
      int j;
      for (j = 0; j < bbl->num_nextpcs; j++) {
        /*if (pc_dominates(in, bbl->nextpcs[j], bbl->pc)) {
          flow[j] = 10;
        } else */{
          flow[j] = 1;
        }
        in_flow += flow[j];
      }
    }

    double total_prob = 0;
    for (j = 0; j < bbl->num_nextpcs; j++) {
      if (bbl->nextpcs[j].get_val() != PC_JUMPTABLE) {
        ASSERT(in_flow);
        bbl->nextpc_probs[j] = (double)flow[j]/in_flow;
        total_prob += bbl->nextpc_probs[j];
      }
    }
    if (total_prob > 1.001) {
      err("%lx: total_prob=%f, in_flow=%lld\nsucc:", (long)bbl->pc, total_prob, in_flow);
      int i;
      for (i = 0; i < bbl->num_nextpcs; i++) {
        if (bbl->nextpcs[i].get_val() == PC_JUMPTABLE) {
          printf("%cindir", i?' ':',');
        }
        printf("%c%lx(%f)",i?' ':',', (unsigned long)bbl->nextpcs[i].get_val(),
            bbl->nextpc_probs[i]);
      }
      printf("\n");
      ASSERT(0);
    }
    for (j = 0; j < bbl->num_nextpcs; j++) {
      if (bbl->nextpcs[j].get_val() == PC_JUMPTABLE) {
        bbl->nextpc_probs[j] = 1 - total_prob;
      }
    }
  }

  for (i = 0; i < in->num_bbls; i++) {
    unsigned long long in_flow = 0;
    long long sum_pred_occurences = 0;
    long long num_occur;
    src_ulong nip2;
    void *iter;
    bbl_t *bbl;
    int j;

    bbl = &in->bbls[i];
    //ASSERT(bbl->num_preds);

    long long pred_occurences[bbl->num_preds];

    for (j = 0; j < bbl->num_preds; j++) {
      pred_occurences[j] = 0;
    }

    for(iter = edge_table_find_next(dst_table, NULL, bbl->pc, &nip2, &num_occur);
        iter;
        iter = edge_table_find_next(dst_table, iter, bbl->pc, &nip2, &num_occur)) {
      in_flow += num_occur;
      for (j = 0; j < bbl->num_preds; j++) {
        unsigned long predbbl_num;
        bbl_t *predbbl;

        if (bbl->preds[j] == PC_JUMPTABLE) {
          continue;
        }
        predbbl_num = pc2bbl(&in->bbl_map, bbl->preds[j]);
        ASSERT(predbbl_num >= 0 && predbbl_num < in->num_bbls);
        predbbl = &in->bbls[predbbl_num];
        if (bbl_last_insn(predbbl) == nip2) {
          pred_occurences[j] = num_occur;
          sum_pred_occurences += num_occur;
        }
      }
    }

    if (in_flow == 0) {
      ASSERT(sum_pred_occurences == 0);
      for (j = 0; j < bbl->num_preds; j++) {
        if (bbl->preds[j] == PC_JUMPTABLE) {
          pred_occurences[j] = 1;
        } else if (pc_dominates(in, bbl->pc, bbl->preds[j])) {
          pred_occurences[j] = 10;
        } else {
          pred_occurences[j] = 1;
        }
        sum_pred_occurences += pred_occurences[j];
        in_flow += pred_occurences[j];
      }

      //compute in_flow statically.. need control flow analysis
      //tb->in_flow = ??;
      bbl->in_flow = in_flow; //not precise at all
    } else {
      bbl->in_flow = in_flow;
    }
    
    for (j = 0; j < bbl->num_preds; j++) {
      ASSERT(in_flow);
      if (bbl->preds[j] != PC_JUMPTABLE) {
        bbl->pred_weights[j] = (double)pred_occurences[j]/in_flow;
      } else {
        bbl->pred_weights[j] = 1 - (double)sum_pred_occurences/in_flow;
      }
    }

    /* check consistency */
    ASSERT(check_bbl_preds(in, bbl));

    if (bbl->num_preds > 1) {
      preds_sort_struct_t preds_sort[bbl->num_preds];
      int j;

      /* sort the preds based on their weights. */
      for (j = 0; j < bbl->num_preds; j++) {
        preds_sort[j].pc = bbl->preds[j];
        preds_sort[j].weight = bbl->pred_weights[j];
      }
      qsort(preds_sort, bbl->num_preds, sizeof(preds_sort_struct_t),
          preds_sort_invcompare);

      for (j = 0; j < bbl->num_preds; j++) {
        bbl->preds[j] = preds_sort[j].pc;
        bbl->pred_weights[j] = preds_sort[j].weight;
      }
    }

    /* check consistency */
    ASSERT(check_bbl_preds(in, bbl));
  }
}

static void
update_flows_at_function_entries(input_exec_t *in)
{
  for (long i = 0; i < in->num_bbls; i++) {
    /*string function_name;
    if (in->pc_is_etfg_function_entry(in->bbls[i].pc, function_name)) */{
      in->bbls[i].in_flow = 0; //init to 0
    }
  }
  for (long i = 0; i < in->num_bbls; i++) {
    bbl_t const *bbl = &in->bbls[i];
    for (long f = 0; f < bbl->num_fcalls; f++) {
      if (bbl->fcalls[f].get_tag() != tag_const) {
        continue;
      }
      src_ulong nextpc = bbl->fcalls[f].get_val();
      if (nextpc != PC_UNDEFINED && nextpc != PC_JUMPTABLE) {
        long idx = pc_find_bbl(in, nextpc);
        ASSERT(idx >= 0 && idx < in->num_bbls);
        in->bbls[idx].in_flow += bbl->in_flow;
      }
    }
  }
  for (long i = 0; i < in->num_bbls; i++) {
    string function_name;
    if (   in->bbls[i].in_flow == 0
        && in->pc_is_etfg_function_entry(in->bbls[i].pc, function_name)) {
      if (function_name == ETFG_MAIN_FUNCTION_IDENTIFIER) {
        in->bbls[i].in_flow = ETFG_MAIN_IN_FLOW;
      } else {
        in->bbls[i].in_flow = ETFG_FUNCTION_IN_FLOW;
      }
    }
  }
}

static bool
belong_to_same_innermost_natural_loop(input_exec_t const *in, src_ulong pc1, src_ulong pc2)
{
  long idx1 = pc_find_bbl(in, pc1);
  ASSERT(idx1 >= 0 && idx1 < in->num_bbls);
  long idx2 = pc_find_bbl(in, pc2);
  ASSERT(idx2 >= 0 && idx2 < in->num_bbls);

  if (   in->innermost_natural_loop[idx1] == in->innermost_natural_loop[idx2]
      && in->innermost_natural_loop[idx1] != -1) {
    return true;
  } else if (   in->innermost_natural_loop[idx1] == idx2
             || in->innermost_natural_loop[idx2] == idx1) {
    return true;
  } else {
    return false;
  }
}

static void
split_flow_at_nextpcs(input_exec_t const *in, src_ulong pc, unsigned long long in_flow, nextpc_t const *nextpcs, long num_nextpcs, unsigned long long *out_flow)
{
  set<long> loop_nextpcs;
  set<long> non_loop_nextpcs;
  if (num_nextpcs == 0) {
    return;
  }
  if (num_nextpcs == 1) {
    out_flow[0] = in_flow;
    return;
  }
  for (long i = 0; i < num_nextpcs; i++) {
    if (nextpcs[i].get_val() == PC_JUMPTABLE || nextpcs[i].get_val() == PC_UNDEFINED) {
      continue;
    }
    if (belong_to_same_innermost_natural_loop(in, pc, nextpcs[i].get_val())) {
      loop_nextpcs.insert(i);
    } else {
      non_loop_nextpcs.insert(i);
    }
  }
  if (loop_nextpcs.size() == 0 || non_loop_nextpcs.size() == 0) {
    size_t total_nextpcs = loop_nextpcs.size() + non_loop_nextpcs.size();
    for (long i = 0; i < num_nextpcs; i++) {
      if (nextpcs[i].get_val() == PC_JUMPTABLE || nextpcs[i].get_val() == PC_UNDEFINED) {
        out_flow[i] = 0;
        continue;
      }
      out_flow[i] = in_flow / total_nextpcs;
    }
    return;
  }
  unsigned long long non_loop_nextpc_flow = in_flow / (32 * non_loop_nextpcs.size());
  unsigned long long loop_nextpc_flow = (31 * in_flow) / (32 * loop_nextpcs.size());
  for (long i = 0; i < num_nextpcs; i++) {
    if (nextpcs[i].get_val() == PC_JUMPTABLE || nextpcs[i].get_val() == PC_UNDEFINED) {
      out_flow[i] = 0;
      continue;
    }
    if (loop_nextpcs.count(i)) {
      out_flow[i] = loop_nextpc_flow;
    } else {
      out_flow[i] = non_loop_nextpc_flow;
    }
  }
}

static void
compute_nextpc_probs(input_exec_t *in)
{
  for (long i = 0; i < in->num_bbls; i++) {
    bbl_t *bbl = &in->bbls[i];
    unsigned long long flow[bbl->num_nextpcs];
    unsigned long long in_flow = 1ULL << 32;
    split_flow_at_nextpcs(in, bbl->pc, in_flow, bbl->nextpcs, bbl->num_nextpcs, flow);
    for (long n = 0; n < bbl->num_nextpcs; n++) {
      bbl->nextpc_probs[n] = (double)flow[n]/(double)in_flow;
    }
  }
}

static long
bbl_get_nextpc_idx(bbl_t const *bbl, src_ulong nextpc)
{
  for (long i = 0; i < bbl->num_nextpcs; i++) {
    if (   bbl->nextpcs[i].get_tag() == tag_const
        && bbl->nextpcs[i].get_val() == nextpc) {
      return i;
    }
  }
  NOT_REACHED();
}


static void
compute_flow_on_cfg(input_exec_t *in)
{
  for (size_t num_iter = 0; num_iter < COMPUTE_FLOW_ON_CFG_ITER; num_iter++) {
    for (long i = 0; i < in->num_bbls; i++) {
      bbl_t *bbl = &in->bbls[i];
      string function_name;
      if (in->pc_is_etfg_function_entry(bbl->pc, function_name)) {
        continue;
      }
      //bbl->in_flow = 0;
      unsigned long long new_in_flow = 0;
      for (long p = 0; p < bbl->num_preds; p++) {
        src_ulong pred = bbl->preds[p];
        if (pred == PC_JUMPTABLE || pred == PC_UNDEFINED) {
          continue;
        }
        long idx = pc_find_bbl(in, pred);
        ASSERT(idx >= 0 && idx < in->num_bbls);
        bbl_t const *bbl_pred = &in->bbls[idx];
        long nextpc_idx = bbl_get_nextpc_idx(bbl_pred, bbl->pc);
        ASSERT(nextpc_idx >= 0 && nextpc_idx < bbl_pred->num_nextpcs);
        new_in_flow += bbl_pred->in_flow * bbl_pred->nextpc_probs[nextpc_idx];
      }
      //cout << __func__ << " " << __LINE__ << ": iter = " << num_iter << "; setting in_flow for " << hex << bbl->pc << dec << " to " << new_in_flow << endl;
      bbl->in_flow = max(1ULL, new_in_flow);
      ASSERT(bbl->in_flow >= 0 && bbl->in_flow < COST_INFINITY);
    }
  }
}

static void
estimate_flow_statically(input_exec_t *in)
{
  //graph<bbl_ptr_t, node<bbl_ptr_t>, edge<bbl_ptr_t>> g_cfg = in->get_cfg_as_graph();
  //graph<bbl_ptr_t, node<bbl_ptr_t>, edge<bbl_ptr_t>> g_fcall = in->get_fcalls_as_graph();
  compute_nextpc_probs(in);

  for (size_t num_iter = 0; num_iter < COMPUTE_FLOW_NUM_ITER_FOR_FUNCTION_ENTRIES; num_iter++) {
    update_flows_at_function_entries(in);
    compute_flow_on_cfg(in);  //intra-procedural
  }
  for (long i = 0; i < in->num_bbls; i++) {
    bbl_t *bbl = &in->bbls[i];
    for (long j = 0; j < bbl->num_preds; j++) {
      src_ulong pred = bbl->preds[j];
      if (pred != PC_JUMPTABLE) {
        long idx = pc_find_bbl(in, pred);
        ASSERT(idx >= 0 && idx < in->num_bbls);
        bbl_t const *bbl_pred = &in->bbls[idx];
        long nextpc_idx = bbl_get_nextpc_idx(bbl_pred, bbl->pc);
        ASSERT(nextpc_idx >= 0 && nextpc_idx < in->num_bbls);
        double pred_flow = bbl_pred->in_flow * bbl_pred->nextpc_probs[nextpc_idx];
        bbl->pred_weights[j] = pred_flow/bbl->in_flow;
      } else {
        bbl->pred_weights[j] = PRED_WEIGHT_JUMPTABLE_PC;
      }
    }
  }
}

static void
compute_flow(input_exec_t *in)
{
  for (long i = 0; i < in->num_bbls; i++) {
    bbl_t *bbl = &in->bbls[i];
    bbl->nextpc_probs = (bbl->num_nextpcs == 0)?NULL:
      (double *)malloc(bbl->num_nextpcs * sizeof(double));
    ASSERT(!bbl->num_nextpcs || bbl->nextpc_probs);
  }

  if (profile_name) {
    edge_table_t *src_table, *dst_table;
    edge_table_init(&dst_table, 65536);
    edge_table_init(&src_table, 65536);

    read_edge_tables(src_table, dst_table);
    compute_flow_from_edge_frequencies(in, src_table, dst_table);

    edge_table_free(src_table);
    edge_table_free(dst_table);
  } else {
    estimate_flow_statically(in);
  }
}

int
is_executed_more_than(src_ulong nip, long long n)
{
  static edge_table_t *src_table = NULL;

  if (!profile_name) {
    return 1;
  }
  if (!src_table) {
    edge_table_init (&src_table, 65536);
    read_edge_tables (src_table, NULL);
  }

  unsigned long long in_flow = 0;
  long long num_occur;
  src_ulong nip2;
  void *iter;

  for (iter=edge_table_find_next(src_table, NULL, nip, &nip2, &num_occur);
      iter;
      iter = edge_table_find_next(src_table, iter, nip, &nip2, &num_occur)) {
    in_flow += num_occur;
  }

  if (in_flow <= n) {
    return 0;
  } else {
    return 1;
  }
}

static void
loop_free(loop_t *loop)
{
  if (!loop) {
    return;
  }
  free(loop->pcs);
  loop->pcs = NULL;

  //free(loop->out_edges);
  delete[] loop->out_edges;
  loop->out_edges = NULL;

  free(loop->out_edge_weights);
  loop->out_edge_weights = NULL;
  free(loop);
}

void
bbl_free(bbl_t *bbl)
{
  if (bbl->live_regs) {
    //free(bbl->live_regs);
    delete [] bbl->live_regs;
    bbl->live_regs = NULL;
  }
  if (bbl->live_ranges) {
    //free(bbl->live_ranges);
    delete [] bbl->live_ranges;
    bbl->live_ranges = NULL;
  }
  if (bbl->rdefs) {
    unsigned long i;
    for (i = 0; i < bbl->num_insns + 1; i++) {
      rdefs_free(&bbl->rdefs[i]);
    }
    free(bbl->rdefs);
    bbl->rdefs = NULL;
  }
  if (bbl->preds) {
    free(bbl->preds);
    bbl->preds = NULL;
    bbl->num_preds = 0;
  }
  if (bbl->pred_weights) {
    free(bbl->pred_weights);
    bbl->pred_weights = NULL;
  }
  loop_free(bbl->loop);
  if (bbl->pcs) {
    free(bbl->pcs);
    bbl->pcs = NULL;
  }
  if (bbl->nextpcs) {
    //free(bbl->nextpcs);
    delete[] bbl->nextpcs;
    bbl->nextpcs = NULL;
  }
  if (bbl->nextpc_probs) {
    free(bbl->nextpc_probs);
    bbl->nextpc_probs = NULL;
  }
  if (bbl->fcalls) {
    //free(bbl->fcalls);
    delete[] bbl->fcalls;
    bbl->fcalls = NULL;
  }
  if (bbl->fcall_returns) {
    free(bbl->fcall_returns);
    bbl->fcall_returns = NULL;
  }
}

void
free_bbl_map(input_exec_t *in)
{
  myhash_destroy(&in->bbl_map, imap_elem_free);
}

static void
eliminate_unreachable_bbls(input_exec_t *in)
{
  bbl_t *orig_bbls;
  long i, j;

  orig_bbls = (bbl_t *)malloc(in->num_bbls * sizeof(bbl_t));
  ASSERT(orig_bbls);

  memcpy(orig_bbls, in->bbls, in->num_bbls * sizeof(bbl_t));
  
  for (i = 0, j = 0; i < in->num_bbls; i++) {
    if (orig_bbls[i].reachable) {
      memcpy(&in->bbls[j], &orig_bbls[i], sizeof(bbl_t));
      j++;
    } else {
      printf("eliminating bbl at %lx\n", (long)orig_bbls[i].pc);
      bbl_free(&orig_bbls[i]);
    }
  }
  free(orig_bbls);
  in->num_bbls = j;
}


/* looks at reachable bbls to decide new reachable bbls. Returns true if no change. */
static bool
compute_reachable_bbls_helper_iter(input_exec_t const *in, bool *reachable)
{
  bool new_reachable[in->num_bbls];
  long i, index;
  bool ret;

  memset(new_reachable, 0x0, in->num_bbls);

  for (i = 0; i < in->num_bbls; i++) {
    if (jumptable_belongs(in->jumptable, in->bbls[i].pc)) {
      new_reachable[i] = true;
    }
  }
  /*index = pc2bbl(&in->bbl_map, in->entry);
  ASSERT(index >= 0 && index < in->num_bbls);
  new_reachable[index] = true;*/

  for (i = 0; i < in->num_bbls; i++) {
    if (!reachable[i]) {
      continue;
    }
    bbl_t *bbl = &in->bbls[i];
    long j;
    //printf("looking at %ld nextpcs of %lx\n", bbl->num_nextpcs, (long)bbl->pc);
    for (j = 0; j < bbl->num_nextpcs; j++) {
      if (   bbl->nextpcs[j].get_tag() != tag_const
          || bbl->nextpcs[j].get_val() == PC_UNDEFINED
          || bbl->nextpcs[j].get_val() == PC_JUMPTABLE) {
        continue;
      }
      //printf("marking %lx reachable\n", (long)bbl->nextpcs[j]);
      index = pc2bbl(&in->bbl_map, bbl->nextpcs[j].get_val());
      if (index == -1) {
        continue;
      }
      ASSERT(index >= 0 && index < in->num_bbls);
      new_reachable[index] = true;
    }
    for (j = 0; j < bbl->num_fcalls; j++) {
      if (   bbl->fcalls[j].get_tag() != tag_const
          || bbl->fcalls[j].get_val() == PC_UNDEFINED
          || bbl->fcalls[j].get_val() == PC_JUMPTABLE) {
        continue;
      }
      index = pc2bbl(&in->bbl_map, bbl->fcalls[j].get_val());
      if (index == -1) {
        continue;
      }
      ASSERT(index >= 0 && index < in->num_bbls);
      new_reachable[index] = true;
    }
  }
  ret = true;
  for (i = 0; i < in->num_bbls; i++) {
    ASSERT(!new_reachable[i] || reachable[i]);
    if (!new_reachable[i] && reachable[i]) {
      ret = false;
      reachable[i] = new_reachable[i];
    }
  }
  return ret;
}



static void
compute_reachable_bbls(input_exec_t *in)
{
  bool reachable[in->num_bbls];
  bbl_t *orig_bbls;
  long i, j;

  for (i = 0; i < in->num_bbls; i++) {
    reachable[i] = true;
  }
  while (!compute_reachable_bbls_helper_iter(in, reachable)); //compute fixed point

  for (i = 0; i < in->num_bbls; i++) {
    in->bbls[i].reachable = reachable[i];
  }
}



void
build_cfg(input_exec_t *in)
{
  struct rbtree_elem const *iter;
  unsigned long num_bbls;

#if ARCH_SRC == ARCH_I386 || ARCH_SRC == ARCH_X64
  compute_insn_boundaries(in, &in->insn_boundaries);
#endif
  determine_bbl_leaders_and_jumptable_targets(in);
  num_bbls = rbtree_count(&in->bbl_leaders);

  //printf("%s() %d: num_bbls = %d\n", __func__, __LINE__, (int)num_bbls);

  in->bbls = (bbl_t *)malloc((num_bbls + 1)*sizeof(bbl_t));
  ASSERT(in->bbls);

  iter = rbtree_begin(&in->bbl_leaders);

  while (iter) {
    src_ulong pc, next_bbl, pcs[BBL_MAX_NUM_INSNS];
    unsigned long bbl_size, bbl_num_insns;
    src_ulong /**fcalls, */*fcall_returns;
    struct pc_elem_t const *elem;
    vector<nextpc_t> nextpcs;
    //long num_nextpcs;
    long num_fcalls;
    bbl_t *bbl;

    elem = rbtree_entry(iter, struct pc_elem_t, rb_elem);
    pc = elem->pc;
    if (!is_executable(in, pc)) {
      iter = rbtree_next(iter);
      continue;
    }

    elem = find_next_bbl(&in->bbl_leaders, pc);
    if (elem) {
      iter = &elem->rb_elem;
      next_bbl = elem->pc;
    } else {
      iter = NULL;
      next_bbl = 0;
    }
    //fcalls = (src_ulong *)malloc(BBL_MAX_NUM_INSNS * sizeof(src_ulong));
    //ASSERT(fcalls);
    vector<nextpc_t> fcalls;
    fcall_returns = (src_ulong *)malloc(BBL_MAX_NUM_INSNS * sizeof(src_ulong));
    ASSERT(fcall_returns);
    /*num_nextpcs = */bbl_scan(in, pc, next_bbl, &bbl_size, &bbl_num_insns, pcs, nextpcs,
        &fcalls, fcall_returns, &num_fcalls);

    bbl = bbl_alloc(in);
    bbl->pc = pc;
    bbl->size = bbl_size;
    bbl->num_insns = bbl_num_insns;
    bbl->pcs = (src_ulong *)malloc(bbl_num_insns * sizeof(src_ulong));
    ASSERT(bbl->pcs);
    memcpy(bbl->pcs, pcs, bbl_num_insns * sizeof(src_ulong));
    //bbl->nextpcs = (src_ulong *)malloc(num_nextpcs * sizeof(src_ulong));
    bbl->nextpcs = new nextpc_t[nextpcs.size()];
    ASSERT(bbl->nextpcs);
    for (size_t i = 0; i < nextpcs.size(); i++) {
      bbl->nextpcs[i] = nextpcs.at(i);
      CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": bbl " << hex << bbl->pc << dec << ": nextpc " << i << ": " << bbl->nextpcs[i].nextpc_to_string() << endl);
    }
    //memcpy(bbl->nextpcs, nextpcs, num_nextpcs * sizeof(src_ulong));
    DBG(CFG, "%s() %d: %lx: num_nextpcs = %ld\n", __func__, __LINE__, (long)bbl->pc, nextpcs.size());
    bbl->num_nextpcs = nextpcs.size();
    //bbl->fcalls = (src_ulong *)realloc(fcalls, num_fcalls * sizeof(src_ulong));
    bbl->fcalls = new nextpc_t[num_fcalls];
    for (size_t i = 0; i < fcalls.size(); i++) {
      bbl->fcalls[i] = fcalls.at(i);
      CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": bbl " << hex << bbl->pc << dec << ": fcall " << i << ": " << bbl->fcalls[i].nextpc_to_string() << endl);
    }
    ASSERT(!num_fcalls || bbl->fcalls);
    bbl->fcall_returns = (src_ulong *)realloc(fcall_returns,
        num_fcalls * sizeof(src_ulong));
    ASSERT(!num_fcalls || bbl->fcall_returns);
    bbl->num_fcalls = num_fcalls;
  }

  /* Obtain pc->bbl_index mapping */
  compute_bbl_map(in);

#if CALLRET != CALLRET_ID
  add_fcall_return_addrs_to_jumptable(in);
#endif

  //if (!in->fresh) {
    jumptable_add(in->jumptable, in->entry);
  //}

  compute_reachable_bbls(in);
  jumptable_add_plt_bbls(in);
#if JUMPTABLE_ADD == JUMPTABLE_ADD_ALL
  jumptable_add_all_bbls(in);
#elif JUMPTABLE_ADD == JUMPTABLE_ADD_UNREACHABLE
  jumptable_add_unreachable_bbls(in);
#else
  if (cfg_eliminate_unreachable_bbls) {
    eliminate_unreachable_bbls(in);
    free_bbl_map(in);
    compute_bbl_map(in);
  }
#endif
}

static void
dfs_for_pcs(input_exec_t const *in, src_ulong pc, src_ulong *outpcs, long *outpcs_len,
    edge_t *out_edges, double *out_edge_weights, long *out_edges_len,
    src_ulong const *inpcs, long inpcs_len, double cur_weight)
{
  double external_edge_weight_sum;
  long i;//, num_nextpcs;
  //src_ulong *nextpcs;
  //vector<double> nextpc_weights_vec;
  //vector<nextpc_t> nextpcs_vec;

  ASSERT(*outpcs_len < inpcs_len);
  outpcs[*outpcs_len] = pc;
  (*outpcs_len)++;

  //nextpc_weights = (double *)malloc(BBL_MAX_NUM_INSNS * sizeof(double));
  //ASSERT(nextpc_weights);
  //nextpcs = (src_ulong *)malloc(BBL_MAX_NUM_INSNS * sizeof(src_ulong));
  //ASSERT(nextpcs);
  nextpc_t *nextpcs = new nextpc_t[BBL_MAX_NUM_INSNS];
  ASSERT(nextpcs);
  double *nextpc_weights = new double[BBL_MAX_NUM_INSNS];
  ASSERT(nextpc_weights);

  //num_nextpcs = pc_get_nextpcs(in, pc, nextpcs, nextpc_weights);
  long num_nextpcs = pc_get_nextpcs(in, pc, nextpcs, nextpc_weights);

  ASSERT(num_nextpcs < BBL_MAX_NUM_INSNS);
  //nextpc_weights = (double *)realloc(nextpc_weights, BBL_MAX_NUM_INSNS * sizeof(double));
  //nextpcs = (src_ulong *)realloc(nextpcs, BBL_MAX_NUM_INSNS * sizeof(src_ulong));

  external_edge_weight_sum = 0;
  for (i = 0; i < num_nextpcs; i++) {
    if (array_search(inpcs, inpcs_len, nextpcs[i].get_val()) == -1) {
      out_edges[*out_edges_len].src = pc;
      out_edges[*out_edges_len].dst = nextpcs[i];
      out_edge_weights[*out_edges_len] = cur_weight * nextpc_weights[i];
      (*out_edges_len)++;
      external_edge_weight_sum += nextpc_weights[i];
    }
  }

  /* dfs in correct order. */
  for (i = num_nextpcs - 1; i >= 0; i--) {
    if (   array_search(inpcs, inpcs_len, nextpcs[i].get_val()) != -1
        && array_search(outpcs, *outpcs_len, nextpcs[i].get_val()) == -1) {
      /* need to visit, but not already visited */
      dfs_for_pcs(in, nextpcs[i].get_val(), outpcs, outpcs_len, out_edges, out_edge_weights,
          out_edges_len, inpcs, inpcs_len, cur_weight * (1 - external_edge_weight_sum));
    }
  }
  //free(nextpc_weights);
  //free(nextpcs);
  delete[] nextpc_weights;
  delete[] nextpcs;
}

static src_ulong *
rdfs_till(input_exec_t const *in, src_ulong pc, src_ulong *pcs, long *pcs_size,
    long *len, src_ulong stop)
{
  src_ulong *preds;
  long i, num_preds;

  ASSERT(*pcs_size >= *len);

  if (array_search(pcs, *len, pc) != -1) {
    return pcs;
  }

  if (*pcs_size == *len) {
    *pcs_size *= 2;
    pcs = (src_ulong *)realloc(pcs, *pcs_size * sizeof(src_ulong));
    ASSERT(pcs);
  }
  pcs[*len] = pc;
  (*len)++;

  if (pc == stop) {
    return pcs;
  }

  preds = (src_ulong *)malloc(BBL_MAX_NUM_INSNS * sizeof(src_ulong));
  ASSERT(preds);

  num_preds = pc_get_preds(in, pc, preds);

  ASSERT(num_preds < BBL_MAX_NUM_INSNS);
  preds = (src_ulong *)realloc(preds, num_preds * sizeof(src_ulong));

  for (i = 0; i < num_preds; i++) {
    ASSERT(preds[i] != PC_JUMPTABLE);
    pcs = rdfs_till(in, preds[i], pcs, pcs_size, len, stop);
  }
  free(preds);
  return pcs;
}

static src_ulong *
add_loop_pcs(input_exec_t const *in, src_ulong *pcs, long *len, src_ulong pc,
    src_ulong pred)
{
#define MIN_PCS_SIZE 16
  long pcs_size;
  /* reverse-dfs starting from pred and ending at pc, adding all pcs on the way. */

  pcs_size = *len;
  if (pcs_size < MIN_PCS_SIZE) {
    pcs_size = MIN_PCS_SIZE;
  }
  if (pcs_size != *len) {
    pcs = (src_ulong *)realloc(pcs, pcs_size * sizeof(src_ulong));
    ASSERT(pcs);
  }
  pcs = rdfs_till(in, pred, pcs, &pcs_size, len, pc);
  pcs = (src_ulong *)realloc(pcs, *len * sizeof(src_ulong));
  ASSERT(pcs);

  return pcs;
}

static void
normalize_weights(double *weights, long n)
{
  double sum;
  long i;

  sum = 0;
  for (i = 0; i < n; i++) {
    ASSERT(weights[i] != 0);
    sum += weights[i];
  }
  for (i = 0; i < n; i++) {
    ASSERT(sum > 0);
    weights[i] /= sum;
  }
}

static loop_t *
compute_loop(input_exec_t const *in, bbl_t const *bbl)
{
  bool loop_exists;
  src_ulong *pcs;
  loop_t *ret;
  long i, len;

  loop_exists = false;
  pcs = NULL;
  len = 0;

  for (i = 0; i < bbl->num_preds; i++) {
    if (bbl->preds[i] == PC_JUMPTABLE) {
      continue;
    }
    if (pc_dominates(in, bbl->pc, bbl->preds[i])) {
      loop_exists = true;
      pcs = add_loop_pcs(in, pcs, &len, bbl->pc, bbl->preds[i]);
    }
  }
  if (!loop_exists) {
    return NULL;
  }

  ret = (loop_t *)malloc(sizeof(loop_t));
  ASSERT(ret);
  ret->pcs = (src_ulong *)malloc(len * sizeof(src_ulong));
  ASSERT(ret->pcs);
  //ret->out_edges = (edge_t *)malloc(len * sizeof(edge_t));
  ret->out_edges = new edge_t[len];
  ASSERT(ret->out_edges);
  ret->out_edge_weights = (double *)malloc(len * sizeof(double));
  ASSERT(ret->out_edge_weights);
  ret->num_pcs = 0;
  ret->num_out_edges = 0;
  dfs_for_pcs(in, bbl->pc, ret->pcs, &ret->num_pcs, ret->out_edges,
      ret->out_edge_weights, &ret->num_out_edges, pcs, len, 1);
  ASSERT(ret->num_pcs <= len);
  ASSERT(ret->num_out_edges <= len);
  free(pcs);
  //ret->out_edges = (edge_t *)realloc(ret->out_edges,
  //    ret->num_out_edges * sizeof(edge_t));
  auto old_out_edges = ret->out_edges;
  ret->out_edges = new edge_t[ret->num_out_edges];
  for (size_t i = 0; i < ret->num_out_edges; i++) {
    ret->out_edges[i] = old_out_edges[i];
  }
  delete[] old_out_edges;
  ret->out_edge_weights = (double *)realloc(ret->out_edge_weights,
      ret->num_out_edges * sizeof(double));
  normalize_weights(ret->out_edge_weights, ret->num_out_edges);
  return ret;
}

static void
compute_loops(input_exec_t *in)
{
  long i;

  for (i = 0; i < in->num_bbls; i++) {
    in->bbls[i].loop = compute_loop(in, &in->bbls[i]);
  }
}

void
analyze_cfg(input_exec_t *in)
{
  compute_dominators_and_postdominators(in);
  bbls_link_indir_jumps(in);
  bbl_compute_predecessors(in);
  compute_innermost_natural_loops(in);
  compute_flow(in);
  compute_liveness(in, 2);
  //compute_live_ranges(in, 2);
  //compute_rdefs(in);
  compute_loops(in);
}

static void
reloc_print(FILE *file, reloc_t const *reloc)
{
  fprintf(file, "%s : %s : 0x%lx : 0x%lx : %ld : %ld : %ld : 0x%x\n", reloc->symbolName,
      reloc->sectionName, (long)reloc->symbolValue, (long)reloc->offset, (long)reloc->type,
      (long)reloc->addend, (long)reloc->calcValue, reloc->orig_contents);
}

static void
symbol_print(FILE *file, symbol_t const *symbol)
{
  fprintf(file, "%s : 0x%lx : %ld : %hhd : %hhd : %d\n", symbol->name, (long)symbol->value,
      (long)symbol->size, symbol->bind, symbol->type, (int)symbol->shndx);
}

static void
dyn_entry_print(FILE *file, dyn_entry_t const *dyn_entry)
{
  fprintf(file, "%s : %ld : 0x%lx\n", dyn_entry->name, (long)dyn_entry->tag,
      (long)dyn_entry->value);
}

void
cfg_print(FILE *file, input_exec_t const *in)
{
  si_entry_t const *si;
  long i;

  fprintf(file, "CFG:");
  for (i = 0;  i < in->num_bbls; i++) {
    bbl_print(file, in, i, &in->bbls[i], lookup_symbol(in, in->bbls[i].pc));
  }

  fprintf(file, "\nSections:\n");
  for (int i = 0; i < in->num_input_sections; i++) {
    input_section_t const* s = &in->input_sections[i];
    fprintf(file, "index %d name %s addr %lx type %d flags %d size %d info %d align %d entry_size %d\n",
        s->orig_index, s->name, (long)s->addr, s->type, s->flags, s->size, s->info, s->align, s->entry_size);
    //printf("index %d name %s addr %lx type %d flags %d size %d info %d align %d entry_size %d\n",
    //    s->orig_index, s->name, (long)s->addr, s->type, s->flags, s->size, s->info, s->align, s->entry_size);
#if ARCH_SRC != ARCH_ETFG
    fprintf(file, "data ascii = {");
    if (s->data) {
      for (size_t d = 0; d < s->size; d++) {
        fprintf(file, "0x%hhx,", s->data[d]);
      }
    }
    fprintf(file, "}\n");
#endif
  }

  fprintf(file, "\nSymbols:\n");
  /*for (i = 0; i < in->num_symbols; i++) {
    fprintf(file, "SYMBOL%ld: ", i);
    symbol_print(file, &in->symbols[i]);
  }*/
  for (const auto &s : in->symbol_map) {
    fprintf(file, "SYMBOL%ld: ", (long)s.first);
    symbol_print(file, &s.second);
  }

  fprintf(file, "\nDynamic Symbols:\n");
  /*for (i = 0; i < in->num_dynsyms; i++) {
    fprintf(file, "DYNSYM%ld: ", i);
    symbol_print(file, &in->dynsyms[i]);
  }*/
  for (const auto &s : in->dynsym_map) {
    fprintf(file, "DYNSYM%ld: ", (long)s.first);
    symbol_print(file, &s.second);
  }

  fprintf(file, "\nRelocs:\n");
  for (i = 0; i < in->num_relocs; i++) {
    fprintf(file, "RELOC%ld: ", i);
    reloc_print(file, &in->relocs[i]);
  }

  fprintf(file, "\nDynamic Entries:\n");
  for (i = 0; i < in->num_dyn_entries; i++) {
    fprintf(file, "DYNENTRY%ld: ", i);
    dyn_entry_print(file, &in->dyn_entries[i]);
  }
  fprintf(file, "\nFunction type info:\n");
  for (i = 0; i < in->num_fun_type_info; i++) {
    fprintf(file, "FUNCTION%ld: ", i);
    fun_type_info_print(file, &in->fun_type_info[i]);
  }
}

long
pc_get_nextpcs(struct input_exec_t const *in, src_ulong pc, nextpc_t *nextpcs,
    double *nextpc_weights)
{
  bbl_t const *bbl;
  long bblindex;

  bbl = &in->bbls[pc_find_bbl(in, pc)];
  bblindex = bbl_insn_index(bbl, pc);
  if (bblindex == bbl->num_insns - 1) {
    //memcpy(nextpcs, bbl->nextpcs, bbl->num_nextpcs * sizeof(src_ulong));
    nextpc_t::nextpc_array_copy(nextpcs, bbl->nextpcs, bbl->num_nextpcs);
    if (nextpc_weights) {
      memcpy(nextpc_weights, bbl->nextpc_probs, bbl->num_nextpcs * sizeof(double));
    }
    CPP_DBG_EXEC(SRC_INSN, cout << __func__ << " " << __LINE__ << ": returning " << bbl->num_nextpcs << " for " << hex << pc << dec << endl);
    return bbl->num_nextpcs;
  }
  nextpcs[0] = bbl_insn_address(bbl, bblindex + 1);
  if (nextpc_weights) {
    nextpc_weights[0] = 1;
  }
  CPP_DBG_EXEC(SRC_INSN, cout << __func__ << " " << __LINE__ << ": returning " << bbl->num_nextpcs << " for " << hex << pc << dec << endl);
  return 1;
}

long
pc_get_preds(struct input_exec_t const *in, src_ulong pc, src_ulong *preds)
{
  bbl_t const *bbl;
  long bblindex;

  bbl = &in->bbls[pc_find_bbl(in, pc)];
  bblindex = bbl_insn_index(bbl, pc);
  if (bblindex == 0) {
    memcpy(preds, bbl->preds, bbl->num_preds * sizeof(src_ulong));
    return bbl->num_preds;
  }
  preds[0] = bbl_insn_address(bbl, bblindex - 1);
  return 1;
}

long
pc_get_next_edges_reducing_loops(struct input_exec_t const *in, src_ulong pc,
    edge_t *next_edges, double *next_edge_weights)
{
  nextpc_t nextpcs[MAX_NUM_NEXTPCS];
  long i, n, bblindex;
  bbl_t const *bbl;

  bbl = &in->bbls[pc_find_bbl(in, pc)];
  bblindex = bbl_insn_index(bbl, pc);
  if (!bbl->loop || bblindex != 0) {
    n = pc_get_nextpcs(in, pc, nextpcs, next_edge_weights);
    for (i = 0; i < n; i++) {
      next_edges[i].src = pc;
      next_edges[i].dst = nextpcs[i];
    }
    return n;
  }
#if HARVEST_LOOPS == 0
  return 0;
#endif
  //memcpy(next_edges, bbl->loop->out_edges, bbl->loop->num_out_edges * sizeof(edge_t));
  for (size_t i = 0; i < bbl->loop->num_out_edges; i++) {
    next_edges[i] = bbl->loop->out_edges[i];
  }
  memcpy(next_edge_weights, bbl->loop->out_edge_weights,
    bbl->loop->num_out_edges * sizeof(double));
  return bbl->loop->num_out_edges;
}

void
obtain_live_outs(input_exec_t const *in, regset_t *live_outs,
    nextpc_t const *nextpcs, long num_nextpcs)
{
  autostop_timer func_timer(__func__);
  long i;
  for (i = 0; i < num_nextpcs; i++) {
    si_entry_t const *si;
    long index;

    if (nextpcs[i].get_tag() != tag_const) {
      regset_copy(&live_outs[i], src_regset_live_at_jumptable());
      continue;
    }
    if (nextpcs[i].get_val() == PC_JUMPTABLE) {
      regset_copy(&live_outs[i], src_regset_live_at_jumptable());
      continue;
    }
    if (nextpcs[i].get_val() == PC_UNDEFINED) {
      regset_copy(&live_outs[i], regset_empty());
      continue;
    }
    index = pc2index(in, nextpcs[i].get_val());
    if (index == -1) {
      regset_copy(&live_outs[i], regset_empty());
      continue;
    }
    ASSERT(index >= 0 && index < in->num_si);
    si = &in->si[index];
    if (si->bbl->live_regs) {
      regset_copy(&live_outs[i], &si->bbl->live_regs[si->bblindex]);
    } else {
      regset_copy(&live_outs[i], regset_empty());
    }
  }
}

src_ulong
get_function_containing_pc(struct input_exec_t const *in, src_ulong pc)
{
  src_ulong ret;
  //long i;

  ret = INSN_PC_INVALID;
  for (const auto &s : in->symbol_map) {
    //symbol_print(stdout, &in->symbols[i]);
    //printf("%s() %d: in->symbols[i].value = %lx, pc = %lx\n", __func__, __LINE__, (unsigned long)in->symbols[i].value, (unsigned long)pc);
    //printf("%s() %d: in->symbols[i].type = %d, STT_FUNC = %d\n", __func__, __LINE__, (int)in->symbols[i].type, (int)STT_FUNC);
    if (   s.second.value <= pc
        && (ret == INSN_PC_INVALID || s.second.value > ret)
        && s.second.type == STT_FUNC) {
      ret = s.second.value;
    }
  }
  /*if (ret == INSN_PC_INVALID) {
    printf("%s() %d: could not find function pc for %lx\n", __func__, __LINE__, (unsigned long)pc);
  }
  ASSERT(ret != INSN_PC_INVALID);*/
  return ret;
}

src_ulong
function_name2pc(struct input_exec_t const *in, char const *function_name)
{
  //long i;

  for (const auto &s : in->symbol_map) {
    if (   !strcmp(s.second.name, function_name)
        && s.second.type == STT_FUNC) {
      return s.second.value;
    }
  }
  printf("%s(): failure for '%s'\n", __func__, function_name);
  NOT_REACHED();
  return INSN_PC_INVALID;
}

symbol_t const*
pc_has_associated_symbol(struct input_exec_t const *in, src_ulong pc)
{
  src_ulong orig_pc = pc_get_orig_pc(pc);
  CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": pc = 0x" << hex << pc << dec << ", orig_pc = 0x" << hex << pc << dec << "\n");
  for (const auto &s : in->symbol_map) {
    CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": symbol name " << s.second.name << " : value 0x" << hex << s.second.value << dec << " : size " << s.second.size << " : bind " << int(s.second.bind) << " : type " << int(s.second.type) << " : shndx " << int(s.second.shndx) << " : other " << int(s.second.other) << endl);
    if (   s.second.value == orig_pc
        /*&& s.second.type == STT_FUNC*/) {
      CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": returning symbol name " << s.second.name << endl);
      return &s.second;
    }
  }
  return NULL;
}

symbol_t const*
pc_has_associated_symbol_of_func_or_notype(struct input_exec_t const *in, src_ulong pc)
{
  src_ulong orig_pc = pc_get_orig_pc(pc);
  CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": pc = 0x" << hex << pc << dec << ", orig_pc = 0x" << hex << pc << dec << "\n");
  for (const auto &s : in->symbol_map) {
    CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": symbol name " << s.second.name << " : value 0x" << hex << s.second.value << dec << endl);
    if (   s.second.value == orig_pc
        && (s.second.type == STT_FUNC || s.second.type == STT_NOTYPE)) {
      CPP_DBG_EXEC(CFG, cout << __func__ << " " << __LINE__ << ": returning symbol name " << s.second.name << endl);
      return &s.second;
    }
  }
  return NULL;
}


bool
pc_is_reachable(struct input_exec_t const *in, src_ulong pc)
{
  long bblindex;
  bblindex = pc_find_bbl(in, pc);
  ASSERT(bblindex >= 0 && bblindex < in->num_bbls);
  return in->bbls[bblindex].reachable;
}

void
add_cfg_edge(cfg_edge_t *table[], long src, long newdst)
{
  cfg_edge_t *new_edge = (cfg_edge_t *)malloc(sizeof(cfg_edge_t));
  ASSERT(new_edge);
  new_edge->dst = newdst;
  new_edge->next = table[src];
  table[src] = new_edge;
}

