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

  static alias_val_t top()
  {
    return alias_val_t();
  }

  map<graph_loc_id_t, lr_status_ref> const &get_loc_map() const { return m_loc_map; }
  lr_status_ref const &get_loc_status(graph_loc_id_t loc_id) const { return m_loc_map.at(loc_id); }
  bool loc_exists(graph_loc_id_t loc_id) const { return m_loc_map.count(loc_id) != 0; }
  static alias_val_t start_pc_val(graph_with_aliasing_instance const *graph)
  {
    //autostop_timer func_timer(__func__);
    set<cs_addr_ref_id_t> const &relevant_addr_refs = graph->graph_get_relevant_addr_refs();
    context *ctx = graph->get_context();
    consts_struct_t const &cs = ctx->get_consts_struct();
    map<graph_loc_id_t, lr_status_ref> loc_map;
    for (auto loc : graph->get_live_locids(T_PC::start())) {
      loc_map.insert(make_pair(loc, lr_status_t::start_pc_value(graph->graph_loc_get_value_for_node(loc), relevant_addr_refs, ctx, graph->get_argument_regs(), graph->get_symbol_map())));
    }
    return alias_val_t(loc_map);
  }

  bool alias_val_meet(alias_val_t const &other)
  {
    autostop_timer func_timer(__func__);
    bool changed = false;
    for (const auto &l : other.m_loc_map) {
      if (!m_loc_map.count(l.first)) {
        m_loc_map.insert(l);
        changed = true;
      } else {
        auto old_val = m_loc_map.at(l.first);
        m_loc_map.at(l.first) = lr_status_t::lr_status_meet(m_loc_map.at(l.first), l.second);
        //changed = changed || !old_val.equals(m_loc_map.at(l.first));
        changed = changed || old_val != m_loc_map.at(l.first);
      }
    }
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

  string to_string(graph_with_aliasing_instance const *graph/*, T_PC const &p*/, string prefix) const
  {
    graph_cp_location loc;
    stringstream ss;
    for (map<graph_loc_id_t, lr_status_ref>::const_iterator it = m_loc_map.begin();
         it != m_loc_map.end();
         it++) {
      loc = graph->get_loc(/*p, */it->first);
      lr_status_ref const &lrs = it->second;
      ss << prefix << " loc" << it->first << ": " << loc.to_string() << ": " << lrs->lr_status_to_string(graph->get_consts_struct()) << endl;
    }
    return ss.str();
  }
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class alias_dfa_t : public data_flow_analysis<T_PC, T_N, T_E, alias_val_t<T_PC,T_N,T_E,T_PRED>, dfa_dir_t::forward>
{
private:
  bool m_update_callee_memlabels;
  graph_memlabel_map_t &m_memlabel_map;
public:
  alias_dfa_t(graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const* graph, bool update_callee_memlabels, graph_memlabel_map_t &memlabel_map, map<T_PC, alias_val_t<T_PC, T_N, T_E, T_PRED>> &init_vals) : data_flow_analysis<T_PC, T_N, T_E, alias_val_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::forward>(graph, init_vals), m_update_callee_memlabels(update_callee_memlabels), m_memlabel_map(memlabel_map)
  { }

  virtual bool xfer_and_meet(shared_ptr<T_E const> const &e, alias_val_t<T_PC, T_N, T_E, T_PRED> const &in, alias_val_t<T_PC, T_N, T_E, T_PRED> &out) override
  {
    autostop_timer func_timer(string("alias_analysis.") + __func__);
    DYN_DEBUG2(alias_analysis, cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl);

    graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const *g = dynamic_cast<graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const *>(this->m_graph);
    ASSERT(g);
    map<graph_loc_id_t, lr_status_ref> const &in_loc_map = in.get_loc_map();
    DYN_DEBUG2(alias_analysis, cout << __func__ << " " << __LINE__ << ": in =\n" << in.to_string(g, "\t") << endl);

    //g->node_update_memlabel_map_for_mlvars(e->get_from_pc(), in_loc_map, m_update_callee_memlabels, m_memlabel_map); //not required; we are updating the memlabels at to_pc for each edge anyways
    g->edge_update_memlabel_map_for_mlvars(e, in_loc_map, m_update_callee_memlabels, m_memlabel_map);

    map<graph_loc_id_t, lr_status_ref> ret = g->compute_new_lr_status_on_locs(e, in_loc_map);
    bool changed = out.alias_val_meet(alias_val_t<T_PC, T_N, T_E, T_PRED>(ret));
    DYN_DEBUG2(alias_analysis, if (changed) cout << __func__ << ':' << __LINE__ << ": out =\n" << out.to_string(g, "\t") << endl);
    if (changed) {
      map<graph_loc_id_t, lr_status_ref> const &out_loc_map = out.get_loc_map();
      g->node_update_memlabel_map_for_mlvars(e->get_to_pc(), out_loc_map, m_update_callee_memlabels, m_memlabel_map);
    }

    return changed;
  }
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class alias_analysis/* : public data_flow_analysis<T_PC, T_N, T_E, alias_val_t<T_PC,T_N,T_E,T_PRED>, dfa_dir_t::forward>*/
{
private:
  graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const* m_graph;
  map<T_PC, alias_val_t<T_PC, T_N, T_E, T_PRED>> m_vals;
  bool m_update_callee_memlabels;
  graph_memlabel_map_t &m_memlabel_map;
public:
  alias_analysis(graph_with_aliasing<T_PC, T_N, T_E, T_PRED> const* g, bool update_callee_memlabels, graph_memlabel_map_t &memlabel_map)
    : m_graph(g), m_vals(), m_update_callee_memlabels(update_callee_memlabels), m_memlabel_map(memlabel_map)
  {
    /*data_flow_analysis<T_PC, T_N, T_E, alias_val_t<T_PC,T_N,T_E,T_PRED>, dfa_dir_t::forward>(g, alias_val_t<T_PC,T_N,T_E,T_PRED>::start_pc_val(g)), */
  }

  alias_analysis(alias_analysis const &other) = delete;

  alias_analysis(graph_with_aliasing<T_PC, T_N, T_E, T_PRED> *g, alias_analysis const &other) : m_graph(g), m_vals(other.m_vals), m_update_callee_memlabels(other.m_update_callee_memlabels), m_memlabel_map(g->get_memlabel_map_ref())
  {
  }

  bool incremental_solve(shared_ptr<T_E const> const &e)
  {
    alias_dfa_t<T_PC, T_N, T_E, T_PRED> dfa(m_graph, m_update_callee_memlabels, m_memlabel_map, m_vals);
    return dfa.incremental_solve(e);
  }

  bool solve()
  {
    alias_dfa_t<T_PC, T_N, T_E, T_PRED> dfa(m_graph, m_update_callee_memlabels, m_memlabel_map, m_vals);
    return dfa.solve();
  }

  void reset(bool update_callee_memlabels)
  {
    m_update_callee_memlabels = update_callee_memlabels;
    this->m_vals.clear();
    alias_dfa_t<T_PC, T_N, T_E, T_PRED> dfa(m_graph, m_update_callee_memlabels, m_memlabel_map, m_vals);
    dfa.initialize(alias_val_t<T_PC, T_N, T_E, T_PRED>::start_pc_val(m_graph));
    m_graph->node_update_memlabel_map_for_mlvars(T_PC::start(), m_vals.at(T_PC::start()).get_loc_map(), update_callee_memlabels, m_memlabel_map);
  }

  map<T_PC, alias_val_t<T_PC, T_N, T_E, T_PRED>> const &get_values() const { return m_vals; }
};

}
