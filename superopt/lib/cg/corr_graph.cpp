#include <cassert>
#include <string>
#include <vector>
#include <stack>
#include <fstream>
#include <algorithm>
#include <functional>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>

#include "support/debug.h"
#include "support/consts.h"
#include "cg/corr_graph.h"
#include "support/utils.h"
#include "expr/expr.h"
#include "expr/expr_simplify.h"
#include "expr/solver_interface.h"
#include "expr/aliasing_constraints.h"
#include "expr/context_cache.h"
#include "support/log.h"
#include "support/globals.h"
#include "tfg/parse_input_eq_file.h"
#include "tfg/tfg_llvm.h"
#include "tfg/tfg_asm.h"
#include "support/globals_cpp.h"
#include "i386/regs.h"
#include "ppc/regs.h"
#include "gsupport/fsignature.h"
//#include "eq/invariant_state_helper.h"
//#include "graph/infer_invariants_dfa.h"

#include <boost/lexical_cast.hpp>
#include "rewrite/cfg_pc.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"

#define DEBUG_TYPE "corr_graph"
static char as1[4096];

bool g_correl_locals_flag = true;
bool g_correl_locals_try_all_possibilities = false;

namespace eqspace {


pair<bool,bool> 
corr_graph::counter_example_translate_on_edge(counter_example_t const &c_in, corr_graph_edge_ref const &edge, counter_example_t &ret, counter_example_t &rand_vals, set<memlabel_ref> const& relevant_memlabels) const
{
  autostop_timer func_timer(__func__);
  context *ctx = this->get_context();
  tfg const &src_tfg = this->get_eq()->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();

  DYN_DEBUG2(ce_translate, cout << __func__ << " " << __LINE__ << ": edge = " << edge->to_string() << ", c_in =\n" << c_in.counter_example_to_string() << endl);

  counter_example_t src_out_ce(ctx);
  counter_example_t dst_out_ce(ctx);
  pair<bool, bool> dst_exec_econd_status = dst_tfg.counter_example_translate_on_edge_composition(c_in, edge->get_dst_edge(), dst_out_ce, rand_vals, relevant_memlabels);
  if(!dst_exec_econd_status.first)
    return make_pair(false,false);
  DYN_DEBUG2(ce_translate,
    counter_example_t dst_out_ce_combo(dst_out_ce);
    dst_out_ce_combo.counter_example_union(rand_vals);
    cout << __func__ << " " << __LINE__ << ": dst out ce combo =\n" << dst_out_ce_combo.counter_example_to_string() << endl;
  );
  pair<bool, bool> src_exec_econd_status = src_tfg.counter_example_translate_on_edge_composition(dst_out_ce, edge->get_src_edge(), src_out_ce, rand_vals, relevant_memlabels);
  DYN_DEBUG2(ce_translate,
    counter_example_t src_out_ce_combo(src_out_ce);
    src_out_ce_combo.counter_example_union(rand_vals);
    cout << __func__ << " " << __LINE__ << ": src out ce combo =\n" << src_out_ce_combo.counter_example_to_string() << endl;
  );

  if(src_exec_econd_status.first) {
    ret = src_out_ce;
//    counter_example_t ret1(ctx);
//    graph_with_execution::counter_example_translate_on_edge(c_in, edge, ret1, rand_vals, mlasserts);
//    counter_example_t ret2 = ret;
//    ret2.counter_example_union(rand_vals);
//    ret1.counter_example_union(rand_vals);
//    if(!ret2.counter_example_equals(ret1)) {
//      cout << __func__ << " " << __LINE__ << ": ret2 out ce =\n" << ret2.counter_example_to_string() << endl;
//      cout << __func__ << " " << __LINE__ << ": ret1 out ce =\n" << ret1.counter_example_to_string() << endl;
//      cout << __func__ << " " << __LINE__ << ": rand_vals out ce =\n" << rand_vals.counter_example_to_string() << endl;
//    }
//    ASSERT(ret2.counter_example_equals(ret1) );
  }
  return src_exec_econd_status;
}

bool
corr_graph::dst_edge_exists_in_cg_edges(shared_ptr<tfg_edge const> const& dst_edge) const
{
  list<corr_graph_edge_ref> cg_edges;
  tfg const &dst_tfg = this->get_dst_tfg();
  shared_ptr<tfg_edge_composition_t> dst_ec = mk_edge_composition<pc,tfg_edge>(dst_edge);
  //get_edges(cg_edges);
  this->get_edges(cg_edges);
  for (auto cg_edge : cg_edges) {
    shared_ptr<tfg_edge_composition_t> de = cg_edge->get_dst_edge();
    if (de == dst_ec) {
      return true;
    }
  }
  return false;
}

bool
corr_graph::dst_edge_exists_in_outgoing_cg_edges(pcpair const &pp, pc const& to_pc) const
{
  list<corr_graph_edge_ref> cg_edges;
  tfg const &dst_tfg = this->get_dst_tfg();
  this->get_edges_outgoing(pp, cg_edges);
  for (auto cg_edge : cg_edges) {
    shared_ptr<tfg_edge_composition_t> de = cg_edge->get_dst_edge();
    //if (de == dst_ec) {
    //  return true;
    //}
    if (de->graph_edge_composition_get_to_pc() == to_pc) {
      return true;
    }
  }
  return false;
}

state const &corr_graph::get_src_start_state() const
{
  return this->get_src_tfg().get_start_state();
}

state const &corr_graph::get_dst_start_state() const
{
  return this->get_dst_tfg().get_start_state();
}

const pc& corr_graph_node::get_src_pc() const
{
  return this->get_pc().get_first();
}

const pc& corr_graph_node::get_dst_pc() const
{
  return this->get_pc().get_second();
}

/*void corr_graph::add_edge(shared_ptr<corr_graph_edge> e)
{
  graph_with_predicates::add_edge(e);
  m_guessing.new_edge_added(e);
}*/

void corr_graph::init_cg_node(shared_ptr<corr_graph_node> n)
{
  pcpair const& pp = n->get_pc();
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();
  predicate_set_t const &src_preds = src_tfg.get_assume_preds(pp.get_first());
  predicate_set_t const &dst_preds = dst_tfg.get_assume_preds(pp.get_second());
  predicate_set_t preds = src_preds;
  preds.insert(dst_preds.begin(), dst_preds.end());

  for (auto pred : preds) {
    add_assume_pred(pp, pred);
  }

  // initialize to epsilon for new node
  m_src_suffixpath[pp] = mk_epsilon_ec<pc,tfg_edge>();
}


void corr_graph::cg_add_node(shared_ptr<corr_graph_node> n)
{
  this->graph::add_node(n);
  this->init_cg_node(n);
}

shared_ptr<eqcheck> const& corr_graph::get_eq() const
{
  return m_eq;
}

void
corr_graph::write_failure_to_proof_file(string const &filename, string const &function_name)
{
  ofstream proof_file;
  proof_file.open(filename, ofstream::out | ofstream::app);
  if (!proof_file) {
    cout << __func__ << " " << __LINE__ << ": failed to open proof file '" << filename << "'\n" << endl;
    cout << __func__ << " " << __LINE__ << ": errno = " << strerror(errno) << endl;
  }
  ASSERT(proof_file);
  proof_file << FUNCTION_NAME_FIELD_IDENTIFIER " " << function_name << endl;
  proof_file << PROOF_FILE_RESULT_PREFIX << "0\n";
  proof_file.close();
}

void
corr_graph::write_provable_cg_to_proof_file(string const &filename, string const &function_name) const
{
  ofstream proof_file;
  proof_file.open(filename, ofstream::out | ofstream::app);
  if (!proof_file) {
    cout << __func__ << " " << __LINE__ << ": failed to open proof file '" << filename << "'\n" << endl;
    cout << __func__ << " " << __LINE__ << ": errno = " << strerror(errno) << endl;
  }
  ASSERT(proof_file);
  proof_file << FUNCTION_NAME_FIELD_IDENTIFIER " " << function_name << endl;
  //proof_file << "=result\n";
  //proof_file << result << endl;
  //if (result) {
  //  //proof_file << "=fcall_side_effects\n";
  //  //proof_file << fcall_side_effects << endl;
  //  proof_file << this->cg_to_string_for_eq();
  //}
  //proof_file << "\n";
  proof_file << PROOF_FILE_RESULT_PREFIX "1\n";
  this->graph_to_stream(proof_file);
  proof_file.close();
}

string
corr_graph::cg_preds_to_string_for_eq(pcpair const &pp) const
{
  predicate_set_t const &preds = this->graph_with_guessing_get_invariants_at_pc(pp);
  int num_preds = 0;
  stringstream ss;
  for (const auto &pred : preds) {
    ss << "=Pred" << num_preds << endl;
    ss << pred->to_string_for_eq() << endl;
    num_preds++;
  }
  stringstream ss2;
  ss2 << "=Node" << pp.to_string() << ": " << num_preds << " preds\n";
  return ss2.str() + ss.str();
}

list<shared_ptr<corr_graph_node>>
corr_graph::find_cg_nodes_with_dst_node(pc dst) const
{
  list<shared_ptr<corr_graph_node>> ns, ret;
  get_nodes(ns);
  for (auto n : ns) {
    if (n->get_pc().get_second() == dst) {
      ret.push_back(n);
    }
  }
  return ret;
}

void
corr_graph::cg_add_precondition()
{
  context *ctx = this->get_context();
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();

  assert(m_eq->get_src_tfg().get_argument_regs().size() == dst_tfg.get_argument_regs().size());
  for (const auto &src_a : src_tfg.get_argument_regs().get_map()) {
    expr_ref src = src_a.second;
    ASSERT(dst_tfg.get_argument_regs().count(src_a.first));
    expr_ref dst = dst_tfg.get_argument_regs().at(src_a.first); //dst_tfg.get_argument_regs()[i];
    if (src == dst) {
      continue;
    }
    stringstream ss;
    ss << PRECOND_PREFIX << src_a.first;
    predicate_ref start_cond = predicate::mk_predicate_ref(precond_t(), src, dst, ss.str());
    add_assume_pred(find_entry_node()->get_pc(), start_cond);
  }

  expr_ref src_mem, dst_mem;
  bool ret;
  ret = this->get_src_start_state().get_memory_expr(this->get_src_start_state(), src_mem);
  ASSERT(ret);
  ret = this->get_dst_start_state().get_memory_expr(this->get_dst_start_state(), dst_mem);
  ASSERT(ret);

  memlabel_t ml = this->get_memlabel_assertions().get_memlabel_all_except_locals_and_stack();
  expr_ref memeq = ctx->mk_memmasks_are_equal(src_mem, dst_mem, ml);
  predicate_ref start_cond = predicate::mk_predicate_ref(precond_t(), memeq, expr_true(ctx), string(PRECOND_PREFIX "memmask-") + ml.to_string());
  add_assume_pred(find_entry_node()->get_pc(), start_cond);
}

void
corr_graph::cg_add_asserts_at_pc(pcpair const &pp)
{
  //add asserts of the constituent pcs of the respective tfgs
  //add asserts appearing on the constituent incoming paths of the CG's incoming edges
  //NOT_IMPLEMENTED();
  if (pp.is_exit()) {
    this->cg_add_exit_asserts_at_pc(pp);
  }
  this->populate_simplified_assert_map({ pp });
}

void
corr_graph::cg_add_exit_asserts_at_pc(pcpair const &exit)
{
  context* ctx = this->get_context();
  tfg const &src_tfg = m_eq->get_src_tfg();
  pc const &p_src = exit.get_first();
  pc const &p_dst = exit.get_second();
  ASSERT(p_src == p_dst);
  auto n = find_node(exit);
  map<string_ref, expr_ref> const &return_regs = m_eq->get_src_tfg().get_return_regs_at_pc(p_src);
  //cout << __func__ << " " << __LINE__ << ": p_src = " << p_src.to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": return_regs.size() = " << return_regs.size() << endl;
  expr_ref src_mem, dst_mem;
  set<memlabel_ref> ret_memlabels;
  string ret_mem_name;
  for(const auto &pr : return_regs) {
    string_ref const &src_name = pr.first;
    expr_ref const &src = pr.second;
    map<string_ref, expr_ref> const &retregs = this->get_dst_tfg().get_return_regs_at_pc(p_dst);
    if (!retregs.count(src_name)) {
      cout << __func__ << " " << __LINE__ << ": could not find " << src_name->get_str() << " in:\n";
      for (const auto &retreg : retregs) {
        cout << retreg.first->get_str() << endl;
      }
    }
    ASSERT(retregs.count(src_name));
    expr_ref const &dst = retregs.at(src_name);
    ASSERT(src->get_sort() == dst->get_sort() || (src->is_bool_sort() && dst->is_bv_sort() && dst->get_sort()->get_size() == 1));
    if (src->is_array_sort()) {
      ASSERT(src->get_operation_kind() == expr::OP_MEMMASK);
      ret_memlabels.insert(mk_memlabel_ref(src->get_args().at(1)->get_memlabel_value()));
      if (!src_mem) {
        src_mem = src->get_args().at(0);
        dst_mem = dst->get_args().at(0);
      } else {
        ASSERT(src_mem == src->get_args().at(0));
        ASSERT(dst_mem == dst->get_args().at(0));
      }
      ret_mem_name += src_name->get_str();
    } else {
      expr_ref src_bv = src;
      if (src_bv->is_bool_sort()) {
        src_bv = ctx->mk_bool_to_bv(src);
      }
      predicate_ref exit_cond = predicate::mk_predicate_ref(precond_t(), src_bv, dst, string("exit.") + src_name->get_str());
      CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << ": adding exit cond " << exit_cond->to_string(true) << " at " << n->get_pc().to_string() << endl);
      this->add_assert_pred(n->get_pc(), exit_cond);
    }
  }
  if (ret_memlabels.size()) {
    ASSERT(src_mem);
    ASSERT(dst_mem);

    context* ctx = src_mem->get_context();
    memlabel_t ml = memlabel_t::memlabel_union(ret_memlabels);
    expr_ref memeq = ctx->mk_memmasks_are_equal(src_mem, dst_mem, ml);
    predicate_ref exit_cond = predicate::mk_predicate_ref(precond_t(), memeq, expr_true(ctx), string("exit.")+ret_mem_name);
    CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << ": adding exit cond " << exit_cond->to_string(true) << " at " << n->get_pc().to_string() << endl);
    this->add_assert_pred(n->get_pc(), exit_cond);
  }
}

