#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include "support/debug.h"
#include "support/c_utils.h"
#include "superopt/src_env.h"
#include "valtag/regset.h"
#include "exec/state.h"
#include "superopt/typestate.h"
#include "rewrite/transmap.h"
#include "i386/execute.h"
#include "ppc/execute.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "ppc/insn.h"
//#include "imm_map.h"
#include "support/src-defs.h"
#include "equiv/equiv.h"
#include "rewrite/translation_instance.h"
#include "valtag/regcons.h"
#include "valtag/regmap.h"
#include "rewrite/peephole.h"
#include "valtag/regcount.h"
#include "rewrite/src-insn.h"
#include "rewrite/dst-insn.h"
#include "rewrite/static_translate.h"
#include "sym_exec/sym_exec.h"
#include "rewrite/transmap_set.h"
#include "valtag/memslot_map.h"
#include "valtag/memslot_set.h"
//#include "superopt/rand_states.h"
#include "valtag/nextpc.h"

static char as1[409600];
//void *membase = NULL;
imm_vt_map_t *src_env_imm_map = NULL;

void
state_init(state_t *state)
{
  //memset(state, 0xff, sizeof(state_t));
  state->memory = (uint32_t *)malloc32(MEMORY_SIZE + MEMORY_PADSIZE);
}

static void
state_rename(state_t *out, regmap_t const *regmap)
{
  state_t in_copy, *in;
  long i, j;

  in = &in_copy;
  memcpy(in, out, sizeof(state_t));

  for (i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < MAX_NUM_EXREGS(i); j++) {
      if (   regmap_get_elem(regmap, i, reg_identifier_t(j)).reg_identifier_get_id() >= 0
          && regmap_get_elem(regmap, i, reg_identifier_t(j)).reg_identifier_get_id() < MAX_NUM_EXREGS(i)) {
        out->exregs[i][regmap_get_elem(regmap, i, reg_identifier_t(j)).reg_identifier_get_id()] = in->exregs[i][j];
      }
    }
  }
}

static void
state_inv_rename(state_t *out, regmap_t const *regmap)
{
  regmap_t inv_regmap;

  regmap_invert(&inv_regmap, regmap);
  state_rename(out, &inv_regmap);
}

void
state_rename_using_transmap(state_t *state, transmap_t const *tmap)
{
  int i, j, group;
  state_t copy;

  state_init(&copy);
  state_copy(&copy, state);

  for (i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < MAX_NUM_EXREGS(i); j++) {
      for (const auto &l : transmap_get_elem(tmap, i, j)->get_locs()) {
        if (   l.get_loc() >= TMAP_LOC_EXREG(0)
            && l.get_loc() < TMAP_LOC_EXREG(MAX_NUM_EXREG_GROUPS)) {
          group = l.get_loc() - TMAP_LOC_EXREG(0);
          reg_identifier_t const &reg = l.get_reg_id();
          ASSERT(reg.reg_identifier_is_valid());
          ASSERT(!reg.reg_identifier_is_unassigned());
          //ASSERT(reg >= 0 && reg < MAX_NUM_EXREGS(group));
          state->exregs[group][reg.reg_identifier_get_id()] = copy.exregs[i][j];
        } else {
          ASSERT(l.get_loc() == TMAP_LOC_SYMBOL);
          dst_ulong symbol_id = l.get_regnum();
          ASSERT(symbol_id >= 0 && symbol_id < MAX_NUM_MEMLOCS);
          state->memlocs[symbol_id] = copy.exregs[i][j];
        }
      }
    }
  }
  state_free(&copy);
}

