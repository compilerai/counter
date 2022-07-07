#pragma once

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E>
class graph_loop_t {
private:
  T_PC m_head;
  set<T_PC> m_nodes;
  list<dshared_ptr<T_E const>> m_back_edges;
public:
  graph_loop_t(T_PC const& head, set<T_PC> const& nodes, list<dshared_ptr<T_E const>> const& back_edges)
    : m_head(head), m_nodes(nodes), m_back_edges(back_edges)
  { }
  T_PC const& get_head() const { return m_head; }
  set<T_PC> const& get_nodes() const { return m_nodes; }
  list<dshared_ptr<T_E const>> const& get_back_edges() const { return m_back_edges; }
  void graph_loop_union(graph_loop_t const& other);
  bool graph_loop_contains_pc(T_PC const& p) const { return set_belongs(m_nodes, p); }
};

}
