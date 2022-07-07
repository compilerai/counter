#pragma once

#include "support/dyn_debug.h"
#include "support/searchable_queue.h"
#include "support/dshared_ptr.h"

#include "gsupport/edge_id.h"
#include "gsupport/edge_with_graph_ec_no_unroll.h"

#include "graph/dfa_without_regions.h"
#include "graph/graph_with_paths.h"

using namespace std;

namespace eqspace {

template <typename T_PC, typename T_N, typename T_E>
class graph_with_regions_t;

template<typename T_PC, typename T_E>
class graph_ec_unroll_t;

//template<typename T_PC, typename T_E, typename T_VAL>
//class val_with_ec_t
//{
//private:
//  mutable T_VAL m_val;
//  mutable T_VAL m_anchor_node_val;
//  mutable dshared_ptr<graph_ec_unroll_t<T_PC, T_E>> m_subgraph;
//  mutable graph_edge_composition_ref<T_PC, T_E> m_ec;
//public:
//  val_with_ec_t(T_VAL const& v, dshared_ptr<graph_ec_unroll_t<T_PC, T_E>> const& subgraph) : m_val(v), m_anchor_node_val(v), m_subgraph(subgraph), m_ec(m_subgraph->graph_ec_unroll_get_ec_from_source_to_sink())
//  { }
//  val_with_ec_t(val_with_ec_t const& other) : m_val(other.m_val), m_anchor_node_val(other.m_anchor_node_val), m_subgraph(make_dshared<graph_ec_unroll_t<T_PC, T_E>>(*other.m_subgraph)), m_ec(other.m_ec)
//  { }
//  static val_with_ec_t top()
//  {
//    return val_with_ec_t<T_PC, T_E, T_VAL>(T_VAL::top(), dshared_ptr<graph_ec_unroll_t<T_PC, T_E>>::dshared_nullptr());
//  }
//  void set_val(T_VAL const& v) const { m_val = v; }
//  void set_anchor_node_val(T_VAL const& v) const { m_anchor_node_val = v; }
//  T_VAL& get_val() { return m_val; }
//  T_VAL& get_val_ref() const { return m_val; }
//  T_VAL const& get_val() const { return m_val; }
//  T_VAL& get_anchor_node_val() { return m_anchor_node_val; }
//  T_VAL const& get_anchor_node_val() const { return m_anchor_node_val; }
//
//  dshared_ptr<graph_ec_unroll_t<T_PC, T_E> const> get_subgraph() const { return m_subgraph; }
//  graph_edge_composition_ref<T_PC, T_E> const& get_ec() const { return m_ec; }
//  void set_subgraph(dshared_ptr<graph_ec_unroll_t<T_PC, T_E> const> const& subgraph) const
//  {
//    m_subgraph = make_dshared<graph_ec_unroll_t<T_PC, T_E>>(*subgraph);
//    m_ec = m_subgraph->graph_ec_unroll_get_ec_from_source_to_sink();
//  }
//  bool subgraph_union(dshared_ptr<graph_ec_unroll_t<T_PC, T_E> const> const& subgraph)
//  {
//    bool changed = m_subgraph->append_graph(subgraph);
//    if (changed) {
//      m_ec = m_subgraph->graph_ec_unroll_get_ec_from_source_to_sink();
//    }
//    return changed;
//  }
//  bool subgraph_is_empty() const
//  {
//    return m_subgraph->get_nodes().size() == 0;
//  }
//
//  //void subgraph_add_node(dshared_ptr<T_N> const& n)
//  //{
//  //  m_subgraph->add_node(n);
//  //}
//  bool subgraph_add_edge(dshared_ptr<T_E const> const& e)
//  {
//    int from_pc_count = 0;
//    int to_pc_count = (m_subgraph->get_all_pcs().size() && e->get_to_pc() == m_subgraph->graph_ec_unroll_get_source_pc().get_pc()) ? 1 : 0;
//    bool edge_added = m_subgraph->graph_ec_unroll_add_edge(e, from_pc_count, to_pc_count);
//    m_ec = m_subgraph->graph_ec_unroll_get_ec_from_source_to_sink();
//    return edge_added;
//  }
//
//  //void meet(T_VAL const& other_val, graph_edge_composition_ref<T_PC, T_E> const& other_ec)
//  //{
//  //  DYN_DEBUG(dfa_over_paths_debug, cout << _FNLN_ << ": m_ec = " << (m_ec ? m_ec->graph_edge_composition_to_string() : "nullptr") << endl);
//  //  DYN_DEBUG(dfa_over_paths_debug, cout << _FNLN_ << ": other_ec = " << (other_ec ? other_ec->graph_edge_composition_to_string() : "nullptr") << endl);
//  //  if (!m_ec || m_ec->is_epsilon() || (m_val != other_val)) {
//  //    m_val = other_val;
//  //    m_ec = other_ec;
//  //    //return DFA_XFER_RETVAL_CHANGED;
//  //  } else {
//  //    auto orig_ec = m_ec;
//  //    m_ec = graph_edge_composition_t<T_PC, T_E>::mk_parallel(m_ec, other_ec);
//  //    //return m_ec == orig_ec ? DFA_XFER_RETVAL_UNCHANGED : DFA_XFER_RETVAL_CHANGED;
//  //  }
//  //}
//  static map<T_PC, val_with_ec_t<T_PC, T_E, T_VAL>> construct_initial_values(map<T_PC, T_VAL> const& invals, dshared_ptr<graph_ec_unroll_t<T_PC, T_E>> const& g)
//  {
//    map<T_PC, val_with_ec_t<T_PC, T_E, T_VAL>> ret;
//    for (auto const& pv : invals) {
//      dshared_ptr<graph_ec_unroll_t<T_PC, T_E>> subgraph = make_dshared<graph_ec_unroll_t<T_PC, T_E>>();
//      ret.insert(make_pair(pv.first, val_with_ec_t(pv.second, subgraph)));
//    }
//    return ret;
//  }
//};

template <typename T_PC, typename T_N, typename T_E, typename T_VAL, dfa_dir_t DIR>
class dfa_over_paths : public data_flow_analysis<T_PC, T_N, edge_with_graph_ec_no_unroll_t<T_PC,T_E>, T_VAL, DIR>
{
protected:
  dshared_ptr<graph_with_paths<T_PC, T_N, T_E, predicate> const> m_original_graph;
public:

