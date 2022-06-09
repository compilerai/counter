#include <map>
#include <stdlib.h>
#include <stdbool.h>
#include <iostream>
#include "rewrite/nomatch_pairs.h"
#include "rewrite/nomatch_pairs_set.h"
#include "support/debug.h"
#include "support/c_utils.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "ppc/regs.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "ppc/insn.h"
#include "valtag/regmap.h"
#include "valtag/transmap.h"
#include "support/src-defs.h"

static char as1[40960];

/*static bool
regset_belongs_vreg_mod(regset_t const *regset, int group, int vreg)
{
  //if (group == -1) {
  //  return regset_belongs_vreg(regset, vreg);
  //} else {
    return regset_belongs_exvreg(regset, group, vreg);
  //}
}*/

static bool
dst_iseq_all_reg1_touchs_occur_before_any_reg2_def(dst_insn_t const *iseq,
    int iseq_len, int group, reg_identifier_t const &vreg1, reg_identifier_t const &vreg2)
{
  bool vreg2_def_found = 0;
  int i;

  for (i = 0; i < iseq_len; i++) {
    struct regset_t use, def;
    dst_insn_get_usedef_regs(&iseq[i], &use, &def);
    regset_union(&use, &def);
    if (vreg2_def_found && regset_belongs_ex(&use, group, vreg1)) {
      DYN_DBG2(peep_tab_add, "vreg1 %s belongs to use after vreg2 %s def, returning false\n",
          vreg1.reg_identifier_to_string().c_str(), vreg2.reg_identifier_to_string().c_str());
      return false;
    }
    if (regset_belongs_ex(&def, group, vreg2)) {
      vreg2_def_found = true;
    }
  }
  return true;
}


bool
transmap_entry_t::transmap_entries_cannot_match(transmap_entry_t const &other, dst_insn_t const *iseq, long iseq_len) const
{
  int i, j/*, reg1, reg2*/, group;

  for (const auto &loc : this->m_locs) {
    bool found_loc_in_other = false;
    for (const auto &other_loc : other.m_locs) {
      if (loc.get_loc() == other_loc.get_loc()) {
        if (   loc.get_loc() >= TMAP_LOC_EXREG(0)
            && loc.get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
          group = loc.get_loc() - TMAP_LOC_EXREG(0);
        } else {
          continue; //NOT_IMPLEMENTED()?
        }
        reg_identifier_t const &reg1 = loc.get_reg_id();
        reg_identifier_t const &reg2 = other_loc.get_reg_id();
        if (reg1.reg_identifier_get_id() != reg2.reg_identifier_get_id()) { //if the transmaps map them to the same reg anyways (i.e., reg1 == reg2), then they can be matched to the same src-reg too, so only emit nomatch-flags when reg1 != reg2
          if (   !dst_iseq_all_reg1_touchs_occur_before_any_reg2_def(iseq, iseq_len, group,
                     reg_identifier_t(reg1), reg_identifier_t(reg2))
              || !dst_iseq_all_reg1_touchs_occur_before_any_reg2_def(iseq, iseq_len, group,
                     reg_identifier_t(reg2), reg_identifier_t(reg1))) {
            DYN_DBG2(peep_tab_add, "returning true on group %d: %s,%s\n", group, reg1.reg_identifier_to_string().c_str(), reg2.reg_identifier_to_string().c_str());
            return true;
          }
        }
        found_loc_in_other = true;
        break;
      }
    }
    if (!found_loc_in_other) {
      return false; //transmaps won't match anyways. do not need nomatch_pairs entry
    }
  }
  return false;
}

static void
nomatch_pairs_vec_add_if_does_not_exist(vector<nomatch_pairs_t> &vec, exreg_group_id_t group1, reg_identifier_t const &reg1, exreg_group_id_t group2, reg_identifier_t const &reg2)
{
  for (const auto &n : vec) {
    if (nomatch_pair_belongs(&n, group1, reg1, group2, reg2)) {
      return;
    }
  }
  nomatch_pairs_t n;
  n.src_reg1 = reg1;
  n.src_reg2 = reg2;
  n.group1 = group1;
  n.group2 = group2;
  vec.push_back(n);
}

