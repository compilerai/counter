#include <map>
#include "insn/dst-insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "insn/insn_list.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "support/c_utils.h"
#include "support/debug.h"
#include "support/types.h"
#include "rewrite/bbl.h"
#include "i386/cpu_consts.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "i386/insntypes.h"
#include "valtag/regmap.h"
#include "valtag/regcons.h"
#include "superopt/src_env.h"
#include "rewrite/nomatch_pairs_set.h"
//#include "ppc/regmap.h"
//#include "ppc/execute.h"
#include "i386/execute.h"
#include "insn/src-insn.h"
#include "rewrite/static_translate.h"
//#include "imm_map.h"
#include "valtag/imm_vt_map.h"
#include "rewrite/peephole.h"
#include "support/c_utils.h"
#include "i386/disas.h"
#include "insn/jumptable.h"
#include "support/src-defs.h"
#include "valtag/transmap.h"
//#include "temporaries.h"
#include "valtag/regcount.h"
#include "valtag/memset.h"
#include "valtag/elf/elf.h"
#include "rewrite/translation_instance.h"
//#include "imm_map.h"
#include "valtag/nextpc_map.h"
#include "support/timers.h"
#include "cutils/imap_elem.h"
#include "tfg/tfg.h"
//#include "sym_exec/sym_exec.h"
#include "exec/state.h"
extern "C" {
#include "gas/common/as.h"
void ppc_md_begin(void);
void i386_md_begin(void);
}
#include "ppc/insn.h"
//#include "rewrite/harvest.h"

static char as1[409600];
static char as2[4096];
static char as3[4096];
static char as4[4096];

#define CMN COMMON_DST
#include "cmn/cmn_fingerprintdb.h"
#include "cmn/cmn_src_insn.h"
#undef CMN


bool
dst_iseq_contains_nop(dst_insn_t const *iseq, size_t iseq_len)
{
  int i;

  for (i = 0; i < iseq_len; i++) {
    if (dst_insn_is_nop(&iseq[i])) {
      return true;
    }
  }
  return false;
}

void
gas_init(void)
{
  autostop_timer func_timer(__func__);
  symbol_begin();
  //frag_init();
  subsegs_begin();
  read_begin();
  input_scrub_begin();
  expr_begin();
#if (ARCH_SRC == ARCH_I386 || ARCH_DST == ARCH_I386 || ARCH_SRC == ARCH_X64 || ARCH_DST == ARCH_X64)
  i386_md_begin();
#endif
  text_section = subseg_new(TEXT_SECTION_NAME, 0);
  subseg_set(text_section, 0);
  //output_file_create("gas.dummy.outfile");
#if (ARCH_SRC == ARCH_PPC || ARCH_DST == ARCH_PPC)
  ppc_md_begin();
#endif
}

bool
dst_iseq_contains_div(dst_insn_t const *iseq, long iseq_len)
{
  long i;
  for (i = 0; i < iseq_len; i++) {
    if (dst_insn_is_intdiv(&iseq[i])) {
      return true;
    }
  }
  return false;
}

static long
dst_insns_unconditional_branch_collapse(dst_insn_t *src, dst_insn_t const *dst)
{
  src_tag_t pcrel_tag;
  unsigned long pcrel_val;

  if (!dst_insn_is_unconditional_direct_branch(dst)) {
    return -1;
  }
  if (dst_insn_is_function_call(dst)) {
    return -1;
  }
  if (!dst_insn_get_pcrel_value(dst, &pcrel_val, &pcrel_tag)) {
    return -1;
  }
  dst_insn_set_pcrel_value(src, pcrel_val, pcrel_tag);
  if (pcrel_tag == tag_const) {
    return pcrel_val;
  }
  return -1;
}

static bool 
dst_iseq_abs_inums_collapse_unconditional_chains(dst_insn_t *iseq, long iseq_len)
{
  src_tag_t pcrel_tag;
  unsigned long pcrel_val;
  long i;
  bool changed = false;
  //collapse unconditional branch chains
  for (i = 0; i < iseq_len; i++) {
    if (!dst_insn_get_pcrel_value(&iseq[i], &pcrel_val, &pcrel_tag)) {
      continue;
    }
    if (pcrel_tag != tag_abs_inum) {
      continue;
    }
    do {
      ASSERT(pcrel_val >= 0 && pcrel_val < iseq_len);
      pcrel_val = dst_insns_unconditional_branch_collapse(&iseq[i], &iseq[pcrel_val]);
      if (pcrel_val != (unsigned long)-1) {
        changed = true;
      }
    } while (pcrel_val != -1);
  }
  return changed;
}

static bool
dst_iseq_abs_inums_simplify_branch_patterns(dst_insn_t *iseq, long iseq_len)
{
  src_tag_t pcrel_tag;
  unsigned long pcrel_val;
  bool changed = false;
  //identify patterns of the form: .i0: jCC .i2; .i1: jmp .NEXTPC0; .i2:...
  //change to .i0: jNCC .NEXTPC0; i2: ... (jNCC is the negation of jCC)
  //also identify patterns of the form: .i0: jmp .i1; .i1: ...
  for (long i = 0; i < iseq_len; i++) {
    src_tag_t nextpc_tag;
    unsigned long nextpc_val;
    if (   i + 2 < iseq_len
        && dst_insn_is_conditional_direct_branch(&iseq[i])
        && dst_insn_get_pcrel_value(&iseq[i], &pcrel_val, &pcrel_tag)
        && pcrel_tag == tag_abs_inum
        && pcrel_val == i + 2
        && dst_insn_is_unconditional_direct_branch(&iseq[i + 1])
        && dst_insn_get_pcrel_value(&iseq[i + 1], &nextpc_val, &nextpc_tag)) {
      dst_insn_set_pcrel_value(&iseq[i], nextpc_val, nextpc_tag);
      dst_insn_invert_branch_condition(&iseq[i]);
      dst_insn_put_nop(&iseq[i + 1], 1);
      changed = true;
    } else if (   i + 1 < iseq_len
               && dst_insn_is_unconditional_direct_branch(&iseq[i])
               && dst_insn_get_pcrel_value(&iseq[i], &pcrel_val, &pcrel_tag)
               && pcrel_tag == tag_abs_inum
               && pcrel_val == i + 1) {
      dst_insn_put_nop(&iseq[i], 1);
      changed = true;
    }
  }
  return changed;
}

static long
dst_iseq_abs_inums_remove_dead_code(dst_insn_t *iseq, long iseq_len)
{
  bool reachable[iseq_len];
  long map[iseq_len];
  src_tag_t pcrel_tag;
  unsigned long pcrel_val;
  bool any_change;
  long i, j;

  for (i = 0; i < iseq_len; i++) {
    reachable[i] = false;
  }
  ASSERT(iseq_len > 0);
  reachable[0] = true;

  do {
    any_change = false;
    for (i = 0; i < iseq_len; i++) {
      if (!reachable[i]) {
        continue;
      }
      if (dst_insn_is_unconditional_indirect_branch_not_fcall(&iseq[i])) {
        continue;
      }
      if (!dst_insn_get_pcrel_value(&iseq[i], &pcrel_val, &pcrel_tag)) {
        reachable[i + 1] = true;
        continue;
      }
      if (dst_insn_is_function_call(&iseq[i])) {
        reachable[i + 1] = true;
      }
      if (   dst_insn_is_conditional_direct_branch(&iseq[i])
          || dst_insn_is_conditional_indirect_branch(&iseq[i])) {
        reachable[i + 1] = true;
      }
      if (pcrel_tag == tag_abs_inum) {
        ASSERT(pcrel_val >= 0 && pcrel_val < iseq_len);
        if (!reachable[pcrel_val]) {
          reachable[pcrel_val] = true;
          any_change = true;
        }
      }
    }
  } while (any_change);

  j = 0;
  for (i = 0; i < iseq_len; i++) {
    if (   !reachable[i]
        || dst_insn_is_nop(&iseq[i])
        || (   dst_insn_get_pcrel_value(&iseq[i], &pcrel_val, &pcrel_tag)
            && pcrel_val == i + 1
            && pcrel_tag == tag_abs_inum)) {
      map[i] = -1;
      continue;
    }
    ASSERT(j <= i);
    if (j < i) {
      dst_insn_copy(&iseq[j], &iseq[i]);
    }
    map[i] = j;
    j++;
  }
  long new_iseq_len = j;

  dst_iseq_rename_abs_inums_using_map(iseq, new_iseq_len, map, iseq_len);
  return new_iseq_len;
}

