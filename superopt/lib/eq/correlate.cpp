#include "i386/regs.h"
#include "ppc/regs.h"
#include "expr/state.h"
#include "correlate.h"
#include "expr/solver_interface.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_inductive_preds.h"
#include "support/log.h"
#include "support/dyn_debug.h"
#include "eq/correl_entry.h"

#define DEBUG_TYPE "correlate"
namespace eqspace {

static string
path_to_string(list<shared_ptr<tfg_edge const>> const &path)
{
  stringstream ss;
  for (auto e : path) {
    ss << e->to_string() << "-";
  }
  return ss.str();
}

correl_entry_t *
correlate::choose_most_promising_correlation_entry(list<backtracker_node_t *> const &frontier) const
{
  autostop_timer func_timer("removeMostPromising");
  DYN_DEBUG(oopsla_log, cout << "Inside function -- removeMostPromising()..\n ");
  ASSERT(frontier.size());
  // From the correl entries with least rank, first choose the most promising dst_ec
  shared_ptr<tfg_edge_composition_t> chosen_dst_ec = choose_most_promising_dst_ec(frontier);
  list<backtracker_node_t *> chosen_src_corrs = get_correl_entries_with_chosen_dst_ec(frontier, chosen_dst_ec);
  list<backtracker_node_t *> chosen_corrs = get_correl_entries_with_least_rank(chosen_src_corrs);
  
  // From the correl entries with chosen dst_ec, choose the entry with most promising src_ec
  DYN_DEBUG(oopsla_log, cout << "Choosing correlation based on RANK in C, A and staticHeuristic -- ");
  list<backtracker_node_t *>::const_iterator max_weight_iter = chosen_corrs.end();
  for (auto iter = chosen_corrs.begin(); iter != chosen_corrs.end(); iter++) {
    if (   max_weight_iter == chosen_corrs.end()
        || correl_entry_less(*max_weight_iter, *iter)) {
      max_weight_iter = iter;
    }
  }
  ASSERT(max_weight_iter != chosen_corrs.end());
  correl_entry_t *ret = dynamic_cast<correl_entry_t *>(*max_weight_iter);
  ASSERT(ret);

  return ret;
}

list<backtracker_node_t *>
correlate::get_correl_entries_with_least_rank(list<backtracker_node_t *> const &frontier) const
{
  ASSERT(frontier.size());
  context* ctx = m_eq->get_context();
  list<backtracker_node_t *> ret;
  int least_rank = -1;
  if(ctx->get_config().disable_dst_bv_rank)
    return frontier;
  DYN_DEBUG2(oopsla_log, cout << "Inside function -- comparePromiseForProductCFGs \n" << endl);
  DYN_DEBUG2(oopsla_log, cout << "Chosen Product-CFGs with least RANK in A: \n" << endl);
  int i = 1;
  for(auto const &f : frontier) 
  {
    correl_entry_t *f_ce = dynamic_cast<correl_entry_t *>(f);
    ASSERT(f_ce);
    if(least_rank == -1 || (f_ce->get_bv_rank().get_dst_rank() < least_rank))
    {
      ret.clear();
      ret.push_back(f);
      least_rank = f_ce->get_bv_rank().get_dst_rank();
    }
    else if(least_rank == f_ce->get_bv_rank().get_dst_rank())
    {
      ret.push_back(f);
      DYN_DEBUG2(oopsla_log, cout << "Product-CFG " << i++ << ": \n" << f_ce->correl_entry_to_string("  ") << endl);
    }
  }
  ASSERT(ret.size() > 0);
  return ret;
}

void 
correl_entry_t::set_bv_rank(bv_rank_val_t bv_rank) { 
  m_bv_rank = bv_rank;
}

shared_ptr<corr_graph const>
correlate::find_correlation(local_sprel_expr_guesses_t const& lsprel_assumes, string const &cg_filename)
{
  autostop_timer func_timer("bestFirstSearch");
  DYN_DEBUG(oopsla_log, cout << "Inside top level function -- bestFirstSearch() \n\n");
  stats::get().set_value("eq-state", "find_correlation");
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg const &dst_tfg = m_eq->get_dst_tfg();
  string const& function_name = m_eq->get_function_name();
  consts_struct_t const &cs = src_tfg.get_consts_struct();
  DYN_DEBUG(dot_files_gen, 
    dst_tfg.to_dot("dst-tfg-structure.dot", false); 
    src_tfg.to_dot("src-tfg-structure.dot", false);
  );
  //dst_tfg.to_dot("dst-tfg.dot", true);
  //src_tfg.to_dot("src-tfg.dot", true);

  context* ctx = m_eq->get_context();

  shared_ptr<corr_graph> cg;

  if (cg_filename != "") {
    ifstream in(cg_filename);
    if(!in.is_open()) {
      cout << __func__ << " " << __LINE__ << ": could not open cg_filename " << cg_filename << "." << endl;
      exit(1);
    }
    if (!eqcheck::istream_position_at_function_name(in, function_name)) {
      cout << __func__ << " " << __LINE__ << ": could not find function '" << function_name << "' in cg_filename " << cg_filename << "." << endl;
      exit(1);
    }
    string line;
    bool end;
    end = !getline(in, line);
    ASSERT(!end);
    ASSERT(string_has_prefix(line, PROOF_FILE_RESULT_PREFIX));
    shared_ptr<corr_graph> cg = corr_graph::corr_graph_from_stream(in, ctx);
    ASSERT(cg);
    if (cg) {
      cout << "Successfully read CG from file: " << cg_filename << endl;
    }
  }

  if (!cg) {
    pcpair pp_entry = pcpair(src_tfg.find_entry_node()->get_pc(), dst_tfg.find_entry_node()->get_pc());
    shared_ptr<corr_graph_node> start_node = make_shared<corr_graph_node>(pp_entry);
    cg = make_shared<corr_graph>(mk_string_ref("cg"), m_eq);
    cg->cg_add_node(start_node);
    cg->graph_with_guessing_add_node(start_node->get_pc());
    //cg->graph_init_invariant_state_for_pc(start_node->get_pc());
    ASSERT(cg->get_eq());

    cg->cg_add_precondition(/*relevant_memlabels*/);
    cg->update_src_suffixpaths_cg(nullptr);
    cg->populate_simplified_assert_map({ start_node->get_pc() });
    cg->add_local_sprel_expr_assumes(lsprel_assumes);
  }
  //cg->update_graph_assumes();

  correl_entry_t *correl_entry = new correl_entry_t(*this, cg);
  CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": initial correl_entry:\n" << correl_entry->correl_entry_to_string("  "));
  backtracker bt(correl_entry);

  auto chrono_start = std::chrono::system_clock::now();
  DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": started CG construction by initializing the backtracker search tree.\n");
  DYN_DEBUG(oopsla_log, cout << "Started Product-CFG construction by initializing the backtracker search tree -- ");
  DYN_DEBUG(oopsla_log, cout << "Output of function initProductCFG(): \n"<< correl_entry->correl_entry_to_string("  ") << endl);
  while (1) {
    auto chrono_now = std::chrono::system_clock::now();
    DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": exploring the next CG. timeout = " << m_eq->get_global_timeout() << endl);
    auto elapsed = chrono_now - chrono_start;
    if (std::chrono::seconds(m_eq->get_global_timeout()) < elapsed) {
      cout << __func__ << " " << __LINE__ << ": Equivalence check timed out after " << chrono_duration_to_string(elapsed) << " seconds.\n";
      return nullptr;
    }
    float mem_consumed_gb = get_rss_gb();
    if (m_eq->get_memory_threshold_gb() < mem_consumed_gb) {
      cout << __func__ << " " << __LINE__ << ": Equivalence check memory threshold hit after " << chrono_duration_to_string(elapsed) << " seconds.  Memory consumed: " << mem_consumed_gb << " GiB, memory threshold: " << m_eq->get_memory_threshold_gb() << " GiB" << endl;
      return nullptr;
    }

    list<backtracker_node_t *> frontier = bt.get_frontier();
    if (frontier.size() == 0) {
      DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": exhausted backtracking correlation search space. elapsed = " << chrono_duration_to_string(elapsed) << " seconds.\n");
      return nullptr;
    }
    DYN_DEBUG(oopsla_log, cout << "frontier.size() = " << frontier.size() << endl);
    correl_entry_t *correl_entry = choose_most_promising_correlation_entry(frontier);
    ++m_corr_counter;
    DYN_DEBUG(correlate, cout << _FNLN_ << ": frontier.size() = " << frontier.size() << endl);
    DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": " << timestamp() << ": chosen correlation (#" << m_corr_counter << "):\n" << correl_entry->correl_entry_to_string("  ") << endl);
    DYN_DEBUG(invariants_map, cout << __func__ << " " << __LINE__ << ": cg =\n" << correl_entry->get_cg()->graph_with_guessing_invariants_to_string(false) << endl);
    
    DYN_DEBUG(oopsla_log, cout << "Output of function removeMostPromising() :\n");
    DYN_DEBUG(oopsla_log, cout << timestamp() << ": chosen correlation (#" << m_corr_counter << "):\n" << correl_entry->correl_entry_to_string("  ") << endl);

//    ostringstream ss;
//    ss << "Chosen product-TFG #" << m_corr_counter << " which has " << correl_entry->get_cg()->get_all_pcs().size() << " nodes...";
//    MSG(ss.str().c_str());
    if (correl_entry->explore(bt) == backtracker_node_t::FOUND_SOLUTION/* && correl_entry->dst_edge_id_is_empty()*/) {
      elapsed = std::chrono::system_clock::now() - chrono_start;
      DYN_DEBUG(oopsla_log, cout << "Constructed Product-CFG proves Bisimulation! elapsed time= " << chrono_duration_to_string(elapsed) << " seconds.\n");
      return correl_entry->get_cg();
    }
  }
  NOT_REACHED();
}

bool
correlate::preds_map_contains_exit_pcpair(map<pcpair, unordered_set<predicate>> const &preds_map)
{
  for (const auto &pm : preds_map) {
    if (pm.first.is_exit()) {
      return true;
    }
  }
  return false;
}

