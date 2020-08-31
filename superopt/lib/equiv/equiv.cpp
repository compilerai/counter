#include "equiv/equiv.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/mman.h>
#include "superoptdb_rpc.h"
#include "support/c_utils.h"
#include "support/debug.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "i386/regs.h"
#include "valtag/regset.h"
#include "valtag/regmap.h"
#include "valtag/regcons.h"
#include "ppc/insn.h"
#include "support/src-defs.h"
#include "i386/execute.h"
#include "ppc/execute.h"
#include "superopt/src_env.h"
//#include "imm_map.h"
#include "superopt/superopt.h"
#include "superopt/execute.h"
#include "rewrite/transmap.h"
#include "rewrite/static_translate.h"
#include "superopt/superoptdb.h"
#include "rewrite/function_pointers.h"
#include "rewrite/translation_instance.h"
#include "support/timers.h"
#include "equiv/bequiv.h"
#include "rewrite/bbl.h"
#include "rewrite/src-insn.h"
#include "rewrite/dst-insn.h"
#include "superopt/typestate.h"
#include "rewrite/peephole.h"
//#include "rewrite/harvest.h"
#include "equiv/jtable.h"
#include "sym_exec/sym_exec.h"
#include "superopt/rand_states.h"
#include "valtag/memslot_set.h"
#include "superopt/fingerprintdb_live_out.h"
#include "rewrite/txinsn.h"
#include "valtag/nextpc.h"

//static uint8_t /**dst_exec_code = NULL, */*src_exec_code = NULL;
//void *memtmp = 0;
extern CLIENT *rpc_client;

//bool verify_boolean = true;
bool verify_execution = true;
bool verify_fingerprint = true;
bool verify_syntactic_check = true;
bool fp_imm_map_is_constant = true; //XXX: this should be false. currently, there is a bug in fingerprintdb_query_using_typestate
bool terminate_on_equiv_failure = false;
bool peepgen_assume_fcall_conventions = false;
use_types_t default_use_types = USE_TYPES_OPS_AND_GE_FINAL;

//char const *peeptab_filename;


//translation_instance_t ti_fallback(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());

//static imm_map_t fp_imm_map;
//state_t fp_state[NUM_FP_STATES];
//rand_states_t fp_state;

static char as1[4096000];
static char as2[40960];
static char as3[40960];
static char pbuf1[409600], pbuf2[5120], pbuf3[5120];

//state_t *rand_state = NULL;
//int imm_map_index[NUM_STATES];

uint8_t **exec_next_tcodes = NULL;
//txinsn_t *exec_next_tcodes = NULL;
//static src_ulong exec_insn_pcs[SRC_MAX_ILEN];
nextpc_t *exec_nextpcs = NULL;
long exec_nextpcs_size = 0;
long num_exec_nextpcs = 0;
//static long num_exec_insn_pcs = 0;

static void
dst_gen_nextpc_code(uint8_t *buf, size_t size, long nextpc_num)
{
  dst_insn_t iseq[64], *iptr, *iend;
  uint8_t *ptr, *end;

  iptr = iseq;
  iend = iptr + sizeof iseq/sizeof iseq[0];

  fixed_reg_mapping_t const &dst_fixed_reg_mapping = fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity();

#if ARCH_DST == ARCH_I386
  iptr += i386_insn_put_movl_imm_to_mem(nextpc_num,
      (dst_ulong)(unsigned long)SRC_ENV_NEXTPC_NUM, -1, iptr, iend - iptr);
  iptr += i386_insn_put_movq_mem_to_reg(R_ESP, SRC_ENV_SCRATCH(0), iptr, iend - iptr);
  iptr += i386_insn_put_popa(dst_fixed_reg_mapping, iptr, iend - iptr, I386_AS_MODE_64);
  iptr += i386_insn_put_function_return64(dst_fixed_reg_mapping, iptr, iend - iptr);
#else
  NOT_IMPLEMENTED();
#endif

  ptr = buf;
  end = buf + size;

  ptr += dst_iseq_assemble(NULL, iseq, iptr - iseq, ptr, end - ptr, I386_AS_MODE_64);
  ASSERT(ptr < end);
}



void
alloc_exec_nextpcs(long num_live_out)
{
  long i, max_alloc;

  if (num_live_out <= num_exec_nextpcs) {
    return;
  }

  if (num_live_out > exec_nextpcs_size) {
    cout << __func__ << " " << __LINE__ << ": exec_nextpcs_size = " << exec_nextpcs_size << endl;
    cout << __func__ << " " << __LINE__ << ": num_live_out = " << num_live_out << endl;
    NOT_REACHED();
    //exec_nextpcs_size = (num_live_out + 1) * 2;
    //exec_next_tcodes = (txinsn_t *)realloc(exec_next_tcodes,
    //    exec_nextpcs_size * sizeof(txinsn_t));
    //ASSERT(exec_next_tcodes);
    //exec_nextpcs = (src_ulong *)realloc(exec_nextpcs,
    //    exec_nextpcs_size * sizeof(src_ulong));
    //ASSERT(exec_nextpcs);
  }
  ASSERT(num_live_out <= exec_nextpcs_size);

  max_alloc = 256;
  for (i = num_exec_nextpcs; i < num_live_out; i++) {
    uint8_t *exec_next_tcode = (uint8_t *)malloc32(max_alloc * sizeof(uint8_t));
    ASSERT(exec_next_tcode);
    //exec_next_tcodes[i] = txinsn_t(tag_var, (long)exec_next_tcode);
    exec_next_tcodes[i] = exec_next_tcode;
    dst_gen_nextpc_code(exec_next_tcode, max_alloc, i);
    exec_nextpcs[i] = (src_ulong)i + 100;
    ASSERT(exec_nextpcs[i].get_val() != PC_JUMPTABLE && exec_nextpcs[i].get_val() != PC_UNDEFINED);
  }
  num_exec_nextpcs = num_live_out;
}

void
equiv_init(void)
{
  ASSERT(sizeof(state_reg_t) == MAX_NUM_TE_PER_ENTRY);
  ASSERT(sizeof(state_t) <= SRC_ENV_SIZE);
  if (!verify_execution && !verify_fingerprint) {
    return;
  }

  //dst_exec_code = (uint8_t *)mmap(NULL, MAX_CODE_LEN, PROT_EXEC | PROT_READ | PROT_WRITE,
  //    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  //src_exec_code = (uint8_t *)mmap(NULL, MAX_CODE_LEN, PROT_EXEC | PROT_READ | PROT_WRITE,
  //    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  translation_instance_t &ti_fallback = get_ti_fallback();

  translation_instance_init(&ti_fallback, FBGEN);

  read_peep_trans_table(&ti_fallback, &ti_fallback.peep_table,
      SUPEROPTDBS_DIR "fb.trans.tab", NULL);

  if (init_execution_environment() < 0) {
    fprintf(stderr, "failed to initialize execution environment. exiting.\n");
    exit(1);
  }
  //rand_state = (state_t *)malloc(NUM_STATES * sizeof(state_t));
  //ASSERT(rand_state);
  //init_rand_states(rand_state, NUM_STATES, imm_map_index, NUM_IMM_MAPS);
  //function_pointer_register("boolean_check_equivalence", boolean_check_equivalence);

  //exec_next_tcodes = new txinsn_t[MAX_EXEC_NEXTPCS];
  exec_next_tcodes = new uint8_t *[MAX_EXEC_NEXTPCS];
  ASSERT(exec_next_tcodes);
  exec_nextpcs = new nextpc_t[MAX_EXEC_NEXTPCS];
  ASSERT(exec_nextpcs);
  exec_nextpcs_size = MAX_EXEC_NEXTPCS;
}

static char *
test_details(int state_num, src_insn_t const *src_iseq,
    size_t src_iseq_len,
    dst_insn_t const *dst_iseq,
    size_t dst_iseq_len,
    transmap_t const *in_tmap,
    regset_t const *live_out, long num_live_out, bool mem_live_out,
    state_t const *istate, state_t const *dst_outstate,
    state_t const *src_outstate, char *buf, size_t size)
{
  char *ptr = buf, *end = buf + size;
  long l, i;

  ptr += snprintf(ptr, end - ptr, "verification test on state #%d:\n"
      "src_iseq:\n%s.\ndst_iseq:\n%s.\n", state_num,
      src_iseq_to_string(src_iseq, src_iseq_len, as1, sizeof as1),
      dst_iseq_to_string(dst_iseq, dst_iseq_len, as2, sizeof as2));
  ASSERT(ptr < end);
  for (l = 0; l < num_live_out; l++) {
    ptr += snprintf(ptr, end - ptr, "live_out[%ld]: %s\n", l,
        regset_to_string(&live_out[l], as1, sizeof as1));
  }
  ASSERT(ptr < end);
  ptr += snprintf(ptr, end - ptr, "in_tmap:\n%s\n",
      //regmap_to_string(regmap, as1, sizeof as1),
      transmap_to_string(in_tmap, as2, sizeof as2));
  ASSERT(ptr < end);
  /*ptr += snprintf(ptr, end - ptr, "const_src_iseq:\n%s.\nconst_i386_iseq:\n%s.\n",
      src_iseq_to_string(const_src_iseq, src_iseq_len, as1, sizeof as1),
      i386_iseq_to_string(const_i386_iseq, i386_iseq_len, as2, sizeof as2));
  ASSERT(ptr < end);*/
  ptr += snprintf(ptr, end - ptr, "input_state:\n%s\n",
      state_to_string(istate, as1, sizeof as1));
  ASSERT(ptr < end);
  ptr += snprintf(ptr, end - ptr, "src_outstate:\n%s\n",
      state_to_string(src_outstate, as1, sizeof as1));
  ASSERT(ptr < end);
  ptr += snprintf(ptr, end - ptr, "dst_outstate:\n%s\n",
      state_to_string(dst_outstate, as1, sizeof as1));
  ASSERT(ptr < end);
  if (mem_live_out) {
    for (i = 0; i < (MEMORY_SIZE + MEMORY_PADSIZE) >> 2; i++) {
      if (src_outstate->memory[i] != dst_outstate->memory[i]) {
        ptr += snprintf(ptr, end - ptr, "memory mismatch on dword %ld: %x<->%x\n", i,
            src_outstate->memory[i], dst_outstate->memory[i]);
        ASSERT(ptr < end);
      }
    }
  }
  ptr += snprintf(ptr, end - ptr, "imm_map:\n%s\n",
      imm_vt_map_to_string(&src_outstate->imm_vt_map, as1, sizeof as1));
  return buf;
}

static void
alloc_exec_insn_pcs(long num_insns)
{
  NOT_IMPLEMENTED();
}

static bool
transmap_maps_to_dst_reg(transmap_t const *tmap, exreg_group_id_t g, exreg_id_t dst_reg)
{
  for (const auto &tg : tmap->extable) {
    for (const auto &tr : tg.second) {
      if (tr.second.transmap_entry_maps_to_dst_reg(g, dst_reg)) {
        return true;
      }
    }
  }
  return false;
}