long
dst_iseq_optimize_branching_code(dst_insn_t *iseq, long iseq_len, long iseq_size)
{
  //long i, j, iseq_copy_len;
  //dst_insn_t *iseq_copy;

  if (!iseq_len) {
    return 0;
  }
  ASSERT(iseq_len);
  //iseq_copy = new dst_insn_t[iseq_len];
  //ASSERT(iseq_copy);
  //dst_iseq_copy(iseq_copy, iseq, iseq_len);
  //iseq_copy_len = iseq_len;

  dst_iseq_convert_pcrel_inums_to_abs_inums(iseq, iseq_len);

  bool something_changed = true;
  while (something_changed) {
    something_changed = false;
    something_changed = dst_iseq_abs_inums_collapse_unconditional_chains(iseq, iseq_len) || something_changed;
    something_changed = dst_iseq_abs_inums_simplify_branch_patterns(iseq, iseq_len) || something_changed;
    long new_iseq_len = dst_iseq_abs_inums_remove_dead_code(iseq, iseq_len);
    ASSERT(new_iseq_len <= iseq_len);
    if (new_iseq_len < iseq_len) {
      iseq_len = new_iseq_len;
      something_changed  = true;
    }
  }

  dst_iseq_convert_abs_inums_to_pcrel_inums(iseq, iseq_len);

  //XXX: ensure that the last nextpc remains last, may need inverting branch conditions
  //free(iseq_copy);
  //delete [] iseq_copy;
  DBG(CONNECT_ENDPOINTS, "after:\n%s\n",
      dst_iseq_to_string(iseq, iseq_len, as1, sizeof as1));
  return iseq_len;
}

void
dst_iseq_pick_reg_assignment(regmap_t *regmap, dst_insn_t const *dst_iseq,
    long dst_iseq_len)
{
  regcons_t regcons;
  bool ret;

  dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1);
  ret = dst_infer_regcons_from_assembly(as1, &regcons);
  ASSERT(ret);
  ret = regmap_assign_using_regcons(regmap, &regcons, ARCH_DST, NULL, fixed_reg_mapping_t());
  ASSERT(ret);
  //printf("as1:\n%s\n", as1);
  //printf("regcons:\n%s\n", regcons_to_string(&regcons, as1, sizeof as1));
  //printf("regmap:\n%s\n", regmap_to_string(regmap, as1, sizeof as1));
}

void
dst_iseq_add_insns_to_exit_paths(dst_insn_t *dst_iseq, long *dst_iseq_len,
    size_t dst_iseq_size, dst_insn_t const *exit_insns, long num_exit_insns)
{
  long i, len, nextpc_num, indir_branch_code_start;
  long nextpc_map[*dst_iseq_len];
  dst_insn_t *ptr, *end;
  bool is_indir_branch;
  //valtag_t indir_op;
  valtag_t *op;

  if (num_exit_insns == 0) {
    return;
  }
  len = *dst_iseq_len;

  memset(nextpc_map, 0xff, sizeof nextpc_map);
  indir_branch_code_start = -1;

  ptr = dst_iseq + *dst_iseq_len;
  end = dst_iseq + dst_iseq_size;
  for (i = 0; i < len; i++) {
    if (   (op = dst_insn_get_pcrel_operand(&dst_iseq[i]))
        && op->tag == tag_var) {
      nextpc_num = op->val;
      if (nextpc_map[nextpc_num] == -1) {
        nextpc_map[nextpc_num] = ptr - dst_iseq;
        dst_iseq_copy(ptr, exit_insns, num_exit_insns);
        ptr += num_exit_insns;
        ASSERT(ptr < end);
        dst_insn_put_jump_to_nextpc(ptr, end - ptr, nextpc_num);
        ptr++;
        ASSERT(ptr < end);
      }
      op->val = nextpc_map[nextpc_num] - (i + 1);
      op->tag = tag_const;
    }

    is_indir_branch = dst_insn_is_indirect_branch(&dst_iseq[i]);
    //dst_insn_get_indir_branch_operand(&indir_op, &dst_iseq[i]);

    if (is_indir_branch) {
      if (indir_branch_code_start == -1) {
        indir_branch_code_start = ptr - dst_iseq;
        dst_iseq_copy(ptr, exit_insns, num_exit_insns);
        ptr += num_exit_insns;
        dst_insn_copy(ptr, &dst_iseq[i]);
        ptr++;
        ASSERT(ptr < end);
      }
      dst_insn_put_jump_to_pcrel(indir_branch_code_start - (i + 1), &dst_iseq[i], 1);
    }
  }
  ASSERT(ptr < end);
  *dst_iseq_len = ptr - dst_iseq;
}

static bool
nextpc_occurs_only_as_fcall_arg_in_dst_iseq(dst_insn_t const *iseq, size_t len, src_ulong nextpc)
{
  valtag_t *op;
  for (size_t i = 0; i < len; i++) {
    if (   (op = dst_insn_get_pcrel_operand(&iseq[i]))
        && op->tag == tag_var
        && op->val == nextpc) {
      if (!dst_insn_is_function_call(&iseq[i])) {
        return false;
      }
    }
  }
  //cout << __func__ << " " << __LINE__ << ": returning true for nextpc " << hex << nextpc << dec << " in:\n" << dst_iseq_to_string(iseq, len, as1, sizeof as1) << endl;
  return true;
}

