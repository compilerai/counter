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
class graph_region_edge_t : public edge<graph_region_pc_t<T_PC, T_E>> {
private:
  list<dshared_ptr<T_E const>> m_constituent_edges;
public:
  graph_region_edge_t(graph_region_pc_t<T_PC, T_E> const& from, graph_region_pc_t<T_PC, T_E> const& to, list<dshared_ptr<T_E const>> const& edges) : edge<graph_region_pc_t<T_PC, T_E>>(from, to), m_constituent_edges(edges)
  { }
  list<dshared_ptr<T_E const>> const& get_constituent_edges() const
  {
    return m_constituent_edges;
  }
  dshared_ptr<graph_region_edge_t<T_PC, T_N, T_E> const> add_constituent_edge(dshared_ptr<T_E const> const& e) const
  {
    dshared_ptr<graph_region_edge_t<T_PC, T_N, T_E>> new_e = make_dshared<graph_region_edge_t<T_PC, T_N, T_E>>(*this);
    if (!list_contains(new_e->m_constituent_edges, e)) {
      new_e->m_constituent_edges.push_back(e);
    }
    return new_e;
  }
};

}