/*void corr_graph::to_dot(string filename, bool embed_info) const
{
  context *ctx = this->get_context();
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();
  state const &src_start_state = src_tfg.get_start_state();
  state const &dst_start_state = dst_tfg.get_start_state();
  ofstream fo;
  fo.open(filename.data());
  fo << "digraph tfg {" << endl;
  list<shared_ptr<corr_graph_edge>> es;
  get_edges(es);
  for(shared_ptr<corr_graph_edge> e : es)
  {
    stringstream label;
    if(embed_info)
    {
      if(e->is_back_edge())
        label << "B" << "\\l";
      else
        label << "F" << "\\l";
      label << "Cond-src: " << ctx->expr_to_string(src_tfg.tfg_edge_composition_get_edge_cond(e->get_src_edge())) << "\\l";
      label << "TF-src: " << src_tfg.tfg_edge_composition_get_to_state(e->get_src_edge(), src_start_state).to_string(this->get_start_state(), "\\l        ");
      label << "\\l";
      label << "Cond-dst: " << ctx->expr_to_string(dst_tfg.tfg_edge_composition_get_edge_cond(e->get_dst_edge())) << "\\l";
      label << "TF-dst: " << dst_tfg.tfg_edge_composition_get_to_state(e->get_dst_edge(), dst_start_state).to_string(this->get_start_state(), "\\l        ");
    }

    string label_str = label.str();
    boost::algorithm::replace_all(label_str, "\n", "\\l        ");

    fo << "\"" << e->get_from_pc().to_string() << "\"" << "->" << "\"" << e->get_to_pc().to_string() << "\"" <<
          "[label=" << "\"" << label_str << "\"" <<
          "fontsize=5 " <<
          "fontname=mono" <<
          "]" << endl;
  }
  fo << "}" << endl;
  fo.close();
}*/

//void
//corr_graph::cg_import_locs(tfg const &tfg, graph_loc_id_t start_loc_id)
//{
//  map<graph_loc_id_t, graph_cp_location> locs = this->get_locs();
//  for (auto loc : tfg.get_locs()) {
//    //cout << __func__ << " " << __LINE__ << ": importing loc at " << loc.first + start_loc_id << ": " << loc.second.to_string() << endl;
//    graph_add_loc(locs, loc.first + start_loc_id, loc.second);
//  }
//  this->set_locs(locs);
//}

//void
//corr_graph::init_graph_locs(graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, bool update_callee_memlabels, map<graph_loc_id_t, graph_cp_location> const &llvm_locs)
//{
//  NOT_REACHED();
//  //ASSERT(update_callee_memlabels);
//  //ASSERT(llvm_locs.size() == 0);
//  //graph_loc_id_t src_max_loc_id = m_eq->get_src_tfg().get_max_loc_id(map<graph_loc_id_t, graph_cp_location>(), this->get_locs());
//  //m_dst_start_loc_id = round_up_to_nearest_power_of_ten(src_max_loc_id);
//  //cg_import_locs(m_eq->get_src_tfg(), 0);
//  //cg_import_locs(this->get_dst_tfg(), m_dst_start_loc_id);
//  //map<graph_loc_id_t, graph_cp_location> locs = this->get_locs();
//  //this->cg_add_locs_for_dst_rodata_symbols(locs);
//  //this->graph_locs_remove_duplicates_mod_comments(locs);
//  //this->set_locs(locs);
//
//  //this->populate_auxilliary_structures_dependent_on_locs();
//}

//void
//corr_graph::cg_add_locs_for_dst_rodata_symbols(map<graph_loc_id_t, graph_cp_location>& locs)
//{
//  context* ctx = this->get_context();
//  tfg const& src_tfg = m_eq->get_src_tfg();
//  state const& src_start_state = src_tfg.get_start_state();
//  string src_memname = src_start_state.get_memname();
//  for (auto const& sym : this->get_symbol_map().get_map()) {
//    if (sym.second.is_dst_const_symbol()) {
//      graph_cp_location loc;
//
//      //size_t sym_size = sym.second.get_size();
//      bool sym_is_const = sym.second.is_const();
//      ASSERT(sym_is_const);
//      loc.m_type = GRAPH_CP_LOCATION_TYPE_MEMMASKED;
//      loc.m_reg_type = reg_type_memory;
//      loc.m_reg_index_i = -1;
//      loc.m_memname = mk_string_ref(src_memname);
//      loc.m_memlabel = mk_memlabel_ref(memlabel_t::memlabel_symbol(sym.first/*, sym_size*/, sym_is_const));
//      loc.m_addr = ctx->mk_zerobv(DWORD_LEN);
//      loc.m_nbytes = -1;
//      graph_loc_id_t loc_id = this->get_max_loc_id(locs, locs) + 1;
//      CPP_DBG_EXEC(TFG_LOCS, cout << __func__ << " " << __LINE__ << ": adding loc at loc_id " << loc_id << ": " << loc.to_string() << endl);
//      this->graph_add_loc(locs, loc_id, loc);
//      m_cg_locs_for_dst_rodata_symbol_masks_on_src_mem.insert(loc_id);
//    }
//  }
//}

//void
//corr_graph::populate_loc_liveness()
//{
//  this->clear_loc_liveness();
//  list<shared_ptr<corr_graph_node>> ns;
//  get_nodes(ns);
//  for (const auto &n : ns) {
//    set<graph_loc_id_t> src_locs, dst_locs;
//    m_eq->get_src_tfg().get_all_loc_ids(src_locs);
//    this->get_dst_tfg().get_all_loc_ids(dst_locs);
//    this->add_live_locs(n->get_pc(), src_locs);
//    this->add_live_locs(n->get_pc(), dst_locs, m_dst_start_loc_id);
//    this->add_live_locs(n->get_pc(), m_cg_locs_for_dst_rodata_symbol_masks_on_src_mem);
//  }
//}

/*
static void
preds_prime_using_local_sprel_expr_guesses(predicate_set_t &pset, local_sprel_expr_guesses_t const &guesses, graph_locals_map_t const &locals_map)
{
  predicate_set_t preds_to_be_removed;
  //bool ret = false;
  //cout << __func__ << " " << __LINE__ << ": ge.first = " << ge.first << ", ge.second = " << expr_string(ge.second) << endl;
  for (auto &pred : pset) {
    //cout << __func__ << " " << __LINE__ << ": pred = " << pred.to_string(true) << endl;
    if (pred.get_local_sprel_expr_assumes().contains_conflicting_local_sprel_expr_guess(guesses, locals_map)) {
      preds_to_be_removed.insert(pred);
    } else {
      //cout << __func__ << " " << __LINE__ << ": before: " << pred.to_string(true) << endl;
      pred.subtract_local_sprel_expr_assumes(guesses);
      //cout << __func__ << " " << __LINE__ << ": after: " << pred.to_string(true) << endl;
    }
  }
  for (auto pred : preds_to_be_removed) {
    pset.erase(pred);
  }
  //cout << __func__ << " " << __LINE__ << ": returning " << (ret ? "true" : "false") << endl;
  //return ret;
}
*/

static set<local_sprel_expr_guesses_t>
preds_get_all_present_local_sprel_expr_guesses(predicate_set_t const &preds, local_sprel_expr_guesses_t const &local_sprel_expr_assumes_required_to_prove, graph_locals_map_t const &locals_map)
{
  set<local_sprel_expr_guesses_t> ret;
  for (auto pred : preds) {
    local_sprel_expr_guesses_t const &ge = pred->get_local_sprel_expr_assumes();
    if (local_sprel_expr_assumes_required_to_prove.contains_conflicting_local_sprel_expr_guess(ge, locals_map)) {
      continue;
    }
    ret.insert(ge);
  }
  return ret;
}

static bool
local_sprel_expr_are_compatible_in_src_dst_expr_syntactic_structure(expr_ref src, expr_ref dst, local_id_t local_id, expr_ref sprel_expr, consts_struct_t const &cs)
{
  map<nextpc_id_t, set<unsigned>> local_argument_pos;      //map from nextpc_id_t to the potential function argument positions in which the local variable occurs
  map<nextpc_id_t, set<unsigned>> sprel_expr_argument_pos; //map from nextpc_id_t to the potential function argument positions in which the local variable occurs
  local_argument_pos = expr_gen_nextpc_farg_pos_map(src, cs.get_expr_value(reg_type_local, local_id));
  sprel_expr_argument_pos = expr_gen_nextpc_farg_pos_map(dst, sprel_expr);
  /*cout << __func__ << " " << __LINE__ << ": src = " << expr_string(src) << endl;
  cout << __func__ << " " << __LINE__ << ": dst = " << expr_string(dst) << endl;
  cout << __func__ << " " << __LINE__ << ": local_id = " << local_id << endl;
  cout << __func__ << " " << __LINE__ << ": sprel_expr = " << expr_string(sprel_expr) << endl;
  cout << __func__ << " " << __LINE__ << ": local_argument_pos =\n";
  for (auto la : local_argument_pos) {
    cout << la.first << " -->";
    for (auto p : la.second) {
      cout << " " << p;
    }
    cout << endl;
  }
  cout << __func__ << " " << __LINE__ << ": sprel_expr_argument_pos =\n";
  for (auto la : sprel_expr_argument_pos) {
    cout << la.first << " -->";
    for (auto p : la.second) {
      cout << " " << p;
    }
    cout << endl;
  }*/
  if (!nextpc_farg_pos_maps_are_compatible(local_argument_pos, sprel_expr_argument_pos)) {
    //cout << __func__ << " " << __LINE__ << ": returning false\n";
    return false;
  }
  //cout << __func__ << " " << __LINE__ << ": returning true\n";
  return true;
}

static set<local_sprel_expr_guesses_t>
gen_potential_guesses_for_src_dst_pair(local_sprel_expr_guesses_t const &all_eq_guesses, expr_ref src, expr_ref dst, local_sprel_expr_guesses_t const &local_sprel_expr_assumes_required_to_prove, consts_struct_t const &cs, set<local_id_t> const &src_locals_in, graph_locals_map_t const &locals_map, tfg const &dst_tfg)
{
  autostop_timer func_timer(__func__);

  set<expr_ref> dst_stack_exprs;
  set<local_id_t> src_locals = src_locals_in;
  expr_identify_local_variables(src, src_locals, cs);
  DYN_DEBUG3(houdini,
    cout << __func__ << " " << __LINE__ << ": src = " << expr_string(src) << ": src_locals (size " << src_locals.size() << "):";
    for (auto src_local : src_locals) {
      cout << " " << src_local;
    }
    cout << endl;
  );

  if (src_locals_in.size() != 0) {
    dst_stack_exprs = dst_tfg.identify_stack_pointer_relative_expressions();
  } else {
    expr_identify_stack_pointer_relative_expressions(dst, dst_stack_exprs, cs);
  }

  DYN_DEBUG3(local_sprel_expr,
    cout << __func__ << " " << __LINE__ << ": dst = " << expr_string(dst) << "\ndst_stack_pointer_relative_exprs (size " << dst_stack_exprs.size() << "):";
    for (auto dst_stack_expr : dst_stack_exprs) {
      cout << " " << expr_string(dst_stack_expr);
    }
    cout << endl;
  );

  vector<pair<local_id_t, expr_ref>> relevant_ge_pairs;
  DYN_DEBUG(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": all_eq_guesses.size() = " << all_eq_guesses.size() << endl);
  DYN_DEBUG2(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": all_eq_guesses: " << all_eq_guesses.to_string() << endl);
  set<pair<local_id_t, expr_ref>> const &guess_exprs = all_eq_guesses.get_local_sprel_expr_pairs();
  for (auto ge : guess_exprs) {
    DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": considering " << ge.first << " vs. " << expr_string(ge.second) << endl);
    if (local_sprel_expr_assumes_required_to_prove.contains_conflicting_local_sprel_expr_guess(ge, locals_map)) {
      DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": contains conflicting local sprel_expr guess\n");
      continue;
    }
    DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": g_correl_locals_try_all_possibilities = " << g_correl_locals_try_all_possibilities << "\n");

    bool src_locals_contain_ge_local = (cs.get_expr_value(reg_type_local, ge.first) == src) || set_belongs(src_locals, ge.first);
    bool dst_stack_exprs_contain_ge_stack_expr = set_belongs(dst_stack_exprs, ge.second);
    bool compatible_with_src_dst = local_sprel_expr_are_compatible_in_src_dst_expr_syntactic_structure(src, dst, ge.first, ge.second, cs);
    DYN_DEBUG3(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": src_locals_contain_ge_local = " << src_locals_contain_ge_local << endl);
    DYN_DEBUG3(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": dst_stack_exprs_contain_ge_stack_expr = " << dst_stack_exprs_contain_ge_stack_expr << endl);
    DYN_DEBUG3(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": compatible_with_src_dst = " << compatible_with_src_dst << endl);
    if (g_correl_locals_try_all_possibilities) {
      relevant_ge_pairs.push_back(ge);
    } else if (   src_locals_contain_ge_local
               && dst_stack_exprs_contain_ge_stack_expr
               && compatible_with_src_dst) {
      relevant_ge_pairs.push_back(ge);
    }
  }

  vector<vector<pair<local_id_t, expr_ref>>> relevant_guesses = get_all_subsets(relevant_ge_pairs);
  set<local_sprel_expr_guesses_t> ret;
  for (auto rg : relevant_guesses) {
    local_sprel_expr_guesses_t g(rg);
    if (g.is_incompatible(locals_map)) {
      continue;
    }
    ret.insert(g);
  }
  DYN_DEBUG(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl);
  return ret;
}

set<local_sprel_expr_guesses_t>
corr_graph::generate_local_sprel_expr_guesses(pcpair const &pp, predicate_set_t const &lhs, edge_guard_t const &guard, local_sprel_expr_guesses_t const &local_sprel_expr_assumes_required_to_prove, expr_ref src_in, expr_ref dst_in, set<local_id_t> const &src_locals) const
{
  autostop_timer func_timer(__func__);
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();
  context *ctx = this->get_context();
  consts_struct_t const &cs = this->get_consts_struct();
  graph_locals_map_t const &locals_map = src_tfg.get_locals_map();
  local_sprel_expr_guesses_t const &all_eq_guesses = m_eq->get_local_sprel_expr_guesses();
  DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": all_eq_guesses.size() = " << all_eq_guesses.size() << endl);

  expr_ref src = src_tfg.expr_simplify_at_pc(src_in, pp.get_first());
  expr_ref dst = dst_tfg.expr_simplify_at_pc(dst_in, pp.get_second());

  set<local_sprel_expr_guesses_t> new_guesses = gen_potential_guesses_for_src_dst_pair(all_eq_guesses, src, dst, local_sprel_expr_assumes_required_to_prove, cs, src_locals, locals_map, dst_tfg);

  DYN_DEBUG2(local_sprel_expr,
      //cout << __func__ << " " << __LINE__ << ": src = " << expr_string(src) << endl;
      //cout << __func__ << " " << __LINE__ << ": dst = " << expr_string(dst) << endl;
      cout << __func__ << " " << __LINE__ << ": new_guesses.size() = " << new_guesses.size() << endl;
      for (auto g : new_guesses) {
        cout << __func__ << " " << __LINE__ << ": " << g.to_string() << endl;
      }
  );

  set<local_sprel_expr_guesses_t> existing_assumes = preds_get_all_present_local_sprel_expr_guesses(lhs, local_sprel_expr_assumes_required_to_prove, locals_map);
  DYN_DEBUG3(houdini,
      cout << __func__ << " " << __LINE__ << ": existing_assumes.size() = " << existing_assumes.size() << endl;
      for (auto g : existing_assumes) {
        cout << __func__ << " " << __LINE__ << ": " << g.to_string() << endl;
      }
  );

  set<local_sprel_expr_guesses_t> all_guesses = existing_assumes;
  set_union(all_guesses, new_guesses);

  local_sprel_expr_guesses_t empty;
  all_guesses.erase(empty);
  set<local_sprel_expr_guesses_t> all_guesses_compatible;

  for (auto g : all_guesses) {
    if (g.is_incompatible(locals_map)) {
      DYN_DEBUG(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": guesses " << g.to_string() << " are incompatible. ignoring ..." << endl);
      continue;
    }
    g.set_union(local_sprel_expr_assumes_required_to_prove);
    all_guesses_compatible.insert(g);
  }

  DYN_DEBUG(local_sprel_expr, cout << __func__ << " " << __LINE__ << ": all_guesses_compatible.size() = " << all_guesses_compatible.size() << endl);
  //if (all_guesses_compatible.size() > 16) {
  //  cout << __func__ << " " << __LINE__ << ": all_guesses.size() = " << all_guesses_compatible.size() << endl;
  //}
  return all_guesses_compatible;
}