long
dst_iseq_connect_end_points(dst_insn_t *iseq, long len, size_t iseq_size,
    nextpc_t const *nextpcs,
    //uint8_t const * const *next_tcodes,
    //txinsn_t const *next_tinsns,
    transmap_t const *out_tmaps,
    transmap_t const * const *next_tmaps, regset_t const *live_outs,
    long num_nextpcs, bool redir_indir_branch,
    eqspace::state const *start_state, int memloc_offset_reg,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    map<symbol_id_t, symbol_t> const& symbol_map,
    i386_as_mode_t mode)
{
  transmap_t const *indir_tmap, *indir_out_tmap = NULL;
  dst_insn_t *retptr, *iseq_end;
  regset_t const *indir_live_out;
  long i, j, indir_branch_code_start;
  long map[num_nextpcs];
  valtag_t *op;

  retptr = &iseq[len];
  iseq_end = iseq + iseq_size;
  indir_tmap = NULL;
  indir_live_out = NULL;
  DBG(CONNECT_ENDPOINTS, "before:\n%s\n",
      dst_iseq_to_string(iseq, len, as1, sizeof as1));
  DBG(CONNECT_ENDPOINTS, "before:\nnum_nextpcs = %ld\n", num_nextpcs);

  /* first deal with all the direct branches */
  for (i = 0; i < num_nextpcs; i++) {
    DBG(CONNECT_ENDPOINTS, "nextpcs[%ld]=%lx\n", i, (long)nextpcs[i].get_val());
    if (nextpcs[i].get_val() == PC_UNDEFINED) {
      map[i] = PC_UNDEFINED;
      continue;
    }
    if (nextpcs[i].get_val() == PC_JUMPTABLE) {
      map[i] = PC_JUMPTABLE;
      DBG(CONNECT_ENDPOINTS, "found pc_jumptable nextpc%ld\n", i);
      ASSERT(!indir_tmap);
      ASSERT(!indir_live_out);
      indir_tmap = next_tmaps[i];
      indir_live_out = &live_outs[i];
      indir_out_tmap = &out_tmaps[i];
      continue;
    }
    map[i] = retptr - iseq;
    if (   !PC_IS_NEXTPC_CONST(nextpcs[i].get_val())
        && !nextpc_occurs_only_as_fcall_arg_in_dst_iseq(iseq, len, i)
        && !pc_is_a_nextpc_symbol(symbol_map, nextpcs[i].get_val())) {
      DBG(CONNECT_ENDPOINTS, "Adding transmap_translation_insns for nextpc%ld at "
          "%ld. out_tmap:\n%s\nnext_tmap:\n%s\nlive_out:\n%s\n", i, map[i],
          transmap_to_string(&out_tmaps[i], as1, sizeof as1),
          transmap_to_string(next_tmaps[i], as2, sizeof as2),
          regset_to_string(&live_outs[i], as3, sizeof as3));
      DYN_DBG(tmap_trans, "  nextpc %lx: calling transmap_translation_insns\n",
          (long)nextpcs[i].get_val());
      retptr += transmap_translation_insns(&out_tmaps[i], next_tmaps[i],
          start_state, retptr, iseq_end - retptr, memloc_offset_reg,
          dst_fixed_reg_mapping,
          mode);
      ASSERT(retptr < iseq_end);
    }

    DBG(CONNECT_ENDPOINTS, "Adding branch-out for nextpc%ld at "
        "%zd.\n", i, retptr - iseq);
    //we use nextpc (tag_var), as tag_vars are converted
    //to pc-relative representation. see i386_iseq_assemble
    //retptr += dst_insn_put_jump_to_nextpc(retptr, iseq_end - retptr,
    //    (long)next_tcodes[i]);

    /*if (next_tinsns[i].txinsn_get_tag() == tag_var) {
      retptr += dst_insn_put_jump_to_nextpc(retptr, iseq_end - retptr,
          (long)next_tinsns[i].txinsn_get_val());
    } else {
      retptr += dst_insn_put_jump_to_pcrel((dst_insn_t *)next_tinsns[i].txinsn_get_val() - (retptr + 1), retptr, iseq_end - retptr);
    }*/
    retptr += dst_insn_put_jump_to_src_insn_pc(retptr, iseq_end - retptr,
        (long)nextpcs[i].get_val());
    ASSERT(retptr < iseq_end);
  }

  for (i = 0; i < len; i++) {
    if (   (op = dst_insn_get_pcrel_operand(&iseq[i]))
        && op->tag == tag_var) {
      if (op->val >= 0 && op->val < num_nextpcs) {
        DBG(CONNECT_ENDPOINTS, "op->val.pcrel=%ld, map[%ld]=%ld\n", op->val,
            op->val, map[op->val]);
        /*ASSERT(   map[op->val.pcrel] != PC_JUMPTABLE
               && map[op->val.pcrel] != PC_UNDEFINED);*/
        ASSERT(map[op->val] != PC_JUMPTABLE);
        op->val = map[op->val] - (i + 1);
        op->tag = tag_const;
      }
    }
  }

  len = retptr - iseq;

  dst_iseq_convert_pcrel_inums_to_abs_inums(iseq, len);


  long *insn_map = new long[len];
  long iseq_copy_len;
  dst_insn_t *iseq_copy;

  //iseq_copy = (dst_insn_t *)malloc(len * sizeof(dst_insn_t));
  iseq_copy = new dst_insn_t[len];
  ASSERT(iseq_copy);
  dst_iseq_copy(iseq_copy, iseq, len);
  iseq_copy_len = len;

  j = 0;
  for (i = 0; i < len; i++) {
    bool is_indir_branch;

    is_indir_branch = dst_insn_is_indirect_branch(&iseq_copy[i]);

    insn_map[i] = j;

#if ARCH_DST == ARCH_I386
    operand_t indir_op;
    i386_insn_get_indir_branch_operand(&indir_op, &iseq_copy[i]);
#else
    NOT_IMPLEMENTED();
#endif

    if (redir_indir_branch && is_indir_branch) {
      ASSERT(indir_tmap);
      ASSERT(indir_out_tmap);
      ASSERT(indir_live_out);
      indir_branch_code_start = len;
#if ARCH_DST == ARCH_I386
      j += i386_indir_branch_redir(&iseq[j], iseq_end - &iseq[j],
          &indir_op, &iseq_copy[i], indir_out_tmap, indir_tmap, indir_live_out, start_state, memloc_offset_reg, dst_fixed_reg_mapping, mode);
#else
      NOT_IMPLEMENTED();
#endif
    } else {
      dst_insn_copy(&iseq[j], &iseq_copy[i]);
      j++;
    }
  }
  len = j;
  ASSERT(len >= iseq_copy_len);
  ASSERT(len < iseq_end - iseq);
  //free(iseq_copy);
  delete [] iseq_copy;

  dst_iseq_rename_abs_inums_using_map(iseq, len, insn_map, iseq_copy_len);
  dst_iseq_convert_abs_inums_to_pcrel_inums(iseq, len);
  /*for (i = 0; i < len; i++) {
    src_tag_t pcrel_tag;
    uint64_t pcrel_val;

    if (!dst_insn_get_pcrel_value(&iseq[i], &pcrel_val, &pcrel_tag)) {
      continue;
    }
    if (pcrel_tag != tag_abs_inum) {
      continue;
    }
    ASSERT(pcrel_val >= 0 && pcrel_val < iseq_copy_len);
    pcrel_val = insn_map[pcrel_val];
    ASSERT(pcrel_val >= 0 && pcrel_val < len);
    dst_insn_set_pcrel_value(&iseq[i], pcrel_val, pcrel_tag);
  }*/
  delete [] insn_map;
  DBG(CONNECT_ENDPOINTS, "after:\n%s\n",
      dst_iseq_to_string(iseq, len, as1, sizeof as1));
  return dst_iseq_optimize_branching_code(iseq, len, iseq_size);
}

void
dst_iseq_rename_vconsts(dst_insn_t *iseq, long iseq_len,
    imm_vt_map_t const *imm_map/*,
    src_ulong const *nextpcs, long num_nextpcs, regmap_t const *regmap*/)
{
  long i;

  for (i = 0; i < iseq_len; i++) {
    dst_insn_rename_constants(&iseq[i], imm_map/*, nextpcs, num_nextpcs, regmap*/);
  }
}

static void
dst_insn_replace_nextpc_with_pcrel(long pcrel_val, dst_insn_t *insn)
{
  valtag_t *op;

  if (   (op = dst_insn_get_pcrel_operand(insn))
      && op->tag == tag_var) {
    op->tag = tag_const;
    op->val = pcrel_val;
  }
}

long
dst_iseq_make_position_independent(dst_insn_t *iseq, long iseq_len,
    long iseq_size, uint32_t scratch_addr)
{
  long i, j, nextpc_num, orig_insn, fallthrough_insn, taken_branch_insn, ret;
  dst_insn_t *iseq_copy;
  long map[iseq_len + 1];
  valtag_t *op;

  dst_iseq_convert_pcrel_inums_to_abs_inums(iseq, iseq_len);

  //iseq_copy = (dst_insn_t *)malloc(iseq_len * sizeof(dst_insn_t));
  iseq_copy = new dst_insn_t[iseq_len];
  ASSERT(iseq_copy);
  dst_iseq_copy(iseq_copy, iseq, iseq_len);

  for (i = 0, j = 0; i < iseq_len; i++) {
    map[i] = j;
    if (   (op = dst_insn_get_pcrel_operand(&iseq_copy[i]))
        && op->tag == tag_var) {
      nextpc_num = op->val;
      ASSERT(   (nextpc_num >= 0 && nextpc_num < iseq_len)
             || nextpc_num == NEXTPC_TYPE_ERROR);
     
      orig_insn = j;
      dst_insn_copy(&iseq[j], &iseq_copy[i]);
      j++;
      ASSERT(j < iseq_size);

      fallthrough_insn = j;
      j++;

      taken_branch_insn = j;
      ASSERT(j < iseq_size);
      j += dst_insn_put_movl_nextpc_constant_to_mem(nextpc_num, scratch_addr,
          &iseq[j], iseq_size - j);
      j += dst_insn_put_jump_to_indir_mem(scratch_addr, &iseq[j], iseq_size - j);
      ASSERT(iseq_len < iseq_size);

      dst_insn_replace_nextpc_with_pcrel(taken_branch_insn - (orig_insn + 1),
          &iseq[orig_insn]);
      ret = dst_insn_put_jump_to_pcrel(j - (fallthrough_insn + 1),
          &iseq[fallthrough_insn], 1);
      ASSERT(ret == 1);
    } else {
      dst_insn_copy(&iseq[j], &iseq_copy[i]);
      j++;
    }
    ASSERT(j < iseq_size);
  }
  map[i] = j;
  //free(iseq_copy);
  delete [] iseq_copy;

  dst_iseq_rename_abs_inums_using_map(iseq, j, map, iseq_len + 1);
  dst_iseq_convert_abs_inums_to_pcrel_inums(iseq, j);

  return j;
}