//int
nomatch_pairs_set_t
compute_nomatch_pairs(//nomatch_pairs_t *nomatch_pairs, int nomatch_pairs_size,
    transmap_t const *in_tmap, transmap_t const *out_tmaps, long num_out_tmaps,
    dst_insn_t const *iseq, long iseq_len)
{
  int i, j, l, group/*, num_nomatch_pairs*/;
  vector<nomatch_pairs_t> nomatch_pairs;

  transmap_t tmap_union;
  transmap_copy(&tmap_union, in_tmap);
  for (long i = 0; i < num_out_tmaps; i++) {
    transmaps_union(&tmap_union, &tmap_union, &out_tmaps[i]);
  }

  //num_nomatch_pairs = 0;

  for (const auto &g1 : tmap_union.extable) {
    for (const auto &r1 : g1.second) {
      for (const auto &g2 : tmap_union.extable) {
        for (const auto &r2 : g2.second) {
          exreg_group_id_t group1 = g1.first;
          reg_identifier_t const &i = r1.first;
          exreg_group_id_t group2 = g2.first;
          reg_identifier_t const &j = r2.first;
          if (   group1 == group2
              && i == j) {
            continue;
          }
          DYN_DBG3(peep_tab_add, "Checking %s<->%s for nomatch pairs\n", i.reg_identifier_to_string().c_str(), j.reg_identifier_to_string().c_str());
          DYN_DBG3(peep_tab_add, "tmap_union =\n%s\n", transmap_to_string(&tmap_union, as1, sizeof as1));
          if (transmap_get_elem(&tmap_union, group1, i)->transmap_entries_cannot_match(
              *transmap_get_elem(&tmap_union, group2, j), iseq, iseq_len)) {
            DYN_DBG2(peep_tab_add, "Adding %s<->%s to nomatch pairs\n", i.reg_identifier_to_string().c_str(), j.reg_identifier_to_string().c_str());
            nomatch_pairs_vec_add_if_does_not_exist(nomatch_pairs, group1, i, group2, j);
            //nomatch_pairs[num_nomatch_pairs].src_reg1 = i;
            //nomatch_pairs[num_nomatch_pairs].src_reg2 = j;
            //nomatch_pairs[num_nomatch_pairs].group = group;
            //num_nomatch_pairs++;
            //ASSERT(num_nomatch_pairs <= nomatch_pairs_size);
          }
        }
      }
    }
  }
  //DBG2(peep_tab_add, "returning %d\n", num_nomatch_pairs);
  //return num_nomatch_pairs;
  nomatch_pairs_set_t ret(nomatch_pairs);
  return ret;
}

char *
nomatch_pairs_to_string(nomatch_pairs_t const *nomatch_pairs, int num_nomatch_pairs,
    char *buf, size_t buf_size)
{
  char *ptr = buf, *end = buf+buf_size;
  int i;
  *ptr = '\0';
  for (i = 0; i < num_nomatch_pairs; i++) {
    ptr += snprintf(ptr, end - ptr, "\t%s<-->%s\n",
        nomatch_pairs[i].src_reg1.reg_identifier_to_string().c_str(), nomatch_pairs[i].src_reg2.reg_identifier_to_string().c_str());
  }
  return buf;
}