paths_weight_t
correlate::paths_weight(shared_ptr<tfg_edge_composition_t> const &path)
{
  //return the length of the shortest path
  tuple<int,int,int,int,int> pth_stats = get_path_and_edge_counts_for_edge_composition(path);
  paths_weight_t ret = get<4>(pth_stats);
//  ASSERT(ret != 0); // ret can be 0 for epsilon paths
  return ret;
}

bool
correlate::dst_edge_exists_in_cg_outgoing_edges(corr_graph const &cg_in_out, shared_ptr<corr_graph_node> cg_node, shared_ptr<tfg_edge const> const &dst_edge)
{
  list<shared_ptr<corr_graph_edge const>> outgoing;
  cg_in_out.get_edges_outgoing(cg_node->get_pc(), outgoing);
  for (auto e : outgoing) {
    if (   e->get_from_pc().get_second() == dst_edge->get_from_pc()
        && e->get_to_pc().get_second() == dst_edge->get_to_pc()) {
      return true;
    }
  }
  return false;
}


//shared_ptr<tfg_edge_composition_t>
//correlate::find_implied_path(shared_ptr<corr_graph const> cg_parent, shared_ptr<corr_graph> cg, pcpair const &pp, shared_ptr<tfg_edge_composition_t> const &src_path_ec, shared_ptr<tfg_edge_composition_t> const &dst_ec, bool &implied_check_status/*, predicate &edge_cond_pred*/) const
//{
//  autostop_timer func_timer(__func__);
//  context *ctx = m_eq->get_context();
//  tfg const &src_tfg = cg->get_eq()->get_src_tfg();
//  tfg const &dst_tfg = cg->get_dst_tfg();
//  consts_struct_t const &cs = ctx->get_consts_struct();
//  shared_ptr<tfg_edge_composition_t> ret_src_path_ec = nullptr;
//  
//  mytimer mt;
//  DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": semantically_simplify_tfg_edge_composition" << endl; mt.start(););
//  shared_ptr<tfg_edge_composition_t> src_path_ec_simpl = src_tfg.semantically_simplify_tfg_edge_composition(src_path_ec);
//  DYN_DEBUG(correlate, mt.stop(); cout << __func__ << " " << __LINE__ << ": semantically_simplify_tfg_edge_composition in " << mt.get_elapsed_secs() << "s " << endl);
//  if(!src_path_ec_simpl || src_path_ec_simpl->is_epsilon())
//    {implied_check_status = false; return ret_src_path_ec;}
//  DYN_DEBUG(correlate, cout << _FNLN_ << ": src_path_ec_simpl = " << src_path_ec_simpl->graph_edge_composition_to_string() << endl);
//
//
//  mytimer mt1;
//  DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": start " << endl; mt1.start(););
//  expr_ref dst_edge_cond = dst_tfg.tfg_edge_composition_get_edge_cond(dst_ec, true);
//  expr_ref src_edge_cond = src_tfg.tfg_edge_composition_get_edge_cond(src_path_ec_simpl, true, true);
//  DYN_DEBUG2(correlate, cout << _FNLN_ << ": src_path_edge.get_cond() = " << expr_string(src_edge_cond) << ", dst_edge_cond = " << expr_string(dst_edge_cond) << endl);
//  
//  query_comment qc(__func__);
//
//  // aliasing constraints are constraints asserting well formedness of an ec, i.e. the edge condition of that ec can be evaluated
//  aliasing_constraints_t src_edge_alias_cond = src_tfg.collect_aliasing_constraints_for_path_edge_cond(pp.get_first(), src_path_ec_simpl);
//  unordered_set<predicate> src_edge_cond_alias_pred = src_edge_alias_cond.get_ismemlabel_preds();
//  aliasing_constraints_t dst_edge_alias_cond = dst_tfg.collect_aliasing_constraints_for_path_edge_cond(pp.get_second(), dst_ec);
//  unordered_set<predicate> dst_edge_cond_alias_pred = dst_edge_alias_cond.get_ismemlabel_preds();
//
////  expr_ref src_edge_cond = src_tfg.tfg_edge_composition_get_edge_cond(src_path_ec, true);
//  expr_ref dst_implies_src = expr_or(expr_not(dst_edge_cond), src_edge_cond);
//  predicate pred(precond_t(src_tfg), dst_implies_src, expr_true(ctx), __func__, predicate::provable);
//
//  unordered_set<predicate> lhs_preds = src_tfg.collect_assumes_around_path(pp.get_first(), src_path_ec_simpl);
//  predicate_set_t src_dst_assume_alias_preds = lhs_preds;
//  predicate_set_union(src_dst_assume_alias_preds, src_edge_cond_alias_pred);
//  predicate_set_union(src_dst_assume_alias_preds, dst_edge_cond_alias_pred);
//
//  bool guess_made_weaker;
//  mytimer mt2;
//  DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": start " << endl; mt2.start(););
//  proof_status_t proof_status = cg_parent->decide_hoare_triple(/*lhs_preds*/src_dst_assume_alias_preds, pp, mk_epsilon_ec<pcpair, corr_graph_node, corr_graph_edge>(), pred, guess_made_weaker);
//  DYN_DEBUG(correlate, mt2.stop(); cout << __func__ << " " << __LINE__ << ": find_implied_path query took " << mt2.get_elapsed_secs() << "s " << endl);
//  DYN_DEBUG(correlate, mt1.stop(); cout << __func__ << " " << __LINE__ << ": find_implied_path query  and collect took " << mt1.get_elapsed_secs() << "s " << endl);
//  if (proof_status == proof_status_timeout) {
//    NOT_IMPLEMENTED();
//  }
//  if (pred.get_proof_status() == predicate::provable) {
//    if (guess_made_weaker) {
//      cg->add_local_sprel_expr_assumes(pred.get_local_sprel_expr_assumes());
//    }
//    //CPP_DBG_EXEC2(CORRELATE, cout << __func__ << ':' << __LINE__ << ": returning nullptr bcoz src_ec is not feasible semantically" << endl);
//    ret_src_path_ec = src_path_ec_simpl;
//  }
//  else
//    implied_check_status = false;
//
//  return ret_src_path_ec;
//}

//shared_ptr<tfg_edge_composition_t>
//correlate::find_implying_paths(shared_ptr<corr_graph> cg, pcpair const &pp, list<shared_ptr<tfg_edge_composition_t>> const &src_paths, shared_ptr<tfg_edge_composition_t> const &dst_ec/*, predicate &edge_cond_pred*/) const
//{
//  autostop_timer func_timer(__func__);
//  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
//  context *ctx = m_eq->get_context();
//  tfg const &src_tfg = cg->get_eq()->get_src_tfg();
//  tfg const &dst_tfg = cg->get_dst_tfg();
//  consts_struct_t const &cs = ctx->get_consts_struct();
//  CPP_DBG_EXEC2(CORRELATE, cout << __func__<< " dst_ec " << dst_ec->graph_edge_composition_to_string() << " cond:  " << ctx->expr_to_string(dst_tfg.tfg_edge_composition_get_edge_cond(dst_ec, true)) << "\n");
//  CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " dst_ec: " << dst_ec->graph_edge_composition_to_string() << endl);
//  CPP_DBG_EXEC2(CORRELATE, cout << __func__ << ": src_paths:\n " << path_ec_list_to_string(src_paths) <<endl);
//
//  ASSERT(src_paths.size() > 0);
//
//  query_comment qc(string(__func__) + ".implication_check");
//  list<shared_ptr<tfg_edge_composition_t>> implying_paths;
//  local_sprel_expr_guesses_t local_sprel_expr_assumes_required = cg->get_local_sprel_expr_assumes();
//  expr_ref implying_paths_cond = expr_false(ctx);
//  expr_ref dst_edge_cond = dst_tfg.tfg_edge_composition_get_edge_cond(dst_ec, true);
//
//  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
//  for (const auto &src_path_ec : src_paths) {
//    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
//    edge_guard_t src_path_eg(src_path_ec);
//    predicate pred(precond_t(src_path_eg, ctx), expr_true(ctx), dst_edge_cond, string(__func__) + ".implication_check", predicate::provable);
//    unordered_set<predicate> lhs_preds = src_tfg.collect_assumes_around_path(pp.get_first(), src_path_ec);
//    bool guess_made_weaker;
//    // XXX: add aliasing constraints pred for src and dst edge conds
//    proof_status_t proof_status = cg->decide_hoare_triple(lhs_preds, pp, mk_epsilon_ec<pcpair, corr_graph_node, corr_graph_edge>(), pred, guess_made_weaker);
//    if (proof_status == proof_status_timeout) {
//      NOT_IMPLEMENTED();
//    }
//    CPP_DBG_EXEC(CORRELATE, cout << "correlate_edges " << __func__ << " " << __LINE__ << ": implication check: src_path_edge = " << src_path_ec->graph_edge_composition_to_string() /*src_path_edge.to_string() */<< ": src_path_edge_cond = " << src_path_eg.edge_guard_to_string()/* expr_string(src_path_edge.get_cond()) */<< ", dst_ec = " << dst_ec->graph_edge_composition_to_string() << ", dst_edge_cond = " << expr_string(dst_edge_cond) << ", pred.get_proof_status = " << predicate::status_to_string(pred.get_proof_status()) << endl);
//    if (pred.get_proof_status() == predicate::provable) {
//      implying_paths.push_back(src_path_ec);
//    }
//  }
//
//  CPP_DBG_EXEC(CORRELATE, cout << "correlate_edges " << __func__ << " " << __LINE__ << ": dst_ec = " << dst_ec->graph_edge_composition_to_string() << ", implying_paths.size() = " << implying_paths.size() << ", implying_paths_cond = " << expr_string(implying_paths_cond) << ", dst_edge_cond = " << expr_string(dst_edge_cond) << endl);
//  stats::get().inc_counter("nr-equality-check");
//
//  CPP_DBG_EXEC(GENERAL, cout << __func__ << " dst_ec: " << dst_ec->graph_edge_composition_to_string() << endl);
//  CPP_DBG_EXEC(GENERAL, cout << __func__ << ": implying_src_paths:\n " << path_ec_list_to_string(implying_paths) <<endl);
//
//  shared_ptr<tfg_edge_composition_t> src_paths_ec;
//  shared_ptr<edge_guard_t> src_paths_eg;
//  for (auto const& pth_ec : implying_paths) {
//    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
//    if (!src_paths_ec) {
//      src_paths_ec = pth_ec;
//      src_paths_eg = make_shared<edge_guard_t>(pth_ec);
//    } else {
//      src_paths_ec = mk_parallel(src_paths_ec, pth_ec);
//      edge_guard_t eg(pth_ec);
//      src_paths_eg->add_guard_in_parallel(eg);
//    }
//  }
//  if (implying_paths.size() > 0) {
//    expr_ref src_paths_expr = src_paths_eg->edge_guard_get_expr(src_tfg, true);
//    predicate pred(precond_t(src_tfg), src_paths_expr, dst_edge_cond, string(__func__) + ".equality_check", predicate::provable);
//    unordered_set<predicate> lhs_preds = src_tfg.collect_assumes_around_path(pp.get_first(), src_paths_ec);
//    bool guess_made_weaker;
//    // XXX: add aliasing constraints pred for src and dst edge conds
//    proof_status_t proof_status = cg->decide_hoare_triple(lhs_preds, pp, mk_epsilon_ec<pcpair, corr_graph_node, corr_graph_edge>(), pred, guess_made_weaker);
//    if (proof_status == proof_status_timeout) {
//      NOT_IMPLEMENTED();
//    }
//    CPP_DBG_EXEC(CORRELATE, cout << "correlate_edges " << __func__ << " " << __LINE__ << ": equality check: src_paths_edge_guard = " << src_paths_eg->edge_guard_to_string() << ", src_paths_eg_expr = " << expr_string(src_paths_expr) << ", dst_ec = " << dst_ec->graph_edge_composition_to_string() << ", dst_edge_cond = " << expr_string(dst_edge_cond) << ", pred.get_proof_status = " << predicate::status_to_string(pred.get_proof_status()) << endl);
//    if (pred.get_proof_status() == predicate::provable) {
//      if (guess_made_weaker) {
//        cg->add_local_sprel_expr_assumes(pred.get_local_sprel_expr_assumes());
//      }
//      CPP_DBG_EXEC(CORRELATE, cout << "correlate_edges " << __func__ << " " << __LINE__ << ": src_path_to_be_correlated = " << src_paths_ec->graph_edge_composition_to_string() << endl);
//      return src_paths_ec;
//    }
//  }
//  CPP_DBG_EXEC(CORRELATE, cout << "correlate_edges " << __func__ << " " << __LINE__ << ": implying_edges not found. dst_edge->get_cond() = " << expr_string(dst_edge_cond) << endl);
//  CPP_DBG_EXEC(CORRELATE, cout << "correlate_edges " << __func__ << " " << __LINE__ << ": implying_paths.size() = " << implying_paths.size() << ", implying_paths_cond = " << (src_paths_eg ? src_paths_eg->edge_guard_to_string() : "null") << endl);
//  return nullptr;
//}