static void
dst_iseq_get_c2v_regmap(regmap_t *c2v, dst_insn_t const *dst_iseq, long dst_iseq_len, fixed_reg_mapping_t const &dst_fixed_reg_mapping)
{
  static char assembly[409600];
  regcons_t regcons;
  regmap_t regmap;
  bool ret;

  dst_iseq_to_string(dst_iseq, dst_iseq_len, assembly, sizeof assembly);
  ret = dst_infer_regcons_from_assembly(assembly, &regcons);
  ASSERT(ret);
  //cout << __func__ << " " << __LINE__ << ": regcons =\n" << regcons_to_string(&regcons, as1, sizeof as1) << endl;
  regmap_init(&regmap);
  ret = regmap_assign_using_regcons(&regmap, &regcons, ARCH_DST, NULL, dst_fixed_reg_mapping);
  ASSERT(ret);
  //cout << __func__ << " " << __LINE__ << ": regmap =\n" << regmap_to_string(&regmap, as1, sizeof as1) << endl;
  regmap_invert(c2v, &regmap);
  //cout << __func__ << " " << __LINE__ << ": c2v =\n" << regmap_to_string(c2v, as1, sizeof as1) << endl;
}

static bool
regmap_maps_dst_reserved_register_to_this_reg(regmap_t const *regmap, exreg_group_id_t group, exreg_id_t this_reg)
{
  if (regmap->regmap_extable.count(group) == 0) {
    return false;
  }
  map<reg_identifier_t, reg_identifier_t> const &m = regmap->regmap_extable.at(group);
  for (const auto &p : m) {
    if (   p.second == this_reg
        && regset_belongs_ex(&dst_reserved_regs, group, p.first)) {
      return true;
    }
  }
  return false;
}

static int
dst64_iseq_find_unused_gpr_var(dst_insn_t const *insns, int n_insns, regmap_t const *c2v)
{
  regset_t def, touch;
  dst64_iseq_get_usedef_regs(insns, n_insns, &touch, &def);
  regset_union(&touch, &def);
  for (exreg_id_t r = 0; r < DST_NUM_EXREGS(DST_EXREG_GROUP_GPRS); r++) {
    if (regset_belongs_ex(&touch, DST_EXREG_GROUP_GPRS, r)) {
      continue;
    }
    if (regmap_maps_dst_reserved_register_to_this_reg(c2v, DST_EXREG_GROUP_GPRS, r)) {
      continue;
    }
    return r;
  }
  return -1;
#if 0
  bool available[DST_NUM_GPRS];
  int i, j;

  memset(available, 0, sizeof available);
  for (i = 0; i < I386_NUM_GPRS; i++) {
    if (regmap_get_elem(c2v, I386_EXREG_GROUP_GPRS, i).reg_identifier_get_id() != R_ESP) {
      available[i] = true;
    }
  }

  for (i = 0; i < n_insns; i++) {
    if (dst_insn_is_nop(&insns[i])) {
      continue;
    }
#if ARCH_DST == ARCH_I386
    for (j = 0; j < I386_MAX_NUM_OPERANDS; j++) {
      if (insns[i].op[j].type == op_reg) {
        ASSERT(insns[i].op[j].valtag.reg.tag == tag_var);
        ASSERT(   insns[i].op[j].valtag.reg.val >= 0
               && insns[i].op[j].valtag.reg.val < I386_NUM_GPRS);
        available[insns[i].op[j].valtag.reg.val] = false;
      } else if (insns[i].op[j].type == op_mem) {
        //ASSERT(insns[i].op[j].tag.mem.all == tag_const);
        if (insns[i].op[j].valtag.mem.base.val != -1) {
          ASSERT(insns[i].op[j].valtag.mem.base.tag == tag_var);
          ASSERT(   insns[i].op[j].valtag.mem.base.val >= 0
                 && insns[i].op[j].valtag.mem.base.val < I386_NUM_GPRS);
          available[insns[i].op[j].valtag.mem.base.val] = false;
        }
        if (insns[i].op[j].valtag.mem.index.val != -1) {
          ASSERT(insns[i].op[j].valtag.mem.index.tag == tag_var);
          ASSERT(   insns[i].op[j].valtag.mem.index.val >= 0
                 && insns[i].op[j].valtag.mem.index.val < I386_NUM_GPRS);
          available[insns[i].op[j].valtag.mem.index.val] = false;
        }
      }
    }
#else
    NOT_IMPLEMENTED();
#endif
  }
  for (i = 0; i < I386_NUM_GPRS; i++) {
    if (available[i]) {
      return i;
    }
  }
  return -1;
#endif
}

long
dst_iseq_sandbox_without_converting_sboxed_abs_inums_to_pcrel_inums(dst_insn_t *dst_iseq, long dst_iseq_len, long dst_iseq_size,
    int tmpreg, fixed_reg_mapping_t const &dst_fixed_reg_mapping, long cur_insn_num, long *insn_num_map)
{
  dst_insn_t dst_iseq_tmp[dst_iseq_len], *iptr, *iend;
  regmap_t c2v;
  int r_ds;
  long i;

  dst_iseq_copy(dst_iseq_tmp, dst_iseq, dst_iseq_len);
  dst_iseq_get_c2v_regmap(&c2v, dst_iseq_tmp, dst_iseq_len, dst_fixed_reg_mapping);

  //regset_clear(ctemps);

  iptr = dst_iseq;
  iend = dst_iseq + dst_iseq_size;

  r_ds = regmap_get_elem(&c2v, I386_EXREG_GROUP_SEGS, R_DS).reg_identifier_get_id();
  int r_esp = regmap_get_elem(&c2v, I386_EXREG_GROUP_GPRS, R_ESP).reg_identifier_get_id();
  ASSERT(tmpreg != r_esp);

  if (tmpreg == -1) {
    tmpreg = dst64_iseq_find_unused_gpr_var(dst_iseq_tmp, dst_iseq_len, &c2v);
    ASSERT(tmpreg != r_esp);
  }
  //CPP_DBG_EXEC(EQUIV, cout << __func__ << " " << __LINE__  << ": tmpreg = " << tmpreg << endl);

  for (i = 0; i < dst_iseq_len; i++) {
    int itmpreg = -1;

    if (insn_num_map) {
      insn_num_map[i] = iptr - dst_iseq;
    }

    if (tmpreg == -1) {
      itmpreg = dst64_iseq_find_unused_gpr_var(&dst_iseq_tmp[i], 1, &c2v);
      ASSERT(itmpreg >= 0 && itmpreg < I386_NUM_EXREGS(I386_EXREG_GROUP_GPRS));
      ASSERT(itmpreg != r_esp);
      
      iptr += dst_insn_put_movl_reg_var_to_mem(SRC_ENV_TMPREG_STORE, itmpreg, r_ds,
          iptr, iend - iptr);
      ASSERT(iptr < iend);
    } else {
      itmpreg = tmpreg;
      ASSERT(itmpreg != r_esp);
    }
    //dst_insn_sandbox() converts tag_const pcrels to tag_sboxed_abs_inum; the call later to dst_iseq_rename_abs_inums_using_map() then follows-up on that
    iptr += dst_insn_sandbox(iptr, iend - iptr, &dst_iseq_tmp[i], &c2v/*, ctemps*/,
        itmpreg, cur_insn_num + i);
    ASSERT(iptr < iend);
    if (tmpreg == -1) {
      ASSERT(itmpreg != -1);
      iptr += dst_insn_put_movl_mem_to_reg_var(itmpreg, SRC_ENV_TMPREG_STORE, r_ds,
          iptr, iend - iptr);
      ASSERT(iptr < iend);
    }
  }

  return iptr - dst_iseq;
}

