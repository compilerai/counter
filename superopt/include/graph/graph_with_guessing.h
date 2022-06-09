#pragma once

#include <map>
#include <list>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

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
#include "graph/graph_with_points.h"
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
class graph_with_guessing : public graph_with_points<T_PC, T_N, T_E, T_PRED>
{
private:
  mutable map<reason_for_counterexamples_t, map<T_PC, invariant_state_t<T_PC, T_N, T_E, T_PRED>>> m_invariant_state_map;
  //mutable list<dshared_ptr<T_E const>> m_invariant_state_changed_at_from_pc;
  mutable map<reason_for_counterexamples_t, list<T_PC>> m_invariant_state_changed_at_pc;
  mutable bool m_graph_is_stable = true;
  set<expr_ref> m_graph_bvconsts;
public:

  graph_with_guessing(string const &name, string const& fname, context *ctx/*, memlabel_assertions_t const& memlabel_assertions*/)
    : graph_with_points<T_PC, T_N, T_E, T_PRED>(name, fname, ctx/*, memlabel_assertions*/)
  { }

  graph_with_guessing(graph_with_guessing const &other);

  void graph_ssa_copy(graph_with_guessing const& other)
  {
    graph_with_points<T_PC, T_N, T_E, T_PRED>::graph_ssa_copy(other);
    ASSERT(other.m_invariant_state_map.empty());
  }

  graph_with_guessing(istream& is, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx);

  virtual ~graph_with_guessing() = default;

  void set_graph_bvconsts(set<expr_ref> const& graph_bvconsts)
  {
    m_graph_bvconsts = graph_bvconsts;
  }
  
  set<expr_ref> compute_graph_bvconsts(set<graph_edge_composition_ref<T_PC, T_E>> const& corr_paths = {}) const;

  //returns true if all asserts are still provable
  bool recompute_invariants_at_all_pcs();

  //returns true if all asserts are still provable
  void update_invariant_state_over_edge(dshared_ptr<T_E const> const &e, string const& graph_debug_name /*for debugging*/);

  bool check_monotonicity_of_invariants(T_PC const& p, int start_check_at_num_ces, set<expr_group_t::expr_idx_t> const& bv_exprs_to_remove);
  void check_CEs_satisfy_preds(list<nodece_ref<T_PC, T_N, T_E>> const& cur_ces, unordered_set<shared_ptr<T_PRED const>> const& preds);
  //bool
  //invariant_state_exists_for_pc(T_PC const &p) const
  //{
  //  return m_invariant_state_map.count(p) != 0;
  //}
  invariant_state_t<T_PC, T_N, T_E, T_PRED> const&
  get_invariant_state_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples) const
  {
    if (!m_invariant_state_map.count(reason_for_counterexamples) || !m_invariant_state_map.at(reason_for_counterexamples).count(p)) {
      cout << __func__ << ':' << __LINE__ << ": pc : " << p.to_string() << endl;
      cout << __func__ << ':' << __LINE__ << ": reason: " << reason_for_counterexamples.reason_for_counterexamples_to_string() << endl;
      cout << __func__ << ':' << __LINE__ << ": m_invariant_state_map pcs:\n";
      for (auto const& reason_inv : m_invariant_state_map) {
        cout << " " << reason_inv.first.reason_for_counterexamples_to_string();
        for (auto const& pc_inv : reason_inv.second) {
          cout << pc_inv.first.to_string() << ':';
        }
        cout << endl;
      }
      cout << endl;
      cout.flush();
    }
    ASSERT(m_invariant_state_map.count(reason_for_counterexamples));
    ASSERT(m_invariant_state_map.at(reason_for_counterexamples).count(p));
    return m_invariant_state_map.at(reason_for_counterexamples).at(p);
  }

  invariant_state_t<T_PC, T_N, T_E, T_PRED>&
  get_invariant_state_at_pc_ref(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples)
  {
    if (!m_invariant_state_map.count(reason_for_counterexamples) || !m_invariant_state_map.at(reason_for_counterexamples).count(p)) {
      cout << __func__ << ':' << __LINE__ << ": pc : " << p.to_string() << endl;
      cout << __func__ << ':' << __LINE__ << ": reason: " << reason_for_counterexamples.reason_for_counterexamples_to_string() << endl;
      cout << __func__ << ':' << __LINE__ << ": m_invariant_state_map pcs:\n";
      for (auto const& reason_inv : m_invariant_state_map) {
        cout << " " << reason_inv.first.reason_for_counterexamples_to_string();
        for (auto const& pc_inv : reason_inv.second) {
          cout << pc_inv.first.to_string() << ':';
        }
        cout << endl;
      }
      cout << endl;
      cout.flush();
    }
    ASSERT(m_invariant_state_map.count(reason_for_counterexamples));
    ASSERT(m_invariant_state_map.at(reason_for_counterexamples).count(p));
    return m_invariant_state_map.at(reason_for_counterexamples).at(p);
  }

  unordered_set<shared_ptr<T_PRED const>>
  graph_with_guessing_get_invariants_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples) const
  {
    if (!m_invariant_state_map.count(reason_for_counterexamples) || !m_invariant_state_map.at(reason_for_counterexamples).count(p)) {
      return {};
    }
    if (!m_invariant_state_map.at(reason_for_counterexamples).at(p).is_stable()) {
      return {};
    }
    return m_invariant_state_map.at(reason_for_counterexamples).at(p).get_all_preds();
  }

  virtual map<T_PC, unordered_set<shared_ptr<T_PRED const>>>
  graph_with_guessing_get_invariants_map(reason_for_counterexamples_t const& reason_for_counterexamples) const
  {
    map<T_PC, unordered_set<shared_ptr<T_PRED const>>> ret;
    for (auto const& p : this->get_all_pcs()) {
      auto const& preds = this->graph_with_guessing_get_invariants_at_pc(p, reason_for_counterexamples);
      //cout << __func__ << " " << __LINE__ << ": preds.size() = " << preds.size() << endl;
      ret.insert(make_pair(p, preds));
      //cout << __func__ << " " << __LINE__ << ": ret.at(" << p.to_string() << ").size() = " << ret.at(p).size() << endl;
    }
    return ret;
  }

  string graph_with_guessing_invariants_to_string(bool with_points) const
  {
    stringstream ss;
    for (auto const& invstate : m_invariant_state_map.at(reason_for_counterexamples_t::inductive_invariants())) {
      T_PC const &p = invstate.first;
      invariant_state_t<T_PC, T_N, T_E, T_PRED> const &istate = invstate.second;
      ss << p.to_string() << ":\n";
      ss << istate.invariant_state_to_string(/*p, */"  ", with_points);
      ss << "\n\n";
    }
    return ss.str();
  }

  //friend class invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>;

  //virtual bool need_stability_at_pc(T_PC const& p) const = 0;

  virtual bool graph_is_stable() const override { return m_graph_is_stable; }
  virtual bool mark_graph_unstable(T_PC const& pp, failcond_t const& failcond) const override
  {
    bool old = m_graph_is_stable;
    m_graph_is_stable = false;
    return old;
  }

  void graph_with_guessing_sync_preds() const override
  {
    for (auto const& reason : reason_for_counterexamples_t::get_all_reasons()) {
      if (!this->m_invariant_state_map.count(reason)) {
        continue;
      }
      for (auto const& p : this->get_all_pcs()) {
        if (!this->m_invariant_state_map.at(reason).count(p)) {
          continue;
        }
        this->m_invariant_state_map.at(reason).at(p).invariant_state_sync_preds_for_all_eqclasses();
      }
    }
  }

  //virtual void add_node(dshared_ptr<T_N> n) override;
  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;