shared_ptr<corr_graph_edge const>
correlate::add_corr_graph_edge(shared_ptr<corr_graph> cg, pcpair const &from_pc, pcpair const &to_pc, shared_ptr<tfg_edge_composition_t> src_edge, shared_ptr<tfg_edge_composition_t> const &dst_ec/*, edge_guard_t const &g*/) const
{
  //ASSERT(cg->find_node(pcpair(src_edge->get_to_pc(), dst_edge->get_to_pc())));
  tfg const &src_tfg = cg->get_eq()->get_src_tfg();
  tfg const &dst_tfg = cg->get_dst_tfg();

  shared_ptr<corr_graph_node> n_from = cg->find_node(from_pc);
  shared_ptr<corr_graph_node> n_to = cg->find_node(to_pc);
  context *ctx = cg->get_context();
  unordered_set<expr_ref> assumes; // empty as actual assumes come from src and dst edges
  auto e = mk_corr_graph_edge(n_from, n_to, src_edge, dst_ec);
  cg->add_edge(e);

  // update suffixpaths in light of new edge
  cg->update_src_suffixpaths_cg(e);
  return e;
}

void
correlate::remove_corr_graph_edge(corr_graph& cg, shared_ptr<corr_graph_edge const> edge)
{
  pcpair to_pc = edge->get_to_pc();
  cout << __func__ << " " << __LINE__ << ": removing edge " << edge->to_string() << endl;
  cg.remove_edge(edge);
  list<shared_ptr<corr_graph_edge const>> incoming;
  cg.get_edges_incoming(to_pc, incoming);
  if (incoming.size() == 0) {
    cout << __func__ << " " << __LINE__ << ": removing node " << to_pc.to_string() << endl;
    cg.remove_node(to_pc);
  }
}

//TODO make a class for pc set or scc
bool is_pcs_in_same_scc(pc pc1, pc pc2, const list<scc<pc>>& sccs)
{
  for(list<scc<pc>>::const_iterator iter = sccs.begin(); iter != sccs.end(); ++iter)
  {
    scc<pc> s = *iter;
    bool pc1_in = s.is_member(pc1);
    bool pc2_in = s.is_member(pc2);
    if(pc1_in && pc2_in)
      return true;
    if((pc1_in && !pc2_in) || (!pc1_in && pc2_in))
      return false;
  }
  assert(false);
}

struct compare_sort_edges
{
  bool operator() (shared_ptr<tfg_edge const> first, shared_ptr<tfg_edge const> second) const
  {
    if(first->get_from_pc() == first->get_to_pc())
      return true;

    if(second->get_from_pc() == second->get_to_pc())
      return false;

    unsigned first_nr = get_order_nr(first->get_to_pc());
    unsigned second_nr = get_order_nr(second->get_to_pc());
    bool first_in_scc = is_pcs_in_same_scc(first->get_to_pc(), first->get_from_pc(), m_sccs);
    bool second_in_scc = is_pcs_in_same_scc(second->get_to_pc(), second->get_from_pc(), m_sccs);

    if((first_in_scc && second_in_scc) || (!first_in_scc && !second_in_scc))
      return first_nr < second_nr;

    if(first_in_scc)
      return true;

    if(second_in_scc)
      return false;

    return false;
  }

  unsigned get_order_nr(pc p) const
  {
    unsigned ret = 0;
    for(list<pc>::const_iterator iter = m_pc_order.begin(); iter != m_pc_order.end(); ++iter)
    {
      if(*iter == p)
        return ret;
      ret++;
    }
    assert(false);
  }

  list<scc<pc>> m_sccs;
  list<pc> m_pc_order;
};

void
correlate::get_dst_edges_in_dfs_and_scc_first_order_rec(tfg const &dst_tfg, pc dst_pc, set<tfg_edge_ref> &done, set<pc>& visited, list<tfg_edge_ref> &dst_edges)
{
  if(visited.count(dst_pc) > 0)
    return;
  visited.insert(dst_pc);
  //tfg& dst_tfg = m_eq->get_dst_tfg();
  list<shared_ptr<tfg_edge const>> dst_es;
  dst_tfg.get_edges_outgoing(dst_pc, dst_es);
  compare_sort_edges comp;
  dst_tfg.get_sccs(comp.m_sccs);
  comp.m_pc_order = dst_tfg.topological_sorted_labels();
  dst_es.sort(comp);
  for (list<shared_ptr<tfg_edge const>>::const_iterator iter = dst_es.begin(); iter != dst_es.end(); ++iter) {
    tfg_edge_ref const& dst_edge = *iter;
    if (done.count(dst_edge) > 0) {
      continue;
    }
    dst_edges.push_back(dst_edge);
    done.insert(dst_edge);
    get_dst_edges_in_dfs_and_scc_first_order_rec(dst_tfg, dst_edge->get_to_pc(), done, visited, dst_edges);
  }
}

// Get dst edges in depth first post order -- RPO is reverse of this order
// Due to presence of loops, need to calculate SCCs first
void
correlate::get_dst_edges_in_dfs_and_scc_first_order(tfg const &t, list<tfg_edge_ref> &dst_edges)
{
  dst_edges.clear();
  set<tfg_edge_ref> done;
  set<pc> visited;
  get_dst_edges_in_dfs_and_scc_first_order_rec(t, t.find_entry_node()->get_pc(), done, visited, dst_edges);
}



bool
correl_entry_ladder_t::dst_edge_appears_earlier(map<pc, int> const &ordered_dst_pcs, shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b)
{
  if(a==b)
    return false;

  bool a_is_back_edge  = false;
  bool b_is_back_edge = false;
  pc const &a_to_pc = a->graph_edge_composition_get_to_pc();
  pc const &b_to_pc = b->graph_edge_composition_get_to_pc();
  pc const &a_from_pc = a->graph_edge_composition_get_from_pc();
  pc const &b_from_pc = b->graph_edge_composition_get_from_pc();
  ASSERT(ordered_dst_pcs.count(a_to_pc));
  ASSERT(ordered_dst_pcs.count(b_to_pc));
  ASSERT(ordered_dst_pcs.count(a_from_pc));
  ASSERT(ordered_dst_pcs.count(b_from_pc));
  int a_to_index = ordered_dst_pcs.at(a_to_pc);
  int b_to_index = ordered_dst_pcs.at(b_to_pc);
  int a_from_index = ordered_dst_pcs.at(a_from_pc);
  int b_from_index = ordered_dst_pcs.at(b_from_pc);
  if(a_to_index <= a_from_index)
    a_is_back_edge = true;
  if(b_to_index <= b_from_index)
    b_is_back_edge = true;

  if((a_is_back_edge && b_is_back_edge) || (!a_is_back_edge && !b_is_back_edge))
  {
    if(a_to_index != b_to_index)
      return a_to_index < b_to_index;
    else if(a_from_index != b_from_index)
      return b_from_index < a_from_index;
  }
  else {
    if(a_is_back_edge)
      a_to_index = a_from_index;
    else if(b_is_back_edge)
      b_to_index = b_from_index;
    if(a_to_index != b_to_index)
      return a_to_index < b_to_index;
    else if(a_from_index != b_from_index)
      return b_from_index < a_from_index;
  }

  cout << __func__ << " " << __LINE__ << ": a edge = " << a->graph_edge_composition_to_string() << endl;
  cout << __func__ << " " << __LINE__ << ": b edge = " << b->graph_edge_composition_to_string() << endl;
  NOT_REACHED();
  //for (const auto &de : ordered_dst_edges) {
  //  if (de.get_from() == b_from_pc && de.get_to() == b_to_pc) {
  //    return false;
  //  }
  //  if (de.get_from() == a_from_pc && de.get_to() == a_to_pc) {
  //    return true;
  //  }
  //}
  //NOT_REACHED();
}