static bool
transmaps_map_to_dst_reserved_regs(transmap_t const *in_tmap,
    transmap_t const *out_tmaps, long num_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping)
{
  for (const auto &g : dst_reserved_regs.excregs) {
    for (const auto &r : g.second) {
      exreg_id_t r_reserved = dst_fixed_reg_mapping.get_mapping(g.first, r.first.reg_identifier_get_id());
      if (r_reserved == -1) {
        continue;
      }
      if (transmap_maps_to_dst_reg(in_tmap, g.first, r_reserved)) {
        return true;
      }
      for (long l = 0; l < num_live_out; l++) {
        if (transmap_maps_to_dst_reg(&out_tmaps[l], g.first, r_reserved)) {
          return true;
        }
      }
    }
  }
  return false;
}

static void
fixed_reg_mapping_add_unused_reg_mapping_for_dst_stack_regnum(fixed_reg_mapping_t &exec_fixed_reg_mapping,
    dst_insn_t const *dst_iseq, long dst_iseq_len)
{
  if (exec_fixed_reg_mapping.get_mapping(DST_EXREG_GROUP_GPRS, DST_STACK_REGNUM) != -1) {
    return;
  }
  regset_t use, def;
  dst_iseq_get_usedef_regs(dst_iseq, dst_iseq_len, &use, &def);
  regset_union(&use, &def);
  for (exreg_id_t r = 0; r < DST_NUM_EXREGS(DST_EXREG_GROUP_GPRS); r++) {
    if (regset_belongs_ex(&use, DST_EXREG_GROUP_GPRS, r)) {
      continue;
    }
    exec_fixed_reg_mapping.add(DST_EXREG_GROUP_GPRS, DST_STACK_REGNUM, r);
    return;
  }
  NOT_REACHED();
}

bool
execution_check_equivalence(src_insn_t const src_iseq[], int src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    dst_insn_t const dst_iseq[], int dst_iseq_len,
    regset_t const *live_out, long num_live_out, bool mem_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping)
{
  //cout << __func__ << " " << __LINE__ << endl;
  state_t *dst_outstate;
  state_t *src_outstate;
  bool ret;
  int i;

  stopwatch_run(&execution_check_equivalence_timer);

  if (!src_iseq_supports_execution_test(src_iseq, src_iseq_len)) {
    DBG(EQUIV, "%s", "src_iseq does not support execution test.\n");
    stopwatch_stop(&execution_check_equivalence_timer);
    return true;
  }
  if (!dst_iseq_supports_execution_test(dst_iseq, dst_iseq_len)) {
    DBG(EQUIV, "%s", "dst_iseq does not support execution test.\n");
    stopwatch_stop(&execution_check_equivalence_timer);
    return true;
  }
  ASSERT(!transmaps_map_to_dst_reserved_regs(in_tmap, out_tmaps, num_live_out, dst_fixed_reg_mapping));
  DBG2(EQUIV, "Verifying:\nsrc:\n%s\ndst:\n%s\nin_tmap:\n%s\n",
      src_iseq_to_string(src_iseq, src_iseq_len, pbuf1, sizeof pbuf1),
      dst_iseq_to_string(dst_iseq, dst_iseq_len, pbuf2, sizeof pbuf2),
      transmap_to_string(in_tmap, pbuf3, sizeof pbuf3));

  /*src_iseq_to_string(src_iseq, src_iseq_len, pbuf1, sizeof pbuf1);
  src_infer_regcons_from_assembly(assembly, &regcons);
  regmap_assign_using_regcons(&regmap, &regcons, NULL);*/

  /*if (!src_iseq_assign_vregs_to_cregs(const_src_iseq, src_iseq, src_iseq_len,
      &regmap, true)) {
    // this can happen for opcodes like 'bt'
    ASSERT(src_iseq_assign_vregs_to_cregs(const_src_iseq, src_iseq, src_iseq_len,
      &regmap, false));
    DBG(EQUIV, "%s", "returning true because could not assign vregs to cregs for "
        "src_iseq\n");
    return true;
  }*/

  //transmap_copy(&concrete_in_tmap, transmap);
  //transmaps_copy(concrete_out_tmaps, out_tmaps, num_live_out);

  //transmap_copy(&concrete_in_tmap_copy, &concrete_in_tmap);

  /*if (!i386_iseq_assign_vregs_to_cregs_using_transmap_and_temporaries(const_i386_iseq,
      i386_iseq, i386_iseq_len, &concrete_in_tmap, &temps, true)) {
    // this can happen for opcodes like 'bt'
    ASSERT(i386_iseq_assign_vregs_to_cregs_using_transmap_and_temporaries(const_i386_iseq,
        i386_iseq, i386_iseq_len, &concrete_in_tmap, &temps, false));
    DBG(EQUIV, "%s", "returning true because could not assign vregs to cregs for "
        "i386_iseq\n");
    return true;
  }*/
  /*XXX: transmaps_acquiesce(&concrete_in_tmap_copy, concrete_out_tmaps, NULL,
      NULL, num_live_out, &concrete_in_tmap); //change out_tmaps just as in_tmap has been changed
  */

  /*DBG(EQUIV, "Executing:\nsrc:\n%s\ni386:\n%s\ntmap:\n%s\n",
      src_iseq_to_string(const_src_iseq, src_iseq_len, pbuf1, sizeof pbuf1),
      i386_iseq_to_string(const_i386_iseq, i386_iseq_len, pbuf2, sizeof pbuf2),
      transmap_to_string(in_tmap, pbuf3, sizeof pbuf3));
  DBG(EQUIV, "regmap:\n%s\n",
      regmap_to_string(&regmap, pbuf1, sizeof pbuf1));*/

  //regset_t use, def;
  //src_iseq_get_usedef_regs(src_iseq, src_iseq_len, &use, &def);
 
  rand_states_t *rand_states = new rand_states_t;
  rand_states->init_rand_states_using_src_iseq(src_iseq, src_iseq_len, *in_tmap, out_tmaps, live_out, num_live_out);
  size_t num_states = rand_states->get_num_states();
  //state_t *rand_state = (state_t *)malloc(NUM_STATES * sizeof(state_t));
  //ASSERT(rand_state);
  //int imm_map_index[NUM_IMM_MAPS];
  //size_t num_states = init_rand_states_using_src_iseq(rand_state, NUM_STATES, imm_map_index, NUM_IMM_MAPS, src_iseq, src_iseq_len);

  dst_outstate = (state_t *)malloc(num_states * sizeof(state_t));
  src_outstate = (state_t *)malloc(num_states * sizeof(state_t));
  ASSERT(dst_outstate);
  ASSERT(src_outstate);

  states_init(src_outstate, num_states);
  states_init(dst_outstate, num_states);
  alloc_exec_nextpcs(num_live_out);

  DBG3(EQUIV, "%s", "Generating src out states\n");
  DBG_EXEC3(EQUIV,
    long i;
    for (i = 0; i < num_live_out; i++) {
      printf("exec_next_tcodes[%ld] = %p\n", i, exec_next_tcodes[i]);
    });
  //DBG2(EQUIV, "before src exec, rand_state[0]:\n%s\n",
  //    state_to_string(&rand_state[0], as1, sizeof as1));
  if (!src_gen_out_states(src_iseq, src_iseq_len, *rand_states,
          src_outstate, num_states, exec_nextpcs,
          exec_next_tcodes, in_tmap,
          out_tmaps, num_live_out)) {
    DBG(EQUIV, "%s", "Could not generate src out states, returning true\n");
    //states_free(rand_state, num_states);
    //rand_states->rand_states_free();
    states_free(src_outstate, num_states);
    states_free(dst_outstate, num_states);
    //free(rand_state);
    delete rand_states;
    free(dst_outstate);
    free(src_outstate);
    return true;
  }
  DBG3(EQUIV, "%s", "Generating dst out states\n");
  DBG_EXEC2(EQUIV,
    long i;
    for (i = 0; i < num_live_out; i++) {
      printf("exec_next_tcodes[%ld] = %p\n", i, exec_next_tcodes[i]);
    });
  DBG2(EQUIV, "before dst exec, rand_states[0]:\n%s\n",
      state_to_string(&rand_states->rand_states_get_state0(), as1, sizeof as1));
  fixed_reg_mapping_t exec_fixed_reg_mapping = dst_fixed_reg_mapping;
  fixed_reg_mapping_add_unused_reg_mapping_for_dst_stack_regnum(exec_fixed_reg_mapping, dst_iseq, dst_iseq_len);
  rand_states->make_dst_stack_regnum_point_to_stack_top(exec_fixed_reg_mapping);
  ret = dst_gen_out_states/*_using_transmap*/(dst_iseq, dst_iseq_len,
      *rand_states, dst_outstate, num_states, exec_nextpcs,
      exec_next_tcodes/*,
      in_tmap, out_tmaps*/, num_live_out/*, true*/, exec_fixed_reg_mapping);
  ASSERT(ret);

  ret = true;
  /*for (m = 0; m < NUM_IMM_MAPS && ret; m++) {
    int i;
    for (i = ((m == 0)?0:imm_map_index[m - 1]); i < imm_map_index[m]; i++) {
    }
  }*/
  regset_t dst_live_out[num_live_out];
  memslot_set_t dst_live_out_memslot[num_live_out];
  for (size_t i = 0; i < num_live_out; i++) {
    regset_rename_using_transmap(&dst_live_out[i], &dst_live_out_memslot[i], &live_out[i], &out_tmaps[i]);
  }
  for (i = 0; i < num_states; i++) {
    ASSERT(src_outstate[i].nextpc_num != NEXTPC_TYPE_ERROR);
    ASSERT(dst_outstate[i].nextpc_num != NEXTPC_TYPE_ERROR);
    ASSERT(dst_outstate[i].nextpc_num >= 0 && dst_outstate[i].nextpc_num < num_live_out);
    ASSERT(src_outstate[i].nextpc_num >= 0 && src_outstate[i].nextpc_num < num_live_out);
    if (states_equivalent(&src_outstate[i], &dst_outstate[i], dst_live_out, dst_live_out_memslot,
            num_live_out, mem_live_out)) {
      DBG3(EQUIV, "PASSED: %s", test_details(i, src_iseq, src_iseq_len,
            dst_iseq, dst_iseq_len, in_tmap,
            live_out, num_live_out, mem_live_out, &rand_states->rand_states_get_state(i), &dst_outstate[i], &src_outstate[i],
            pbuf1, sizeof pbuf1));
    } else {
      DBG2(EQUIV, "FAILED: %s", test_details(i, src_iseq, src_iseq_len,
            dst_iseq, dst_iseq_len, in_tmap,
            live_out, num_live_out, mem_live_out, &rand_states->rand_states_get_state(i), &dst_outstate[i], &src_outstate[i],
            pbuf1, sizeof pbuf1));
      ret = false;
      break;
    }
  }
  //rand_states->rand_states_free();
  states_free(src_outstate, num_states);
  states_free(dst_outstate, num_states);
  //free(rand_state);
  delete rand_states;
  free(dst_outstate);
  free(src_outstate);
  DBG(EQUIV, "execution test %s.\n", ret ?"passed":"failed");
  stopwatch_stop(&execution_check_equivalence_timer);
  return ret;
}

