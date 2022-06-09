#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <map>
#include <memory>

#include "support/dyn_debug.h"
#include "support/dshared_ptr.h"

#include "graph/dfa_types.h"
#include "graph/graph_with_regions.h"
#include "graph/graph_region_type.h"

using namespace std;

namespace eqspace {

template <typename T_PC, typename T_N, typename T_E>
class graph;

template <typename T_PC, typename T_N, typename T_E>
class graph_with_regions_t;

template <typename T_PC, typename T_N, typename T_E>
class graph_region_t;

template <typename T_PC, typename T_N, typename T_E>
class graph_region_node_t;

template <typename T_PC, typename T_N, typename T_E>
class graph_region_edge_t;

template <typename T_PC, typename T_E>
class graph_region_pc_t;

template <typename T_PC, typename T_N, typename T_E, typename T_VAL, dfa_dir_t DIR, dfa_state_type_t DFA_STATE_TYPE>
class dfa_flow_insensitive
{
protected:
  dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> m_graph;
  T_VAL &m_vals;
private:
  list<dshared_ptr<T_E const>> m_postorder_edges;
  set<T_PC> m_pcs_that_may_need_a_visit; //used only for dfa_state_type_t::global
  //set<T_PC> m_visited;
public:

  dfa_flow_insensitive(dfa_flow_insensitive const &other) = delete;

  dfa_flow_insensitive(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g, T_VAL& init_vals)
    : m_graph(g),
      m_vals(init_vals)
  {
    ASSERT(g);
    //ASSERT(g->graph_regions_are_consistent());
    m_postorder_edges = g->compute_postorder();
    //ASSERT(m_postorder_edges.size() == g->get_edges().size());
    //ASSERT(list_intersect((list<dshared_ptr<T_E const>> const)m_postorder_edges, g->get_edges()).size() == m_postorder_edges.size());
  }

  virtual bool postprocess(bool changed) { return changed; }

  virtual dfa_xfer_retval_t xfer_and_meet_flow_insensitive(dshared_ptr<T_E const> const &e, T_VAL &in_out) = 0;

  bool solve(list<T_PC> start_pcs = {})
  {
    if (DFA_STATE_TYPE == dfa_state_type_t::global) {
      this->mark_all_pcs_may_need_a_visit();
    }

    //m_visited.clear();
    if (!start_pcs.size()) {
      if (DIR == dfa_dir_t::forward) {
        start_pcs.push_back(m_graph->get_entry_pc());
      } else if (DIR == dfa_dir_t::backward) {
        m_graph->get_exit_pcs(start_pcs);
        if (!start_pcs.size()) {
          for (auto const& p : m_graph->get_all_pcs()) {
            start_pcs.push_back(p);
          }
        }
      }
    }
    ASSERT(start_pcs.size());

    std::function<list<T_PC> (void)> pop_externally_modified_pcs = [](void) { return list<T_PC>(); };

    dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> top_node = m_graph->get_top_level_region_node();
    //list<dshared_ptr<T_E const>> next_edges;
    ASSERT(top_node);
    pair<bool, list<dshared_ptr<T_E const>>> pr = this->solve_for_region_node(top_node, start_pcs/*, next_edges*/, pop_externally_modified_pcs);
    ASSERT(pr.second.size() == 0);
    //bool changed = fixed_point_iteration(p_worklist/*.first, p_worklist.second*/);
    return postprocess(pr.first);
  }

