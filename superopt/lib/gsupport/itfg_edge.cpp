//#include "graph/tfg.h"
#include <boost/algorithm/string.hpp>
#include "expr/solver_interface.h"
//#include "cg/corr_graph.h"
/* #include "graph_cp.h" */

#include <iostream>
#include "support/log.h"
#include "support/debug.h"
#include "support/utils.h"
#include "support/globals_cpp.h"
#include "expr/memlabel.h"
#include "expr/consts_struct.h"
#include "expr/expr_simplify.h"
#include "expr/local_sprel_expr_guesses.h"
#include "graph/lr_map.h"
#include "gsupport/sprel_map.h"
#include "tfg/parse_input_eq_file.h"
#include "graph/graph_fixed_point_iteration.h"
//#include "sym_exec/sym_exec.h"
//#include "rewrite/locals_info_cpp.h"
#include "rewrite/transmap.h"
#include "rewrite/static_translate.h"
#include <fstream>
#include <boost/algorithm/string/replace.hpp>
#include "support/timers.h"
#include "support/stopwatch.h"
#include "support/debug.h"
#include "support/distinct_sets.h"
#include "expr/context_cache.h"
#include "graph/expr_loc_visitor.h"

namespace eqspace {

itfg_edge_ref
mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition/*, state const &start_state*/, unordered_set<expr_ref> const& assumes/*, bool is_indir_exit, expr_ref indir_tgt*/, te_comment_t const& comment)
{
  itfg_edge ie(from_pc, to_pc, state_to, condition/*, start_state*/, assumes, comment);
  return itfg_edge::get_itfg_edge_manager()->register_object(ie);
  //return make_shared<itfg_edge const>(from_pc, to_pc, state_to, condition, start_state, comment);
}

//itfg_edge_ref
//mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition, state const &start_state/*, bool is_indir_exit, expr_ref indir_tgt*/, te_comment_t const& te_comment)
//{
//  itfg_edge ie(from_pc, to_pc, state_to, condition, start_state, te_comment.get_str());
//  return itfg_edge::get_itfg_edge_manager()->register_object(ie);
//  //return make_shared<itfg_edge const>(from_pc, to_pc, state_to, condition, start_state, comment);
//}



//itfg_edge_ref
//mk_itfg_edge(pc const &from_pc, pc const &to_pc, eqspace::state const &state_to, expr_ref const &condition)
//{
//  itfg_edge ie(from_pc, to_pc, state_to, condition);
//  return itfg_edge::get_itfg_edge_manager()->register_object(ie);
//  //return make_shared<itfg_edge const>(from_pc, to_pc, state_to, condition);
//}

manager<itfg_edge> *
itfg_edge::get_itfg_edge_manager()
{
  static manager<itfg_edge> *ret = NULL;
  if (!ret) {
    ret = new manager<itfg_edge>;
  }
  return ret;
}

itfg_edge::itfg_edge(pc const &from_pc, pc const &to_pc, const state& state_to, expr_ref const &condition/*, state const &start_state*/, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment) :
  edge_with_cond<pc>(from_pc, to_pc/*, from, to*/, condition, state_to/*, start_state*/, assumes),
  m_comment(te_comment), m_is_managed(false)
  /*,m_is_indir_exit(is_indir_exit),
    m_indir_tgt(indir_tgt),*/
{
  //this->set_to_state(state_to);
  //this->set_cond(condition);
}

//itfg_edge::itfg_edge(/*shared_ptr<tfg_node> from, shared_ptr<tfg_node> to, */pc const &from_pc, pc const &to_pc, const state& state_to, expr_ref const &condition, state const &start_state/*, bool is_indir_exit, expr_ref indir_tgt*/, string comment) :
//  edge_with_cond<pc, tfg_node>(from_pc, to_pc/*, from, to*/, condition, state_to, start_state),
//  m_comment(comment), m_is_managed(false)
//  /*,m_is_indir_exit(is_indir_exit),
//    m_indir_tgt(indir_tgt),*/
//{
//  //this->set_to_state(state_to);
//  //this->set_cond(condition);
//}
//
//itfg_edge::itfg_edge(pc const &from_pc, pc const &to_pc, const state& state_to, expr_ref const &condition/*, bool is_indir_exit, expr_ref indir_tgt*/) :
//  edge_with_cond<pc, tfg_node>(from_pc, to_pc/*, from, to*/, condition, state_to), m_is_managed(false)
//    /*,m_is_indir_exit(is_indir_exit),
//      m_indir_tgt(indir_tgt)*/
//{
//  //this->set_to_state(state_to);
//  //this->set_cond(condition);
//}

itfg_edge::~itfg_edge()
{
  if (m_is_managed) {
    this->get_itfg_edge_manager()->deregister_object(*this);
  }
}

string tfg_node::to_string() const
{
  return node<pc>::to_string();
}