long
dst_iseq_sandbox(dst_insn_t *dst_iseq, long dst_iseq_len, long dst_iseq_size,
    int tmpreg/*, regset_t *ctemps*/, fixed_reg_mapping_t const &dst_fixed_reg_mapping)
{
  long insn_num_map[dst_iseq_len];

  long dst_iseq_sandboxed_len = dst_iseq_sandbox_without_converting_sboxed_abs_inums_to_pcrel_inums(dst_iseq, dst_iseq_len, dst_iseq_size,
      tmpreg, dst_fixed_reg_mapping, 0, insn_num_map);

  dst_iseq_rename_sboxed_abs_inums_using_map(dst_iseq, dst_iseq_sandboxed_len, insn_num_map, dst_iseq_len);
  dst_iseq_convert_sboxed_abs_inums_to_pcrel_inums(dst_iseq, dst_iseq_sandboxed_len);

  //cout << __func__ << " " << __LINE__ << ": returning " << dst_iseq_to_string(dst_iseq, iptr - dst_iseq, as1, sizeof as1) << endl;

  //ctemps->excregs[I386_EXREG_GROUP_GPRS][r_esp] = 0xffffffff;
  //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ds] = 0xffff;
  //ctemps->excregs[I386_EXREG_GROUP_SEGS][r_ss] = 0xffff;

  return dst_iseq_sandboxed_len;
}

void
dst_iseq_compute_fingerprint(uint64_t *fingerprint,
    dst_insn_t const *dst_iseq, long dst_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    regset_t const *live_out, long num_live_out, bool mem_live_out,
    rand_states_t const &rand_states)
{
  NOT_REACHED();
}

static bool
dst_iseq_inv_rename_locals_symbols(dst_insn_t *dst_iseq, long dst_iseq_len,
    imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  bool ret;
  long i;
  ret = true;
  for (i = 0; i < dst_iseq_len; i++) {
    if (!dst_insn_inv_rename_locals_symbols(&dst_iseq[i], imm_vt_map, tag)) {
      DBG(H2P, "returning false for inv_renaming %ldth insn\n", i);
      ret = false;
    }
  }
  return ret;
}

bool
dst_iseq_inv_rename_symbols(dst_insn_t *dst_iseq, long dst_iseq_len,
    imm_vt_map_t *imm_vt_map)
{
  return dst_iseq_inv_rename_locals_symbols(dst_iseq, dst_iseq_len, imm_vt_map,
      tag_imm_symbol);
}

bool
dst_iseq_inv_rename_locals(dst_insn_t *dst_iseq, long dst_iseq_len,
    imm_vt_map_t const *imm_vt_map)
{
  return dst_iseq_inv_rename_locals_symbols(dst_iseq, dst_iseq_len, (imm_vt_map_t *)imm_vt_map,
      tag_imm_local);
}

void
dst_iseq_rename_symbols_to_addresses(dst_insn_t *dst_iseq, long dst_iseq_len,
    input_exec_t const *in, struct chash const *tcodes)
{
  long i;
  for (i = 0; i < dst_iseq_len; i++) {
    dst_insn_rename_symbols_to_addresses(&dst_iseq[i], in, tcodes);
  }
}

unsigned long
dst_iseq_hash_func(struct dst_insn_t const *insns, int n_insns)
{
  unsigned long ret;
  int i;

  ret = 104729 * n_insns;
  for (i = 0; i < n_insns; i++) {
    ret += (i + 1) * 3989 * dst_insn_hash_func(&insns[i]);
  }
  return ret;
}

bool
dst_insn_get_pcrel_value(struct dst_insn_t const *insn, unsigned long *pcrel_val,
    src_tag_t *pcrel_tag)
{
  vector<valtag_t> vs = dst_insn_get_pcrel_values(insn);
  ASSERT(vs.size() <= 1);
  if (vs.size() == 0) {
    return false;
  }
  *pcrel_val = vs.at(0).val;
  *pcrel_tag = vs.at(0).tag;
  return true;
}

void
dst_insn_set_pcrel_value(struct dst_insn_t *insn, unsigned long pcrel_val,
    src_tag_t pcrel_tag)
{
  vector<valtag_t> vs;
  if (pcrel_tag != tag_invalid) {
    valtag_t vt = { .val = (long)pcrel_val, .tag = pcrel_tag };
    vs.push_back(vt);
  }
  dst_insn_set_pcrel_values(insn, vs);
}

valtag_t *
dst_insn_get_pcrel_operand(dst_insn_t const *insn)
{
  return dst_insn_get_pcrel_operands(insn);
}

int
dst_get_callee_save_regnum(int index)
{
  const auto &dst_gprs = dst_callee_save_regs.excregs.at(DST_EXREG_GROUP_GPRS);
  vector<int> dst_gprs_vector;
  int num_gprs = dst_gprs.size();
  ASSERT(index >= 0 && index < num_gprs);
  for (auto iter = dst_gprs.begin(); iter != dst_gprs.end(); iter++) {
    dst_gprs_vector.push_back(iter->first.reg_identifier_get_id());
  }
  sort(dst_gprs_vector.begin(), dst_gprs_vector.end());
  return dst_gprs_vector.at(index);
}

size_t
dst_iseq_get_idx_for_marker(dst_insn_t const *dst_iseq, size_t dst_iseq_len, size_t marker)
{
  size_t num_seen = 0;
  for (size_t i = 0; i < dst_iseq_len; i++) {
    if (dst_iseq_has_marker_prefix(&dst_iseq[i], dst_iseq_len - i)) {
      if (num_seen == marker) {
        return i;
      }
      num_seen++;
    }
  }
  NOT_REACHED();
}

void
dst_iseq_patch_jump_to_nextpcs_using_markers(dst_insn_t *dst_iseq, size_t dst_iseq_len, vector<size_t> const &nextpc_code_indices, size_t num_nextpcs)
{
  vector<size_t> markers;
  for (size_t i = 0; i < dst_iseq_len; i++) {
    if (dst_iseq_has_marker_prefix(&dst_iseq[i], dst_iseq_len - i)) {
      markers.push_back(i);
    }
  }
  ASSERT(markers.size() > num_nextpcs);
  for (size_t i = 0; i < num_nextpcs; i++) {
    size_t idx = nextpc_code_indices.at(i);
    ASSERT(idx < markers.size());
    size_t dst_iseq_idx = markers.at(idx);
    dst_insn_put_jump_to_nextpc(&dst_iseq[dst_iseq_idx], 1, i);
  }
/*
  ASSERT(markers.size() >= stop_marker);
  ASSERT(stop_marker > start_marker);
  size_t start = markers.at(start_marker) + 1;
  size_t stop = markers.at(stop_marker);
  ASSERT(start <= stop);
  dst_iseq_copy(dst_iseq, &dst_iseq[start], stop - start);
  return stop - start;
*/
}

static void
dst64_insn_get_usedef_regs(struct dst_insn_t const *insn,
    regset_t *use, regset_t *def)
{
#if ARCH_DST == ARCH_I386
  if (!x64_insn_get_usedef_regs(insn, use, def))
#endif
  {
    dst_insn_get_usedef_regs(insn, use, def);
  }
}

