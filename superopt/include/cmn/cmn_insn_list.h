#include "cmn.h"
#include "insn/src-insn.h"
#include "insn/dst-insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"
#include "support/src-defs.h"
#include "expr/expr.h"
#include "eq/parse_input_eq_file.h"
//#include "rewrite/memvt_list.h"
//#include "rewrite/memlabel_map.h"

#define MAX_CALL_ARGS MEMVT_LIST_MAX_ELEMS

/*static bool
cmn_insn_add_call_arg(cmn_insn_t *insn)
{
  ASSERT(insn->memtouch.n_elem >= 0);
  ASSERT(insn->memtouch.n_elem <= MEMVT_LIST_MAX_ELEMS);
  if (insn->memtouch.n_elem == MEMVT_LIST_MAX_ELEMS) {
    return false;
  }
  cmn_insn_add_memtouch_elem(insn, R_SS, R_ESP, -1,
      insn->memtouch.n_elem * (DWORD_LEN/BYTE_LEN), DWORD_LEN/BYTE_LEN, true);
  return true;
}

static bool
cmn_insn_add_memtouch(cmn_insn_t *dst, cmn_insn_t const *orig)
{
  if (cmn_insn_is_nop(dst)) {
    cmn_insn_copy(dst, orig);
    return true;
  }
  //if (cmn_insn_is_function_call(dst)) {
  //  if (cmn_insn_add_call_arg(dst)) {
  //    return true;
  //  }
  //}
  return false;
}*/

static cmn_insn_list_elem_t *
cmn_insn_add_nop_insn_to_list(cmn_insn_list_elem_t *ls)
{
  cmn_insn_list_elem_t *cur;
  cmn_insn_t cmn_insn_nop;

  cmn_insn_put_nop(&cmn_insn_nop, 1);
  //cur = (cmn_insn_list_elem_t *)malloc(sizeof(cmn_insn_list_elem_t));
  cur = new cmn_insn_list_elem_t;
  assert(cur);
  cmn_insn_copy(&cur->insn[0], &cmn_insn_nop);
  cmn_insn_put_jump_to_nextpc(&cur->insn[1], 1, 0);
  cur->num_insns = 2;
  cur->num_nextpcs = 1;
  cur->function_name = "nop";
  cur->insn_num_in_function = 0;
  DBG(SYM_EXEC, "cur->insn: %s\n",
      cmn_iseq_to_string(cur->insn, cur->num_insns, as1, sizeof as1));
  cur->next = ls;
  ls = cur;
  return ls;
}

/*static cmn_insn_list_elem_t *
cmn_insn_add_memtouch_variants_to_list(cmn_insn_list_elem_t *ls, cmn_insn_t *cmn_insn_var, long cmn_insn_var_len, regmap_t const *st_regmap, imm_vt_map_t const *st_imm_map, src_ulong const *nextpcs, long num_nextpcs)
{
  cmn_insn_t cmn_insn_with_memtouch;
  cmn_insn_list_elem_t *cur;

  assert(cmn_insn_var_len == 1);

  assert(!cmn_insn_is_nop(cmn_insn_var));
  cmn_insn_put_nop(&cmn_insn_with_memtouch, 1);

  while (cmn_insn_add_memtouch(&cmn_insn_with_memtouch, cmn_insn_var)) {
    int i;
    cur = (cmn_insn_list_elem_t *)malloc(sizeof(cmn_insn_list_elem_t));
    assert(cur);
    cmn_insn_copy(&cur->insn, &cmn_insn_with_memtouch);
    memvt_list_rename(&cur->insn.memtouch, st_regmap);
    ASSERT(num_nextpcs <= 2);
    cur->num_nextpcs = num_nextpcs;
    imm_vt_map_copy(&cur->st_imm_map, st_imm_map);
    regmap_copy(&cur->st_regmap, st_regmap);
    for (i = 0; i < num_nextpcs; i++) {
      cur->nextpcs[i] = nextpcs[i];
    }
    DBG(SYM_EXEC, "cur->insn: %s\n",
        cmn_iseq_to_string(&cur->insn, 1, as1, sizeof as1));
    cur->next = ls;
    ls = cur;
  }
  return ls;
}*/