void
state_rename_using_transmap_inverse(struct state_t *state, struct transmap_t const *tmap)
{
  autostop_timer func_timer(__func__);
  int i, j, group;
  state_t copy;

  state_init(&copy);
  state_copy(&copy, state);

  for (i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < MAX_NUM_EXREGS(i); j++) {
      for (const auto &l : transmap_get_elem(tmap, i, j)->get_locs()) {
        if (   l.get_loc() >= TMAP_LOC_EXREG(0)
            && l.get_loc() < TMAP_LOC_EXREG(MAX_NUM_EXREG_GROUPS)) {
          group = l.get_loc() - TMAP_LOC_EXREG(0);
          reg_identifier_t const &reg = l.get_reg_id();
          ASSERT(reg.reg_identifier_is_valid());
          ASSERT(!reg.reg_identifier_is_unassigned());
          //ASSERT(reg >= 0 && reg < MAX_NUM_EXREGS(group));
          state->exregs[i][j] = copy.exregs[group][reg.reg_identifier_get_id()];
        } else {
          ASSERT(l.get_loc() == TMAP_LOC_SYMBOL);
          dst_ulong symbol_id = l.get_regnum();
          ASSERT(symbol_id >= 0 && symbol_id < MAX_NUM_MEMLOCS);
          state->exregs[i][j] = copy.memlocs[symbol_id];
        }
      }
    }
  }
  state_free(&copy);
}

void
state_copy_to_env(state_t *state)
{
  state_t *cpustate;
  int i, j;

  cpustate = (state_t *)SRC_ENV_ADDR;
  /*for (i = 0; i < SRC_NUM_REGS; i++) {
    *((src_ulong *)((uint8_t *)cpustate + SRC_ENV_OFF(i))) = cpustate->regs[i];
  }*/
  for (i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < MAX_NUM_EXREGS(i); j++) {
      //*((src_ulong *)((uint8_t *)cpustate + SRC_ENV_EXOFF(i, j))) =
      //    state->exregs[i][j].state_reg_to_int64();
      cpustate->exregs[i][j] = state->exregs[i][j];
    }
  }
  for (i = 0; i < MAX_NUM_MEMLOCS; i++) {
    cpustate->memlocs[i] = state->memlocs[i];
  }
  cpustate->last_executed_insn = state->last_executed_insn;
}

void
state_copy_from_env(state_t *state)
{
  state_t *cpustate;
  int i, j;

  cpustate = (state_t *)SRC_ENV_ADDR;
  /*for (i = 0; i < SRC_NUM_REGS; i++) {
    cpustate->regs[i] = *((src_ulong *)((uint8_t *)cpustate + SRC_ENV_OFF(i)));
  }*/
  for (i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < MAX_NUM_EXREGS(i); j++) {
      //state->exregs[i][j].state_reg_from_int64(
      //    *((src_ulong *)((uint8_t *)cpustate + SRC_ENV_EXOFF(i, j))));
      state->exregs[i][j] = cpustate->exregs[i][j];
    }
  }
  for (i = 0; i < MAX_NUM_MEMLOCS; i++) {
    state->memlocs[i] = cpustate->memlocs[i];
  }
  state->last_executed_insn = cpustate->last_executed_insn;
}

/*void
i386_execute_code_on_state(uint8_t const *code, state_t *state)
{
  int i;

  membase = (uint32_t *)(state->memory);
  state_copy_to_env(state);
  state->nextpc_num = ((long(*)())(code))();
  state_copy_from_env(state);

  for (i = 0; i < (MEMORY_PADSIZE >> 2); i++) {
    state->memory[i] = state->memory[i] ^ state->memory[(MEMORY_SIZE >> 2) + i];
  }
  //state->memory[1] = state->memory[1] ^ state->memory[(MEMORY_SIZE>>2) + 1];
}*/

static bool
dst_iseq_window_rename_nextpcs_to_src_pcrels(dst_insn_t *dst_iseq,
    long dst_iseq_len, long start, long const *nextpc2src_pcrel_vals,
    src_tag_t const *nextpc2src_pcrel_tags, long num_nextpcs)
{
  long i, pcrel_val;
  valtag_t *pcrel;

  for (i = 0; i < dst_iseq_len; i++) {
    if (pcrel = dst_insn_get_pcrel_operand(&dst_iseq[i])) {
      if (pcrel->tag == tag_const) {
        if (pcrel->val == -1) {
          pcrel->val = start;
          pcrel->tag = tag_abs_inum;
        }
        continue;
      }
      if (pcrel->tag == tag_reloc_string) {
        return false;
      }
      ASSERT(pcrel->tag == tag_var);
      ASSERT(   pcrel->val >= 0
             && pcrel->val < num_nextpcs);
 
      pcrel_val = pcrel->val;
      pcrel->val = nextpc2src_pcrel_vals[pcrel_val];
      pcrel->tag = nextpc2src_pcrel_tags[pcrel_val];
      if (pcrel->tag == tag_const) {
        pcrel->tag = tag_abs_inum;
      }
      DBG3(EQUIV, "Renamed pcrel_val %ld tag %ld to %ld tag %ld\n",
          (long)pcrel_val, (long)tag_var, (long)pcrel->val,
          (long)pcrel->tag);
    }
  }
  return true;
}

