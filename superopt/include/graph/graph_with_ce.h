#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"
#include "support/dyn_debug.h"

#include "expr/sprel_map_pair.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "expr/pc.h"
#include "expr/local_sprel_expr_guesses.h"

#include "gsupport/sprel_map.h"
#include "gsupport/edge_with_cond.h"
#include "gsupport/sprel_map.h"

#include "graph/graph.h"
#include "graph/graph_with_proofs.h"
//#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"
#include "graph/graphce.h"
#include "graph/nodece.h"
#include "graph/nodece_set.h"
#include "graph/reason_for_counterexamples.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_ce : public graph_with_proofs<T_PC, T_N, T_E, T_PRED>
{
private:
  static int const FRESH_COUNTER_EXAMPLE_MAX_PROP_COUNT = 3;
  static int const RANKING_CES_PROPAGATION_THRESHOLD = 20;

  //m_global_CE_set: updated using virtual function add_CE_at_pc;
  //read using get_counterexamples_at_pc()
  //should not be updated through any other function (because add_CE_at_pc is overridden in derived classes)
  mutable map<reason_for_counterexamples_t, nodece_set_ref<T_PC, T_N, T_E>> m_global_CE_set;

  //stats
  mutable std::map<T_PC, map<reason_for_counterexamples_t, unsigned>> m_CE_generated_pernode_map;
  mutable std::map<T_PC, map<reason_for_counterexamples_t, unsigned>> m_CE_generated_added_pernode_map;
  list<T_PC> m_topological_ordered_pcs;
 
public:
  graph_with_ce(string const &name, string const& fname, context *ctx/*, memlabel_assertions_t const& memlabel_assertions*/)
    : graph_with_proofs<T_PC, T_N, T_E, T_PRED>(name, fname, ctx/*, memlabel_assertions*/)
  { }

  graph_with_ce(graph_with_ce const &other)
    : graph_with_proofs<T_PC, T_N, T_E, T_PRED>(other), m_global_CE_set(other.m_global_CE_set), m_CE_generated_pernode_map(other.m_CE_generated_pernode_map), m_CE_generated_added_pernode_map(other.m_CE_generated_added_pernode_map), m_topological_ordered_pcs(other.m_topological_ordered_pcs)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  void graph_ssa_copy(graph_with_ce const& other)
  {
    graph_with_proofs<T_PC, T_N, T_E, T_PRED>::graph_ssa_copy(other);
    ASSERT(other.m_global_CE_set.empty());
  }

  graph_with_ce(istream& is, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx) : graph_with_proofs<T_PC, T_N, T_E, T_PRED>(is, name, read_edge_fn, ctx)
  {
    autostop_timer ft(string("graph_with_ce_constructor.") + name);
    string line;
    bool end;
    do {
      end = !getline(is, line);
      ASSERT(!end);
    } while (line == "");
    string const prefix = "=global counterexamples for reason ";
    while (is_line(line, prefix)) {
      string reason_str = line.substr(prefix.size());
      reason_for_counterexamples_t reason = reason_for_counterexamples_t::reason_for_counterexamples_from_string(reason_str);
      nodece_set_ref<T_PC, T_N, T_E> nodece_set;
      string prefix = "global ";
      line = nodece_set_t<T_PC, T_N, T_E>::nodece_set_from_stream(is, *this, ctx, prefix, nodece_set);
      bool inserted = m_global_CE_set.insert(make_pair(reason, nodece_set)).second;
      ASSERT(inserted);
    }
    if (line != "=graph_with_ce done") {
      cout << _FNLN_ << ": line = '" << line  << "'\n";
    }
    ASSERT(line == "=graph_with_ce done");
  }

  virtual ~graph_with_ce() = default;

  //concrete_values_set_t<T_PC,T_N,T_E,T_PRED> & get_concrete_values_set() { return m_concrete_values; }
  //concrete_values_set_t<T_PC,T_N,T_E,T_PRED> const& get_concrete_values_set() const { return m_concrete_values; }
  //void get_concrete_ces_at_pc(T_PC const &p, list<counter_example_t> &ces ) const
  //{
  //  ces.clear();
  //  for(auto const& nce : this->get_counterexamples_at_pc(p))
  //    ces.push_back(nce->nodece_get_counter_example());
  //}
  virtual bool CE_copy_required_for_input_edge(edge_id_t<T_PC> const& e) const
  {
    if (!a_preceeds_b_in_topological_order(e.get_from_pc(), e.get_to_pc())) {
      return true;
    }
    return false;
  }

  bool a_preceeds_b_in_topological_order(T_PC const& a, T_PC const& b) const
  {
 
    DYN_DEBUG2(ce_clone, cout << _FNLN_ << "a " << a.to_string() << ", b: " << b.to_string() << endl;
                        cout << "order: " << endl;
                        for(auto const& p : m_topological_ordered_pcs) {
                          cout << p.to_string() << endl;
                        }
             );
    if(a == b)
      return false;
    bool found_a = false;
    bool found_b = false;
    for(auto const& p : m_topological_ordered_pcs) {
      if (p == a) {
        found_a = true;
        if (found_b)
          return false;
      } else if (p == b) {
        found_b = true;
        if (found_a)
          return true;
      }
    }
    cout << _FNLN_ << "a " << a.to_string() << ", b: " << b.to_string() << endl;
    cout << "order: " << endl;
    for(auto const& p : m_topological_ordered_pcs) {
      cout << p.to_string() << endl;
    }
    NOT_REACHED();
  }

  virtual void populate_pcs_topological_order()
  {
    m_topological_ordered_pcs = this->topological_sorted_labels();
  }

  void set_pcs_topological_order(list<T_PC> const& sorted_pcs)
  {
    m_topological_ordered_pcs = sorted_pcs;
  }

  list<nodece_ref<T_PC, T_N, T_E>> get_counterexamples_with_last_visted_pc(T_PC const& p, reason_for_counterexamples_t const& reason) const
  {
    list<nodece_ref<T_PC, T_N, T_E>> ret;
    for (auto const& nce : this->get_counterexamples_for_reason(reason)) {
      if(nce->nodece_get_last_visited_pc() == p)
        ret.push_back(nce);
    }
    return ret;
  }

  unsigned get_num_counterexamples_visited_pc(T_PC const& p, reason_for_counterexamples_t const& reason) const
  {
    unsigned ret = 0;
    for (auto const& nce : this->get_counterexamples_for_reason(reason)) {
      if (nce->ce_visited_input_pc(p))
        ret++;
    }
    return ret;
  }

  list<nodece_ref<T_PC, T_N, T_E>> const& get_counterexamples_for_reason(reason_for_counterexamples_t const& reason) const
  {
    if (!m_global_CE_set.count(reason)) {
      static list<nodece_ref<T_PC, T_N, T_E>> empty_set;
      return empty_set;
    }
    return m_global_CE_set.at(reason)->get_nodeces();
  }

  unsigned get_counterexamples_generated_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason) const
  {
    if (!m_CE_generated_pernode_map.count(p) || !m_CE_generated_pernode_map.at(p).count(reason)) {
      return 0;
    }
    return m_CE_generated_pernode_map.at(p).at(reason);
  }

  unsigned get_counterexamples_generated_added_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason) const
  {
    if (!m_CE_generated_added_pernode_map.count(p) || !m_CE_generated_added_pernode_map.at(p).count(reason)) {
      return 0;
    }
    return m_CE_generated_added_pernode_map.at(p).at(reason);
  }

