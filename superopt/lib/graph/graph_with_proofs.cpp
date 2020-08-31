#include "graph/graph_with_proofs.h"
//#include "graph/tfg.h"
#include "support/globals.h"
#include "support/dyn_debug.h"
#include "expr/context.h"
#include "expr/context_cache.h"
//#include "cg/corr_graph.h"
#include "gsupport/tfg_edge.h"
#include "gsupport/corr_graph_edge.h"

namespace eqspace {
template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
template<typename T_PRECOND_TFG>
bool
graph_with_proofs<T_PC, T_N, T_E, T_PRED>::prove_using_local_sprel_expr_guesses(context *ctx, T_PRED_SET const &lhs_set, precond_t const& precond/* edge_guard_t const &guard*/, local_sprel_expr_guesses_t &local_sprel_expr_assumes_required_to_prove, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, /*rodata_map_t const &rodata_map,*/ graph_memlabel_map_t const &memlabel_map, set<local_sprel_expr_guesses_t> const &all_guesses, expr_ref src, expr_ref dst, query_comment const &qc, bool timeout_res/*, set<cs_addr_ref_id_t> const &relevant_addr_refs, vector<memlabel_ref> const& relevant_memlabels*/, memlabel_assertions_t const& mlasserts, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg, T_PC const &pp, list<counter_example_t> &counter_example)
{
  autostop_timer func_timer(__func__);
  consts_struct_t const &cs = ctx->get_consts_struct();
  graph_symbol_map_t const &symbol_map = src_tfg.get_symbol_map();
  graph_locals_map_t const &locals_map = src_tfg.get_locals_map();
  //cout << __func__ << " " << __LINE__ << ": sprel_map_pair = " << sprel_map_pair.to_string_for_eq() << endl;
  //memlabel_assertions_t const& mlasserts = this->get_memlabel_assertions();

  precond_t this_precond = precond;
  this_precond.set_local_sprel_expr_assumes(local_sprel_expr_assumes_required_to_prove);
  //precond_t precond(guard, local_sprel_expr_assumes_required_to_prove);
  DYN_DEBUG(houdini, cout << __func__ << " " << __LINE__ << ": local_sprel_expr_assumes_required_to_prove = " << local_sprel_expr_assumes_required_to_prove.to_string() << endl);
  if (prove(ctx, lhs_set, this_precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, /*rodata_map,*/ memlabel_map, src, dst, qc, timeout_res, symbol_map, locals_map, mlasserts/*relevant_addr_refs, relevant_memlabels*/, alias_cons, src_tfg, cs, counter_example)) {
    DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": returning true because prove() returned true\n");
    return true;
  }

