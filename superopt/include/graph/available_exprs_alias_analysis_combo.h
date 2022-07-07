#pragma once

#include "graph/alias_dfa.h"
#include "graph/avail_exprs.h"
#include "graph/sp_version_relations.h"
#include "graph/graph_with_aliasing.h"


namespace eqspace {

//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//class available_exprs_alias_analysis_combo_dfa_t;

class ftmap_pointsto_analysis_combo_dfa_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class available_exprs_alias_analysis_combo_val_t {
private:
  alias_val_t<T_PC, T_N, T_E, T_PRED> m_alias_val;
  avail_exprs_t m_avail_exprs;
  sp_version_relations_t m_sp_version_relations;
  //map<graph_loc_id_t, graph_cp_location>& m_locs;
  graph_locs_map_t const& m_locs;
  bool m_is_top = false;

  //set<pc> m_visited;
public:
  available_exprs_alias_analysis_combo_val_t() = delete;
  available_exprs_alias_analysis_combo_val_t(graph_locs_map_t& locs_map) : m_locs(locs_map)
  { }
  available_exprs_alias_analysis_combo_val_t(alias_val_t<T_PC,T_N,T_E,T_PRED> const& alias_val, avail_exprs_t const& avail_exprs, sp_version_relations_t const& sp_version_relations, graph_locs_map_t& locs_map)
  : m_alias_val(alias_val),
    m_avail_exprs(avail_exprs),
    m_sp_version_relations(sp_version_relations),
    m_locs(locs_map)
  { }

  //void mark_visited(pc const &p) { m_visited.insert(p); }
  //bool is_visited(pc const& p) const { return m_visited.count(p); }

  static available_exprs_alias_analysis_combo_val_t top(graph_locs_map_t& locs_map)
  {
    //map<graph_loc_id_t,graph_cp_location> m;
    //static graph_locs_map_t dummy_locs_unused(m);
    //available_exprs_alias_analysis_combo_val_t ret(dummy_locs_unused);
    available_exprs_alias_analysis_combo_val_t ret(locs_map);
    ret.m_is_top = true;
    return ret;
  }
  bool is_top() const { return m_is_top; }
  //bool combo_meet(available_exprs_alias_analysis_combo_val_t const& other)
  //{
  //  m_alias_val.alias_val_meet(other.m_alias_val);
  //  NOT_IMPLEMENTED();
  //}
  //bool operator!=(available_exprs_alias_analysis_combo_val_t const& other) const
  //{
  //  if (m_is_top != other.m_is_top) {
  //    return true;
  //  }
  //  if (m_is_top) {
  //    return false;
  //  }
  //  return    m_alias_val != other.m_alias_val
  //         || m_avail_exprs != other.m_avail_exprs
  //  ;
  //}
  graph_locs_map_t const& get_graph_locs_map() const { return m_locs; }
  map<graph_loc_id_t, graph_cp_location> const& get_locs() const { return m_locs.graph_locs_map_get_locs(); }
  map<graph_loc_id_t, expr_ref> const& get_locid2expr_map() const { return m_locs.graph_locs_map_get_locid2expr_map(); }
  map<expr_id_t, graph_loc_id_t> const& get_exprid2locid_map() const { return m_locs.graph_locs_map_get_exprid2locid_map(); }
  avail_exprs_t const& get_avail_exprs() const { return m_avail_exprs; }
  sp_version_relations_t const& get_sp_version_relations() const { return m_sp_version_relations; }
  map<graph_loc_id_t, lr_status_ref> get_lr_status_map(graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g) const;

  static avail_exprs_t avail_exprs_intersect_over_call_contexts(map<call_context_ref, available_exprs_alias_analysis_combo_val_t<T_PC, T_N, T_E, T_PRED>> const& m);

  static sp_version_relations_t sp_version_relations_intersect_over_call_contexts(map<call_context_ref, available_exprs_alias_analysis_combo_val_t<T_PC, T_N, T_E, T_PRED>> const& m);

  static map<graph_loc_id_t, lr_status_ref> lr_status_union_over_call_contexts(map<call_context_ref, map<graph_loc_id_t, lr_status_ref>> const& m);

  static available_exprs_alias_analysis_combo_val_t start_pc_value(dshared_ptr<graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const> g, graph_locs_map_t& locs, set<graph_loc_id_t> const& start_relevant_loc_ids, map<graph_loc_id_t,expr_ref> const& locid2expr_map, string const& fname, bool function_is_program_entry);

  //void add_start_pc_value_for_new_locs(map<graph_loc_id_t, graph_cp_location> const& new_locs)
  //{
  //  NOT_IMPLEMENTED();
  //}

  friend class ftmap_pointsto_analysis_combo_dfa_t;
  //friend class available_exprs_alias_analysis_combo_dfa_t<T_PC, T_N, T_E, T_PRED>;
};

}