bool
corr_graph::cg_correlation_exists(corr_graph_edge_ref const& new_cg_edge) const
{
  list<corr_graph_edge_ref> all_edges;
  this->get_edges(all_edges);
  pcpair const& to_pc = new_cg_edge->get_to_pc();
  for(auto const& e : all_edges)
  {
    if (e == new_cg_edge)
      continue;
    if(e->get_to_pc() == to_pc)
      return true;
  }
  return false;
}

bool
corr_graph::cg_correlation_exists_to_another_src_pc(corr_graph_edge_ref const& new_cg_edge) const
{
  if (cg_correlation_exists(new_cg_edge)) {
    return false;
  }
  list<corr_graph_edge_ref> all_edges;
  this->get_edges(all_edges);
  pcpair const& to_pc = new_cg_edge->get_to_pc();
  for(auto const& e : all_edges)
  {
    if (e == new_cg_edge)
      continue;
    if(e->get_to_pc().get_second() == to_pc.get_second() && e->get_to_pc().get_first() != to_pc.get_first())
      return true;
  }
  return false;
}

shared_ptr<corr_graph>
corr_graph::cg_copy_corr_graph() const
{
  autostop_timer ft(__func__);
  //cout << __func__ << " " << __LINE__ << ": this graph_with_guessing_invariants_to_string =\n" << graph_with_guessing_invariants_to_string() << endl;
  shared_ptr<corr_graph> cg_new = make_shared<corr_graph>(*this);
  //cout << __func__ << " " << __LINE__ << ": cg_new graph_with_guessing_invariants_to_string =\n" << cg_new->graph_with_guessing_invariants_to_string() << endl;
  return cg_new;
}

bool
corr_graph::src_path_contains_already_correlated_node_as_intermediate_node(shared_ptr<tfg_edge_composition_t> const &src_path, pc const &dst_pc) const
{
  list<shared_ptr<corr_graph_node>> cur_correlated_nodes;
  set<pc> cur_correlated_src_pcs;
  cur_correlated_nodes = this->find_cg_nodes_with_dst_node(dst_pc);
  for (auto cur_correlated_node: cur_correlated_nodes) {
    cur_correlated_src_pcs.insert(cur_correlated_node->get_pc().get_first());
  }
  list<shared_ptr<tfg_edge const>> const &constituent_edges = src_path->get_constituent_atoms();
  for (auto const &e : constituent_edges) {
    if (set_belongs(cur_correlated_src_pcs, e->get_to_pc())) {
      return true;
    }
  }
  return false;
}

void
corr_graph::cg_print_stats() const
{
  consts_struct_t const &cs = this->get_consts_struct();
  local_sprel_expr_guesses_t local_sprel_expr_assumes_required_at_exit;
  cout << "Printing proof stats:\n";
  size_t num_internal_nodes = 0;
  size_t num_exit_nodes = 0;
  size_t num_provable_preds_internal = 0;
  size_t num_provable_preds_internal_guarded = 0;
  size_t num_provable_preds_exit = 0;
  size_t num_provable_preds_exit_guarded = 0;
  //for (auto p : this->get_all_pcs()) {
  //  if (p.is_start()) {
  //    continue;
  //  }
  //  predicate_set_t preds;
  //  this->get_preds_not_unproven(p, preds);
  //  size_t num_provable = 0;
  //  size_t num_provable_requiring_local_sprel_expr_assumes = 0;
  //  /*for (const auto &pred : preds) {
  //    if (   (!p.is_exit() || pred.get_comment() == "exit")
  //        && pred.get_proof_status() == predicate::provable) {
  //      num_provable++;
  //      if (!pred.get_local_sprel_expr_assumes().is_empty()) {
  //        num_provable_requiring_local_sprel_expr_assumes++;
  //        if (pred.get_comment() == "exit") {
  //          local_sprel_expr_assumes_required_at_exit.set_union(pred.get_local_sprel_expr_assumes());
  //        }
  //      }
  //    }
  //  }*/
  //  if (!p.is_exit() && !p.is_start()) {
  //    num_internal_nodes++;
  //    num_provable_preds_internal += num_provable;
  //    num_provable_preds_internal_guarded += num_provable_requiring_local_sprel_expr_assumes;
  //  } else if (p.is_exit()) {
  //    num_exit_nodes++;
  //    num_provable_preds_exit += num_provable;
  //    num_provable_preds_exit_guarded += num_provable_requiring_local_sprel_expr_assumes;
  //  }
  //  cout << "pc " << p.to_string() << ": num_provable " << num_provable << ", num_provable_requiring_local_sprel_expr_assumes " << num_provable_requiring_local_sprel_expr_assumes << endl;
  //}
  local_sprel_expr_guesses_t const &correlation_local_sprel_guesses = this->get_local_sprel_expr_assumes();
  cout << "correlation_local_sprel_guesses.size: " << correlation_local_sprel_guesses.size() << endl;
  cout << "local_sprel_expr_assumes_required_at_exit:\n" << local_sprel_expr_assumes_required_at_exit.to_string_for_eq();
  cout << "local_sprel_expr_assumes_required_at_exit size: " << local_sprel_expr_assumes_required_at_exit.size() << endl;
  cout << "num_nodes: " << this->get_all_pcs().size() << endl;
  cout << "num_internal_nodes: " << num_internal_nodes << endl;
  cout << "num_provable_preds_internal: " << num_provable_preds_internal << endl;
  cout << "num_provable_preds_internal_guarded: " << num_provable_preds_internal_guarded << endl;
  cout << "num_exit_nodes: " << num_exit_nodes << endl;
  cout << "num_provable_preds_exit: " << num_provable_preds_exit << endl;
  cout << "num_provable_preds_exit_guarded: " << num_provable_preds_exit_guarded << endl;
}

bool
corr_graph::need_stability_at_pc(pcpair const &p) const
{
  if (p.is_start() || p.is_exit())
    return false;

  //allow reordering of function calls that do not have side effects, by relaxing stability requirements on those nodes
  if (!p.get_first().is_fcall()) {
    return true;
  }
  tfg const &src_tfg = this->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();
  expr_ref npc_src = src_tfg.get_incident_fcall_nextpc(p.get_first());
  if (npc_src && !npc_src->is_nextpc_const()) {
    //cout << __func__ << " " << __LINE__ << ": npc_src = " << expr_string(npc_src) << "\n";
    return true;
  }
  expr_ref npc_dst = dst_tfg.get_incident_fcall_nextpc(p.get_second());
  if (npc_dst && !npc_dst->is_nextpc_const()) {
    //cout << __func__ << " " << __LINE__ << ": npc_dst = " << expr_string(npc_dst) << "\n";
    return true;
  }
  if (!npc_src && !npc_dst) {
    return false;
  }
  if (!npc_src || !npc_dst) {
    return true;
  }
  ASSERT(npc_src && npc_dst);

  memlabel_t ml_writeable = dst_tfg.get_incident_fcall_memlabel_writeable(p.get_second());
  if (   !memlabel_t::memlabel_contains_heap(&ml_writeable)
      && !memlabel_t::memlabel_contains_symbol(&ml_writeable)) {
    return false;
  }
  /*
  unsigned src_nextpc_num = npc_src->get_nextpc_num();
  unsigned dst_nextpc_num = npc_dst->get_nextpc_num();
  if (   !src_tfg.fcall_nextpc_writes_heap_or_symbol(src_nextpc_num)
      && !src_tfg.fcall_nextpc_writes_heap_or_symbol(dst_nextpc_num)) {
    //cout << __func__ << " " << __LINE__ << ":\n";
    return false;
  }
  //cout << __func__ << " " << __LINE__ << ":\n";
  */
  return true;
}

static shared_ptr<tfg_edge_composition_t>
cg_edge_composition_to_tfg_edge_composite(shared_ptr<cg_edge_composition_t> const &ec/*, state const &tfg_start_state*/, std::function<shared_ptr<tfg_edge_composition_t> (corr_graph_edge_ref const &)> f)
{
  if (ec == nullptr) {
    return nullptr;
  }
  std::function<shared_ptr<tfg_edge_composition_t> (cg_edge_composition_t::type t, shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b)> fold_tfg_ec_ops = [](cg_edge_composition_t::type t, shared_ptr<tfg_edge_composition_t> const &a, shared_ptr<tfg_edge_composition_t> const &b)
  {
    if (t == cg_edge_composition_t::series) {
      ASSERT(a->graph_edge_composition_get_to_pc() == b->graph_edge_composition_get_from_pc());
      shared_ptr<tfg_edge_composition_t> ret = mk_series(a, b);
      return ret;
    } else if (t == cg_edge_composition_t::parallel) {
      ASSERT(a->graph_edge_composition_get_from_pc() == b->graph_edge_composition_get_from_pc());
      ASSERT(a->graph_edge_composition_get_to_pc() == b->graph_edge_composition_get_to_pc());
      shared_ptr<tfg_edge_composition_t> ret = mk_parallel(a, b);
      return ret;
    } else NOT_REACHED();
  };

  return ec->visit_nodes(f, fold_tfg_ec_ops);
}

shared_ptr<tfg_edge_composition_t>
cg_edge_composition_to_src_edge_composite(corr_graph const &cg, shared_ptr<cg_edge_composition_t> const &ec, tfg const &src_tfg/*, state const &src_start_state*/)
{
  std::function<shared_ptr<tfg_edge_composition_t> (corr_graph_edge_ref const &)> f = [&src_tfg](corr_graph_edge_ref const& e)
  {
    ASSERT(e);
    shared_ptr<tfg_edge_composition_t> ret = e->get_src_edge();
    graph<pc, tfg_node, tfg_edge> const &src_graph = (graph<pc, tfg_node, tfg_edge> const &)src_tfg;
    return ret;
  };
  return cg_edge_composition_to_tfg_edge_composite(ec/*, src_start_state*/, f);
}

shared_ptr<tfg_edge_composition_t>
cg_edge_composition_to_dst_edge_composite(corr_graph const &cg, shared_ptr<cg_edge_composition_t> const &ec, tfg const &dst_tfg/*, state const &dst_start_state*/)
{
  std::function<shared_ptr<tfg_edge_composition_t> (corr_graph_edge_ref const &)> f = [](corr_graph_edge_ref const &e)
  {
    shared_ptr<tfg_edge_composition_t> ret = e->get_dst_edge();
    return ret;
  };
  return cg_edge_composition_to_tfg_edge_composite(ec/*, dst_start_state*/, f);
}

list<pair<expr_ref, predicate_ref>>
corr_graph::edge_composition_apply_trans_funs_on_pred(predicate_ref const &p, shared_ptr<cg_edge_composition_t> const &ec) const
{
  autostop_timer func_timer(__func__);
  list<pair<expr_ref, predicate_ref>> ret;
  context *ctx = get_context();
  ASSERT(ec);
  if (ec->is_epsilon()) {
    ret.push_back(make_pair(expr_true(ctx), p));
    return ret;
  }

  tfg const &src_tfg = get_eq()->get_src_tfg();
  tfg const &dst_tfg = get_dst_tfg();
  state const &src_start_state = src_tfg.get_start_state();
  state const &dst_start_state = dst_tfg.get_start_state();

  //cout << __func__ << " " << __LINE__ << ": src_tfg = " << &src_tfg << endl;
  //cout << __func__ << " " << __LINE__ << ": dst_tfg = " << &dst_tfg << endl;

  shared_ptr<tfg_edge_composition_t> src_ec = cg_edge_composition_to_src_edge_composite(*this, ec, src_tfg/*, src_start_state*/);
  shared_ptr<tfg_edge_composition_t> dst_ec = cg_edge_composition_to_dst_edge_composite(*this, ec, dst_tfg/*, dst_start_state*/);

  CPP_DBG_EXEC(STATS,
      auto path_edge_count = get_path_and_edge_counts_for_edge_composition(src_ec);
      int path_count = get<0>(path_edge_count);
      DYN_DEBUG(counters_enable, stats::get().add_counter("query-paths",path_count))
  );

  list<pair<edge_guard_t, predicate_ref>> preds_apply_src = src_tfg.apply_trans_funs(src_ec, p, true);

  DYN_DEBUG2(weakest_precondition,
      cout << __func__ << " " << __LINE__ << ": after applying pred " << p->to_string(true) << " to src_ec " << src_ec->graph_edge_composition_to_string() << ", got " << preds_apply_src.size() << " predicates:" << endl;
      size_t i = 0;
      for (auto eps : preds_apply_src) {
        cout << i << ". " << eps.first.edge_guard_to_string() << ": " << eps.second->to_string(true) << endl;
        i++;
      }
  );

  for (auto eps : preds_apply_src) {
    predicate_ref ps = eps.second;
    edge_guard_t eg = eps.first;
    edge_guard_t psg = ps->get_edge_guard();
    //cout << __func__ << " " << __LINE__ << ": eg = " << eg.edge_guard_to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": psg = " << psg.edge_guard_to_string() << endl;
    CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << ": ps = " << ps->to_string(true) << endl);
    CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << ": ec = " << ec->graph_edge_composition_to_string() << endl);
    eg.add_guard_in_series(psg/*, src_tfg*/);
    ps = ps->set_edge_guard(eg);
    list<pair<edge_guard_t, predicate_ref>> preds_apply_dst = dst_tfg.apply_trans_funs(dst_ec/*, cg.get_dst_fcall_edge_overrides()*/, ps, true);

    DYN_DEBUG2(weakest_precondition,
        cout << __func__ << " " << __LINE__ << ": after applying pred " << ps->to_string(true) << " to dst_ec " << dst_ec->graph_edge_composition_to_string() << ", got " << preds_apply_dst.size() << " predicates:" << endl;
        size_t i = 0;
        for (auto eps : preds_apply_dst) {
          cout << i << ". " << eps.first.edge_guard_to_string() << ": " << eps.second->to_string(true) << endl;
          i++;
        }
    );

    for (auto psd : preds_apply_dst) {
      ret.push_back(make_pair(psd.first.edge_guard_get_expr(dst_tfg, true), psd.second));
    }
  }
  return ret;
}

