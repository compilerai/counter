#pragma once

#include "gsupport/pc.h"
#include "graph/graph.h"
#include "graph/graph_with_proofs.h"
//#include "graph/concrete_values_set.h"
#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"
#include "gsupport/sprel_map.h"
/* #include "graph/graph_cp.h" */
#include "expr/local_sprel_expr_guesses.h"

#include "gsupport/edge_with_cond.h"
//#include "tfg/edge_guard.h"
//#include "graph/concrete_values_set.h"
#include "support/timers.h"
#include "graph/delta.h"
//#include "eq/invariant_state.h"
#include "graph/graphce.h"
#include "graph/nodece.h"
#include "graph/nodece_set.h"
#include "gsupport/sprel_map.h"
#include "expr/sprel_map_pair.h"
#include "support/dyn_debug.h"

#include <map>
#include <list>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_ce : public graph_with_proofs<T_PC, T_N, T_E, T_PRED>
{
private:
  static int const FRESH_COUNTER_EXAMPLE_MAX_PROP_COUNT = 3;
  static int const RANKING_CES_PROPAGATION_THRESHOLD = 20;

  //m_CE_pernode_map: updated using virtual function add_CE_at_pc;
  //read using get_counterexamples_at_pc()
  //should not be updated through any other function (because add_CE_at_pc is overridden in derived classes)
  mutable std::map<T_PC, nodece_set_ref<T_PC, T_N, T_E>> m_CE_pernode_map;
  std::map<T_PC, nodece_set_ref<T_PC, T_N, T_E>> m_CE_pernode_map_for_propagation;

  //stats
  mutable std::map<T_PC, unsigned> m_CE_generated_pernode_map;
  mutable std::map<T_PC, unsigned> m_CE_generated_added_pernode_map;
 
public:
  graph_with_ce(string const &name, context *ctx, memlabel_assertions_t const& memlabel_assertions)
    : graph_with_proofs<T_PC, T_N, T_E, T_PRED>(name, ctx, memlabel_assertions)
  { }

  graph_with_ce(graph_with_ce const &other)
    : graph_with_proofs<T_PC, T_N, T_E, T_PRED>(other), m_CE_pernode_map(other.m_CE_pernode_map), m_CE_pernode_map_for_propagation(other.m_CE_pernode_map_for_propagation), m_CE_generated_pernode_map(other.m_CE_generated_pernode_map), m_CE_generated_added_pernode_map(other.m_CE_generated_added_pernode_map)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_ce(istream& is, string const& name, context* ctx) : graph_with_proofs<T_PC, T_N, T_E, T_PRED>(is, name, ctx)
  {
    string line;
    bool end;
    do {
      end = !getline(is, line);
      ASSERT(!end);
    } while (line == "");
    string const prefix = "=counterexamples at pc ";
    while (is_line(line, prefix)) {
      istringstream iss(line);
      string pc_str = line.substr(prefix.length());
      T_PC p = T_PC::create_from_string(pc_str);
      nodece_set_ref<T_PC, T_N, T_E> nodece_set;
      string prefix = "pc " + p.to_string() + " ";
      line = nodece_set_t<T_PC, T_N, T_E>::nodece_set_from_stream(is/*, this->get_start_state()*/, ctx, prefix, nodece_set);
      m_CE_pernode_map.insert(make_pair(p, nodece_set));
   }
    ASSERT(line == "=graph_with_ce done");
  }

  virtual ~graph_with_ce() = default;

  //concrete_values_set_t<T_PC,T_N,T_E,T_PRED> & get_concrete_values_set() { return m_concrete_values; }
  //concrete_values_set_t<T_PC,T_N,T_E,T_PRED> const& get_concrete_values_set() const { return m_concrete_values; }
  void get_concrete_ces_at_pc(T_PC const &p, list<counter_example_t> &ces ) const
  {
    ces.clear();
    if (m_CE_pernode_map.count(p)) {
      for(auto const& nce : m_CE_pernode_map.at(p)->get_nodeces())
        ces.push_back(nce->nodece_get_counter_example(this));
    }
  }

  list<nodece_ref<T_PC, T_N, T_E>> const& get_counterexamples_at_pc(T_PC const &p) const
  {
    if (!m_CE_pernode_map.count(p)) {
      static list<nodece_ref<T_PC, T_N, T_E>> empty_set;
      return empty_set;
    }
    return m_CE_pernode_map.at(p)->get_nodeces();
  }

  unsigned get_counterexamples_generated_at_pc(T_PC const &p) const
  {
    if (!m_CE_generated_pernode_map.count(p)) {
      return 0;
    }
    return m_CE_generated_pernode_map.at(p);
  }

  unsigned get_counterexamples_generated_added_at_pc(T_PC const &p) const
  {
    if (!m_CE_generated_added_pernode_map.count(p)) {
      return 0;
    }
    return m_CE_generated_added_pernode_map.at(p);
  }

  nodece_ref<T_PC, T_N, T_E> const &get_most_recent_counter_example_at_pc(T_PC const &p) const
  {
    ASSERT(m_CE_pernode_map.count(p));
    ASSERT(m_CE_pernode_map.at(p)->size() > 0);
    return m_CE_pernode_map.at(p)->get_nodeces().back();
  }

  nodece_ref<T_PC, T_N, T_E> get_random_counter_example_at_pc(T_PC const &p) const
  {
    if (   !m_CE_pernode_map.count(p)
        || m_CE_pernode_map.at(p)->size() == 0) {
      return nullptr;
    }
    unsigned victim_idx = random_number(m_CE_pernode_map.at(p)->size());
    return *next(m_CE_pernode_map.at(p)->get_nodeces().begin(), victim_idx);
  }

  proof_status_t
  decide_hoare_triple(unordered_set<shared_ptr<T_PRED const>> const &pre, T_PC const &from_pc, graph_edge_composition_ref<T_PC,T_E> const &pth, shared_ptr<T_PRED const> const &post, bool &guess_made_weaker) const override
  {
    autostop_timer func_timer(string(__func__) + ".ce");
    context *ctx = this->get_context();
    //list<nodece_ref<T_PC, T_N, T_E>> const &ce_set = this->get_counterexamples_at_pc(from_pc);
    //the following check is not needed because the CEs are propagated as they are created anyways
    //also before uncommenting the following check: we should also perhaps check that the CEs in ce_set satisfy PRE.
    //list<nodece_ref<T_PC, T_N, T_E>> txed_ce_set = propagate_counter_examples_across_ec(ce_set, pth);
    //if (disprove_predicate_through_counter_examples_on_path(post, txed_ce_set)) {
    //  ASSERT(post.get_proof_status() == predicate::not_provable);
    //  CPP_DBG_EXEC(DECIDE_HOARE_TRIPLE, cout << __func__ << " " << __LINE__ << ": proof query failed\n");
    //  return true;
    //}
    list<counter_example_t> returned_ces;
    proof_status_t ret = graph_with_proofs<T_PC, T_N, T_E, T_PRED>::decide_hoare_triple_helper(pre, from_pc, pth, post, returned_ces, guess_made_weaker);
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    if (ret == proof_status_disproven) {
      DYN_DEBUG(decide_hoare_triple, cout << __func__ << " " << __LINE__ << ": proof query failed; calling add_fresh_counterexample_at_pc_and_propagate()\n");
      if (returned_ces.empty()) {
        cout << __func__ << " " << __LINE__ << ": WARNING no counter examples\n";
        return proof_status_timeout;
      }
      if (pth && pth->is_epsilon()) {
        // no path to propagate CE; just add them to from_pc
        for (auto const& ret_ce : returned_ces) {
          ASSERT (!ret_ce.is_empty());
          this->add_fresh_counterexample_at_pc(from_pc, ret_ce);
        }
      } else {
        unsigned before_ces = 0;
        if (pth && !pth->is_epsilon()) {
          T_PC const& to_pc = pth->graph_edge_composition_get_to_pc();
          before_ces = this->get_counterexamples_at_pc(to_pc).size();
        }
        for (auto const& ret_ce : returned_ces) {
          ASSERT (!ret_ce.is_empty());
          bool is_stable = this->add_fresh_counterexample_at_pc_and_propagate(from_pc, ret_ce);
          if (!is_stable) {
            break;
          }
        }
        if (pth && !pth->is_epsilon()) {
          T_PC const& to_pc = pth->graph_edge_composition_get_to_pc();
          if (m_CE_generated_pernode_map.count(to_pc) == 0)
            m_CE_generated_pernode_map[to_pc] = 0;
          m_CE_generated_pernode_map[to_pc] += returned_ces.size();
          unsigned added_ces = this->get_counterexamples_at_pc(to_pc).size()-before_ces;
          if (m_CE_generated_added_pernode_map.count(to_pc) == 0)
            m_CE_generated_added_pernode_map[to_pc] = 0;
          m_CE_generated_added_pernode_map[to_pc] += added_ces;
        }
      }
    }
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return ret;
  }
//  virtual void generate_new_CE(T_PC const& from_pc, graph_edge_composition_ref<T_PC, T_E> const &dst_ec) const = 0;

  unsigned propagate_CEs_across_edge_while_checking_implied_cond_and_stability(shared_ptr<T_E const> const &ed, bool &implied_check_status, bool &stability_check_status, bool propagate_initial_CEs)
  {
    autostop_timer fn(__func__);
    context *ctx = this->get_context();
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    unsigned propagated_CEs = 0;
    T_PC const &from_pc = ed->get_from_pc();
    T_PC const &to_pc = ed->get_to_pc();
    DYN_DEBUG(ce_add, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": ed = " << ed->to_string() << ", m_CE_pernode_map.count(" << from_pc.to_string() << ") = " << m_CE_pernode_map.count(from_pc) << endl);
//    if (ctx->get_config().disable_propagation_based_pruning)
//      return propagated_CEs;
    mytimer mt1;
    DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": start " << endl; mt1.start(););
//    if (!m_CE_pernode_map.count(from_pc) && propagate_initial_CEs) { //generate new CEs
//      generate_new_CE(from_pc, ed);
//    }
    list<nodece_ref<T_PC, T_N, T_E>> nces;
    if(propagate_initial_CEs && m_CE_pernode_map.count(from_pc)) 
      nces = m_CE_pernode_map.at(from_pc)->get_nodeces();
    else if(!propagate_initial_CEs && m_CE_pernode_map_for_propagation.count(from_pc)) {
      nces = m_CE_pernode_map_for_propagation.at(from_pc)->get_nodeces();
      m_CE_pernode_map_for_propagation.erase(from_pc);
    }
    DYN_DEBUG(ce_add, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": nces at(" << from_pc.to_string() << ").size() = " << nces.size() << endl);
    graph_edge_composition_ref<T_PC, T_E> pth = mk_edge_composition<T_PC, T_E>(ed);
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    typename list<nodece_ref<T_PC, T_N, T_E>>::reverse_iterator it;
    for(it = nces.rbegin(); it != nces.rend(); ++it)
    {
      auto const& nce = *it;
      counter_example_t const &ce = nce->nodece_get_counter_example(this);
      counter_example_t out_ce(ctx);      
      counter_example_t rand_vals(ctx);
      pair<bool,bool> status = this->counter_example_translate_on_edge(ce, ed, out_ce, rand_vals, this->get_relevant_memlabels());
      if(!status.first && status.second){
        if (!ctx->get_config().disable_propagation_based_pruning) {
          DYN_DEBUG(correlate, mt1.stop(); cout << __func__ << " " << __LINE__ << ": disprove econd through_counter_examples_on_path in " << mt1.get_elapsed_secs() << "s " << endl);
          implied_check_status = false;
          stats::get().inc_counter("# of Paths Prunned -- CEsSatisfyCorrelCriterion");
        }
        return propagated_CEs;
      }
      if (!status.first) {
        DYN_DEBUG(ce_add, cout << __func__ << " " << __LINE__ << ": propagation failed across edge " << pth->graph_edge_composition_to_string() << endl);
        continue;
      }
      out_ce.counter_example_union(rand_vals);
      DYN_DEBUG2(ce_add, cout << __func__ << " " << __LINE__ << ": CE propagated across " << pth->graph_edge_composition_to_string() << " =\n" << out_ce.counter_example_to_string() << endl);
      shared_ptr<graph_edge_composition_t<T_PC, T_E> const> const &ce_pth = nce->nodece_get_path();
      shared_ptr<graph_edge_composition_t<T_PC, T_E> const> new_pth = mk_series(ce_pth, pth);
      nodece_ref<T_PC, T_N, T_E> prop_nce = mk_nodece(nce->get_graphce_ref(), new_pth, out_ce);
      prop_nce->get_graphce_ref()->graphce_counter_example_union(rand_vals);
      PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
      map<T_PC, size_t> pc_seen_count = prop_nce->nodece_get_pc_seen_count();
      PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
      bool is_stable = this->add_nodece_at_pc_and_propagate(ed->get_to_pc(), prop_nce, pc_seen_count);
      if (!is_stable) {                                           
        if (!ctx->get_config().disable_propagation_based_pruning) {
          DYN_DEBUG(correlate, mt1.stop(); cout << __func__ << " " << __LINE__ << ": disprove mem state through_counter_examples_on_path in " << mt1.get_elapsed_secs() << "s " << endl);
          stability_check_status = false;
          stats::get().inc_counter("# of Paths Prunned -- InvRelatesHeapAtEachNode");
        }
        return propagated_CEs;
      }
      PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
      ++propagated_CEs;
      if(propagate_initial_CEs && propagated_CEs == RANKING_CES_PROPAGATION_THRESHOLD){
        advance(it, 1);
        nces.erase(it.base(), nces.end());
        m_CE_pernode_map_for_propagation[from_pc] = mk_nodece_set(nces);
        break;
      }
    }
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    DYN_DEBUG(correlate, mt1.stop(); cout << __func__ << " " << __LINE__ << ": Not able to disprove econd / mem state through_counter_examples_on_path in " << mt1.get_elapsed_secs() << "s " << endl);
    return propagated_CEs;
  }

  //returns true if disprove was successful
  bool disprove_predicate_through_counter_examples(unordered_set<T_PRED> const& assumes, expr_ref const& precond_expr, expr_ref const& lhs_expr, expr_ref const& rhs_expr, list<nodece_ref<T_PC, T_N, T_E>> const &ce_set) const
  {
    //Some code de-duplication with propagate_CE_over_edge, try to merge in a single function
    context *ctx = this->get_context();
    //precond_t const &precond = pred.get_precond();
    //expr_ref const &lhs_expr = pred.get_lhs_expr();
    //expr_ref const &rhs_expr = pred.get_rhs_expr();
    //edge_guard_t const &eg = precond.precond_get_guard();
    //local_sprel_expr_guesses_t const &lse_assumes = precond.get_local_sprel_expr_assumes();
    set<memlabel_ref> const& relevant_memlabels = this->get_relevant_memlabels();

    //expr_ref precond_expr = precond.precond_get_expr(this->get_src_tfg(), true);

    for (nodece_ref<T_PC, T_N, T_E> const &nce : ce_set) {
      counter_example_t const &ce = nce->nodece_get_counter_example(this);
      counter_example_t rand_vals(ctx);
      expr_ref const_val;
      if (!ce.evaluate_expr_assigning_random_consts_as_needed(precond_expr, const_val, rand_vals, relevant_memlabels)) {
        continue;
      }
      ASSERT(const_val->is_bool_sort());
      ASSERT(const_val->is_const());
      if (!const_val->get_bool_value()) {
        continue;
      }
      if (!this->counter_example_satisfies_preds_at_pc(ce, assumes, rand_vals, this->get_memlabel_assertions())) {
        continue;
      }
      expr_ref lhs_const, rhs_const;
      if (!ce.evaluate_expr_assigning_random_consts_as_needed(lhs_expr, lhs_const, rand_vals, relevant_memlabels)) {
        continue;
      }
      if (!ce.evaluate_expr_assigning_random_consts_as_needed(rhs_expr, rhs_const, rand_vals, relevant_memlabels)) {
        continue;
      }
      ASSERT(lhs_const->is_const());
      ASSERT(rhs_const->is_const());
      ASSERT(lhs_const->get_sort() == rhs_const->get_sort());

      if (lhs_const != rhs_const) {
        DYN_DEBUG2(correlate, cout << __func__ << " " << __LINE__ << ": lhs_const = " << expr_string(lhs_const) << endl);
        DYN_DEBUG2(correlate, cout << __func__ << " " << __LINE__ << ": rhs_const = " << expr_string(rhs_const) << endl);
        nce->get_graphce_ref()->graphce_counter_example_union(rand_vals);
        return true;
      }
    }
    return false;
  }

  virtual bool is_stable_at_pc(T_PC const& p) const = 0;
  virtual bool check_stability_for_added_edge(shared_ptr<T_E const> const& ed) const = 0;

