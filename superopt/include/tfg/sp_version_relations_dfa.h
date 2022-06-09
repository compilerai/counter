#pragma once
#include <map>
#include <string>

#include "support/types.h"

#include "expr/expr.h"
#include "expr/sp_version.h"
#include "expr/pc.h"

#include "gsupport/tfg_edge.h"

#include "graph/dfa.h"

using namespace std;

namespace eqspace {

enum class sp_relation_status_t {
         SP_RELATION_STATUS_TOP,
         SP_RELATION_STATUS_AND_MASK_OFFSET,
         SP_RELATION_STATUS_OR_MASK_OFFSET,
         SP_RELATION_STATUS_LT,
         SP_RELATION_STATUS_LE,
         SP_RELATION_STATUS_GT,
         SP_RELATION_STATUS_GE,
         SP_RELATION_STATUS_BOTTOM
};

class sp_relation_t
{
  sp_relation_status_t m_status;

  // only for SP_RELATION_STATUS_AND_MASK_OFFSET and SP_RELATION_STATUS_OR_MASK_OFFSET
  unsigned m_masklen; // `mask'-lower bits are masked away (by 0 with AND, by 1 with OR)
  mybitset m_offset;  // offset added after masking
  // ((x >> masklen) << masklen) + offset

private:
  sp_relation_t(sp_relation_status_t status)
    : m_status(status)
  {}
  sp_relation_t(sp_relation_status_t status, unsigned masklen, mybitset offset)
    : m_status(status),
      m_masklen(masklen),
      m_offset(offset)
  {
    ASSERT(status == sp_relation_status_t::SP_RELATION_STATUS_AND_MASK_OFFSET || status == sp_relation_status_t::SP_RELATION_STATUS_OR_MASK_OFFSET);
  }

  static mybitset or_mask_for_masklen(unsigned masklen, size_t bvsize)
  {
    return mybitset((1<<masklen)-1, bvsize);
  }
  static mybitset and_mask_for_masklen(unsigned masklen, size_t bvsize)
  {
    return ~or_mask_for_masklen(masklen, bvsize);
  }

  static sp_relation_t meet_offsets(sp_relation_t const& a, sp_relation_t const& b);

  static string sp_relation_status_to_string(sp_relation_status_t sps);
  static sp_relation_status_t sp_relation_status_from_string(string const& s);

  bool implies_equality() const
  {
    if (!(   m_status == sp_relation_status_t::SP_RELATION_STATUS_AND_MASK_OFFSET
          || m_status == sp_relation_status_t::SP_RELATION_STATUS_OR_MASK_OFFSET)) {
      return false;
    }
    return m_masklen == 0 && m_offset.is_zero();
  }

public:
  sp_relation_t(istream& in);
  ostream& sp_relation_to_stream(ostream& os) const;

  bool is_top() const    { return m_status == sp_relation_status_t::SP_RELATION_STATUS_TOP; }
  bool is_bottom() const { return m_status == sp_relation_status_t::SP_RELATION_STATUS_BOTTOM; }

  sp_relation_t inverse() const;
  // if    (x,y) -> this,
  //       (y,z) -> other
  // then, (x,z) -> this.compse(other)
  sp_relation_t compose(sp_relation_t const& other) const;
  bool implies(sp_relation_t const& other) const;

  bool meet(sp_relation_t const& other);

  bool operator==(sp_relation_t const& other) const
  {
    // NOTE: too strong? do not check mask and offset for cases where they may not be set?
    return    m_status == other.m_status
           && m_masklen == other.m_masklen
           && m_offset == other.m_offset
           ;
  }
  bool operator!=(sp_relation_t const& other) const { return !(this->operator==(other)); }

  list<shared_ptr<predicate const>> to_non_trivial_preds(expr_ref const& a, expr_ref const& b) const;

  static sp_relation_t top()    { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_TOP); }
  static sp_relation_t bottom() { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_BOTTOM); }
  static sp_relation_t and_mask_offset(unsigned mask, mybitset offset) { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_AND_MASK_OFFSET, mask, offset); }
  static sp_relation_t or_mask_offset(unsigned mask, mybitset offset)  { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_OR_MASK_OFFSET, mask, offset); }
  static sp_relation_t less_than()    { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_LT); }
  static sp_relation_t greater_than() { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_GT); }
  static sp_relation_t less_than_or_equal_to()    { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_LE); }
  static sp_relation_t greater_than_or_equal_to() { return sp_relation_t(sp_relation_status_t::SP_RELATION_STATUS_GE); }
};

class sp_version_relations_val_t
{
  // locid -> sp-version -> sp-relation
  map<graph_loc_id_t,map<expr_ref,sp_relation_t>> m_loc_to_spv_to_sp_relation;
  // values absent in map are automatically considered bottom
  bool m_is_top;

public:
  sp_version_relations_val_t() : m_is_top(false) {}

  sp_version_relations_val_t(istream& is, context* ctx);
  ostream& sp_version_relations_to_stream(ostream& os, map<graph_loc_id_t,expr_ref> const& locid_to_expr_map) const;

  static sp_version_relations_val_t top()
  {
    sp_version_relations_val_t ret;
    ret.m_is_top = true;
    return ret;
  }

  bool meet(sp_version_relations_val_t const &other);

  map<expr_ref,sp_relation_t> get_sp_relation_for_loc(graph_loc_id_t locid) const
  {
    if (m_loc_to_spv_to_sp_relation.count(locid))
      return m_loc_to_spv_to_sp_relation.at(locid);
    return {};
  }

  bool add_sp_relation_for_loc(graph_loc_id_t locid, expr_ref const& spv, sp_relation_t const& spr)
  {
    auto itr = m_loc_to_spv_to_sp_relation.find(locid);
    if (itr == m_loc_to_spv_to_sp_relation.end()) {
      auto p = m_loc_to_spv_to_sp_relation.emplace(locid, map<expr_ref,sp_relation_t>());
      itr = p.first;
    }
    if (itr->second.count(spv)) {
      return false;
    }
    ASSERT(!itr->second.count(spv));
    itr->second.emplace(spv, spr);
    return true;
  }

  void erase(graph_loc_id_t locid) { m_loc_to_spv_to_sp_relation.erase(locid); }
  void erase_relations_with_spv(expr_ref spv)
  {
    for (auto& [_,spv_to_sp_relation] : m_loc_to_spv_to_sp_relation) {
      spv_to_sp_relation.erase(spv);
    }
  }

  list<shared_ptr<predicate const>> sp_version_relations_to_preds(map<graph_loc_id_t,expr_ref> const& locid_to_expr_map) const;

  string to_string() const;

  static optional<pair<sp_relation_t,expr_ref>> compute_sp_relation_for_assign_expr(expr_ref const& e);
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class sp_version_relations_analysis : public data_flow_analysis<T_PC, T_N, T_E, sp_version_relations_val_t, dfa_dir_t::forward>
{
  static bool expr_is_sp_or_sp_version(expr_ref const& e)
  {
    return expr_is_stack_pointer(e) || expr_is_sp_version(e);
  }

  list<pair<sp_relation_t,expr_ref>> do_xfer_for_assign_expr_given_existing_sp_version_relations(expr_ref const& ex, sp_version_relations_val_t const& sp_version_relations, set<graph_loc_id_t> const& killed_locs) const;

public:
  sp_version_relations_analysis(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g, map<T_PC, sp_version_relations_val_t> &init_vals);

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, sp_version_relations_val_t const& in, sp_version_relations_val_t &out) override;
};

}