void
corr_graph::update_dst_fcall_edge_using_src_fcall_edge(pc const &src_pc, pc const &dst_pc)
{
  //update dst edge
  //cout << __func__ << " " << __LINE__ << ": cg->locs.size() = " << this->get_locs().size() << endl;
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg &dst_tfg = this->get_dst_tfg();
  state const &src_start_state = src_tfg.get_start_state();
  state const &dst_start_state = dst_tfg.get_start_state();
  list<shared_ptr<tfg_edge const>> src_outgoing, dst_outgoing;
  context *ctx = src_tfg.get_context();
  int dst_stackpointer_gpr = m_eq->get_stackpointer_gpr();
  string const dst_stackpointer_key = string(G_DST_KEYWORD) + "." G_REGULAR_REG_NAME + boost::lexical_cast<string>(DST_EXREG_GROUP_GPRS) + "." + boost::lexical_cast<string>(dst_stackpointer_gpr);
  expr_ref stackpointer = dst_start_state.get_expr(dst_stackpointer_key/*, dst_start_state*/);

  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": src_pc = " << src_pc.to_string() << endl);
  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": dst_pc = " << dst_pc.to_string() << endl);

  //ASSERT(src_pc.get_subsubindex() == PC_SUBSUBINDEX_FCALL_START);
  //ASSERT(dst_pc.get_subsubindex() == PC_SUBSUBINDEX_FCALL_START);
  src_tfg.get_edges_outgoing(src_pc, src_outgoing);
  dst_tfg.get_edges_outgoing(dst_pc, dst_outgoing);
  ASSERT(src_outgoing.size() == 1);
  ASSERT(dst_outgoing.size() == 1);
  shared_ptr<tfg_edge const> const &src_edge = *src_outgoing.begin();
  shared_ptr<tfg_edge const> const &dst_edge = *dst_outgoing.begin();
  ASSERT(src_edge->get_cond() == expr_true(ctx));
  ASSERT(dst_edge->get_cond() == expr_true(ctx));
  expr_ref src_nextpc = src_edge->get_function_call_nextpc();
  expr_ref dst_nextpc = dst_edge->get_function_call_nextpc();

  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": src_nextpc = " << expr_string(src_nextpc) << endl);
  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": dst_nextpc = " << expr_string(dst_nextpc) << endl);

  tuple<nextpc_id_t, vector<farg_t>, mlvarname_t, mlvarname_t> t_src = src_edge->get_to_state().get_nextpc_args_memlabel_map_for_edge(src_start_state, ctx);
  vector<farg_t> src_retvals = src_edge->get_to_state().state_get_fcall_retvals();
  //graph_memlabel_map_t &dst_memlabel_map = dst_tfg.get_memlabel_map_ref();
  fsignature_t src_fsignature(get<1>(t_src), src_retvals);
  string_ref src_mlvarname_readable = get<2>(t_src);
  string_ref src_mlvarname_writeable = get<3>(t_src);

  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": src_mlvarname_readable = " << src_mlvarname_readable->get_str() << endl);
  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": src_mlvarname_writeable = " << src_mlvarname_writeable->get_str() << endl);

  memlabel_t src_ml_readable = src_tfg.get_memlabel_map().at(src_mlvarname_readable)->get_ml();
  memlabel_t src_ml_writeable = src_tfg.get_memlabel_map().at(src_mlvarname_writeable)->get_ml();

  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": src_ml_readable = " << src_ml_readable.to_string() << endl);
  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": src_ml_writeable = " << src_ml_writeable.to_string() << endl);

  tuple<nextpc_id_t, vector<farg_t>, mlvarname_t, mlvarname_t> t_dst = dst_edge->get_to_state().get_nextpc_args_memlabel_map_for_edge(dst_start_state, ctx);
  string_ref dst_mlvarname_readable = get<2>(t_dst);
  string_ref dst_mlvarname_writeable = get<3>(t_dst);

  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": dst_mlvarname_readable = " << dst_mlvarname_readable->get_str() << endl);
  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": dst_mlvarname_writeable = " << dst_mlvarname_writeable->get_str() << endl);

  graph_memlabel_map_t &dst_memlabel_map = dst_tfg.get_memlabel_map_ref();
  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": old dst_ml_readable = " << dst_memlabel_map.at(dst_mlvarname_readable)->get_ml().to_string() << endl);
  DYN_DEBUG(update_dst_fcall_edge, cout << __func__ << " " << __LINE__ << ": old dst_ml_writeable = " << dst_memlabel_map.at(dst_mlvarname_writeable)->get_ml().to_string() << endl);

  dst_memlabel_map[dst_mlvarname_readable] = mk_memlabel_ref(src_ml_readable);
  dst_memlabel_map[dst_mlvarname_writeable] = mk_memlabel_ref(src_ml_writeable);


  long &dst_max_memlabel_varnum = dst_tfg.get_max_memlabel_varnum_ref();
  //cout << __func__ << " " << __LINE__ << ": dst_max_memlabel_varnum = " << dst_max_memlabel_varnum << endl;
  string const& dst_tfgname = dst_tfg.get_name()->get_str();
  //ASSERT(dst_edge->tfg_edge_is_atomic());
  std::function<void (pc const&, state&)> f_update_state = [&src_fsignature, &dst_stackpointer_key, &stackpointer, &dst_start_state, ctx, &dst_tfgname, &dst_max_memlabel_varnum, &dst_edge](pc const& p, state &st)
  {
    ASSERT(p == dst_edge->get_from_pc());
    st = st.add_nextpc_args_and_retvals_for_edge(src_fsignature, dst_stackpointer_key, stackpointer, dst_start_state, ctx, dst_tfgname, dst_max_memlabel_varnum);
  };
  shared_ptr<tfg_edge const> new_dst_edge = dst_edge->tfg_edge_update_state(f_update_state);
  dst_tfg.remove_edge(dst_edge);
  dst_tfg.add_edge(new_dst_edge);

  //redo alias analysis on dst tfg
  //dst_tfg.clear_ismemlabel_assumes();

  //cout << __func__ << " " << __LINE__ << ": cg->locs.size() = " << this->get_locs().size() << endl;
  dst_tfg.split_memory_in_graph_and_propagate_sprels_until_fixpoint(false, src_tfg.get_locs());

  this->dst_fcall_edge_mark_updated(dst_pc);

  this->populate_simplified_assert_map(this->get_all_pcs());
  //the following two lines are not strictly necessary, but good to have, I think. However, if we add locs, we must also populate aliasing info (lr_map) for them. For now, commenting this out
  //this->cg_import_locs(dst_tfg, m_dst_start_loc_id);
  //this->populate_auxilliary_structures_dependent_on_locs();

  //cout << __func__ << " " << __LINE__ << ": cg->locs.size() = " << this->get_locs().size() << endl;
  DYN_DEBUG(update_dst_fcall_edge,
      string const& proof_file = m_eq->get_proof_filename();
      if (proof_file != "") {
        string dst_tfg_file = proof_file;
        stringstream ss;
        ss << "dst_tfg-" << dst_pc.get_index();
        string new_ext = ss.str();
        boost::filesystem::path dst_tfg_path(dst_tfg_file);
        if (dst_tfg_path.has_extension()) {
          dst_tfg_path.replace_extension(new_ext);
          dst_tfg_file = dst_tfg_path.native();
        } else {
          dst_tfg_file += string(".") + new_ext;
        }
        tfg::generate_dst_tfg_file(dst_tfg_file, m_eq->get_function_name(), m_eq->get_rodata_map(), dst_tfg, m_eq->get_dst_iseq(), m_eq->get_dst_insn_pcs());
        cout << __func__ << " " << __LINE__ << ": outputted dst_tfg to " << dst_tfg_file << endl;
      }
      cout << __func__ << " " << __LINE__ << ": after: dst_tfg =\n" << dst_tfg.graph_to_string() << endl;
  );
}

/*map<shared_ptr<tfg_edge>, state> const &
corr_graph::get_dst_fcall_edge_overrides() const
{
  return m_dst_fcall_edge_overrides;
}*/
callee_summary_t
corr_graph::get_callee_summary_for_nextpc(nextpc_id_t nextpc_num, size_t num_fargs) const
{
  tfg const &dst_tfg = this->get_dst_tfg();
  return dst_tfg.get_callee_summary_for_nextpc(nextpc_num, num_fargs);
}

bool
corr_graph::propagate_sprels()
{
  return false;
}

sprel_map_pair_t
corr_graph::get_sprel_map_pair(pcpair const &pp) const
{
  autostop_timer timer(__func__);
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();
  sprel_map_t const &src_sprel_map = src_tfg.get_sprel_map(pp.get_first());
  sprel_map_t const &dst_sprel_map = dst_tfg.get_sprel_map(pp.get_second());

  sprel_map_pair_t ret(src_sprel_map, dst_sprel_map);
  /* Q: why compute this over-and-over again? Why not compute in constructor and save as member? */
  /* A: Because for dst tfg we re-compute sprels in update_dst_fcall_edge_using_src_fcall_edge */
  return ret;
}

void 
corr_graph::generate_new_CE(pcpair const& cg_from_pc, shared_ptr<tfg_edge_composition_t> const& dst_ec) const
{
  autostop_timer func_timer(__func__);
  tfg const &dst_tfg = this->get_dst_tfg();
  context* ctx = this->get_context();

  //expr_ref dst_edge_cond = dst_tfg.tfg_edge_composition_get_edge_cond(dst_ec, true);
  expr_ref const& dst_edge_cond = dst_tfg.tfg_edge_composition_get_edge_cond(dst_ec, /*simplified*/true);
  ASSERT(dst_edge_cond->is_bool_sort());
  CPP_DBG_EXEC(CORRELATE, cout << __func__<< " dst_ec " << dst_ec->graph_edge_composition_to_string() << " cond:  " << ctx->expr_to_string(dst_edge_cond) << "\n");

  aliasing_constraints_t const& dst_edge_alias_cond = dst_tfg.collect_aliasing_constraints_around_path(cg_from_pc.get_second(), dst_ec);
  predicate_set_t dst_edge_cond_alias_pred = dst_edge_alias_cond.get_ismemlabel_preds();
  // cannot add src assumes as a src path is not known
  predicate_ref pred = predicate::mk_predicate_ref(precond_t(), dst_edge_cond, expr_false(ctx), __func__);
  bool guess_made_weaker;
  
  proof_status_t proof_status = this->decide_hoare_triple(dst_edge_cond_alias_pred, cg_from_pc, mk_epsilon_ec<pcpair,corr_graph_edge>(), pred, guess_made_weaker);
}

bool
corr_graph::dst_edge_composition_proves_false(pcpair const &cg_from_pc, shared_ptr<tfg_edge_composition_t> const &dst_ec) const
{
  autostop_timer func_timer(__func__);
  tfg const &dst_tfg = this->get_dst_tfg();
  context* ctx = this->get_context();

  expr_ref const& dst_edge_cond = dst_tfg.tfg_edge_composition_get_edge_cond(dst_ec, /*simplified*/true);
  ASSERT(dst_edge_cond->is_bool_sort());
  DYN_DEBUG2(correlate, cout << __func__<< " dst_ec " << dst_ec->graph_edge_composition_to_string() << " cond:  " << ctx->expr_to_string(dst_edge_cond) << "\n");

  if (dst_edge_cond->is_const()) {
    return !dst_edge_cond->get_bool_value();
  }
  aliasing_constraints_t const& dst_edge_alias_cond = dst_tfg.collect_aliasing_constraints_around_path(cg_from_pc.get_second(), dst_ec);
  predicate_set_t dst_edge_cond_alias_pred = dst_edge_alias_cond.get_ismemlabel_preds();
  // cannot add src assumes as a src path is not known
  predicate_ref pred = predicate::mk_predicate_ref(precond_t(), dst_edge_cond, expr_false(ctx), __func__);
  bool guess_made_weaker;
  proof_status_t proof_status = this->decide_hoare_triple(dst_edge_cond_alias_pred, cg_from_pc, mk_epsilon_ec<pcpair,corr_graph_edge>(), pred, guess_made_weaker);
  if (proof_status == proof_status_timeout) {
    NOT_IMPLEMENTED();
  }
  /*if (proves_false) {
    -    cout << __func__ << " " << __LINE__ << ": " << get_timestamp(tbuf1, sizeof tbuf1) << ": ignoring chosen correlation because dst_edge_proves_false:\n" << this->cg_to_string(false) << endl;
    -    cout << "pcpair: " << cg_from_pc.to_string() << endl;
    -    cout << "dst_edge: " << dst_edge->to_string(NULL) << endl << endl;
    -  }*/
  //ASSERT(guess_made_weaker == (pred.get_proof_status() == predicate::not_provable)); //there is no chance of weakening due to local sprel expr guesses here
  return (proof_status == proof_status_proven);
}

bool
corr_graph::cg_covers_dst_edge_at_pcpair(pcpair const &pp, shared_ptr<tfg_edge const> const &dst_e, tfg const &dst_tfg) const
{
  list<corr_graph_edge_ref> cg_outgoing;
  this->get_edges_outgoing(pp, cg_outgoing);
  for (const auto &e : cg_outgoing) {
    auto cg_dst_edge = e->get_dst_edge();
    if (   cg_dst_edge->graph_edge_composition_get_from_pc() == dst_e->get_from_pc()
        && cg_dst_edge->graph_edge_composition_get_to_pc() == dst_e->get_to_pc()) {
      return true;
    }
  }
  return false;
}

bool
corr_graph::cg_covers_all_dst_tfg_paths(tfg const &dst_tfg) const
{
  set<pcpair> pps = this->get_all_pcs();
  for (const auto &pp : pps) {
    pc const &dst_pc = pp.get_second();
    list<shared_ptr<tfg_edge const>> outgoing;
    dst_tfg.get_edges_outgoing(dst_pc, outgoing);
    for (const auto &dst_e : outgoing) {
      if (!this->cg_covers_dst_edge_at_pcpair(pp, dst_e, dst_tfg)) {
        return false;
      }
    }
  }
  return true;
}


expr_group_ref
corr_graph::compute_mem_eqclass(pcpair const& pp, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const
{
  pair<expr_ref, expr_ref> const& src_dst_mem = compute_mem_eq_exprs(pp, src_interesting_exprs, dst_interesting_exprs);
  expr_ref const& src = src_dst_mem.first;
  expr_ref const& dst = src_dst_mem.second;
  if(src && dst)
    return mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_ARR_EQ, { src, dst });
  else 
    return nullptr;
}

static bool
memmasks_are_compatible(context* ctx, expr_ref const& src, expr_ref const& dst)
{
  if (   src->get_operation_kind() != expr::OP_MEMMASK
      || dst->get_operation_kind() != expr::OP_MEMMASK)
    return false;

  memlabel_t const& dst_ml = dst->get_args().at(1)->get_memlabel_value();
  ASSERT(memlabel_t::memlabel_is_atomic(&dst_ml));
  expr_ref tmp = ctx->mk_memmask(src, dst_ml);
  tmp = ctx->expr_do_simplify(tmp);
  if (tmp != src) {
    DYN_DEBUG3(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": ignoring because masking src with dst's mask does not yield the original src. ml = " << dst_ml.to_string() << ", src = " << expr_string(src) << ", masked src = " << expr_string(tmp) << endl);
    return false;
  }
  return true;
}