bool
src_inv_regmap_agrees_with_nomatch_pairs(regmap_t const *inv_src_regmap,
    //nomatch_pairs_t const *nomatch_pairs, int num_nomatch_pairs
    nomatch_pairs_set_t const &nomatch_pairs_set
    )
{
  autostop_timer func_timer(__func__);
  int i;
  //DBG2(PEEP_TAB, "num_nomatch_pairs = %d\n", num_nomatch_pairs);
  for (i = 0; i < nomatch_pairs_set.num_nomatch_pairs; i++) {
    reg_identifier_t const &r1 = nomatch_pairs_set.nomatch_pairs[i].src_reg1;
    reg_identifier_t const &r2 = nomatch_pairs_set.nomatch_pairs[i].src_reg2;
    exreg_group_id_t group1 = nomatch_pairs_set.nomatch_pairs[i].group1;
    ASSERT(group1 >= 0 && group1 < SRC_NUM_EXREG_GROUPS);
    exreg_group_id_t group2 = nomatch_pairs_set.nomatch_pairs[i].group2;
    ASSERT(group2 >= 0 && group2 < SRC_NUM_EXREG_GROUPS);
    DBG2(PEEP_TAB, "Checking %s(%s)<->%s(%s)\n", r1.reg_identifier_to_string().c_str(), regmap_get_elem(inv_src_regmap, group1, r1).reg_identifier_to_string().c_str(),
        r2.reg_identifier_to_string().c_str(), regmap_get_elem(inv_src_regmap, group2, r2).reg_identifier_to_string().c_str());
    ASSERT(regmap_get_elem(inv_src_regmap, group1, r1).reg_identifier_is_valid());
    ASSERT(!regmap_get_elem(inv_src_regmap, group1, r1).reg_identifier_is_unassigned());
    ASSERT(regmap_get_elem(inv_src_regmap, group2, r2).reg_identifier_is_valid());
    ASSERT(!regmap_get_elem(inv_src_regmap, group2, r2).reg_identifier_is_unassigned());
    if (regmap_get_elem(inv_src_regmap, group1, r1) == regmap_get_elem(inv_src_regmap, group2, r2)) {
      return false;
    }
  }
  return true;
}

void
nomatch_pair_copy(nomatch_pairs_t *dst, nomatch_pairs_t const *src)
{
  dst->src_reg1 = src->src_reg1;
  dst->src_reg2 = src->src_reg2;
  dst->group1 = src->group1;
  dst->group2 = src->group2;
}

char *
nomatch_pairs_set_to_string(nomatch_pairs_set_t const &nomatch_pairs_set, char *buf, size_t size)
{
  return nomatch_pairs_to_string(nomatch_pairs_set.nomatch_pairs, nomatch_pairs_set.num_nomatch_pairs, buf, size);
}

bool
nomatch_pair_equals(nomatch_pairs_t const *a, nomatch_pairs_t const  *b)
{
  return    a->src_reg1 == b->src_reg1
         && a->src_reg2 == b->src_reg2
         && a->group1 == b->group1
         && a->group2 == b->group2
  ;
}

static void
nomatch_pair_rename(nomatch_pairs_t *dst, nomatch_pairs_t const *src, regmap_t const *regmap)
{
  ASSERT(regmap->regmap_extable.count(src->group1));
  ASSERT(regmap->regmap_extable.at(src->group1).count(src->src_reg1));
  ASSERT(regmap->regmap_extable.count(src->group2));
  ASSERT(regmap->regmap_extable.at(src->group2).count(src->src_reg2));
  dst->group1 = src->group1;
  dst->group2 = src->group2;
  dst->src_reg1 = regmap_get_elem(regmap, src->group1, src->src_reg1);
  dst->src_reg2 = regmap_get_elem(regmap, src->group2, src->src_reg2);
}

void
nomatch_pairs_set_copy_using_regmap(nomatch_pairs_set_t *out, nomatch_pairs_set_t const  *in, regmap_t const *regmap)
{
  *out = *in; //the equality operator will resize the array appropriately
  for (long i = 0; i < out->num_nomatch_pairs; i++) {
    nomatch_pair_rename(&out->nomatch_pairs[i], &in->nomatch_pairs[i], regmap);
  }
}

bool
nomatch_pair_belongs(nomatch_pairs_t const *nomatch_pair, exreg_group_id_t group1, reg_identifier_t const &reg1, exreg_group_id_t group2, reg_identifier_t const &reg2)
{
  if (   nomatch_pair->group1 == group1
      && nomatch_pair->src_reg1 == reg1
      && nomatch_pair->group2 == group2
      && nomatch_pair->src_reg2 == reg2) {
    return true;
  }
  if (   nomatch_pair->group2 == group1
      && nomatch_pair->src_reg2 == reg1
      && nomatch_pair->group1 == group2
      && nomatch_pair->src_reg1 == reg2) {
    return true;
  }
  return false;
}
