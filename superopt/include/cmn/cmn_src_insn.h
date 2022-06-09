#include "support/c_utils.h"
#include "support/consts.h"
#include "valtag/regcons.h"
#include "rewrite/bbl.h"
#include "cmn/cmn.h"
//#include "rewrite/locals_info.h"
#include "support/globals.h"
//#include "rewrite/memlabel_map.h"
#include "valtag/fixed_reg_mapping.h"
#include "x64/insntypes.h"
#include "support/types.h"

void
cmn_iseq_convert_pcs_to_inums(cmn_insn_t *cmn_iseq, cmn_ulong const *pcs,
    long cmn_iseq_len, nextpc_t *nextpcs, long *num_nextpcs)
{
#if (CMN == COMMON_DST && ARCH_DST == ARCH_X64 && DST_LONG_BITS == 32)
  //ASSERT(sizeof(cmn_ulong) == 8);
#error "incorrect long size"
#endif


  //cout << __func__ << " " << __LINE__ << ": before: cmn_iseq =\n" << cmn_iseq_to_string(cmn_iseq, cmn_iseq_len, as1, sizeof as1) << endl;
  vector<valtag_t> pcrel_valtags[cmn_iseq_len];
  //uint64_t pcrel_vals[cmn_iseq_len];
  long index, i;

  if (num_nextpcs) {
    *num_nextpcs = 0;
  }
  for (i = 0; i < cmn_iseq_len; i++) {
    vector<valtag_t> vs = cmn_insn_get_pcrel_values(&cmn_iseq[i]);
    /*if (vs.size() == 0) {
      pcrel_vals[i] = 0;
      pcrel_tags[i] = tag_invalid;
    } else if (vs.size() == 1) {
      pcrel_vals[i] = vs.at(0).val;
      pcrel_tags[i] = vs.at(0).tag;
    } else {
      NOT_IMPLEMENTED();
    }*/
    pcrel_valtags[i] = vs;
    /*DBG2(SRC_ISEQ_FETCH, "pcs[%ld]=%x, pcrel_vals[%ld]=%lx\n", i, pcs[i], i, (long)pcrel_valtags[i].at(0).val);*/

    if (nextpcs && *num_nextpcs == 0 && cmn_insn_is_indirect_branch(&cmn_iseq[i])) {
      nextpcs[*num_nextpcs] = PC_JUMPTABLE;
      (*num_nextpcs)++;
    }
  }

  for (i = 0; i < cmn_iseq_len; i++) {
    vector<valtag_t> vs = cmn_insn_get_pcrel_values(&cmn_iseq[i]);
    for (size_t pv = 0; pv < pcrel_valtags[i].size(); pv++) {
      if (pcrel_valtags[i].at(pv).tag != tag_var) {
        continue;
      }
      ASSERT(pcrel_valtags[i].at(pv).val >= 0 && pcrel_valtags[i].at(pv).val < PC_NEXTPC_CONSTS_MAX_COUNT);
      if (nextpcs) {
        //cout << __func__ << " " << __LINE__ << ": i = " << i << ", pcrel_valtags[" << i << "].at(" << pv << ").val = " << pcrel_valtags[i].at(pv).val << endl;
        //cout << __func__ << " " << __LINE__ << ": *num_nextpcs = " << *num_nextpcs << endl;
        long n;
        for (n = 0; n < *num_nextpcs; n++) {
          //cout << __func__ << " " << __LINE__ << ": nextpcs[" << n << "] = " << hex << nextpcs[n] << dec << endl;
          if (nextpcs[n].get_val() == PC_NEXTPC_CONST(pcrel_valtags[i].at(pv).val)) {
            pcrel_valtags[i][pv].tag = tag_var;
            pcrel_valtags[i][pv].val = n;
            break;
          }
        }
        if (n == *num_nextpcs) {
          nextpcs[*num_nextpcs] = PC_NEXTPC_CONST(pcrel_valtags[i].at(pv).val);
          pcrel_valtags[i][pv].tag = tag_var;
          pcrel_valtags[i][pv].val = *num_nextpcs;
          (*num_nextpcs)++;
        }
      }
    }
  }

  //printf("%s() %d: cmn_iseq_len = %d\n", __func__, __LINE__, (int)cmn_iseq_len);
  for (i = 0; i < cmn_iseq_len; i++) {
    for (size_t pv = 0; pv < pcrel_valtags[i].size(); pv++) {
      if (   pcrel_valtags[i].at(pv).tag != tag_const
          && pcrel_valtags[i].at(pv).tag != tag_input_exec_reloc_index) {
        continue;
      }
      /*if (pcrel_valtags[i].at(pv).val == PC_JUMPTABLE) {
        continue;
      }*/
      if (pcrel_valtags[i].at(pv).tag == tag_const) {
        index = array_search(pcs, cmn_iseq_len, pcrel_valtags[i].at(pv).val);
        DBG2(SRC_ISEQ_FETCH, "%s() %d: pcrel_val = 0x%lx\n", __func__, __LINE__, pcrel_valtags[i].at(pv).val);
        DBG2(SRC_ISEQ_FETCH, "%s() %d: index = %ld\n", __func__, __LINE__, index);
        if (index != -1) {
          //pcrel_vals[i] = index - (i + 1);
          pcrel_valtags[i][pv].val = index - (i + 1);
          continue;
        }
      }
      DBG2(SRC_ISEQ_FETCH, "%s() %d: nextpcs = %p\n", __func__, __LINE__, nextpcs);
      if (!nextpcs) {
        continue;
      }
      index = nextpc_t::nextpc_array_search(nextpcs, *num_nextpcs, nextpc_t(pcrel_valtags[i].at(pv).tag, pcrel_valtags[i].at(pv).val));
      DBG2(SRC_ISEQ_FETCH, "%s() %d: pcrel_vals = 0x%lx\n", __func__, __LINE__, (long)pcrel_valtags[i].at(pv).val);
      DBG2(SRC_ISEQ_FETCH, "%s() %d: index = %ld\n", __func__, __LINE__, index);
      if (index == -1) {
        nextpcs[*num_nextpcs] = nextpc_t(pcrel_valtags[i].at(pv).tag, pcrel_valtags[i].at(pv).val);
        index = *num_nextpcs;
        (*num_nextpcs)++;
      }
      //pcrel_tags[i] = tag_var;
      //pcrel_vals[i] = index;
      pcrel_valtags[i][pv].tag = tag_var;
      pcrel_valtags[i][pv].val = index;
    }
  }
  for (i = 0; i < cmn_iseq_len; i++) {
    cmn_insn_set_pcrel_values(&cmn_iseq[i], pcrel_valtags[i]);
    /*if (pcrel_tags[i] != tag_invalid) {
      cmn_insn_set_pcrel_values(&cmn_iseq[i], pcrel_vals[i], pcrel_tags[i]);
    }*/
  }
  //cout << __func__ << " " << __LINE__ << ": after: cmn_iseq =\n" << cmn_iseq_to_string(cmn_iseq, cmn_iseq_len, as1, sizeof as1) << endl;
}

