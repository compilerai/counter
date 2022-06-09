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
#include "support/searchable_queue.h"
#include "support/dshared_ptr.h"

using namespace std;

namespace eqspace {

template <typename T_PC, typename T_N, typename T_E>
class graph_with_regions_t;

template<typename T_PC, typename T_N, typename T_E>
class graph_region_node_t;

enum dfa_dir_t { forward, backward };

class dfa_xfer_retval_t {
public:
  enum val_t { unchanged, changed/*, restart*/ };
private:
  val_t m_val;
public:
  dfa_xfer_retval_t(val_t val) : m_val(val)
  { }
  bool operator==(dfa_xfer_retval_t const& other) const
  {
    return m_val == other.m_val;
  }
  bool operator!=(dfa_xfer_retval_t const& other) const
  {
    return !(*this == other);
  }
  string to_string() const {
    return    m_val == unchanged ? "unchanged"
            : m_val == changed   ? "changed"
            : "restart"
    ;
  }
};

extern dfa_xfer_retval_t DFA_XFER_RETVAL_UNCHANGED;
extern dfa_xfer_retval_t DFA_XFER_RETVAL_CHANGED;
//extern dfa_xfer_retval_t DFA_XFER_RETVAL_RESTART;

template <typename T_PC, typename T_N, typename T_E, typename T_VAL, dfa_dir_t DIR>
class data_flow_analysis
{
public:

  data_flow_analysis(data_flow_analysis const &other) = delete;

  data_flow_analysis(graph_with_regions_t<T_PC, T_N, T_E> const* g/*, T_VAL const& boundary_val*/, map<T_PC,T_VAL> & init_vals)
    : m_graph(g),
      m_vals(init_vals)
  {
    ASSERT(g != nullptr);
    ASSERT(g->graph_regions_are_consistent());
  }

  virtual bool postprocess(bool changed) { return changed; }

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, T_VAL const& in, T_VAL &out) = 0;
  virtual set<T_PC> xfer_and_meet_over_region(dshared_ptr<graph_region_node_t<T_PC,T_N,T_E>> const& e) { NOT_IMPLEMENTED(); }

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

  bool solve(list<T_PC> const& start_pcs = {})
  {
    //pair<queue<shared_ptr<T_E const>>,set<shared_ptr<T_E const>>> p_worklist = get_initial_worklist(start_pcs);
    searchable_queue_t<dshared_ptr<T_E const>> p_worklist = get_initial_worklist(start_pcs);
    bool changed = fixed_point_iteration(p_worklist/*.first, p_worklist.second*/);
    return postprocess(changed);
  }

  bool incremental_solve(list<dshared_ptr<T_E const>> const& eset)
  {
    //queue<shared_ptr<T_E const>> worklist;   set<shared_ptr<T_E const>> workset;
    searchable_queue_t<dshared_ptr<T_E const>> worklist;
    for (auto const& e : eset) {
      //worklist.push(e);                  workset.insert(e);
      worklist.searchable_queue_push_back(e);
    }
    bool changed = this->fixed_point_iteration(worklist/*, workset*/);
    return this->postprocess(changed);
  }

protected:

  graph_with_regions_t<T_PC, T_N, T_E> const* m_graph;
  map<T_PC, T_VAL> &m_vals;

private:

  list<dshared_ptr<T_E const>> solve_per_edge(dshared_ptr<T_E const> e)
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

    dfa_xfer_retval_t changed = xfer_and_meet(e, m_vals.at(from_pc), m_vals.at(to_pc));
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " xfer_and_meet returned " << changed.to_string() << " for " << e->to_string() << endl);

    list<dshared_ptr<T_E const>> ret;
    if (changed == DFA_XFER_RETVAL_CHANGED) {
      if (DIR == dfa_dir_t::forward) {
        DYN_DEBUG2(dfa, cout << __func__ << " " << __LINE__ << ": inserting outgoing edges\n");
        //insert_outgoing_edges(e->get_to_pc(), worklist/*, workset*/);
        ret = m_graph->get_edges_outgoing(e->get_to_pc());
      } else {
        ASSERT(DIR == dfa_dir_t::backward);
        //insert_incoming_edges(e->get_from_pc(), worklist/*, workset*/);
        ret = m_graph->get_edges_incoming(e->get_from_pc());
      }
    }

    return ret;
  }



  /* Kildall's worklist algo
     we'll use a worklist for keeping track of edges whose value needs to be recalculated
     workset: lookup cache with same content as worklist

    TODO: traverse in topological order to minimize the xfer's required
  */
  bool fixed_point_iteration(searchable_queue_t<dshared_ptr<T_E const>>& worklist)
  {
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " start\n");

    bool changed_something = false;
    while (!worklist.empty()) {
      // pop an EDGE to work on
      //shared_ptr<T_E const> e = worklist.front();
      //worklist.pop();
      //workset.erase(e);
      dshared_ptr<T_E const> e = worklist.searchable_queue_pop_front();

      list<dshared_ptr<T_E const>> next_edges = this->solve_per_edge(e);
      changed_something = (next_edges.size() > 0);
      for (auto const& next_e : next_edges) {
        worklist.searchable_queue_push_back(next_e);
      }
    }
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " exit.  changed_something = " << changed_something << endl);
    return changed_something;
  }

  searchable_queue_t<dshared_ptr<T_E const>> get_initial_worklist(list<T_PC> const& start_pcs)
  {
    searchable_queue_t<dshared_ptr<T_E const>> worklist;
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


  void insert_incoming_edges(T_PC pc, searchable_queue_t<dshared_ptr<T_E const>> &worklist)
  {
    list<dshared_ptr<T_E const>> incoming;
    m_graph->get_edges_incoming(pc, incoming);
    for (auto const& e : incoming) {
      worklist.searchable_queue_push_back(e);
    }
  }

  void insert_outgoing_edges(T_PC pc, searchable_queue_t<dshared_ptr<T_E const>> &worklist)
  {
    list<dshared_ptr<T_E const>> outgoing;
    m_graph->get_edges_outgoing(pc, outgoing);
    for (auto const& e : outgoing) {
      worklist.searchable_queue_push_back(e);
    }
  }

};

}
