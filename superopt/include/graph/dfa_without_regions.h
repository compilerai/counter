#pragma once

#include "support/dyn_debug.h"
#include "support/dshared_ptr.h"
//#include "support/searchable_queue.h"

#include "graph/dfa_types.h"

using namespace std;

namespace eqspace {

template <typename T_PC, typename T_N, typename T_E>
class graph;

template <typename T_PC, typename T_N, typename T_E, typename T_VAL, dfa_dir_t DIR>
class dfa_without_regions_t
{
public:

  dfa_without_regions_t(dfa_without_regions_t const &other) = delete;

  dfa_without_regions_t(dshared_ptr<graph<T_PC, T_N, T_E> const> g, map<T_PC,T_VAL> & init_vals)
    : m_graph(g),
      m_vals(init_vals)
  {
    ASSERT(g);
    m_postorder_edges = g->compute_postorder();
  }

  virtual bool postprocess(bool changed) { return changed; }

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, T_VAL const& in, T_VAL &out) = 0;

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

  list<dshared_ptr<T_E const>> solve_per_edge(dshared_ptr<T_E const> e, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
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
      ret = get_modified_edges_due_to_change_across_edge(e, pop_externally_modified_pcs);
    } else {
      ASSERT(pop_externally_modified_pcs().size() == 0);
    }

    return ret;
  }

  bool fixed_point_iteration(list<dshared_ptr<T_E const>>& worklist, std::function<list<T_PC> (void)> pop_externally_modified_pcs)
  {
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " start\n");

    bool changed_something = false;
    while (!worklist.empty()) {
      //dshared_ptr<T_E const> e = list_pop_front(worklist);
      dshared_ptr<T_E const> e = this->m_graph->remove_next_edge_from_list_in_postorder(worklist, m_postorder_edges, DIR == dfa_dir_t::forward);

      list<dshared_ptr<T_E const>> next_edges = this->solve_per_edge(e, pop_externally_modified_pcs);
      changed_something = (next_edges.size() > 0) || changed_something;
      for (auto const& next_e : next_edges) {
        worklist.push_back(next_e);
      }
    }
    DYN_DEBUG(dfa, cout << demangled_type_name(typeid(*this)) << "::" << __func__ << ':' << __LINE__ << " exit.  changed_something = " << changed_something << endl);
    return changed_something;
  }

  bool solve(list<T_PC> const& start_pcs = {})
  {
    if (m_graph->get_all_pcs().size() == 0) {
      return false;
    }
    //pair<queue<shared_ptr<T_E const>>,set<shared_ptr<T_E const>>> p_worklist = get_initial_worklist(start_pcs);
    list<dshared_ptr<T_E const>> p_worklist = get_initial_worklist(start_pcs);

    std::function<list<T_PC> (void)> f = [](void) { return list<T_PC>(); };

    bool changed = fixed_point_iteration(p_worklist, f);
    return postprocess(changed);
  }

protected:
  dshared_ptr<graph<T_PC, T_N, T_E> const> m_graph;
  map<T_PC, T_VAL> &m_vals;

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
  list<dshared_ptr<T_E const>> m_postorder_edges;

private:
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

  list<dshared_ptr<T_E const>> get_modified_edges_due_to_change_across_edge(dshared_ptr<T_E const> e, std::function<list<T_PC> (void)> pop_externally_modified_pcs) const
  {
    list<dshared_ptr<T_E const>> ret;
    if (DIR == dfa_dir_t::forward) {
      ret = m_graph->get_edges_outgoing(e->get_to_pc());
    } else {
      ASSERT(DIR == dfa_dir_t::backward);
      ret = m_graph->get_edges_incoming(e->get_from_pc());
    }
    DYN_DEBUG2(dfa,
      cout << __func__ << " " << __LINE__ << ": after inserting incoming/outgoing edges at " << e->get_to_pc().to_string() << ", ret (size " << ret.size() << "):";
      for (auto const& r : ret) {
        cout << " " << r->get_from_pc().to_string() << "=>" << r->get_to_pc().to_string();
      }
      cout << endl;
    );

    this->add_to_worklist_edges_due_to_externally_modified_pcs(ret, pop_externally_modified_pcs);
    DYN_DEBUG2(dfa,
      cout << __func__ << " " << __LINE__ << ": after inserting incoming/outgoing edges of externally modified pcs, ret (size " << ret.size() << "):";
      for (auto const& r : ret) {
        cout << " " << r->get_from_pc().to_string() << "=>" << r->get_to_pc().to_string();
      }
      cout << endl;
    );

    return ret;
  }
};

}
