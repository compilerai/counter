#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/searchable_queue.h"
#include "support/timers.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/pc.h"

#include "gsupport/edge_with_cond.h"
#include "gsupport/failcond.h"
#include "gsupport/sprel_map.h"

#include "graph/graph.h"
#include "graph/graph_ec_unroll.h"
#include "graph/graph_with_proofs.h"
#include "graph/graph_with_ce.h"
//#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"
#include "graph/dfa.h"
#include "graph/dfa_over_paths.h"
#include "graph/point_set.h"
#include "graph/invariant_state.h"
#include "graph/eqclasses.h"
#include "graph/expr_group.h"
#include "graph/reason_for_counterexamples.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_points : public graph_with_ce<T_PC, T_N, T_E, T_PRED>
{
public:
  using global_exprs_map_t = point_set_t::global_exprs_map_t;

private:
  //map<T_PC, map<reason_for_counterexamples_t, eqclasses_ref>> m_expr_eqclasses;
  //mutable map<T_PC, map<reason_for_counterexamples_t, map<eqclasses_t::expr_group_id_t, point_set_t>>> m_point_set;
  map<reason_for_counterexamples_t, map<T_PC, eqclasses_ref>> m_per_pc_expr_eqclasses;
  mutable map<reason_for_counterexamples_t, point_set_t> m_point_set;
  mutable map<reason_for_counterexamples_t, map<T_PC, set<expr_group_t::expr_idx_t>>> m_per_pc_out_of_bound_exprs;

public:

  graph_with_points(string const &name, string const& fname, context *ctx)
    : graph_with_ce<T_PC, T_N, T_E, T_PRED>(name, fname, ctx)
  { }

  graph_with_points(graph_with_points const &other)
    : graph_with_ce<T_PC, T_N, T_E, T_PRED>(other),
      //m_expr_eqclasses(other.m_expr_eqclasses),
      m_per_pc_expr_eqclasses(other.m_per_pc_expr_eqclasses),
      m_point_set(other.m_point_set),
      m_per_pc_out_of_bound_exprs(other.m_per_pc_out_of_bound_exprs)
  { }

  void graph_ssa_copy(graph_with_points const& other)
  {
    graph_with_ce<T_PC, T_N, T_E, T_PRED>::graph_ssa_copy(other);
  }

  graph_with_points(istream& is, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx);

//  map<eqclasses_t::expr_group_id_t, point_set_t> const& get_point_set_map(/*T_PC const& p, */reason_for_counterexamples_t const& reason_for_counterexamples) const
//  {
////    ASSERT(m_point_set.count(p));
//    ASSERT(m_point_set.count(reason_for_counterexamples));
//    return m_point_set.at(reason_for_counterexamples);
//  }

  point_set_t const& get_point_set(/*T_PC const& p, */reason_for_counterexamples_t const& reason_for_counterexamples/*, eqclasses_t::expr_group_id_t egid*/) const
  {
//    ASSERT(m_point_set.count(p));
    ASSERT(m_point_set/*.at(p)*/.count(reason_for_counterexamples));
//    ASSERT(m_point_set/*.at(p)*/.at(reason_for_counterexamples).count(egid));
    return m_point_set/*.at(p)*/.at(reason_for_counterexamples);
  }

  virtual ~graph_with_points() = default;

  bool graph_has_eqclass_exprs_at_pc_for_eg(reason_for_counterexamples_t const& reason_for_counterexamples, T_PC const& p, expr_group_t::expr_idx_t egid) const
  {
    if (!m_per_pc_expr_eqclasses.count(reason_for_counterexamples))
      return false;
    if (!m_per_pc_expr_eqclasses.at(reason_for_counterexamples).count(p))
      return false;
    if (!m_per_pc_expr_eqclasses.at(reason_for_counterexamples).at(p)->get_expr_group_ls().count(egid))
      return false;
    return true;
  }

  expr_group_ref const& graph_get_eqclass_exprs_at_pc_for_eg(reason_for_counterexamples_t const& reason_for_counterexamples, T_PC const& p, expr_group_t::expr_idx_t egid) const
  {
    ASSERT(m_per_pc_expr_eqclasses.count(reason_for_counterexamples));
    ASSERT(m_per_pc_expr_eqclasses.at(reason_for_counterexamples).count(p));;
    ASSERT(m_per_pc_expr_eqclasses.at(reason_for_counterexamples).at(p)->get_expr_group_ls().count(egid));
    return (m_per_pc_expr_eqclasses.at(reason_for_counterexamples).at(p)->get_expr_group_ls().at(egid));
  }

  set<expr_group_t::expr_idx_t> const& graph_with_points_get_out_of_bound_exprs(reason_for_counterexamples_t const& reason_for_counterexamples, T_PC const& p) const
  {
    return m_per_pc_out_of_bound_exprs[reason_for_counterexamples][p];
  }

protected:

  void graph_add_eqclass_exprs_at_pc_for_eg(reason_for_counterexamples_t const& reason_for_counterexamples, T_PC const& p, eqclasses_ref const& eqclasses) {
    ASSERT (!m_per_pc_expr_eqclasses.count(reason_for_counterexamples) || !m_per_pc_expr_eqclasses.at(reason_for_counterexamples).count(p));
    map<T_PC, eqclasses_ref> pc_eqclass_exprs;
    if(m_per_pc_expr_eqclasses.count(reason_for_counterexamples))
      pc_eqclass_exprs = m_per_pc_expr_eqclasses.at(reason_for_counterexamples);
    pc_eqclass_exprs.insert(make_pair(p, eqclasses));
    m_per_pc_expr_eqclasses[reason_for_counterexamples] = pc_eqclass_exprs;
  }

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;

  global_exprs_map_t graph_get_global_exprs_map(reason_for_counterexamples_t const& reason_for_counterexamples) const
  {
    ASSERT(m_point_set.count(reason_for_counterexamples));
//      return mk_eqclasses(map<eqclasses_t::expr_group_id_t, expr_group_ref>());
    return m_point_set.at(reason_for_counterexamples).get_global_exprs_map();
  }

  //virtual bool is_stable_at_pc(T_PC const& pp) const override
  //{
  //  return this->get_invariant_state_at_pc(pp).is_stable();
  //}

  // exprs in same equivalence-class may be related but two exprs in two different eq-classes are definitely not related
  // in addition, the distribution is NOT based on sorts
  // exprs having different sorts can belong to same equivalence-class
  virtual eqclasses_ref compute_expr_eqclasses_at_pc(T_PC const& p) const = 0;

  //returns true if preds changed at p
  virtual bool add_CE_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<T_PC, T_N, T_E> const &nce) const override;