pair<expr_ref, expr_ref>
corr_graph::compute_mem_eq_exprs(pcpair const& pp, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const
{
  context* ctx = this->get_context();
  tfg const &src_tfg = this->get_src_tfg();

  set<memlabel_ref> interesting_memlabels;
  expr_ref src_mem, dst_mem;
  // collect src and dst memories and corresponding memlabels
  for (auto const& dst_expr : dst_interesting_exprs) {
    if (   dst_expr->get_operation_kind() == expr::OP_MEMMASK
        && !dst_expr->get_args().at(1)->get_memlabel_value().memlabel_is_stack()
        && !dst_expr->get_args().at(1)->get_memlabel_value().memlabel_is_local()
        && dst_expr->get_args().at(1)->get_memlabel_value().memlabel_is_arg() == -1) {
      // find matching src memmask
      expr_ref const& dst_memmask = dst_expr;
      memlabel_t ml = dst_memmask->get_args().at(1)->get_memlabel_value();
      auto src_itr = find_if(src_interesting_exprs.begin(), src_interesting_exprs.end(),
          [ctx,dst_memmask](expr_ref const& pe) { return memmasks_are_compatible(ctx, pe, dst_memmask); });
      if (src_itr == src_interesting_exprs.end()) {
        continue; //this can happen for rodata symbols appearing only in dst
      }
      ASSERT(src_itr != src_interesting_exprs.end());

      expr_ref const& src_memmask = (*src_itr);
      ASSERT(src_memmask->get_operation_kind() == expr::OP_MEMMASK);
      ASSERT(ml == src_memmask->get_args().at(1)->get_memlabel_value());
      interesting_memlabels.insert(mk_memlabel_ref(ml));
      if (!src_mem) {
        src_mem = src_memmask->get_args().at(0);
        dst_mem = dst_memmask->get_args().at(0);
      } else {
        ASSERT(src_mem == src_memmask->get_args().at(0));
        ASSERT(dst_mem == dst_memmask->get_args().at(0));
      }
    }
  }

  // create memories with composite "non-stack" memlabels
  pair<expr_ref, expr_ref> ret;
  if (interesting_memlabels.size()) {
    ASSERT(src_mem);
    ASSERT(dst_mem);

    memlabel_t ml = memlabel_t::memlabel_union(interesting_memlabels);
    expr_ref src = ctx->mk_memmask_composite(src_mem, ml);
    expr_ref dst = ctx->mk_memmask_composite(dst_mem, ml);
    return make_pair(src, dst);
  } else {
    return ret;
  }
}

static void
get_inequality_exprs_for_expr_pair(expr_ref const& e1, expr_ref const& e2, vector<expr_ref> &out)
{
  if (   !e1->is_bv_sort()
      || e1->get_sort() != e2->get_sort())
    return;

  context* ctx = e1->get_context();

  expr_ref const& e1_e2_s  = ctx->mk_bvslt(e1,e2);
  expr_ref const& e1_e2_us = ctx->mk_bvult(e1,e2);
  expr_ref const& e2_e1_s  = ctx->mk_bvslt(e2,e1);
  expr_ref const& e2_e1_us = ctx->mk_bvult(e2,e1);
  expr_ref const& e1_e2_s1  = ctx->mk_bvsle(e1,e2);
  expr_ref const& e1_e2_us1 = ctx->mk_bvule(e1,e2);
  expr_ref const& e2_e1_s1  = ctx->mk_bvsle(e2,e1);
  expr_ref const& e2_e1_us1 = ctx->mk_bvule(e2,e1);

  out.push_back(e1_e2_s );
  out.push_back(e1_e2_us);
  out.push_back(e2_e1_s );
  out.push_back(e2_e1_us);

  out.push_back(e1_e2_s1 );
  out.push_back(e1_e2_us1);
  out.push_back(e2_e1_s1 );
  out.push_back(e2_e1_us1);
}

static void
get_sign_bit_exprs(context* ctx, set<expr_ref> const &exprs, vector<expr_ref> &sign_bit_exprs)
{
  for (auto const& e: exprs) {
    if (!e->is_bv_sort())
      continue;
    ASSERT(e->is_bv_sort());
    unsigned msb = e->get_sort()->get_size() -1;
    sign_bit_exprs.push_back(ctx->mk_bvextract(e,msb,msb));
  }
}


bool
corr_graph::expr_pred_def_is_likely_nearer(pcpair const &pp, expr_ref const &a, expr_ref const &b) const
{
  //the idea of this function is that dependent values occur after the values on which they depend; for now we use some simple heuristics, e.g., dst > src, higher numbered llvm vars > lower numbered llvm vars, larger llvm strings > smaller llvm string (e.g., input.src.llvm-%mul2 > input.src.llvm-%mul)
  string astr = expr_string(a);
  string bstr = expr_string(b);
  bool a_is_dst = astr.find("input.dst") != string::npos;
  bool b_is_dst = bstr.find("input.dst") != string::npos;
  if (a_is_dst && !b_is_dst) {
    return false;
  }
  if (!a_is_dst && b_is_dst) {
    return true;
  }
  if (a->is_var() && b->is_var()) {
    string const &astr = a->get_name()->get_str();
    string const &bstr = b->get_name()->get_str();
    char const *input_src_llvm_prefix = G_INPUT_KEYWORD "." G_SRC_KEYWORD "." G_LLVM_PREFIX "-%";
    if (   string_has_prefix(astr, input_src_llvm_prefix)
        && string_has_prefix(bstr, input_src_llvm_prefix)) {
      string avarname = astr.substr(strlen(input_src_llvm_prefix));
      string bvarname = bstr.substr(strlen(input_src_llvm_prefix));
      ASSERT(avarname.size() > 0);
      ASSERT(bvarname.size() > 0);
      if (   avarname.at(0) >= '0' && avarname.at(0) <= '9'
          && bvarname.at(0) >= '0' && bvarname.at(0) <= '9') {
        // llvm %{num} vars, %1, %2 etc.
        return string_to_int(avarname) < string_to_int(bvarname);
      } else {
        string avar_prefix = string_get_maximal_alpha_prefix(avarname);
        if (!avar_prefix.empty() && string_has_prefix(bvarname, avar_prefix)) {
          // vars with common prefix and numeric suffix, e.g. %sub, %sub2 etc.
          string avar_num = avarname.substr(avar_prefix.length());
          string bvar_num = bvarname.substr(avar_prefix.length());
          if (   (avar_num.empty() || isdigit(avar_num.front()))
              && (bvar_num.empty() || isdigit(bvar_num.front()))) {
            return string_to_int(avar_num) < string_to_int(bvar_num);
          }
        }
        return avarname < bvarname;
      }
    }
  }
  return a->get_id() < b->get_id();
}

bool
corr_graph::src_expr_contains_fcall_mem_on_incoming_edge(pcpair const &p, expr_ref const &src_e) const
{
  list<corr_graph_edge_ref> incoming;
  this->get_edges_incoming(p, incoming);
  if (incoming.size() != 1) {
    return false;
  }
  corr_graph_edge_ref const &ed = incoming.front();
  if (!ed->cg_is_fcall_edge(*this)) {
    return false;
  }

  context* ctx = this->get_context();
  shared_ptr<tfg_edge_composition_t> src_ec = ed->get_src_edge();
  auto ret_preds = this->get_src_tfg().graph_apply_trans_funs(src_ec, predicate::mk_predicate_ref(precond_t(), src_e, expr_true(ctx), "tmp"), true);
  ASSERT(ret_preds.size() == 1);
  expr_ref const& xfer_e = ret_preds.front().second->get_lhs_expr();
  return this->get_context()->expr_contains_fcall_mem(xfer_e);
}

bool
corr_graph::src_expr_is_relatable_at_pc(pcpair const &p, expr_ref const &src_e) const
{
  // Handle some special cases first
  string expr_str = expr_string(src_e);
  if (src_e->is_const()) {
    DYN_DEBUG3(expr_eqclasses, cout << __func__ << ':' << __LINE__ << " ignoring " << expr_str << " because it is constant\n");
    return false;
  }
  if (   expr_str.find(LLVM_CONTAINS_UNSUPPORTED_OP_SYMBOL) != string::npos
      || expr_str.find(LLVM_CONTAINS_FLOAT_OP_SYMBOL) != string::npos
      || expr_str.find(G_LLVM_HIDDEN_REGISTER_NAME) != string::npos
      || expr_str.find(LLVM_STATE_INDIR_TARGET_KEY_ID) != string::npos
      || expr_str.find(LLVM_CALLEE_SAVE_REGNAME) != string::npos     // captured by sprel
      //|| expr_str.find(PHI_NODE_TMPVAR_SUFFIX) != string::npos       // this is required in a few cases
      || expr_str.find("." GEPOFFSET_KEYWORD) != string::npos) {     // introduced by us
    DYN_DEBUG3(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": ignoring meta-variable " << expr_str << endl;);
    return false;
  }

  if (expr_contains_array_constant(src_e)) {
    DYN_DEBUG3(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": ignoring " << expr_str << " because contains array constant" << endl);
    return false; //the solver cannot handle array constants and we do not expect any predicates of this type to be necessary for the proof
  }
  if (src_e->is_var() && src_e->is_array_sort()) {
    DYN_DEBUG3(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": ignoring " << expr_str << " because array-vars cannot be correlated; only memmasks can be correlated" << endl);
    return false;
  }

  if (!src_e->is_array_sort() && this->src_expr_contains_fcall_mem_on_incoming_edge(p, src_e)) {
    DYN_DEBUG3(expr_eqclasses, cout << __func__ << ':' << __LINE__ << " ignoring " << expr_string(src_e) << " because it is not array sort but contains mem fcall; we expect all mem fcall expressions to get correlated anyways (using array sort), so we do not need to correlate this expression.\n");
    return false;
  }
  return true;
}

expr_group_ref
corr_graph::compute_bv_bool_eqclass(pcpair const& pp, set<expr_ref> const& src_interesting_exprs, set<expr_ref> const& dst_interesting_exprs) const
{
  //Important: the expressions should be globally (across all PCs in the graph) in a particular sorted order
  //This is necessary to ensure that all the preds we obtain are in a canonical form (through the row-reduced echelon form for bitvectors)
  //Canonical form of predicates allows us to compare them across PCs
  vector<expr_ref> bv_bool_exprs(dst_interesting_exprs.begin(), dst_interesting_exprs.end());
  for (auto const& pe : src_interesting_exprs) {
    expr_ref const& src_expr = pe;
    if (this->src_expr_is_relatable_at_pc(pp, src_expr)) {
      bv_bool_exprs.push_back(src_expr);
    }
  }

  // proxy index for sorting both bv_bool_exprs and guards in a particular order
  vector<size_t> proxy_index(bv_bool_exprs.size());
  iota(proxy_index.begin(), proxy_index.end(), 0);
  std::function<bool (size_t, size_t)> expr_defined_later = [this, &pp, bv_bool_exprs](size_t i, size_t j)
  {
    return this->expr_pred_def_is_likely_nearer(pp, bv_bool_exprs.at(i), bv_bool_exprs.at(j));
  };
  sort(proxy_index.begin(), proxy_index.end(), expr_defined_later);

  if (bv_bool_exprs.size()) {
    vector<expr_ref> ret_exprs(bv_bool_exprs.size());
    // collect sorted exprs,guards using sorted proxy index
    for (size_t i = 0; i < ret_exprs.size(); ++i) {
      ret_exprs[i] = bv_bool_exprs[proxy_index[i]];
    }
    return mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_BV_EQ, ret_exprs);
  }
  else
    return nullptr;
}

void
corr_graph::select_llvmvars_designated_by_src_loc_id(graph_loc_id_t loc_id, set<expr_ref>& dst, set<expr_ref> const& src) const
{
  tfg const &src_tfg = m_eq->get_src_tfg();
  expr_ref const &ex = src_tfg.graph_loc_get_value_for_node(loc_id);
  if (ex->is_var() && context::is_src_llvmvar(ex->get_name()->get_str())) {
    if (set_belongs(src, ex)) {
      dst.insert(ex);
    }
  }
}

void
corr_graph::select_non_llvmvars_from_src_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src)
{
  for (auto const& e : src) {
    if (!e->is_var() || !context::is_src_llvmvar(e->get_name()->get_str())) {
      dst.insert(e);
    }
  }
}

bool
corr_graph::is_live_llvmvar_at_pc(expr_ref const& e, pcpair const &pp) const
{
  if(e->is_var() && context::is_src_llvmvar(e->get_name()->get_str()))
  {
    tfg const &src_tfg = m_eq->get_src_tfg();
    map<graph_loc_id_t, graph_cp_location> const& live_locs = src_tfg.get_live_locs(pp.get_first());
    for (auto const& l : live_locs) {
      expr_ref const &ex = src_tfg.graph_loc_get_value_for_node(l.first);
      if(e == ex)
        return true;
    }
  }
  return false;
}

void
corr_graph::select_llvmvars_live_at_pc_and_add_to_dst(set<expr_ref> &dst, set<expr_ref> const& src, pcpair const &pp) const
{
  tfg const &src_tfg = m_eq->get_src_tfg();
  map<graph_loc_id_t, graph_cp_location> const& live_locs = src_tfg.get_live_locs(pp.get_first());
  for (auto const& l : live_locs) {
    expr_ref const &ex = src_tfg.graph_loc_get_value_for_node(l.first);
    this->select_llvmvars_designated_by_src_loc_id(l.first, dst, src);
  }
}

void
corr_graph::select_llvmvars_modified_on_incoming_edge_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const &pp) const
{
  tfg const& src_tfg = this->get_src_tfg();
  list<corr_graph_edge_ref> incoming;
  this->get_edges_incoming(pp, incoming);
  for (auto const& e : incoming) {
    map<graph_loc_id_t, expr_ref> const& locs_modified_on_incoming_edge = src_tfg.tfg_edge_composition_get_potentially_written_locs(e->get_src_edge());
    for (auto const& l : locs_modified_on_incoming_edge) {
      expr_ref const &ex = src_tfg.graph_loc_get_value_for_node(l.first);
      this->select_llvmvars_designated_by_src_loc_id(l.first, dst, src);
    }
  }
}

void
corr_graph::select_llvmvars_correlated_at_pred_and_add_to_dst(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const& pp) const
{
  list<corr_graph_edge_ref> incoming;
  this->get_edges_incoming(pp, incoming);
  for (auto const& ed : incoming) {
    pcpair const& from_pc = ed->get_from_pc();
    if (from_pc.is_start())
      continue;
    for (auto const& e : src) {
      if (!e->is_var() || !context::is_src_llvmvar(e->get_name()->get_str())) {
        continue;
      }
      if (this->expr_llvmvar_is_correlated_with_dst_loc_at_pc(e, from_pc)) {
        dst.insert(e);
      }
    }
  }
}

void
corr_graph::select_llvmvars_having_sprel_mappings(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const& pp) const
{
  tfg const &src_tfg = m_eq->get_src_tfg();
  sprel_map_t const &src_sprel_map = src_tfg.get_sprel_map(pp.get_first());
  for (auto const& emapping : src_sprel_map.sprel_map_get_submap()) {
    select_llvmvars_appearing_in_expr(dst, src, emapping.second.first);
  }
}

void
corr_graph::select_llvmvars_appearing_in_pred_avail_exprs_at_pc(set<expr_ref>& dst, set<expr_ref> const& src, pcpair const& pp) const
{
  tfg const& src_tfg = this->get_src_tfg();
  pred_avail_exprs_t const &pred_avail_exprs = this->get_src_pred_avail_exprs_at_pc(pp);
  for (auto const& path_avexprs : pred_avail_exprs) {
    for (auto const& avail_expr : path_avexprs.second) {
      graph_loc_id_t loc_id = avail_expr.first;
      expr_ref l = src_tfg.graph_loc_get_value_for_node(loc_id);
      expr_ref const &e = avail_expr.second;
      select_llvmvars_appearing_in_expr(dst, src, l);
      select_llvmvars_appearing_in_expr(dst, src, e);
    }
  }
}