  dfa_over_paths(dfa_over_paths const &other) = delete;

  dfa_over_paths(dshared_ptr<graph_with_paths<T_PC, T_N, T_E, predicate> const> g, map<T_PC, T_VAL>& init_vals)
    : data_flow_analysis<T_PC, T_N, edge_with_graph_ec_no_unroll_t<T_PC,T_E>, T_VAL, DIR>(g->graph_get_reduced_graph_with_anchor_nodes_only(), init_vals), m_original_graph(g)
  {
    //populate_init_vals_with_pc_unroll(init_vals);
    //for (auto const& p : init_vals) {
    //  if (!this->m_graph->pc_is_anchor_pc(p.first)) {
    //    for (auto const& pp : init_vals) {
    //      cout << "pp.first = "  << pp.first.to_string() << endl;
    //    }
    //    cout << _FNLN_ << ": p.first = " << p.first.to_string() << endl;
    //  }
    //  ASSERT(this->m_graph->pc_is_anchor_pc(p.first));
    //}
  }

  //virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, val_with_ec_t<T_PC, T_E, T_VAL> const& in, val_with_ec_t<T_PC, T_E, T_VAL> &out) override
  //{
  //  //auto ec = graph_edge_composition_t<T_PC, T_E>::mk_series(in_ec, graph_edge_composition_t<T_PC, T_E>::mk_edge_composition(e));

  //  //cout << _FNLN_ << ": e = " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << endl;
  //  //cout << "in:\n" << in.get_val().invariant_state_to_string("  ") << endl;
  //  //cout << "out:\n" << out.get_val().invariant_state_to_string("  ") << endl;