protected:
  virtual bool add_point_using_CE_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<T_PC, T_N, T_E> const &nce) const;
  eqclasses_ref graph_with_points_add_eqclasses_at_pc(T_PC const& p, reason_for_counterexamples_t const& reason_for_counterexamples, eqclasses_ref const& expr_eqclasses_at_pc);
  void graph_with_points_pop_last_point_for_unchanged_expr_groups(T_PC const& p, reason_for_counterexamples_t const& reason_for_counterexamples, set<eqclasses_t::expr_group_id_t> const& changed_expr_groups) const;

private:
eqclasses_ref compute_expr_eqclasses_at_pc_using_global_exprs_map(reason_for_counterexamples_t const& reason_for_counterexamples, T_PC const& p, eqclasses_ref const& eqclass_exprs_at_p) const;

static pair<bool, expr_group_t::expr_idx_t> expr_present_in_global_exprs_map( point_exprs_t const& pe, global_exprs_map_t const& global_eg)
{
  for (auto const& id_pe : global_eg) {
    if (pe == id_pe.second)
      return make_pair(true, id_pe.first);
  }
  return make_pair(false, (expr_group_t::expr_idx_t)-1);
}

global_exprs_map_t add_eqclass_exprs_to_global_exprs_map(reason_for_counterexamples_t const& reason_for_counterexamples, eqclasses_ref const& expr_eqclasses) 
{
  auto idx = get_next_global_expr_idx(reason_for_counterexamples);
  global_exprs_map_t global_eg;
  if (m_point_set.count(reason_for_counterexamples))
   global_eg = this->graph_get_global_exprs_map(reason_for_counterexamples); 

  for (auto const& [egid, eg] : expr_eqclasses->get_expr_group_ls()) {
    for (auto const& [id, e] : eg->get_expr_vec()) {
      if (!(expr_present_in_global_exprs_map(e, global_eg).first))
        global_eg.insert(make_pair(idx++, e));
    }
  }
  return global_eg;
}

expr_group_t::expr_idx_t get_next_global_expr_idx(reason_for_counterexamples_t const& reason_for_counterexamples) const
{
  expr_group_t::expr_idx_t ret = expr_group_t::expr_idx_begin();
  if (!m_point_set.count(reason_for_counterexamples) || !this->graph_get_global_exprs_map(reason_for_counterexamples).size())
    return ret;
  for (auto const& [id, e] : this->graph_get_global_exprs_map(reason_for_counterexamples)) {
    if (id > ret)
      ret = id;
  }
  return (ret+1);
}


};

}