void
dst64_iseq_get_usedef_regs(struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    regset_t *use, regset_t *def)
{
  regset_clear(use);
  regset_clear(def);
  for (int i = dst_iseq_len - 1; i >= 0; i--) {
    regset_t iuse, idef;

    dst64_insn_get_usedef_regs(&dst_iseq[i], &iuse, &idef);

    regset_diff(use, &idef);
    regset_union(def, &idef);
    regset_union(use, &iuse);
  }
}

void
dst_iseq_rename_cregs_to_vregs(dst_insn_t *iseq, long iseq_len)
{
  for (long i = 0; i < iseq_len; i++) {
    dst_insn_rename_cregs_to_vregs(&iseq[i]);
  }
}

void
dst_iseq_rename_vregs_to_cregs(dst_insn_t *iseq, long iseq_len)
{
  for (long i = 0; i < iseq_len; i++) {
    dst_insn_rename_vregs_to_cregs(&iseq[i]);
  }
}

static bool
dst_insn_is_related_to_dst_iseq(dst_insn_t const &dst_insn, dst_insn_t const *dst_iseq, long dst_iseq_len)
{
  if (dst_opcodes_related(dst_insn, dst_iseq, dst_iseq_len)) {
    return true;
  }
  return false;
}

static bool
dst_insn_vec_is_related_to_dst_iseq(vector<dst_insn_t> const &dst_insn_vec,
    dst_insn_t const *dst_iseq, long dst_iseq_len)
{
  DBG(DST_INSN_RELATED, "dst_iseq:\n%s\n", dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
  DBG(DST_INSN_RELATED, "dst_insn_vec:\n%s\n", dst_insn_vec_to_string(dst_insn_vec, as1, sizeof as1));
  for (const auto &dst_insn : dst_insn_vec) {
    if (!dst_insn_is_related_to_dst_iseq(dst_insn, dst_iseq, dst_iseq_len)) {
      DBG(DST_INSN_RELATED, "%s", "returning false\n");
      return false;
    }
  }
  DBG(DST_INSN_RELATED, "%s", "returning true\n");
  return true;
}

static bool
dst_insn_vec_accesses_memory(vector<dst_insn_t> const &insn_vec)
{
  memset_t memuse, memdef;
  regset_t use, def;
  memset_init(&memuse);
  memset_init(&memdef);
  dst_insn_vec_get_usedef(insn_vec, &use, &def, &memuse, &memdef);
  bool ret = memuse.memaccess.size() > 0 || memdef.memaccess.size() > 0;
  memset_free(&memuse);
  memset_free(&memdef);
  return ret;
}

static list<vector<dst_insn_t>>
get_memloc_variants(vector<dst_insn_t> const &insn_vec, dst_strict_canonicalization_state_t *scanon_state,
    fixed_reg_mapping_t const &fixed_reg_mapping)
{
  list<vector<dst_insn_t>> ret;
  vector<dst_insn_t> cp = insn_vec;
  symbol_id_t symbol_id = 0;
  dst_insn_t *dst_iseq = new dst_insn_t[cp.size()];
  ASSERT(dst_iseq);
  long i = 0;
  for (auto &insn : cp) {
    dst_insn_rename_memory_operand_to_symbol(insn, symbol_id);
    dst_insn_copy(&dst_iseq[i], &insn);
    symbol_id++;
    i++;
  }
  ASSERT(i == cp.size());
  dst_insn_t *insn_var[1];
  insn_var[0] = new dst_insn_t[cp.size()];
  ASSERT(insn_var[0]);
  long insn_var_len[1];
  regmap_t st_regmap[1];
  imm_vt_map_t st_imm_map[1];
  //cout << __func__ << " " << __LINE__ << ": before strict canonicalize: dst_iseq =\n" << dst_iseq_to_string(dst_iseq, cp.size(), as1, sizeof as1) << endl;
  int n = dst_iseq_strict_canonicalize(scanon_state, dst_iseq, cp.size(), insn_var,
     insn_var_len, st_regmap, st_imm_map, fixed_reg_mapping, nullptr, 1, true);
  ASSERT(n == 1);
  //cout << __func__ << " " << __LINE__ << ": after strict canonicalize: dst_iseq =\n" << dst_iseq_to_string(insn_var[0], insn_var_len[0], as1, sizeof as1) << endl;
  cp = dst_insn_vec_from_iseq(insn_var[0], insn_var_len[0]);
  ret.push_back(cp);
  delete [] insn_var[0];
  delete [] dst_iseq;
  return ret;
}

static size_t
read_dst_insns_helper(vector<dst_insn_t> *buf, size_t size,
    function<bool (vector<dst_insn_t> const &)> filter_fn,
    dst_insn_t const *dst_iseq, long dst_iseq_len,
    dst_strict_canonicalization_state_t *scanon_state,
    fixed_reg_mapping_t const &fixed_reg_mapping)
{
  vector<dst_insn_t> *ptr, *end;
  ptr = buf;
  end = buf + size;
  char const *dst_fbtest_filename = ABUILD_DIR "/dst_fbtest";

  //cout << __func__ << " " << __LINE__ << ": dst_iseq = " << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;

  dst_insn_list_elem_t *ls;
  ls = dst_read_insn_list_from_file(dst_fbtest_filename, NULL, NULL);

  for (cmn_insn_list_elem_t *cur = ls; cur; cur = cur->next) {
    //dst_insn_copy(ptr, &cur->insn[0]);
    //cout << __func__ << " " << __LINE__ << ": dst_iseq = " << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
    vector<dst_insn_t> insn_vec;
    //ptr->clear();
    insn_vec.push_back(cur->insn[0]);
    //cout << __func__ << " " << __LINE__ << ": considering insn_vec " << dst_insn_vec_to_string(insn_vec, as1, sizeof as1) << "\n";
    if (dst_iseq && !dst_insn_vec_is_related_to_dst_iseq(insn_vec, dst_iseq, dst_iseq_len)) {
      //cout << __func__ << " " << __LINE__ << ": ignoring insn_vec " << dst_insn_vec_to_string(insn_vec, as1, sizeof as1) << " because not related\n";
      continue;
    }
    if (!filter_fn(insn_vec)) {
      //cout << __func__ << " " << __LINE__ << ": ignoring insn_vec " << dst_insn_vec_to_string(insn_vec, as1, sizeof as1) << " because filter_fn returned false\n";
      continue;
    }
    *ptr = insn_vec;
    ptr++;
    ASSERT(ptr < end);
    if (dst_insn_vec_accesses_memory(insn_vec)) {
      list<vector<dst_insn_t>> memloc_variants = get_memloc_variants(insn_vec, scanon_state, fixed_reg_mapping);
      for (const auto &memloc_variant : memloc_variants) {
        *ptr = memloc_variant;
        ptr++;
        ASSERT(ptr < end);
      }
    }
    ASSERT(ptr < end);
  }
  return ptr - buf;
}

size_t
read_all_dst_insns(vector<dst_insn_t> *buf, size_t size, function<bool (vector<dst_insn_t>)> filter_fn, dst_strict_canonicalization_state_t *scanon_state, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  return read_dst_insns_helper(buf, size, filter_fn, NULL, 0, scanon_state, fixed_reg_mapping);
}

size_t
read_related_dst_insns(std::vector<dst_insn_t> *buf, size_t size, dst_insn_t const *dst_iseq, long dst_iseq_len, function<bool (vector<dst_insn_t>)> filter_fn, dst_strict_canonicalization_state_t *scanon_state, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  //cout << __func__ << " " << __LINE__ << ": dst_iseq = " << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
  return read_dst_insns_helper(buf, size, filter_fn, dst_iseq, dst_iseq_len, scanon_state, fixed_reg_mapping);
}

char *
dst_insn_vec_to_string(std::vector<dst_insn_t> const &insn_vec, char *buf, size_t size)
{
  size_t iseq_len = insn_vec.size();
  dst_insn_t *iseq = new dst_insn_t[iseq_len];
  dst_insn_vec_copy_to_arr(iseq, insn_vec);
  dst_iseq_to_string(iseq, iseq_len, buf, size);
  delete [] iseq;
  return buf;
}

void
dst_insn_vec_get_usedef_regs(std::vector<dst_insn_t> const &insn_vec, regset_t *use, regset_t *def)
{
  memset_t memuse, memdef;
  memset_init(&memuse);
  memset_init(&memdef);
  dst_insn_vec_get_usedef(insn_vec, use, def, &memuse, &memdef);
  memset_free(&memuse);
  memset_free(&memdef);
}

void
dst_insn_vec_get_usedef(std::vector<dst_insn_t> const &insn_vec, regset_t *use, regset_t *def, memset_t *memuse, memset_t *memdef)
{
  size_t iseq_len = insn_vec.size();
  dst_insn_t *iseq = new dst_insn_t[iseq_len];
  dst_insn_vec_copy_to_arr(iseq, insn_vec);
  dst_iseq_get_usedef(iseq, iseq_len, use, def, memuse, memdef);
  delete [] iseq;
}

void
dst_insn_vec_copy_to_arr(dst_insn_t *arr, std::vector<dst_insn_t> const &vec)
{
  size_t i = 0;
  for (const auto &di : vec) {
    dst_insn_copy(&arr[i], &di);
    i++;
  }
}

bool
dst_insn_vec_is_nop(vector<dst_insn_t> const &insn_vec)
{
  size_t dst_iseq_len = insn_vec.size();
  dst_insn_t *dst_iseq = new dst_insn_t[dst_iseq_len];
  dst_insn_vec_copy_to_arr(dst_iseq, insn_vec);
  bool ret = dst_iseq_is_nop(dst_iseq, dst_iseq_len);
  delete [] dst_iseq;
  return ret;
}



bool
dst_insn_vecs_equal(vector<dst_insn_t> const &a, vector<dst_insn_t> const &b)
{
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); i++) {
    if (!dst_insns_equal(&a.at(i), &b.at(i))) {
      return false;
    }
  }
  return true;
}