uint64_t
state_compute_fingerprint(state_t const *instate, state_t const *state,
    regset_t const *live_out, memslot_set_t const *live_out_memslots,
    long num_live_out, bool mem_live_out)
{
  memslot_set_t const *live_memslots;
  regset_t const *live_regs;
  uint64_t ret = 0;
  long i, j, n;

  ASSERT(state->nextpc_num != NEXTPC_TYPE_ERROR);
  if (state->nextpc_num == NEXTPC_TYPE_ERROR) {
    return FINGERPRINT_TYPE_ERROR;
  }
  stopwatch_run(&state_compute_fingerprint_timer);

  DBG_EXEC3(EQUIV,
     cout << __func__ << " " << __LINE__ << ": fingerprinting state = " << state_to_string(state, as1, sizeof as1) << endl;
  );
  //printf("state->nextpc_num = %d\n", (int)state->nextpc_num);
  //printf("num_live_out = %d\n", (int)num_live_out);
  ASSERT(state->nextpc_num >= 0 && state->nextpc_num < num_live_out);
  ret = 1ULL << (64 - state->nextpc_num);
  live_regs = &live_out[state->nextpc_num];
  live_memslots = &live_out_memslots[state->nextpc_num];
  DBG_EXEC2(EQUIV,
      regsets_to_string(live_out, num_live_out, as1, sizeof as1);
      printf("%s() %d: Computing fingerprint. live_out: %s\n", __func__, __LINE__, as1);
  );
  DBG3(EQUIV, "ret = %llx.\n", (long long)ret);

  for (const auto &g : live_regs->excregs) {
    i = g.first;
    for (const auto &r : g.second) {
      j = r.first.reg_identifier_get_id();
      /*if (!regset_belongs_ex(live_regs, i, j)) {
        continue;
      }*/
      n = i * MAX_NUM_EXREGS(0) + j;
      uint64_t mask = regset_get_elem(live_regs, i, j);
      uint64_t regret = state->exregs[i][j].state_reg_compute_fingerprint(n, mask, instate->exregs[i][j]);
      DBG3(EQUIV, "i = %ld, j = %ld: instate->exreg=%lx, state->exreg=%lx, mask=%lx, "
          "regret = %lx.\n", i, j, (long)instate->exregs[i][j].state_reg_to_int64(),
          (long)state->exregs[i][j].state_reg_to_int64(), (long)mask, (long)regret);
      ret ^= regret;
    }
  }
  DBG3(EQUIV, "ret = %llx.\n", (long long)ret);
  for (const auto &m : live_memslots->get_map()) {
    i = m.first;
    /*if (!live_memslots->memslot_set_belongs(i)) {
      continue;
    }*/
    n = MAX_NUM_EXREG_GROUPS * MAX_NUM_EXREGS(0) + i;
    uint64_t mask = live_memslots->memslot_set_get_elem(i);
    uint64_t regret = state->memlocs[i].state_reg_compute_fingerprint(n, mask, instate->memlocs[i]);
    DBG3(EQUIV, "looking at memloc %ld. mask = %llx, insstate->memlocs[i] = %llx, state->memlocs[i] = %llx, regret = %llx\n", (long)i, (long long)mask, (long long)instate->memlocs[i].state_reg_to_int64(), (long long)state->memlocs[i].state_reg_to_int64(), (long long)regret);
    ret ^= regret;
  }
  DBG3(EQUIV, "ret = %llx.\n", (long long)ret);
  if (mem_live_out) {
    for (i = 0; i < (MEMORY_SIZE >> 2); i++) {
      uint64_t memret;
      j = i % (MEMORY_SIZE >> 2);
      memret = instate->memory[j] ^ state->memory[j];
      DBG3(EQUIV, "instate->memory[%ld]=%lx, state->memory[%ld]=%ld, "
          "memret = %lx.\n", j, (long)instate->memory[j], j, (long)state->memory[j],
          (long)memret);
      memret *= (j + 1) * 32451ULL;
      ret ^= (uint64_t)memret;
      DBG3(EQUIV, "j = %ld: memret = %llx.\n", j, (long long)memret);
    }
  }
  if (ret == FINGERPRINT_TYPE_ERROR) {
    //this should be very rare. just set the fingerprint to some canonical value which is different from FINGERPRINT_TYPE_ERROR
    ret = FINGERPRINT_TYPE_ERROR - 1;
  }
  stopwatch_stop(&state_compute_fingerprint_timer);

  return ret;
}

/*static void
fingerprintdb_compute_typestate(typestate_t *typestate_initial,
    typestate_t *typestate_final, fingerprintdb_elem_t * const *fingerprintdb)
{
  fingerprintdb_elem_t *cur;
  long i;

  for (i = 0; i < FINGERPRINTDB_SIZE; i++) {
    for (cur = fingerprintdb[i]; cur; cur = cur->next) {
      fingerprintdb_elem_compute_typestate(typestate_initial, typestate_final, cur);
    }
  }
}*/

/*void
src_fingerprintdb_remove_variants(src_fingerprintdb_elem_t **ret, src_insn_t const *src_iseq,
    long src_iseq_len, regset_t const *live_out,
    transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_live_out,
    use_types_t use_types, bool mem_live_out, regmap_t const *in_regmap)
{
  regset_t use, def, touch, dst_touch, temps;
  src_fingerprintdb_elem_t *elem, *prev;
  uint64_t fp[NUM_FP_STATES];
  regmap_t regmap, pregmap;
  regset_t all_live_out;
  bool next;
  long i;

  src_iseq_get_usedef_regs(src_iseq, src_iseq_len, &use, &def);
  regset_copy(&touch, &use);
  regset_clear(&all_live_out);
  for (i = 0; i < num_live_out; i++) {
    regset_union(&all_live_out, &live_out[i]);
  }
  //do not consider def'ed registers that are not live at exit.
  regset_intersect(&def, &all_live_out);
  regset_union(&touch, &def);

  regset_rename_using_transmap(&dst_touch, &touch, in_tmap);

  regmap_canonical_for_regset(&regmap, &dst_touch);

  regset_get_temporaries(&temps, &dst_touch);
  dst_regmap_add_mappings_for_regset(&regmap, &temps);

  if (in_regmap) {
    regmaps_combine(&regmap, in_regmap);
  }


  //XXX: have some mechanism to ensure that relative permutations of
  //the temporary registers are not considered

  DBG(TYPESTATE, "regmap:\n%s\n", regmap_to_string(&regmap, as1, sizeof as1));
  DBG_EXEC(TYPESTATE,
      src_fingerprintdb_to_string(as1, sizeof as1, ret, false);
      printf("%s() %d: Printing src_fingerprintdb:\n%s\n", __func__, __LINE__, as1);
  );

  for (next = regmap_permutation_enumerate_first(&pregmap, &regmap);
      next;
      next = regmap_permutation_enumerate_next(&pregmap, &regmap)) {
    transmap_t r_in_tmap, r_out_tmaps[num_live_out];

    DBG(TYPESTATE, "pregmap:\n%s\n", regmap_to_string(&pregmap, as1, sizeof as1));
    transmap_copy(&r_in_tmap, in_tmap);
    transmaps_copy(r_out_tmaps, out_tmaps, num_live_out);
    if (   !transmaps_rename_using_dst_regmap(&r_in_tmap, r_out_tmaps,
               num_live_out, &pregmap)
        && !regmaps_equal(&pregmap, &regmap)) {
      DBG(TYPESTATE, "transmaps_rename failed. pregmap != regmap. pregmap:\n%s\n",
          regmap_to_string(&pregmap, as1, sizeof as1));
      continue;
    }

    src_iseq_compute_fingerprint(fp, src_iseq, src_iseq_len,
        &r_in_tmap, r_out_tmaps, live_out, num_live_out, mem_live_out);
    DBG(TYPESTATE, "fp[0] = %llx.\n", (unsigned long long)fp[0]);

    ASSERT(NUM_FP_STATES > 0);
    for (prev = NULL, elem = ret[fp[0] % FINGERPRINTDB_SIZE];
        elem;
        prev = elem, elem = elem->next) {
      DBG(TYPESTATE, "elem = %p\n", elem);
      if (elem->cmn_iseq_len != src_iseq_len) {
        DBG(TYPESTATE, "elem->src_iseq_len (%ld) != src_iseq_len (%ld)\n",
            elem->cmn_iseq_len, src_iseq_len);
        continue;
      }
      DBG(TYPESTATE, "cmn_iseq_len = %ld\n", src_iseq_len);
      if (!src_iseqs_equal(elem->cmn_iseq, src_iseq, src_iseq_len)) {
        DBG(TYPESTATE, "src_iseqs not equal. elem->src_iseq:\n%s\n",
            src_iseq_to_string(elem->cmn_iseq, elem->cmn_iseq_len, as1, sizeof as1));
        continue;
      }
      if (elem->num_live_out != num_live_out) {
        DBG(TYPESTATE, "num_live_out not equal, num_live_out = %ld\n", num_live_out);
        continue;
      }
      DBG(TYPESTATE, "num_live_out = %ld\n", num_live_out);
      if (!regset_arr_equal(elem->live_out, live_out, elem->num_live_out)) {
        DBG(TYPESTATE, "live out not equal. elem->live_out[0]:\n%s\n",
            regset_to_string(&elem->live_out[0], as1, sizeof as1));
        continue;
      }
      if (!transmaps_equal(&elem->in_tmap, &r_in_tmap)) {
        DBG(TYPESTATE, "in_tmap not equal. elem->in_tmap:\n%s\n",
            transmap_to_string(&elem->in_tmap, as1, sizeof as1));
        continue;
      }
      if (!transmap_arr_equal(elem->out_tmaps, r_out_tmaps, num_live_out)) {
        DBG(TYPESTATE, "out_tmaps not equal. elem->out_tmaps[0]:\n%s\n",
            transmap_to_string(&elem->out_tmaps[0], as1, sizeof as1));
        continue;
      }
      if (elem->mem_live_out != mem_live_out) {
        DBG(TYPESTATE, "mem_live_out not equal. elem->mem_live_out: %d.\n",
            elem->mem_live_out);
        continue;
      }
      DBG(TYPESTATE, "elem->dst_regmap:\n%s\n",
          regmap_to_string(&elem->dst_regmap, as1, sizeof as1));
      DBG(TYPESTATE, "pregmap:\n%s\n",
          regmap_to_string(&pregmap, as1, sizeof as1));
      if (!regmaps_equal(&elem->dst_regmap, &pregmap)) {
        DBG(TYPESTATE, "%s", "regmaps not equal\n");
        continue;
      }
      DBG_EXEC(TYPESTATE,
          printf("Removing elem %p to src_fingerprintdb:\n%s", elem,
              src_iseq_to_string(elem->cmn_iseq, elem->cmn_iseq_len, as1, sizeof as1));
          regsets_to_string(elem->live_out, elem->num_live_out, as1, sizeof as1);
          printf("--\nlive :%s\n", as1);
          printf("--\n%s",
              transmaps_to_string(&elem->in_tmap, elem->out_tmaps, elem->num_live_out,
                  as1, sizeof as1));
          printf("memlive : %s\n", elem->mem_live_out ? "true" : "false");
          printf("== fp[0] %llx ==\n", (long long)elem->fp[0]);
          printf("pregmap:\n%s\n", regmap_to_string(&pregmap, as1, sizeof as1));
      );
      if (!prev) {
        DBG(JTABLE, "setting ret[%ld] to %p\n",
            (long)(fp[0] % FINGERPRINTDB_SIZE), elem->next);
        ret[fp[0] % FINGERPRINTDB_SIZE] = elem->next;
      } else {
        DBG(JTABLE, "setting prev(%p)->next to %p\n", prev, elem->next);
        prev->next = elem->next;
      }
      src_fingerprintdb_elem_free(elem);
      break;
    }
    if (use_types != USE_TYPES_NONE) {
      break;
    }
  }
  DBG_EXEC(TYPESTATE,
      src_fingerprintdb_to_string(as1, sizeof as1, ret, false);
      printf("%s() %d: Printing src_fingerprintdb:\n%s\n", __func__, __LINE__, as1);
  );
}*/


