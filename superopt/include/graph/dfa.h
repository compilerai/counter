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
#include "graph/graph_region_type.h"
#include "graph/dfa_flow_insensitive.h"

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

template <typename T_PC, typename T_N, typename T_E, typename T_VAL, dfa_dir_t DIR>
class data_flow_analysis : public dfa_flow_insensitive<T_PC, T_N, T_E, map<T_PC, T_VAL>, DIR, dfa_state_type_t::per_pc>
{
public:

  data_flow_analysis(data_flow_analysis const &other) = delete;

  data_flow_analysis(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g, map<T_PC,T_VAL> & init_vals)
    : dfa_flow_insensitive<T_PC, T_N, T_E, map<T_PC, T_VAL>, DIR, dfa_state_type_t::per_pc>(g, init_vals)
  { }

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, T_VAL const& in, T_VAL &out) = 0;

  void initialize(std::function<T_VAL (T_PC)> boundary_val_fn)
  {
    this->m_vals.clear();
    if (DIR == dfa_dir_t::forward) {
      T_PC entry = this->m_graph->get_entry_pc();
      this->m_vals.insert(make_pair(entry, boundary_val_fn(entry)));
    } else {
      ASSERT(DIR == dfa_dir_t::backward);
      list<T_PC> exit_pcs;
      this->m_graph->get_exit_pcs(exit_pcs);
      for (auto const& pc : exit_pcs) {
        this->m_vals.insert(make_pair(pc, boundary_val_fn(pc)));
      }
    }
    for (const auto &p : this->m_graph->get_all_pcs()) {
      if (this->m_vals.count(p) == 0) {
        this->m_vals.insert(make_pair(p, T_VAL::top()));
      }
    }
  }

  void initialize(T_VAL const &boundary_val)
  {
    std::function<T_VAL (T_PC)> f = [&boundary_val](T_PC const &p) { return boundary_val; };
    this->initialize(f);
  }

  dfa_xfer_retval_t xfer_and_meet_flow_insensitive(dshared_ptr<T_E const> const& e, map<T_PC, T_VAL>& in_out) override
  {
    T_PC const& from_pc = (DIR == dfa_dir_t::forward) ? e->get_from_pc() : e->get_to_pc();
    T_PC const& to_pc = (DIR == dfa_dir_t::forward) ? e->get_to_pc() : e->get_from_pc();
    if (!in_out.count(from_pc)) {
      auto ret = in_out.insert(make_pair(from_pc, T_VAL::top()));
      //cout << __func__ << " " << __LINE__ << ": adding value at " << from_pc.to_string() << endl;
      ASSERT(ret.second);
    }
    if (!in_out.count(to_pc)) {
      auto ret = in_out.insert(make_pair(to_pc, T_VAL::top()));
      //cout << __func__ << " " << __LINE__ << ": adding value at " << to_pc.to_string() << endl;
      ASSERT(ret.second);
    }
    ASSERT(in_out.count(from_pc));
    ASSERT(in_out.count(to_pc));

    return this->xfer_and_meet(e, in_out.at(from_pc), in_out.at(to_pc));
  }
};

}