bool
dst_iseq_supports_boolean_test(dst_insn_t const *dst_iseq, long dst_iseq_len)
{
  for (long i = 0; i < dst_iseq_len; i++) {
    if (!dst_insn_supports_boolean_test(&dst_iseq[i])) {
      return false;
    }
  }
  return true;
}

bool
dst_iseq_is_nop(dst_insn_t const *dst_iseq, long dst_iseq_len)
{
  for (size_t i = 0; i < dst_iseq_len - 1; i++) {
    if (!dst_insn_is_nop(&dst_iseq[i])) {
      return false;
    }
  }
  dst_insn_t const *last_insn = &dst_iseq[dst_iseq_len - 1];
  if (dst_insn_is_unconditional_direct_branch(last_insn)) {
    valtag_t const *vt = dst_insn_get_pcrel_operand(last_insn);
    ASSERT(vt);
    if (vt->tag == tag_var && vt->val == 0) {
      return true;
    }
  }
  return false;
}

bool
dst_iseqs_equal(dst_insn_t const *a, dst_insn_t const *b, long iseq_len)
{
  for (size_t i = 0; i < iseq_len; i++) {
    if (!dst_insns_equal(&a[i], &b[i])) {
      return false;
    }
  }
  return true;
}

static void
dst_insn_vec_patch_nextpc_id_to_jump_to_fallthrough(vector<dst_insn_t> &iseq, nextpc_id_t nextpc_id)
{
  size_t iseq_len = iseq.size();
  size_t i = 0;
  for (dst_insn_t &insn : iseq) {
    i++;
    src_tag_t pcrel_tag;
    unsigned long pcrel_val;
    if (!dst_insn_get_pcrel_value(&insn, &pcrel_val, &pcrel_tag)) {
      continue;
    }
    if (pcrel_tag == tag_var && pcrel_val == nextpc_id) {
      unsigned long fallthrough_pcrel = iseq_len - i;
      dst_insn_set_pcrel_value(&insn, fallthrough_pcrel, tag_const);
    }
  }
  //use two instructions that together act as nop; do not use a single insn nop as that has special handling in our code.
  //dst_insn_t di;
  //size_t ret = dst_insn_put_xchg_reg_with_reg_var(0, 1, &di, 1);
  //ASSERT(ret == 1);
  //iseq.push_back(di);
  //size_t ret = dst_insn_put_xchg_reg_with_reg_var(0, 1, &di, 1);
  //ASSERT(ret == 1);
  //iseq.push_back(di);
  dst_insn_t nop;
  size_t ret = dst_insn_put_nop(&nop, 1);
  ASSERT(ret == 1);
  iseq.push_back(nop);
}

static nextpc_id_t
dst_insn_vec_get_last_nextpc_num_not_indir(vector<dst_insn_t> const &iseq)
{
  nextpc_id_t nextpc_id = -1;
  for (const auto &insn : iseq) {
    unsigned long pcrel_val;
    src_tag_t pcrel_tag;
    if (!dst_insn_get_pcrel_value(&insn, &pcrel_val, &pcrel_tag)) {
      continue;
    }
    if (pcrel_tag == tag_var) {
      if ((int)pcrel_val > nextpc_id) {
        nextpc_id = pcrel_val;
      }
    }
  }
  return nextpc_id;
}

void
dst_insn_vec_convert_last_nextpc_to_fallthrough(vector<dst_insn_t> &iseq)
{
  nextpc_id_t nextpc_id = dst_insn_vec_get_last_nextpc_num_not_indir(iseq);
  //cout << __func__ << " " << __LINE__ << ": before, iseq = " << dst_insn_vec_to_string(iseq, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": nextpc_id = " << nextpc_id << endl;
  if (nextpc_id == -1) {
    return;
  }
  dst_insn_vec_patch_nextpc_id_to_jump_to_fallthrough(iseq, nextpc_id);
  //cout << __func__ << " " << __LINE__ << ": after, iseq = " << dst_insn_vec_to_string(iseq, as1, sizeof as1) << endl;
}

void
dst_iseq_canonicalize_vregs(dst_insn_t *dst_iseq, long dst_iseq_len, regmap_t *dst_regmap, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  autostop_timer func_timer(__func__);
  dst_insn_t *dst_iseq_canon[1];
  dst_iseq_canon[0] = new dst_insn_t[dst_iseq_len];
  long dst_iseq_canon_len;

  //cout << __func__ << " " << __LINE__ << ": before, dst_iseq = " << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
  dst_iseq_rename_vregs_to_cregs(dst_iseq, dst_iseq_len);
  dst_strict_canonicalization_state_t *dst_canon_state = dst_strict_canonicalization_state_init();
  long num_canon_iseqs = dst_iseq_strict_canonicalize(dst_canon_state,
      dst_iseq, dst_iseq_len, dst_iseq_canon, &dst_iseq_canon_len, dst_regmap, NULL, fixed_reg_mapping, nullptr, 1, false);
  ASSERT(num_canon_iseqs == 1);
  ASSERT(dst_iseq_canon_len == dst_iseq_len);
  dst_strict_canonicalization_state_free(dst_canon_state);
  dst_iseq_copy(dst_iseq, dst_iseq_canon[0], dst_iseq_len);
  //cout << __func__ << " " << __LINE__ << ": after, dst_iseq = " << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
  delete [] dst_iseq_canon[0];
}

void
dst_insn_vec_copy(std::vector<dst_insn_t> &dst, std::vector<dst_insn_t> const &src)
{
  dst.clear();
  for (const auto &src_insn : src) {
    dst.push_back(src_insn);
  }
}

vector<dst_insn_t>
dst_insn_vec_from_iseq(dst_insn_t const *dst_iseq, long dst_iseq_len)
{
  vector<dst_insn_t> ret;
  for (long i = 0; i < dst_iseq_len; i++) {
    ret.push_back(dst_iseq[i]);
  }
  return ret;
}