src_fingerprintdb_elem_t **
superoptdb_get_itable_work(itable_t *itable,
    itable_position_t *ipos,// bool use_types,
    typestate_t *typestate_initial,
    typestate_t *typestate_final)
{
  NOT_IMPLEMENTED();
#if 0
  superoptdb_get_itable_work_retval *ret;
  src_fingerprintdb_elem_t **fdb;
  int dummy;

  /*if (use_types) {
    NOT_IMPLEMENTED();
  }*/

  ASSERT(rpc_client);
  ret = superoptdb_server_get_itable_work_1(&dummy, rpc_client);
  ASSERT(ret);
  ASSERT(ret->itable);
  if (!strcmp(ret->itable, "")) {
    return NULL;
  }
  snprintf(itable->id_string, sizeof itable->id_string, "superoptdb");
  *itable = itable_from_string(ret->itable);

  typestate_init_using_itable(typestate_initial, typestate_final, itable);
  //typestate_init_to_top(typestate_initial);
  //typestate_init_to_top(typestate_final);

  fdb = src_fingerprintdb_init();
  ASSERT(fdb);
  ASSERT(strcmp(ret->fingerprintdb, ""));
  src_fingerprintdb_add_from_string(fdb, ret->fingerprintdb, itable->use_types,
      typestate_initial, typestate_final);
  ASSERT(!src_fingerprintdb_is_empty(fdb));
  itable_position_from_string(ipos, ret->start_sequence);
  return fdb;
#endif
}

/*
static bool
i386_iseq_compute_fingerprint(i386_insn_t const *i386_iseq, long i386_iseq_len,
    regset_t const *live_out, long num_live_out, bool mem_live_out, uint64_t *fingerprint)
{
  //i386_insn_t i386_iseq_const[i386_iseq_len];
  transmap_t out_tmaps[num_live_out];
  //char assembly[40960];
  //regcons_t regcons;
  state_t outstate[NUM_FP_STATES];
  //regmap_t regmap;
  //uint64_t fp;
  //bool ret;
  long i;

  //i386_iseq_to_string(i386_iseq, i386_iseq_len, assembly, sizeof assembly);
  //ret = i386_infer_regcons_from_assembly(assembly, &regcons);
  //ASSERT(ret);
  //i386_iseq_copy(i386_iseq_const, i386_iseq, i386_iseq_len);
  //i386_iseq_rename(i386_iseq_const, i386_iseq_len, &regmap, &fp_imm_map,
  //    regmap_identity(), NULL, 0);

  alloc_exec_nextpcs(num_live_out);

  for (i = 0; i < num_live_out; i++) {
    transmap_copy(&out_tmaps[i], i386_transmap_default());
  }

  DBG3(EQUIV, "fp_state[0]:\n%s\n", state_to_string(&fp_state[0], as1, sizeof as1));
  for (i = 0; i < NUM_FP_STATES; i++) {
    state_init(&outstate[i]);
  }
  if (!i386_gen_out_states_using_transmap(i386_iseq, i386_iseq_len, fp_state,
      outstate, NUM_FP_STATES,
      exec_nextpcs, (uint8_t const **)exec_next_tcodes, i386_transmap_default(),
      out_tmaps, num_live_out)) {
    return false;
  }
  for (i = 0; i < NUM_FP_STATES; i++) {
    DBG3(EQUIV, "outstate[%ld]:\n%s\n", i, state_to_string(&outstate[i], as1, sizeof as1));
    fingerprint[i] = state_compute_fingerprint(&fp_state[i], &outstate[i], live_out,
        num_live_out, mem_live_out);
    state_free(&outstate[i]);
  }
  return true;
}
*/

bool
check_fingerprints(src_insn_t const *src_iseq, long src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    dst_insn_t const *dst_iseq, long dst_iseq_len, regset_t const *live_out,
    long num_live_out, bool mem_live_out, rand_states_t const &rand_states)
{
  typestate_t typestate_initial, typestate_final;
  regset_t dst_live_out[num_live_out];
  memslot_set_t dst_live_out_memslots[num_live_out];
  uint64_t dst_fp, src_fp;
  bool ret;
  long i;

  for (i = 0; i < num_live_out; i++) {
    regset_rename_using_transmap(&dst_live_out[i], &dst_live_out_memslots[i], &live_out[i], &out_tmaps[i]);
  }

  if (!src_iseq_supports_execution_test(src_iseq, src_iseq_len)) {
    DBG(EQUIV, "%s", "src_iseq does not support execution test.\n");
    return true;
  }
  if (!dst_iseq_supports_execution_test(dst_iseq, dst_iseq_len)) {
    DBG(EQUIV, "%s", "dst_iseq does not support execution test.\n");
    return true;
  }

  src_fingerprintdb_elem_t **fdb, **rows;
  long num_rows, max_alloc;
  char *ptr, *end, *buf;

  /*src_fp = src_iseq_compute_fingerprint(src_iseq, src_iseq_len, in_tmap, out_tmaps,
      live_out, num_live_out);
  dst_fp = dst_iseq_compute_fingerprint(dst_iseq, dst_iseq_len, dst_live_out,
      num_live_out);
  if (src_fp == dst_fp) {
    DBG(EQUIV, "fingerprint test passed: %lx <-> %lx\n", (long)src_fp, (long)dst_fp);
    return true;
  } else {
    DBG(EQUIV, "fingerprint test failed: %lx <-> %lx\n", (long)src_fp, (long)dst_fp);
    return false;
  }*/

  max_alloc = 409600;
  fdb = src_fingerprintdb_init();
  ASSERT(fdb);

  buf = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(buf);
  ptr = buf;
  end = ptr + max_alloc;
  ptr += src_harvested_iseq_to_string(ptr, end - ptr, src_iseq, src_iseq_len,
      live_out, num_live_out, mem_live_out);
  ptr += snprintf(ptr, end - ptr, "--\n");
  ASSERT(ptr < end);
  transmaps_to_string(in_tmap, out_tmaps, num_live_out, ptr, end - ptr);
  ptr += strlen(ptr);
  ASSERT(ptr < end);
  ptr += snprintf(ptr, end - ptr, "== 0 ==\n");
  ASSERT(ptr < end);

  src_fingerprintdb_add_from_string(fdb, buf, USE_TYPES_NONE, NULL, NULL, rand_states);
  ASSERT(!src_fingerprintdb_is_empty(fdb));

  /*rows = (fingerprintdb_elem_t **)malloc(FINGERPRINTDB_SIZE
      * sizeof(fingerprintdb_elem_t *));
  ASSERT(rows);*/
  ret = src_fingerprintdb_query(fdb, dst_iseq, dst_iseq_len, NULL);

  src_fingerprintdb_free(fdb);
  //free(rows);
  free(buf);
  //ret = num_rows != 0;
  DBG(EQUIV, "returning %s.\n", ret?"true":"false");
  return ret;
}

static void
output_equivalence_after_rationalizing(char const *filename,
    src_insn_t const *src_iseq, long src_iseq_len, transmap_t const *in_tmap,
    transmap_t const *out_tmaps, regset_t const *live_out, long num_live_out,
    bool mem_live_out, dst_insn_t const *dst_iseq,
    long dst_iseq_len)
{
  //transmap_t r_in_tmap, r_out_tmaps[num_live_out];
  dst_insn_t *r_dst_iseq;
  regmap_t inv_dst_regmap;
  peep_entry_t e;
  FILE *fp;
  int ret;

  //r_dst_iseq = (dst_insn_t *)malloc(dst_iseq_len * sizeof(dst_insn_t));
  r_dst_iseq = new dst_insn_t[dst_iseq_len];
  ASSERT(r_dst_iseq);
  dst_iseq_copy(r_dst_iseq, dst_iseq, dst_iseq_len);
  long r_dst_iseq_len = dst_iseq_optimize_branching_code(r_dst_iseq, dst_iseq_len, dst_iseq_len);
  //transmap_copy(&r_in_tmap, in_tmap);
  //transmaps_copy(r_out_tmaps, out_tmaps, num_live_out);
  //regmap_invert(&inv_dst_regmap, dst_regmap);
  //dst_regmap_add_identity_mappings_for_unmapped(&inv_dst_regmap);
  //transmaps_rename_using_dst_regmap(&r_in_tmap, r_out_tmaps, num_live_out,
  //    &inv_dst_regmap, NULL);
  //dst_iseq_copy(r_dst_iseq, dst_iseq, dst_iseq_len);
  //dst_iseq_rename_var(r_dst_iseq, dst_iseq_len, &inv_dst_regmap, NULL, NULL, NULL,
  //    NULL, NULL, 0);

  //memset(&e, 0, sizeof(peep_entry_t));
  e.src_iseq = (src_insn_t *)src_iseq;
  e.src_iseq_len = src_iseq_len;
  e.dst_iseq = r_dst_iseq/*(dst_insn_t *)dst_iseq*/;
  e.dst_iseq_len = r_dst_iseq_len;
  e.live_out = (regset_t *)live_out;
  e.num_live_outs = num_live_out;
  e.mem_live_out = mem_live_out;
  //e.in_tmap = (transmap_t *)/*&r_*/in_tmap;
  e.set_in_tmap((transmap_t *)/*&r_*/in_tmap);
  e.out_tmap = (transmap_t *)/*r_*/out_tmaps;

  //while (acquire_file_lock(filename) != 0);
  fp = fopen(filename, "a");
  ASSERT(fp);
  peeptab_entry_write(fp, &e, NULL);
  ret = fclose(fp);
  ASSERT(ret == 0);
  //release_file_lock(filename);
  //free(r_dst_iseq);
  delete [] r_dst_iseq;
}