void
cmn_iseq_convert_pcrel_inums_to_abs_inums(cmn_insn_t *iseq, long iseq_len)
{
  //cout << __func__ << " " << __LINE__ << ": before: iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
  src_tag_t pcrel_tag;
  uint64_t pcrel_val;
  long i;

  for (i = 0; i < iseq_len; i++) {
    vector<valtag_t> vs = cmn_insn_get_pcrel_values(&iseq[i]);
    for (size_t vn = 0; vn < vs.size(); vn++) {
      pcrel_val = vs.at(vn).val;
      pcrel_tag = vs.at(vn).tag;
      if (pcrel_tag != tag_const) {
        continue;
      }
      /*if (pcrel_val == PC_JUMPTABLE) {
        continue;
      }*/
      pcrel_val += i + 1;
      ASSERT(pcrel_val >= 0 && pcrel_val <= iseq_len);
      pcrel_tag = tag_abs_inum;
      vs[vn].val = pcrel_val;
      vs[vn].tag = pcrel_tag;
    }
    cmn_insn_set_pcrel_values(&iseq[i], vs);
  }
  //cout << __func__ << " " << __LINE__ << ": after: iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
}

static void
cmn_iseq_rename_abs_inum_tags_using_map(cmn_insn_t *iseq, long iseq_len,
    long const *map, long map_size, src_tag_t abs_inum_tag)
{
  //cout << __func__ << " " << __LINE__ << ": before: iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
  for (long i = 0; i < iseq_len; i++) {
    vector<valtag_t> vs = cmn_insn_get_pcrel_values(&iseq[i]);
    for (size_t vn = 0; vn < vs.size(); vn++) {
      src_tag_t pcrel_tag = vs.at(vn).tag;
      uint64_t pcrel_val = vs.at(vn).val;
      if (pcrel_tag == abs_inum_tag) {
        if (pcrel_val < 0 || pcrel_val >= map_size) {
          cout << __func__ << " " << __LINE__ << ": pcrel_val = 0x" << hex << pcrel_val << dec << endl;
          cout << __func__ << " " << __LINE__ << ": map_size = 0x" << hex << map_size << dec << endl;
        }
        ASSERT(pcrel_val >= 0 && pcrel_val < map_size);
        while (map[pcrel_val] == -1) {
          pcrel_val++; //it is possible that pcrel_val was eliminated.
        }
        //ASSERT(map[pcrel_val] >= 0 && map[pcrel_val] <= iseq_len);
        pcrel_val = map[pcrel_val];
        vs[vn].val = pcrel_val;
        vs[vn].tag = pcrel_tag;
      }
    }
    cmn_insn_set_pcrel_values(&iseq[i], vs);
  }
  //cout << __func__ << " " << __LINE__ << ": after: iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
}

void
cmn_iseq_rename_abs_inums_using_map(cmn_insn_t *iseq, long iseq_len,
    long const *map, long map_size)
{
  cmn_iseq_rename_abs_inum_tags_using_map(iseq, iseq_len, map, map_size, tag_abs_inum);
}

void
cmn_iseq_rename_sboxed_abs_inums_using_map(cmn_insn_t *iseq, long iseq_len,
    long const *map, long map_size)
{
  cmn_iseq_rename_abs_inum_tags_using_map(iseq, iseq_len, map, map_size, tag_sboxed_abs_inum);
}

static void
cmn_iseq_convert_abs_inum_tags_to_pcrel_inums(cmn_insn_t *iseq, long iseq_len, src_tag_t abs_inum_tag, src_tag_t pcrel_tag, long const *binpc_map)
{
  //src_tag_t pcrel_tag;
  //uint64_t pcrel_val;
  //cout << __func__ << " " << __LINE__ << ": before: iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
  for (long i = 0; i < iseq_len; i++) {
    vector<valtag_t> vs = cmn_insn_get_pcrel_values(&iseq[i]);
    for (size_t vn = 0; vn < vs.size(); vn++) {
      uint64_t orig_pcrel_val = vs.at(vn).val;
      src_tag_t orig_pcrel_tag = vs.at(vn).tag;
      if (orig_pcrel_tag != abs_inum_tag) {
        continue;
      }
      ASSERT(binpc_map || (orig_pcrel_val >= 0 && orig_pcrel_val <= iseq_len));
      ASSERT(!binpc_map || binpc_map[i + 1] != -1);
      orig_pcrel_val -= binpc_map ? binpc_map[i + 1] : i + 1;
      orig_pcrel_tag = pcrel_tag;
      vs[vn].val = orig_pcrel_val;
      vs[vn].tag = orig_pcrel_tag;
    }
    cmn_insn_set_pcrel_values(&iseq[i], vs);
  }
  //cout << __func__ << " " << __LINE__ << ": after: iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
}

void
cmn_iseq_convert_abs_inums_to_pcrel_inums(cmn_insn_t *iseq, long iseq_len)
{
  cmn_iseq_convert_abs_inum_tags_to_pcrel_inums(iseq, iseq_len, tag_abs_inum, tag_const, NULL);
}

