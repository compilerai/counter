#include "support/src-defs.h"
#include "rewrite/regconst_map_possibilities.h"
#include "valtag/memslot_set.h"
#include "support/debug.h"
#include "valtag/transmap.h"
#include "valtag/regmap.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "support/utils.h"

using namespace std;

bool
regconst_map_possibilities_t::equals(regconst_map_possibilities_t const &other) const
{
  return m_set == other.m_set;
}

string
regconst_map_possibilities_t::to_string() const
{
  stringstream ss;
  for (const auto &p : m_set) {
    ss << "(" << p.first << ", " << p.second.reg_identifier_to_string() << ")\n";
  }
  return ss.str();
}

static bool
src_reg_has_mapping_in_transmaps(exreg_group_id_t g, reg_identifier_t const &r, transmap_t const *in_tmap,
    transmap_t const *out_tmaps, size_t num_out_tmaps)
{
  if (in_tmap->transmap_has_mapping_for_src_reg(g, r)) {
    return true;
  }
  for (size_t i = 0; i < num_out_tmaps; i++) {
    if (out_tmaps[i].transmap_has_mapping_for_src_reg(g, r)) {
      return true;
    }
  }
  return false;
}

bool
src_inv_regmap_agrees_with_regconst_map_possibilities(regmap_t const *inv_src_regmap,
          transmap_t const *in_tmap, transmap_t const *out_tmaps, size_t num_live_out,
          regconst_map_possibilities_t const &regconst_map_possibilities)
{
  autostop_timer func_timer(__func__);
  for (const auto &g : inv_src_regmap->regmap_extable) {
    for (const auto &r : g.second) {
      if (   r.second.reg_identifier_denotes_constant()
          && src_reg_has_mapping_in_transmaps(g.first, r.first, in_tmap, out_tmaps, num_live_out)
          && !regconst_map_possibilities.belongs(g.first, r.first)) {
        return false;
      }
    }
  }
  return true;
}

void
regconst_map_possibilities_t::add_reg_id(exreg_group_id_t group, reg_identifier_t const &regid)
{
  m_set.insert(make_pair(group, regid));
}

bool
regconst_map_possibilities_t::belongs(exreg_group_id_t group, reg_identifier_t const &regid) const
{
  return set_belongs(m_set, make_pair(group, regid));
}

static bool
transmap_loc_is_used_by_other_src_regs(transmap_loc_t const &loc, transmap_t const *out_tmaps, size_t num_out_tmaps, exreg_group_id_t src_group, reg_identifier_t const &src_reg_id)
{
  for (size_t i = 0; i < num_out_tmaps; i++) {
    for (const auto &g : out_tmaps[i].extable) {
      for (const auto &r : g.second) {
        if (g.first == src_group && r.first == src_reg_id) {
          continue;
        }
        if (r.second.tmap_entry_contains_loc(loc)) {
          return true;
        }
      }
    }
  }
  return false;
}

static bool
regconst_possibility_exists(exreg_group_id_t src_group, reg_identifier_t const &src_reg_id, transmap_entry_t const &tmap, dst_insn_t const *iseq, long iseq_len, transmap_t const *out_tmaps, size_t num_nextpcs)
{
  size_t max_bin1size = 10240;
  uint8_t *bin1 = new uint8_t[max_bin1size];
  if (dst_iseq_rename_randomly_and_assemble(iseq, iseq_len, num_nextpcs, bin1, max_bin1size, I386_AS_MODE_32) == 0) {
    delete [] bin1;
    return true;
  }
  dst_insn_t *iseq_copy = new dst_insn_t[iseq_len];
  dst_iseq_copy(iseq_copy, iseq, iseq_len);
  bool ret = true;
  for (const auto &loc : tmap.get_locs()) {
    if (   transmap_loc_is_used_by_other_src_regs(loc, out_tmaps, num_nextpcs, src_group, src_reg_id)
        || !dst_iseq_replace_tmap_loc_with_const(iseq_copy, iseq_len, loc, ETFG_CANON_CONST0)) {
      ret = false;
      break;
    }
  }
  if (ret) {
    ret = (dst_iseq_rename_randomly_and_assemble(iseq_copy, iseq_len, num_nextpcs, bin1, max_bin1size, I386_AS_MODE_32) != 0);
  }
  delete [] iseq_copy;
  delete [] bin1;
  return ret;
}

regconst_map_possibilities_t
compute_regconst_map_possibilities(struct transmap_t const *tmap, struct transmap_t const *out_tmaps, struct dst_insn_t const *iseq, long iseq_len, size_t num_nextpcs)
{
  regconst_map_possibilities_t ret;
  /*regset_t out_used;
  memslot_set_t out_used_memslots;
  regset_clear(&out_used);
  out_used_memslots.memslot_set_clear();
  for (size_t i = 0; i < num_nextpcs; i++) {
    regset_t ouse;
    transmap_get_used_dst_regs(&out_tmaps[i], &ouse);
    set<stack_offset_t> stack_slots = transmap_get_used_stack_slots(&out_tmaps[i]);
    regset_union(&out_used, &ouse);
    for (auto slot : stack_slots) {
      out_used_memslots.memslot_set_add_mapping(slot.get_offset(), MAKE_MASK(DWORD_LEN));
    }
  }*/
  for (const auto &g : tmap->extable) {
    for (const auto &r : g.second) {
      CPP_DBG_EXEC(REGCONST, cout << __func__ << " " << __LINE__ << ": g.first = " << g.first << ", r.first = " << r.first.reg_identifier_to_string() << ": tmap_entry = " << r.second.transmap_entry_to_string() << endl);
      if (regconst_possibility_exists(g.first, r.first, r.second, iseq, iseq_len, out_tmaps, num_nextpcs)) {
        CPP_DBG_EXEC(REGCONST, cout << __func__ << " " << __LINE__ << ": g.first = " << g.first << ", r.first = " << r.first.reg_identifier_to_string() << ": tmap_entry = " << r.second.transmap_entry_to_string() << ": regconst possibility exists!\n" << endl);
        ret.add_reg_id(g.first, r.first);
      }
    }
  }
  return ret;
}