list<backtracker_node_t *>
correlate::get_correl_entries_with_chosen_dst_ec(list<backtracker_node_t *> const &frontier, shared_ptr<tfg_edge_composition_t> const &chosen_dst_ec) const
{
  ASSERT(frontier.size());
  list<backtracker_node_t *> ret;
  for(auto const &f : frontier)
  {
    correl_entry_t *f_ce = dynamic_cast<correl_entry_t *>(f);
    ASSERT(f_ce);
    if(f_ce->get_dst_ec() == chosen_dst_ec)
      ret.push_back(f);
  }
  ASSERT(ret.size() > 0);
  return ret;
}

shared_ptr<tfg_edge_composition_t>
correlate::choose_most_promising_dst_ec(list<backtracker_node_t *> const &frontier) const
{
  ASSERT(frontier.size());
  //cout << __func__ << " " << __LINE__ << ": m_potential_correlations.size() = " << m_potential_correlations.size() << endl;

  list<backtracker_node_t *>::const_iterator max_weight_iter = frontier.end();
  for (auto iter = frontier.begin(); iter != frontier.end(); iter++) {
    if (   max_weight_iter == frontier.end()
        || correl_entry_b_has_more_promising_dst_ec(*max_weight_iter, *iter)) {
      max_weight_iter = iter;
    }
  }
  ASSERT(max_weight_iter != frontier.end());
  correl_entry_t *ret_ce = dynamic_cast<correl_entry_t *>(*max_weight_iter);
  ASSERT(ret_ce);
  shared_ptr<tfg_edge_composition_t> ret_dst_ec = ret_ce->get_dst_ec();
  if(ret_dst_ec != nullptr)
  {
    CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": chosen dst_ec =\n" << ret_ce->get_dst_ec()->graph_edge_composition_to_string() << endl);
    DYN_DEBUG2(oopsla_log, cout << "Chosen pathset in A according to reverse-post-order =\n" << ret_ce->get_dst_ec()->graph_edge_composition_to_string() << endl);
  }
  return ret_dst_ec;
}

bool
correlate::correl_entry_b_has_more_promising_dst_ec(backtracker_node_t const *_a, backtracker_node_t const *_b) const
{
  correl_entry_t const &a = *dynamic_cast<correl_entry_t const *>(_a);
  correl_entry_t const &b = *dynamic_cast<correl_entry_t const *>(_b);
  CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": comparing correl_entry a:\n" << a.correl_entry_to_string("  ") << endl << "with correl_entry b:\n" << b.correl_entry_to_string("  ") << endl);

  shared_ptr<tfg_edge_composition_t> a_dst_edge = a.get_dst_ec();
  shared_ptr<tfg_edge_composition_t> b_dst_edge = b.get_dst_ec();
  shared_ptr<corr_graph const> a_cg = a.get_cg(); //get<0>(a);
  shared_ptr<corr_graph const> b_cg = b.get_cg(); //get<0>(b);
  pcpair const &a_pp = a.get_pp();
  pcpair const &b_pp = b.get_pp();

  // When unrolled factor > 1, multiple src_paths possible for a single dst_edge. If in a the dst_edge is already correlated but not in b yet, a should win over b. The function will return true for "b_dst_edge_already_correlated_in_a" and false for "a_dst_edge_already_correlated_in_b"
  bool a_dst_edge_already_correlated_in_b = b_cg->dst_edge_exists_in_outgoing_cg_edges(a_pp, a_dst_edge->graph_edge_composition_get_to_pc());
  bool b_dst_edge_already_correlated_in_a = a_cg->dst_edge_exists_in_outgoing_cg_edges(b_pp, b_dst_edge->graph_edge_composition_get_to_pc());

  if(a_dst_edge_already_correlated_in_b != b_dst_edge_already_correlated_in_a) {
  	bool ret = a_dst_edge_already_correlated_in_b;
    CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
    return ret;
  }
  // If dst_edges are not equal
  if (a_dst_edge != b_dst_edge) {
    // If a_dst_edge and b_dst_edge are unrolled version of paths between same from_pc and to_pc, choose the ec with smallest length to mantain incremtality
    if(   a_dst_edge->graph_edge_composition_get_from_pc() == b_dst_edge->graph_edge_composition_get_from_pc()
       && a_dst_edge->graph_edge_composition_get_to_pc() == b_dst_edge->graph_edge_composition_get_to_pc())
    {
      ASSERT(paths_weight({a_dst_edge}) != paths_weight({b_dst_edge}));
      bool ret = paths_weight({a_dst_edge}) > paths_weight({b_dst_edge});
      CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": returning " << ret << " because of weight of dst edges" << endl);
      return ret;
    }
    // If a_dst_edge and b_dst_edge are paths between different from_pc and to_pcs, choose the ec which appears later in RPO
    // bec if the entry which appears later in RPO exists then all the earlier edges are correlated in that CG.
    bool ret = correl_entry_ladder_t::dst_edge_appears_earlier(m_dst_pcs_ordered, a_dst_edge, b_dst_edge);
    CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": returning " << ret << " because dst_edge_appears_earlier" << endl);
    return ret;
  }
  ASSERT(a_dst_edge == b_dst_edge);
  return false;

}

bool
correlate::correl_entry_less(backtracker_node_t const *_a, backtracker_node_t const *_b) const
{
  correl_entry_t const &a = *dynamic_cast<correl_entry_t const *>(_a);
  correl_entry_t const &b = *dynamic_cast<correl_entry_t const *>(_b);
  CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": comparing correl_entry a:\n" << a.correl_entry_to_string("  ") << endl << "with correl_entry b:\n" << b.correl_entry_to_string("  ") << endl);
  context* ctx = m_eq->get_context();
  //comparison order:
  //  (1)  if src_pc_to is already correlated with dst_pc_to : WINS
  //  (2)  if unroll factor of src path a > b, then b wins
  //  (3)  if src_path contains already correlated node as intermediate node : LOOSES
  //  (4)  if some other src_pc (!= src_pc_to) is already correlated with dst_pc_to : LOOSES
  //  (5)  implying over implied : WINS
  //  (6)  For same unroll factor, src_path with shortest length WINS
  //  Returns true if b WINS, return false if a WINS
  shared_ptr<corr_graph const> a_cg = a.get_cg(); //get<0>(a);
  shared_ptr<corr_graph const> b_cg = b.get_cg(); //get<0>(b);
  pcpair const &a_pp = a.get_pp();
  pcpair const &b_pp = b.get_pp();
  //shared_ptr<tfg_edge_composition_t> a_dst_edge = a.get_dst_ec();
  //shared_ptr<tfg_edge_composition_t> b_dst_edge = b.get_dst_ec();
  shared_ptr<tfg_edge_composition_t> a_dst_edge = a.get_cg_edge()->get_dst_edge();
  shared_ptr<tfg_edge_composition_t> b_dst_edge = b.get_cg_edge()->get_dst_edge();
  //pc const &a_src_pc_to = a.get_src_pc(); //get<3>(a);
  //pc const &b_src_pc_to = b.get_src_pc(); //get<3>(b);
  //shared_ptr<tfg_edge_composition_t> const &a_src_path = a.get_candidate_src_path(); //get<4>(a);
  //shared_ptr<tfg_edge_composition_t> const &b_src_path = b.get_candidate_src_path(); //get<4>(b);
  shared_ptr<tfg_edge_composition_t> const &a_src_path = a.get_cg_edge()->get_src_edge(); //get<4>(a);
  shared_ptr<tfg_edge_composition_t> const &b_src_path = b.get_cg_edge()->get_src_edge(); //get<4>(b);
  pc a_src_pc_to = a_src_path->graph_edge_composition_get_to_pc();
  pc b_src_pc_to = b_src_path->graph_edge_composition_get_to_pc();
  int a_path_delta = a.get_path_delta(); //get<4>(a);
  int b_path_delta = b.get_path_delta(); //get<4>(b);
  int a_path_mu = a.get_path_mu(); //get<4>(a);
  int b_path_mu = b.get_path_mu(); //get<4>(b);
  bv_rank_val_t a_rank = a.get_bv_rank(); //get<4>(a);
  bv_rank_val_t b_rank = b.get_bv_rank(); //get<4>(b);
  //bool a_implying = a.get_implying(); //get<5>(a);
  //bool b_implying = b.get_implying(); //get<5>(b);

  ASSERT(a_dst_edge == b_dst_edge);
  bool a_to_pc_correlation_already_exists = a_cg->cg_correlation_exists(a.get_cg_edge());
  bool b_to_pc_correlation_already_exists = b_cg->cg_correlation_exists(b.get_cg_edge());
  if(!ctx->get_config().disable_all_static_heuristics)
  {
    if (a_to_pc_correlation_already_exists != b_to_pc_correlation_already_exists) {
      bool ret = b_to_pc_correlation_already_exists;
      CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
      return ret;
    }
  }
  