void
cmn_iseq_convert_sboxed_abs_inums_to_pcrel_inums(cmn_insn_t *iseq, long iseq_len)
{
  cmn_iseq_convert_abs_inum_tags_to_pcrel_inums(iseq, iseq_len, tag_sboxed_abs_inum, tag_const, NULL);
}

void
cmn_iseq_convert_sboxed_abs_inums_to_binpcrel_inums(cmn_insn_t *iseq, long iseq_len, long const *binpc_map/*should be size iseq_len+1*/)
{
  cmn_iseq_convert_abs_inum_tags_to_pcrel_inums(iseq, iseq_len, tag_sboxed_abs_inum, tag_binpcrel_inum, binpc_map);
}


long
cmn_iseq_eliminate_nops(cmn_insn_t *iseq, src_ulong* canon_insn_pcs, long iseq_len)
{
  //src_tag_t pcrel_tag;
  long map[iseq_len];
  //uint64_t pcrel_val;
  long i, j;

  //cout << __func__ << " " << __LINE__ << ": before: iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;

  /* Eliminate nop instructions. */
  j = 0;
  for (i = 0; i < iseq_len; i++) {
    map[i] = j;
    if (!cmn_insn_is_nop(&iseq[i])) {
      if (i != j) {
        cmn_insn_copy(&iseq[j], &iseq[i]);
        if (canon_insn_pcs) {
          canon_insn_pcs[j] = canon_insn_pcs[i];
        }
      }
      vector<valtag_t> vs = cmn_insn_get_pcrel_values(&iseq[j]);
      for (size_t vn = 0; vn < vs.size(); vn++) {
        uint64_t pcrel_val = vs.at(vn).val;
        src_tag_t pcrel_tag = vs.at(vn).tag;
        if (pcrel_tag == tag_const) {
          pcrel_tag = tag_abs_inum;
          pcrel_val += i + 1;
          vs[vn].val = pcrel_val;
          vs[vn].tag = pcrel_tag;
        }
      }
      cmn_insn_set_pcrel_values(&iseq[j], vs);
      j++;
    }
  }
  //cout << __func__ << " " << __LINE__ << ": iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
  cmn_iseq_rename_abs_inums_using_map(iseq, j, map, iseq_len);
  //cout << __func__ << " " << __LINE__ << ": iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
  cmn_iseq_convert_abs_inums_to_pcrel_inums(iseq, j);
  //cout << __func__ << " " << __LINE__ << ": iseq =\n" << cmn_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
  return j;
}

void
cmn_iseq_canonicalize_jump_targets(cmn_insn_t *iseq, long iseq_len)
{
  for (long i = 0; i < iseq_len; i++) {
    CPP_DBG_EXEC4(SRC_ISEQ_FETCH, cout << __func__ << " " << __LINE__ << ": i = " << i << ": " << cmn_iseq_to_string(&iseq[i], 1, as1, sizeof as1) << endl);
    vector<valtag_t> vs = cmn_insn_get_pcrel_values(&iseq[i]);
    for (size_t vn = 0; vn < vs.size(); vn++) {
      src_tag_t pcrel_tag = vs.at(vn).tag;
      uint64_t pcrel_val = vs.at(vn).val;
      CPP_DBG_EXEC4(SRC_ISEQ_FETCH, cout << __func__ << " " << __LINE__ << ": vn = " << vn << ": " << ({pcrel2str(pcrel_val, pcrel_tag, NULL, 0, as1, sizeof as1); as1;}) << endl);
      if (pcrel_tag == tag_const && pcrel_val != PC_JUMPTABLE) {
        long to_index = i + 1 + pcrel_val;
        src_tag_t to_pcrel_tag;
        uint64_t to_pcrel_val;
        ASSERT(to_index >= 0 && to_index < iseq_len);
        if (cmn_insn_is_unconditional_branch_not_fcall(&iseq[to_index])) {
          vector<valtag_t> to_vs = cmn_insn_get_pcrel_values(&iseq[to_index]);
          for (auto to_vt : to_vs) {
            if (to_vt.tag == tag_var) {
              vs[vn] = to_vt;
            }
          }
        }
      }
    }
    cmn_insn_set_pcrel_values(&iseq[i], vs);
    CPP_DBG_EXEC4(SRC_ISEQ_FETCH, cout << __func__ << " " << __LINE__ << ": i = " << i << ": after setting pcrel value, " << cmn_iseq_to_string(&iseq[i], 1, as1, sizeof as1) << endl);
  }
}

long
cmn_iseq_canonicalize(cmn_insn_t *iseq, src_ulong* canon_insn_pcs, long iseq_len)
{
  long i;
  for (i = 0; i < iseq_len; i++) {
    cmn_insn_canonicalize(&iseq[i]);
  }
  iseq_len = cmn_iseq_eliminate_nops(iseq, canon_insn_pcs, iseq_len);
  cmn_iseq_canonicalize_jump_targets(iseq, iseq_len);
  return iseq_len;
}


static long
cmn_insn_strict_canonicalize_helper(cmn_strict_canonicalization_state_t *scanon_state,
    long insn_index,
    cmn_insn_t *iseq_var[], long *iseq_var_len, regmap_t *st_regmap,
    imm_vt_map_t *st_imm_map, fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var, long max_num_iseq_var,
    long (*cmn_operand_strict_canonicalize_fn)(cmn_strict_canonicalization_state_t *, long, long,
        cmn_insn_t **, long *, regmap_t *, imm_vt_map_t *, fixed_reg_mapping_t const &, long, long))
{
  long i, num_implicit_ops;

  //ASSERT(!cmn_insn_is_nop(insn));
  ASSERT(num_iseq_var >= 1);
  //num_implicit_ops = cmn_insn_num_implicit_operands(&iseq_var[0][insn_index]);
  size_t num_operands = cmn_insn_get_max_num_imm_operands(&iseq_var[0][insn_index]);
  CPP_DBG_EXEC(STRICT_CANON, cout << __func__ << " " << __LINE__ << ": num_iseq_var = " << num_iseq_var << endl);
  CPP_DBG_EXEC(STRICT_CANON, cout << __func__ << " " << __LINE__ << ": iseq_var[0] = " << cmn_iseq_to_string(iseq_var[0], *iseq_var_len, as1, sizeof as1) << endl);
  CPP_DBG_EXEC(STRICT_CANON, cout << __func__ << " " << __LINE__ << ": num_operands = " << num_operands << endl);
  for (i = 0; i < num_operands; i++) {
    DBG2(STRICT_CANON, "canonicalizing operand %ld.\n", i);
    num_iseq_var = (*cmn_operand_strict_canonicalize_fn)(scanon_state, insn_index, i,
        iseq_var, iseq_var_len, st_regmap, st_imm_map, fixed_reg_mapping, num_iseq_var, max_num_iseq_var);
  }
  return num_iseq_var;
}