static long
gen_back_and_forth_transmap_translation_insns(dst_insn_t *buf, long size,
    transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping)
{
  dst_insn_t *ptr, *end;
  long i;

  ASSERT(in_tmap);
  ptr = buf;
  end = buf + size;

#if ARCH_SRC == ARCH_PPC
  ptr += transmap_translation_insns(transmap_default(), in_tmap, NULL, ptr, end - ptr, -1, dst_fixed_reg_mapping, I386_AS_MODE_64);
  ptr += transmap_translation_insns(in_tmap, transmap_default(), NULL, ptr, end - ptr, -1, dst_fixed_reg_mapping, I386_AS_MODE_64);

  for (i = 0; i < num_live_out; i++) {
    ptr += transmap_translation_insns(transmap_default(), &out_tmaps[i], NULL,
        ptr, end - ptr, -1, dst_fixed_reg_mapping, I386_AS_MODE_64);
    ptr += transmap_translation_insns(&out_tmaps[i], transmap_default(), NULL,
        ptr, end - ptr, -1, dst_fixed_reg_mapping, I386_AS_MODE_64);
  }
#endif
  return ptr - buf;
}

long
src_iseq_gen_prefix_insns(dst_insn_t *buf, long size,
    transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping)
{
  /* back and forth transmap translation insns ensures that all insignificant
     bits are removed (e.g., for flag mappings). */
  return gen_back_and_forth_transmap_translation_insns(buf, size, in_tmap, out_tmaps,
      num_live_out, dst_fixed_reg_mapping);
}

static transmap_t
gen_transmap_env_for_regs(regset_t const &touch)
{
  transmap_t tmap_env;
  for (const auto &g : touch.excregs) {
    for (const auto &r : g.second) {
      exreg_id_t rid = r.first.reg_identifier_get_id();
      tmap_env.extable[g.first][rid].set_to_symbol_loc(TMAP_LOC_SYMBOL, SRC_ENV_ADDR + SRC_ENV_EXOFF(g.first, rid));
    }
  }
  return tmap_env;
}

/* the in_tmap and out_tmaps are needed to remove insignificant bits from src state
   by converting back and forth between transmap_default and in_tmap/out_tmap,
   before executing the code */
/*static bool
src_gen_exec_code(src_insn_t const *src_iseq, long src_iseq_len,
    imm_map_t const *imm_map, uint8_t *src_code, size_t *src_codesize,
    src_ulong const *nextpcs, //src_ulong const *insn_pcs,
    uint8_t const * const *next_tcodes,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    long num_live_out)
{
#define MAX_PREFIX_CODE 4096
  long max_alloc = (src_iseq_len + 10) * 10;
  transmap_t out_tmaps_default[num_live_out];
  src_insn_t src_iseq_renamed[src_iseq_len];
  //size_t prefix_code_len[src_iseq_len];
  //uint8_t *prefix_code[src_iseq_len];
  i386_insn_t i386_iseq[max_alloc];
  long i, i386_iseq_len;
  uint8_t *ptr, *end;

  for (i = 0; i < src_iseq_len; i++) {
    src_insn_copy(&src_iseq_renamed[i], &src_iseq[i]);
    src_insn_rename_constants(&src_iseq_renamed[i], imm_map, NULL, 0, NULL);
  }

  DBG3(EQUIV, "Translating using ti_fallback, src_iseq:\n%s\n",
      src_iseq_to_string(src_iseq_renamed, src_iseq_len, as1, sizeof as1));
  i386_iseq_len = peep_substitute_using_ti_fallback(src_iseq_renamed,
        src_iseq_len, nextpcs, //insn_pcs,
        num_live_out, i386_iseq, max_alloc,
        in_tmap, out_tmaps);

  if (   i386_iseq_len == -1
      || !i386_iseq_supports_execution_test(i386_iseq, i386_iseq_len)) {
    return false;
  }

  for (i = 0; i < num_live_out; i++) {
    transmap_copy(&out_tmaps_default[i], transmap_fallback());
  }
  DBG3(EQUIV, "Generating sandboxed code for i386_iseq:\n%s\n",
      i386_iseq_to_string(i386_iseq, i386_iseq_len, as1, sizeof as1));

  ptr = src_code;
  end = src_code + *src_codesize;

  ptr += i386_iseq_gen_exec_code(i386_iseq, i386_iseq_len, NULL, ptr,
      end - ptr, nextpcs, next_tcodes,
      transmap_default(), out_tmaps_default,
      num_live_out);
  ASSERT(ptr < end);
  *src_codesize = ptr - src_code;
  DBG3(EQUIV, "src_code: (size %zd)\n%s\n", *src_codesize,
      hex_string(src_code, *src_codesize, as1, sizeof as1));
  return true;
}*/