string
itfg_edge::to_string_with_comment() const
{
  string ret = this->to_string();
  return ret + " " + m_comment.get_string();
}

/*string tfg_edge_composite::to_string(bool state) const
{
  stringstream ss;
  ss << edge::to_string();
  ss << " [composite: " << m_composition_node->to_string() << "]";

  if(state)
  {
    ss << "\n";
    ss << to_string_labels();
  }
  return ss.str();
}

string tfg_edge_composite::to_string_proof_file() const
{
  stringstream ss;
  ss << m_composition_node->to_string();
  return ss.str();
}

string tfg_edge_composite::to_string_labels() const
{
  stringstream ss;
  ss << "Backedge: " << (is_back_edge() ? "B" : "F") << "\n";
  //ss << "Pred: " + get_cond()->to_string() + "\n";
  ss << "Pred:\n" + get_cond()->to_string_table() + "\n";
  ss << "State:\n" + get_to_state().to_string();
  return ss.str();
}*/

//void
//itfg_edge::substitute_variables_at_output(transmap_t const *tmap, state const &start_state)
//{
//  //m_to_state.reorder_state_elements_using_transmap(m_to_state, tmap);
//  this->get_to_state().reorder_state_elements_using_transmap(tmap, start_state);
//}

//itfg_edge_ref
//itfg_edge::convert_jmp_nextpcs_to_callret(consts_struct_t const &cs, int r_esp, int r_eax, tfg const *tfg)
//{
//  state const &start_state = tfg->get_start_state();
//  if (!this->get_to_pc().is_exit()) {
//    return NULL;
//  }
//  unsigned nextpc_id = atoi(this->get_to_pc().get_index());
//  if (nextpc_id == 0) { //XXX: hack: assumes nextpc0 is ret
//    return NULL;
//  }
//  if (this->edge_is_indirect_branch()) {
//    NOT_IMPLEMENTED();
//  }
//  /*if (!nextpc_args_map.count(nextpc_id)) {
//    return NULL;
//  }*/
//  pc from_pc = this->get_from_pc();
//  pc to_pc = this->get_to_pc();
//  to_pc.set_index(0, 0, 0);
//
//  ASSERT(nextpc_id >= 0 && nextpc_id < cs.m_nextpc_consts.size());
//  size_t num_args = 0; //nextpc_args_map.at(nextpc_id).first;
//  expr_ref nextpc_id_expr = cs.m_nextpc_consts[nextpc_id];
//
//  state to_state = this->get_to_state();
//  to_state.apply_function_call(nextpc_id_expr, num_args, r_esp, r_eax, start_state);
//  //shared_ptr<tfg_node> to_node = tfg->find_node(to_pc);
//  //shared_ptr<tfg_node> from_node = tfg->find_node(from_pc);
//
//  //context *ctx = this->get_cond()->get_context();
//  itfg_edge_ref new_e = mk_itfg_edge(from_pc, to_pc, to_state, this->get_cond()/*, this->get_start_state(), false, expr_false(ctx)*/);
//  return new_e;
//}

//expr_ref
//itfg_edge::get_div_by_zero_cond() const
//{
//  context *ctx = this->get_cond()->get_context();
//  state const& to_state = this->get_to_state();
//  expr_ref ret = expr_false(ctx);
//  std::function<void (string const&, expr_ref)> f = [&ret, ctx](string const& k, expr_ref e)
//  {
//    expr_ref eret = ctx->expr_get_div_by_zero_cond(e);
//    if (eret != expr_false(ctx)) {
//      ret = ctx->mk_or(ret, eret);
//    }
//  };
//  to_state.state_visit_exprs(f);
//  return ret;
//}

//expr_ref
//itfg_edge::get_ismemlabel_heap_access_cond() const
//{
//  context *ctx = this->get_cond()->get_context();
//  state const& to_state = this->get_to_state();
//  expr_ref ret = expr_false(ctx);
//  std::function<void (string const&, expr_ref)> f = [&ret, ctx](string const& k, expr_ref e)
//  {
//    expr_ref eret = ctx->expr_get_memlabel_heap_access_cond(e);
//    if (eret != expr_false(ctx)) {
//      ret = ctx->mk_or(ret, eret);
//    }
//  };
//  to_state.state_visit_exprs(f);
//  return ret;
//}

shared_ptr<itfg_edge const>
itfg_edge::itfg_edge_visit_exprs(std::function<expr_ref (string const&, expr_ref)> f) const
{
  state to_state = this->get_to_state();
  expr_ref cond = this->get_cond();
  cond = f("edgecond", cond);
  to_state.state_visit_exprs(f);
  unordered_set<expr_ref> const& assumes = unordered_set_visit_exprs(this->get_assumes(), f);
  return mk_itfg_edge(this->get_from_pc(), this->get_to_pc(), to_state, cond/*, state()*/, assumes, this->get_comment());
}

}