static bool
check_fingerprintdb_rows(src_fingerprintdb_elem_t const * const *rows, long num_rows,
    dst_insn_t const *dst_iseq,
    long dst_iseq_len, long num_nextpcs, long nextpc_indir,
    uint64_t **fps,
    long const *num_fps, char const *equiv_output_filename,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    quota const &qt, size_t num_fp_states)
{
  src_fingerprintdb_elem_t const *cur;
  bool ret = false;
  long r, f;

  DBG2(EQUIV, "%s() called\n", __func__);
  DBG2(EQUIV, "dst_iseq:\n%s\nfingerprintdb_query returned %ld rows\n",
      dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1), num_rows);
  /*if (num_rows && dst_iseq_contains_nop(dst_iseq, dst_iseq_len)) {
    return false;
  }*/
  for (r = 0; r < num_rows; r++) {
    for (cur = rows[r]; cur; cur = cur->next) {
      bool fp_not_found = false;
     
      /*nextpc_indir = compute_nextpc_indir(cur->src_iseq, cur->src_iseq_len,
          cur->num_live_out);*/
      for (f = 0; f < num_fp_states; f++) {
        if (array_search(fps[f], num_fps[f], cur->fdb_fp[f]) == -1) {
          DBG2(EQUIV, "fp not found for f = %ld. num_fps[f] = %ld, fps[f][0]=%llx\n", f, num_fps[f], (unsigned long long)fps[f][0]);
          fp_not_found = true;
          break;
        }
      }
      if (fp_not_found) {
        continue;
      }
      if (!check_equivalence(cur->src_iseq, cur->src_iseq_len, &cur->in_tmap,
          cur->out_tmaps,
          dst_iseq, dst_iseq_len, cur->live_out,
          nextpc_indir,
          cur->num_live_out, cur->mem_live_out, dst_fixed_reg_mapping, 0, qt)) {
        DBG(EQUIV, "%s", "equivalence check failed.\n");
        continue;
      }
      DBG(EQUIV, "%s", "equivalence check passed!\n");
      ret = true;
      if (equiv_output_filename) {
        output_equivalence_after_rationalizing(equiv_output_filename,
            cur->src_iseq, cur->src_iseq_len,
            &cur->in_tmap, cur->out_tmaps, cur->live_out, cur->num_live_out,
            cur->mem_live_out/*, &cur->dst_regmap*/, dst_iseq, dst_iseq_len);
      }
      /*superoptdb_add_after_rationalizing(NULL, cur->src_iseq, cur->src_iseq_len,
          &cur->in_tmap, cur->out_tmaps,
          cur->live_out,
          cur->num_live_out, &cur->dst_regmap,
          dst_iseq, dst_iseq_len);*/
    }
  }
  return ret;
}

bool
src_fingerprintdb_query(src_fingerprintdb_elem_t **fingerprintdb,
    dst_insn_t const *dst_iseq, long dst_iseq_len,
    char const *equiv_output_filename)
{
  NOT_IMPLEMENTED();
/*
#define MAX_NUM_FPS 1024
  static fingerprintdb_elem_t const *rows[FINGERPRINTDB_SIZE];
  long num_nextpcs, nextpc_indir, i, n, r, num_fps;
  uint64_t fingerprint[NUM_FP_STATES];
  static uint64_t fps[MAX_NUM_FPS];
  fingerprintdb_elem_t *row, *cur;
  regset_t use, def, sub;
  bool mem_live_out;
  bool done;

  dst_iseq_get_usedef_regs(i386_iseq, i386_iseq_len, &use, &def);
  //regset_copy(&touch, &use);
  //regset_union(&touch, &def);

  num_nextpcs = i386_iseq_compute_num_nextpcs(i386_iseq, i386_iseq_len, &nextpc_indir);

  DBG3(EQUIV, "def:\n%s\n", regset_to_string(&def, as1, sizeof as1));

  num_fps = 0;

  for (mem_live_out = true;; mem_live_out = false) {
    for (i = 0, done = false, regset_subset_enumerate_first(&sub, &def);
        !done;
        done = regset_subset_enumerate_next(&sub, &def)) {
      DBG3(EQUIV, "sub:\n%s\n", regset_to_string(&sub, as1, sizeof as1));
      regset_t toucha[num_nextpcs];
      for (n = 0; n < num_nextpcs; n++) {
        regset_copy(&toucha[n], &sub);
      }
      if (!i386_iseq_compute_fingerprint(i386_iseq, i386_iseq_len, toucha,
          num_nextpcs, mem_live_out, fingerprint)) {
        return false;
      }
      DBG3(EQUIV, "Computed fingerprint %llx for\n%s\n", (long long)fingerprint[0],
          i386_iseq_to_string(i386_iseq, i386_iseq_len, as1, sizeof as1));
      rows[i] = fingerprintdb[fingerprint[0] % FINGERPRINTDB_SIZE];
      if (rows[i] && array_search(rows, i, rows[i]) == -1) {
        DBG(EQUIV, "Added row %p\n", rows[i]);
        ASSERT(i < FINGERPRINTDB_SIZE);
        i++;
      }
      if (array_search(fps, num_fps, fingerprint[0]) == -1) {
        fps[num_fps] = fingerprint[0];
        num_fps++;
      }
    }
  }

  return check_fingerprintdb_rows(rows, i, i386_iseq, i386_iseq_len, num_nextpcs,
      nextpc_indir, fps, num_fps, equiv_output_filename);
*/
}

/*long
typestate_check_using_bincode(uint8_t const *bincode, size_t binsize,
    typestate_t const *typestate)
{
  NOT_IMPLEMENTED();
}*/

