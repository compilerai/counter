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

using namespace std;

namespace eqspace {

template <typename T_PC, typename T_N, typename T_E>
class graph;

enum dfa_dir_t { forward, backward };

template <typename T_PC, typename T_N, typename T_E, typename T_VAL, dfa_dir_t DIR>
class data_flow_analysis
{
public:

  data_flow_analysis(data_flow_analysis const &other) = delete;

  data_flow_analysis(graph<T_PC, T_N, T_E> const* g/*, T_VAL const& boundary_val*/, map<T_PC,T_VAL> & init_vals)
    : m_graph(g),
      m_vals(init_vals)
  { ASSERT(g != nullptr); }

  virtual bool postprocess(bool changed) { return changed; }

  virtual bool xfer_and_meet(shared_ptr<T_E const> const &e, T_VAL const& in, T_VAL &out) = 0;

  void initialize(std::function<T_VAL (T_PC)> boundary_val_fn)
  {
    m_vals.clear();
    if (DIR == dfa_dir_t::forward) {
      T_PC entry = m_graph->get_entry_pc();
      m_vals.insert(make_pair(entry, boundary_val_fn(entry)));
    } else {
      ASSERT(DIR == dfa_dir_t::backward);
      list<T_PC> exit_pcs;
      m_graph->get_exit_pcs(exit_pcs);
      for (auto const& pc : exit_pcs) {
        m_vals.insert(make_pair(pc, boundary_val_fn(pc)));
      }
    }
    for (const auto &p : m_graph->get_all_pcs()) {
      if (m_vals.count(p) == 0) {
        m_vals.insert(make_pair(p, T_VAL::top()));
      }
    }
  }

  void initialize(T_VAL const &boundary_val)
  {
    std::function<T_VAL (T_PC)> f = [&boundary_val](T_PC const &p) { return boundary_val; };
    this->initialize(f);
  }

  bool solve(void)
  {
    pair<queue<shared_ptr<T_E const>>,set<shared_ptr<T_E const>>> p_worklist = get_initial_worklist();
    bool changed = fixed_point_iteration(p_worklist.first, p_worklist.second);
    return postprocess(changed);
  }

  bool incremental_solve(shared_ptr<T_E const> e)
  {
    queue<shared_ptr<T_E const>> worklist;   set<shared_ptr<T_E const>> workset;
    worklist.push(e);                  workset.insert(e);
    bool changed = this->fixed_point_iteration(worklist, workset);
    return this->postprocess(changed);
  }

protected:

  graph<T_PC, T_N, T_E> const* m_graph;
  map<T_PC, T_VAL> &m_vals;

private:

  bool solve_per_edge(shared_ptr<T_E const> e)
  {
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " visiting " << e->to_string() << endl);

    T_PC from_pc, to_pc;
    if (DIR == dfa_dir_t::forward) {
      from_pc = e->get_from_pc();
      to_pc = e->get_to_pc();
    }
    else {
      ASSERT(DIR == dfa_dir_t::backward);
      // notion of {from,to}_pc is swapped in case of backward dfa
      from_pc = e->get_to_pc();
      to_pc = e->get_from_pc();
    }

    if (!m_vals.count(from_pc)) {
      auto ret = m_vals.insert(make_pair(from_pc, T_VAL::top()));
      //cout << __func__ << " " << __LINE__ << ": adding value at " << from_pc.to_string() << endl;
      ASSERT(ret.second);
    }
    if (!m_vals.count(to_pc)) {
      auto ret = m_vals.insert(make_pair(to_pc, T_VAL::top()));
      //cout << __func__ << " " << __LINE__ << ": adding value at " << to_pc.to_string() << endl;
      ASSERT(ret.second);
    }
    //ASSERT(m_vals.count(from_pc));
    //ASSERT(m_vals.count(to_pc));

    bool ret = xfer_and_meet(e, m_vals.at(from_pc), m_vals.at(to_pc));
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " xfer_and_meet returned " << ret << " for " << e->to_string() << endl);
    return ret;
  }



  /* Kildall's worklist algo
     we'll use a worklist for keeping track of edges whose value needs to be recalculated
     workset: lookup cache with same content as worklist

    TODO: traverse in topological order to minimize the xfer's required
  */
  bool fixed_point_iteration(queue<shared_ptr<T_E const>>& worklist, set<shared_ptr<T_E const>>& workset)
  {
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " start\n");

    bool changed_something = false;
    while (!worklist.empty()) {
      // pop an EDGE to work on
      shared_ptr<T_E const> e = worklist.front();
      worklist.pop();
      workset.erase(e);

      bool changed = solve_per_edge(e);
      if (changed) {
        changed_something = true;
        if (DIR == dfa_dir_t::forward) {
          DYN_DEBUG2(dfa, cout << __func__ << " " << __LINE__ << ": inserting outgoing edges\n");
          insert_outgoing_edges(e->get_to_pc(), worklist, workset);
        }
        else {
          ASSERT(DIR == dfa_dir_t::backward);
          insert_incoming_edges(e->get_from_pc(), worklist, workset);
        }
      }
    }
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " exit.  changed_something = " << changed_something << endl);
    return changed_something;
  }



  pair<queue<shared_ptr<T_E const>>,set<shared_ptr<T_E const>>> get_initial_worklist()
  {
    queue<shared_ptr<T_E const>> worklist;
    set<shared_ptr<T_E const>> workset;
    if (DIR == dfa_dir_t::forward) {
      T_PC entry = m_graph->get_entry_pc();
      insert_outgoing_edges(entry, worklist, workset);
    }
    else {
      ASSERT(DIR == dfa_dir_t::backward);
      list<T_PC> exit_pcs;
      m_graph->get_exit_pcs(exit_pcs);
      if (exit_pcs.size()) {
        for (auto const& pc : exit_pcs) {
          insert_incoming_edges(pc, worklist, workset);
        }
      }
      else {
        // no exit pcs; add all edges
        for (auto const& pc : m_graph->get_all_pcs()) {
          insert_incoming_edges(pc, worklist, workset);
        }
      }
    }
    return make_pair(worklist, workset);
  }


  void insert_incoming_edges(T_PC pc, queue<shared_ptr<T_E const>> &worklist, set<shared_ptr<T_E const>> &workset)
  {
    list<shared_ptr<T_E const>> incoming;
    m_graph->get_edges_incoming(pc, incoming);
    for (auto const& e : incoming) {
      if (!workset.count(e)) {
        worklist.push(e);
        workset.insert(e);
      }
    }
  }

  void insert_outgoing_edges(T_PC pc, queue<shared_ptr<T_E const>> &worklist, set<shared_ptr<T_E const>> &workset)
  {
    list<shared_ptr<T_E const>> outgoing;
    m_graph->get_edges_outgoing(pc, outgoing);
    for (auto const& e : outgoing) {
      if (!workset.count(e)) {
        worklist.push(e);
        workset.insert(e);
      }
    }
  }

};

}