static long
cmn_iseq_strict_canonicalize_helper(cmn_strict_canonicalization_state_t *scanon_state,
    cmn_insn_t *iseq_var[], long *iseq_var_len,
    regmap_t *st_regmap, imm_vt_map_t *st_imm_map,// memlabel_map_t *memlabel_map,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var,
    long max_num_iseq_var,
    long (*cmn_operand_strict_canonicalize_fn)(cmn_strict_canonicalization_state_t *, long, long,
        cmn_insn_t **, long *, regmap_t *, imm_vt_map_t *, fixed_reg_mapping_t const &, long, long)/*,
    long (*cmn_memtouch_strict_canonicalize_fn)(long, long,
        cmn_insn_t **, long *, memlabel_map_t *, long, long)*/)
{
  cmn_insn_t insn[max_num_iseq_var];
  long i;

  ASSERT(num_iseq_var >= 1);
  for (i = 0; i < iseq_var_len[0]; i++) {
    DBG(PEEP_TAB, "canonicalizing insn %ld.\n", i);
    num_iseq_var = cmn_insn_strict_canonicalize_helper(scanon_state, i,
        iseq_var, iseq_var_len,
        st_regmap, st_imm_map, fixed_reg_mapping, num_iseq_var, max_num_iseq_var,
        cmn_operand_strict_canonicalize_fn);
  }
  /*for (i = 0; i < num_iseq_var; i++) {
    iseq_var_len[i] = iseq_len;
  }*/
  return num_iseq_var;
}

long
cmn_iseq_strict_canonicalize_regs(cmn_strict_canonicalization_state_t *scanon_state,
    cmn_insn_t *iseq, long iseq_len,
    regmap_t *st_regmap)
{
  cmn_insn_t *iseq_var[1];
  long iseq_var_len, ret;
  imm_vt_map_t l_st_imm_map;
  regmap_t l_st_regmap;

  if (!st_regmap) {
    st_regmap = &l_st_regmap;
  }

  iseq_len = cmn_iseq_canonicalize(iseq, nullptr, iseq_len);

  //iseq_var[0] = (cmn_insn_t *)malloc(iseq_len * sizeof(cmn_insn_t));
  iseq_var[0] = new cmn_insn_t[iseq_len];
  ASSERT(iseq_var[0]);
  cmn_iseq_copy(iseq_var[0], iseq, iseq_len);
  iseq_var_len = iseq_len;
  regmap_init(&st_regmap[0]);
  fixed_reg_mapping_t fixed_reg_mapping;

  ret = cmn_iseq_strict_canonicalize_helper(scanon_state, iseq_var, &iseq_var_len,
      st_regmap, &l_st_imm_map, fixed_reg_mapping, 1, 1, cmn_operand_strict_canonicalize_regs/*, NULL*/);
  ASSERT(ret == 1);
  cmn_iseq_copy(iseq, iseq_var[0], iseq_var_len);
  //free(iseq_var[0]);
  delete [] iseq_var[0];

  return iseq_var_len;
}

long
cmn_iseq_strict_canonicalize(struct cmn_strict_canonicalization_state_t *scanon_state,
    cmn_insn_t const *iseq, long const iseq_len,
    cmn_insn_t *iseq_var[], long *iseq_var_len, regmap_t *st_regmap,
    imm_vt_map_t *st_imm_map, fixed_reg_mapping_t const &fixed_reg_mapping,
    src_ulong *canon_insn_pcs,
    long max_num_iseq_vars, bool canonicalize_imms)
{
  imm_vt_map_t l_st_imm_map[max_num_iseq_vars];
  regmap_t l_st_regmap[max_num_iseq_vars];
  long i, num_iseq_vars;

  if (!st_regmap) {
    st_regmap = l_st_regmap;
  }
  if (!st_imm_map) {
    st_imm_map = l_st_imm_map;
  }

  ASSERT(max_num_iseq_vars >= 1);
  cmn_iseq_copy(iseq_var[0], iseq, iseq_len);
  iseq_var_len[0] = cmn_iseq_canonicalize(iseq_var[0], canon_insn_pcs, iseq_len);
  imm_vt_map_init(&st_imm_map[0]);
  regmap_init(&st_regmap[0]);
  num_iseq_vars = 1;

  num_iseq_vars = cmn_iseq_strict_canonicalize_helper(scanon_state, iseq_var,
      iseq_var_len, st_regmap, st_imm_map, fixed_reg_mapping,
      num_iseq_vars, max_num_iseq_vars,
      cmn_operand_strict_canonicalize_regs);
  ASSERT(num_iseq_vars == 1);
  DBG(HARVEST, "before canonicalizing imms, iseq_var[0]:\n%s\n",
      cmn_iseq_to_string(iseq_var[0], iseq_var_len[0], as1, sizeof as1));
  DBG(HARVEST, "st_imm_map[0]:\n%s\n",
      imm_vt_map_to_string(&st_imm_map[0], as1, sizeof as1));
  if (!canonicalize_imms) {
    return num_iseq_vars;
  }

  num_iseq_vars = cmn_iseq_strict_canonicalize_helper(scanon_state, iseq_var,
      iseq_var_len, st_regmap, st_imm_map, fixed_reg_mapping,
      num_iseq_vars, max_num_iseq_vars,
      cmn_operand_strict_canonicalize_imms/*, NULL*/);
  DBG_EXEC(HARVEST,
    long i;
    printf("%s() %d: num_iseq_vars = %ld.\n", __func__, __LINE__, num_iseq_vars);
    for (i = 0; i < num_iseq_vars; i++) {
      printf("after canonicalizing imms, iseq_var[%ld]:\n%s\n", i,
          cmn_iseq_to_string(iseq_var[i], iseq_var_len[i], as1, sizeof as1));
      printf("st_imm_map[i]:\n%s\n", imm_vt_map_to_string(&st_imm_map[i], as1, sizeof as1));
    }
  );
  for (i = 1; i < num_iseq_vars; i++) {
    regmap_copy(&st_regmap[i], &st_regmap[0]);
  }
  return num_iseq_vars;
}