void
states_init(state_t *states, int num_states)
{
  int i;
  for (i = 0; i < num_states; i++) {
    state_init(&states[i]);
  }
}

void
state_free(state_t *state)
{
  free32(state->memory);
}

void
states_free(state_t *states, int num_states)
{
  int i;
  for (i = 0; i < num_states; i++) {
    state_free(&states[i]);
    //free32(states[i].memory);
  }
}

void
state_copy(state_t *dst, state_t const *src)
{
  autostop_timer func_timer(__func__);
  uint32_t *dst_memory = dst->memory;
  memcpy(dst, src, sizeof(state_t));
  dst->memory = dst_memory;
  memcpy(dst->memory, src->memory, MEMORY_SIZE + MEMORY_PADSIZE);
}

char *
state_to_string(state_t const *state, char *buf, size_t size)
{
  char *ptr = buf, *end = buf + size;
  int i, j;
  /*for (i = 0; i < SRC_NUM_REGS; i++) {
    ptr += snprintf(ptr, end - ptr, "reg%d : 0x%x\n", i, state->regs[i]);
  }*/
  for (i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < MAX_NUM_EXREGS(i); j++) {
      ptr += snprintf(ptr, end - ptr, "exreg%d.%d : 0x%llx\n", i, j, (unsigned long long)state->exregs[i][j].state_reg_to_int64());
    }
  }
  for (i = 0; i < MAX_NUM_MEMLOCS; i++) {
    ptr += snprintf(ptr, end - ptr, "memlocs[%d] : 0x%llx\n", i, (unsigned long long)state->memlocs[i].state_reg_to_int64());
  }
  ptr += snprintf(ptr, end - ptr, "stack:");
  for (i = 0; i < (STATE_MAX_STACK_SIZE >> 2); i++) {
    ptr += snprintf(ptr, end - ptr, "%c%08x", (i % 16)?' ':'\n', state->stack[i]);
  }
  ptr += snprintf(ptr, end - ptr, "nextpc_num: %d\n", state->nextpc_num);

  ptr += snprintf(ptr, end - ptr, "memory:");
  for (i = 0; i < ((MEMORY_SIZE + MEMORY_PADSIZE) >> 2); i++) {
    ptr += snprintf(ptr, end - ptr, "%c%08x", (i % 16)?' ':'\n', state->memory[i]);
  }
  static char tbuf[4096000];
  ptr += snprintf(ptr, end - ptr, "\nimm_vt_map:\n%s",
      imm_vt_map_to_string(&state->imm_vt_map, tbuf, sizeof tbuf));
  ptr += snprintf(ptr, end - ptr, "\n");
  assert(ptr <= end);
  //return (end-ptr);
  return buf;
}