  bool solve_for_worklist(list<dshared_ptr<T_E const>> const& initial_worklist, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    DYN_DEBUG(dfa,
      cout << demangled_type_name(typeid(*this)) << "::" << _FNLN_ << ": initial_worklist =\n";
      for (auto const& e : initial_worklist) {
        cout << "  " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << endl;
      }
    );


    list<dshared_ptr<T_E const>> p_worklist = initial_worklist;
    this->add_to_worklist_edges_due_to_externally_modified_pcs(p_worklist, pop_externally_modified_pcs);
    ASSERT(pop_externally_modified_pcs().size() == 0); //there should be no more externally modified pcs left now
    //bool changed = fixed_point_iteration(p_worklist, pop_externally_modified_pcs);
    dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> top_node = m_graph->get_top_level_region_node();
    graph_region_t<T_PC, T_N, T_E> const& top_node_region = top_node->get_region();
    graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g = *top_node_region.get_region_graph();
    list<dshared_ptr<T_E const>> next_edges;
    if (DIR == dfa_dir_t::forward) {
      next_edges = top_node->get_pc().region_pc_get_outgoing_edges();
    } else if (DIR == dfa_dir_t::backward) {
      next_edges = top_node->get_pc().region_pc_get_incoming_edges();
    } else NOT_REACHED();

    list<dshared_ptr<T_E const>> external_edges;
    bool changed = graph_dfa_iterate_on_worklist_till_fixpoint(g, p_worklist, next_edges, external_edges, pop_externally_modified_pcs);
    ASSERT(external_edges.size() == 0);
    return postprocess(changed);
  }

private:
  static string edges_to_string(list<dshared_ptr<T_E const>>& es)
  {
    stringstream ss;
    ss << "{";
    for (auto const& e : es) {
      ss << " " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << ";";
    }
    ss << "}";
    return ss.str();
  }

