#pragma once

#include "gsupport/graph_ec.h"
#include "gsupport/edge.h"

namespace eqspace {

template<typename T_PC, typename T_E>
class edge_with_graph_ec_no_unroll_t : public edge<T_PC> {
private:
  shared_ptr<graph_edge_composition_t<T_PC, T_E> const> m_ec;
public:
  edge_with_graph_ec_no_unroll_t(T_PC const& from, T_PC const& to, shared_ptr<graph_edge_composition_t<T_PC, T_E> const> const& ec) : edge<T_PC>(from, to), m_ec(ec)
  { }

  edge_with_graph_ec_no_unroll_t(T_PC const& from, T_PC const& to) : edge<T_PC>(from, to), m_ec(nullptr)
  { NOT_REACHED(); }

  shared_ptr<graph_edge_composition_t<T_PC, T_E> const> const& get_ec() const { return m_ec; }

  bool edge_with_graph_ec_no_unroll_contains_edge_at_from_pc(dshared_ptr<T_E const> const& e) const
  {
    return m_ec->graph_ec_outgoing_edges_contain_edge(e);
  }

  state const& get_to_state() const { NOT_REACHED(); }
  expr_ref const& get_cond() const { NOT_REACHED(); }
  static shared_ptr<edge_with_graph_ec_no_unroll_t<T_PC, T_E> const> empty() { NOT_REACHED(); }
  static shared_ptr<edge_with_graph_ec_no_unroll_t<T_PC, T_E> const> empty(T_PC const& p) { NOT_REACHED(); }
  void visit_exprs_const(function<void (T_PC const&, string const&, expr_ref)> f) const { NOT_REACHED(); }
  string to_string_concise() const { return m_ec->to_string_concise(); }
  te_comment_t const& get_te_comment() const { NOT_REACHED(); }

private:
};

}