void
corr_graph::select_llvmvars_appearing_in_expr(set<expr_ref>& dst, set<expr_ref> const& src, expr_ref const& e)
{
  context *ctx = e->get_context();
  expr_list ls = ctx->expr_get_vars(e);
  for (auto const& le : ls) {
    if (set_belongs(src, le)) {
      dst.insert(le);
    }
  }
}

bool
corr_graph::expr_llvmvar_is_correlated_with_dst_loc_at_pc(expr_ref const& e, pcpair const& pp) const
{
  for (auto const &eqclass : this->get_invariant_state_at_pc(pp).get_eqclasses()) {
    invariant_state_eqclass_bv_t<pcpair, corr_graph_node, corr_graph_edge, predicate> const *eqclass_bv;
    eqclass_bv = dynamic_cast<invariant_state_eqclass_bv_t<pcpair, corr_graph_node, corr_graph_edge, predicate> const *>(eqclass);
    if (!eqclass_bv) {
      continue;
    }

    expr_group_t::expr_idx_t e_index = eqclass->get_exprs_to_be_correlated()->expr_group_get_index_for_expr(e);
    if (e_index == (expr_group_t::expr_idx_t)-1) {
      continue;
    }

    if (eqclass_bv->expr_index_has_some_predicate(e_index)) { //if any predicate exists, then it is possible for dst correlation
      return true;
    }
  }
  return false;
}


set<expr_ref> 
corr_graph::compute_bv_rank_dst_exprs(pcpair const& pp) const
{
  set<expr_ref> dst_live_exprs = this->get_dst_tfg().get_live_loc_exprs_ignoring_memslot_symbols_at_pc(pp.get_second());
  set<expr_ref> dst_spreled_exprs = this->get_dst_tfg().get_spreled_loc_exprs_at_pc(pp.get_second());
  set<expr_ref> dst_interesting_exprs;
  set_difference(dst_live_exprs, dst_spreled_exprs, dst_interesting_exprs);
  set_erase_if<expr_ref>(dst_interesting_exprs, [](expr_ref const& e) -> bool { return e->is_array_sort(); });
// now we are left with non-memmask exprs
  break_sse_reg_exprs(dst_interesting_exprs);
  return dst_interesting_exprs;
}

eqclasses_ref
corr_graph::compute_expr_eqclasses_at_pc(pcpair const& p) const
{
  autostop_timer ft(__func__);
  context* ctx = this->get_context();

  delta_t delta(ctx->get_config().max_lookahead, ctx->get_config().unroll_factor);

  set<expr_ref> src_interesting_exprs = this->get_src_tfg().get_interesting_exprs_till_delta(p.get_first(), delta);

  set<expr_ref> dst_live_exprs = this->get_dst_tfg().get_live_loc_exprs_at_pc(p.get_second());
  set<expr_ref> dst_spreled_exprs = this->get_dst_tfg().get_spreled_loc_exprs_at_pc(p.get_second());
  set<expr_ref> dst_interesting_exprs;
  set_union(dst_live_exprs, dst_spreled_exprs, dst_interesting_exprs);


  set<expr_ref> src_relevant_exprs;
  this->select_non_llvmvars_from_src_and_add_to_dst(src_relevant_exprs, src_interesting_exprs);
  DYN_DEBUG2(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": after adding non llvmvars, src_relevant_exprs.size() = " << src_relevant_exprs.size() << endl);
  this->select_llvmvars_live_at_pc_and_add_to_dst(src_relevant_exprs, src_interesting_exprs, p);
  DYN_DEBUG2(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": after adding live llvmvars, src_relevant_exprs.size() = " << src_relevant_exprs.size() << endl);
  this->select_llvmvars_modified_on_incoming_edge_and_add_to_dst(src_relevant_exprs, src_interesting_exprs, p);
  DYN_DEBUG2(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": after adding modified llvmvars, src_relevant_exprs.size() = " << src_relevant_exprs.size() << endl);
  this->select_llvmvars_correlated_at_pred_and_add_to_dst(src_relevant_exprs, src_interesting_exprs, p);
  DYN_DEBUG2(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": after adding correlated llvmvars, src_relevant_exprs.size() = " << src_relevant_exprs.size() << endl);
  this->select_llvmvars_having_sprel_mappings(src_relevant_exprs, src_interesting_exprs, p);
  DYN_DEBUG2(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": after adding llvmvars with sprel mappings, src_relevant_exprs.size() = " << src_relevant_exprs.size() << endl);
  this->select_llvmvars_appearing_in_pred_avail_exprs_at_pc(src_relevant_exprs, src_interesting_exprs, p);
  DYN_DEBUG2(expr_eqclasses, cout << __func__ << " " << __LINE__ << ": after adding llvmvars appearing in pred_avail_exprs, src_relevant_exprs.size() = " << src_relevant_exprs.size() << endl);
  DYN_DEBUG2(expr_eqclasses,
      cout << __func__ << ':' << __LINE__ << ": " << p.to_string() << ": src_interesting_exprs = [" << src_interesting_exprs.size() << "] :: ";
      for (auto const& e : src_interesting_exprs) cout << expr_string(e) << "; ";
      cout << endl;
  );
  DYN_DEBUG(expr_eqclasses,
      cout << __func__ << ':' << __LINE__ << ": " << p.to_string() << ": src_interesting_exprs.size() = " << src_interesting_exprs.size() << ", src_relevant_exprs.size() = " << src_relevant_exprs.size() << endl;
  );

  DYN_DEBUG(expr_eqclasses,
      cout << __func__ << ':' << __LINE__ << ": " << p.to_string() << ": src_relevant_exprs = [" << src_relevant_exprs.size() << "] :: ";
      for (auto const& pe : src_relevant_exprs) cout << expr_string(pe) << "; ";
      cout << endl;
      cout << __func__ << ':' << __LINE__ << ": " << p.to_string() << ": dst_interesting_exprs = [" << dst_interesting_exprs.size() << "] :: ";
      for (auto const& e : dst_interesting_exprs) cout << expr_string(e) << "; ";
      cout << endl
  );

  list<expr_group_ref> ret;
  expr_group_ref mem_eqlass = compute_mem_eqclass(p, src_relevant_exprs, dst_interesting_exprs);
  if (mem_eqlass)
    ret.push_back(mem_eqlass);

  // remove array sort exprs from src and dst
  set_erase_if<expr_ref>(src_relevant_exprs, [](expr_ref const& p) -> bool { return p->is_array_sort(); });
  set_erase_if<expr_ref>(dst_interesting_exprs, [](expr_ref const& e) -> bool { return e->is_array_sort(); });
  // now we are left with non-memmask exprs

  break_sse_reg_exprs(dst_interesting_exprs);
  expr_group_ref bv_bool_eqclass = compute_bv_bool_eqclass(p, src_relevant_exprs, dst_interesting_exprs);
  if (bv_bool_eqclass) {
    ret.push_back(bv_bool_eqclass);
  }

  if (!ctx->get_config().disable_inequality_inference) {
    vector<expr_group_ref> ineq_eqclasses = compute_ineq_eqclasses(p, src_relevant_exprs, dst_interesting_exprs);
    for (auto const& ineq_eqclass : ineq_eqclasses) {
      ret.push_back(ineq_eqclass);
    }
  }

  expr_group_ref dst_ineq_eqclass = compute_dst_ineq_eqclass(p);
  if (dst_ineq_eqclass) {
    ret.push_back(dst_ineq_eqclass);
  }

  // we need to compute ismemlabel constraints for all exprs in all eqclasses
  // custom comparator required for deterministic output
  set<expr_ref,expr_ref_cmp_t> all_eqclass_exprs_at_pc;
  for (auto const& expr_g : ret) {
    for (auto const& e : expr_g->get_expr_vec()) {
      all_eqclass_exprs_at_pc.insert(e);
    }
  }
  expr_group_ref ismemlabel_eqclass = compute_ismemlabel_eqclass(p, all_eqclass_exprs_at_pc);
  if(ismemlabel_eqclass) {
    ret.push_back(ismemlabel_eqclass);
  }

  return mk_eqclasses(ret);
}

bv_rank_val_t 
corr_graph::calculate_rank_bveqclass_at_pc(pcpair const &pp) const
{
  //src_bv_rank is taken as all exprs that have no predicate, assuming that src_bv_rank will be compared for entries having same dst_bv_rank
  unsigned dst_bv_rank = 0;
  unsigned src_dst_bv_rank = 0;
  unsigned bv_total_exprs = 0;
  if(!pp.is_start() && !pp.is_exit() && !(pp.get_first().is_fcall()))
  {
    this->get_invariant_state_at_pc(pp).update_state_for_ranking();
    set<expr_ref> dst_interesting_exprs = this->compute_bv_rank_dst_exprs(pp);
    unsigned dst_bv_preds = dst_interesting_exprs.size();

    for(auto const& eqclass : this->get_invariant_state_at_pc(pp).get_eqclasses()) {
      invariant_state_eqclass_bv_t<pcpair, corr_graph_node, corr_graph_edge, predicate> const *eqclass_bv;
      eqclass_bv = dynamic_cast<invariant_state_eqclass_bv_t<pcpair, corr_graph_node, corr_graph_edge, predicate> const *>(eqclass);
      if (!eqclass_bv) {
        continue;
      }
      expr_group_ref const &expr_group = eqclass->get_exprs_to_be_correlated();
      if (expr_group->get_type() == expr_group_t::EXPR_GROUP_TYPE_BV_EQ) {
        dst_bv_rank = dst_bv_preds;
        for(auto const &e : dst_interesting_exprs)
        {
          auto idx = expr_group->expr_group_get_index_for_expr(e);
          if(idx == (expr_group_t::expr_idx_t)-1) {
            DYN_DEBUG3(correlate, cout << __func__ << "  " << __LINE__ << " Not part of eqclass : dst_interesting_expr " << expr_string(e) << " idx = " << idx << endl);
            dst_bv_rank--;
          }
          else if (eqclass_bv->expr_index_has_some_predicate(idx)) { //if any predicate exists for this dst expression
            DYN_DEBUG3(correlate, cout << __func__ << "  " << __LINE__ << " Found pred : dst_interesting_expr " << expr_string(e) << " idx = " << idx << endl);
            dst_bv_rank--;
          }
          else
            DYN_DEBUG2(correlate, cout << __func__ << "  " << __LINE__ << " No pred found : dst_interesting_expr " << expr_string(e) << " idx = " << idx << endl);
        }
        set<expr_ref> src_live_exprs;
        for(auto const& e:  expr_group->get_expr_vec())
        {
          if(this->is_live_llvmvar_at_pc(e, pp))
          {
            src_live_exprs.insert(e);
          }
        }
        bv_total_exprs = src_live_exprs.size();
        src_dst_bv_rank = bv_total_exprs;
//        bv_total_exprs = eqclass_bv->num_exprs_ignoring_bool_sort();
//        src_dst_bv_rank = eqclass_bv->exprs_has_no_predicate();
        for(auto const &e : src_live_exprs)
        {
          auto idx = expr_group->expr_group_get_index_for_expr(e);
          if(idx == (expr_group_t::expr_idx_t)-1) {
            DYN_DEBUG3(correlate, cout << __func__ << "  " << __LINE__ << " Not part of eqclass : src_live_expr " << expr_string(e) << " idx = " << idx << endl);
            src_dst_bv_rank--;
          }
          else if (eqclass_bv->expr_index_has_some_predicate(idx)) { //if any predicate exists for this dst expression
            DYN_DEBUG3(correlate, cout << __func__ << "  " << __LINE__ << " Found pred : src_live_expr " << expr_string(e) << " idx = " << idx << endl);
            src_dst_bv_rank--;
          }
          else
            DYN_DEBUG2(correlate, cout << __func__ << "  " << __LINE__ << " No pred found : src_live_expr " << expr_string(e) << " idx = " << idx << endl);
        }
      } else { NOT_REACHED(); }
    }
  }
  DYN_DEBUG2(correlate, cout << __func__ << "  " << __LINE__ << " BV RANKS " << dst_bv_rank << ", " << src_dst_bv_rank << ", " << bv_total_exprs << " at pcpair " << pp.to_string() << endl);
  return bv_rank_val_t(dst_bv_rank, src_dst_bv_rank, bv_total_exprs);
}


expr_group_ref
corr_graph::compute_ismemlabel_eqclass(pcpair const& pp, set<expr_ref,expr_ref_cmp_t> const& all_eqclass_exprs) const
{
  set<expr_ref> ismemlabel_exprs;
  for (auto const &e : all_eqclass_exprs) {
    vector<expr_ref> const& new_exprs = generate_ismemlabel_constraints_from_expr(e, this->get_memlabel_map());
    ismemlabel_exprs.insert(new_exprs.begin(), new_exprs.end());
  }
  if (ismemlabel_exprs.size())
    return mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_HOUDINI, vector<expr_ref>(ismemlabel_exprs.begin(), ismemlabel_exprs.end()));
  else
    return nullptr;
}

expr_group_ref
corr_graph::compute_dst_ineq_eqclass(pcpair const& pp) const
{
  context* ctx = this->get_context();
  vector<expr_ref> ineq_exprs;
  set<expr_ref> dst_branch_affecting_exprs;
  tfg const& dst_tfg = this->get_dst_tfg();

  bool consider_non_vars = ctx->get_config().consider_non_vars_for_dst_ineq;
  // pairwise inequality relations for dst exprs only
  for (auto const& locid : dst_tfg.get_branch_affecting_locs_at_pc(pp.get_second())) {
    expr_ref const& dst_e = dst_tfg.graph_loc_get_value_for_node(locid);
    if ((consider_non_vars || dst_e->is_var()) && dst_e->is_bv_sort() && dst_e->get_sort()->get_size() >= BYTE_LEN && dst_e->get_sort()->get_size() <= DWORD_LEN) {
      dst_branch_affecting_exprs.insert(dst_e);
    }
  }
  for (auto i = dst_branch_affecting_exprs.cbegin(); i != dst_branch_affecting_exprs.cend(); ++i) {
    auto const& dst_expr_i = *i;
    for (auto j = next(i); j != dst_branch_affecting_exprs.cend(); ++j) {
      auto const& dst_expr_j = *j;
      get_inequality_exprs_for_expr_pair(dst_expr_i, dst_expr_j, ineq_exprs);
    }
  }
  if (ineq_exprs.size())
    return mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_HOUDINI, ineq_exprs);
  else
    return nullptr;
}

void
corr_graph::break_sse_reg_exprs(set<expr_ref> &dst_interesting_exprs) const
{
  context* ctx = this->get_context();
  set<expr_ref> vec_exprs;
  for (auto itr = dst_interesting_exprs.begin(); itr != dst_interesting_exprs.end(); ) {
  	expr_ref dst_expr = *itr;
    // break xmm register into 4 parts and add loc for them individually
    if (dst_expr->is_bv_sort() && dst_expr->get_sort()->get_size() == 4*DWORD_LEN) { // xmm registers
      itr = dst_interesting_exprs.erase(itr);
      for (size_t i = 0; i < 4; ++i) {
  			unsigned l = (i+1)*DWORD_LEN-1;
  		  unsigned r = i*DWORD_LEN;
        vec_exprs.insert(ctx->mk_bvextract(dst_expr, l, r));
      }
    }
    else { ++itr; }
  }
  dst_interesting_exprs.insert(vec_exprs.begin(), vec_exprs.end());
}

