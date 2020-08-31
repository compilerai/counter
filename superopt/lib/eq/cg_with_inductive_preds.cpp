#include <cassert>
#include <string>
#include <vector>
#include <stack>
#include <fstream>
#include <algorithm>
#include <functional>

#include "support/debug.h"
#include "support/consts.h"
#include "eq/cg_with_inductive_preds.h"
#include "eq/corr_graph.h"
#include "eq/eqcheck.h"
#include "support/utils.h"
#include "expr/expr.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "expr/aliasing_constraints.h"
#include "expr/context_cache.h"
#include "support/log.h"
#include "support/globals.h"
#include "tfg/parse_input_eq_file.h"
#include "support/globals_cpp.h"
#include "i386/regs.h"
#include "ppc/regs.h"
//#include "eq/invariant_state_helper.h"
//#include "graph/infer_invariants_dfa.h"

#include <boost/lexical_cast.hpp>


namespace eqspace {

using namespace std;

//shared_ptr<cg_with_inductive_preds>
//cg_with_inductive_preds::cg_read_from_file(string const &filename, shared_ptr<eqcheck> const& e)
//{
//  ifstream in(filename);
//  context *ctx = e->get_context();
//  if (!in.is_open()) {
//    cout << __func__ << " " << __LINE__ << ": parsing failed: could not open file " << filename << endl;
//    return nullptr;
//  }
//  string const &function_name = e->get_function_name();
//  string match_line(string(FUNCTION_NAME_FIELD_IDENTIFIER) + " " + function_name);
//  string line;
//  bool end = !getline(in, line);
//  while (!is_line(line, match_line)) {
//    end = !getline(in, line);
//    if (end) {
//      cout << __func__ << " " << __LINE__ << ": function " << function_name << " not found in cg file " << filename << endl;
//      return nullptr;
//    }
//  }
//  if (!is_next_line(in, "=result")) {
//    cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
//    NOT_REACHED();
//    return nullptr;
//  }
//  getline(in, line);
//  if (line != "1") {
//    cout << __func__ << " " << __LINE__ << ": proof result was NOT-PROVABLE\n";
//    NOT_REACHED();
//    return nullptr;
//  }
//  tfg const &src_tfg = e->get_src_tfg();
//  tfg const &dst_tfg = e->get_dst_tfg();
//  consts_struct_t const &cs = src_tfg.get_consts_struct();
//  //state const &src_start_state = src_tfg.get_start_state();
//  //state const &dst_start_state = dst_tfg.get_start_state();
//  //const auto &symbol_map = src_tfg.get_symbol_map();
//  //const auto &locals_map = src_tfg.get_locals_map();
//  //map<string_ref, expr_ref> const &argument_regs = src_tfg.get_argument_regs();
//  shared_ptr<corr_graph> cg = make_shared<corr_graph>(mk_string_ref("cg"), e/*, cs, src_start_state, dst_start_state, symbol_map, locals_map, argument_regs*/);
//  getline(in, line);
//  ASSERT(is_line(line, "=DstTfg:"));
//  getline(in, line);
//  ASSERT(is_line(line, "=TFG:"));
//  tfg *tfg_dst_preprocessed = new tfg(in, "dst", ctx);
//  //tfg_dst_preprocessed->graph_from_stream(in);
//  //read_tfg(in, &tfg_dst_preprocessed, "dst", ctx, false);
//  //tfg_dst_preprocessed->populate_transitive_closure();
//  //tfg_dst_preprocessed->populate_reachable_and_unreachable_incoming_edges_map();
//  //tfg_dst_preprocessed->populate_auxilliary_structures_dependent_on_locs();
//  tfg_dst_preprocessed->tfg_preprocess_before_eqcheck({}, "", false);
//  cg->set_dst_tfg(*tfg_dst_preprocessed);
//  delete tfg_dst_preprocessed;
//  do {
//    getline(in, line);
//  } while (line == "");
//  if (!is_line(line, "=Nodes:")) {
//    cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  }
//  ASSERT(is_line(line, "=Nodes:"));
//  getline(in, line);
//  while (!is_line(line, "=Edges:")) {
//    pcpair pp = pcpair::create_from_string(line);
//    //cout << __func__ << " " << __LINE__ << ": pp = " << pp.to_string() << endl;
//    shared_ptr<tfg_node const> src_node = src_tfg.find_node(pp.get_first());
//    ASSERT(src_node);
//    shared_ptr<tfg_node const> dst_node = dst_tfg.find_node(pp.get_second());
//    ASSERT(dst_node);
//    shared_ptr<corr_graph_node> cg_node = make_shared<corr_graph_node>(pp);
//    cg->cg_add_node(cg_node);
//    getline(in, line);
//  }
//  ASSERT(is_line(line, "=Edges:"));
//  getline(in, line);
//  while (is_line(line, "=Edge")) {
//    getline(in, line);
//    size_t arrow = line.find(" => ");
//    ASSERT(arrow != string::npos);
//    string from_pc_str = line.substr(0, arrow);
//    string to_pc_str = line.substr(arrow + 4);
//    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//    //cout << __func__ << " " << __LINE__ << ": from_pc_str = " << from_pc_str << "." <<  endl;
//    //cout << __func__ << " " << __LINE__ << ": to_pc_str = " << to_pc_str << "." << endl;
//    pcpair from_pc = pcpair::create_from_string(from_pc_str);
//    pcpair to_pc = pcpair::create_from_string(to_pc_str);
//    shared_ptr<corr_graph_node const> from_node = cg->find_node(from_pc);
//    ASSERT(from_node);
//    shared_ptr<corr_graph_node const> to_node = cg->find_node(to_pc);
//    ASSERT(to_node);
//    getline(in, line);
//    ASSERT(is_line(line, "=SrcEdge"));
//    pc src_from_pc, src_to_pc, dst_from_pc, dst_to_pc;
//    string src_comment, dst_comment;
//    getline(in, line);
//    //edge_read_pcs_and_comment(line, src_from_pc, src_to_pc, src_comment);
//    shared_ptr<tfg_edge_composition_t> src_ec = parse_edge_composition<pc, tfg_node, tfg_edge>(/*src_tfg, */line.c_str());
//    getline(in, line);
//    ASSERT(is_line(line, "=SrcEdgecond"));
//    expr_ref src_econd;
//    line = read_expr(in, src_econd, ctx);
//
//    ASSERT(is_line(line, "=DstEdge"));
//    getline(in, line);
//    //edge_read_pcs_and_comment(line, dst_from_pc, dst_to_pc, dst_comment);
//    shared_ptr<tfg_edge_composition_t> dst_ec = parse_edge_composition<pc, tfg_node, tfg_edge>(/*dst_tfg, */line.c_str());
//    getline(in, line);
//    ASSERT(is_line(line, "=DstEdgecond"));
//    expr_ref dst_econd;
//    line = read_expr(in, dst_econd, ctx);
//    auto cg_edge = make_shared<corr_graph_edge>(from_node, to_node, src_ec, dst_ec, src_tfg, dst_tfg, cg->get_start_state(), ctx);
//    cg->add_edge(cg_edge);
//  }
//  ASSERT(is_line(line, "=ProvableInvariants"));
//  map<pcpair, unordered_set<predicate>> inductive_preds_map;
//  getline(in, line);
//  while (is_line(line, "=Node")) {
//    size_t colon = line.find(":");
//    ASSERT(colon != string::npos);
//    string pp_str = line.substr(5, colon);
//    pcpair pp = pcpair::create_from_string(pp_str);
//    unordered_set<predicate> invariants;
//    getline(in, line);
//    while (is_line(line, "=Assume") || is_line(line, "=ProvablePred")) {
//      shared_ptr<predicate> invariant;
//      line = predicate::predicate_from_stream(in, ctx/*, &src_tfg, ctx*/, invariant);
//      invariants.insert(*invariant);
//    }
//    inductive_preds_map.insert(make_pair(pp, invariants));
//    //cg->add_preds(pp, invariants);
//  }
//  ASSERT(is_line(line, "=LocalSprelAssumptions"));
//  local_sprel_expr_guesses_t lsprel_guesses(cs);
//  line = read_local_sprel_expr_assumptions(in, ctx, lsprel_guesses);
//  cg->add_local_sprel_expr_assumes(lsprel_guesses);
//
//  cg->populate_transitive_closure();
//  cg->cg_add_precondition(/*relevant_memlabels*/);
//  cg->update_src_suffixpaths_cg(nullptr);
//
//  cg->cg_do_alias_analysis();
//  cg->cg_populate_simplified_values();
//
//  shared_ptr<cg_with_inductive_preds> cgi = make_shared<cg_with_inductive_preds>(*cg, inductive_preds_map);
//  ASSERT(cgi);
//  return cgi;
//}

expr_ref
cg_with_inductive_preds::cg_get_simplified_edgecond(shared_ptr<corr_graph_edge const> const& e) const
{
  context* ctx = this->get_context();
  tfg const &src_tfg = this->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();

  shared_ptr<tfg_edge_composition_t> src_ec = e->get_src_edge();
  shared_ptr<tfg_edge_composition_t> dst_ec = e->get_dst_edge();
  expr_ref src_edge_cond = src_tfg.tfg_edge_composition_get_edge_cond(src_ec, true);
  expr_ref dst_edge_cond = dst_tfg.tfg_edge_composition_get_edge_cond(dst_ec, true);
  ASSERT(src_edge_cond->is_bool_sort());
  ASSERT(dst_edge_cond->is_bool_sort());

  return expr_and(src_edge_cond, dst_edge_cond);
}

bool
cg_with_inductive_preds::cg_outgoing_edges_at_pc_are_well_formed(pcpair const& pp) const
{
  context* ctx = this->get_context();
  list<shared_ptr<corr_graph_edge const>> outgoing;
  this->get_edges_outgoing(pp, outgoing);
  tfg const &src_tfg = this->get_src_tfg();
  if (outgoing.empty()) {
    return true;
  }

  expr_ref econd_union = expr_false(ctx);
  predicate_set_t src_assume_preds; 
  for (auto const& e_out : outgoing) {
    // use more precise edgeconds
    econd_union = expr_or(econd_union, this->cg_get_simplified_edgecond(e_out));
    predicate_set_t lhs_preds = src_tfg.collect_assumes_around_path(pp.get_first(), e_out->get_src_edge());
    predicate::predicate_set_union(src_assume_preds, lhs_preds);
  }
  econd_union = ctx->expr_do_simplify(econd_union);
  bool guess_made_weaker;
  // not adding aliasing constraints as no counter example is expected out of the query
  predicate_ref econd_is_true = predicate::mk_predicate_ref(precond_t(), econd_union, expr_true(ctx), string("outgoing_edges_are_well_formed.") + pp.to_string());
  proof_status_t proof_status = this->graph_with_proofs::decide_hoare_triple(src_assume_preds, pp, mk_epsilon_ec<pcpair,corr_graph_edge>(), econd_is_true, guess_made_weaker);

  if (proof_status == proof_status_timeout) {
    NOT_IMPLEMENTED();
  }
  if (proof_status != proof_status_proven) {
    CPP_DBG_EXEC(CG_IS_WELL_FORMED, cout << __func__ << ':' << __LINE__ << ": CG outgoing edge check failed! econd_union = " << expr_string(econd_union) << endl);
    return false;
  }
  return true;
}

bool
cg_with_inductive_preds::check_preds_on_edge(shared_ptr<corr_graph_edge const> const& e_in, predicate_set_t const& preds) const
{
  pcpair const& from_pp = e_in->get_from_pc();

  for (const auto &pred : preds) {
    bool guess_made_weaker;
    shared_ptr<graph_edge_composition_t<pcpair,corr_graph_edge> const> cg_ec = mk_edge_composition<pcpair,corr_graph_edge>(e_in);
    proof_status_t proof_status = this->graph_with_proofs::decide_hoare_triple({}/*from_preds*/, from_pp, cg_ec, pred, guess_made_weaker);
    if (proof_status == proof_status_timeout) {
      NOT_IMPLEMENTED();
    }
    if (proof_status != proof_status_proven) {
      DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": pred failed on edge " << e_in->to_string() << ": " << pred->to_string(true) << endl);
      return false;
    }
  }
  return true;
}