//  nodece_ref<T_PC, T_N, T_E> const &get_most_recent_counter_example_at_pc(T_PC const &p) const
//  {
//    ASSERT(m_global_CE_set.count(p));
//    ASSERT(m_global_CE_set.at(p)->size() > 0);
//    return m_global_CE_set.at(p)->get_nodeces().back();
//  }

//  nodece_ref<T_PC, T_N, T_E> get_random_counter_example_at_pc(T_PC const &p) const
//  {
//    if (   !m_global_CE_set.count(p)
//        || m_global_CE_set.at(p)->size() == 0) {
//      return nullptr;
//    }
//    unsigned victim_idx = random_number(m_global_CE_set.at(p)->size());
//    return *next(m_global_CE_set.at(p)->get_nodeces().begin(), victim_idx);
//  }

  proof_status_t
  decide_hoare_triple(list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const &pre, T_PC const &from_pc, graph_edge_composition_ref<T_PC,T_E> const &pth, shared_ptr<T_PRED const> const &post, bool &guess_made_weaker, reason_for_counterexamples_t const& reason_for_counterexamples) const override
  {
    autostop_timer func_timer(string(__func__) + ".ce");
    ASSERT(pth);
    //list<nodece_ref<T_PC, T_N, T_E>> const &ce_set = this->get_counterexamples_at_pc(from_pc);
    //the following check is not needed because the CEs are propagated as they are created anyways
    //also before uncommenting the following check: we should also perhaps check that the CEs in ce_set satisfy PRE.
    //list<nodece_ref<T_PC, T_N, T_E>> txed_ce_set = propagate_counter_examples_across_ec(ce_set, pth);
    //if (disprove_predicate_through_counter_examples_on_path(post, txed_ce_set)) {
    //  ASSERT(post.get_proof_status() == predicate::not_provable);
    //  CPP_DBG_EXEC(DECIDE_HOARE_TRIPLE, cout << "proof query failed\n");
    //  return true;
    //}
    list<counter_example_t> returned_ces;
    proof_status_t ret;
    proof_result_t proof_result = graph_with_proofs<T_PC, T_N, T_E, T_PRED>::decide_hoare_triple_helper(pre, from_pc, pth, post/*, returned_ces*/, guess_made_weaker);
    ret = proof_result.get_proof_status();
    returned_ces = proof_result.get_counterexamples();
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    if (ret == proof_status_disproven) {
      DYN_DEBUG(decide_hoare_triple_debug, cout << "proof query failed; calling add_fresh_counterexample_at_pc_and_propagate()\n");
      if (returned_ces.empty()) {
        DYN_DEBUG(decide_hoare_triple, cout << "WARNING no counter examples\n");
        return proof_status_timeout;
      }
      /*if (pth && pth->is_epsilon()) {
        // no path to propagate CE; just add them to from_pc
        for (auto const& ret_ce : returned_ces) {
          ASSERT (!ret_ce.is_empty());
          this->add_fresh_counterexample_at_pc(from_pc, ret_ce);
        }
      } else */{
        unsigned before_ces = 0;
        if (pth && !pth->is_epsilon()) {
          T_PC const& to_pc = pth->graph_edge_composition_get_to_pc();
          before_ces = this->get_num_counterexamples_visited_pc(to_pc, reason_for_counterexamples);
        }
        for (auto const& ret_ce : returned_ces) {
          ASSERT (!ret_ce.is_empty());
          //if (pth && !pth->is_epsilon()) {
          bool is_stable = this->add_fresh_counterexample_at_pc_and_propagate(from_pc, reason_for_counterexamples, ret_ce, pth);
          if (!is_stable) {
            break;
          }
         //}
        }
        if (pth && !pth->is_epsilon()) {
          T_PC const& to_pc = pth->graph_edge_composition_get_to_pc();
          if (m_CE_generated_pernode_map.count(to_pc) == 0)
            m_CE_generated_pernode_map[to_pc][reason_for_counterexamples] = 0;
          m_CE_generated_pernode_map[to_pc][reason_for_counterexamples] += returned_ces.size();
          unsigned added_ces = this->get_num_counterexamples_visited_pc(to_pc, reason_for_counterexamples) - before_ces;
          if (m_CE_generated_added_pernode_map.count(to_pc) == 0)
            m_CE_generated_added_pernode_map[to_pc][reason_for_counterexamples] = 0;
          m_CE_generated_added_pernode_map[to_pc][reason_for_counterexamples] += added_ces;
        }
      }
    }
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return ret;
  }