vector<expr_group_ref>
corr_graph::compute_ineq_eqclasses(pcpair const& pp, set<expr_ref> const& src_relevant_exprs, set<expr_ref> const& dst_interesting_exprs) const
{
  vector<expr_group_ref> ret;
  tfg const& src_tfg = this->get_src_tfg();
  vector<expr_ref> args;
  for (auto const& p : src_tfg.get_argument_regs().get_map()) {
    args.push_back(p.second);
  }
  for (auto const& locid : src_tfg.get_branch_affecting_locs_at_pc(pp.get_first())) {
    expr_ref const& src_e = src_tfg.graph_loc_get_value_for_node(locid);
    if (!set_belongs(src_relevant_exprs, src_e)) {
      continue;
    }
    if (src_e->is_var() && src_e->is_bv_sort() && src_e->get_sort()->get_size() >= BYTE_LEN) {
      ret.push_back(mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_BV_INEQ, { src_e }));
      for (auto const& arg : args) {
        if (   arg == src_e
            || arg->get_sort() != src_e->get_sort())
          continue;
        expr_ref src_e_minus_arg = expr_bvsub(src_e, arg);
        ret.push_back(mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_BV_INEQ, { src_e_minus_arg }));
      }
    }
  }
  tfg const& dst_tfg = this->get_dst_tfg();
  for (auto const& locid : dst_tfg.get_branch_affecting_locs_at_pc(pp.get_second())) {
    expr_ref const& dst_e = dst_tfg.graph_loc_get_value_for_node(locid);
    if (!set_belongs(dst_interesting_exprs, dst_e)) {
      continue;
    }
    if (dst_e->is_var() && dst_e->is_bv_sort() && dst_e->get_sort()->get_size() >= BYTE_LEN) {
      ret.push_back(mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_BV_INEQ, { dst_e }));
      //for (auto const& arg : args) {
      //  if (arg->get_sort() != dst_e->get_sort())
      //    continue;
      //  expr_ref dst_e_minus_arg = expr_bvsub(dst_e, arg);
      //  ret.push_back(mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_BV_INEQ, { dst_e_minus_arg }));
      //}
    }
  }
  return ret;
}

void
corr_graph::check_asserts_over_edge(corr_graph_edge_ref const &cg_edge) const
{
  NOT_IMPLEMENTED();
}

//set<pair<expr_ref, expr_ref>>
//corr_graph::get_relevant_guarded_exprs_at_pc(pcpair const& p) const
//{
//  autostop_timer ft(__func__);
//  tfg const& src_tfg = this->get_src_tfg();
//
//  set<pair<expr_ref, expr_ref>> relevant_guarded_exprs_at_pc;
//
//  // collect constraints for lookahead exprs as well because the counter-example will get propagated and it must be
//  // able to add "points" to the invariant_state's point_set for our guessing algorithm to work.
//  context* ctx = this->get_context();
//  eqclasses_ref eqclasses = this->get_expr_eqclasses_at_pc(p);
//  list<expr_group_ref> const& expr_groups = eqclasses->get_expr_group_ls();
//  for (auto const& expr_group : expr_groups) {
//    ASSERT(expr_group);
//    for (size_t i = 0; i < expr_group->size(); ++i) {
//      expr_ref e = expr_group->at(i);
//      //cout << __func__ << " " << __LINE__ << ": expr_group->size = " << expr_group->size() << endl;
//      //cout << __func__ << " " << __LINE__ << ": i = " << i << endl;
//      //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
//      //cout << __func__ << " " << __LINE__ << ": expr_group = " << expr_group << endl;
//      edge_guard_t lookahead_guard = expr_group->edge_guard_at(i);
//      expr_ref lookahead_guard_expr = lookahead_guard.edge_guard_get_expr(src_tfg, true);
//      relevant_guarded_exprs_at_pc.insert(make_pair(lookahead_guard_expr, e));
//    }
//  }
//  return relevant_guarded_exprs_at_pc;
//}

predicate_set_t
corr_graph::collect_inductive_preds_around_path(pcpair const& from_pc, shared_ptr<cg_edge_composition_t> const& pth) const
{
  autostop_timer func_timer(string("corr_graph::") + __func__);

  //typedef pair<shared_ptr<cg_edge_composition_t>, predicate_set_t> path_preds_t;

  std::function<predicate_set_t (corr_graph_edge_ref const &)> pf = [this](corr_graph_edge_ref const &e)
  {
    predicate_set_t from_preds;
    if (!e->is_empty()) {
      from_preds = this->graph_with_guessing_get_invariants_at_pc(e->get_from_pc());
    }
    return from_preds;
  };

  return this->pth_collect_preds_using_atom_func(pth, pf, this->graph_with_guessing_get_invariants_at_pc(from_pc));
}

bool
corr_graph::check_stability_for_added_edge(corr_graph_edge_ref const& cg_edge) const
{
  set<pcpair> reachable_pcs = this->get_all_reachable_pcs(cg_edge->get_to_pc());
  reachable_pcs.insert(cg_edge->get_to_pc());
  reachable_pcs.insert(cg_edge->get_from_pc());

  for (auto const& pp : reachable_pcs) {
    if (!this->is_stable_at_pc(pp)) {
      DYN_DEBUG(eqcheck, cout << __func__ << ':' << __LINE__ << ": stability check failed at " << pp.to_string() << endl);
      return false;
    }
  }
  return true;
}

bool
corr_graph::falsify_fcall_proof_query(expr_ref const& src, expr_ref const& dst, pcpair const& from_pc, counter_example_t& counter_example) const
{
  autostop_timer ft(__func__);
  auto const& relevant_memlabels = this->get_relevant_memlabels();

  expr_find_op fcall_op(src, expr::OP_FUNCTION_CALL);
  expr_find_op fcall_op2(dst, expr::OP_FUNCTION_CALL);
  expr_vector src_fcalls = fcall_op.get_matched_expr();
  expr_vector dst_fcalls = fcall_op2.get_matched_expr();

  if (   src_fcalls.empty()
      && dst_fcalls.empty()) {
    return false;
  }

  ASSERT (from_pc.get_first().get_subsubindex() == PC_SUBSUBINDEX_FCALL_START);
  ASSERT (from_pc.get_second().get_subsubindex() == PC_SUBSUBINDEX_FCALL_START);

  // get all function vars
  set<expr_ref> fvars;
  for (auto const& fc : src_fcalls) {
    fvars.insert(fc->get_args().at(0));
  }
  for (auto const& fc : dst_fcalls) {
    fvars.insert(fc->get_args().at(0));
  }

  // get all symbol slots addr
  set<expr_ref> sym_addrs;
  for (auto const &eqclass : this->get_invariant_state_at_pc(from_pc).get_eqclasses()) {
    expr_group_ref const &expr_group = eqclass->get_exprs_to_be_correlated();
    for (auto const& ex : expr_group->get_expr_vec()) {
      if (   ex->get_operation_kind() == expr::OP_SELECT
          && ex->get_args().at(1)->get_memlabel_value().memlabel_represents_symbol())
        sym_addrs.insert(ex->get_args().at(2));
    }
  }

  nodece_ref<pcpair,corr_graph_node,corr_graph_edge> nce = this->get_random_counter_example_at_pc(from_pc);
  if (!nce) {
    return false;
  }
  //cout << __func__ << " " << __LINE__ << ": nce = " << nce << endl;
  counter_example = nce->nodece_get_counter_example(this);

  context* ctx = this->get_context();
  counter_example_t rand_vals(ctx);
  for (auto const& fv : fvars) {
    string_ref const& fname = fv->get_name();
    sort_ref return_sort = fv->get_sort()->get_range_sort();
    expr_ref default_value;

    if (return_sort->is_bv_kind()) {
      default_value = counter_example_t::gen_random_value_for_counter_example(ctx, return_sort);
    } else {
      ASSERT(return_sort->is_array_kind());
      ASSERT(return_sort->get_domain_sort().size() == 1);

      sort_ref range_sort = return_sort->get_range_sort();
      map<vector<expr_ref>,expr_ref> point_values;
      for (auto const& addr : sym_addrs) {
        expr_ref value = counter_example_t::gen_random_value_for_counter_example(ctx, range_sort);
        expr_ref addr_evaluated;
        bool success = counter_example.evaluate_expr_assigning_random_consts_as_needed(addr, addr_evaluated, rand_vals, relevant_memlabels);
        ASSERT(success);
        vector<expr_ref> addr_v = { addr_evaluated };
        point_values.insert(make_pair(addr_v, value));
      }
      expr_ref def_val = counter_example_t::gen_random_value_for_counter_example(ctx, range_sort);
      array_constant_ref fn_mem = ctx->mk_array_constant(point_values, def_val);
      default_value = ctx->mk_array_const(fn_mem, return_sort);
    }
    expr_ref fn_expr = ctx->mk_array_const_with_def(fv->get_sort(), default_value);
    counter_example.set_varname_value(fname, fn_expr);
  }

  expr_ref src_evaluated, dst_evaluated;
  bool success = counter_example.evaluate_expr_assigning_random_consts_as_needed(src, src_evaluated, rand_vals, relevant_memlabels);
  if (!success) {
    // evaluation can still due to slot addresses being out of bound
    // this can only happen if args are part of slot address
    return false;
  }
  success = counter_example.evaluate_expr_assigning_random_consts_as_needed(dst, dst_evaluated, rand_vals, relevant_memlabels);
  if (!success) {
    return false;
  }

  if (src_evaluated != dst_evaluated) {
    counter_example.counter_example_union(rand_vals);
    CPP_DBG_EXEC(CE_FUZZER, cout << __func__ << " " << __LINE__ << ": query falsified by generated counter example.\nsrc: " << expr_string(src) << " = " << expr_string(src_evaluated) << "\ndst: " << expr_string(dst) << " = " << expr_string(dst_evaluated) << endl);
    return true;
  } else {
    return false;
  }
}

bool
corr_graph::graph_edge_is_well_formed(corr_graph_edge_ref const& e) const
{
  autostop_timer ft(__func__);

  context* ctx = this->get_context();
  tfg const &src_tfg = this->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();

  shared_ptr<tfg_edge_composition_t> src_ec = e->get_src_edge();
  shared_ptr<tfg_edge_composition_t> dst_ec = e->get_dst_edge();


  expr_ref const& src_edge_cond = src_tfg.tfg_edge_composition_get_edge_cond(src_ec, /*simplified*/true);
  expr_ref const& dst_edge_cond = dst_tfg.tfg_edge_composition_get_edge_cond(dst_ec, /*simplified*/true);

  ASSERT(src_edge_cond->is_bool_sort());
  ASSERT(dst_edge_cond->is_bool_sort());
  pcpair const& from_pc = e->get_from_pc();

  DYN_DEBUG(graph_edge_is_well_formed, cout << __func__ << " " << __LINE__ << ": src_ec = " << src_ec->graph_edge_composition_to_string() << endl);
  DYN_DEBUG(graph_edge_is_well_formed, cout << __func__ << " " << __LINE__ << ": dst_ec = " << dst_ec->graph_edge_composition_to_string() << endl);

//  aliasing_constraints_t const& src_edge_alias_cond = src_tfg.collect_aliasing_constraints_around_path(from_pc.get_first(), src_ec);
//  predicate_set_t src_edge_cond_alias_pred = src_edge_alias_cond.get_ismemlabel_preds();
//  aliasing_constraints_t const& dst_edge_alias_cond = dst_tfg.collect_aliasing_constraints_around_path(from_pc.get_second(), dst_ec);
//  predicate_set_t dst_edge_cond_alias_pred = dst_edge_alias_cond.get_ismemlabel_preds();
//
//    predicate_set_t lhs_preds;// = src_tfg.collect_assumes_around_path(from_pc.get_first(), src_ec);
  predicate_set_t src_dst_assume_alias_preds;// = lhs_preds;
//  predicate_set_union(src_dst_assume_alias_preds, src_edge_cond_alias_pred);
//  predicate_set_union(src_dst_assume_alias_preds, dst_edge_cond_alias_pred);

  DYN_DEBUG(graph_edge_is_well_formed, cout << __func__ << " " << __LINE__ << ": src_edge_cond = " << expr_string(src_edge_cond) << endl);
  DYN_DEBUG(graph_edge_is_well_formed, cout << __func__ << " " << __LINE__ << ": dst_edge_cond = " << expr_string(dst_edge_cond) << endl);

  predicate_ref pred = predicate::mk_predicate_ref(precond_t(), expr_or(expr_not(dst_edge_cond), src_edge_cond), expr_true(ctx), __func__);
  bool guess_made_weaker;
  mytimer mt2;
  DYN_DEBUG(correlate, cout << __func__ << " " << __LINE__ << ": start " << endl; mt2.start(););
  proof_status_t proof_status = this->decide_hoare_triple(src_dst_assume_alias_preds, from_pc, mk_epsilon_ec<pcpair,corr_graph_edge>(), pred, guess_made_weaker);
  DYN_DEBUG(correlate, mt2.stop(); cout << __func__ << " " << __LINE__ << ": query took " << mt2.get_elapsed_secs() << "s " << endl);
  if (proof_status == proof_status_timeout) {
    NOT_IMPLEMENTED();
  }
  if (proof_status == proof_status_proven) {
    if (guess_made_weaker) {
      this->add_local_sprel_expr_assumes(pred->get_local_sprel_expr_assumes());
    }
  }
  //ASSERT(guess_made_weaker == (pred.get_proof_status() == predicate::not_provable)); //there is no chance of weakening due to local sprel expr guesses here
  return proof_status == proof_status_proven;
}

aliasing_constraints_t
corr_graph::get_aliasing_constraints_for_edge(corr_graph_edge_ref const& e) const
{
  autostop_timer ft(string("corr_graph::")+__func__);

  tfg const& src_tfg = this->get_src_tfg();
  tfg const& dst_tfg = this->get_dst_tfg();
  pcpair const& from_pc = e->get_from_pc();
  shared_ptr<tfg_edge_composition_t> src_ec = e->get_src_edge();
  shared_ptr<tfg_edge_composition_t> dst_ec = e->get_dst_edge();

  aliasing_constraints_t ret;
  ret.aliasing_constraints_union(src_tfg.graph_with_paths<pc, tfg_node, tfg_edge, predicate>::collect_aliasing_constraints_around_path(from_pc.get_first(), src_ec));
  ret.aliasing_constraints_union(dst_tfg.graph_with_paths<pc, tfg_node, tfg_edge, predicate>::collect_aliasing_constraints_around_path(from_pc.get_second(), dst_ec));

  std::function<expr_ref (expr_ref const&)> f = [this, &from_pc](expr_ref const& e)
  {
    return this->expr_simplify_at_pc(e, from_pc);
  };
  ret.visit_exprs(f);

  //cout << "corr_graph::" << __func__ << " " << __LINE__ << ": returning for " << e->to_string() << ":\n" << ret.to_string_for_eq() << endl;
  return ret;
}

