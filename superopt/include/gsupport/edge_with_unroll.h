#pragma once

#include "support/dshared_ptr.h"

#include "gsupport/te_comment.h"

namespace eqspace {

template<typename T_PC, typename T_E>
class edge_with_unroll_t {
private:
  shared_ptr<T_E const> m_edge;
  int m_from_pc_unroll;

  static char const sep = '#'; //also hardcoded in lexer files edge_guard.l and cg_edge_composition.l
public:
  //static int const PC_UNROLL_UNKNOWN = -1;
  //edge_with_unroll_t(shared_ptr<T_E const> const& e) : m_edge(e), m_from_pc_unroll(EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN)
  //{ }
  //edge_with_unroll_t(dshared_ptr<T_E const> const& e) : edge_with_unroll_t(e.get_shared_ptr())
  //{ }
  edge_with_unroll_t(shared_ptr<T_E const> const& e, int unroll) : m_edge(e), m_from_pc_unroll(unroll)
  { /*ASSERT(unroll == EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN);*/ }
  edge_with_unroll_t(dshared_ptr<T_E const> const& e, int unroll) : edge_with_unroll_t(e.get_shared_ptr(), unroll)
  { }
  shared_ptr<T_E const> const& edge_with_unroll_get_edge() const { return m_edge; }
  int edge_with_unroll_get_unroll() const { return m_from_pc_unroll; }
  bool operator==(edge_with_unroll_t<T_PC, T_E> const& other) const
  {
    return m_edge == other.m_edge && m_from_pc_unroll == other.m_from_pc_unroll;
  }
  string to_string_concise() const
  {
    stringstream ss;
    ss << m_edge->to_string_concise();
    if (m_from_pc_unroll != EDGE_WITH_UNROLL_PC_UNROLL_UNKNOWN) {
      ss << sep << m_from_pc_unroll;
    }
    return ss.str();
  }
  operator dshared_ptr<T_E const>() const
  {
    return m_edge;
  }
  operator shared_ptr<T_E const>() const
  {
    return m_edge;
  }

  bool is_empty() const
  {
    return m_edge->is_empty();
  }
  T_PC const& get_from_pc() const
  {
    return m_edge->get_from_pc();
  }
  T_PC const& get_to_pc() const
  {
    return m_edge->get_to_pc();
  }
  expr_ref get_cond() const
  {
    return m_edge->get_cond();
  }
  state const& get_to_state() const
  {
    return m_edge->get_to_state();
  }
  te_comment_t const& get_te_comment() const { return m_edge->get_te_comment(); }
};

}

namespace std
{
using namespace eqspace;
template<typename T_PC, typename T_E>
struct hash<edge_with_unroll_t<T_PC, T_E>>
{
  std::size_t operator()(edge_with_unroll_t<T_PC, T_E> const& te) const
  {
    size_t seed = 0;
    myhash_combine<size_t>(seed, (size_t)te.edge_with_unroll_get_edge().get());
    return seed;
  }
};

}