bool
states_equivalent(state_t const *state1, state_t const *state2,
    regset_t const *live_regs_arr, memslot_set_t const *live_memslots_arr,
    long num_live_outs, bool mem_live_out)
{
  regset_t const *live_regs;
  memslot_set_t const *live_memslots;
  int i, j;

  if (state1->nextpc_num != state2->nextpc_num) {
    DBG(EQUIV, "mismatch on branch_taken: %d <-> %d\n", state1->nextpc_num,
        state2->nextpc_num);
    return false;
  }

  ASSERT(state1->nextpc_num >= 0 && state1->nextpc_num < num_live_outs);
  live_regs = &live_regs_arr[state1->nextpc_num];
  live_memslots = &live_memslots_arr[state1->nextpc_num];

  /*for (i = 0; i < SRC_NUM_GPRS; i++) {
    if (regset_belongs_vreg(live_regs, i)) {
      if (state1->regs[i] != state2->regs[i]) {
        DBG(EQUIV, "mismatch on regs[%d]: %x <-> %x\n", i, state1->regs[i],
            state2->regs[i]);
        return false;
      }
    }
  }*/
  for (i = 0; i < SRC_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < SRC_NUM_EXREGS(i); j++) {
      if ((state1->exregs[i][j].state_reg_to_int64() & regset_get_elem(live_regs, i, j)) != (state2->exregs[i][j].state_reg_to_int64() & regset_get_elem(live_regs, i, j))) {
        DBG(EQUIV, "mismatch on exregs[%d][%d]: %lx <-> %lx\n", i, j,
            state1->exregs[i][j].state_reg_to_int64(), state2->exregs[i][j].state_reg_to_int64());
        return false;
      }
    }
  }
  for (i = 0; i < MAX_NUM_MEMLOCS; i++) {
    if ((state1->memlocs[i].state_reg_to_int64() & live_memslots->memslot_set_get_elem(i)) != (state2->memlocs[i].state_reg_to_int64() & live_memslots->memslot_set_get_elem(i))) {
      return false;
    }
  }
#if 0
  for (i = SRC_NUM_GPRS; i < SRC_NUM_REGS; i++) {
    if (regset_belongs(live_regs, i)) {
      if (i >= PPC_REG_CRFN(0) && i < PPC_REG_CRFN(8)) {
        // XXX: ignore the OV bit for now.
        if ((state1->regs[i] & 0xe) != (state2->regs[i] & 0xe)) {
          DBG(EQUIV, "mismatch on crf[%d]: %x <-> %x\n", i - PPC_REG_CRFN(0),
              state1->regs[i], state2->regs[i]);
          return false;
        }
      } else {
        if (state1->regs[i] != state2->regs[i]) {
          DBG(EQUIV, "mismatch on regs[%d]: %x <-> %x\n", i, state1->regs[i],
              state2->regs[i]);
          return false;
        }
      }
    }
  }
  /*
  if (regset_belongs(live_regs, PPC_REG_LR)) {
    if (state1->lr != state2->lr) {
      DBG(EQUIV, "mismatch on lr: %x <-> %x\n", state1->lr, state2->lr);
      return false;
    }
  }
  if (regset_belongs(live_regs, PPC_REG_CTR)) {
    if (state1->ctr != state2->ctr) {
      DBG(EQUIV, "mismatch on ctr: %x <-> %x\n", state1->ctr, state2->ctr);
      return false;
    }
  }
  for (i = 0; i < 8; i++) {
    if (regset_belongs(live_regs, PPC_REG_CRFN(i))) {

      DBG(EQUIV, "crf[%d]: %x <-> %x\n", i, state1->crf[i],
          state2->crf[i]);
      // XXX: ignore the OV bit for now
      if ((state1->crf[i] & 0xe) != (state2->crf[i] & 0xe)) {
        DBG(EQUIV, "mismatch on crf[%d]: %x <-> %x\n", i, state1->crf[i],
            state2->crf[i]);
        return false;
      }
    }
  }
  if (regset_belongs(live_regs, PPC_REG_XER_OV)) {
    if (state1->xer[0] != state2->xer[0]) {
      DBG(EQUIV, "mismatch on xer[0]: %x <-> %x\n", state1->xer[0],
          state2->xer[0]);
      return false;
    }
  }
  if (regset_belongs(live_regs, PPC_REG_XER_CA)) {
    if (state1->xer[1] != state2->xer[1]) {
      DBG(EQUIV, "mismatch on xer[1]: %x <-> %x\n",  state1->xer[1],
          state2->xer[1]);
      return false;
    }
  }
  if (regset_belongs(live_regs, PPC_REG_XER_SO)) {
    if (state1->xer[2] != state2->xer[2]) {
      DBG(EQUIV, "mismatch on xer[2]: %x <-> %x\n",  state1->xer[2],
          state2->xer[2]);
      return false;
    }
  }
  if (regset_belongs(live_regs, PPC_REG_XER_BC)) {
    if (state1->xer[3] != state2->xer[3]) {
      DBG(EQUIV, "mismatch on xer[3]: %x <-> %x\n",  state1->xer[3],
          state2->xer[3]);
      return false;
    }
  }
  if (regset_belongs(live_regs, PPC_REG_XER_CMP)) {
    if (state1->xer[4] != state2->xer[4]) {
      DBG(EQUIV, "mismatch on xer[4]: %x <-> %x\n",  state1->xer[4],
          state2->xer[4]);
      return false;
    }
  }*/