//  if(a_path_delta != b_path_delta) {
//    bool a_is_power_of_2 = (ceil(log2(a_path_delta)) == floor(log2(a_path_delta)));
//    bool b_is_power_of_2 = (ceil(log2(b_path_delta)) == floor(log2(b_path_delta)));
//
//    bool ret;
//    if(a_is_power_of_2 && b_is_power_of_2) 
//      ret = a_path_delta > b_path_delta;
//    else if(a_is_power_of_2 && !b_is_power_of_2) 
//      ret = false;
//    else if(!a_is_power_of_2 && b_is_power_of_2) 
//      ret = true;
//    else
//      ret = a_path_delta > b_path_delta;
//    CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << ": returning " << ret << " because of path unroll factor of src paths (in both implying and implied case)" << endl);
//    return ret;
//  }
  if(a_path_delta != b_path_delta) {
    bool ret;
    if(ctx->get_config().choose_longest_delta_first)
      ret = a_path_delta < b_path_delta;
    else
      ret = a_path_delta > b_path_delta;
    CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << ": returning " << ret << " because of path unroll factor of src paths (in both implying and implied case)" << endl);
    return ret;
  }
  ASSERT(a_path_delta == b_path_delta);

  if(!ctx->get_config().disable_all_static_heuristics)
  {
    bool a_src_path_contains_already_correlated_node_as_intermediate_node = a_cg->src_path_contains_already_correlated_node_as_intermediate_node(a_src_path, a_dst_edge->graph_edge_composition_get_to_pc());
    bool b_src_path_contains_already_correlated_node_as_intermediate_node = b_cg->src_path_contains_already_correlated_node_as_intermediate_node(b_src_path, b_dst_edge->graph_edge_composition_get_to_pc());
    if (a_src_path_contains_already_correlated_node_as_intermediate_node != b_src_path_contains_already_correlated_node_as_intermediate_node) {
      bool ret = a_src_path_contains_already_correlated_node_as_intermediate_node;
      CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
      return ret;
    }

    bool a_dst_pc_correlated_with_another_src_pc = a_cg->cg_correlation_exists_to_another_src_pc(a.get_cg_edge());
    bool b_dst_pc_correlated_with_another_src_pc = b_cg->cg_correlation_exists_to_another_src_pc(b.get_cg_edge());

    if (a_dst_pc_correlated_with_another_src_pc != b_dst_pc_correlated_with_another_src_pc) {
      bool ret = a_to_pc_correlation_already_exists;
      CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
      return ret;
    }
  }

  // if a has less number of src exprs that are uncorrelated than b : a WINS
  if(!ctx->get_config().disable_src_bv_rank)
  {
    if(a_rank.get_src_rank() != b_rank.get_src_rank())
    {
      bool ret = (a_rank.get_src_rank() > b_rank.get_src_rank());
      return ret;
    }
    ASSERT(a_rank.get_src_rank() == b_rank.get_src_rank());
  }
  // XXX: USe path path_mu along with path_weight 
  if(a_path_mu != b_path_mu)
  {
    bool ret;
    if(ctx->get_config().choose_shortest_path_first)
      ret = a_path_mu > b_path_mu;
    else
      ret = a_path_mu < b_path_mu;

    return ret;
  }
  ASSERT(a_path_mu == b_path_mu);
  bool ret;
  if(ctx->get_config().choose_shortest_path_first)
    ret = paths_weight(a_src_path) > paths_weight(b_src_path);
  else
    ret = paths_weight(a_src_path) < paths_weight(b_src_path);
  CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": returning " << ret << " because of weight of src paths (implying case)" << endl);
  return ret;
  
  NOT_REACHED();
}

//correlate::tfg_frontier_t::tfg_frontier_t(tfg const &t, correlate const &c) : m_correlate(c)
//{
//  //get_dst_edges_in_dfs_and_scc_first_order(t, m_dst_edges);
//  //cout << __func__ << " " << __LINE__ << ": m_dst_edges.size() = " << m_dst_edges.size() << endl;
//}

string
correl_entry_t::correl_entry_to_string(string prefix) const
{
  correl_entry_t const &e = *this;// ???
  shared_ptr<corr_graph const> cg = e.get_cg(); //get<0>(e);
  pcpair const &pp = e.get_pp(); //get<1>(e);
  shared_ptr<tfg_edge_composition_t> const &dst_ec = e.get_dst_ec(); //get<2>(e);
  //bool implying = e.get_implying(); //get<5>(e);
  stringstream ss;
  //ss << cg->cg_to_string(false, prefix) << endl;
  //ss << cg->cg_to_string_for_eq(false, false, prefix);
  cg->graph<pcpair, corr_graph_node, corr_graph_edge>::graph_to_stream(ss);
  if (dst_ec) {
    pc src_pc_to = e.get_cg_edge()->get_src_edge()->graph_edge_composition_get_to_pc();
    shared_ptr<tfg_edge_composition_t> const &path = e.get_cg_edge()->get_src_edge(); //get<4>(e);
    ss << prefix << "pcpair: " << pp.to_string() << endl;
    ss << prefix << "pathset in A: " << dst_ec->graph_edge_composition_to_string() << endl;
    ss << prefix << "To pc in C: " << src_pc_to.to_string() << endl;
    ss << prefix << "pathset delta: " << m_path_delta << endl;
    ss << prefix << "pathset mu: " << m_path_mu << endl;
    ss << prefix << "RANK: " << m_bv_rank.to_string();
    ss << prefix << "pathset in C: " << path->graph_edge_composition_to_string() << endl;
    //ss << prefix << "implying: " << implying << endl;
  }
  return ss.str();
}

string
correlate::path_to_string(list<shared_ptr<tfg_edge const>> const &path)
{
  stringstream ss;
  bool first = true;
  for (auto edge : path) {
    ss << (first ? "" : ".") << "(" << edge->to_string(/*NULL*/) << ")";
    first = false;
  }
  return ss.str();
}

string
correlate::path_list_to_string(list<list<shared_ptr<tfg_edge const>>> const &pathlist)
{
  stringstream ss;
  int num_paths = 1;
  for (auto path : pathlist) {
    ss << num_paths << ". " << path_to_string(path) << endl;
    num_paths++;
  }
  return ss.str();
}
string
correlate::path_ec_list_to_string(list<shared_ptr<tfg_edge_composition_t>> const &path_ec_list)
{
  stringstream ss;
  int num_paths = 1;
  for (auto path : path_ec_list) {
    ss << num_paths << ". " << path->graph_edge_composition_to_string() << endl;
    num_paths++;
  }
  return ss.str();
}

backtracker_node_t::backtracker_explore_retval_t
correl_entry_t::get_next_level_possibilities(list<backtracker_node_t *> &children, backtracker &bt)
{
  autostop_timer func_timer("expandProductCFG");
  DYN_DEBUG(oopsla_log, cout << "Inside function -- expandProductCFG()\n\n");
  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  children.clear();
  if (m_ladder.ladder_get_status() == correl_entry_ladder_t::LADDER_JUST_STARTED) {
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    backtracker_node_t::backtracker_explore_retval_t ret = this->correl_entry_apply(bt);
    if (ret == backtracker_node_t::ENUMERATED_CHILDREN) { //indicates that the application failed
      PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
      return backtracker_node_t::ENUMERATED_CHILDREN;
    }
    if (ret == backtracker_node_t::DELAYED_EXPLORATION) {
      PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
      return backtracker_node_t::DELAYED_EXPLORATION;
    }
    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  }
  m_ladder.ladder_update_status();
  ASSERT(this->dst_edge_id_is_empty());
  if (m_ladder.ladder_get_status() == correl_entry_ladder_t::LADDER_DONE) {
    return backtracker_node_t::ENUMERATED_CHILDREN;
  }
  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  shared_ptr<corr_graph const> const &cg = this->get_cg();
  context *ctx = cg->get_context();
  pcpair chosen_pcpair;
  list<shared_ptr<tfg_edge_composition_t>> unrolled_dst_pths;
  bool ret = m_ladder.get_next_dst_edge_composition_to_correlate(cg, m_correlate.get_ordered_dst_pcs(), unrolled_dst_pths, chosen_pcpair);
  if (!ret) {
    DYN_DEBUG(oopsla_log, cout << "No incomplete node found by findIncompleteNode() in the constructed partial product-CFG!" << endl);
    return backtracker_node_t::FOUND_SOLUTION;
  }
  ASSERT(unrolled_dst_pths.size() > 0);
  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  ASSERT(   m_ladder.ladder_get_status() == correl_entry_ladder_t::LADDER_ENUM_IMPLIED_CHILDREN/*
         || m_ladder.ladder_get_status() == correl_entry_ladder_t::LADDER_ENUM_IMPLYING_CHILDREN*/);
  for(auto const &ec : unrolled_dst_pths) {
    list<correl_entry_t> next = this->get_correlate().get_next_potential_correlations(cg, chosen_pcpair, ec, this->get_bv_rank()/*, m_ladder.ladder_get_status() == correl_entry_ladder_t::LADDER_ENUM_IMPLYING_CHILDREN*/);

    PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
    for (const auto &ce : next) {
      PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
      correl_entry_t *e = new correl_entry_t(ce);
      children.push_back(e);
    }
  }
  if (children.size() == 0) {
    ostringstream ss;
    ss << "product-TFG was not fruitful.\n";
    MSG(ss.str().c_str());
  } else {
    ostringstream ss;
    ss << "product-TFG created " << children.size() << " possibilities that may be explored further.\n";
    MSG(ss.str().c_str());
  }
  return backtracker_node_t::ENUMERATED_CHILDREN;
}

list<backtracker_node_t *>
correl_entry_t::find_siblings_with_same_dst_ec_src_to_pc_higher_delta() const
{
  list<backtracker_node_t *> ret_siblings;
  list<backtracker_node_t *> const& siblings = this->get_siblings();
  correl_entry_t const &tt = *this; 
  shared_ptr<corr_graph const> cg = tt.get_cg(); 
  ASSERT(cg);
  shared_ptr<tfg_edge_composition_t> const &dst_ec = tt.get_dst_ec();
  pc src_pc_to = tt.get_cg_edge()->get_src_edge()->graph_edge_composition_get_to_pc();
  int delta = tt.get_path_delta(); 

  for(auto const& s : siblings)
  {
    correl_entry_t *s_ce = dynamic_cast<correl_entry_t *>(s);
    ASSERT(s_ce);
    if (!s_ce->get_cg_edge() || !s_ce->get_cg_edge()->get_src_edge())
      continue;
    pc ce_src_pc = s_ce->get_cg_edge()->get_src_edge()->graph_edge_composition_get_to_pc();
    // Cannot assert that the CGs are equivalent, if one the siblings is correlated but with higher BV_RANK
    if (s_ce->get_dst_ec() == dst_ec && ce_src_pc == src_pc_to) {
      //Only one correl entry with one delta, so all siblings with same dst_ec and src_to_pc will have a different value of delta
      ASSERT(s_ce->get_path_delta() != delta);
      if(s_ce->get_path_delta() > delta)
        ret_siblings.push_back(s);
    }
  }
  return ret_siblings;
}