//  virtual void generate_new_CE(T_PC const& from_pc, graph_edge_composition_ref<T_PC, T_E> const &dst_ec) const = 0;

  //returns true if disprove was successful
  //bool disprove_predicate_through_counter_examples(unordered_set<shared_ptr<T_PRED const>> const& assumes, expr_ref const& precond_expr, expr_ref const& lhs_expr, expr_ref const& rhs_expr, list<nodece_ref<T_PC, T_N, T_E>> const &ce_set) const
  //{
  //  //Some code de-duplication with propagate_CE_over_edge, try to merge in a single function
  //  context *ctx = this->get_context();
  //  //precond_t const &precond = pred.get_precond();
  //  //expr_ref const &lhs_expr = pred.get_lhs_expr();
  //  //expr_ref const &rhs_expr = pred.get_rhs_expr();
  //  //edge_guard_t const &eg = precond.precond_get_guard();
  //  //local_sprel_expr_guesses_t const &lse_assumes = precond.get_local_sprel_expr_assumes();
  //  set<memlabel_ref> const& relevant_memlabels = this->get_relevant_memlabels();

  //  //expr_ref precond_expr = precond.precond_get_expr(this->get_src_tfg(), true);

  //  for (nodece_ref<T_PC, T_N, T_E> const &nce : ce_set) {
  //    counter_example_t const &ce = nce->nodece_get_counter_example(this);
  //    counter_example_t rand_vals(ctx);
  //    expr_ref const_val;
  //    if (!ce.evaluate_expr_assigning_random_consts_as_needed(precond_expr, const_val, rand_vals, relevant_memlabels)) {
  //      continue;
  //    }
  //    ASSERT(const_val->is_bool_sort());
  //    ASSERT(const_val->is_const());
  //    if (!const_val->get_bool_value()) {
  //      continue;
  //    }
  //    if (!this->counter_example_satisfies_preds_at_pc(ce, assumes, rand_vals, relevant_memlabels)) {
  //      continue;
  //    }
  //    expr_ref lhs_const, rhs_const;
  //    if (!ce.evaluate_expr_assigning_random_consts_as_needed(lhs_expr, lhs_const, rand_vals, relevant_memlabels)) {
  //      continue;
  //    }
  //    if (!ce.evaluate_expr_assigning_random_consts_as_needed(rhs_expr, rhs_const, rand_vals, relevant_memlabels)) {
  //      continue;
  //    }
  //    ASSERT(lhs_const->is_const());
  //    ASSERT(rhs_const->is_const());
  //    ASSERT(lhs_const->get_sort() == rhs_const->get_sort());

  //    if (lhs_const != rhs_const) {
  //      DYN_DEBUG2(correlate, cout << __func__ << " " << __LINE__ << ": lhs_const = " << expr_string(lhs_const) << endl);
  //      DYN_DEBUG2(correlate, cout << __func__ << " " << __LINE__ << ": rhs_const = " << expr_string(rhs_const) << endl);
  //      nce->get_graphce_ref()->graphce_counter_example_union(rand_vals);
  //      return true;
  //    }
  //  }
  //  return false;
  //}

  virtual void add_edge(dshared_ptr<T_E const> e) override;
  void graph_propagate_CEs_across_new_edge(dshared_ptr<T_E const> const &ed);

  virtual bool graph_is_stable() const = 0;
  //virtual bool is_stable_at_pc(T_PC const& p) const = 0;
  //virtual bool check_stability_for_added_edge(shared_ptr<T_E const> const& ed) const = 0;
  
  virtual bool is_memvar_or_memalloc_var(expr_ref const& e) const
  {
    ASSERT(e->is_var());
    string name = e->get_name()->get_str(); 
    if(e->is_array_sort() && (string_has_prefix(name, this->get_memvar_version_at_pc(T_PC::start())->get_name()->get_str()) || string_has_prefix(name, this->get_memvar_version_at_pc(T_PC::start())->get_name()->get_str())))
      return true;
    if(e->is_memalloc_sort() && (string_has_prefix(name, this->get_memallocvar_version_at_pc(T_PC::start())->get_name()->get_str()) || string_has_prefix(name, this->get_memallocvar_version_at_pc(T_PC::start())->get_name()->get_str())))
      return true;
    return false;

  }

