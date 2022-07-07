#pragma once

#include "support/timers.h"
#include "support/utils.h"
#include "support/log.h"
#include "support/globals_cpp.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/pc.h"
#include "expr/proof_result.h"

#include "gsupport/sprel_map.h"
#include "gsupport/edge_with_cond.h"
#include "gsupport/failcond.h"
#include "gsupport/memlabel_assertions.h"

#include "graph/graph.h"
#include "graph/graph_with_execution.h"
//#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"
#include "graph/prove.h"
#include "graph/reason_for_counterexamples.h"

namespace eqspace {

using namespace std;

//graph_with_proof_obligations; graph_with_proofs for short
template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_proofs : public graph_with_execution<T_PC, T_N, T_E, T_PRED>
{
private:
  dshared_ptr<memlabel_assertions_t> m_memlabel_assertions;

  using T_PRED_REF = shared_ptr<T_PRED const>;
  using T_PRED_SET = unordered_set<T_PRED_REF>;
public:
  graph_with_proofs(string const &name, string const& fname, context *ctx/*, memlabel_assertions_t const& mlasserts*/)
    : graph_with_execution<T_PC, T_N, T_E, T_PRED>(name, fname, ctx),
      //m_memlabel_assertions(mlasserts)
      m_memlabel_assertions(mk_memlabel_assertions(ctx))
  {
  }

  graph_with_proofs(graph_with_proofs const &other)
    : graph_with_execution<T_PC, T_N, T_E, T_PRED>(other),
      m_memlabel_assertions(mk_memlabel_assertions(other.m_memlabel_assertions)) // or, just copy pointers?
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  void graph_ssa_copy(graph_with_proofs const& other)
  {
    graph_with_path_sensitive_locs<T_PC, T_N, T_E, T_PRED>::graph_ssa_copy(other);
    ASSERT(other.graph_get_memlabel_assertion_submap().empty());
  }

  graph_with_proofs(istream& is, string const& name, std::function<dshared_ptr<T_E const> (istream&, string const&, string const&, context*)> read_edge_fn, context* ctx)
    : graph_with_execution<T_PC, T_N, T_E, T_PRED>(is, name, read_edge_fn, ctx),
      m_memlabel_assertions(mk_memlabel_assertions(is, ctx))
  {
    autostop_timer ft(string("graph_with_proofs_constructor.") + name);
    string line;
    bool end = !getline(is, line);
    ASSERT(!end);
    ASSERT(line == "=graph_with_proofs done");
  }

  virtual ~graph_with_proofs() = default;

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;

  virtual rodata_map_t const &get_rodata_map() const
  {
    static rodata_map_t empty_rodata_map;
    return empty_rodata_map; //return empty for now; CG would return something non-empty
  }

  dshared_ptr<memlabel_assertions_t> const& graph_get_memlabel_assertions() const { ASSERT(m_memlabel_assertions); return m_memlabel_assertions; }
  //set<memlabel_ref> const&     get_relevant_memlabels() const  { return m_memlabel_assertions->get_relevant_memlabels(); }
  bool       graph_uses_concrete_memlabel_assertion() const    { return m_memlabel_assertions->uses_concrete_assertion(); }
  map<expr_id_t, pair<expr_ref, expr_ref>> graph_get_memlabel_assertion_submap() const { return m_memlabel_assertions->memlabel_assertions_get_submap(); }
  map<expr_id_t, pair<expr_ref, expr_ref>> graph_create_concrete_mlasserts_copy_and_get_submap() const 
  {
    ASSERT(!this->graph_uses_concrete_memlabel_assertion());
    dshared_ptr<memlabel_assertions_t> mlasserts2 = mk_memlabel_assertions(*m_memlabel_assertions);
    mlasserts2->memlabel_assertions_use_concrete_assertion();
    return mlasserts2->memlabel_assertions_get_submap();
  }

  void use_abstract_memlabel_assertion() { m_memlabel_assertions->memlabel_assertions_use_abstract_assertion(); }

  virtual expr_ref graph_get_memlabel_assertion_on_preallocated_memlabels_for_all_memallocs() const { return this->get_context()->mk_bool_true(); }

  //decide_hoare_triple:
  //sets the proof status of POST; returns true iff post is made weaker (either by marking it not-provable or by strengthening its local_sprel_expr_assumes_required
  virtual proof_status_t decide_hoare_triple(list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const &pre, T_PC const &from_pc, graph_edge_composition_ref<T_PC,T_E> const &pth, shared_ptr<T_PRED const> const &post, bool& guess_made_weaker, reason_for_counterexamples_t const& reason_for_counterexamples) const
  {
    autostop_timer func_timer(string(__func__) +  ".proof");
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    proof_status_t ret;
    list<counter_example_t> ret_ces;
    proof_result_t proof_result = decide_hoare_triple_helper(pre, from_pc, pth, post/*, ret_ces*/, guess_made_weaker);
    ret = proof_result.get_proof_status();
    ret_ces = proof_result.get_counterexamples();
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    return ret;
  }


  //template <typename T_PRECOND_TFG>
  //static bool prove_using_local_sprel_expr_guesses(context *ctx, list<guarded_pred_t> const &lhs_set, precond_t const& precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref src, expr_ref dst, query_comment const &qc, bool timeout_res, dshared_ptr<memlabel_assertions_t> const& mlasserts, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg, T_PC const &pp, list<counter_example_t> &counter_example); //this is public to allow tools/prove_using_local_sprel_expr_guesses.cpp to call this directly

  proof_result_t decide_hoare_triple_helper(list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const &pre, T_PC const &from_pc, graph_edge_composition_ref<T_PC,T_E> const &pth, shared_ptr<T_PRED const> const &post/*, list<counter_example_t> &ret_counter_example*/, bool& guess_made_weaker) const;
  
  proof_result_t decide_hoare_triple_check_ub(list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const &pre_all, T_PC const &from_pc, graph_edge_composition_ref<T_PC,T_E> const &pth/*, local_sprel_expr_guesses_t const& local_sprel_expr_assumes*/, aliasing_constraints_t const& alias_cons, shared_ptr<T_PRED const> const &post/*, list<counter_example_t> &ret_counter_example*/, bool& guess_made_weaker) const;

  //virtual bool graph_has_stack() const = 0;

  // Add the logic to return the mem alloc exprs that are reaching pc 'p'
  virtual vector<expr_ref> graph_get_mem_alloc_exprs(T_PC const& p) const
  {
//    vector<expr_ref> ret;
//    if(!this->is_SSA())
//      p = this->get_entry_pc();
//    return this->get_reaching_mem_alloc_expr_at_pc(p); 
    vector<expr_ref> ret = { this->get_memallocvar_version_at_pc(p) };
    //bool success = this->get_start_state().get_memory_expr(this->get_start_state(), mem_expr);
    //ASSERT(success);
    //vector<expr_ref> ret = { get_corresponding_mem_alloc_from_mem_expr(mem_expr) };
    return ret;
  }

  void populate_memlabel_assertions();
  //virtual local_sprel_expr_guesses_t get_local_sprel_expr_assumes() const = 0;
  bool check_wfconds_on_edge(dshared_ptr<T_E const> const& e_in) const;
  bool check_wfconds_on_outgoing_edges(T_PC const &p) const;

  static void inc_well_formedness_check_failed_stats_counter(shared_ptr<T_PRED const> const& pred);
  static void inc_edge_assumes_sat_failed_stats_counter(shared_ptr<T_PRED const> const& pred);

  virtual set<shared_ptr<T_PRED const>> get_sp_version_relations_preds_at_pc(T_PC const &p) const { return {}; }

protected:
  virtual predicate_ref check_preds_on_edge_compositions(T_PC const& from_pc, unordered_set<expr_ref> const& preconditions, map<src_dst_cg_path_tuple_t, unordered_set<shared_ptr<T_PRED const>>> const& ec_preds) const = 0;

  virtual bool mark_graph_unstable(T_PC const& pp, failcond_t const& failcond) const = 0;

private:
  virtual void graph_with_guessing_sync_preds() const = 0;

  bool
  proof_query_is_trivial_helper(list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const &lhs_set, expr_ref const &src, expr_ref const &dst, bool &proof_result, T_PC const& from_pc, list<counter_example_t> &counter_example) const
  {
    autostop_timer ft(__func__);

    if (src == dst) {
      proof_result = true;
      return true;
    }
    for (const auto &ec_lhs_pred : lhs_set) {
      graph_edge_composition_ref<pc,tfg_edge> ec = ec_lhs_pred.first;
      shared_ptr<T_PRED const> lhs_pred = ec_lhs_pred.second;
      precond_t const &precond = lhs_pred->get_precond();
      expr_ref const &pred_src_expr = lhs_pred->get_lhs_expr();
      expr_ref const &pred_dst_expr = lhs_pred->get_rhs_expr();
      bool precond_trivial = precond.precond_is_trivial();
      if (   precond_trivial
          && pred_src_expr == src
          && pred_dst_expr == dst) {
        proof_result = true;
        DYN_DEBUG(prove_debug,
          cout << __func__ << " " << __LINE__ << ": lhs_pred = " << lhs_pred->to_string(true) << endl;
          cout << __func__ << " " << __LINE__ << ": src = " << expr_string(src) << endl;
          cout << __func__ << " " << __LINE__ << ": dst = " << expr_string(dst) << endl;
          cout << __func__ << " " << __LINE__ << ": precond_trivial = " << precond_trivial << endl;
          cout << __func__ << " " << __LINE__ << ": returning true\n";
        );
        return true;
      }
      //if (   src->get_operation_kind() == expr::OP_SELECT
      //    && dst->get_operation_kind() == expr::OP_SELECT
      //    && pred_src_expr == src->get_args().at(OP_SELECT_ARGNUM_MEM)
      //    && pred_dst_expr == dst->get_args().at(OP_SELECT_ARGNUM_MEM)) {
      //  //if memories are equal, selects on memories are guaranteed to be equal
      //  proof_result = true;
      //  return true;
      //}
    }

    //commenting out the falsify functions because they do not respect some other preconditions (e.g., aliasing constraints) and so can lead to assertion failures later (because CE evaluation returns failure later)
    //if (this->falsify_fcall_proof_query(src, dst, from_pc, counter_example)) {
    //  proof_result = false;
    //  return true;
    //}

    //if (proof_query_falsified_by_CE(lhs_set, src, dst, counter_example)) { //will assign random values in CE as needed
    //  proof_result = false;
    //  return true;
    //}

    return false;
  }

  bool
  proof_query_is_trivial(list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const &lhs_set, expr_ref const &src, expr_ref const &dst, bool &proof_result, T_PC const& from_pc, list<counter_example_t> &counter_example) const
  {
    bool ret = proof_query_is_trivial_helper(lhs_set, src, dst, proof_result, from_pc, counter_example);
    if (ret)
      return true;

    context *ctx = src->get_context();
    expr_ref bool_true = ctx->mk_bool_true();
    // if top level operation is OR then check if the atoms can be proven trivially
    if (   (   src->get_operation_kind() == expr::OP_OR
            && dst == bool_true)
        || (   dst->get_operation_kind() == expr::OP_OR
            && src == bool_true)) {
      expr_ref or_expr = src->get_operation_kind() == expr::OP_OR ? src : dst;
      for (auto const& arg : or_expr->get_args()) {
        if (proof_query_is_trivial_helper(lhs_set, arg, bool_true, proof_result, from_pc, counter_example)) {
          return true;
        }
      }
    }
    return false;
  }

  //virtual bool falsify_fcall_proof_query(expr_ref const& src, expr_ref const& dst, T_PC const& from_pc, counter_example_t& counter_example) const
  //{
  //  // implemented in CG
  //  return false;
  //}

  //static bool proof_query_falsified_by_CE(unordered_set<shared_ptr<T_PRED const>> const &lhs_set, expr_ref const &src, expr_ref const &dst, set<memlabel_ref> const& relevant_memlabels, counter_example_t &counter_example)
  //{
  //  return false; //NOT_IMPLEMENTED()
  //  size_t MAX_ITER = 2;
  //  context *ctx = src->get_context();
  //  T_PRED_REF post = T_PRED::mk_predicate_ref(precond_t(), src, dst, "falsify-try-post");
  //  for (size_t iter  = 0; iter < MAX_ITER; iter++) {
  //    if (adjust_CE_to_falsify_proof_query(lhs_set, post, relevant_memlabels, counter_example)) {
  //      return true;
  //    }
  //  }
  //  return false;
  //}

  //static bool adjust_CE_to_falsify_proof_query(unordered_set<shared_ptr<T_PRED const>> const &lhs_set, shared_ptr<T_PRED const> const &post, set<memlabel_ref> const& relevant_memlabels, counter_example_t &counter_example)
  //{
  //  for (const auto &lhs_pred : lhs_set) {
  //    if (!lhs_pred->predicate_try_adjust_CE_to_prove_pred(counter_example)) {
  //      return false;
  //    }
  //  }
  //  if (!post->predicate_try_adjust_CE_to_disprove_pred(counter_example)) {
  //    return false;
  //  }

  //  return CE_falsifies_proof_query(lhs_set, post, relevant_memlabels, counter_example);
  //}

  //static bool CE_falsifies_proof_query(unordered_set<shared_ptr<T_PRED const>> const &lhs_set, shared_ptr<T_PRED const> const &post, set<memlabel_ref> const& relevant_memlabels, counter_example_t &counter_example)
  //{
  //  for (const auto &lhs_pred : lhs_set) {
  //    bool eval_result;
  //    if (!lhs_pred->predicate_eval_on_counter_example(relevant_memlabels, eval_result, counter_example)) {
  //      CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": lhs predicate evaluation failed on counter_example. pred = " << lhs_pred->to_string(true) << ", counter_example =\n" << counter_example.counter_example_to_string() << endl);
  //      return false;
  //    }
  //    if (!eval_result) {
  //      CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": lhs predicate evaluation returned false on counter_example. pred = " << lhs_pred->to_string(true) << ", counter_example =\n" << counter_example.counter_example_to_string() << endl);
  //      return false;
  //    }
  //  }
  //  bool eval_result;
  //  if (!post->predicate_eval_on_counter_example(relevant_memlabels, eval_result, counter_example)) {
  //    CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": post predicate evaluation failed on counter_example. pred = " << post->to_string(true) << ", counter_example =\n" << counter_example.counter_example_to_string() << endl);
  //    return false;
  //  }
  //  if (eval_result) {
  //    CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": post predicate evaluation returned false on counter_example. pred = " << post->to_string(true) << ", counter_example =\n" << counter_example.counter_example_to_string() << endl);
  //    return false;
  //  }
  //  return true;
  //}

  pair<expr_ref, expr_ref> pred_lhs_rhs_convert_memmask_to_select_for_symbols_and_locals(context *ctx, shared_ptr<T_PRED const> const &pred) const;

  //static set<local_id_t>
  //identify_local_variables_in_memmask_expr(expr_ref const &e)
  //{
  //  autostop_timer func_timer(__func__);
  //  set<local_id_t> src_locals;
  //  if (e->get_operation_kind() == expr::OP_MEMMASK) {
  //    memlabel_t const &ml = e->get_args().at(OP_MEMMASK_ARGNUM_MEMLABEL)->get_memlabel_value();
  //    if (memlabel_t::memlabel_contains_local(&ml)) {
  //      ASSERT(ml.get_type() == memlabel_t::MEMLABEL_MEM);
  //      ASSERT(ml.get_alias_set_size() == 1);
  //      alias_region_t const &as_elem = *ml.get_alias_set().begin();
  //      ASSERT(as_elem.alias_region_get_alias_type() == alias_region_t::alias_type_local);
  //      local_id_t local_id = as_elem.alias_region_get_var_id();
  //      src_locals.insert(local_id);
  //    }
  //  }
  //  return src_locals;
  //}

  //virtual tfg_suffixpath_t get_src_suffixpath_ending_at_pc(T_PC const &p) const = 0;
  virtual avail_exprs_t const &get_src_avail_exprs/*_at_pc*/(/*T_PC const &p*/) const = 0;

  template<typename T_PRECOND_TFG>
  proof_result_t prove_wrapper(list<pair<graph_edge_composition_ref<pc,tfg_edge>, shared_ptr<T_PRED const>>> const &lhs_in, precond_t const& precond, graph_edge_composition_ref<pc, tfg_edge> const& eg/*, local_sprel_expr_guesses_t const&local_sprel_expr_assumes_required_to_prove*/, aliasing_constraints_t const& alias_cons, expr_ref src, expr_ref dst, query_comment const &qc/*, bool timeout_res*//*, set<allocsite_t> const &src_locals*/, T_PC const &pp/*, list<counter_example_t> &counter_example*/) const;
};

}
