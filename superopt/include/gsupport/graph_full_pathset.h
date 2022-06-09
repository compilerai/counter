#pragma once
#include <list>
#include <map>

#include "expr/pc.h"

#include "gsupport/graph_ec.h"
#include "gsupport/tfg_edge.h"

namespace eqspace {

template<typename T_PC, typename T_E>
class graph_full_pathset_t {
private:
  T_PC m_from_pc;
  T_PC m_to_pc;
  unsigned m_unroll_factor_mu;
  unsigned m_unroll_factor_delta;
  graph_edge_composition_ref<T_PC, T_E> m_graph_ec;
  bool m_is_managed;

  graph_full_pathset_t(T_PC const& from_pc, T_PC const& to_pc, unsigned unroll_factor_mu, unsigned unroll_factor_delta, graph_edge_composition_ref<T_PC, T_E> const& graph_ec) : m_from_pc(from_pc), m_to_pc(to_pc), m_unroll_factor_mu(unroll_factor_mu), m_unroll_factor_delta(unroll_factor_delta), m_graph_ec(graph_ec), m_is_managed(false)
  { }

  //template<typename T_N>
  //graph_full_pathset_t(istream& is, string const& prefix_edge_composition, graph<T_PC, T_N, T_E> const& t); //this is implemented in class tfg
public:
  graph_edge_composition_ref<T_PC, T_E> const& get_graph_ec() const { return m_graph_ec; }
  T_PC const& get_from_pc() const { return m_from_pc; }
  T_PC const& get_to_pc() const { return m_to_pc; }
  unsigned get_unroll_factor_mu() const { return m_unroll_factor_mu; }
  unsigned get_unroll_factor_delta() const { return m_unroll_factor_delta; }

  string graph_full_pathset_to_string_concise() const;

  static manager<graph_full_pathset_t<T_PC, T_E>> *get_graph_full_pathset_manager();

  bool operator==(graph_full_pathset_t const &other) const
  {  return this->m_graph_ec == other.m_graph_ec; }

  bool operator<(graph_full_pathset_t const &other) const
  {  return this->m_graph_ec < other.m_graph_ec; }

  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  bool id_is_zero() const { return !m_is_managed ; }
  ~graph_full_pathset_t();
  bool is_atom() const { return m_graph_ec->is_atom(); }
  shared_ptr<T_E const> get_atom() const { return m_graph_ec->get_atom()->edge_with_unroll_get_edge(); }

  void graph_full_pathset_to_stream(ostream& os, string const& prefix) const;
  string graph_full_pathset_to_string(string const& prefix) const;

  static graph_full_pathset_t<T_PC, T_E> graph_full_pathset_read_attributes_from_string(string const& str);
  bool graph_full_pathset_has_same_attributes_as(graph_full_pathset_t<T_PC, T_E> const& other) const;

  static shared_ptr<graph_full_pathset_t<T_PC, T_E> const> mk_graph_full_pathset(T_PC const& from_pc, T_PC const& to_pc, unsigned unroll_factor_mu, unsigned unroll_factor_delta, graph_edge_composition_ref<T_PC, T_E> const& graph_ec);
  static shared_ptr<graph_full_pathset_t<T_PC, T_E> const> mk_graph_full_pathset(T_PC const& from_pc, graph_edge_composition_ref<T_PC, T_E> const& graph_ec);
  //static shared_ptr<tfg_full_pathset_t const> mk_tfg_full_pathset(istream& is, string const& prefix_src_edge_composition, context* ctx);
};

//template<typename T_PC, typename T_E>
//shared_ptr<graph_full_pathset_t<T_PC, T_E> const> mk_graph_full_pathset(T_PC const& from_pc, T_PC const& to_pc, unsigned unroll_factor_mu, unsigned unroll_factor_delta, graph_edge_composition_ref<T_PC, T_E> const& graph_ec);
//shared_ptr<tfg_full_pathset_t const> mk_tfg_full_pathset(istream& is, string const& prefix_src_edge_composition, context* ctx);

using tfg_full_pathset_t = graph_full_pathset_t<pc, tfg_edge>;

}

namespace std
{
using namespace eqspace;
template<typename T_PC, typename T_E>
struct hash<graph_full_pathset_t<T_PC, T_E>>
{
  std::size_t operator()(graph_full_pathset_t<T_PC, T_E> const &s) const
  {
    return hash<const void *>()(s.get_graph_ec().get());
  }
};

}
