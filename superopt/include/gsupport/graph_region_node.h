#pragma once

#include <map>
#include <list>
#include <assert.h>
#include <sstream>
#include <set>
#include <stack>
#include <memory>

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E>
class graph_region_node_t : public node<graph_region_pc_t<T_PC, T_E>> {
private:
  graph_region_t<T_PC, T_N, T_E> m_region;
public:
  graph_region_node_t(graph_region_pc_t<T_PC, T_E> const& p, graph_region_t<T_PC, T_N, T_E> const& r) : node<graph_region_pc_t<T_PC, T_E>>(p), m_region(r)
  { }
  void graph_region_node_print(ostream& os, string const& prefix) const;
  graph_region_t<T_PC, T_N, T_E> const& get_region() const { return m_region; }

  list<T_PC> graph_region_node_get_constituent_pcs_among_these_pcs(list<T_PC> const& pcs) const
  {
    set<T_PC> constituent_pcs = this->graph_region_node_get_all_constituent_pcs();
    list<T_PC> n_pcs;
    for (auto const& p : pcs) {
      if (set_belongs(constituent_pcs, p)) {
        n_pcs.push_back(p);
      }
    }
    return n_pcs;
  }

  set<T_PC> graph_region_node_get_all_constituent_pcs() const
  {
    if (m_region.get_type() == graph_region_type_instruction) {
      return { m_region.get_leaf()->get_pc() };
    } else {
      set<T_PC> ret;
      for (auto const& gn : m_region.get_region_graph()->get_nodes()) {
        set_union(ret, gn->graph_region_node_get_all_constituent_pcs());
      }
      return ret;
    }
  }
};

}