static bool
cmn_insn_inv_rename(cmn_insn_t *insn, regmap_t const *regmap,
    vconstants_t const *vconstants)
{
  if (cmn_insn_is_nop(insn)) {
    return true;
  }

  cmn_insn_inv_rename_registers(insn, regmap);
  if (!cmn_insn_inv_rename_memory_operands(insn, regmap, vconstants)) {
    DBG(PEEP_TAB, "returning %s\n", "false");
    NOT_REACHED();
    return false;
  }
  cmn_insn_inv_rename_constants(insn, vconstants);
  return true;
}

static bool
cmn_iseq_inv_rename(cmn_insn_t *iseq, int iseq_len, regmap_t const *regmap,
    vconstants_t *vconstants[])
{
  autostop_timer func_timer(__func__);
  int i;
  for (i = 0; i < iseq_len; i++) {
    if (!cmn_insn_inv_rename(&iseq[i], regmap, vconstants[i])) {
      DBG(I386_READ, "returning false for instruction %d\n", i);
      NOT_REACHED();
      return false;
    }
  }
  return true;
}

/*static void
read_memtouch(char const *line, memvt_list_t *mls)
{
  char const *pipe_start, *pipe_end;

  pipe_start = strchr(line, '|');
  ASSERT(pipe_start);
  pipe_end = strchr(pipe_start + 1, '|');
  ASSERT(pipe_end);

  memvt_list_from_string(mls, pipe_start + 1, pipe_end - pipe_start - 1);
}

static void
cmn_iseq_fill_memtouchs(cmn_insn_t *iseq, memvt_list_t const *memtouchs, long iseq_len)
{
  long i;
  for (i = 0; i < iseq_len; i++) {
    memvt_list_copy(&iseq[i].memtouch, &memtouchs[i]);
  }
}*/

static void
vconstants_alloc(vconstants_t *vconstants[], size_t num_insns, size_t num_vconstants_per_insn)
{
  autostop_timer func_timer(__func__);
  for (size_t i = 0; i < num_insns; i++) {
    vconstants[i] = new vconstants_t[num_vconstants_per_insn];
  }
}

static void
vconstants_free(vconstants_t *vconstants[], size_t num_insns)
{
  for (size_t i = 0; i < num_insns; i++) {
    delete [] vconstants[i];
  }
}