static long
src_fingerprintdb_query_using_bincode_for_typestate(
    src_fingerprintdb_elem_t **fingerprintdb,
    uint8_t const *bincode, size_t binsize, itable_t const *itable,
    itable_position_t *ipos,
    long long const *max_costs,// bool use_types,
    typestate_t const *typestate_initial,
    typestate_t const *typestate_final, regset_t const *def,
    memslot_set_t const *memslot_def,
    src_fingerprintdb_elem_t const *rows[], long *num_rows, long max_num_rows,
    uint64_t *fps, long *num_fps, long max_num_fps, long fp_state_num,
    fingerprintdb_live_out_t const &src_fingerprintdb_live_out,
    rand_states_t const &rand_states)
{
  // if I see a type error, return the index of the instruction that failed typecheck
#define MAX_NUM_LIVE_OUT 32
  ASSERT(ipos);
  ASSERT(itable->get_num_nextpcs() <= MAX_NUM_LIVE_OUT);
  static memslot_set_t live_out_memslot[MAX_NUM_LIVE_OUT];
  static regset_t live_out[MAX_NUM_LIVE_OUT];
  static typestate_t out_typestate;
  src_fingerprintdb_elem_t const *cur;
  long i, type_mismatch_index;
  static memslot_set_t memslot_sub;
  static bool init = false;
  static state_t outstate;
  bool mem_live_out;
  static regset_t sub;
  uint64_t fp;
  bool done, done_memslots;

  stopwatch_run(&fingerprintdb_query_using_bincode_for_typestate_timer);

  if (!init) {
    state_init(&outstate);
    typestate_init(&out_typestate);
    init = true;
  }

  //DBG_EXEC(STATS, stats_num_fingerprinted_dst_sequences[ipos->sequence_len]++;);

  DBG3(JTABLE, "bincode: %s\n", hex_string(bincode, binsize, as1, sizeof as1));
  *num_fps = 0;
  *num_rows = 0;

  state_copy(&outstate, &rand_states.rand_states_get_state(fp_state_num));
  if (itable->get_typesystem().get_use_types() != USE_TYPES_NONE) {
    typestate_copy(&out_typestate, &typestate_initial[0]);
  }

  DBG4(JTABLE, "bincode at %p (len %ld):\n%s\n", bincode, binsize,
      hex_string(bincode, binsize, as1, sizeof as1));

  STATS(stats_num_fingerprint_tests[ipos->sequence_len]++;);

  DBG2(JTABLE, "executing ipos: %s\n",
      itable_position_to_string(ipos, as1, sizeof as1));
  DBG_EXEC2(JTABLE,
      static dst_insn_t dst_iseq[2048];
      long dst_iseq_len;
      dst_iseq_len = itable_construct_dst_iseq(dst_iseq, itable, ipos);
      printf("%s\n", dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1)); fflush(stdout););

  dst_execute_code_on_state_and_typestate(bincode, &outstate,
      (itable->get_typesystem().get_use_types() == USE_TYPES_NONE)?NULL:&out_typestate,
      (itable->get_typesystem().get_use_types() == USE_TYPES_NONE)?NULL:typestate_final,
      itable->get_fixed_reg_mapping());

  DBG2(JTABLE, "outstate.nextpc_num = %ld\n", (long)outstate.nextpc_num);

  if (outstate.nextpc_num == NEXTPC_TYPE_ERROR) {
    //ASSERT(itable->get_use_types() != USE_TYPES_NONE);
    //if there has to be a type mismatch, it should happen in the first iteration
    //ASSERT(regset_equals(&sub, def));
    /*return ipos ? itable_position_next(ipos, itable, max_costs,
        out_typestate.type_mismatching_insn_index) : 0;*/
    ASSERT(   outstate.last_executed_insn >= 0
           && outstate.last_executed_insn < ipos->sequence_len);
    DBG2(JTABLE, "typecheck failed at index %d for ipos: %s\nsub: %s\n",
        outstate.last_executed_insn,
        itable_position_to_string(ipos, as1, sizeof as1),
        regset_to_string(&sub, as2, sizeof as2));
    DBG_EXEC2(JTABLE,
        static dst_insn_t dst_iseq[2048];
        long dst_iseq_len;
        dst_iseq_len = itable_construct_dst_iseq(dst_iseq, itable, ipos);
        printf("%s\n", dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1)););

    STATS(stats_num_fingerprint_type_failures[ipos->sequence_len]
                  [outstate.last_executed_insn]++;);
    stopwatch_stop(&fingerprintdb_query_using_bincode_for_typestate_timer);
    ASSERT(*num_rows == 0);
    return outstate.last_executed_insn;
  }
  ASSERT(outstate.nextpc_num >= 0 && outstate.nextpc_num < itable->get_num_nextpcs());

  STATS(stats_num_fingerprint_typecheck_success[ipos->sequence_len]++;);
  DBG_EXEC(JTABLE,
      if ((stats_num_fingerprint_typecheck_success[ipos->sequence_len] % 100000) == 0) {
        printf("num typecheck succeeded at sequence len %ld {%ld,...}: %lld\n", ipos->sequence_len, ipos->sequence[0],
            stats_num_fingerprint_typecheck_success[ipos->sequence_len]);
        fflush(stdout);
      }
  );

  //if (ipos->sequence_len <= 2) {
    DBG2(JTABLE, "typecheck succeeded for ipos: %s\n",
        itable_position_to_string(ipos, as1, sizeof as1));
    DBG_EXEC2(JTABLE,
      static dst_insn_t dst_iseq[2048];
      long dst_iseq_len;
      dst_iseq_len = itable_construct_dst_iseq(dst_iseq, itable, ipos);
      printf("%s() %d: dst_iseq:\n%s\n", __func__, __LINE__,
          dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
    );
  //}

  /*DBG2(JTABLE, "typecheck succeeded for ipos: %s\nsub: %s\n",
      itable_position_to_string(ipos, as1, sizeof as1),
      regset_to_string(&sub, as2, sizeof as2));*/

  DBG3(JTABLE, "enumerating subsets for def: %s\n", regset_to_string(def, as1, sizeof as1));
  DBG3(JTABLE, "fingerprintdb_live_out: %s\n", src_fingerprintdb_live_out.fingerprintdb_live_out_to_string().c_str());
  mem_live_out = src_fingerprintdb_live_out.get_mem_live_out();
  /*for (mem_live_out = src_fingerprintdb_live_out.get_mem_live_max();; mem_live_out = false) */{
    stopwatch_run(&debug3_timer);
    for (done = false, regset_subset_enumerate_first(&sub, def, &src_fingerprintdb_live_out);
        !done;
        done = !regset_subset_enumerate_next(&sub, def, &src_fingerprintdb_live_out)) {
      /*if (!src_fingerprintdb_live_out.fingerprintdb_live_out_agrees_with_regset(sub)) {
        DBG3(JTABLE, "ignoring sub: %s because it does not agree with src_fingerprintdb_live_out\n",
            regset_to_string(&sub, as1, sizeof as1));
        continue;
      }*/
      for (i = 0; i < itable->get_num_nextpcs(); i++) {
        regset_copy(&live_out[i], &sub);
        live_out[i].regset_round_up_bitmaps_to_byte_size();
      }
      DBG3(JTABLE, "considering sub: %s : it agrees with src_fingerprintdb_live_out\n",
          regset_to_string(&sub, as1, sizeof as1));
      /*for (done_memslots = false, memslot_set_t::memslot_set_subset_enumerate_first(&memslot_sub, memslot_def, &src_fingerprintdb_live_out);
           !done_memslots;
           done_memslots = !memslot_set_t::memslot_set_subset_enumerate_next(&memslot_sub, memslot_def, &src_fingerprintdb_live_out))*/ {
        /*if (!src_fingerprintdb_live_out.fingerprintdb_live_out_agrees_with_memslot_set(memslot_sub)) {
          DBG3(JTABLE, "ignoring memslot_sub: %s because it does not agree with src_fingerprintdb_live_out\n",
              memslot_sub.memslot_set_to_string().c_str());
          continue;
        }*/
        memslot_sub = src_fingerprintdb_live_out.get_live_memslots();
        stopwatch_run(&debug2_timer);
        DBG3(JTABLE, "considering memslot_sub: %s it agrees with src_fingerprintdb_live_out\n",
            memslot_sub.memslot_set_to_string().c_str());
        DBG3(JTABLE, "*num_fps = %ld\n", (long int)*num_fps);
        DBG3(JTABLE, "def: %s\n", regset_to_string(def, as1, sizeof as1));
        DBG3(JTABLE, "sub: %s\n", regset_to_string(&sub, as1, sizeof as1));
        DBG3(JTABLE, "memslot_def: %s\n", memslot_def->memslot_set_to_string().c_str());
        DBG3(JTABLE, "memslot_sub: %s\n", memslot_sub.memslot_set_to_string().c_str());
        for (i = 0; i < itable->get_num_nextpcs(); i++) {
          live_out_memslot[i].memslot_set_copy(memslot_sub);
          live_out_memslot[i].memslot_set_round_up_bitmaps_to_byte_size();
        }
        fp = state_compute_fingerprint(&rand_states.rand_states_get_state(fp_state_num), &outstate, live_out, live_out_memslot, itable->get_num_nextpcs(), mem_live_out);
        DBG2(JTABLE, "outstate =\n%s\n", state_to_string(&outstate, as1, sizeof as1));
        DBG2(JTABLE, "fp = %llx\n", (long long)fp);
        DBG(EQUIV, "fp = %llx for fp_state_num %ld, mem_live_out %d, sub %s, memslot_sub %s\n", (long long)fp, fp_state_num, mem_live_out, regset_to_string(&sub, as1, sizeof as1), memslot_sub.memslot_set_to_string().c_str());
        ASSERT(fp != FINGERPRINT_TYPE_ERROR);
        if (fp != FINGERPRINT_TYPE_ERROR) {
          if (fp_state_num == 0) {
            ASSERT(*num_rows < max_num_rows);
            rows[*num_rows] = fingerprintdb[fp % FINGERPRINTDB_SIZE];
            if (rows[*num_rows] && array_search(rows, *num_rows, rows[*num_rows]) == -1) {
              DBG2(JTABLE, "Added row %p.\n", rows[*num_rows]);
              ASSERT(*num_rows < FINGERPRINTDB_SIZE);
              (*num_rows)++;
            }
          }
          if (array_search(fps, *num_fps, fp) == -1) {
            ASSERT(*num_fps < max_num_fps);
            fps[*num_fps] = fp;
            (*num_fps)++;
            ASSERT(*num_fps < MAX_NUM_FPS);
          }
        }
        stopwatch_stop(&debug2_timer);
      }
    }
    stopwatch_stop(&debug3_timer);
    /*if (!mem_live_out) {
      break;
    }*/
  }

  stopwatch_stop(&fingerprintdb_query_using_bincode_for_typestate_timer);
  return ipos->sequence_len - 1;
}

long
src_fingerprintdb_query_using_jtable_and_ipos(src_fingerprintdb_elem_t **fingerprintdb,
    jtable_t *jtable, itable_t const *itable,
    itable_position_t *ipos,
    uint8_t *binbuf, size_t max_binsize, //bincode and binbuf are only allocated by the caller, otherwise they are read/written only here.
    long long const *max_costs,// bool use_types,
    typestate_t const *typestate_initial,
    typestate_t const *typestate_final, regset_t const *def,
    memslot_set_t const *memslot_def, bool *found,
    char const *equiv_output_filename, rand_states_t const &rand_states,
    uint64_t **fps, long *num_fps,
    fingerprintdb_live_out_t const &src_fingerprintdb_live_out)
{
  static src_fingerprintdb_elem_t const *frows[FINGERPRINTDB_SIZE];
  static src_fingerprintdb_elem_t const *rows[FINGERPRINTDB_SIZE];
  long next, fnext, num_rows, num_frows, dst_iseq_len;
  //ASSERT(rand_states.get_num_states() <= NUM_FP_STATES);
  //static uint64_t **fps/*[NUM_FP_STATES][MAX_NUM_FPS]*/;
  //static long *num_fps/*[NUM_FP_STATES]*/;
  static dst_insn_t dst_iseq[2048];
  size_t binsize;
  int i, f;

  stopwatch_run(&fingerprintdb_query_using_bincode_timer);

  ASSERT(found);
  *found = false;

  f = 0;
  do {
    int regcons_unsatisfiable_index;

    binsize = jtable_get_executable_binary(binbuf, max_binsize, jtable, ipos, f);

    DBG3(JTABLE, "ipos:\n%s\n", itable_position_to_string(ipos, as1, sizeof as1));
    /*if (   ipos.sequence_len == 2
        && ipos.sequence[0] == 248
        && ipos.sequence[1] == 111) {
      DBG_SET(JTABLE, 3);
    } else {
      DBG_SET(JTABLE, 1);
    }*/
    DBG3(JTABLE, "fingerprinting using bincode:\n%s\n",
        hex_string(binbuf, binsize, as1, sizeof as1));
    DBG3(JTABLE, "rand_state[%ld] =\n%s\n", (long)f,
        state_to_string(&rand_states.rand_states_get_state(f), as1, sizeof as1));

    fnext = src_fingerprintdb_query_using_bincode_for_typestate(fingerprintdb,
        binbuf, binsize, itable, ipos, max_costs, &typestate_initial[f],
        &typestate_final[f],
        def, memslot_def,
        frows, &num_frows, FINGERPRINTDB_SIZE, fps[f], &num_fps[f], MAX_NUM_FPS, f,
        src_fingerprintdb_live_out,
        rand_states);

    CPP_DBG_EXEC3(JTABLE,
        cout << __func__ << " " << __LINE__ << ": fps[" << f << "] =";
	for (long i = 0; i < num_fps[f]; i++) {
	  cout << " " << hex << fps[f][i] << dec;
	}
	cout << endl;
    );

    if (f == 0) {
      num_rows = num_frows;
      memcpy(rows, frows, num_frows * sizeof(src_fingerprintdb_elem_t const *));
      next = fnext;
    } else {
      if (fnext != ipos->sequence_len - 1) {
        num_rows = 0;
      }
      //next = MIN(next, fnext);
      if (next > fnext) {
        next = fnext;
      }
    }

    ASSERT(num_rows == 0 || fnext == ipos->sequence_len - 1);
    DBG2(JTABLE, "ipos: %s, f = %d, fnext = %ld, num_frows = %ld, num_rows = %ld, num_fps[f] = %ld.\n", itable_position_to_string(ipos, as1, sizeof as1), f, fnext, num_frows, num_rows, num_fps[f]);

    if (num_rows == 0) {
      stopwatch_stop(&fingerprintdb_query_using_bincode_timer);
      return next;
    }
    ASSERT(fnext == ipos->sequence_len - 1);
    ASSERT(next == ipos->sequence_len - 1);

    imm_vt_map_t const &imm_vt_map = rand_states.rand_states_get_state(f).imm_vt_map;
    CPP_DBG_EXEC3(JTABLE,
        cout << "imm_vt_map for state[" << f << "] =\n" << imm_vt_map_to_string(&imm_vt_map, as1, sizeof as1) << endl;
    );
    regcons_unsatisfiable_index = jtable_construct_executable_binary(jtable, ipos,
        &imm_vt_map, src_fingerprintdb_live_out, 0, f);
    ASSERT(regcons_unsatisfiable_index == -1);
    f++;
  } while (f < rand_states.get_num_states());

  ASSERT(num_rows);
  ASSERT(next == ipos->sequence_len - 1);
  dst_iseq_len = itable_construct_dst_iseq(dst_iseq, itable, ipos);
  DBG2(JTABLE, "dst_iseq:\n%s\n",
      dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1));
  *found = check_fingerprintdb_rows(rows, num_rows, dst_iseq, dst_iseq_len,
      itable->get_num_nextpcs(), itable->get_nextpc_indir(), fps, num_fps,
      equiv_output_filename, itable->get_fixed_reg_mapping(), jtable->qt, rand_states.get_num_states());

  stopwatch_stop(&fingerprintdb_query_using_bincode_timer);
  return ipos->sequence_len - 1;
}