bool
cg_with_inductive_preds::check_inductive_preds_on_edge(shared_ptr<corr_graph_edge const> const& e_in) const
{
  pcpair const& to_pp = e_in->get_to_pc();
  map<pcpair, predicate_set_t> const& inductive_preds = this->graph_with_guessing_get_invariants_map();
  predicate_set_t const &preds = inductive_preds.at(to_pp);
  return this->check_preds_on_edge(e_in, preds);
}

bool
cg_with_inductive_preds::check_asserts_on_edge(shared_ptr<corr_graph_edge const> const &e_in) const
{
  autostop_timer ft(__func__);
  pcpair const& to_pp = e_in->get_to_pc();
  predicate_set_t const &assert_preds = this->get_simplified_assert_preds(to_pp);
  return this->check_preds_on_edge(e_in, assert_preds);
}

bool
cg_with_inductive_preds::check_equivalence_proof() const
{
  autostop_timer ft(__func__);

  for (auto pp : this->get_all_pcs()) {
    // 1. outgoing edges are well-formed
    if (!this->cg_outgoing_edges_at_pc_are_well_formed(pp)) {
      cout << __func__ << " " << __LINE__ << ": failed at pp = " << pp.to_string() << endl;
      MSG("Equivalence check failed at cg_with_inductive_preds; indicates a bug in find_correlation()\n");
      return false;
    }
    list<shared_ptr<corr_graph_edge const>> incoming;
    this->get_edges_incoming(pp, incoming);
    for (auto const& e_in : incoming) {
      // 2. inductive preds are provable
      if (!check_inductive_preds_on_edge(e_in)) {
        MSG("Equivalence check failed at cg_with_inductive_preds; indicates a bug in find_correlation()\n");
        return false;
      }
      // 3. assertions are provable
      if (!check_asserts_on_edge(e_in)) {
        MSG("Equivalence check failed at cg_with_inductive_preds; indicates a bug in find_correlation()\n");
        return false;
      }
    }
  }
  MSG("Equivalence check passed: product CFG has strong enough inductive invariants\n");
  return true;
}

}