  pair<bool, list<dshared_ptr<T_E const>>> solve_for_region_node(dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> const& region_node, list<T_PC> const& start_pcs/*, list<dshared_ptr<T_E const>>& next_edges*/, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    list<dshared_ptr<T_E const>> next_edges;
    if (DIR == dfa_dir_t::forward) {
      next_edges = region_node->get_pc().region_pc_get_outgoing_edges();
    } else if (DIR == dfa_dir_t::backward) {
      next_edges = region_node->get_pc().region_pc_get_incoming_edges();
    } else NOT_REACHED();
    //if (containing_graph) {
    //  if (DIR == dfa_dir_t::forward) {
    //    next_edges = graph_get_outgoing_edges_for_region_pc(*containing_graph, region_node->get_pc());
    //  } else if (DIR == dfa_dir_t::backward) {
    //    next_edges = graph_get_incoming_edges_for_region_pc(*containing_graph, region_node->get_pc());
    //  } else NOT_REACHED();
    //}
    graph_region_t<T_PC, T_N, T_E> const& region = region_node->get_region();

    if (region.get_type() == graph_region_type_instruction) {
      ASSERT(start_pcs.size() == 1);
      ASSERT(start_pcs.front() == region_node->get_pc().region_pc_get_entry_pc());
      DYN_DEBUG(dfa, cout << _FNLN_ << ": solving for instruction region at " << start_pcs.front().to_string() << ", next_edges = " << edges_to_string(next_edges) << endl);
      return make_pair(true, next_edges);
    } else {
      graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g = *region.get_region_graph();
      list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>> start_nodes_and_their_pcs = identify_region_nodes_for_pcs(g, start_pcs);

      if (region.get_type() == graph_region_type_procedure) {
        DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << _FNLN_ << ": solving for procedure region at " << start_pcs.front().to_string() << endl);
        return make_pair(solve_for_procedure_region(g, start_nodes_and_their_pcs, next_edges, pop_externally_modified_pcs), next_edges);
      } else if (region.get_type() == graph_region_type_loop) {
        DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << _FNLN_ << ": solving for loop region at " << start_pcs.front().to_string() << endl);
        return make_pair(solve_for_loop_region(g, start_nodes_and_their_pcs, next_edges, pop_externally_modified_pcs), next_edges);
      } else if (region.get_type() == graph_region_type_loop_body) {
        DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << _FNLN_ << ": solving for loop region at " << start_pcs.front().to_string() << endl);
        return make_pair(solve_for_loop_body_region(g, start_nodes_and_their_pcs, next_edges, pop_externally_modified_pcs), next_edges);
      } else if (region.get_type() == graph_region_type_bbl) {
        DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << _FNLN_ << ": solving for bbl region at " << start_pcs.front().to_string() << endl);
        return make_pair(solve_for_bbl_region(g, start_nodes_and_their_pcs, next_edges, pop_externally_modified_pcs), next_edges);
      } else NOT_REACHED();
    }
  }

  static string start_nodes_and_their_pcs_to_string(list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>> const& start_nodes_and_their_pcs)
  {
    stringstream ss;
    for (auto const& start_node_and_its_pcs : start_nodes_and_their_pcs) {
      ss << start_node_and_its_pcs.first->get_pc().to_string() << "{";
      for (auto const& p : start_node_and_its_pcs.second) {
        ss << " " << p.to_string();
      }
      ss << "} ; ";
    }
    return ss.str();
  }

  virtual bool solve_for_procedure_region(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>> const& start_nodes_and_their_pcs, list<dshared_ptr<T_E const>>& next_edges, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    DYN_DEBUG(dfa, cout << _FNLN_ << ": base implementation: " << start_nodes_and_their_pcs_to_string(start_nodes_and_their_pcs) << endl);
    return fixed_point_iteration(g, start_nodes_and_their_pcs, next_edges, pop_externally_modified_pcs);
  }

  virtual bool solve_for_loop_region(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>> const& start_nodes_and_their_pcs, list<dshared_ptr<T_E const>>& next_edges, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    DYN_DEBUG(dfa, cout << _FNLN_ << ": base implementation: " << start_nodes_and_their_pcs_to_string(start_nodes_and_their_pcs) << endl);
    return fixed_point_iteration(g, start_nodes_and_their_pcs, next_edges, pop_externally_modified_pcs);
  }

  virtual bool solve_for_loop_body_region(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>> const& start_nodes_and_their_pcs, list<dshared_ptr<T_E const>>& next_edges, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    DYN_DEBUG(dfa, cout << _FNLN_ << ": base implementation: " << start_nodes_and_their_pcs_to_string(start_nodes_and_their_pcs) << endl);
    return fixed_point_iteration(g, start_nodes_and_their_pcs, next_edges, pop_externally_modified_pcs);
  }

  virtual bool solve_for_bbl_region(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>> const& start_nodes_and_their_pcs, list<dshared_ptr<T_E const>>& next_edges, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    DYN_DEBUG(dfa, cout << _FNLN_ << ": base implementation: " << start_nodes_and_their_pcs_to_string(start_nodes_and_their_pcs) << endl);
    return fixed_point_iteration(g, start_nodes_and_their_pcs, next_edges, pop_externally_modified_pcs);
  }

  void mark_all_pcs_may_need_a_visit()
  {
    for (auto const& p : this->m_graph->get_all_pcs()) {
      m_pcs_that_may_need_a_visit.insert(p);
    }
  }

  list<dshared_ptr<T_E const>> solve_per_edge(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, dshared_ptr<T_E const> e, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " visiting " << e->to_string() << endl);

    //m_visited.insert(e->get_from_pc());
    T_PC from_pc, to_pc;
    if (DIR == dfa_dir_t::forward) {
      from_pc = e->get_from_pc();
      to_pc = e->get_to_pc();
    } else {
      ASSERT(DIR == dfa_dir_t::backward);
      // notion of {from,to}_pc is swapped in case of backward dfa
      from_pc = e->get_to_pc();
      to_pc = e->get_from_pc();
    }

    //if (!m_vals.count(from_pc)) {
    //  auto ret = m_vals.insert(make_pair(from_pc, T_VAL::top()));
    //  //cout << __func__ << " " << __LINE__ << ": adding value at " << from_pc.to_string() << endl;
    //  ASSERT(ret.second);
    //}
    //if (!m_vals.count(to_pc)) {
    //  auto ret = m_vals.insert(make_pair(to_pc, T_VAL::top()));
    //  //cout << __func__ << " " << __LINE__ << ": adding value at " << to_pc.to_string() << endl;
    //  ASSERT(ret.second);
    //}
    //ASSERT(m_vals.count(from_pc));
    //ASSERT(m_vals.count(to_pc));

    //dfa_xfer_retval_t changed = xfer_and_meet(e, m_vals.at(from_pc), m_vals.at(to_pc));
    dfa_xfer_retval_t changed = xfer_and_meet_flow_insensitive(e, m_vals);
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " xfer_and_meet returned " << changed.to_string() << " for " << e->to_string() << endl);
    list<T_PC> externally_modified_pcs = pop_externally_modified_pcs();
    DYN_DEBUG(dfa,
        cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " externally_modified_pcs (size " << externally_modified_pcs.size() << ") =";
        for (auto const& epc : externally_modified_pcs) {
          cout << " " << epc.to_string();
        }
        cout << endl;
    );

    ASSERT(changed == dfa_xfer_retval_t::changed || externally_modified_pcs.size() == 0);
    list<dshared_ptr<T_E const>> ret;
    list<T_PC> nextpcs;

    if (DFA_STATE_TYPE == dfa_state_type_t::global) {
      if (changed == DFA_XFER_RETVAL_CHANGED) {
        this->mark_all_pcs_may_need_a_visit();
      } else {
        m_pcs_that_may_need_a_visit.erase(from_pc);
      }
      if (m_pcs_that_may_need_a_visit.count(to_pc)) {
        nextpcs.push_back(to_pc);
      }
    } else {
      if (changed == DFA_XFER_RETVAL_CHANGED) {
        nextpcs.push_back(to_pc);
      }
    }
    list_union_append(nextpcs, externally_modified_pcs);
    DYN_DEBUG(dfa,
        cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " nextpcs =";
        for (auto const& npc : nextpcs) {
          cout << " " << npc.to_string();
        }
        cout << endl;
    );
    for (auto const& p : nextpcs) {
      dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> n;

      n = find_node_with_pc(g, p);
      DYN_DEBUG(dfa,
        cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " p = " << p.to_string() << ", n = " << n << endl;
      );
      if (n) {
        list_union_append(ret, this->solve_for_region_node(n, {p}, pop_externally_modified_pcs).second);
      }
    }

    return ret;
  }

  dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const> find_node_with_pc(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, T_PC const& p)
  {
    for (auto const& n : g.get_nodes()) {
      list<T_PC> n_pcs = n->graph_region_node_get_constituent_pcs_among_these_pcs({p});
      if (n_pcs.size()) {
        return n;
      }
    }
    return dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>::dshared_nullptr();
  }

  list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>>
  identify_region_nodes_for_pcs(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, list<T_PC> const& start_pcs)
  {
    DYN_DEBUG(dfa,
      cout << _FNLN_ << ": start_pcs =";
      for (auto const& start_pc : start_pcs) {
        cout << " " << start_pc.to_string();
      }
      cout << endl;
    );
    list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>> ret;
    for (auto const& n : g.get_nodes()) {
      set<T_PC> constituent_pcs = n->graph_region_node_get_all_constituent_pcs();
      list<T_PC> n_pcs = n->graph_region_node_get_constituent_pcs_among_these_pcs(start_pcs);
      DYN_DEBUG(dfa,
        cout << _FNLN_ << ": n_pcs size " << n_pcs.size() << " for " << n->get_pc().to_string() << ":";
        for (auto const& n_pc : n_pcs) {
          cout << " " << n_pc.to_string();
        }
        cout << endl;
      );

      if (n_pcs.size()) {
        ret.push_back(make_pair(n, n_pcs));
      }
    }
    return ret;
  }

  /* Kildall's worklist algo
     we'll use a worklist for keeping track of edges whose value needs to be recalculated
     workset: lookup cache with same content as worklist

    TODO: traverse in reverse postorder (or postorder) to minimize the xfer's required
  */
  bool fixed_point_iteration(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, list<pair<dshared_ptr<graph_region_node_t<T_PC, T_N, T_E> const>, list<T_PC>>> const& start_nodes_and_their_pcs, list<dshared_ptr<T_E const>>& external_edges, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    list<dshared_ptr<T_E const>> external_edges_in = external_edges;
    external_edges.clear();
    list<dshared_ptr<T_E const>> worklist;// = get_initial_worklist(g, start_pcs);
    for (auto const& start_node_and_its_pcs : start_nodes_and_their_pcs) {
      list<dshared_ptr<T_E const>> next_es;
      auto start_node_entry = start_node_and_its_pcs.first;
      //if (DIR == dfa_dir_t::forward) {
      //  next_es = graph_get_outgoing_edges_for_region_pc(g, start_node_entry->get_pc());
      //} else if (DIR == dfa_dir_t::backward) {
      //  next_es = graph_get_incoming_edges_for_region_pc(g, start_node_entry->get_pc());
      //} else NOT_REACHED();
      next_es = solve_for_region_node(start_node_entry, start_node_and_its_pcs.second/*, next_es*/, pop_externally_modified_pcs).second;
      for (auto next_e : next_es) {
        if (list_contains(external_edges_in, next_e)) {
          external_edges.push_back(next_e);
        } else {
          worklist.push_back(next_e);
        }
      }
      //worklist.searchable_queue_push_back(next_es);
    }
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " start worklist size " << worklist.size() << "\n");
    //set<T_PC> pc_vals_modified;

    return graph_dfa_iterate_on_worklist_till_fixpoint(g, worklist, external_edges_in, external_edges, pop_externally_modified_pcs);
  }

  bool graph_dfa_iterate_on_worklist_till_fixpoint(graph<graph_region_pc_t<T_PC, T_E>, graph_region_node_t<T_PC, T_N,T_E>, graph_region_edge_t<T_PC, T_N, T_E>> const& g, list<dshared_ptr<T_E const>> const& worklist_in, list<dshared_ptr<T_E const>> const& external_edges_in, list<dshared_ptr<T_E const>>& external_edges, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    bool changed_something = false;
    auto worklist = worklist_in;
    while (!worklist.empty()) {
      // pop an EDGE to work on
      //shared_ptr<T_E const> e = worklist.front();
      //worklist.pop();
      //workset.erase(e);
      //dshared_ptr<T_E const> e = worklist.searchable_queue_pop_front();
      dshared_ptr<T_E const> e = this->m_graph->remove_next_edge_from_list_in_postorder(worklist, m_postorder_edges, DIR == dfa_dir_t::forward);

      DYN_DEBUG(dfa, cout << _FNLN_ << ": solving for edge " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << endl);
      list<dshared_ptr<T_E const>> next_edges = this->solve_per_edge(g, e, pop_externally_modified_pcs/*, pc_vals_modified*/);
      changed_something = (next_edges.size() > 0) || changed_something;

      DYN_DEBUG(dfa,
          cout << _FNLN_ << ": solved for edge " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << ", next edges:";
          for (auto const& next_e : next_edges) {
            cout << " " << next_e->get_from_pc().to_string() << "=>" << next_e->get_to_pc().to_string();
          }
          cout << endl;
      );
      for (auto const& next_e : next_edges) {
        if (list_contains(external_edges_in, next_e)) {
          if (!list_contains(external_edges, next_e)) {
            external_edges.push_back(next_e);
          }
        } else {
          worklist.push_back(next_e);
        }
      }
    }
    DYN_DEBUG(dfa,
      cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " exit.  changed_something = " << changed_something << endl;
      //cout << "pc_vals_modified (size " << pc_vals_modified.size() << "):";
      //for (auto const& p : pc_vals_modified) {
      //  cout << " " << p.to_string();
      //}
      cout << endl;
      cout << "external_edges (size " << external_edges.size() << "):";
      for (auto const& ee : external_edges) {
        cout << " " << ee->get_from_pc().to_string() << "=>" << ee->get_to_pc().to_string();
      }
      cout << endl;
    );
    //for (auto iter = external_edges_in.begin(); iter != external_edges_in.end(); iter++) {
    //  auto e = *iter;
    //  if (list_contains(external_edges, e)) {
    //    continue;
    //  }
    //  bool edge_needs_exploration;
    //  if (DIR == dfa_dir_t::forward) {
    //    edge_needs_exploration = set_belongs(pc_vals_modified, e->get_from_pc());
    //  } else if (DIR == dfa_dir_t::backward) {
    //    edge_needs_exploration = set_belongs(pc_vals_modified, e->get_to_pc());
    //  }
    //  if (edge_needs_exploration) {
    //    external_edges.push_back(e);
    //  }
    //}
    return changed_something;
  }


  //bool incremental_solve(list<dshared_ptr<T_E const>> const& eset)
  //{
  //  //queue<shared_ptr<T_E const>> worklist;   set<shared_ptr<T_E const>> workset;
  //  searchable_queue_t<dshared_ptr<T_E const>> worklist;
  //  for (auto const& e : eset) {
  //    //worklist.push(e);                  workset.insert(e);
  //    worklist.searchable_queue_push_back(e);
  //  }
  //  bool changed = this->fixed_point_iteration(worklist/*, workset*/);
  //  return this->postprocess(changed);
  //}