backtracker_node_t::backtracker_explore_retval_t
correl_entry_t::correl_entry_apply(backtracker &bt)
{
  //applies the desired correlation; returns true iff it is feasible
  ASSERT(!this->dst_edge_id_is_empty());
  correl_entry_t &tt = *this; //dst_frontier.choose_next_potential_correlation();
  shared_ptr<corr_graph> cg_new = tt.get_cg(); 
  ASSERT(cg_new);
  ASSERT(cg_new->get_eq());
  context *ctx = cg_new->get_context();
  pcpair const &pp = tt.get_pp();
  //shared_ptr<tfg_edge_composition_t> const &dst_ec = tt.get_dst_ec();
  shared_ptr<corr_graph_edge const> cg_edge = tt.get_cg_edge();
  pc src_pc_to = cg_edge->get_src_edge()->graph_edge_composition_get_to_pc();
  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  //shared_ptr<tfg_edge_composition_t> const &src_path_set = tt.get_candidate_src_path(); //get<4>(tt);
  //bool implying = tt.get_implying();
  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  //bool implied_check_status = true;
  //shared_ptr<corr_graph const> cg_new = m_correlate.correlate_dst_edge_with_src_path(cg, pp, dst_ec, src_path_set, src_pc_to/*, implying*/, implied_check_status);
  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  if (!cg_new->cg_check_new_cg_edge(cg_edge)) {
    return backtracker_node_t::ENUMERATED_CHILDREN;
  }
  bv_rank_val_t parent_bv_rank = bv_rank_val_t();
  if (this->get_parent_node()) {
    correl_entry_t const *parent_ce = dynamic_cast<correl_entry_t const*>(this->get_parent_node());
    ASSERT(parent_ce);
    parent_bv_rank = parent_ce->get_bv_rank();
  }
  bv_rank_val_t prev_bv_rank = this->get_bv_rank();
  bv_rank_val_t bv_rank = cg_new->calculate_rank_bveqclass_at_pc(cg_edge->get_to_pc());
  bv_rank_val_t new_bv_rank = bv_rank + parent_bv_rank;
  this->set_bv_rank(new_bv_rank);
//  unsigned dst_bv_rank = get<0>(bv_rank);
//  dst_bv_rank += get<0>(parent_bv_rank);
//  this->set_bv_rank(make_tuple(dst_bv_rank, get<1>(prev_bv_rank), get<2>(prev_bv_rank)));
  DYN_DEBUG2(correlate, cout << __func__ << "  " << __LINE__ << " NEW BV RANK " << new_bv_rank.to_string() << endl);
  DYN_DEBUG(oopsla_log, cout << " NEW RANK after inferInvariantsAndCounterExample()" << new_bv_rank.to_string() << endl);
  list<backtracker_node_t *> frontier = bt.get_frontier();
  ASSERT(frontier.size() != 0);
  if (this == m_correlate.choose_most_promising_correlation_entry(frontier)) { //recheck that this is still the most promising correlation entry (after computing the rank for the new CEs at the head of the new cg edge)
    this->set_dst_edge_id_to_empty();
    return backtracker_node_t::FOUND_SOLUTION;
  } else {
    DYN_DEBUG(oopsla_log, cout << " Chosen Product-CFG is not the most promising correlation after inferInvariantsAndCounterExample()" << new_bv_rank.to_string() << endl);
    return backtracker_node_t::DELAYED_EXPLORATION;
  }
  //return true;
}

//returns a map from pair of(to_pc, delta) to pair of(mu, path)
map< pair<pc,int>, pair<int,shared_ptr<tfg_edge_composition_t>>> 
correlate::get_src_unrolled_paths(shared_ptr<corr_graph const> const &cg, pcpair const &chosen_pcpair, shared_ptr<tfg_edge_composition_t> const &next_dst_ec/*, bool implying*/) const
{
  DYN_DEBUG(oopsla_log, cout << "Inside getCandCorrelations().. Enumeratedpaths are:" << endl);
  tfg const &dst_tfg = cg->get_dst_tfg();
  tfg const &src_tfg = cg->get_eq()->get_src_tfg();
  pc dst_pc_to = next_dst_ec->graph_edge_composition_get_to_pc();
  int from_pc_subsubindex = dst_pc_to.get_subsubindex();
  expr_ref incident_fcall_nextpc = dst_tfg.get_incident_fcall_nextpc(dst_pc_to);
  pc src_pc_from = chosen_pcpair.get_first();
  unsigned max_unroll = cg->get_eq()->get_quota().get_unroll_factor(); //MAX_UNROLL
  map< pair<pc,int>, shared_ptr<tfg_edge_composition_t> > src_paths_to_pcs;
  src_tfg.get_unrolled_paths_from(src_pc_from, src_paths_to_pcs, from_pc_subsubindex, incident_fcall_nextpc, max_unroll);
  map< pair<pc,int>, shared_ptr<tfg_edge_composition_t> > src_paths_to_pcs_twice_unroll;
  context *ctx = cg->get_context();
  bool disable_residual_loop_unroll = ctx->get_config().disable_residual_loop_unroll;
  if (!disable_residual_loop_unroll) {
    src_tfg.get_unrolled_paths_from(src_pc_from, src_paths_to_pcs_twice_unroll, from_pc_subsubindex, incident_fcall_nextpc, 2*max_unroll);
  }

  map< pair<pc,int>, pair<int, shared_ptr<tfg_edge_composition_t>>> ret_paths;
  for(auto const &path : src_paths_to_pcs) {
    int path_mu = path.second->find_max_node_occurence();
    int path_delta = path.first.second;
    pc const &to_pc = path.first.first;
    if(path_mu >= path_delta && max_unroll > 1 && !disable_residual_loop_unroll)// because loop residue will not be genrated for unroll == 1
    {
      ASSERT(src_paths_to_pcs_twice_unroll.count(path.first));
      auto const& path_twice_mu = src_paths_to_pcs_twice_unroll.at(path.first);
      int new_path_mu = path_twice_mu->find_max_node_occurence();
      if(new_path_mu > path_mu) {
        ret_paths.insert(make_pair(make_pair(to_pc, path_delta), make_pair(new_path_mu, path_twice_mu)));
        continue;
      }
    }
    ret_paths.insert(make_pair(make_pair(to_pc, path_delta), make_pair(path_mu, path.second)));
  }
  ASSERT(src_paths_to_pcs.size() == ret_paths.size());
  
  if(!ctx->get_config().disable_epsilon_src_paths)
  {
    expr_ref dst_edge_cond = dst_tfg.tfg_edge_composition_get_edge_cond(next_dst_ec, /*simplified*/true);
    ASSERT(dst_edge_cond->is_bool_sort());
    CPP_DBG_EXEC(CORRELATE, cout << __func__<< " dst_ec " << next_dst_ec->graph_edge_composition_to_string() << " cond:  " << ctx->expr_to_string(dst_edge_cond) << "\n");

    if (dst_edge_cond->is_const()) {
      ASSERT(dst_edge_cond->get_bool_value());
      ret_paths.insert(make_pair(make_pair(chosen_pcpair.get_first(), 0), make_pair(0, mk_epsilon_ec<pc,tfg_edge>(chosen_pcpair.get_first()))));
    }
  }
  int i = 1;
  for(auto const& p : ret_paths)
  {
    DYN_DEBUG(oopsla_log, cout << "P" << i++ << " FullPathset from nC = " << chosen_pcpair.get_first().to_string() << ", wC = " << p.first.first.to_string() << ", delta = " << p.first.second << ", mu = " << p.second.first << /*" is = " << p.second.second->graph_edge_composition_to_string() <<*/ endl);
    DYN_DEBUG(oopsla_log, cout << edge_and_path_stats_string_for_edge_composition(p.second.second) << endl << endl);
  }
  stats::get().add_counter("# of paths enumerated", ret_paths.size());

  return ret_paths;
}

