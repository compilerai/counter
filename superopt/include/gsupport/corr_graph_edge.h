#pragma once

#include "support/log.h"

#include "expr/pc.h"

#include "gsupport/te_comment.h"
#include "gsupport/node.h"
#include "gsupport/graph_full_pathset.h"

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

corr_graph_edge_ref mk_corr_graph_edge(dshared_ptr<corr_graph_node const> from, dshared_ptr<corr_graph_node const> to, shared_ptr<tfg_full_pathset_t const> const& src_edge, shared_ptr<tfg_full_pathset_t const> const& dst_edge, unordered_set<expr_ref> const& assumes = {});
corr_graph_edge_ref mk_corr_graph_edge(pcpair const& from, pcpair const& to, shared_ptr<tfg_full_pathset_t const> const& src_edge, shared_ptr<tfg_full_pathset_t const> const& dst_edge, unordered_set<expr_ref> const& assumes = {});

class corr_graph_edge : public edge_with_assumes<pcpair>
{
private:
  corr_graph_edge(dshared_ptr<corr_graph_node const> from, dshared_ptr<corr_graph_node const> to, shared_ptr<tfg_full_pathset_t const> const& src_edge, shared_ptr<tfg_full_pathset_t const> const& dst_edge, unordered_set<expr_ref> const& assumes)
  : edge_with_assumes<pcpair>(from->get_pc(), to->get_pc(), assumes),
    m_src_edge(src_edge), m_dst_edge(dst_edge)
  { }

  corr_graph_edge(pcpair const& from, pcpair const& to, shared_ptr<tfg_full_pathset_t const> const& src_edge, shared_ptr<tfg_full_pathset_t const> const& dst_edge, unordered_set<expr_ref> const& assumes)
  : edge_with_assumes<pcpair>(from, to, assumes),
    m_src_edge(src_edge), m_dst_edge(dst_edge)
  { }

public:
  corr_graph_edge(pcpair const& from_pc, pcpair const& to_pc) : edge_with_assumes<pcpair>(from_pc, to_pc, {}), m_src_edge(nullptr), m_dst_edge(nullptr) { NOT_REACHED(); }
  virtual ~corr_graph_edge();

  expr_ref const& get_cond() const { NOT_REACHED(); }
  //expr_ref const& get_simplified_cond() const { NOT_REACHED(); }
  state const& get_to_state() const { NOT_REACHED(); }
  //state const& get_simplified_to_state() const { NOT_REACHED(); }
  //unordered_set<expr_ref> const& get_assumes() const { NOT_REACHED(); }
  te_comment_t const& get_te_comment() const { NOT_REACHED(); }

  bool operator==(corr_graph_edge const& other) const
  {
    return    this->edge_with_assumes::operator==(other)
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

  shared_ptr<tfg_full_pathset_t const> get_src_edge() const { return m_src_edge; }
  shared_ptr<tfg_full_pathset_t const> get_dst_edge() const { return m_dst_edge; }
  tfg_edge_ref get_src_alloca_edge() const;
  tfg_edge_ref get_dst_alloca_edge() const;
  tfg_edge_ref get_src_dealloc_edge() const;
  tfg_edge_ref get_dst_dealloc_edge() const;

  bool cg_edge_is_fcall_edge() const
  {
    return this->get_to_pc().is_fcall_pc();
  }

  bool cg_edge_is_alloca_edge() const
  {
    bool ret = this->get_src_alloca_edge() != nullptr;
    ASSERT(ret == (this->get_dst_alloca_edge() != nullptr));
    return ret;
  }

  bool cg_edge_is_dealloc_edge() const
  {
    bool ret = this->get_src_dealloc_edge() != nullptr;
    ASSERT(ret == (this->get_dst_dealloc_edge() != nullptr));
    return ret;
  }

  set<allocsite_t> get_alloca_ptrs_on_edge() const;

  string to_string() const override;
  string to_string_concise() const;

  shared_ptr<corr_graph_edge const> visit_keys(std::function<string (string const&, expr_ref)> update_keys_fn, bool opt/*, state const& start_state*/) const;
  shared_ptr<corr_graph_edge const> visit_exprs(function<expr_ref (expr_ref const&)> f) const;
  shared_ptr<corr_graph_edge const> visit_exprs(function<expr_ref (string const&, expr_ref const&)> f, bool opt = true/**, state const& start_state = state()*/) const;

  void visit_exprs_const(function<void (string const&, expr_ref)> f) const;
  void visit_exprs_const(function<void (pcpair const&, string const&, expr_ref)> f) const { NOT_IMPLEMENTED(); }


  static corr_graph_edge_ref empty() { return mk_corr_graph_edge(pcpair::start(), pcpair::start(), nullptr, nullptr); }
  static corr_graph_edge_ref empty(pcpair const& pp) { return mk_corr_graph_edge(pp, pp, nullptr, nullptr); }
  virtual bool is_empty() const override { return m_src_edge == nullptr && m_dst_edge == nullptr; }
  bool contains_function_call(void) const { NOT_REACHED(); }

  //static string graph_edge_from_stream(istream &in, string const& first_line, string const &prefix/*, state const &start_state*/, context* ctx, shared_ptr<corr_graph_edge const>& te);
  string to_string_for_eq(string const& prefix) const;

  friend corr_graph_edge_ref mk_corr_graph_edge(dshared_ptr<corr_graph_node const> from, dshared_ptr<corr_graph_node const> to, shared_ptr<tfg_full_pathset_t const> const& src_edge, shared_ptr<tfg_full_pathset_t const> const& dst_edge, unordered_set<expr_ref> const& assumes);
  friend corr_graph_edge_ref mk_corr_graph_edge(pcpair const& from, pcpair const& to, shared_ptr<tfg_full_pathset_t const> const& src_edge, shared_ptr<tfg_full_pathset_t const> const& dst_edge, unordered_set<expr_ref> const& assumes);

private:
  shared_ptr<tfg_full_pathset_t const> const m_src_edge;
  shared_ptr<tfg_full_pathset_t const> const m_dst_edge;
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
      myhash_combine<size_t>(seed, ec_hasher(*te.get_src_edge()->get_graph_ec()));
    }
    if (te.get_dst_edge()) {
      myhash_combine<size_t>(seed, ec_hasher(*te.get_dst_edge()->get_graph_ec()));
    }
    return seed;
  }
};

}