protected:
  virtual void graph_to_stream(ostream& os, string const&prefix="") const override
  {
    this->graph_with_proofs<T_PC, T_N, T_E, T_PRED>::graph_to_stream(os, prefix);
    for (auto const& [reason, ces] : m_global_CE_set) {
      os << "=global counterexamples for reason " << reason.reason_for_counterexamples_to_string() << "\n";
      string prefix = "global ";
      ces->nodece_set_to_stream(os, prefix);
    }
    os << "=graph_with_ce done\n";
  }

  // returns TRUE iff invariant state is stable after adding (and propagating) CE
  virtual bool add_fresh_counterexample_at_pc_and_propagate(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, counter_example_t const &ce, graph_edge_composition_ref<T_PC,T_E> const &query_path_on_which_the_counterexample_was_generated) const
  {
    autostop_timer func_timer(__func__);
    autostop_timer func_pc_timer(string(__func__) + "." + p.to_string() + "." + reason_for_counterexamples.reason_for_counterexamples_to_string());

    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
//    graphce_ref<T_PC, T_N, T_E> gce = mk_graphce<T_PC, T_N, T_E>(ce, p, reason_for_counterexamples.reason_for_counterexamples_to_string() + "." + graphce_t<T_PC,T_N,T_E>::get_next_graphce_name());
//    ASSERT(gce);

    DYN_DEBUG(ce_add, cout << _FNLN_ << " " << "fresh counterexample generated at " << p.to_string() << " for " << reason_for_counterexamples.reason_for_counterexamples_to_string() << " to be translated on path " << query_path_on_which_the_counterexample_was_generated->graph_edge_composition_to_string() << ":\n" << ce.counter_example_to_string() << endl);

    context* ctx = this->get_context();
    T_PC add_pc;
    nodece_ref<T_PC, T_N, T_E> nce;
    if(!query_path_on_which_the_counterexample_was_generated->is_epsilon()) {
      counter_example_t rand_vals(ctx, ctx->get_next_ce_name(RAND_VALS_CE_NAME_PREFIX));
      counter_example_t out_ce = ce;
      auto retval = this->counter_example_translate_on_edge_composition(out_ce, query_path_on_which_the_counterexample_was_generated, rand_vals, this->graph_get_relevant_memlabels());
      if (!retval.is_true()) {
        cout << "Failed to translate fresh counterexample over the path (" << query_path_on_which_the_counterexample_was_generated->graph_edge_composition_to_string() << ") where it was generated.  Replaying for debugging..." << endl;
        set_debug_class_level("ce_add", 2);
        set_debug_class_level("ce_translate", 2);
        set_debug_class_level("ce_eval", 2);
        counter_example_t rand_vals(ctx, ctx->get_next_ce_name(RAND_VALS_CE_NAME_PREFIX));
        counter_example_t out_ce = ce;
        this->counter_example_translate_on_edge_composition(out_ce, query_path_on_which_the_counterexample_was_generated, rand_vals, this->graph_get_relevant_memlabels());
      }
      ASSERTCHECK(retval.is_true(), cout << "Failed to translate counterexample at " << p.to_string() << " over path " << query_path_on_which_the_counterexample_was_generated->graph_edge_composition_to_string() << ":\n" << ce.counter_example_to_string() << endl);
      add_pc = query_path_on_which_the_counterexample_was_generated->graph_edge_composition_get_to_pc();
//      remove_CE_mappings_for_not_defined_vars_at_pc(add_pc, out_ce);
      nce = mk_nodece<T_PC, T_N, T_E>(/*gce,*/ query_path_on_which_the_counterexample_was_generated, out_ce, 1, {add_pc}/*, this->get_invstate_region_for_pc(add_pc)*/);
    } else {
      add_pc = p;
      counter_example_t out_ce = ce;
//      remove_CE_mappings_for_not_defined_vars_at_pc(add_pc, out_ce);
      nce = mk_nodece<T_PC, T_N, T_E>(/*gce,*/ graph_edge_composition_t<T_PC,T_E>::mk_epsilon_ec(), out_ce, 0, {add_pc}/*, this->get_invstate_region_for_pc(add_pc)*/);
    }
    ASSERT(nce);
    return add_nodece_at_pc_and_propagate(add_pc, reason_for_counterexamples, nce);
  }
  void remove_CE_mappings_for_not_defined_vars_at_pc(T_PC const &p, counter_example_t &ret) const;