bool
cmn_insn_exists_in_list(cmn_insn_list_elem_t const *ls, cmn_insn_t const *insns, long num_insns, nextpc_t const *nextpcs, long num_nextpcs, regmap_t const &st_regmap, imm_vt_map_t const &st_imm_map)
{
  return false; //which one to omit is tricky, because fbgen relies on the names of the functions in which the instruction appears to discard more instructions. just return false for now.
  for (cmn_insn_list_elem_t const *cur = ls; cur; cur = cur->next) {
    if (num_insns != cur->num_insns) {
      continue;
    }
    bool unequal = false;
    for (size_t i = 0; i < num_insns; i++) {
      if (!cmn_insns_equal(&insns[i], &cur->insn[i])) {
        unequal = true;
        break;
      }
    }
    if (unequal) {
      continue;
    }
    if (num_nextpcs != cur->num_nextpcs) {
      continue;
    }
    /*for (size_t i = 0; i < num_nextpcs; i++) { //XXX: not sure if this needs to be uncommented
      if (nextpcs[i] != cur->nextpcs[i]) {
        unequal = true;
        break;
      }
    }
    if (unequal) {
      continue;
    }
    if (!regmaps_equal(&st_regmap, &cur->st_regmap)) {
      continue;
    }
    if (!imm_vt_maps_equal(&st_imm_map, &cur->st_imm_map)) {
      continue;
    }*/
    DBG(REFRESH_DB, "%s() %d: returning true for iseq = %s\n", __func__, __LINE__, cmn_iseq_to_string(insns, num_insns, as1, sizeof as1));
    DBG(REFRESH_DB, "%s() %d: cur_iseq = %s\n", __func__, __LINE__, cmn_iseq_to_string(cur->insn, num_insns, as1, sizeof as1));
    return true;
  }
  return false;
}

