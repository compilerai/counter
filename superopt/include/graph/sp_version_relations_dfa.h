#pragma once

#include "support/types.h"

#include "expr/expr.h"
#include "expr/sp_version.h"
#include "expr/pc.h"

#include "gsupport/tfg_edge.h"

#include "graph/dfa.h"
#include "graph/sp_relation.h"

using namespace std;

namespace eqspace {

class sp_version_relations_val_t
{
  // locid -> sp-version -> sp-relation
  map<graph_loc_id_t,map<expr_ref,sp_relation_t>> m_loc_to_spv_to_sp_relation;
  // values absent in map are automatically considered bottom
  bool m_is_top = false;

  map<expr_id_t,expr_pair> submap_from_sp_relations(map<graph_loc_id_t,expr_ref> const& locid_to_expr_map) const;

  sp_version_relations_val_t() {}

  sp_version_relations_val_t(set<graph_loc_id_t> const& locids, set<expr_ref> const& sp_versions)
  {
    for (auto const& locid : locids) {
      auto [itr,_] = m_loc_to_spv_to_sp_relation.emplace(locid,map<expr_ref,sp_relation_t>());
      for (auto const& spv : sp_versions) {
        itr->second.emplace(spv, sp_relation_t::top());
      }
    }
  }

public:
  sp_version_relations_val_t(istream& is, context* ctx);
  ostream& sp_version_relations_to_stream(ostream& os, map<graph_loc_id_t,expr_ref> const& locid_to_expr_map) const;

  static sp_version_relations_val_t bottom()
  {
    return sp_version_relations_val_t();
  }

  static sp_version_relations_val_t top()
  {
    sp_version_relations_val_t ret;
    ret.m_is_top = true;
    return ret;
  }

  static sp_version_relations_val_t sp_version_relations_start_value(set<graph_loc_id_t> const& locids, set<expr_ref> const& sp_versions)
  {
    sp_version_relations_val_t ret(locids, sp_versions);
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

  void erase_top_and_bottom_mappings()
  {
    function<bool(expr_ref const&,sp_relation_t&)> mut_or_erase_spr =
      [](expr_ref const&,sp_relation_t& spr) -> bool { return spr.is_top() || spr.is_bottom(); };
    function<bool(graph_loc_id_t const&,map<expr_ref,sp_relation_t>&)> mut_or_erase_spv2spr =
      [&mut_or_erase_spr](graph_loc_id_t const&,map<expr_ref,sp_relation_t>& m) -> bool
      {
        map_mutate_or_erase(m, mut_or_erase_spr);
        return m.empty();
      };
    map_mutate_or_erase(m_loc_to_spv_to_sp_relation, mut_or_erase_spv2spr);
  }

  set<shared_ptr<predicate const>> sp_version_relations_to_preds(map<graph_loc_id_t,expr_ref> const& locid_to_expr_map) const;

  expr_ref expr_simplify_using_sp_version_relations(expr_ref const& e, map<graph_loc_id_t,expr_ref> const& locid_to_expr_map) const;
  optional<int> compare_using_sp_version_relations(expr_ref const& a, expr_ref const& b, map<graph_loc_id_t,expr_ref> const& locid_to_expr_map) const;
  expr_ref determine_least_element_using_sp_version_relations(set<expr_ref> const& exprs, map<graph_loc_id_t,expr_ref> const& locid_to_expr_map) const;

  string to_string() const;

  static optional<pair<sp_relation_t,expr_ref>> compute_sp_relation_for_assign_expr(expr_ref const& e);
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class sp_version_relations_analysis : public dfa_flow_insensitive<T_PC, T_N, T_E, sp_version_relations_val_t, dfa_dir_t::forward, dfa_state_type_t::global>
{
  static bool expr_is_sp_or_sp_version(expr_ref const& e)
  {
    return expr_is_stack_pointer(e) || expr_is_sp_version(e);
  }

  list<pair<sp_relation_t,expr_ref>> do_xfer_for_assign_expr_given_existing_sp_version_relations(expr_ref const& ex, sp_version_relations_val_t const& sp_version_relations, set<graph_loc_id_t> const& killed_locs) const;

public:
  sp_version_relations_analysis(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g, sp_version_relations_val_t &init_vals)
    : dfa_flow_insensitive<T_PC,T_N,T_E,sp_version_relations_val_t,dfa_dir_t::forward,dfa_state_type_t::global>(g, init_vals)
  { }

  virtual dfa_xfer_retval_t xfer_and_meet_flow_insensitive(dshared_ptr<T_E const> const &e, sp_version_relations_val_t &in_out) override;
};

}