protected:
  void add_to_worklist_edges_due_to_externally_modified_pcs(list<dshared_ptr<T_E const>>& ret, std::function<list<T_PC> (void)> pop_externally_modified_pcs) const
  {
    list<T_PC> externally_modified_pcs = pop_externally_modified_pcs();
    DYN_DEBUG2(dfa, cout << __func__ << " " << __LINE__ << ": externally_modified_pcs size " << externally_modified_pcs.size() << endl);
    for (auto const& p : externally_modified_pcs) {
      DYN_DEBUG2(dfa, cout << __func__ << " " << __LINE__ << ": Found externally modified pc " << p.to_string() << endl);
      if (DIR == dfa_dir_t::forward) {
        list_union_append(ret, m_graph->get_edges_outgoing(p));
      } else {
        ASSERT(DIR == dfa_dir_t::backward);
        list_union_append(ret, m_graph->get_edges_incoming(p));
      }
    }
  }


private:

  list<dshared_ptr<T_E const>> get_initial_worklist(list<T_PC> const& start_pcs)
  {
    list<dshared_ptr<T_E const>> worklist;
    //set<shared_ptr<T_E const>> workset;
    if (DIR == dfa_dir_t::forward) {
      list<T_PC> entry_pcs = start_pcs;
      if (!entry_pcs.size()) {
        entry_pcs.push_back(m_graph->get_entry_pc());
      }
      for (auto const& pc : entry_pcs) {
        insert_outgoing_edges(pc, worklist/*, workset*/);
      }
    }
    else {
      ASSERT(DIR == dfa_dir_t::backward);
      list<T_PC> exit_pcs = start_pcs;
      if (!exit_pcs.size()) {
        m_graph->get_exit_pcs(exit_pcs);
        if (!exit_pcs.size()) {
          for (auto const& pc : m_graph->get_all_pcs()) {
            exit_pcs.push_back(pc);
          }
        }
      }
      for (auto const& pc : exit_pcs) {
        insert_incoming_edges(pc, worklist/*, workset*/);
      }
    }
    return worklist;
  }


  void insert_incoming_edges(T_PC pc, list<dshared_ptr<T_E const>> &worklist)
  {
    list<dshared_ptr<T_E const>> incoming;
    m_graph->get_edges_incoming(pc, incoming);
    for (auto const& e : incoming) {
      worklist.push_back(e);
    }
  }

  void insert_outgoing_edges(T_PC pc, list<dshared_ptr<T_E const>> &worklist)
  {
    list<dshared_ptr<T_E const>> outgoing;
    m_graph->get_edges_outgoing(pc, outgoing);
    for (auto const& e : outgoing) {
      worklist.push_back(e);
    }
  }

};

}