bool
iseqs_match_syntactically(src_insn_t const *src_iseq, long src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    dst_insn_t const *dst_iseq, long dst_iseq_len, regset_t const *live_out,
    long nextpc_indir, long num_live_out)
{
  regset_t use, def, touch, dst_use, dst_def, dst_touch, all_live_out;
  imm_vt_map_t src_imm_map, dst_imm_map;
  long dst_nextpc_indir, i;
  //regset_t use_renamed;

  if (dst_iseq_compute_num_nextpcs(dst_iseq, dst_iseq_len, &dst_nextpc_indir)
      != num_live_out) {
    DBG(EQUIV, "returning false because num_nextpcs don't match. "
        "num_live_out = %ld, dst_num_nextpcs = %ld\n", num_live_out,
        dst_iseq_compute_num_nextpcs(dst_iseq, dst_iseq_len, NULL));
    return false;
  }
  if (dst_nextpc_indir != nextpc_indir) {
    DBG(EQUIV, "returning false because nextpc_indir mismatch. "
        "nextpc_indir=%ld, dst_nextpc_indir=%ld\n", nextpc_indir,
        dst_nextpc_indir);
    return false;
  }
  dst_iseq_get_usedef_regs(dst_iseq, dst_iseq_len, &dst_use, &dst_def);

  regset_copy(&dst_touch, &dst_use);
  regset_union(&dst_touch, &dst_def);
  src_iseq_get_usedef_regs(src_iseq, src_iseq_len, &use, &def);
  regset_copy(&touch, &use);
  regset_union(&touch, &def);
  regset_clear(&all_live_out);

  /*for (i = 0; i < num_live_out; i++) {
    regset_t live_out_def, dst_live_out;
    regset_copy(&live_out_def, &live_out[i]);
    regset_intersect(&live_out_def, &def);
    regset_union(&all_live_out, &live_out[i]);

    regset_rename_using_transmap(&dst_live_out, &live_out_def, &out_tmaps[i]);
    if (!regset_contains(&dst_touch, &dst_live_out)) {
      DBG(EQUIV, "returning false because dst touch does not contain dst_live_out. "
          "dst_touch=%s, dst_live_out[%ld]=%s\n",
          regset_to_string(&i386_touch, as1, sizeof as1), i,
          regset_to_string(&i386_live_out, as2, sizeof as2));
      return false;
    }
  }*/
  //segs are special
#if ARCH_SRC == ARCH_I386
  regset_clear_exreg_group(&use, I386_EXREG_GROUP_SEGS);
#endif
#if ARCH_DST == ARCH_I386
  regset_clear_exreg_group(&dst_use, I386_EXREG_GROUP_SEGS); //segs are special
#endif

  //unsigned flags are sometimes used for signed comparisons (e.g., zero flag is used
  //for jg/jle
#if ARCH_SRC == ARCH_I386
  //regset_clear_exreg_group(&use, I386_EXREG_GROUP_EFLAGS_UNSIGNED);
#endif
#if ARCH_DST == ARCH_I386
  //regset_clear_exreg_group(&dst_use, I386_EXREG_GROUP_EFLAGS_UNSIGNED);
#endif

  regset_union(&use, &all_live_out);
  //regset_rename_using_transmap(&use_renamed, &use, in_tmap);
  /*if (!regset_contains(&use_renamed, &dst_use)) { //this may fail because imprecision in our liveness estimation, e.g., some bits in the dst iseq may be not-live but indicated as live; disable this check
    DBG(EQUIV, "returning false because use_renamed does not contain dst_use. "
        "use=%s, use_renamed=%s, dst_use=%s.\n",
        regset_to_string(&use, as3, sizeof as3),
        regset_to_string(&use_renamed, as1, sizeof as1),
        regset_to_string(&dst_use, as2, sizeof as2));
    return false;
  }*/
  src_iseq_mark_used_vconstants(&src_imm_map, NULL, NULL, src_iseq, src_iseq_len);
  dst_iseq_mark_used_vconstants(&dst_imm_map, NULL, NULL, dst_iseq, dst_iseq_len);
  if (!imm_vt_map_contains(&src_imm_map, &dst_imm_map)) {
    DBG(EQUIV, "returning false because vconstants do not match.\n"
        "src_imm_map:\n%s\ndst_imm_map:\n%s\n",
        imm_vt_map_to_string(&src_imm_map, as1, sizeof as1),
        imm_vt_map_to_string(&dst_imm_map, as2, sizeof as2));
    return false;
  }
  return true;
}

bool
check_equivalence(src_insn_t const src_iseq[], int src_iseq_len,
    transmap_t const *in_tmap, transmap_t const *out_tmaps,
    //i386_regcons_t const *transcons,
    dst_insn_t const dst_iseq[], int dst_iseq_len,
    regset_t const *live_out,
    int nextpc_indir, long num_live_out, bool mem_live_out,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    long max_unroll, quota const &qt)
{
  /*i386_regcons_t const *tcons;

  if (!transcons) {
    tcons = i386_regcons_all_set();
  } else {
    tcons = transcons;
  }*/
  /*if (!superoptimizer_supports(src_iseq, src_iseq_len)) {
    return true;
  }*/
  DBG_EXEC(EQUIV,
      printf("%s() %d: verifying:\n%s--\n", __func__, __LINE__,
          src_iseq_to_string(src_iseq, src_iseq_len, pbuf1, sizeof pbuf1));
      printf("live :%s\n--\n",
          ({regsets_to_string(live_out, num_live_out, pbuf1, sizeof pbuf1); pbuf1;}));
      printf("%s--\n",
          transmaps_to_string(in_tmap, out_tmaps, num_live_out, pbuf1, sizeof pbuf1));
      printf("%s==\n",
          dst_iseq_to_string(dst_iseq, dst_iseq_len, pbuf1, sizeof pbuf1));
  );

  DBG_EXEC(STATS, stats_num_syntactic_match_tests++;);

  ASSERT(!dst_iseq_contains_reloc_reference(dst_iseq, dst_iseq_len));
  if (   verify_syntactic_check
      && !iseqs_match_syntactically(src_iseq, src_iseq_len, in_tmap, out_tmaps,
              dst_iseq, dst_iseq_len, live_out, nextpc_indir, num_live_out)) {
    DBG2(EQUIV, "%s", "syntactic check failed.\n");
    return false;
  }
  //XXXX: check if i386 iseq's live in and live out match with src_iseq/transmap's
  if (verify_execution) {
    STATS(stats_num_execution_tests++;);
    if (!execution_check_equivalence(src_iseq, src_iseq_len, in_tmap, out_tmaps,
          dst_iseq, dst_iseq_len, live_out, num_live_out, mem_live_out,
          dst_fixed_reg_mapping)) {
      DBG2(EQUIV, "%s", "execution check failed.\n");
      return false;
    }
  }

  DBG_EXEC(BEQUIV,
      printf("%s() %d: verifying:\n%s--\n", __func__, __LINE__,
          src_iseq_to_string(src_iseq, src_iseq_len, pbuf1, sizeof pbuf1));
      printf("live :%s\n",
          ({regsets_to_string(live_out, num_live_out, pbuf1, sizeof pbuf1); pbuf1;}));
      printf("memlive : %s\n", mem_live_out ? "true" : "false");
      printf("--\n");
      printf("%s--\n", dst_fixed_reg_mapping.fixed_reg_mapping_to_string().c_str());
      printf("%s--\n",
          transmaps_to_string(in_tmap, out_tmaps, num_live_out, pbuf1, sizeof pbuf1));
      printf("%s==\n",
          dst_iseq_to_string(dst_iseq, dst_iseq_len, pbuf1, sizeof pbuf1));
      char ts[256];
      get_cur_timestamp(ts, sizeof ts);
      printf("cur time: %s.\n", ts);
  );
  /*if (verify_boolean) */{
    /*bool (*boolean_check_equivalence)(src_insn_t const *src_iseq, long src_iseq_len,
        struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
        struct i386_insn_t const *i386_iseq, long i386_iseq_len,
        struct regset_t const *live_out, int nextpc_indir,
        long num_live_out)
      = function_pointer_get("boolean_check_equivalence");
    ASSERT(boolean_check_equivalence);*/
    STATS(stats_num_boolean_tests++;);
    if (!boolean_check_equivalence(src_iseq, src_iseq_len, in_tmap, out_tmaps,
          dst_iseq, dst_iseq_len, live_out, nextpc_indir, num_live_out, mem_live_out,
          dst_fixed_reg_mapping,
          max_unroll, qt)) {
      DBG(BEQUIV, "%s", "boolean check failed.\n");
      DBG_EXEC2(BEQUIV,
          printf("%s() %d: candidate:\n%s--\n", __func__, __LINE__,
              src_iseq_to_string(src_iseq, src_iseq_len, pbuf1, sizeof pbuf1));
          printf("live :%s\n--\n",
              ({regsets_to_string(live_out, num_live_out, pbuf1, sizeof pbuf1); pbuf1;}));
          printf("%s--\n",
              transmaps_to_string(in_tmap, out_tmaps, num_live_out, pbuf1, sizeof pbuf1));
          printf("%s==\n",
              dst_iseq_to_string(dst_iseq, dst_iseq_len, pbuf1, sizeof pbuf1));
      );
      return false;
    }
    DBG(BEQUIV, "%s", "boolean check passed.\n");
  }
  DBG_EXEC(LEARN,
      printf("%s() %d: candidate:\n%s--\n", __func__, __LINE__,
          src_iseq_to_string(src_iseq, src_iseq_len, pbuf1, sizeof pbuf1));
      printf("live :%s\n--\n",
          ({regsets_to_string(live_out, num_live_out, pbuf1, sizeof pbuf1); pbuf1;}));
      printf("%s--\n",
          transmaps_to_string(in_tmap, out_tmaps, num_live_out, pbuf1, sizeof pbuf1));
      printf("%s==\n",
          dst_iseq_to_string(dst_iseq, dst_iseq_len, pbuf1, sizeof pbuf1));
  );
  return true;
}