#endif
  if (mem_live_out && memcmp(state1->memory, state2->memory, MEMORY_SIZE)) {
    DBG(EQUIV, "mismatch on %s.\n", "memory");
    return false;
  }
  return true;
}

bool
dst_iseq_rename_regs_and_transmaps_after_inferring_regcons(dst_insn_t *dst_iseq,
    long dst_iseq_len,
    transmap_t *in_tmap_const, transmap_t *out_tmaps_const, long num_live_out,
    regcons_t *regcons, regmap_t *regmap, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  regcons_t lregcons;
  regmap_t lregmap;
  bool ret;
  long i;

  if (!regmap) {
    regmap = &lregmap;
  }
  if (!regcons) {
    regcons = &lregcons;
  }

  dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1);
  ret = dst_infer_regcons_from_assembly(as1, regcons);
  if (!ret) {
    cout << __func__ << " " << __LINE__ << ": failed to infer regcons for assembly =\n" << as1 << endl;
  }
  ASSERT(ret);
  regmap_init(regmap);
  if (!regmap_assign_using_regcons(regmap, regcons, ARCH_DST, NULL, fixed_reg_mapping)) {
    //cout << __func__ << " " << __LINE__ << ": regcons =\n" << regcons_to_string(regcons, as1, sizeof as1) << endl;
    return false;
  }
  memslot_map_t const *memslot_map = memslot_map_t::memslot_map_for_src_env_state();
  dst_iseq_rename(dst_iseq, dst_iseq_len, regmap, NULL, memslot_map, NULL, NULL, NULL, 0);

  if (in_tmap_const) {
    transmap_rename_using_dst_regmap(in_tmap_const, regmap, memslot_map);
  }
  for (i = 0; i < num_live_out; i++) {
    transmap_rename_using_dst_regmap(&out_tmaps_const[i], regmap, memslot_map);
  }
  return true;
}

/*void
ti_fallback_init(void)
{
  translation_instance_init(&ti_fallback, SUBSETTING);
  peep_table_load(&ti_fallback);
}

void
ti_fallback_free(void)
{
  translation_instance_free(&ti_fallback);
}*/