protected:
  virtual void graph_to_stream(ostream& os) const override
  {
    this->graph_with_proofs<T_PC, T_N, T_E, T_PRED>::graph_to_stream(os);
    for (auto const& pc_ces : m_CE_pernode_map) {
      os << "=counterexamples at pc " << pc_ces.first.to_string() << "\n";
      string prefix = "pc " + pc_ces.first.to_string() + " ";
      pc_ces.second->nodece_set_to_stream(os, prefix);
    }
    os << "=graph_with_ce done\n";
  }

private:
  // returns TRUE iff invariant state is stable after adding (and propagating) CE
  bool add_fresh_counterexample_at_pc_and_propagate(T_PC const &p, counter_example_t const &ce, bool propagate = true) const
  {
    autostop_timer func_timer(__func__);
    autostop_timer func_pc_timer(string(__func__) + "." + p.to_string());

    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    graphce_ref<T_PC, T_N, T_E> gce = mk_graphce<T_PC, T_N, T_E>(ce, p);
    ASSERT(gce);
    map<T_PC, size_t> pc_seen_count;
    nodece_ref<T_PC, T_N, T_E> nce = mk_nodece<T_PC, T_N, T_E>(gce, mk_epsilon_ec<T_PC,T_E>(), ce);

    if (propagate) {
      return add_nodece_at_pc_and_propagate(p, nce, pc_seen_count);
    } else {
      graph_with_ce<T_PC, T_N, T_E, T_PRED>::add_CE_at_pc(p, nce);
      return this->is_stable_at_pc(p);
    }
  }

  void add_fresh_counterexample_at_pc(T_PC const &p, counter_example_t const& ce) const
  {
    add_fresh_counterexample_at_pc_and_propagate(p, ce, /*propagate*/false);
  }

  // add and propagate counterexamples till either of the following happens:
  // (1) predicates stop changing or,
  // (2) invariant state becomes unstable or,
  // (3) we reach the propagation limit
  // return FALSE iff invariant becomes unstable after adding counterexample
  bool add_nodece_at_pc_and_propagate(T_PC const &p, nodece_ref<T_PC, T_N, T_E> const &nce, map<T_PC, size_t> &pc_seen_count) const
  {
    autostop_timer func_timer(__func__);
    //PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    DYN_DEBUG2(ce_add, cout << __func__ << " " << __LINE__ << ": nodece.get_counter_example =\n" << nce->nodece_get_counter_example(this).counter_example_to_string() << endl);

    bool preds_changed_at_p = add_CE_at_pc(p, nce);
    if (!this->is_stable_at_pc(p)) {
      DYN_DEBUG(eqcheck, cout << __func__ << ':' << __LINE__ << ": stability check failed at " << p.to_string() << endl);
      return false;
    }
    if (   !nce->nodece_get_path()->is_epsilon() //if path is epsilon, we propagate unconditionally
        && !preds_changed_at_p) {
      DYN_DEBUG(ce_add, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": not propagating because nodece's path is not epsilon, and preds did not change at pc " << p.to_string() << "\n");
      return true; //the CE did not increase the rank of the column space at pc P; don't propagate it further because it is likely to be useless
    }
    list<shared_ptr<T_E const>> outgoing;
    this->get_edges_outgoing(p, outgoing);
    for (const auto &ed : outgoing) {
      CPP_DBG_EXEC(CE_ADD, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": calling propagate counter_example_across_ec\n");
      nodece_ref<T_PC, T_N, T_E> new_nce = propagate_counter_example_across_ec(nce, mk_edge_composition<T_PC,T_E>(ed));
      CPP_DBG_EXEC(CE_ADD, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": returned from propagate counter_example_across_ec\n");
      if (new_nce) {
        if (!pc_seen_count.count(p)) {
          pc_seen_count.insert(make_pair(p, 0));
        }
        if (pc_seen_count.at(p) < FRESH_COUNTER_EXAMPLE_MAX_PROP_COUNT) {
          pc_seen_count.at(p)++;
          bool is_stable = add_nodece_at_pc_and_propagate(ed->get_to_pc(), new_nce, pc_seen_count);
          if (!is_stable) {
            // stop propagtion as invariant state eventually become unstable and this path is uniikely to be fruitful
            return false;
          }
        }
      } else {
        CPP_DBG_EXEC(CE_ADD, cout << __func__ << " " << __LINE__ << ": propagation failed across edge " << ed->to_string() << endl);
      }
    }
    //PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    return true;
  }

  nodece_ref<T_PC, T_N, T_E> propagate_counter_example_across_ec(nodece_ref<T_PC, T_N, T_E> const &nce, graph_edge_composition_ref<T_PC,T_E> const &pth) const
  {
    autostop_timer func_timer(__func__);
    context *ctx = this->get_context();
    counter_example_t const &ce = nce->nodece_get_counter_example(this);
    counter_example_t out_ce(ctx);
    counter_example_t rand_vals(ctx);
    DYN_DEBUG(ce_add, cout << __func__ << " " << __LINE__ << ": propagating across " << pth->graph_edge_composition_to_string() << ". ce =\n" << ce.counter_example_to_string() << "\n");
    bool successfully_propagated = this->counter_example_translate_on_edge_composition(ce, pth, out_ce, rand_vals, this->get_relevant_memlabels()).first;
    if (!successfully_propagated) {
      DYN_DEBUG(ce_add, cout << __func__ << " " << __LINE__ << ": propagation across " << pth->graph_edge_composition_to_string() << " failed.\n");
      return nullptr;
    }
    out_ce.counter_example_union(rand_vals);
    DYN_DEBUG(ce_add, cout << __func__ << " " << __LINE__ << ": CE propagated across " << pth->graph_edge_composition_to_string() << " =\n" << out_ce.counter_example_to_string() << endl);
    shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &ce_pth = nce->nodece_get_path();
    shared_ptr<graph_edge_composition_t<T_PC,T_E> const> new_pth = mk_series(ce_pth, pth);
    nodece_ref<T_PC, T_N, T_E> ret = mk_nodece(nce->get_graphce_ref(), new_pth, out_ce);
    ret->get_graphce_ref()->graphce_counter_example_union(rand_vals);
    return ret;
  }

  friend class nodece_t<T_PC, T_N, T_E>;
protected:
  virtual bool add_CE_at_pc(T_PC const &p, nodece_ref<T_PC, T_N, T_E> const &nce) const
  {
    autostop_timer func_timer(__func__);
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    DYN_DEBUG(ce_add, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": adding CE at " << p.to_string() << ":\n" << nce->nodece_get_counter_example(this).counter_example_to_string() << endl);
    list<nodece_ref<T_PC, T_N, T_E>> nces;
    if (m_CE_pernode_map.count(p)) {
      nces = m_CE_pernode_map[p]->get_nodeces();
    }
    nces.push_back(nce);
    m_CE_pernode_map[p] = mk_nodece_set(nces);
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    return true; //return true by default; will be overridden in derived class
  }
};

//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//void print_cg_helper(graph_with_ce<T_PC, T_N, T_E, T_PRED> const &g);

}