static void
map_space(void *addr, size_t size)
{
  void *env;
  env = mmap(addr, size,
             PROT_EXEC | PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  ASSERT(env != (void *)-1);
  ASSERT(env == addr);
}

int
init_execution_environment(void)
{
  map_space((char *)SRC_ENV_ADDR - SRC_ENV_PREFIX_SPACE,
             SRC_ENV_SIZE + SRC_ENV_PREFIX_SPACE);

  //ASSERT(sizeof(typestate_t) <= TYPE_ENV_VALUE_SIZE);
  //map_space((char *)TYPE_ENV_VALUE_ADDR, TYPE_ENV_VALUE_SIZE);

  ASSERT(sizeof(typestate_t) <= TYPE_ENV_INITIAL_SIZE);
  map_space((char *)TYPE_ENV_INITIAL_ADDR, TYPE_ENV_INITIAL_SIZE);

  ASSERT(sizeof(typestate_t) <= TYPE_ENV_FINAL_SIZE);
  map_space((void *)TYPE_ENV_FINAL_ADDR, TYPE_ENV_FINAL_SIZE);

  ASSERT(sizeof(typestate_t) <= TYPE_ENV_SCRATCH_SIZE);
  map_space((void *)TYPE_ENV_SCRATCH_ADDR, TYPE_ENV_SCRATCH_SIZE);

  /* void *env;
  env = mmap((void *)SRC_ENV_ADDR - SRC_ENV_PREFIX_SPACE,
             SRC_ENV_SIZE + SRC_ENV_PREFIX_SPACE,
             PROT_EXEC | PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (env == (void *)-1) {
    return -1;
  }

  env = mmap((void *)TYPE_ENV_INITIAL_ADDR, TYPE_ENV_INITIAL_SIZE,
             PROT_EXEC | PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (env == (void *)-1) {
    return -1;
  }

  env = mmap((void *)TYPE_ENV_FINAL_ADDR, TYPE_ENV_FINAL_SIZE,
             PROT_EXEC | PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (env == (void *)-1) {
    return -1;
  }*/

  /*sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs, SIGINT);
  sigprocmask(SIG_BLOCK, &sigs, 0);*/

  return 0;
}

static void
dst_insn_rename_var_to_var_using_sequence(dst_insn_t *insn, long index,
    long const exreg_sequence[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN][MAX_REGS_PER_ITABLE_ENTRY],
    long const exreg_sequence_len[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN],
    int32_t const vconst_sequence[ENUM_MAX_LEN][MAX_VCONSTS_PER_ITABLE_ENTRY],
    long const vconst_sequence_len[ENUM_MAX_LEN],
    long const memloc_sequence[ENUM_MAX_LEN][MAX_MEMLOCS_PER_ITABLE_ENTRY],
    long const memloc_sequence_len[ENUM_MAX_LEN],
    long const nextpc_sequence[ENUM_MAX_LEN][MAX_NEXTPCS_PER_ITABLE_ENTRY],
    long const nextpc_sequence_len[ENUM_MAX_LEN],
    fixed_reg_mapping_t const &fixed_reg_mapping)
{
  if (dst_insn_is_nop(insn)) {
    return;
  }
#if ARCH_DST == ARCH_I386
  long i, reg, imm, val;
  operand_t *op;

  for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    op = &insn->op[i];
    if (op->type == op_reg) {
      ASSERT(op->valtag.reg.tag == tag_var);
      reg = op->valtag.reg.val;
      if (fixed_reg_mapping.var_reg_maps_to_fixed_reg(I386_EXREG_GROUP_GPRS, reg)) {
        continue;
      }
      if (!(reg >= 0 && reg < exreg_sequence_len[I386_EXREG_GROUP_GPRS][index])) {
        cout << __func__ << " " << __LINE__ << ": insn = " << dst_insn_to_string(insn, as1, sizeof as1) << endl;
        cout << __func__ << " " << __LINE__ << ": reg = " << reg << endl;
        cout << __func__ << " " << __LINE__ << ": index = " << index << endl;
        cout << __func__ << " " << __LINE__ << ": exreg_sequence_len[GPRS][index] = " << exreg_sequence_len[I386_EXREG_GROUP_GPRS][index] << endl;
      }
      ASSERT(reg >= 0 && reg < exreg_sequence_len[I386_EXREG_GROUP_GPRS][index]);
      ASSERT(exreg_sequence_len[I386_EXREG_GROUP_GPRS][index] <=
          MAX_REGS_PER_ITABLE_ENTRY);
      op->valtag.reg.val = exreg_sequence[I386_EXREG_GROUP_GPRS][index][reg];
      ASSERT(op->valtag.reg.val >= 0 && op->valtag.reg.val < I386_NUM_GPRS);
    } else if (   op->type == op_imm
               && op->valtag.imm.tag == tag_var) {
      imm = op->valtag.imm.val;
      ASSERT(imm >= 0 && imm < vconst_sequence_len[index]);
      ASSERT(vconst_sequence_len[index] <= MAX_VCONSTS_PER_ITABLE_ENTRY);
      op->valtag.imm.val = vconst_sequence[index][imm];
      ASSERT(op->valtag.imm.val >= 0 && op->valtag.imm.val < NUM_CANON_CONSTANTS);
    } else if (   op->type == op_imm
               && op->valtag.imm.tag == tag_imm_symbol) {
      imm = op->valtag.imm.val;
      ASSERT(imm >= 0 && imm < memloc_sequence_len[index]);
      ASSERT(memloc_sequence_len[index] <= MAX_MEMLOCS_PER_ITABLE_ENTRY);
      op->valtag.imm.val = memloc_sequence[index][imm];
      ASSERT(op->valtag.imm.val >= 0 && op->valtag.imm.val < NUM_CANON_CONSTANTS);
    } else if (op->type == op_mem) {
      if (op->valtag.mem.base.val != -1) {
        ASSERT(op->valtag.mem.base.tag == tag_var);
        ASSERT(op->valtag.mem.base.val >= 0);
        ASSERT(op->valtag.mem.base.val <
            exreg_sequence_len[I386_EXREG_GROUP_GPRS][index]);
        ASSERT(exreg_sequence_len[I386_EXREG_GROUP_GPRS][index] <=
            MAX_REGS_PER_ITABLE_ENTRY);
        val = exreg_sequence[I386_EXREG_GROUP_GPRS][index][op->valtag.mem.base.val];
        ASSERT(val >= 0 && val < I386_NUM_GPRS);
        op->valtag.mem.base.val = val;
      }
      if (op->valtag.mem.index.val != -1) {
        ASSERT(op->valtag.mem.index.tag == tag_var);
        ASSERT(op->valtag.mem.index.val >= 0);
        ASSERT(op->valtag.mem.index.val <
            exreg_sequence_len[I386_EXREG_GROUP_GPRS][index]);
        ASSERT(exreg_sequence_len[I386_EXREG_GROUP_GPRS][index] <=
            MAX_REGS_PER_ITABLE_ENTRY);
        val = exreg_sequence[I386_EXREG_GROUP_GPRS][index][op->valtag.mem.index.val];
        ASSERT(val >= 0 && val < I386_NUM_GPRS);
        op->valtag.mem.index.val = val;
      }
      if (op->valtag.mem.disp.tag == tag_var) {
        imm = op->valtag.mem.disp.val;
        ASSERT(imm >= 0 && imm < vconst_sequence_len[index]);
        ASSERT(vconst_sequence_len[index] <= MAX_VCONSTS_PER_ITABLE_ENTRY);
        op->valtag.mem.disp.val = vconst_sequence[index][imm];
        ASSERT(   op->valtag.mem.disp.val >= 0
               && op->valtag.mem.disp.val < NUM_CANON_CONSTANTS);
      } else if (op->valtag.mem.disp.tag == tag_imm_symbol) {
        imm = op->valtag.mem.disp.val;
        ASSERT(imm >= 0 && imm < memloc_sequence_len[index]);
        ASSERT(memloc_sequence_len[index] <= MAX_MEMLOCS_PER_ITABLE_ENTRY);
        op->valtag.mem.disp.val = memloc_sequence[index][imm];
        ASSERT(   op->valtag.mem.disp.val >= 0
               && op->valtag.mem.disp.val < NUM_CANON_CONSTANTS);
      } else ASSERT(op->valtag.mem.disp.tag == tag_const);
      op->valtag.mem.seg.sel.val = default_seg(op->valtag.mem.base.val);
    } else if (   op->type == op_pcrel
               && op->valtag.pcrel.tag == tag_var) {
      //ASSERT(op->valtag.pcrel.tag == tag_var);
      ASSERT(   op->valtag.pcrel.val >= 0
             && op->valtag.pcrel.val < nextpc_sequence_len[index]);
      val = nextpc_sequence[index][op->valtag.pcrel.val];
      op->valtag.pcrel.val = val;
    }
  }
#else
  NOT_IMPLEMENTED();
#endif
}

static void
dst_iseq_rename_var_to_var_using_sequence(dst_insn_t *iseq, long iseq_len, long index,
    long const exreg_sequence[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN][MAX_REGS_PER_ITABLE_ENTRY],
    long const exreg_sequence_len[I386_NUM_EXREG_GROUPS][ENUM_MAX_LEN],
    int32_t const vconst_sequence[ENUM_MAX_LEN][MAX_VCONSTS_PER_ITABLE_ENTRY],
    long const vconst_sequence_len[ENUM_MAX_LEN],
    long const memloc_sequence[ENUM_MAX_LEN][MAX_MEMLOCS_PER_ITABLE_ENTRY],
    long const memloc_sequence_len[ENUM_MAX_LEN],
    long const nextpc_sequence[ENUM_MAX_LEN][MAX_NEXTPCS_PER_ITABLE_ENTRY],
    long const nextpc_sequence_len[ENUM_MAX_LEN],
    fixed_reg_mapping_t const &fixed_reg_mapping)
{
  long n;
  for (n = 0; n < iseq_len; n++) {
    dst_insn_t *insn = &iseq[n];
    dst_insn_rename_var_to_var_using_sequence(insn, index, exreg_sequence, exreg_sequence_len, vconst_sequence, vconst_sequence_len, memloc_sequence, memloc_sequence_len, nextpc_sequence, nextpc_sequence_len, fixed_reg_mapping);
  }
}





long
itable_construct_dst_iseq(dst_insn_t *iseq, itable_t const *itable,
    itable_position_t const *ipos)
{
  long i, n, iseq_len;

  for (i = 0, iseq_len = 0; i < ipos->sequence_len; i++) {
    n = ipos->sequence[i];
    ASSERT(n >= 0 && n < itable->get_num_entries());
    dst_insn_vec_copy_to_arr(&iseq[iseq_len], itable->get_entry(n).get_iseq_ft());
    size_t itab_iseq_len = itable->get_entry(n).get_iseq_ft().size();
    CPP_DBG_EXEC4(JTABLE, cout << __func__ << " " << __LINE__ << ": i = " << i << ", n = " << n << ", iseq =\n" << dst_iseq_to_string(iseq, iseq_len + itab_iseq_len, as1, sizeof as1) << endl);
    dst_iseq_rename_var_to_var_using_sequence(&iseq[iseq_len], itab_iseq_len, i,
        ipos->exreg_sequence,
        ipos->exreg_sequence_len,
        ipos->vconst_sequence, ipos->vconst_sequence_len,
        ipos->memloc_sequence, ipos->memloc_sequence_len,
        ipos->nextpc_sequence,
        ipos->nextpc_sequence_len,
        itable->get_fixed_reg_mapping());
    iseq_len += itab_iseq_len;
  }
  n = dst_insn_put_jump_to_nextpc(&iseq[iseq_len], 1,
          itable_position_lim(ipos, nextpc_sequence, nextpc_sequence_len, i, 0,
              itable->get_num_nextpcs()) - 1);
  ASSERT(n == 1);
  iseq_len++;
  return iseq_len;
}