#if (!((CMN==COMMON_SRC) && (ARCH_SRC==ARCH_ETFG)))
long
cmn_iseq_from_string(translation_instance_t *ti, cmn_insn_t *cmn_iseq,
    size_t cmn_iseq_size, char const *assembly, i386_as_mode_t i386_as_mode)
{
  autostop_timer func_timer(__func__);
//#define MAX_NUM_INSNS 4096
  size_t max_num_insns = min(cmn_iseq_size, count_occurrences(assembly, '\n') + count_occurrences(assembly, ';'));
  vconstants_t *vconstants[max_num_insns];
  char *line = new char[MAX_LINE_SIZE];
  ASSERT(line);
  char *cmn_assembly, *cmn_ptr, *cmn_end;
  long cmn_iseq_len, binlen;
  char const *newline, *ptr;
  regcons_t regcons;
  uint8_t *bincode;
  regmap_t regmap;
  long max_alloc;
  bool ret;
  long i;

  vconstants_alloc(vconstants, max_num_insns, MAX_NUM_VCONSTANTS_PER_INSN);

  if (!strcmp(assembly, "")) {
    delete [] line;
    vconstants_free(vconstants, max_num_insns);
    return 0;
  }

  max_alloc = 409600;
  cmn_assembly = new char[max_alloc];
  ASSERT(cmn_assembly);
  size_t bincode_size = max_alloc/10;
  bincode = (uint8_t *)malloc((bincode_size) * sizeof(uint8_t));
  ASSERT(bincode);
  //memtouchs = (memvt_list_t *)malloc(MAX_NUM_INSNS * sizeof(memvt_list_t));
  //ASSERT(memtouchs);

  cmn_ptr = cmn_assembly;  cmn_end = cmn_ptr + max_alloc;

  DBG(CMN_ISEQ_FROM_STRING, "assembly:\n%s\n", assembly);
  DBG(CMN_ISEQ_FROM_STRING, "i386_as_mode: %d\n", i386_as_mode);

  if (!cmn_infer_regcons_from_assembly(assembly, &regcons)) {
    DBG(CMN_ISEQ_FROM_STRING, "%s", "returning -1\n");
    delete[] cmn_assembly;
    free(bincode);
    //free(memtouchs);
    //NOT_REACHED();
    delete[] line;
    vconstants_free(vconstants, max_num_insns);
    return -1;
  }

  DBG(CMN_ISEQ_FROM_STRING, "assembly:\n%s\ninferred regcons:\n%s\n",
      assembly, regcons_to_string(&regcons, as1, sizeof as1));

  regmap_init(&regmap);
  if (!regmap_assign_using_regcons(&regmap, &regcons, CMN_ARCH, NULL, fixed_reg_mapping_t())) {
    DBG(CMN_ISEQ_FROM_STRING, "%s", "returning -1\n");
    delete[] cmn_assembly;
    free(bincode);
    //free(memtouchs);
    delete [] line;
    vconstants_free(vconstants, max_num_insns);
    return -1;
  }

  DBG(CMN_ISEQ_FROM_STRING, "regmap:\n%s\n", regmap_to_string(&regmap, as1, sizeof as1));

  ptr = assembly;
  *cmn_ptr = '\0';
  i = 0;
  while ((newline = strchr(ptr, ';')) || (newline = strchr(ptr, '\n'))) {
    int preprocess;
    ASSERT(newline - ptr < MAX_LINE_SIZE);
    strncpy(line, ptr, newline - ptr);
    line[newline - ptr] = '\0';
    ASSERT(i < max_num_insns);
    DBG(CMN_ISEQ_FROM_STRING, "preprocessing line: '%s'\n", line);
    /*read_memtouch(line, &memtouchs[i]);
    DBG(CMN_ISEQ_FROM_STRING, "line%d: memtouch: %s\n", (int)i,
        memvt_list_to_string(&memtouchs[i], NULL, as1, sizeof as1));*/
    preprocess = peep_preprocess_using_regmap(ti, line, &regmap, vconstants[i],
        &cmn_exregname_suffix);
    if (!preprocess) {
      DBG(CMN_ISEQ_FROM_STRING, "%s", "returning -1\n");
      delete[] cmn_assembly;
      free(bincode);
      delete [] line;
      vconstants_free(vconstants, max_num_insns);
      //free(memtouchs);
      //NOT_REACHED();
      return -1;
    }
    cmn_ptr += snprintf(cmn_ptr, cmn_end - cmn_ptr, "%s\n", line);
    ASSERT(cmn_ptr < cmn_end);
    ptr = newline + 1;
    while (*ptr == ' ') {
      ptr++;
    }
    i++;
  }

  DBG(CMN_ISEQ_FROM_STRING, "Calling x86_assemble on:\n'%s'\n", cmn_assembly);
  DBG(CMN_ISEQ_FROM_STRING, "regmap:\n%s\n", regmap_to_string(&regmap, as1, sizeof as1));
  DBG(CMN_ISEQ_FROM_STRING, "i386_as_mode: %d\n", i386_as_mode);
  binlen = cmn_assemble(ti, bincode, bincode_size, cmn_assembly, i386_as_mode);
  ASSERT(binlen < bincode_size);
  if (!binlen) {
    //printf("cmn_assembly:\n%s\n", cmn_assembly);
    DBG(CMN_ISEQ_FROM_STRING, "returning -1, cmn_assembly=%s.\n", cmn_assembly);
    delete[] cmn_assembly;
    free(bincode);
    delete [] line;
    vconstants_free(vconstants, max_num_insns);
    //free(memtouchs);
    //NOT_REACHED();
    return -1;
  }
  /*DBG(CMN_ISEQ_FROM_STRING, "cmn_assemble(%s) returned %ld: %s\n", cmn_assembly, binlen,
      ({
       static char buf[4096], *ptr = buf, *end = buf + sizeof buf;
       int i;
       for (i = 0; i < binlen; i++) {
         ptr += snprintf(ptr, end - ptr, " %hhx", bincode[i]);
       }
       buf;
       }));*/
  ASSERT(binlen && binlen <= bincode_size);

  /*DBG2(CMN_ISEQ_FROM_STRING, "bincode for %s = %s\n", cmn_assembly,
      hex_string(bincode, binlen, as1, sizeof as1));*/
  cmn_iseq_len = cmn_iseq_disassemble(cmn_iseq, bincode, binlen, vconstants,
      NULL, NULL, i386_as_mode);
  ASSERT(cmn_iseq_len > 0);
  ASSERT(cmn_iseq_len <= max_num_insns);

  DBG(CMN_ISEQ_FROM_STRING, "disassembled:\n%s\nregmap:\n%s\n",
     cmn_iseq_to_string(cmn_iseq, cmn_iseq_len, as1, sizeof as1),
     regmap_to_string(&regmap, as2, sizeof as2));
  if (!cmn_iseq_inv_rename(cmn_iseq, cmn_iseq_len, &regmap, vconstants)) {
    DBG(CMN_ISEQ_FROM_STRING, "%s", "returning -1\n");
    delete[] cmn_assembly;
    free(bincode);
    delete [] line;
    vconstants_free(vconstants, max_num_insns);
    //free(memtouchs);
    //NOT_REACHED();
    return -1;
  }
  //cmn_iseq_fill_memtouchs(cmn_iseq, memtouchs, cmn_iseq_len);
  DBG(CMN_ISEQ_FROM_STRING, "returning:\n%s", cmn_iseq_to_string(cmn_iseq, cmn_iseq_len, as1, sizeof as1));
  delete[] cmn_assembly;
  free(bincode);
  delete []  line;
  vconstants_free(vconstants, max_num_insns);
  //free(memtouchs);
  DBG(CMN_ISEQ_FROM_STRING, "returning: %ld\n", cmn_iseq_len);
  return cmn_iseq_len;
}
#endif