  autostop_timer func_timer_enumerate_lsprels(string(__func__) + ".enumerate_lsprels");
  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": prove() returned false\n");
  if (!g_correl_locals_flag) {
    DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": returning false because g_correl_locals_flag is false\n");
    return false;
  }

  size_t guess_num = 0;
  for (auto g : all_guesses) { //iterates in increasing order of size of the guesses to get the weakest precondition
    DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": trying to prove with guesses" << guess_num << ": " << g.to_string() << endl);
    //unordered_set<predicate> lhs_primed = lhs_set;
    //preds_prime_using_local_sprel_expr_guesses(lhs_primed, g, locals_map);
    //unordered_set<predicate> g_exprs = local_sprel_expr_guesses_get_pred_set(g, src_tfg);
    //unordered_set_union(lhs_primed, g_exprs);
    stringstream ss;
    ss << qc.to_string() << ".lsguess" << guess_num;
    guess_num++;
    query_comment qcl(ss.str());
    precond_t this_precond = precond;
    this_precond.set_local_sprel_expr_assumes(g);
    if (prove(ctx, lhs_set, this_precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, /*rodata_map,*/ /*src_*/memlabel_map/*, dst_memlabel_map*/, src, dst, qcl, timeout_res, symbol_map, locals_map, mlasserts/*relevant_addr_refs, relevant_memlabels*/, alias_cons, src_tfg, cs, counter_example)) {
      DYN_DEBUG(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": proof succeeded with guesses: " << g.to_string() << endl);
      local_sprel_expr_assumes_required_to_prove = g;
      DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": local_sprel_expr_assumes_required_to_prove: " << local_sprel_expr_assumes_required_to_prove.to_string_for_eq() << endl);
      return true;
    }
  }

  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": returning false\n");
  return false;
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
template<typename T_PRECOND_TFG>
bool
graph_with_proofs<T_PC, T_N, T_E, T_PRED>::prove_after_enumerating_local_sprel_expr_guesses(unordered_set<shared_ptr<T_PRED const>> const &lhs_in, precond_t const& precond/*edge_guard_t const &guard*/, local_sprel_expr_guesses_t &local_sprel_expr_assumes_required_to_prove, aliasing_constraints_t const& alias_cons, expr_ref src, expr_ref dst, query_comment const &qc, bool timeout_res, set<local_id_t> const &src_locals, T_PC const &pp, list<counter_example_t> &counter_example) const
{
  autostop_timer func_timer(__func__);

  bool proof_result;
  //XXX: generate new CEs make trivially false proof queries. Using comment right now to segregate
  if(qc.to_string() != "generate_new_CE" && this->proof_query_is_trivial(lhs_in, src, dst, proof_result, pp, counter_example)) {
    DYN_DEBUG(decide_hoare_triple, cout << __func__ << ':' << __LINE__ << ": proof query resolved trivially; result = " << proof_result << endl);
    return proof_result;
  }

  autostop_timer func2_timer(string(__func__) + ".proof_query_not_trivial");
  context *ctx = this->get_context();
  T_PRECOND_TFG const &src_tfg = this->get_src_tfg();
  //set<cs_addr_ref_id_t> const &relevant_addr_refs = this->graph_get_relevant_addr_refs();
  //this->graph_get_relevant_addr_refs(relevant_addr_refs, cs);
  graph_memlabel_map_t const &memlabel_map = this->get_memlabel_map();
  tfg_suffixpath_t const &src_suffixpath = this->get_src_suffixpath_ending_at_pc(pp);
  //pred_avail_exprs_t const &src_pred_avail_exprs = m_eq->get_src_pred_avail_exprs_at_pc(pp.get_first());
  pred_avail_exprs_t const &src_pred_avail_exprs = this->get_src_pred_avail_exprs_at_pc(pp);
  sprel_map_pair_t const &sprel_map_pair = this->get_sprel_map_pair(pp);
  //rodata_map_t const &rodata_map = this->get_rodata_map();
  //vector<memlabel_ref> relevant_memlabels;
  //this->graph_get_relevant_memlabels_except_args(relevant_memlabels);
  T_PRED_SET const& lhs_set = lhs_in;

  DYN_DEBUG3(houdini,
    cout << __func__ << " " << __LINE__ << ": after erasing unnecessary elements: src = " << expr_string(src) << endl;
    cout << __func__ << " " << __LINE__ << ": after erasing unnecessary elements: dst = " << expr_string(dst) << endl;
  );

  set<local_sprel_expr_guesses_t> all_guesses;

  if (ctx->get_config().enable_local_sprel_guesses) {
    all_guesses = this->generate_local_sprel_expr_guesses(pp, lhs_set, precond.precond_get_guard(), local_sprel_expr_assumes_required_to_prove, src, dst, src_locals);
  }
  memlabel_assertions_t const& mlasserts = this->get_memlabel_assertions();
  DYN_DEBUG(smt_query,
    stringstream ss;
    string pid = int_to_string(getpid());
    ss << g_query_dir << "/" << PROVE_USING_LOCAL_SPREL_EXPR_GUESSES_FILENAME_PREFIX << "." << qc.to_string();
    string filename = ss.str();
    ofstream fo(filename.data());
    ASSERT(fo);
    output_lhs_set_guard_lsprels_and_src_dst_to_file(fo, lhs_set, precond/*guard*/, local_sprel_expr_assumes_required_to_prove, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, /*rodata_map,*/ alias_cons, memlabel_map/*, dst_memlabel_map*/, all_guesses, src, dst, mlasserts/*relevant_addr_refs*/, src_tfg);
    fo.close();
  	//g_query_files.insert(filename);
        cout << "prove_using_local_sprel_expr_guesses filename " << filename << ' ';
  );
  //expr_num = (expr_num + 1) % 2; //don't allow more than 500 files per process

  mytimer timer;
  timer.start();
  bool ret = prove_using_local_sprel_expr_guesses(ctx, lhs_set, precond/*guard*/, local_sprel_expr_assumes_required_to_prove, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, /*rodata_map,*/ memlabel_map, all_guesses, src, dst, qc, timeout_res/*, relevant_addr_refs, relevant_memlabels*/, mlasserts, alias_cons, src_tfg, pp, counter_example);
  timer.stop();

  DYN_DEBUG(smt_query, cout << " returned " << (ret ? "true" : "false") << endl);

  CPP_DBG_EXEC2(EQCHECK, cout << "prove_using_local_sprel_expr_guesses took " << timer.get_elapsed() / 1e6 << " s\n");
  return ret;
}

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
void
graph_with_proofs<T_PC, T_N, T_E, T_PRED>::graph_to_stream(ostream& os) const
{
  this->graph_with_paths<T_PC, T_N, T_E, T_PRED>::graph_to_stream(os);
  m_memlabel_assertions.memlabel_assertions_to_stream(os);
}

template class graph_with_proofs<pcpair, corr_graph_node, corr_graph_edge, predicate>;
template class graph_with_proofs<pc, tfg_node, tfg_edge, predicate>;

}
