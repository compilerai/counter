#ifndef __BBL_H
#define __BBL_H

#include "support/types.h"
#include "support/src-defs.h"
#include "valtag/nextpc.h"

typedef
struct edge_t {
  src_ulong src;
  nextpc_t dst;
} edge_t;

typedef
struct loop_t {
  src_ulong *pcs;
  long num_pcs;
  edge_t *out_edges;
  double *out_edge_weights;
  long num_out_edges;
} loop_t;

typedef
struct bbl_t {
  src_ulong pc;            /* src pc */
  unsigned long size;      /* size of target code for this block */
  unsigned long num_insns;
  src_ulong *pcs;

  /* predecessors */
  src_ulong *preds;
  unsigned long num_preds;

  /* successors */
  nextpc_t *nextpcs;
  unsigned long num_nextpcs;

  /* function targets. */
  nextpc_t *fcalls;
  src_ulong *fcall_returns;
  unsigned long num_fcalls;

  /* loop struct, if any. */
  loop_t *loop;

  unsigned long indir_targets_start, indir_targets_stop;

  /* set during flow computation */
  unsigned long long in_flow;
  double *pred_weights;
  double *nextpc_probs;

  /* live registers at entry */
  struct regset_t *live_regs;

  /* live ranges */
  struct live_ranges_t *live_ranges;

  /* reaching definitions */
  struct rdefs_t *rdefs;

  /* whether this bbl is reachable from entry bbl or not */
  bool reachable;
} bbl_t;

unsigned long bbl_num_insns(bbl_t const *bbl);
src_ulong bbl_first_insn(bbl_t const *bbl);
src_ulong bbl_last_insn(bbl_t const *bbl);
src_ulong bbl_next_insn(bbl_t const *bbl, src_ulong iter);
src_ulong bbl_prev_insn(bbl_t const *bbl, src_ulong iter);
long bbl_insn_index(bbl_t const *bbl, src_ulong pc);
src_ulong bbl_insn_address(bbl_t const *bbl, unsigned long n);
src_ulong pc_copy(src_ulong orig_pc, long copy_id); //returns the effective pc by combining orig_pc and copy_id
//void bbl_fetch_insn(bbl_t const *bbl, unsigned long index, unsigned char *code);

#endif
