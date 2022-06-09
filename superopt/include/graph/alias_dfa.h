#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/dyn_debug.h"
#include "graph/dfa.h"
#include "graph/lr_map.h"

using namespace std;

namespace eqspace {

// forward decl
template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_aliasing;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class alias_val_t
{
private:
  typedef graph_with_aliasing<T_PC, T_N, T_E, T_PRED> graph_with_aliasing_instance;
  map<graph_loc_id_t, lr_status_ref> m_loc_map;
public:
  alias_val_t() {}
  alias_val_t(map<graph_loc_id_t, lr_status_ref> const &m) : m_loc_map(m) {
    autostop_timer func_timer(string("alias_val_t::constructor_with_map_arg"));
  }

  //static alias_val_t top()
  //{
  //  return alias_val_t();
  //}

  void alias_val_add_lr_status_mapping(graph_loc_id_t loc_id, lr_status_ref const& lr_status)
  {
    bool inserted = m_loc_map.insert(make_pair(loc_id, lr_status)).second;
    ASSERT(inserted);
  }

  map<graph_loc_id_t, lr_status_ref> get_loc_map_for_locs(map<graph_loc_id_t, graph_cp_location> const& locs, set<memlabel_ref> const &cs_addr_ref_ids, set<memlabel_t> const &bottom_set) const;
  map<graph_loc_id_t, lr_status_ref> const &get_loc_map() const { return m_loc_map; }
  lr_status_ref const &get_loc_status(graph_loc_id_t loc_id) const { return m_loc_map.at(loc_id); }
  bool loc_exists(graph_loc_id_t loc_id) const { return m_loc_map.count(loc_id) != 0; }
  static alias_val_t alias_val_start_pc_value(dshared_ptr<graph_with_aliasing_instance const> graph, set<graph_loc_id_t> const& relevant_locids, map<graph_loc_id_t,expr_ref> const& locid2expr_map, string const& fname, bool function_is_program_entry)
  {
    //autostop_timer func_timer(__func__);
    //set<memlabel_ref> const &relevant_addr_refs = graph->graph_get_relevant_addr_refs();
    set<memlabel_ref> const &relevant_addr_refs = graph->graph_get_relevant_memlabels().get_memlabel_set_all_except_heap_and_args();
    context *ctx = graph->get_context();
    //consts_struct_t const &cs = ctx->get_consts_struct();
    map<graph_loc_id_t, lr_status_ref> loc_map;
    for (auto const& loc : relevant_locids) {
      loc_map.insert(make_pair(loc, lr_status_t::lr_status_start_pc_value(locid2expr_map.at(loc), relevant_addr_refs, ctx, graph->get_argument_regs(), graph->get_symbol_map(), fname, function_is_program_entry)));
    }
    //for all other locs, insert lr_status_top
    for (auto const& l : graph->get_locs()) {
      if (!loc_map.count(l.first)) {
        loc_map.insert(make_pair(l.first, lr_status_t::lr_status_top()));
      }
    }
    return alias_val_t(loc_map);
  }

  bool alias_val_meet(alias_val_t const &other)
  {
    stopwatch_run(&alias_val_meet_timer);
    bool changed = false;
    for (const auto &l : other.m_loc_map) {
      ASSERT(m_loc_map.count(l.first));
      /*if (m_loc_map.count(l.first)) */{ //if l.first is not present in m_loc_map, then it indicates bottom (so leave it unchanged)
        auto old_val = m_loc_map.at(l.first);
        m_loc_map.at(l.first) = lr_status_t::lr_status_meet(m_loc_map.at(l.first), l.second);
        //changed = changed || !old_val.equals(m_loc_map.at(l.first));
        changed = changed || old_val != m_loc_map.at(l.first);
      }
    }

    //TODO: remove all bottom values from m_loc_map
    stopwatch_stop(&alias_val_meet_timer);
    return changed;
  }

  bool operator!=(alias_val_t<T_PC, T_N, T_E, T_PRED> const &other) const
  {
    if (this->m_loc_map.size() != other.m_loc_map.size()) {
      return true;
    }
    for (const auto &loc_id_status : this->m_loc_map) {
      if (!other.m_loc_map.count(loc_id_status.first)) {
        return true;
      }
      //if (!other.m_loc_map.at(loc_id_status.first).equals(loc_id_status.second))
      if (other.m_loc_map.at(loc_id_status.first) != loc_id_status.second) {
        return true;
      }
    }
    return false;
  }

  string to_string(context* ctx, map<graph_loc_id_t, graph_cp_location> const& locs, string prefix) const
  {
    stringstream ss;
    for (map<graph_loc_id_t, lr_status_ref>::const_iterator it = m_loc_map.begin();
         it != m_loc_map.end();
         it++) {
      //graph_cp_location const& loc = graph->get_loc(/*p, */it->first);
      graph_cp_location const& loc = locs.at(/*p, */it->first);
      lr_status_ref const &lrs = it->second;
      ss << prefix << " loc" << it->first << ": " << loc.to_string() << ": " << lrs->lr_status_to_string(ctx->get_consts_struct()) << endl;
    }
    return ss.str();
  }
};

//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//class alias_dfa_t : public data_flow_analysis<T_PC, T_N, T_E, alias_val_t<T_PC,T_N,T_E,T_PRED>, dfa_dir_t::forward>
//{
//private:
//  bool m_update_callee_memlabels;
//  graph_memlabel_map_t &m_memlabel_map;
//public:
//  alias_dfa_t(graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const* graph, bool update_callee_memlabels, graph_memlabel_map_t &memlabel_map, map<T_PC, alias_val_t<T_PC, T_N, T_E, T_PRED>> &init_vals) : data_flow_analysis<T_PC, T_N, T_E, alias_val_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::forward>(graph, init_vals), m_update_callee_memlabels(update_callee_memlabels), m_memlabel_map(memlabel_map)
//  { }
//
//  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, alias_val_t<T_PC, T_N, T_E, T_PRED> const &in, alias_val_t<T_PC, T_N, T_E, T_PRED> &out) override
//  {
//    NOT_IMPLEMENTED();
//  }
//};

//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//class alias_analysis/* : public data_flow_analysis<T_PC, T_N, T_E, alias_val_t<T_PC,T_N,T_E,T_PRED>, dfa_dir_t::forward>*/
//{
//private:
//  graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const* m_graph;
//  map<T_PC, alias_val_t<T_PC, T_N, T_E, T_PRED>> m_vals;
//  bool m_update_callee_memlabels;
//  graph_memlabel_map_t &m_memlabel_map;
//public:
//  alias_analysis(graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const* g, bool update_callee_memlabels, graph_memlabel_map_t &memlabel_map)
//    : m_graph(g), m_vals(), m_update_callee_memlabels(update_callee_memlabels), m_memlabel_map(memlabel_map)
//  {
//    /*data_flow_analysis<T_PC, T_N, T_E, alias_val_t<T_PC,T_N,T_E,T_PRED>, dfa_dir_t::forward>(g, alias_val_t<T_PC,T_N,T_E,T_PRED>::start_pc_val(g)), */
//  }
//
//  alias_analysis(alias_analysis const &other) = delete;
//
//  alias_analysis(graph_with_aliasing<T_PC, T_N, T_E, T_PRED> *g, alias_analysis const &other) : m_graph(g), m_vals(other.m_vals), m_update_callee_memlabels(other.m_update_callee_memlabels), m_memlabel_map(g->get_memlabel_map_ref())
//  {
//  }
//
//  //lr_status_ref
//  //alias_analysis_get_lr_status_for_loc_at_pc(graph_loc_id_t loc_id, T_PC const& p) const
//  //{
//  //  if (!m_vals.count(p)) {
//  //    DYN_DEBUG2(tfg_llvm_alias, cout << __func__ << " " << __LINE__ << ": returning nullptr for PC " << p.to_string() << endl);
//  //    return nullptr;
//  //  }
//  //  if (!m_vals.at(p).loc_exists(loc_id)) {
//  //    DYN_DEBUG2(tfg_llvm_alias, cout << __func__ << " " << __LINE__ << ": returning nullptr for loc_id " << loc_id << " at PC " << p.to_string() << endl);
//  //    return nullptr;
//  //  }
//  //  DYN_DEBUG2(tfg_llvm_alias, cout << __func__ << " " << __LINE__ << ": returning non-null lr_status for loc_id " << loc_id << " at PC " << p.to_string() << endl);
//  //  return m_vals.at(p).get_loc_status(loc_id);
//  //}
//
//  map<T_PC, map<graph_loc_id_t, lr_status_ref>>
//  alias_analysis_get_lr_status_map() const
//  {
//    map<T_PC, map<graph_loc_id_t, lr_status_ref>> ret;
//    for (auto const& p_v : m_vals) {
//      T_PC const& p = p_v.first;
//      alias_val_t<T_PC,T_N,T_E,T_PRED> const& av = p_v.second;
//      for (auto const& loc_status : av.get_loc_map()) {
//        ret[p].insert(loc_status);
//      }
//    }
//    return ret;
//  }
//
//  //bool incremental_solve(shared_ptr<T_E const> const &e)
//  //{
//  //  alias_dfa_t<T_PC, T_N, T_E, T_PRED> dfa(m_graph, m_update_callee_memlabels, m_memlabel_map, m_vals);
//  //  return dfa.incremental_solve({e});
//  //}
//
//  bool solve()
//  {
//    alias_dfa_t<T_PC, T_N, T_E, T_PRED> dfa(m_graph, m_update_callee_memlabels, m_memlabel_map, m_vals);
//    dfa.initialize(alias_val_t<T_PC, T_N, T_E, T_PRED>::start_pc_val(m_graph));
//    return dfa.solve();
//  }
//
//  //void reset(bool update_callee_memlabels)
//  //{
//  //  m_update_callee_memlabels = update_callee_memlabels;
//  //  this->m_vals.clear();
//  //  //m_graph->node_update_memlabel_map_for_mlvars(T_PC::start(), m_vals.at(T_PC::start()).get_loc_map(), update_callee_memlabels, m_memlabel_map);
//  //}
//
//  map<T_PC, alias_val_t<T_PC, T_N, T_E, T_PRED>> const &get_values() const { return m_vals; }
//};

}
