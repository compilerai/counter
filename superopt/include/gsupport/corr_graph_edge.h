#pragma once

#include "support/log.h"

#include <list>
#include <vector>
#include <map>
#include <deque>
#include <set>
//#include "tfg/tfg.h"
#include "eq/eqcheck.h"
#include "graph/graph_with_paths.h"
namespace eqspace {

using namespace std;

class corr_graph;

class corr_graph_node : public node<pcpair>
{
public:
  corr_graph_node(pcpair const& pp) :
    node<pcpair>(pp)
  {
  }

  corr_graph_node(corr_graph_node const &other) : node<pcpair>(other)
  {
  }

  virtual ~corr_graph_node() { }

  pc const &get_src_pc() const;
  pc const &get_dst_pc() const;
};

class corr_graph_edge;
using corr_graph_edge_ref = shared_ptr<corr_graph_edge const>;

corr_graph_edge_ref mk_corr_graph_edge(shared_ptr<corr_graph_node const> from, shared_ptr<corr_graph_node const> to, shared_ptr<tfg_edge_composition_t> const& src_edge, shared_ptr<tfg_edge_composition_t> const& dst_edge);
corr_graph_edge_ref mk_corr_graph_edge(pcpair const& from, pcpair const& to, shared_ptr<tfg_edge_composition_t> const& src_edge, shared_ptr<tfg_edge_composition_t> const& dst_edge);

class corr_graph_edge : public edge<pcpair>
{
private:
  corr_graph_edge(shared_ptr<corr_graph_node const> from, shared_ptr<corr_graph_node const> to, shared_ptr<tfg_edge_composition_t> const& src_edge, shared_ptr<tfg_edge_composition_t> const& dst_edge)
  : edge<pcpair>(from->get_pc(), to->get_pc()),
    m_src_edge(src_edge), m_dst_edge(dst_edge)
  { }

  corr_graph_edge(pcpair const& from, pcpair const& to, shared_ptr<tfg_edge_composition_t> const& src_edge, shared_ptr<tfg_edge_composition_t> const& dst_edge)
  : edge<pcpair>(from, to),
    m_src_edge(src_edge), m_dst_edge(dst_edge)
  {
  }

public:
  virtual ~corr_graph_edge();

  expr_ref const& get_cond() const { NOT_REACHED(); }
  expr_ref const& get_simplified_cond() const { NOT_REACHED(); }
  state const& get_to_state() const { NOT_REACHED(); }

  bool operator==(corr_graph_edge const& other) const
  {
    return    this->edge::operator==(other)
           && this->m_src_edge == other.m_src_edge
           && this->m_dst_edge == other.m_dst_edge;
  }
  //bool operator<(corr_graph_edge const& other) const = delete;

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<corr_graph_edge> *get_corr_graph_edge_manager();

  shared_ptr<tfg_edge_composition_t> get_src_edge() const { return m_src_edge; }
  shared_ptr<tfg_edge_composition_t> get_dst_edge() const { return m_dst_edge; }
  bool cg_is_fcall_edge(corr_graph const &cg) const;

  using edge<pcpair>::to_string; // let the compiler know that we are overloading the virtual function to_string
  string to_string(/*state const* start_state*/) const;
  //string to_string_final(string const &prefix, corr_graph const &cg) const;
  string to_string_concise() const;

  shared_ptr<corr_graph_edge const> visit_exprs(function<expr_ref (expr_ref const&)> f) const;
  shared_ptr<corr_graph_edge const> visit_exprs(function<expr_ref (string const&, expr_ref const&)> f, bool opt = true/**, state const& start_state = state()*/) const;

  void visit_exprs_const(function<void (string const&, expr_ref)> f) const;
  void visit_exprs_const(function<void (pcpair const&, string const&, expr_ref)> f) const;

  template<typename T_VAL, xfer_dir_t T_DIR>
  T_VAL xfer(T_VAL const& in, function<T_VAL (T_VAL const&, expr_ref const&, state const&, unordered_set<expr_ref> const&)> atom_f, function<T_VAL (T_VAL const&, T_VAL const&)> meet_f, bool simplified) const
  {
    shared_ptr<tfg_edge_composition_t> src_ec = this->get_src_edge();
    shared_ptr<tfg_edge_composition_t> dst_ec = this->get_dst_edge();

    ASSERT(src_ec);
    ASSERT(dst_ec);

    T_VAL const& src_val = src_ec->xfer_over_graph_edge_composition<T_VAL,T_DIR>(in, atom_f, meet_f, simplified);
    T_VAL const& ret_val = dst_ec->xfer_over_graph_edge_composition<T_VAL,T_DIR>(src_val, atom_f, meet_f, simplified);

    return ret_val;
  }

  static corr_graph_edge_ref empty() { return mk_corr_graph_edge(pcpair::start(), pcpair::start(), nullptr, nullptr); }
  bool is_empty() const { return this->get_from_pc() == pcpair::start() && this->get_to_pc() == pcpair::start(); }

  static string graph_edge_from_stream(istream &in, string const& first_line, string const &prefix/*, state const &start_state*/, context* ctx, shared_ptr<corr_graph_edge const>& te);
  string to_string_for_eq(string const& prefix) const;

  friend corr_graph_edge_ref mk_corr_graph_edge(shared_ptr<corr_graph_node const> from, shared_ptr<corr_graph_node const> to, shared_ptr<tfg_edge_composition_t> const& src_edge, shared_ptr<tfg_edge_composition_t> const& dst_edge);
  friend corr_graph_edge_ref mk_corr_graph_edge(pcpair const& from, pcpair const& to, shared_ptr<tfg_edge_composition_t> const& src_edge, shared_ptr<tfg_edge_composition_t> const& dst_edge);

private:
  shared_ptr<tfg_edge_composition_t> const m_src_edge;
  shared_ptr<tfg_edge_composition_t> const m_dst_edge;
  bool m_is_managed = false;
};

typedef graph_edge_composition_t<pcpair,corr_graph_edge> const cg_edge_composition_t;

}

namespace std
{
template<>
struct hash<corr_graph_edge>
{
  std::size_t operator()(corr_graph_edge const& te) const
  {
    std::hash<graph_edge_composition_t<pc,tfg_edge>> ec_hasher;
    std::size_t seed = 0;
    if (te.get_src_edge()) {
      myhash_combine<size_t>(seed, ec_hasher(*te.get_src_edge()));
    }
    if (te.get_dst_edge()) {
      myhash_combine<size_t>(seed, ec_hasher(*te.get_dst_edge()));
    }
    return seed;
  }
};

}