aliasing_constraints_t
corr_graph::graph_apply_trans_funs_on_aliasing_constraints(corr_graph_edge_ref const& e, aliasing_constraints_t const& als) const
{
  tfg const& src_tfg = this->get_src_tfg();
  tfg const& dst_tfg = this->get_dst_tfg();
  pcpair const& from_pc = e->get_from_pc();
  shared_ptr<tfg_edge_composition_t> src_ec = e->get_src_edge();
  shared_ptr<tfg_edge_composition_t> dst_ec = e->get_dst_edge();

  function<aliasing_constraints_t (/*tfg const&, */tfg_edge_ref const&)> edge_atom_f =
    [](/*tfg const& t, */tfg_edge_ref const& e) -> aliasing_constraints_t
  {
    return aliasing_constraints_t();
    //return t.get_aliasing_constraints_for_edge(e);
  };
  //auto src_edge_atom_f = bind(edge_atom_f, src_tfg, placeholders::_1);
  //auto dst_edge_atom_f = bind(edge_atom_f, dst_tfg, placeholders::_1);

  aliasing_constraints_t ret = als;
  ret = src_tfg.collect_aliasing_constraints_around_path_helper(from_pc.get_first(), src_ec, /*src_*/edge_atom_f, ret);
  ret = dst_tfg.collect_aliasing_constraints_around_path_helper(from_pc.get_second(), dst_ec, /*dst_*/edge_atom_f, ret);

  return ret;
}

predicate_set_t
corr_graph::collect_assumes_for_edge(corr_graph_edge_ref const& e) const
{
  return this->collect_assumes_around_edge(e);
}

predicate_set_t
corr_graph::collect_assumes_around_edge(corr_graph_edge_ref const& e) const
{
  if (e->is_empty()) {
    return {};
  };
  pcpair const& from_pc = e->get_from_pc();
  //predicate_set_t ret_preds = this->get_simplified_assume_preds(e->get_from_pc());
  predicate_set_t ret_preds = this->get_assume_preds(from_pc);
  ASSERT(ret_preds.empty() || from_pc.is_start());

  // collect assumes on intermediate PCs of src and dst composite paths
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();

  predicate_set_t const& src_composite_path_preds = src_tfg.collect_assumes_around_path(from_pc.get_first(), e->get_src_edge());
  predicate_set_t const& dst_composite_path_preds = dst_tfg.collect_assumes_around_path(from_pc.get_second(), e->get_dst_edge());

  predicate::predicate_set_union(ret_preds, src_composite_path_preds);
  predicate::predicate_set_union(ret_preds, dst_composite_path_preds);

  return ret_preds;
}

shared_ptr<corr_graph>
corr_graph::corr_graph_from_stream(istream& in, context* ctx)
{
  string line;
  bool end;
  end = !getline(in, line);
  ASSERT(!end);
  string const cg_prefix = "=corr_graph ";
  ASSERT(string_has_prefix(line, cg_prefix));
  string name = line.substr(cg_prefix.length());
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=src_tfg");
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=TFG:");
  shared_ptr<tfg_llvm_t> src_tfg = make_shared<tfg_llvm_t>(in, name + ".src", ctx);
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=dst_tfg");
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=TFG:");
  shared_ptr<tfg_asm_t> dst_tfg = make_shared<tfg_asm_t>(in, name + ".dst", ctx);
  end = !getline(in, line);
  ASSERT(!end);

  src_tfg->tfg_preprocess_before_eqcheck({}, {}, true);
  dst_tfg->tfg_preprocess_before_eqcheck({}, {}, false);

  ASSERT(line == "=eqcheck_info");
  shared_ptr<eqcheck> eq = make_shared<eqcheck>(in, src_tfg, dst_tfg);
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=graph_with_guessing");
  shared_ptr<corr_graph> cg = make_shared<corr_graph>(in, name, eq);
  do {
    end = !getline(in, line);
    ASSERT(!end);
  } while (line == "");
  if (line != "=corr_graph_done") {
    cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
  }
  ASSERT(line == "=corr_graph_done");

  list<shared_ptr<corr_graph_node>> nodes;
  cg->get_nodes(nodes);
  for (auto n : nodes) {
    cg->init_cg_node(n);
  }

  cg->populate_transitive_closure();
  cg->cg_add_precondition(/*relevant_memlabels*/);
  cg->update_src_suffixpaths_cg(nullptr);

  return cg;
}

void
corr_graph::graph_to_stream(ostream& os) const
{
  tfg const &src_tfg = m_eq->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();
  os << "=corr_graph " << this->get_name()->get_str() << "\n";
  os << "=src_tfg\n";
  src_tfg.graph_to_stream(os);
  os << "=dst_tfg\n";
  dst_tfg.graph_to_stream(os);
  os << "=eqcheck_info\n";
  m_eq->eqcheck_to_stream(os);
  os << "=graph_with_guessing\n";
  this->graph_with_guessing<pcpair, corr_graph_node, corr_graph_edge, predicate>::graph_to_stream(os);
  os << "=corr_graph_done\n";
}

/*string
corr_graph::graph_from_stream(istream& is)
{
  string line;
  bool end;
  end = !getline(is, line);
  ASSERT(!end);
  ASSERT(string_has_prefix(line, "=CORR_GRAPH: "));
  line = this->graph_with_guessing<pcpair, corr_graph_node, corr_graph_edge, predicate>::graph_from_stream(is);
  ASSERT(line == "=CORR_GRAPHdone");
  end = !getline(is, line);
  ASSERT(!end);
  return line;
}*/

pair<bool,bool>
corr_graph::corr_graph_propagate_CEs_on_edge(shared_ptr<corr_graph_edge const > const& cg_new_edge, bool propagate_initial_CEs)
{
  autostop_timer func_timer(__func__);
  context *ctx = m_eq->get_context();

  if (cg_new_edge->get_to_pc().is_exit())
    return make_pair(true, true);
    // skip exit PCs as inference is not performed over them
  mytimer mt;
  DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": propagating CEs across edge: " << cg_new_edge->to_string() << endl; mt.start(););
  bool implied_check_status = true;
  bool stability_check_status = true;
  unsigned propagated_CEs = this->propagate_CEs_across_edge_while_checking_implied_cond_and_stability(cg_new_edge, implied_check_status, stability_check_status, propagate_initial_CEs);
  DYN_DEBUG(eqcheck, mt.stop(); cout << __func__ << " " << __LINE__ << ": propagated " << propagated_CEs << " CEs in " << mt.get_elapsed_secs() << "s across edge: " << cg_new_edge->to_string() << ". implied_check_status = " << implied_check_status << ", stability_check_status = " << stability_check_status << endl);
  return make_pair(implied_check_status, stability_check_status);
}

bool
corr_graph::cg_check_new_cg_edge(corr_graph_edge_ref cg_edge)
{
  autostop_timer func_timer(__func__);
  context *ctx = m_eq->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  tfg const &src_tfg = this->get_eq()->get_src_tfg();
  tfg const &dst_tfg = this->get_dst_tfg();
  stats::get().inc_counter("# of paths expanded");

  mytimer mt1;
  DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": semantically_simplify_tfg_edge_composition: " << endl; mt1.start(););
  corr_graph_edge_ref cg_new_edge = corr_graph::cg_edge_semantically_simplify_src_tfg_ec(*cg_edge, src_tfg);
  if (!cg_new_edge) {
    DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": simplified src_tfg_edge return nullptr\n");
    DYN_DEBUG(eqcheck, mt1.stop(); cout << __func__ << " " << __LINE__ << ": simplified src_tfg_edge return nullptr in " << mt1.get_elapsed_secs() << "secs\n");
    return false;
  }
  DYN_DEBUG(eqcheck, mt1.stop(); cout << __func__ << " " << __LINE__ << ": semantically_simplify_tfg_edge_composition took " << mt1.get_elapsed_secs() << "s " << endl);
  if(cg_edge != cg_new_edge)
  {
    this->remove_edge(cg_edge);
    this->add_edge(cg_new_edge);
  }


  DYN_DEBUG(oopsla_log, cout << "Inside function checkCriterionForEdges for Pathset in C = " << cg_new_edge->get_src_edge()->graph_edge_composition_to_string() << " and pathset in A = " << cg_new_edge->get_dst_edge()->graph_edge_composition_to_string() << endl);
  if (!this->check_asserts_on_edge(cg_new_edge)) {
    DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": check_asserts_on_edge failed; removing edge (correlation failed) with src_path: " << cg_new_edge->get_src_edge()->graph_edge_composition_to_string() << " and dst path: " << cg_new_edge->get_dst_edge()->graph_edge_composition_to_string() << endl);
    DYN_DEBUG(oopsla_log, cout << "checkCriterionForEdges Failed!" << endl);
    return false;
  }
  DYN_DEBUG(oopsla_log, cout << "checkCriterionForEdges Passed, Going inside inferInvariantsAndCounterExample().." << endl);

  pair<bool, bool> is_implied_stable = this->corr_graph_propagate_CEs_on_edge(cg_new_edge, false);
  ASSERT(is_implied_stable.first);
  if(!is_implied_stable.second) {
    CPP_DBG_EXEC2(EQCHECK, cout << __func__ << " " << __LINE__ << " Correlation rejected due to stability while propagating remaining CEs" << endl);
    stats::get().inc_counter("# of Paths Prunned -- InvRelatesHeapAtEachNode");
    DYN_DEBUG(oopsla_log, cout << "Heap states not equal after inferInvariantsAndCounterExample(), returning nullptr" << endl);
    return false;
  }

  if (!cg_new_edge->get_to_pc().is_exit()) {
    if (!this->update_invariant_state_over_edge(cg_new_edge)) { //returns false if some assert becomes unprovable
      DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": update_invariant_state_over_edge failed; removing edge (correlation failed) with src_path: " << cg_new_edge->get_src_edge()->graph_edge_composition_to_string() << " and dst path: " << cg_new_edge->get_dst_edge()->graph_edge_composition_to_string() << endl);
      DYN_DEBUG(oopsla_log, cout << "Heap states not equal after inferInvariantsAndCounterExample(), returning nullptr" << endl);
      return false;
    }
  }

  if (!this->check_stability_for_added_edge(cg_new_edge)) {
    DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": stability check failed; removing edge (correlation failed) with src_path: " << cg_new_edge->get_src_edge()->graph_edge_composition_to_string() << " and dst path: " << cg_new_edge->get_dst_edge()->graph_edge_composition_to_string() << endl);
    DYN_DEBUG(oopsla_log, cout << "Heap states not equal after inferInvariantsAndCounterExample(), returning nullptr" << endl);
    return false;
  }

  DYN_DEBUG(oopsla_log, cout << "inferInvariantsAndCounterExample() completed for Pathset_C: " << cg_new_edge->get_src_edge()->graph_edge_composition_to_string() << " and Pathset_A: " << cg_new_edge->get_dst_edge()->graph_edge_composition_to_string() << endl);
  DYN_DEBUG(counters_enable, stats::get().inc_counter("corr-stability-passed"));

  DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": Successfully correlated edge " << cg_new_edge->to_string() <<" with src_path: " << cg_new_edge->get_src_edge()->graph_edge_composition_to_string() << " and dst path: " << cg_new_edge->get_dst_edge()->graph_edge_composition_to_string() << endl);
  //return cg_new;
  return true;
}

predicate_set_t
corr_graph::get_simplified_non_mem_assumes(corr_graph_edge_ref const& e) const
{
  autostop_timer func_timer(string("corr_graph::") + string(__func__));
  /* using a cache here makes sense for two reasons:
   * * this function will be called for each proof query, thus hit ratio will be high
   * * invalidation is not a problem as `corr_graph_edge`s are immutable
   */
  if (m_non_mem_assumes_cache.count(e)) {
    return m_non_mem_assumes_cache.at(e);
  }

  predicate_set_t ret = this->get_simplified_non_mem_assumes_helper(e);
  m_non_mem_assumes_cache.insert(make_pair(e, ret));

  return ret;
}

predicate_set_t
corr_graph::get_simplified_non_mem_assumes_helper(corr_graph_edge_ref const& e) const
{
  autostop_timer ft(string("corr_graph::") + string(__func__));

  tfg const* src_tfg = &(this->get_src_tfg());
  tfg const* dst_tfg = &(this->get_dst_tfg());

  shared_ptr<tfg_edge_composition_t> const& src_ec = e->get_src_edge();
  shared_ptr<tfg_edge_composition_t> const& dst_ec = e->get_dst_edge();

  function<predicate_set_t (tfg const*, tfg_edge_ref const&)> atom_f =
    [](tfg const* t, tfg_edge_ref const& e) -> predicate_set_t
    { return t->get_simplified_non_mem_assumes(e); };
  function<predicate_set_t (tfg_edge_ref const&)> src_atom_f = bind(atom_f, src_tfg, placeholders::_1);
  function<predicate_set_t (tfg_edge_ref const&)> dst_atom_f = bind(atom_f, dst_tfg, placeholders::_1);

  ASSERT(dst_tfg->pth_collect_preds_using_atom_func(dst_ec, dst_atom_f, {}).size() == 0); // dst tfg is not supposed to have any assumes

  predicate_set_t const src_preds = src_tfg->pth_collect_preds_using_atom_func(src_ec, src_atom_f, {});
  predicate_set_t ret;
  for (auto const& pred : src_preds) {
    bool has_mem_ops = false;
    function<void (string const&, expr_ref const&)> f =
      [&has_mem_ops](string const& s, expr_ref const& e)
      { if (expr_has_mem_ops(e)) { has_mem_ops = true; } };

    pred->visit_exprs(f);
    if (!has_mem_ops) {
      ret.insert(pred);
    }
  }
  return ret;
}

corr_graph_edge_ref
corr_graph::cg_edge_semantically_simplify_src_tfg_ec(corr_graph_edge const& cg_edge, tfg const& src_tfg)
{
  shared_ptr<tfg_edge_composition_t> new_src_edge = src_tfg.semantically_simplify_tfg_edge_composition(cg_edge.get_src_edge());
  if (!new_src_edge) {
    return nullptr;
  }
  return mk_corr_graph_edge(cg_edge.get_from_pc(), cg_edge.get_to_pc(), new_src_edge, cg_edge.get_dst_edge());
}

expr_ref
corr_graph::cg_edge_get_src_cond(corr_graph_edge const& cg_edge) const
{
  tfg const &src_tfg = this->get_eq()->get_src_tfg();
  return src_tfg.tfg_edge_composition_get_edge_cond(cg_edge.get_src_edge(), true);
}

expr_ref
corr_graph::cg_edge_get_dst_cond(corr_graph_edge const& cg_edge) const
{
  tfg const &dst_tfg = this->get_dst_tfg();
  return dst_tfg.tfg_edge_composition_get_edge_cond(cg_edge.get_dst_edge(), true);
}

state
corr_graph::cg_edge_get_src_to_state(corr_graph_edge const& cg_edge) const
{
  tfg const &src_tfg = this->get_eq()->get_src_tfg();
  return src_tfg.tfg_edge_composition_get_to_state(cg_edge.get_src_edge());
}

state
corr_graph::cg_edge_get_dst_to_state(corr_graph_edge const& cg_edge) const
{
  tfg const &dst_tfg = this->get_dst_tfg();
  return dst_tfg.tfg_edge_composition_get_to_state(cg_edge.get_dst_edge());
}

}