long
dst_iseq_get_num_nextpcs(dst_insn_t const *iseq, long iseq_len)
{
  dst_insn_t *iseq_copy = new dst_insn_t[iseq_len];
  dst_iseq_copy(iseq_copy, iseq, iseq_len);
  nextpc_t nextpcs[iseq_len];
  dst_ulong pcs[iseq_len];
  long num_nextpcs;

  for (long i = 0; i < iseq_len; i++) {
    pcs[i] = i;
  }
  //cout << __func__ << " " << __LINE__ << ": before convert_pcs_to_inums, dst_iseq =\n" << dst_iseq_to_string(iseq_copy, iseq_len, as1, sizeof as1) << endl;
  dst_iseq_convert_pcs_to_inums(iseq_copy, pcs, iseq_len, nextpcs, &num_nextpcs);
  //cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(iseq_copy, iseq_len, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": num_nextpcs = " << num_nextpcs << endl;
  ASSERT(num_nextpcs >= 0);
  delete [] iseq_copy;
  return num_nextpcs;
}

void
dst_iseq_rename_fixed_regs(dst_insn_t *dst_iseq, long dst_iseq_len, fixed_reg_mapping_t const &from, fixed_reg_mapping_t const &to)
{
  autostop_timer func_timer(__func__);
  regmap_t regmap;
  for (const auto &g : from.get_map()) {
    exreg_group_id_t group = g.first;
    for (const auto &m : g.second) {
      exreg_id_t machine_regname = m.first;
      exreg_id_t old_reg = m.second;
      exreg_id_t new_reg = to.get_mapping(group, machine_regname);
      ASSERT(old_reg >= 0 && old_reg < DST_NUM_EXREGS(group));
      ASSERT(new_reg >= 0 && new_reg < DST_NUM_EXREGS(group));
      regmap.regmap_extable[group].insert(make_pair(old_reg, new_reg));
    }
  }
  dst_iseq_rename_vregs_using_regmap(dst_iseq, dst_iseq_len, regmap);
}

void dst_iseq_rename_vregs_using_regmap(dst_insn_t *dst_iseq, long dst_iseq_len, regmap_t const &regmap)
{
  for (long i = 0; i < dst_iseq_len; i++) {
    dst_insn_rename_vregs_using_regmap(&dst_iseq[i], regmap);
  }
}

long long
dst_iseq_compute_cost(dst_insn_t const *iseq, long iseq_len, dst_costfn_t costfn)
{
  long long execution_cost = dst_iseq_compute_execution_cost(iseq, iseq_len, costfn);
  long num_touch = dst_iseq_count_touched_regs_as_cost(iseq, iseq_len);
  return execution_cost * 100 + num_touch;
}

void
dst_iseq_rename_tag_src_insn_pcs_to_tag_vars(dst_insn_t *iseq, long iseq_len, nextpc_t const *nextpcs, uint8_t const * const *varvals, size_t num_varvals)
{
  for (long i = 0; i < iseq_len; i++) {
    dst_insn_rename_tag_src_insn_pcs_to_tag_vars(&iseq[i], nextpcs, varvals, num_varvals);
  }
}

void
dst_insn_pcs_to_stream(ostream& ofs, std::vector<dst_ulong> const& dst_insn_pcs)
{
  for (size_t i = 0; i < dst_insn_pcs.size(); i++) {
    ofs << i << " : 0x" << hex << dst_insn_pcs.at(i) << dec << endl;
  }
  ofs << "=dst_insn_pcs done\n";
}

vector<dst_ulong>
dst_insn_pcs_from_stream(istream& is)
{
  string line;
  bool end;
  size_t linenum = 0;
  vector<dst_ulong> ret;

  while (1) {
    end = !getline(is, line);
    ASSERT(!end);
    if (line.at(0) == '=') {
      break;
    }
    istringstream iss(line);
    size_t idx;
    dst_ulong pc;
    string colon;
    iss >> idx;
    if (idx != linenum) {
      cout << __func__ << " " << __LINE__ << ": idx = " << idx << ", linenum = " << linenum << endl;
    }
    ASSERT(idx == linenum);
    iss >> colon;
    ASSERT(colon == ":");
    iss >> hex >> pc >> dec;
    ret.push_back(pc);
    linenum++;
  }
  ASSERT(line == "=dst_insn_pcs done");
  return ret;
}

vector<dst_insn_t>
dst_iseq_from_stream(istream& is)
{
  string line;
  bool end;
  size_t linenum = 0;
  string str;

  while (1) {
    end = !getline(is, line);
    ASSERT(!end);
    if (line.at(0) == '=') {
      ASSERT(line == "=dst_iseq done");
      break;
    }
    str += line + "\n";
    linenum++;
  }
  ASSERT(line == "=dst_iseq done");
  size_t max_alloc = (linenum + 1) * 2;
  dst_insn_t *dst_iseq = new dst_insn_t[max_alloc];
  ASSERT(dst_iseq);
  //cout << __func__ << " " << __LINE__ << ": str = " << str << endl;
  long dst_iseq_len = dst_iseq_from_string(nullptr, dst_iseq, max_alloc, str.c_str(), I386_AS_MODE_32);

  vector<dst_insn_t> ret;
  for (long i = 0; i < dst_iseq_len; i++) {
    ret.push_back(dst_iseq[i]);
  }
  //cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl;
  return ret;

}

fixed_reg_mapping_t
fixed_reg_mapping_t::default_dst_fixed_reg_mapping_for_fallback_translations()
{
  fixed_reg_mapping_t fixed_reg_mapping;
  for (int i = 0; i < DST_NUM_EXREG_GROUPS; i++) {
    for (int j = 0; j < DST_NUM_EXREGS(i); j++) {
      if (regset_belongs_ex(&dst_reserved_regs, i, j)) {
        fixed_reg_mapping.add(i, j, j);
      }
    }
  }
  return fixed_reg_mapping;
}

fixed_reg_mapping_t const &
fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity()
{
  static fixed_reg_mapping_t ret;
  static bool init = false;
  if (!init) {
    for (const auto &g : dst_reserved_regs.excregs) {
      for (const auto &r : g.second) {
        ret.add(g.first, r.first.reg_identifier_get_id(), r.first.reg_identifier_get_id());
      }
    }
    //cout << __func__ << " " << __LINE__ << ": returning\n" << ret.fixed_reg_mapping_to_string() << endl;
    init = true;
  }
  return ret;
}

fixed_reg_mapping_t const &
fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end()
{
  static fixed_reg_mapping_t ret;
  static bool init = false;
  if (!init) {
    for (const auto &g : dst_reserved_regs.excregs) {
      int n = DST_NUM_EXREGS(g.first) - 1;
      for (const auto &r : g.second) {
        ret.add(g.first, r.first.reg_identifier_get_id(), n);
        n--;
      }
    }
    //cout << __func__ << " " << __LINE__ << ": returning\n" << ret.fixed_reg_mapping_to_string() << endl;
    init = true;
  }
  return ret;
}

fixed_reg_mapping_t const &
fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_at_end_var_to_var()
{
  static fixed_reg_mapping_t ret;
  static bool init = false;
  if (!init) {
    for (const auto &g : dst_reserved_regs.excregs) {
      int n = DST_NUM_EXREGS(g.first) - 1;
      for (const auto &r : g.second) {
        //this is a var->var identity map
        ret.add(g.first, n, n);
        n--;
      }
    }
    //cout << __func__ << " " << __LINE__ << ": returning\n" << ret.fixed_reg_mapping_to_string() << endl;
    init = true;
  }
  return ret;
}

bool
get_translated_addr_using_tcodes(uint32_t *new_val, struct chash const *tcodes, src_ulong val)
{
  uint8_t const *txed_val;

  txed_val = tcodes_query(tcodes, val);
  DYN_DBG(stat_trans, "val = 0x%lx, txed_val = %p\n", (long)val, txed_val);
  if (!txed_val) {
    return false;
  }
  //*new_val = MYTEXT_START + 0x400 + (txed_val - START_VMA);
  *new_val = TX_TEXT_START_ADDR + 0x400 + (txed_val - START_VMA);
  return true;
}