int
cmn_iseq_disassemble_with_bin_offsets(cmn_insn_t *cmn_iseq,
    uint8_t *bincode, int binsize,
    vconstants_t *vconstants[], nextpc_t *nextpcs,
    long *num_nextpcs, cmn_ulong *bin_offsets,
    i386_as_mode_t i386_as_mode)
{
  autostop_timer func_timer(__func__);
  cmn_insn_t *cmn_insn;
  uint8_t *binptr;
  long i, j;

  /* printf("bincode:");
  for (i = 0; i < binsize; i++) {
    printf(" %hhx", bincode[i]);
  }
  printf("\n"); */
  for (binptr = bincode, cmn_insn = &cmn_iseq[0], i = 0;
       binptr - bincode < binsize;
       i++, cmn_insn = &cmn_iseq[i]) {
    int size;
    ASSERT(i < binsize);
    bin_offsets[i] = binptr - bincode;
    //size = disas_insn_cmn(binptr, (unsigned long)binptr, cmn_insn, i386_as_mode);
    //cmn_insn_add_implicit_operands(cmn_insn);
    size = cmn_insn_disassemble(cmn_insn, binptr, i386_as_mode);
    ASSERT(size > 0);
    DYN_DBG(i386_disas, "Disassembling %p: %hhx %hhx %hhx %hhx %hhx %hhx. size=%d\n",
        binptr, binptr[0], binptr[1], binptr[2], binptr[3], binptr[4],
        binptr[5], size);
    binptr += size;
    valtag_t *pcrel;
    if ((pcrel = cmn_insn_get_pcrel_operands(cmn_insn))) {
      //printf("vconstants[%ld][0].valid=%d, val=%d, pcrel->pcrel.val=%ld, tag=%ld\n", i, vconstants[i][0].valid, vconstants[i][0].val, (long)pcrel->val.pcrel, (long)pcrel->tag.pcrel);
      if (vconstants && vconstants[i][0].valid) {
        pcrel->tag = vconstants[i][0].tag;
        pcrel->val = vconstants[i][0].val;
      } else {
        cmn_insn_add_to_pcrels(cmn_insn, binptr - bincode, NULL, -1);
      }
    }
  }
  cmn_iseq_convert_pcs_to_inums(cmn_iseq, bin_offsets, i, nextpcs, num_nextpcs);
  ASSERT (binptr - bincode == binsize);
  return i;
}

int
cmn_iseq_disassemble(cmn_insn_t *cmn_iseq, uint8_t *bincode, int binsize,
    vconstants_t *vconstants[], nextpc_t *nextpcs,
    long *num_nextpcs, i386_as_mode_t i386_as_mode)
{
  cmn_ulong bin_offsets[binsize];
  return cmn_iseq_disassemble_with_bin_offsets(cmn_iseq, bincode, binsize, vconstants, nextpcs, num_nextpcs, bin_offsets, i386_as_mode);
}

long
fscanf_harvest_output_cmn(FILE *fp, cmn_insn_t *cmn_iseq, size_t cmn_iseq_size,
    long *cmn_iseq_len, regset_t *live_out, size_t live_out_size,
    size_t *num_live_out, bool *mem_live_out, long linenum)
{
  int sa, num_read, i;
  char *cmn_assembly;
  size_t max_alloc;

  max_alloc = 409600;
  cmn_assembly = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(cmn_assembly);

  //printf("calling fscanf_entry_section. fp = %p\n", fp);
  sa = fscanf_entry_section(fp, cmn_assembly, max_alloc);
  if (sa == EOF) {
    free(cmn_assembly);
    return EOF;
  }
  linenum += sa;

  DBG2(HARVEST, "cmn_assembly:\n%s\n", cmn_assembly);
  *cmn_iseq_len = cmn_iseq_from_string(NULL, cmn_iseq, cmn_iseq_size, cmn_assembly, I386_AS_MODE_CMN);
  if (*cmn_iseq_len < 0) {
    printf("could not disassemble:\n%s\n", cmn_assembly);
  }
  ASSERT(*cmn_iseq_len >= 0);
  DBG2(HARVEST, "*cmn_iseq_len = %ld.\n cmn_iseq:\n%s\n", *cmn_iseq_len,
      cmn_iseq_to_string(cmn_iseq, *cmn_iseq_len, as1, sizeof as1));

  free(cmn_assembly);

  char *live_regs_string[live_out_size];

  linenum += fscanf_live_regs_strings(fp, live_regs_string, live_out_size,
      num_live_out);

  ASSERT(*num_live_out);
  for (i = 0; i < *num_live_out; i++) {
    regset_from_string(&live_out[i], live_regs_string[i]);
    DBG2(HARVEST, "live_regs_string[%d]=%s.\n", i, live_regs_string[i]);
    free(live_regs_string[i]);
  }

  char *mem_live_string;

  mem_live_string = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(mem_live_string);

  linenum += fscanf_mem_live_string(fp, mem_live_string, max_alloc);
  *mem_live_out = mem_live_out_from_string(mem_live_string);

  free(mem_live_string);
  return linenum;
}

unsigned long
cmn_iseq_hash_func_after_canon(cmn_insn_t const *cmn_iseq, long cmn_iseq_len, long table_size)
{
  cmn_insn_t *hash_iseq;
  unsigned long ret;
  long i;

  //hash_iseq = (cmn_insn_t *)malloc(cmn_iseq_len * sizeof(cmn_insn_t));
  hash_iseq = new cmn_insn_t[cmn_iseq_len];
  ASSERT(hash_iseq);

  ASSERT(table_size);
  for (i = 0; i < cmn_iseq_len; i++) {
    cmn_insn_copy(&hash_iseq[i], &cmn_iseq[i]);
  }
  cmn_iseq_hash_canonicalize(hash_iseq, cmn_iseq_len);
  ret = cmn_iseq_hash_func(hash_iseq, cmn_iseq_len);

  ret = ret % table_size;
  DYN_DBG(peep_tab_add, "Calculated index %lu for \n\tcmn_iseq:\n%s",
      ret, cmn_iseq_to_string(hash_iseq, cmn_iseq_len,
        as1, sizeof as1));
  DBG2(PEEP_TAB, "Calculated index %lu for \n\tcmn_iseq:\n%s",
      ret, cmn_iseq_to_string(hash_iseq, cmn_iseq_len,
        as1, sizeof as1));
  //free(hash_iseq);
  delete [] hash_iseq;
  return ret;
}

void
cmn_iseq_hash_canonicalize(cmn_insn_t *insn, int num_insns)
{
  regmap_t hash_regmap;
  imm_vt_map_t hash_imm_map;
  int i;

  regmap_init(&hash_regmap);
  imm_vt_map_init(&hash_imm_map);

  for (i = 0; i < num_insns; i++) {
    cmn_insn_hash_canonicalize(&insn[i], &hash_regmap, &hash_imm_map);
  }
}

