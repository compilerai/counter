#pragma once
#include <fstream>
#include "gsupport/pc.h"
#include "graph/graph.h"
#include "graph/graph_with_execution.h"
#include "support/utils.h"
#include "support/log.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
//#include "predicate.h"
#include "graph/binary_search_on_preds.h"
#include "graph/lr_map.h"
#include "gsupport/sprel_map.h"
/* #include "graph/graph_cp.h" */
#include "expr/local_sprel_expr_guesses.h"
#include "support/globals_cpp.h"
#include "gsupport/edge_with_cond.h"
#include "tfg/edge_guard.h"
//#include "graph/concrete_values_set.h"
#include "support/timers.h"
#include "graph/delta.h"
#include "graph/memlabel_assertions.h"
#include "support/dyn_debug.h"
#include "graph/prove.h"
//#include "graph/invariant_state.h"

#include <map>
#include <list>
#include <cassert>
#include <sstream>
#include <set>
#include <memory>

namespace eqspace {

using namespace std;

//graph_with_proof_obligations; graph_with_proofs for short
template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_proofs : public graph_with_execution<T_PC, T_N, T_E, T_PRED>
{
private:
  memlabel_assertions_t m_memlabel_assertions;

  using T_PRED_REF = shared_ptr<T_PRED const>;
  using T_PRED_SET = unordered_set<T_PRED_REF>;
public:
  graph_with_proofs(string const &name, context *ctx, memlabel_assertions_t const& mlasserts)
    : graph_with_execution<T_PC, T_N, T_E, T_PRED>(name, ctx),
      m_memlabel_assertions(mlasserts)
  { }

  graph_with_proofs(graph_with_proofs const &other)
    : graph_with_execution<T_PC, T_N, T_E, T_PRED>(other),
      m_memlabel_assertions(other.m_memlabel_assertions)
  {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }

  graph_with_proofs(istream& is, string const& name, context* ctx) : graph_with_execution<T_PC, T_N, T_E, T_PRED>(is, name, ctx), m_memlabel_assertions(is, ctx)
  { }

  virtual ~graph_with_proofs() = default;

  virtual void graph_to_stream(ostream& os) const override;

  virtual local_sprel_expr_guesses_t get_local_sprel_expr_assumes() const 
  { //overridden by corr_graph
    return local_sprel_expr_guesses_t();
  }

  virtual list<pair<expr_ref, shared_ptr<T_PRED const>>> edge_composition_apply_trans_funs_on_pred(shared_ptr<T_PRED const> const &p, shared_ptr<graph_edge_composition_t<T_PC,T_E> const> const &ec) const = 0;

  //virtual rodata_map_t const &get_rodata_map() const
  //{
  //  static rodata_map_t empty_rodata_map;
  //  return empty_rodata_map; //return empty for now; CG would return something non-empty
  //}

  memlabel_assertions_t const& get_memlabel_assertions() const { return m_memlabel_assertions; }
  set<memlabel_ref> const&     get_relevant_memlabels() const  { return m_memlabel_assertions.get_relevant_memlabels(); }

  void use_abstract_memlabel_assertion() { m_memlabel_assertions.memlabel_assertions_use_abstract_assertion(); }

  //decide_hoare_triple:
  //sets the proof status of POST; returns true iff post is made weaker (either by marking it not-provable or by strengthening its local_sprel_expr_assumes_required
  virtual proof_status_t decide_hoare_triple(unordered_set<shared_ptr<T_PRED const>> const &pre, T_PC const &from_pc, graph_edge_composition_ref<T_PC,T_E> const &pth, shared_ptr<T_PRED const> const &post, bool& guess_made_weaker) const
  {
    autostop_timer func_timer(string(__func__) +  ".proof");
    context *ctx = this->get_context();
    list<counter_example_t> ret_ces;
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    proof_status_t ret = decide_hoare_triple_helper(pre, from_pc, pth, post, ret_ces, guess_made_weaker);
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    return ret;
  }


  template <typename T_PRECOND_TFG>
  static bool prove_using_local_sprel_expr_guesses(context *ctx, T_PRED_SET const &lhs, precond_t const& precond/*edge_guard_t const &guard*/, local_sprel_expr_guesses_t &local_sprel_expr_assumes_required_to_prove, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, /*rodata_map_t const &rodata_map,*/ graph_memlabel_map_t const &/*src_*/memlabel_map/*, graph_memlabel_map_t const &dst_memlabel_map*/, set<local_sprel_expr_guesses_t> const &all_guesses, expr_ref src, expr_ref dst, query_comment const &qc, bool timeout_res/*, set<cs_addr_ref_id_t> const &relevant_addr_refs, vector<memlabel_ref> const& relevant_memlabels*/, memlabel_assertions_t const& mlasserts, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg, T_PC const &pp, list<counter_example_t> &counter_example); //this is public to allow tools/prove_using_local_sprel_expr_guesses.cpp to call this directly

protected:

  proof_status_t decide_hoare_triple_helper(unordered_set<shared_ptr<T_PRED const>> const &pre, T_PC const &from_pc, graph_edge_composition_ref<T_PC,T_E> const &pth, shared_ptr<T_PRED const> const &post, list<counter_example_t> &ret_counter_example, bool& guess_made_weaker) const
  {
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    autostop_timer func_timer(string(__func__));
    context *ctx = this->get_context();
    //T_PC const &from_pc = pth->graph_edge_composition_get_from_pc();

    DYN_DEBUG2(decide_hoare_triple,
        cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": pre = " << pre.size() << " preds:\n";
        size_t i = 0;
        for (auto const& pred : T_PRED::determinized_pred_set(pre)) {
          cout << "pre." << i++ << ":\n" << pred->to_string_for_eq(true) << endl;
        }
    );
    DYN_DEBUG(decide_hoare_triple,
        cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": decide_hoare_triple new query:\n";
        cout << "from_pc = " << from_pc << endl;
        cout << "pre = [size " << pre.size() << "]\n";
        cout << "pth = " << pth->graph_edge_composition_to_string() << endl;
        cout << "post =\n" << post->to_string_for_eq(true) << endl;
    );

    DYN_DEBUG2(smt_query,
      stringstream ss;
      ss << post->get_comment() << ".from_pc" << from_pc.to_string() << ".path_hash" << md5_checksum(pth->graph_edge_composition_to_string()) << ".pre" << pre.size();
      query_comment qc(ss.str());
      ss.str("");
      string pid = int_to_string(getpid());
      ss << g_query_dir << "/" << DECIDE_HOARE_TRIPLE_FILENAME_PREFIX << "." << qc.to_string();
      string filename = ss.str();
      ofstream fo(filename.data());
      ASSERT(fo);
      this->graph_to_stream(fo);
      fo << "=from_pc " << from_pc.to_string() << endl;
      fo << "=pre size " << pre.size() << endl;
      int i = 0;
      for (auto const& pred : T_PRED::determinized_pred_set(pre)) {
        fo << "=pre." << i++ << ":\n";
        pred->predicate_to_stream(fo, true);
        fo << endl;
      }
      //fo << "=pth\n" << pth->graph_edge_composition_to_string() << endl;
      fo << "=pth\n" << pth->graph_edge_composition_to_string_for_eq("=pth");
      fo << "=post\n";
      post->predicate_to_stream(fo, true);
      fo.close();
      //g_query_files.insert(filename);
      cout << "decide_hoare_triple filename " << filename << '\n';
    );


    DYN_DEBUG2(decide_hoare_triple, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": collect_assumes_around_path()\n");
    T_PRED_SET const &pre_assumes = this->collect_assumes_around_path(from_pc, pth);
    DYN_DEBUG2(decide_hoare_triple, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": collect_inductive_preds_around_path()\n");
    T_PRED_SET const &pre_inductive = this->collect_inductive_preds_around_path(from_pc, pth);
    T_PRED_SET pre_all = pre;
    T_PRED::predicate_set_union(pre_all, pre_assumes);
    T_PRED::predicate_set_union(pre_all, pre_inductive);
    T_PRED::predicate_set_union(pre_all, this->get_global_assumes());

    local_sprel_expr_guesses_t local_sprel_expr_assumes = post->get_local_sprel_expr_assumes();
    local_sprel_expr_assumes.set_union(this->get_local_sprel_expr_assumes());

    DYN_DEBUG2(decide_hoare_triple, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": collect_aliasing_constraints_around_path()\n");
    aliasing_constraints_t alias_cons = this->collect_aliasing_constraints_around_path(from_pc, pth);
    //DYN_DEBUG2(decide_hoare_triple, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": collect_aliasing_constraints from preds pre_all\n");
    //for (auto const& pred : determinized_pred_set(pre_all)) {
    //  alias_cons.aliasing_constraints_union(generate_aliasing_constraints_from_predicate(pred, this->get_src_tfg()));
    //}
    DYN_DEBUG2(decide_hoare_triple, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": aliasing constraints:\n" << alias_cons.to_string_for_eq() << endl);

    DYN_DEBUG2(decide_hoare_triple, cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": edge_composition_apply_trans_funs_on_pred()\n");
    list<pair<expr_ref, T_PRED_REF>> post_apply_set = edge_composition_apply_trans_funs_on_pred(post, pth);
    size_t post_apply_set_size = post_apply_set.size();
    DYN_DEBUG(counters_enable, stats::get().add_counter("query-preds",post_apply_set_size));
    
    size_t post_apply_set_elem_index = 0;
    guess_made_weaker = false;
    proof_status_t ret = proof_status_proven;
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    for (const auto &post_apply_cond_pred : post_apply_set) {
      PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
      expr_ref const &post_apply_cond = post_apply_cond_pred.first;
      shared_ptr<T_PRED const> const &post_apply_pred = post_apply_cond_pred.second;
      precond_t const &post_apply_precond = post_apply_pred->get_precond();
      auto pre_all_with_post_apply_cond = pre_all;

      //// XXX why is this required if we are already adding constraints for to_state?
      //aliasing_constraints_t pred_alias_cons = alias_cons;
      //pred_alias_cons.aliasing_constraints_union(generate_aliasing_constraints_from_expr(post_apply_cond, precond_t));
      //pred_alias_cons.aliasing_constraints_union(generate_aliasing_constraints_from_predicate(post_apply_pred, this->get_src_tfg()));

      expr_ref post_lhs_apply = post_apply_pred->get_lhs_expr();
      expr_ref post_rhs_apply = post_apply_pred->get_rhs_expr();

      this->pred_lhs_rhs_convert_memmask_to_select_for_symbols_and_locals(ctx, post, post_lhs_apply, post_rhs_apply);
      set<local_id_t> src_locals = identify_local_variables_in_memmask_expr(post->get_rhs_expr());

      T_PRED_REF edge_dst_cond = T_PRED::mk_predicate_ref(precond_t(), post_apply_cond, expr_true(ctx), "edge-dst-cond");
      // add the dst edge cond for this path
      pre_all_with_post_apply_cond.insert(T_PRED::mk_predicate_ref(precond_t(), post_apply_cond, expr_true(ctx), "edge-dst-cond"));

      stringstream ss;
      ss << __func__ << "." << post->get_comment() << ".from_pc" << from_pc.to_string() << ".cond_apply_set_elem" << post_apply_set_elem_index << "_of_" << post_apply_set_size;
      query_comment qc(ss.str());

      local_sprel_expr_guesses_t local_sprel_expr_assumes_required = local_sprel_expr_assumes;
      PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
      if (this->prove_after_enumerating_local_sprel_expr_guesses<tfg>(pre_all_with_post_apply_cond, post_apply_precond/*, post_apply_precond.precond_get_guard()*/, local_sprel_expr_assumes_required, /*pred_*/alias_cons, post_lhs_apply, post_rhs_apply, qc, false, src_locals, from_pc, ret_counter_example)) {
        DYN_DEBUG2(decide_hoare_triple, cout << "proof on " << (pth == nullptr ? "(null)" : pth->graph_edge_composition_to_string()) << ": " << post->to_string(true) << ": succeeded\n");
        if (local_sprel_expr_assumes_required.size() > local_sprel_expr_assumes.size()) {
          NOT_IMPLEMENTED();
          //post.add_local_sprel_expr_assumes_as_guards(local_sprel_expr_assumes_required);
          //guess_made_weaker = true;
        }
      } else {
        DYN_DEBUG2(decide_hoare_triple, cout << "proof on " << (pth == nullptr ? "(null)" : pth->graph_edge_composition_to_string()) << ": " << post->to_string(true) << ": failed\n");
        PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
        guess_made_weaker = true;
        return proof_status_disproven;
      }
      PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
      post_apply_set_elem_index++;
    }
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return proof_status_proven;
  }

private:

  bool
  proof_query_is_trivial(unordered_set<shared_ptr<T_PRED const>> const &lhs_set, expr_ref const &src, expr_ref const &dst, bool &proof_result, T_PC const& from_pc, list<counter_example_t> &counter_example) const
  {
    autostop_timer ft(__func__);
    context *ctx = src->get_context();
    for (const auto &lhs_pred : lhs_set) {
      precond_t const &precond = lhs_pred->get_precond();
      expr_ref const &pred_src_expr = lhs_pred->get_lhs_expr();
      expr_ref const &pred_dst_expr = lhs_pred->get_rhs_expr();
      bool precond_trivial = precond.precond_is_trivial();
      //cout << __func__ << " " << __LINE__ << ": lhs_pred = " << lhs_pred->to_string(true) << endl;
      //cout << __func__ << " " << __LINE__ << ": src = " << expr_string(src) << endl;
      //cout << __func__ << " " << __LINE__ << ": dst = " << expr_string(dst) << endl;
      //cout << __func__ << " " << __LINE__ << ": precond_trivial = " << precond_trivial << endl;
      if (   precond_trivial
          && pred_src_expr == src
          && pred_dst_expr == dst) {
        proof_result = true;
        return true;
      }
      if (   src->get_operation_kind() == expr::OP_SELECT
          && dst->get_operation_kind() == expr::OP_SELECT
          && pred_src_expr == src->get_args().at(0)
          && pred_dst_expr == dst->get_args().at(0)) {
        //if memories are equal, selects on memories are guaranteed to be equal
        proof_result = true;
        return true;
      }
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

  virtual bool falsify_fcall_proof_query(expr_ref const& src, expr_ref const& dst, T_PC const& from_pc, counter_example_t& counter_example) const
  {
    // implemented in CG
    return false;
  }

  static bool proof_query_falsified_by_CE(unordered_set<shared_ptr<T_PRED const>> const &lhs_set, expr_ref const &src, expr_ref const &dst, set<memlabel_ref> const& relevant_memlabels, counter_example_t &counter_example)
  {
    return false; //NOT_IMPLEMENTED()
    size_t MAX_ITER = 2;
    context *ctx = src->get_context();
    T_PRED_REF post = T_PRED::mk_predicate_ref(precond_t(), src, dst, "falsify-try-post");
    for (size_t iter  = 0; iter < MAX_ITER; iter++) {
      if (adjust_CE_to_falsify_proof_query(lhs_set, post, relevant_memlabels, counter_example)) {
        return true;
      }
    }
    return false;
  }

  static bool adjust_CE_to_falsify_proof_query(unordered_set<shared_ptr<T_PRED const>> const &lhs_set, shared_ptr<T_PRED const> const &post, set<memlabel_ref> const& relevant_memlabels, counter_example_t &counter_example)
  {
    for (const auto &lhs_pred : lhs_set) {
      if (!lhs_pred->predicate_try_adjust_CE_to_prove_pred(counter_example)) {
        return false;
      }
    }
    if (!post->predicate_try_adjust_CE_to_disprove_pred(counter_example)) {
      return false;
    }

    return CE_falsifies_proof_query(lhs_set, post, relevant_memlabels, counter_example);
  }

  static bool CE_falsifies_proof_query(unordered_set<shared_ptr<T_PRED const>> const &lhs_set, shared_ptr<T_PRED const> const &post, set<memlabel_ref> const& relevant_memlabels, counter_example_t &counter_example)
  {
    for (const auto &lhs_pred : lhs_set) {
      bool eval_result;
      if (!lhs_pred->predicate_eval_on_counter_example(relevant_memlabels, eval_result, counter_example)) {
        CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": lhs predicate evaluation failed on counter_example. pred = " << lhs_pred->to_string(true) << ", counter_example =\n" << counter_example.counter_example_to_string() << endl);
        return false;
      }
      if (!eval_result) {
        CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": lhs predicate evaluation returned false on counter_example. pred = " << lhs_pred->to_string(true) << ", counter_example =\n" << counter_example.counter_example_to_string() << endl);
        return false;
      }
    }
    bool eval_result;
    if (!post->predicate_eval_on_counter_example(relevant_memlabels, eval_result, counter_example)) {
      CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": post predicate evaluation failed on counter_example. pred = " << post->to_string(true) << ", counter_example =\n" << counter_example.counter_example_to_string() << endl);
      return false;
    }
    if (eval_result) {
      CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": post predicate evaluation returned false on counter_example. pred = " << post->to_string(true) << ", counter_example =\n" << counter_example.counter_example_to_string() << endl);
      return false;
    }
    return true;
  }

  void
  pred_lhs_rhs_convert_memmask_to_select_for_symbols_and_locals(context *ctx, shared_ptr<T_PRED const> const &pred, expr_ref &lhs, expr_ref &rhs) const
  {  
    autostop_timer func_timer(__func__);
    consts_struct_t const &cs = ctx->get_consts_struct();

    /* memmask(local/symbol) => select(local/symbol) */
    if (   pred->get_lhs_expr()->get_operation_kind() == expr::OP_MEMMASK
        && pred->get_rhs_expr()->get_operation_kind() == expr::OP_MEMMASK
        && pred->get_lhs_expr()->get_args().at(1) == pred->get_rhs_expr()->get_args().at(1)) {
      memlabel_t ml = pred->get_lhs_expr()->get_args().at(1)->get_memlabel_value();
      tuple<expr_ref, size_t, memlabel_t> variable_size_memlabel_tuple;
      if (ml.memlabel_represents_symbol()) {
        symbol_id_t symbol_id = ml.memlabel_get_symbol_id();
        graph_symbol_map_t const& symbol_map = this->get_symbol_map();
        ASSERT(symbol_map.count(symbol_id));
        graph_symbol_t const& sym = symbol_map.at(symbol_id);
        size_t size = sym.get_size();
        if (size > 0 && size <= DWORD_LEN / BYTE_LEN) {
          expr_ref variable_addr = cs.get_expr_value(reg_type_symbol, symbol_id);
          //expr_ref variable_addr = get<0>(variable_size_memlabel_tuple);
          //memlabel_t ml = get<2>(variable_size_memlabel_tuple);
          lhs = ctx->mk_select(lhs, ml, variable_addr, size, false);
          rhs = ctx->mk_select(rhs, ml, variable_addr, size, false);
        }
      }
    }
  }
  static set<local_id_t>
  identify_local_variables_in_memmask_expr(expr_ref const &e)
  {
    autostop_timer func_timer(__func__);
    set<local_id_t> src_locals;
    if (e->get_operation_kind() == expr::OP_MEMMASK) {
      memlabel_t const &ml = e->get_args().at(1)->get_memlabel_value();
      if (memlabel_t::memlabel_contains_local(&ml)) {
        ASSERT(ml.get_type() == memlabel_t::MEMLABEL_MEM);
        ASSERT(ml.get_alias_set_size() == 1);
        alias_region_t const &as_elem = *ml.get_alias_set().begin();
        ASSERT(as_elem.alias_region_get_alias_type() == alias_region_t::alias_type_local);
        local_id_t local_id = as_elem.alias_region_get_var_id();
        src_locals.insert(local_id);
      }
    }
    return src_locals;
  }

  virtual set<local_sprel_expr_guesses_t> generate_local_sprel_expr_guesses(T_PC const &pp, unordered_set<T_PRED_REF> const &lhs, edge_guard_t const &guard, local_sprel_expr_guesses_t const &local_sprel_expr_assumes_required_to_prove, expr_ref src, expr_ref dst, set<local_id_t> const &src_locals) const = 0;

  virtual tfg_suffixpath_t get_src_suffixpath_ending_at_pc(T_PC const &p) const = 0;
  virtual pred_avail_exprs_t const &get_src_pred_avail_exprs_at_pc(T_PC const &p) const = 0;

  template<typename T_PRECOND_TFG>
  bool prove_after_enumerating_local_sprel_expr_guesses(unordered_set<shared_ptr<T_PRED const>> const &lhs_in, precond_t const& precond/*edge_guard_t const &guard*/, local_sprel_expr_guesses_t &local_sprel_expr_assumes_required_to_prove/*, sprel_map_pair_t const &sprel_map_pair, rodata_map_t const &rodata_map*/, aliasing_constraints_t const& alias_cons, expr_ref src, expr_ref dst, query_comment const &qc, bool timeout_res, set<local_id_t> const &src_locals, T_PC const &pp, list<counter_example_t> &counter_example) const;
};

}