bool
correlate::pc_types_match(pc const& src_pc_to, pc const& dst_pc_to, tfg const& src_tfg, tfg const& dst_tfg)
{
  if ((src_pc_to.is_exit() || dst_pc_to.is_exit()) && dst_pc_to != src_pc_to) {// exit can be correlated with corresponding exit only
    DYN_DEBUG(counters_enable, stats::get().inc_counter("corr-bt-savings"));
    CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": src_pc_to.is_exit != dst_pc_to.is_exit, ignoring." << endl);
    return false;
  }
  set<pc> src_to_pc_reachable_pcs = src_tfg.get_all_reachable_pcs(src_pc_to);
  set<pc> dst_to_pc_reachable_pcs = dst_tfg.get_all_reachable_pcs(dst_pc_to);
  if (   set_belongs(src_to_pc_reachable_pcs, src_pc_to) != set_belongs(dst_to_pc_reachable_pcs, dst_pc_to)
      && !dst_pc_to.is_fcall()) {
    //a pc may reach itself in dst iff a pc may reach itself in src
    // If loop peeling in dst and loop body has function call then src loop head can be correlated with dst non loop head
    CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": set_belongs(src_to_pc_reachable_pcs, src_pc_to) != set_belongs(dst_to_pc_reachable_pcs, dst_pc_to), ignoring." << endl);
    return false;
  }

  if (   src_pc_to.is_fcall()
      || dst_pc_to.is_fcall()) {
    CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": src_pc_to = " << src_pc_to.to_string() << ", dst_pc_to = " << dst_pc_to.to_string() << ", src/dst pc is incident to fcall\n");
    if (src_pc_to.get_subsubindex() != dst_pc_to.get_subsubindex()) {
      CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": ignoring because subsubindex do not match. src_pc_to = " << src_pc_to.to_string() << " for dst_pc_to = " << dst_pc_to.to_string() << "\n");
      return false;
    }
    expr_ref src_incident_fcall_nextpc = src_tfg.get_incident_fcall_nextpc(src_pc_to);
    expr_ref dst_incident_fcall_nextpc = dst_tfg.get_incident_fcall_nextpc(dst_pc_to);
    if (   src_incident_fcall_nextpc
        && dst_incident_fcall_nextpc) {
      CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": src_incident_fcall_nextpc = " << expr_string(src_incident_fcall_nextpc) << ", dst_incident_fcall_nextpc = " << expr_string(dst_incident_fcall_nextpc) << endl);
      if (src_incident_fcall_nextpc->is_nextpc_const() != dst_incident_fcall_nextpc->is_nextpc_const()) {
        CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": ignoring because nextpc types mismatch. src_incident_fcall_nextpc = " << expr_string(src_incident_fcall_nextpc) << ", dst_incident_fcall_nextpc = " << expr_string(dst_incident_fcall_nextpc) << endl);
        return false;
      }
      if (src_incident_fcall_nextpc->is_nextpc_const()) {
        if (src_incident_fcall_nextpc != dst_incident_fcall_nextpc) {
          CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": ignoring because nextpcs mismatch. src_incident_fcall_nextpc = " << expr_string(src_incident_fcall_nextpc) << ", dst_incident_fcall_nextpc = " << expr_string(dst_incident_fcall_nextpc) << endl);
          return false;
        }
      }
      CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": nextpcs match. not ignoring\n");
    } else {
      CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": src_incident_fcall_nextpc.get_ptr() == NULL or dst_incident_fcall_nextpc.get_ptr() == NULL\n");
    }
  }
  return true;
}

static
vector<tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>>>
get_paths_with_matching_to_pc(map<pair<pc,path_delta_t>, pair<path_mu_t, shared_ptr<tfg_edge_composition_t>>> const& src_paths_to_pcs, pc const& src_pc_to)
{
  vector<tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>>> paths_to_pc;
  for(auto const &to_pc_unroll_path_list : src_paths_to_pcs) {
    pc path_to_pc = to_pc_unroll_path_list.first.first;
    path_delta_t path_delta = to_pc_unroll_path_list.first.second;
    path_mu_t path_mu = to_pc_unroll_path_list.second.first;
    shared_ptr<tfg_edge_composition_t> const &src_path_set = to_pc_unroll_path_list.second.second;
    if (path_to_pc == src_pc_to/* && src_path_set.size() > 0*/) {
      paths_to_pc.push_back(make_tuple(path_delta,path_mu, src_path_set));
    }
  }
  return paths_to_pc;
}

list<correl_entry_t>
correlate::get_next_potential_correlations(shared_ptr<corr_graph const> const &cg, pcpair const &chosen_pcpair, shared_ptr<tfg_edge_composition_t> const &next_dst_ec, bv_rank_val_t const& parent_bv_rank/*, bool implying*/) const
{
  list<correl_entry_t> ret;
  tfg const &dst_tfg = cg->get_dst_tfg();
  tfg const &src_tfg = cg->get_eq()->get_src_tfg();
  pc dst_pc_to = next_dst_ec->graph_edge_composition_get_to_pc();
  pc src_pc_from = chosen_pcpair.get_first();
  map<pair<pc,path_delta_t>, pair<path_mu_t,shared_ptr<tfg_edge_composition_t>>> src_paths_to_pcs = get_src_unrolled_paths(cg, chosen_pcpair, next_dst_ec/*, implying*/);

  set<pc> src_to_pcs;
  for(auto const &to_pc_unroll_path_list : src_paths_to_pcs) {
    pc const& src_pc_to = to_pc_unroll_path_list.first.first;
    CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": considering candidate src_pc_to " << src_pc_to.to_string() << " from " << src_pc_from.to_string() << endl);
    if (pc_types_match(src_pc_to, dst_pc_to, src_tfg, dst_tfg)) {
      src_to_pcs.insert(src_pc_to);
    }
  }
  DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": src_to_pcs.size() = " << src_to_pcs.size() << endl);

  int total_possible_correlations = 0;
  set<pc> corr_src_to_pcs;
  if (!cg->get_counterexamples_at_pc(chosen_pcpair).size()) { //generate new CEs
    cg->generate_new_CE(chosen_pcpair, next_dst_ec);
  }
  for (auto const& src_pc_to : src_to_pcs) {
    vector<tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>>> paths_to_pc = get_paths_with_matching_to_pc(src_paths_to_pcs, src_pc_to);
    ASSERT(!paths_to_pc.empty());

    function<bool (tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>> const& , tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>> const& )> sort_by_delta = [](tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>> const& a, tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>> const& b)
    {
      return (get<0>(a) < get<0>(b));
    };
    sort(paths_to_pc.begin(), paths_to_pc.end(), sort_by_delta);
    list<correl_entry_t> feasible_correlations = this->corr_graph_prune_and_add_correlations_to_pc(cg, chosen_pcpair, next_dst_ec, src_pc_to, paths_to_pc);
    if(feasible_correlations.size())
      corr_src_to_pcs.insert(src_pc_to);
    for(auto &corr_entry : feasible_correlations) {
      bv_rank_val_t acc_bv_rank = corr_entry.get_bv_rank() + parent_bv_rank;
      corr_entry.set_bv_rank(acc_bv_rank);
      DYN_DEBUG2(correlate, cout << __func__ << "  " << __LINE__ << " NEW BV RANK " << acc_bv_rank.to_string() << endl);
      ret.push_back(corr_entry);
      total_possible_correlations++;
    }
  }
  cg->update_corr_counter(next_dst_ec, corr_src_to_pcs.size(), total_possible_correlations);
  DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": total_possible_correlations.size() = " << total_possible_correlations << endl);
  DYN_DEBUG(oopsla_log, cout << "End of expandProductCFG() function, Number of new correlations enumerated = " << total_possible_correlations << endl);
  return ret;
}

correl_entry_t::correl_entry_t(correlate const &c, shared_ptr<corr_graph> const &cg) : m_correlate(c), m_cg(cg), m_pp(pcpair::start()), m_cg_edge(nullptr)/*, m_src_pc(pc::start())*/, m_path_delta(0), m_path_mu(0), m_bv_rank(bv_rank_val_t()), m_ladder(c.get_eq()->get_dst_tfg(), correl_entry_ladder_t::LADDER_STARTPC_NODE)
{
}

//correl_entry_t::correl_entry_t(correlate const &c, shared_ptr<corr_graph const> const &cg, pcpair const &pp, shared_ptr<tfg_edge_composition_t> const &dst_ec/*edge_id_t<pc> const &dst_edge_id*/, pc const &src_pc, int path_delta, int path_mu, shared_ptr<tfg_edge_composition_t> const &candidate_src_path/*, bool implying*/) : m_correlate(c), m_cg(cg), m_pp(pp), m_dst_ec(dst_ec)/*m_dst_edge_id(dst_edge_id)*/, m_src_pc(src_pc), m_path_delta(path_delta), m_path_mu(path_mu), m_candidate_src_path(candidate_src_path)/*, m_implying(implying)*/, m_bv_rank(0), m_ladder(c.get_eq()->get_dst_tfg(), correl_entry_ladder_t::LADDER_JUST_STARTED)/*, m_num_times_get_next_level_possibilities_called(0)*/
correl_entry_t::correl_entry_t(correlate const &c, shared_ptr<corr_graph> const& cg_new, shared_ptr<corr_graph_edge const> const& cg_new_edge, int path_delta, int path_mu) : m_correlate(c), m_cg(cg_new), m_pp(cg_new_edge->get_from_pc()), m_cg_edge(cg_new_edge), m_path_delta(path_delta), m_path_mu(path_mu), m_bv_rank(bv_rank_val_t()), m_ladder(c.get_eq()->get_dst_tfg(), correl_entry_ladder_t::LADDER_JUST_STARTED)
{
}

void
correl_entry_ladder_t::get_dst_unrolled_paths_from_pc(shared_ptr<corr_graph const> const &cg, pcpair &chosen_pcpair, map<pc, list<shared_ptr<tfg_edge_composition_t>>> &unrolled_dst_pths)
{
  tfg const &dst_tfg = cg->get_dst_tfg();
  tfg const &src_tfg = cg->get_eq()->get_src_tfg();
  context *ctx = cg->get_context();
  int max_unroll_dst = ctx->get_config().dst_unroll_factor;
  ASSERT(max_unroll_dst >= 1);
  map<pair<pc,int>, shared_ptr<tfg_edge_composition_t>> dst_paths_to_pcs;
  int from_pc_subsubindex = chosen_pcpair.get_second().get_subsubindex();
  expr_ref incident_fcall_nextpc = dst_tfg.get_incident_fcall_nextpc(chosen_pcpair.get_second());
  dst_tfg.get_unrolled_paths_from(chosen_pcpair.get_second(), dst_paths_to_pcs, from_pc_subsubindex, incident_fcall_nextpc, max_unroll_dst);
  for(auto const &pc_freq_ec : dst_paths_to_pcs) {
    auto const &ec = pc_freq_ec.second;
    pc const& to_pc = pc_freq_ec.first.first;
    if (!cg->dst_edge_exists_in_outgoing_cg_edges(chosen_pcpair, to_pc)) {
      if (!cg->dst_edge_composition_proves_false(chosen_pcpair, ec)) { // edge condition proves false
        DYN_DEBUG2(correlate, cout << __func__ << " " << __LINE__ << ": adding " << chosen_pcpair.to_string() << " and " << ec->graph_edge_composition_to_string() << " to unrolled chosen dst edges\n");
        unrolled_dst_pths[to_pc].push_back(ec);

      } else 
        DYN_DEBUG2(correlate, cout << __func__ << " " << __LINE__ << ": ignoring " << chosen_pcpair.to_string() << " and " << ec->graph_edge_composition_to_string() << " because dst_edge_proves_false returned true\n");
    } else 
      DYN_DEBUG2(correlate, cout << __func__ << " " << __LINE__ << ": ignoring " << chosen_pcpair.to_string() << " and " << ec->graph_edge_composition_to_string() << " because dst_edge already exists for to_pc\n");
  }
}

