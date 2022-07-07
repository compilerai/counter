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
  map<expr_ref,sp_relation_t> m_spv_to_sp_relation; // values absent in map are automatically considered bottom
  bool m_is_top = false;

  sp_version_relations_val_t() {} // bottom by default
  sp_version_relations_val_t(map<expr_ref,sp_relation_t> const& in_map) : m_spv_to_sp_relation(in_map) {}

  static optional<pair<expr_ref,sp_relation_t>> compute_sp_relation_for_assign_expr(expr_ref const& e);

public:
  sp_version_relations_val_t(istream& is, string line, context* ctx);

  static string serialization_tag;
  friend ostream& operator<<(ostream&, sp_version_relations_val_t const&);

  static sp_version_relations_val_t top()
  {
    sp_version_relations_val_t ret;
    ret.m_is_top = true;
    return ret;
  }
  static sp_version_relations_val_t bottom()
  {
    return sp_version_relations_val_t();
  }

  bool is_top() const { return m_is_top; }
  bool is_bottom() const { return !m_is_top && m_spv_to_sp_relation.empty(); }
  bool operator==(sp_version_relations_val_t const& other) const
  {
    return    m_spv_to_sp_relation == other.m_spv_to_sp_relation
           && m_is_top == other.m_is_top
    ;
  }
  bool operator!=(sp_version_relations_val_t const& other) const
  {
    return !(*this == other);
  }
  string to_string() const;
  map<expr_ref,sp_relation_t> const& sp_version_relations_get_map() const { ASSERT(!m_is_top); return m_spv_to_sp_relation; }

  bool erase_top_and_bottom_mappings()
  {
    function<bool(expr_ref const&,sp_relation_t&)> spr_mut_or_erase =
      [](expr_ref const&, sp_relation_t& spr) -> bool { return spr.is_top() || spr.is_bottom(); };
    map_mutate_or_erase(m_spv_to_sp_relation, spr_mut_or_erase);
    return m_spv_to_sp_relation.empty();
  }

  bool weak_update(sp_version_relations_val_t const& other);

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  bool meet(sp_version_relations_val_t const& other, map<graph_loc_id_t,sp_version_relations_val_t> const& this_loc_map, map<graph_loc_id_t,sp_version_relations_val_t> const& other_loc_map, map<graph_loc_id_t,expr_ref> const& locid2expr_map, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g);

  bool dep_killed_by_killed_locs(set<graph_loc_id_t> const& killed_locids, graph_locs_map_t const& locs_map);

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static map<graph_loc_id_t,sp_version_relations_val_t> generate_vals_from_gen_set(map<graph_loc_id_t,expr_ref> const& gen_set, set<graph_loc_id_t> const& killed_locids, map<graph_loc_id_t,sp_version_relations_val_t> const& in_map, graph_locs_map_t const& locs_map, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g);

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static map<graph_loc_id_t,sp_version_relations_val_t> generate_vals_for_edge(dshared_ptr<T_E const> const& e, set<graph_loc_id_t> const& killed_locids, map<graph_loc_id_t,sp_version_relations_val_t> const& in_map, map<graph_loc_id_t,expr_ref> const& locid2expr_map, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g);

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static expr_ref get_lb_expr_for_local_ml_at_pc(memlabel_t const& ml, T_PC const& p, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g)
  {
    context* ctx = g.get_context();
    return g.get_var_version_at_pc(p, ctx->get_key_for_local_memlabel_lb(ml, g.get_srcdst_keyword()->get_str()));
  }

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static expr_ref get_ub_expr_for_local_ml_at_pc(memlabel_t const& ml, T_PC const& p, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g)
  {
    context* ctx = g.get_context();
    return g.get_var_version_at_pc(p, ctx->get_key_for_local_memlabel_ub(ml, g.get_srcdst_keyword()->get_str()));
  }
};

ostream& operator<<(ostream&, sp_version_relations_val_t const&);

map<expr_id_t,expr_pair> sp_version_relations_create_submap(map<graph_loc_id_t,sp_version_relations_val_t> const& spvr_map, map<graph_loc_id_t,expr_ref> const& locid2expr);

}

namespace std {
template<>
struct hash<sp_version_relations_val_t>
{
  size_t operator()(sp_version_relations_val_t const &spvr) const
  {
    size_t seed = 0;
    myhash_combine<size_t>(seed, hash<bool>()(spvr.is_top()));
    myhash_combine<size_t>(seed, hash<bool>()(spvr.is_bottom()));
    if (!spvr.is_top() && !spvr.is_bottom()) {
      for (auto const& [spv,spr] : spvr.sp_version_relations_get_map()) {
        myhash_combine<size_t>(seed, hash<expr_ref>()(spv));
        myhash_combine<size_t>(seed, hash<sp_relation_t>()(spr));
      }
    }
    return seed;
  }
};

}