private:

  void graph_propagate_CEs_across_new_edge_for_reason(dshared_ptr<T_E const> const &ed, reason_for_counterexamples_t const& reason);
  nodece_ref<T_PC, T_N, T_E> create_new_nodece_after_propagation(nodece_ref<T_PC, T_N, T_E> const& nce_befor_propagation, shared_ptr<graph_edge_composition_t<T_PC, T_E> const> const& path, counter_example_t const& ce_after_propagation) const;

  virtual bool graph_edge_contains_unknown_function_call(dshared_ptr<T_E const> const& ed) const = 0;

  bool add_nodece_at_pc_and_propagate(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<T_PC, T_N, T_E> const &nce) const
  {
    map<T_PC, size_t> pc_seen_count = nce->nodece_get_pc_seen_count();
    DYN_DEBUG(ce_add, cout << _FNLN_ << ": " << p.to_string() << ": calling add_nodece_at_pc_and_propagate_helper for CE " << nce->nodece_get_counter_example().get_ce_name()->get_str() << endl);
    return add_nodece_at_pc_and_propagate_helper(p, reason_for_counterexamples, nce, pc_seen_count);
  }

  // add and propagate counterexamples till either of the following happens:
  // (1) predicates stop changing or,
  // (2) invariant state becomes unstable or,
  // (3) we reach the propagation limit or,
  // (4) we reach an edge that has an unknown function call
  // return FALSE iff invariant becomes unstable after adding counterexample
  bool add_nodece_at_pc_and_propagate_helper(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<T_PC, T_N, T_E> const &nce, map<T_PC, size_t> &pc_seen_count) const
  {
    autostop_timer func_timer(__func__);
    //autostop_timer func_timer_pc(string(__func__) + "." + p.to_string());
    //PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    //DYN_DEBUG2(ce_add, cout << __func__ << " " << __LINE__ << ": nodece.get_counter_example =\n" << nce->nodece_get_counter_example(this).counter_example_to_string() << endl);

    DYN_DEBUG(ce_add, cout << _FNLN_ << ": " << p.to_string() << ": adding nodece for CE " << nce->nodece_get_counter_example().get_ce_name()->get_str() << endl);
    bool preds_changed_at_p = add_CE_at_pc(p, reason_for_counterexamples, nce);
    if (!this->graph_is_stable()) {
      DYN_DEBUG(print_progress_debug, cout << __func__ << ':' << __LINE__ << ": " << p.to_string() << " is not stable; ignoring propagation." << endl);
      return false;
    }

    //DYN_DEBUG2(ce_add, cout << __func__ << " " << __LINE__ << ": returned from add_CE_at_pc()\n");
    /*if (!nce->nodece_get_path()->is_epsilon()) */{ //if path is epsilon, we propagate unconditionally
      if (!preds_changed_at_p) {
        DYN_DEBUG(ce_add, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": not propagating because nodece's path is not epsilon, and preds did not change at pc " << p.to_string() << "\n");
        return true; //the CE did not increase the rank of the column space at pc P; don't propagate it further because it is likely to be useless
      }
    }
    list<dshared_ptr<T_E const>> outgoing = this->get_edges_outgoing(p);
    for (const auto &ed : outgoing) {
      if (this->graph_edge_contains_unknown_function_call(ed)) {
        continue; //do not propagate across an unknown fcall because this may change later
      }
      if (!pc_seen_count.count(p)) {
        pc_seen_count.insert(make_pair(p, 0));
      }
      if (pc_seen_count.at(p) < FRESH_COUNTER_EXAMPLE_MAX_PROP_COUNT) {
        DYN_DEBUG(ce_add, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": calling propagate counter_example_across_edge " << ed->to_string_concise() << "\n");
        nodece_ref<T_PC, T_N, T_E> new_nce = propagate_nodece_across_edge(nce, ed);
        DYN_DEBUG(ce_add, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": returned from propagate counter_example_across_edge " << ed->to_string_concise() << ".  is_stable = " << this->graph_is_stable() << endl);
        if (!this->graph_is_stable()) {
          return false;
        }
        if (new_nce) {
          pc_seen_count.at(p)++;
          DYN_DEBUG(ce_add, cout << _FNLN_ << ": " << p.to_string() << ": calling add_nodece_at_pc_and_propagate_helper for CE " << new_nce->nodece_get_counter_example().get_ce_name()->get_str() << endl);
          bool is_stable = add_nodece_at_pc_and_propagate_helper(ed->get_to_pc(), reason_for_counterexamples, new_nce, pc_seen_count);
          if (!is_stable) {
            // stop propagtion as invariant state eventually become unstable and this path is uniikely to be fruitful
            return false;
          }
          break;
        } else {
          DYN_DEBUG(ce_add, cout << "propagation failed across edge " << ed->to_string() << endl);
        }
      }
    }
    //PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    return true;
  }

  nodece_ref<T_PC, T_N, T_E> propagate_nodece_across_edge(nodece_ref<T_PC, T_N, T_E> const &nce, dshared_ptr<T_E const> const &ed) const;

  friend class nodece_t<T_PC, T_N, T_E>;
protected:
  //returns true if preds changed at p
  virtual bool add_CE_at_pc(T_PC const &p, reason_for_counterexamples_t const& reason_for_counterexamples, nodece_ref<T_PC, T_N, T_E> const &nce) const;
};

//template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
//void print_cg_helper(graph_with_ce<T_PC, T_N, T_E, T_PRED> const &g);

}