/*
void
cmn_insn_add_memtouch_elem(cmn_insn_t *insn, int seg, int base, int index, int64_t disp,
    int size, bool var)
{
  int n;
  ASSERT(insn->memtouch.n_elem >= 0);
  ASSERT(insn->memtouch.n_elem < MEMVT_LIST_MAX_ELEMS);
  n = insn->memtouch.n_elem;
  insn->memtouch.ls[n].segtype = segtype_sel;
  insn->memtouch.ls[n].seg.sel.val = seg;
  insn->memtouch.ls[n].seg.sel.tag = tag_const;
  //insn->memtouch.ls[n].seg.sel.mod.reg.mask = 0xffff;
  insn->memtouch.ls[n].base.val = base;
  insn->memtouch.ls[n].base.tag = (base == -1) ? tag_invalid : tag_const;
  insn->memtouch.ls[n].index.val = index;
  insn->memtouch.ls[n].index.tag = (index == -1) ? tag_invalid : tag_const;
  insn->memtouch.ls[n].disp.val = disp; //n * (DWORD_LEN/BYTE_LEN);
  insn->memtouch.ls[n].disp.tag = tag_const;
  insn->memtouch.ls[n].addrsize = DWORD_LEN/BYTE_LEN;
  //insn->memtouch.ls[n].size = size;

  if (var) {
    char keyword[strlen(G_VAR_KEYWORD) + 256];
    snprintf(keyword, sizeof keyword, "%s.%d", G_VAR_KEYWORD, n);
    keyword_to_memlabel(&insn->memtouch.mls[n], keyword, MEMSIZE_MAX);
  } else {
    char keyword[strlen(G_MEMLABEL_TOP_SYMBOL) + 256];
    snprintf(keyword, sizeof keyword, "%s", G_MEMLABEL_TOP_SYMBOL);
    keyword_to_memlabel(&insn->memtouch.mls[n], keyword, MEMSIZE_MAX);
  }
  //memlabel_copy(&insn->memtouch.mls[n], memlabel);
  insn->memtouch.n_elem++;
}*/

static void
cmn_iseq_canonicalize_locals_symbols(cmn_insn_t *cmn_iseq, long cmn_iseq_len,
    imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  long i;

  imm_vt_map_init(imm_vt_map);
  for (i = 0; i < cmn_iseq_len; i++) {
    cmn_insn_canonicalize_locals_symbols(&cmn_iseq[i], imm_vt_map, tag);
  }
}

void
cmn_iseq_canonicalize_symbols(cmn_insn_t *cmn_iseq, long cmn_iseq_len,
    imm_vt_map_t *imm_vt_map)
{
  cmn_iseq_canonicalize_locals_symbols(cmn_iseq, cmn_iseq_len, imm_vt_map, tag_imm_symbol);
}

void
cmn_iseq_canonicalize_locals(cmn_insn_t *cmn_iseq, long cmn_iseq_len,
    imm_vt_map_t *imm_vt_map)
{
  cmn_iseq_canonicalize_locals_symbols(cmn_iseq, cmn_iseq_len, imm_vt_map, tag_imm_local);
}

void
cmn_iseq_rename_nextpcs(struct cmn_insn_t *cmn_iseq, long cmn_iseq_len,
    struct nextpc_map_t const *nextpc_map)
{
  long i;
  for (i = 0; i < cmn_iseq_len; i++) {
    cmn_insn_rename_nextpcs(&cmn_iseq[i], nextpc_map);
  }
}

void
cmn_insn_rename_nextpcs(cmn_insn_t *insn, nextpc_map_t const *nextpc_map)
{
  vector<valtag_t> pcrels = cmn_insn_get_pcrel_values(insn);
  //cout << __func__ << " " << __LINE__ << ": cmn_insn = " << cmn_iseq_to_string(insn, 1, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": pcrels.size() = " << pcrels.size() << endl;
  for (valtag_t &pcrel : pcrels) {
    //cout << __func__ << " " << __LINE__ << ": pcrel.val = " << pcrel.val << ", pcrel.tag = " << pcrel.tag << endl;
    nextpc_map_rename_pcrel(pcrel, nextpc_map);
    //cout << __func__ << " " << __LINE__ << ": pcrel.val = " << pcrel.val << ", pcrel.tag = " << pcrel.tag << endl;
  }
  cmn_insn_set_pcrel_values(insn, pcrels);
  //cout << __func__ << " " << __LINE__ << ": cmn_iseq = " << cmn_iseq_to_string(insn, 1, as1, sizeof as1) << endl;
}

void
cmn_iseq_serialize(ostream& os, cmn_insn_t const* iseq, size_t iseq_len)
{
  os << " ";
  for (size_t i = 0; i < iseq_len; i++) {
    iseq[i].serialize(os);
    os << "\n";
    if (i != iseq_len - 1) {
      os << " ";
    } else {
      os << "--\n";
    }
  }
}

size_t
cmn_iseq_deserialize(istream& is, cmn_insn_t* buf, size_t bufsize)
{
  autostop_timer func_timer(__func__);
  string line;
  bool end;
  size_t i = 0;
  end = !getline(is, line);
  ASSERT(!end);
  while (line != "--") {
    //cout << __func__ << " " << __LINE__ << ": i = " << i << ", bufsize = " << bufsize << ", nextchar = '" << nextchar << "'\n";
    ASSERT(i < bufsize);
    istringstream iss_line(line);
    //cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    buf[i].deserialize(iss_line);
    //char newline;
    //is.get(newline);
    //cout << __func__ << " " << __LINE__ << ": newline = '" << newline << "'\n";
    //ASSERT(newline == '\n');
    end = !getline(is, line);
    ASSERT(!end);
    //cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
    i++;
  }
  ASSERT(line == "--");
  return i;
}

/*#if (!((CMN == COMMON_SRC) && (ARCH_SRC == ARCH_ETFG)))
eqspace::tfg *
cmn_iseq_get_tfg(cmn_insn_t const *cmn_iseq, long cmn_iseq_len, context *ctx, sym_exec &se)
{
  return se.cmn_get_tfg(cmn_iseq, cmn_iseq_len, ctx);
}
#endif*/