cmn_insn_list_elem_t *
cmn_read_insn_list_from_file(char const *filename, bool (*accept_insn_check)(cmn_insn_t *), struct cmn_strict_canonicalization_state_t *scanon_state)
{
  cmn_insn_t cmn_insn[2], *cmn_insn_var[1];
  unsigned long insn_size;
  long start_pc, end_pc;
  //memlabel_map_t st_memlabel_map;
  cmn_insn_list_elem_t *ls;
  long cmn_insn_var_len[1];
  imm_vt_map_t st_imm_map;
  nextpc_t nextpcs[INSN_MAX_NEXTPC_COUNT];
  regmap_t st_regmap;
  cmn_ulong pcs[2];
  long num_nextpcs;
  input_exec_t *in;
  cmn_ulong pc;
  bool fetch;
  int ret;

  //in = (input_exec_t *)malloc(sizeof(input_exec_t));
  in = new input_exec_t;
#if (CMN==COMMON_SRC && ARCH_SRC==ARCH_ETFG)
  map<string, dshared_ptr<tfg>> function_tfg_map = get_function_tfg_map_from_etfg_file(filename, g_ctx);
  etfg_read_input_file(in, function_tfg_map);
#else
  read_input_file(in, filename);
#endif

  ls = NULL;

  get_text_section(in, &start_pc, &end_pc);
  fixed_reg_mapping_t fixed_reg_mapping;

  for (pc = start_pc; pc < end_pc; pc += insn_size) {
    int i, num_insns;
    fetch = cmn_insn_fetch_raw(cmn_insn, in, pc, &insn_size);
    ASSERT(fetch);
    if (accept_insn_check && !(*accept_insn_check)(cmn_insn)) {
      continue;
    }
    //cmn_insn_canonicalize(&cmn_insn);
    DBG(REFRESH_DB, "pc = %lx, fetch = %d, cmn_is_src = %d, ARCH_SRC = %d, cmn_insn:\n%s\n", (long)pc, fetch, cmn_is_src, ARCH_SRC,
        cmn_iseq_to_string(cmn_insn, 1, as1, sizeof as1));
    pcs[0] = pc;
    num_insns = 1;
    if (!cmn_insn_is_unconditional_branch_not_fcall(&cmn_insn[0])) {
      DBG(REFRESH_DB, "%s", "is_unconditional_branch_not_fcall()\n");
      long n = cmn_insn_put_jump_to_pcrel(pc + insn_size, &cmn_insn[1], 1);
      ASSERT(n == 1);
      ASSERT(n <= 1);
      num_insns += n;
      if (n == 1) {
        pcs[1] = 0;
      }
    }
    DBG(REFRESH_DB, "pc = %lx, after adding jump_to_nextpc, cmn_insn:\n%s\n", (long)pc,
        cmn_iseq_to_string(cmn_insn, num_insns, as1, sizeof as1));
    cmn_iseq_convert_pcs_to_inums(cmn_insn, pcs, num_insns, nextpcs, &num_nextpcs);
    ASSERT(num_nextpcs <= INSN_MAX_NEXTPC_COUNT);
    DBG(REFRESH_DB, "pc = %lx, after convert pcs to inums, num_nextpcs = %d, cmn_insn:\n%s\n", (long)pc, (int)num_nextpcs,
        cmn_iseq_to_string(cmn_insn, num_insns, as1, sizeof as1));
    //cmn_insn_var[0] = (cmn_insn_t *)malloc(num_insns * sizeof(cmn_insn_t));
    cmn_insn_var[0] = new cmn_insn_t[num_insns];
    ASSERT(cmn_insn_var[0]);
    ret = cmn_iseq_strict_canonicalize(scanon_state, cmn_insn, num_insns, cmn_insn_var,
        cmn_insn_var_len, &st_regmap, &st_imm_map, fixed_reg_mapping, nullptr, 1, true);
    ASSERT(ret == 1);
    ASSERT(cmn_insn_var_len[0] <= 2);
    if (cmn_insn_var_len[0] == 0) {
      DBG(REFRESH_DB, "ignoring %s because strict_canonicalize failed\n", cmn_iseq_to_string(cmn_insn, num_insns, as1, sizeof as1));
      //free(cmn_insn_var[0]);
      long n = cmn_insn_put_jump_to_nextpc(&cmn_insn[0], 1, 0);
      ASSERT(n == 1);
      cmn_insn_var_len[0] = n;
      //delete [] cmn_insn_var[0];
      //continue;
    }
    DBG(REFRESH_DB, "pc = %lx, after strict canonicalize, cmn_insn:\n%s\n", (long)pc,
        cmn_iseq_to_string(cmn_insn_var[0], cmn_insn_var_len[0], as1, sizeof as1));
    num_insns = cmn_insn_var_len[0];
    //ls = cmn_insn_add_memtouch_variants_to_list(ls, cmn_insn_var[0], cmn_insn_var_len[0], &st_regmap, &st_imm_map, nextpcs, num_nextpcs);
    //cmn_insn_list_elem_t *cur = (cmn_insn_list_elem_t *)malloc(sizeof(cmn_insn_list_elem_t));
    string function_name;
    long insn_num_in_function;
    cmn_pc_get_function_name_and_insn_num(in, pc, function_name, insn_num_in_function);
    if (   insn_num_in_function == 0
        || !cmn_insn_exists_in_list(ls, cmn_insn_var[0], num_insns, nextpcs, num_nextpcs, st_regmap, st_imm_map)) {
      DBG(REFRESH_DB, "%s", "adding iseq to insn_list\n");
      cmn_insn_list_elem_t *cur = new cmn_insn_list_elem_t;
      assert(cur);
      for (i = 0; i < num_insns; i++) {
        cmn_insn_copy(&cur->insn[i], &cmn_insn_var[0][i]);
      }
      cur->num_insns = num_insns;
      ASSERT(num_nextpcs);
      ASSERT(num_nextpcs <= 2);
      cur->num_nextpcs = num_nextpcs;
      imm_vt_map_copy(&cur->st_imm_map, &st_imm_map);
      regmap_copy(&cur->st_regmap, &st_regmap);
      for (i = 0; i < num_nextpcs; i++) {
        cur->nextpcs[i] = nextpcs[i];
      }
      cur->function_name = function_name;
      cur->insn_num_in_function = insn_num_in_function;
      DBG(REFRESH_DB, "cur->insn: %s\n",
          cmn_iseq_to_string(cur->insn, cur->num_insns, as1, sizeof as1));
      DBG(REFRESH_DB, "cur->num_nextpcs: %ld\n", cur->num_nextpcs);

      cur->next = ls;
      ls = cur;
    } else {
      DBG(REFRESH_DB, "%s", "ignored iseq. did not add it to insn_list\n");
    }

    //free(cmn_insn_var[0]);
    delete [] cmn_insn_var[0];
  }
  ls = cmn_insn_add_nop_insn_to_list(ls);

  input_exec_free(in);
  free(in);
  return ls;
}

void
cmn_insn_list_free(cmn_insn_list_elem_t *ls)
{
  cmn_insn_list_elem_t *cur, *next;

  for (cur = ls; cur; cur = next) {
    next = cur->next;
    //free(cur);
    delete cur;
  }
}