  //  if (this->m_graph->pc_is_anchor_pc(e->get_from_pc()) && !in.subgraph_is_empty()) {
  //    auto ec = in.get_subgraph()->graph_ec_unroll_get_ec_from_source_to_sink();
  //    dfa_xfer_retval_t ret = xfer_and_meet_over_path(ec, in.get_anchor_node_val(), in.get_val_ref());
  //    in.set_subgraph(make_dshared<graph_ec_unroll_t<T_PC, T_E>>());
  //    in.set_anchor_node_val(in.get_val());
  //  }

  //  auto in_xferred = in;
  //  bool edge_added = in_xferred.subgraph_add_edge(e);
  //  ASSERT(edge_added);
  //  if (out.get_anchor_node_val() != in.get_anchor_node_val()) {
  //    out.set_anchor_node_val(in.get_anchor_node_val());
  //    out.set_subgraph(in_xferred.get_subgraph());
  //    return DFA_XFER_RETVAL_CHANGED;
  //  } else {
  //    return out.subgraph_union(in_xferred.get_subgraph()) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  //  }
  //}

  //bool solve(list<T_PC> const& start_pcs = {})
  //{
  //  list<T_PC> start_pcs_with_unroll;
  //  for (auto const& p : start_pcs) {
  //    start_pcs_with_unroll.push_back(pc_with_unroll<T_PC>(p, 0));
  //  }
  //  return this->template dfa_without_regions_t<pc_with_unroll<T_PC>, T_N, edge_with_graph_ec_t<T_PC, T_E>, val_with_ec_t<T_PC, T_E, T_VAL>, DIR>::solve(start_pcs_with_unroll);
  //}

  bool solve_for_worklist(list<dshared_ptr<T_E const>> const& initial_worklist = {}, list<T_PC>* externally_modified_pcs = nullptr)
  {
    list<dshared_ptr<edge_with_graph_ec_no_unroll_t<T_PC, T_E> const>> ec_worklist;
    for (auto const& e : initial_worklist) {
      list_union_append(ec_worklist, this->identify_edge_with_graph_ecs_no_unroll_containing_edge(e));
    }

    std::function<list<T_PC> (void)> pop_externally_modified_pcs = [externally_modified_pcs](void)
    {
      list<T_PC> ret;
      if (externally_modified_pcs) {
        for (auto const& p : *externally_modified_pcs) {
          ret.push_back(p);
        }
        externally_modified_pcs->clear();
      }
      return ret;
    };

    //this->add_to_worklist_edges_due_to_externally_modified_pcs(ec_worklist, pop_externally_modified_pcs);

    using dfa = data_flow_analysis<T_PC, T_N, edge_with_graph_ec_no_unroll_t<T_PC, T_E>, T_VAL, DIR>;
    return this->dfa::solve_for_worklist(ec_worklist, pop_externally_modified_pcs);
  }

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<edge_with_graph_ec_no_unroll_t<T_PC,T_E> const> const &e, T_VAL const& in, T_VAL &out) override
  {
    return xfer_and_meet_over_path(e->get_ec(), in, out);
  }

  virtual dfa_xfer_retval_t xfer_and_meet_over_path(graph_edge_composition_ref<T_PC,T_E> const &e, T_VAL const& in, T_VAL &out) = 0;

private:

  //void populate_init_vals_with_pc_unroll(map<T_PC, val_with_ec_t<T_PC, T_E, T_VAL>> const& init_vals)
  //{
  //  m_init_vals_with_pc_unroll.clear();
  //  for (auto const& pv : init_vals) {
  //    m_init_vals_with_pc_unroll.insert(make_pair(pc_with_unroll<T_PC>(pv.first, 0), pv.second));
  //  }
  //}

  list<dshared_ptr<edge_with_graph_ec_no_unroll_t<T_PC, T_E> const>> identify_edge_with_graph_ecs_no_unroll_containing_edge(dshared_ptr<T_E const> e)
  {
    list<dshared_ptr<edge_with_graph_ec_no_unroll_t<T_PC, T_E> const>> ret;
    for (auto const& edge_with_graph_ec_no_unroll : this->m_graph->get_edges_outgoing(e->get_from_pc())) {
      if (edge_with_graph_ec_no_unroll->edge_with_graph_ec_no_unroll_contains_edge_at_from_pc(e)) {
        ret.push_back(edge_with_graph_ec_no_unroll);
      }
    }
    return ret;
  }
};

}