bool
correl_entry_ladder_t::get_next_dst_edge_composition_to_correlate(shared_ptr<corr_graph const> const &cg, map<pc,int> const &ordered_dst_pcs, list<shared_ptr<tfg_edge_composition_t>> &unrolled_dst_pths, pcpair &chosen_pcpair) const
{
  map<pair<pcpair, shared_ptr<tfg_edge_composition_t>>, list<shared_ptr<tfg_edge_composition_t>>> candidate_unrolled_pths;
  tfg const &dst_tfg = cg->get_dst_tfg();
  tfg const &src_tfg = cg->get_eq()->get_src_tfg();
  context *ctx = cg->get_context();

  CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ":" << endl);
  for (auto pp : cg->get_all_pcs()) {
    pc dst_pc = pp.get_second();
    CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": dst_pc = " << dst_pc.to_string() << endl);
    map<pc, list<shared_ptr<tfg_edge_composition_t>>> unrolled_dst_pths;
    this->get_dst_unrolled_paths_from_pc(cg, pp, unrolled_dst_pths);

    for (auto to_pc_paths: unrolled_dst_pths) {
      pc const& to_pc = to_pc_paths.first;
      list<shared_ptr<tfg_edge_composition_t>> const &unrolled_reachable_paths = to_pc_paths.second;
      ASSERT(unrolled_reachable_paths.size());
      for(auto const& dst_ec : unrolled_reachable_paths)
        DYN_DEBUG(print_pathsets, cout << _FNLN_ << ": pathset: " << dst_ec->to_string_concise() << endl);
      candidate_unrolled_pths[make_pair(pp,unrolled_reachable_paths.front())] = unrolled_reachable_paths;
    }
  }
  if (candidate_unrolled_pths.size() == 0) {
    return false;
  }
  shared_ptr<tfg_edge_composition_t> chosen_dst_edge;
  for (auto const& cod : candidate_unrolled_pths) {
    if (   !chosen_dst_edge
        || dst_edge_appears_earlier(ordered_dst_pcs, cod.first.second, chosen_dst_edge)) {
      chosen_dst_edge = cod.first.second;
      chosen_pcpair = cod.first.first;
    }
    else if(cod.first.second == chosen_dst_edge && chosen_pcpair.get_first() < cod.first.first.get_first()) {
      chosen_dst_edge = cod.first.second;
      chosen_pcpair = cod.first.first;
    }
    DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": chosen_dst_edge = " << chosen_dst_edge->graph_edge_composition_to_string() << " ,chosen_pcpair = " << chosen_pcpair.to_string() << endl);
    DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": cod.dst_edge = " << cod.first.second->graph_edge_composition_to_string() << " ,cod.pcpair = " << cod.first.first.to_string() << endl);
  }
  ASSERT(chosen_dst_edge);
  ASSERT(candidate_unrolled_pths.count(make_pair(chosen_pcpair,chosen_dst_edge)));
  ASSERT(candidate_unrolled_pths.at(make_pair(chosen_pcpair,chosen_dst_edge)).size());
  unrolled_dst_pths = candidate_unrolled_pths[make_pair(chosen_pcpair,chosen_dst_edge)];
  DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": chosen_dst_edge = " << chosen_dst_edge->graph_edge_composition_to_string() << endl);
  DYN_DEBUG(oopsla_log, cout << "Pcpair returned by findIncompleteNode() in the constructed partial product-CFG, nA = " << chosen_pcpair.get_second().to_string() << endl);
  DYN_DEBUG(oopsla_log, cout << "FullPathSet in A returned by getNextPathsetRPO() with nexthop PC hA = " << chosen_dst_edge->graph_edge_composition_get_to_pc() << " is: " << chosen_dst_edge->graph_edge_composition_to_string() << endl);

  return true;
}


list<correl_entry_t> 
correlate::corr_graph_prune_and_add_correlations_to_pc(shared_ptr<corr_graph const> const& cg_in, pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const& dst_ec, pc const& src_pc_to, vector<tuple<path_delta_t, path_mu_t, shared_ptr<tfg_edge_composition_t>>> const& src_pathsets_to_pc) const
{
  list<correl_entry_t> feasible_correlations;
  for(auto const& delta_mu_path : src_pathsets_to_pc)
  {
    path_delta_t path_delta = get<0>(delta_mu_path);
    path_mu_t path_mu = get<1>(delta_mu_path);
    shared_ptr<tfg_edge_composition_t> const& src_path = get<2>(delta_mu_path);
    shared_ptr<corr_graph_edge const> cg_new_edge;
    shared_ptr<corr_graph> cg_new = this->corr_graph_add_correlation(cg_in, cg_from_pc, dst_ec, src_path, cg_new_edge);
    ASSERT(cg_new && cg_new_edge);
    pair<bool, bool> is_implied_stable =  cg_new->corr_graph_propagate_CEs_on_edge(cg_new_edge);
    DYN_DEBUG(oopsla_log, cout << "Inside function addEdgeAndPropCEs() for PathSet in C =" << cg_new_edge->get_src_edge()->graph_edge_composition_to_string() << " PathSet in A = " << cg_new_edge->get_dst_edge()->graph_edge_composition_to_string() << endl);
    DYN_DEBUG(oopsla_log, cout << "Output of function CEsSatisfyCorrelCriterion(): " << is_implied_stable.first << endl);
    DYN_DEBUG(oopsla_log, cout << "Output of function InvRelatesHeapsAtEachNode(): " << is_implied_stable.second << endl);
    if(is_implied_stable.first && is_implied_stable.second) {
      DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": adding potential correlation " << cg_new_edge->to_string() << " path delta = " << path_delta << " path_mu = " << path_mu << endl);
      correl_entry_t corr_entry(*this, cg_new, cg_new_edge, path_delta, path_mu);
      bv_rank_val_t bv_rank = cg_new->calculate_rank_bveqclass_at_pc(cg_new_edge->get_to_pc());
      corr_entry.set_bv_rank(bv_rank);
      DYN_DEBUG(oopsla_log, cout << "Passed both CEsSatisfyCorrelCriterion and InvRelatesHeapsAtEachNode checks.. Computed RANK = " << bv_rank.to_string() << endl);
      feasible_correlations.push_back(corr_entry);
    }
    if(!is_implied_stable.first)
    {
      DYN_DEBUG(correlate, cout << _FNLN_ << " Removing non implied sibling " << endl);
      stats::get().add_counter("# of Paths Prunned -- CEsSatisfyCorrelCriterion", (src_pathsets_to_pc.size() - path_delta));
      break; // early exit: paths with higher delta will also fail implied check
    }
  }
  return feasible_correlations;
}


shared_ptr<corr_graph>
correlate::corr_graph_add_correlation(shared_ptr<corr_graph const> const& cg_in, pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const& dst_ec, shared_ptr<tfg_edge_composition_t> const& src_path, shared_ptr<corr_graph_edge const>& cg_new_edge) const
{
  autostop_timer func_timer(__func__);
  ASSERT(!cg_new_edge);
  tfg const &src_tfg = cg_in->get_eq()->get_src_tfg();
  tfg const &dst_tfg = cg_in->get_dst_tfg();

  /** create a new CG and add the new edge to it **/
  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);
  shared_ptr<corr_graph> cg_new = cg_in->cg_copy_corr_graph();
  PRINT_PROGRESS("%s() %d:\n", __func__, __LINE__);

  ASSERT(src_path);

  pc src_pc_to = src_path->graph_edge_composition_get_to_pc();

  pcpair cg_to_pc(src_pc_to, dst_ec->graph_edge_composition_get_to_pc());
  bool cg_new_node_added = false;
  if (!cg_new->find_node(cg_to_pc)) {
    autostop_timer bt(string(__func__)+".new_node");
    auto dst_pc_to = dst_ec->graph_edge_composition_get_to_pc();
    if (   dst_tfg.get_name()->get_str() != "ir"
        && dst_pc_to.get_subsubindex() == PC_SUBSUBINDEX_FCALL_START
        && !cg_new->dst_fcall_edge_already_updated(dst_pc_to)) {
      cg_new->update_dst_fcall_edge_using_src_fcall_edge(src_pc_to, dst_ec->graph_edge_composition_get_to_pc());
    }
    cg_new->cg_add_node(make_shared<corr_graph_node>(cg_to_pc));
    cg_new_edge = add_corr_graph_edge(cg_new, cg_from_pc, cg_to_pc, src_path, dst_ec);
    set<pcpair> to_pc_set;
    to_pc_set.insert(cg_to_pc);
    //cg_new->populate_loc_liveness(/*false*/);
    //cg_new->init_linear_relations_to_top_for_new_graph_addrs(to_pc_set);
    cg_new->cg_add_asserts_at_pc(cg_to_pc);
    DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": Added node " << cg_to_pc.to_string() << " to correlation graph" << endl);
    cg_new_node_added = true;
  }
  else {
    cg_new_edge = add_corr_graph_edge(cg_new, cg_from_pc, cg_to_pc, src_path, dst_ec);
  }

  DYN_DEBUG(eqcheck, cout << __func__ << ':' << __LINE__ << ": added edge " << cg_new_edge->to_string() << endl);
  CPP_DBG_EXEC2(CORRELATE, cout << __func__ << " " << __LINE__ << ": dst_edge = " << dst_ec->graph_edge_composition_to_string() << endl);
  CPP_DBG_EXEC3(CORRELATE, cout << __func__ << " " << __LINE__ << ": src_tfg =\n" << src_tfg.graph_to_string() << endl);
  CPP_DBG_EXEC3(CORRELATE, cout << __func__ << " " << __LINE__ << ": dst_tfg =\n" << dst_tfg.graph_to_string() << endl);

  cg_new->populate_transitive_closure();

  if (cg_new_node_added){
    cg_new->graph_with_guessing_add_node(cg_to_pc);
  }

  return cg_new;
}

}