protected:
  map<T_PC, invariant_state_t<T_PC, T_N, T_E, T_PRED>> const& get_invariant_state_map(reason_for_counterexamples_t const& reason) const
  { return m_invariant_state_map.at(reason); }


  //eqclasses_ref get_expr_eqclasses_at_pc(T_PC const& p) const
  //{
  //  if (!m_invariant_state_map.count(p)) {
  //    return mk_eqclasses(map<eqclasses_t::expr_group_id_t, expr_group_ref>());
  //  } else {
  //    return m_invariant_state_map.at(p).get_expr_eqclasses();
  //  }
  //}
  //virtual bool is_stable_at_pc(T_PC const& pp) const override
  //{
  //  return this->get_invariant_state_at_pc(pp).is_stable();
  //}

  // exprs in same equivalence-class may be related but two exprs in two different eq-classes are definitely not related
  // in addition, the distribution is NOT based on sorts
  // exprs having different sorts can belong to same equivalence-class
  //virtual eqclasses_ref compute_expr_eqclasses_at_pc(T_PC const& p) const = 0;

  //returns true if preds changed at p
  virtual bool add_CE_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<T_PC, T_N, T_E> const &nce) const override;

  bool graph_with_guessing_node_is_reachable(T_PC const& p) const;

  void graph_with_guessing_add_node_invariants_top_or_boundary(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples);
  //void graph_with_guessing_add_nodes_invariants_top_or_boundary();

  void graph_with_guessing_remove_node(T_PC const &p);

private:
  //void check_point_sets_are_identical(T_PC const &p) const;
  virtual failcond_t check_node_stability_after_CE_addition(T_PC const& p, nodece_ref<T_PC, T_N, T_E> const& nce) const;
  bool add_point_using_CE_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<T_PC, T_N, T_E> const &nce) const override;
  static bool check_implication(unordered_set<shared_ptr<T_PRED const>> const& old_preds, unordered_set<shared_ptr<T_PRED const>> const& new_preds, context* ctx, relevant_memlabels_t const& rmls);

  //void replay_counter_examples()
  //{
  //  for (auto const& p : this->get_all_pcs()) {
  //    list<nodece_ref<T_PC, T_N, T_E>> const& nces = this->get_counterexamples_at_pc(p);
  //    for (auto const& nce : nces) {
  //      DYN_DEBUG(replay_counter_examples, cout << __func__ << " " << __LINE__ << ": replaying nce at pc " << p.to_string() << "=\n"; nce->nodece_to_stream(cout, ""));
  //      this->add_point_using_CE_at_pc(p, nce);
  //    }
  //  }
  //}
};

}