state_t
state_t::read_concrete_state(istream &iss)
{
  state_t ret;
  int i, j;
  for (i = 0; i < MAX_NUM_EXREG_GROUPS; i++) {
    for (j = 0; j < MAX_NUM_EXREGS(i); j++) {
      string regname;
      uint32_t regval;
      char colon;
      iss >> regname;
      //cout << __func__ << " " << __LINE__ << ": regname = '" << regname << "'\n";
      if (regname.substr(0, 5) != "exreg") {
        cout << __func__ << " " << __LINE__ << ": regname = '" << regname << "'\n";
        fflush(stdout);
      }
      ASSERT(regname.substr(0, 5) == "exreg");
      iss >> colon;
      ASSERT(colon == ':');
      iss >> std::hex >> regval >> std::dec;
      size_t dot;
      exreg_group_id_t groupnum = stoi(regname.substr(5), &dot);
      exreg_id_t regnum = stoi(regname.substr(5 + dot + 1));
      ASSERT(groupnum == i);
      ASSERT(regnum == j);
      ret.exregs[i][j].state_reg_from_int64(regval);
    }
  }
  for (i = 0; i < MAX_NUM_MEMLOCS; i++) {
    string regname;
    uint32_t regval;
    char colon;
    iss >> regname;
    ASSERT(regname.substr(0, 8) == "memlocs[");
    iss >> colon;
    ASSERT(colon == ':');
    iss >> std::hex >> regval >> std::dec;
    size_t dot;
    size_t memloc_id = stoi(regname.substr(8));
    ASSERT(memloc_id == i);
    ret.memlocs[i].state_reg_from_int64(regval);
  }
  string word;

  iss >> word;
  ASSERT(word == "stack:");
  for (int i = 0; i < (STATE_MAX_STACK_SIZE >> 2); i++) {
    uint32_t memval;
    iss >> std::hex >> memval >> std::dec;
    ret.stack[i] = memval;
    //cout << "memory[" << i << "] = " << std::hex << memval << std::dec << endl;
  }

  int nextpc_num;
  iss >> word >> nextpc_num;
  ASSERT(word == "nextpc_num:");
  ret.nextpc_num = nextpc_num;
  iss >> word;
  ASSERT(word == "memory:");
  ret.memory = (uint32_t *)malloc32(((MEMORY_SIZE + MEMORY_PADSIZE) >> 2) * sizeof(uint32_t));
  ASSERT(ret.memory);
  for (int i = 0; i < ((MEMORY_SIZE + MEMORY_PADSIZE) >> 2); i++) {
    uint32_t memval;
    iss >> std::hex >> memval >> std::dec;
    ret.memory[i] = memval;
    //cout << "memory[" << i << "] = " << std::hex << memval << std::dec << endl;
  }
  iss >> word;
  ASSERT(word == "imm_vt_map:");
  ret.imm_vt_map.imm_vt_map_from_stream(iss);
  return ret;
}

void
state_t::set_exreg_to_value(exreg_group_id_t g, exreg_id_t r, uint32_t value)
{
  ASSERT(g >= 0 && g < MAX_NUM_EXREG_GROUPS);
  ASSERT(r >= 0 && r < MAX_NUM_EXREGS(g));
  this->exregs[g][r].state_reg_from_int64(value);
}

void
state_t::dst_state_make_exregs_consistent()
{
#if ARCH_DST == ARCH_I386
  //consider eq, ult, slt as the basis that decides all other values
  bool eq = this->exregs[I386_EXREG_GROUP_EFLAGS_EQ][0].state_reg_to_bool();
  bool ult = this->exregs[I386_EXREG_GROUP_EFLAGS_ULT][0].state_reg_to_bool();
  bool slt = this->exregs[I386_EXREG_GROUP_EFLAGS_SLT][0].state_reg_to_bool();

  this->exregs[I386_EXREG_GROUP_EFLAGS_NE][0].state_reg_from_bool(!eq);
  this->exregs[I386_EXREG_GROUP_EFLAGS_UGE][0].state_reg_from_bool(!ult);
  this->exregs[I386_EXREG_GROUP_EFLAGS_SGE][0].state_reg_from_bool(!slt);

  this->exregs[I386_EXREG_GROUP_EFLAGS_ULE][0].state_reg_from_bool(ult || eq);
  this->exregs[I386_EXREG_GROUP_EFLAGS_SLE][0].state_reg_from_bool(slt || eq);
  this->exregs[I386_EXREG_GROUP_EFLAGS_UGT][0].state_reg_from_bool(!(ult || eq));
  this->exregs[I386_EXREG_GROUP_EFLAGS_SGT][0].state_reg_from_bool(!(slt || eq));

  this->exregs[I386_EXREG_GROUP_EFLAGS_OTHER][0].state_reg_from_int64(i386_eflags_construct_from_cmp_bits(eq, ult, slt));
#else
  NOT_IMPLEMENTED();
#endif
}
