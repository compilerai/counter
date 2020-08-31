//#include "graph/tfg.h"
#include <boost/algorithm/string.hpp>
#include "expr/solver_interface.h"
#include "eq/corr_graph.h"
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
//#include "gsupport/sym_exec.h"
#include "tfg/predicate.h"
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

tfg_edge::~tfg_edge()
{
  if (m_is_managed) {
    tfg_edge::get_tfg_edge_manager()->deregister_object(*this);
  }
}

manager<tfg_edge> *
tfg_edge::get_tfg_edge_manager()
{
  static manager<tfg_edge> *ret = nullptr;
  if (!ret) {
    ret = new manager<tfg_edge>;
  }
  return ret;
}

shared_ptr<tfg_edge const>
tfg_edge::convert_jmp_nextpcs_to_callret(consts_struct_t const &cs, int r_esp, int r_eax, tfg const *tfg) const
{
  state const &start_state = tfg->get_start_state();
  if (!this->get_to_pc().is_exit()) {
    return nullptr;
  }
  unsigned nextpc_id = atoi(this->get_to_pc().get_index());
  if (nextpc_id == 0) { //XXX: hack: assumes nextpc0 is ret
    return nullptr;
  }
  if (this->edge_is_indirect_branch()) {
    NOT_IMPLEMENTED();
  }
  /*if (!nextpc_args_map.count(nextpc_id)) {
    return NULL;
  }*/
  pc from_pc = this->get_from_pc();
  pc to_pc = this->get_to_pc();
  to_pc.set_index(0, 0, 0);

  ASSERT(nextpc_id >= 0 && nextpc_id < cs.m_nextpc_consts.size());
  size_t num_args = 0; //nextpc_args_map.at(nextpc_id).first;
  expr_ref nextpc_id_expr = cs.m_nextpc_consts[nextpc_id];

  state to_state = this->get_to_state();
  to_state.apply_function_call(nextpc_id_expr, num_args, r_esp, r_eax, start_state);
  //shared_ptr<tfg_node> to_node = tfg->find_node(to_pc);
  //shared_ptr<tfg_node> from_node = tfg->find_node(from_pc);

  //context *ctx = this->get_cond()->get_context();
  //shared_ptr<itfg_edge> new_e = make_shared<itfg_edge>(from_pc, to_pc, to_state, this->get_cond()/*, this->get_start_state(), false, expr_false(ctx)*/);
  return mk_tfg_edge(mk_itfg_edge(from_pc, to_pc, to_state, this->get_cond()/*, state()*/, this->get_assumes(), this->get_te_comment()));
}

string
tfg_edge::to_string_with_comment() const
{
  NOT_REACHED();
  //std::function<string (itfg_edge_ref const&)> f_atom = [](itfg_edge_ref const& ie)
  //{
  //  return ie->to_string_with_comment();
  //};
  ////return this->m_ec->to_string(f_atom);
}

tfg_edge::tfg_edge(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t) : edge_with_cond<pc>(e->graph_edge_composition_get_from_pc(),
                                                                                e->graph_edge_composition_get_to_pc(),
                                                                                t.tfg_edge_composition_get_edge_cond(e),
                                                                                t.tfg_edge_composition_get_to_state(e/*, t.get_start_state()*/),
                                                                                t.tfg_edge_composition_get_assumes(e)),
                                                                                m_te_comment(t.tfg_edge_composition_get_te_comment(e))
{
  //m_ec = create_itfg_edge_composition_from_tfg_edge_composition(e, t);
}

//itfg_ec_ref
//tfg_edge::create_itfg_edge_composition_from_tfg_edge_composition(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t)
//{
//  std::function<itfg_ec_ref (edge_id_t<pc> const&)> f_atom =
//    [&t](edge_id_t<pc> const& eid)
//  {
//    shared_ptr<tfg_edge const> te = t.get_edge(eid);
//    return te->m_ec;
//  };
//  std::function<itfg_ec_ref (serpar_composition_node_t<edge_id_t<pc>>::type, itfg_ec_ref const&, itfg_ec_ref const&)> f_fold =
//    [](serpar_composition_node_t<edge_id_t<pc>>::type t, itfg_ec_ref const& a, itfg_ec_ref const& b)
//  {
//    //return make_shared<itfg_ec_node_t const>(convert_type(t), a, b);
//    return mk_itfg_ec(convert_type(t), a, b);
//  };
//  return e->visit_nodes(f_atom, f_fold);
//}

string
tfg_edge::to_string_for_eq(string const& prefix) const
{
  //string const prefix = "";
  string ret;
  ret += this->edge_with_cond<pc>::to_string_for_eq(prefix);
  ret += prefix + ".te_comment\n";
  ret += this->m_te_comment.get_string() + "\n";
  //list<itfg_edge_ref> itfg_edges = this->m_ec->get_constituent_atoms();
  //for (auto const& ie : itfg_edges) {
  //  ret += "=constituent_itfg_edge\n" + ie->to_string_for_eq();
  //}
  /*std::function<string (itfg_edge const&)> f_ie_atom =
    [](itfg_edge const& ie)
  {
    return ie.get_from_pc().to_string() + " => " + ie.get_to_pc().to_string();
  };*/
  //ret += "=itfg_ec\n" + m_ec->itfg_ec_to_string(/*f_ie_atom*/);
  return ret;
}

/*itfg_pathcond_pred_ls_t
tfg_edge::tfg_edge_get_pathcond_pred_ls_for_unsafe_cond(tfg const& t, std::function<expr_ref (itfg_edge_ref const&)> f_unsafe_cond) const
{
  context *ctx = t.get_context();
  std::function<itfg_pathcond_pred_ls_t (itfg_edge_ref const&, itfg_pathcond_pred_ls_t const&)> f_atom =
    [&t, ctx, f_unsafe_cond](itfg_edge_ref const& e, itfg_pathcond_pred_ls_t const& in)
  {
    //expr_ref cond = e->get_div_by_zero_cond();
    expr_ref cond = f_unsafe_cond(e);
    itfg_pathcond_pred_ls_t ret;
    if (cond != expr_false(ctx)) {
      predicate pred(precond_t(), cond, expr_true(ctx), "itfg_edgecond", predicate::provable);
      ret.push_back(make_pair(expr_true(ctx), pred));
    }
    for (auto const& ipp : in) {
      //shared_ptr<tfg_edge> te = make_shared<tfg_edge>(e);
      expr_ref const& icond = ipp.first;
      predicate const& ipred = ipp.second;
      //edge_id_t<pc> eid = te->get_edge_id();
      shared_ptr<edge_with_cond<pc, tfg_node> const> ewc = static_pointer_cast<edge_with_cond<pc, tfg_node> const>(e);
      ASSERT(ewc);
      shared_ptr<serpar_composition_node_t<shared_ptr<edge_with_cond<pc, tfg_node> const>>> ewc_ec = make_shared<serpar_composition_node_t<shared_ptr<edge_with_cond<pc, tfg_node> const>>>(ewc);
      predicate icond_pred(precond_t(), icond, expr_true(ctx), "icond", predicate::provable);

      //cout << __func__ << " " << __LINE__ << ": " << this->to_string() << ": before applying " << ewc->to_string() << ": ipred = " << ipred.to_string(true) << endl;
      list<pair<shared_ptr<serpar_composition_node_t<shared_ptr<edge_with_cond<pc, tfg_node> const>>>, predicate>> ipreds_apply = t.graph_apply_trans_funs_for_edges(ewc_ec, ipred, false);
      list<pair<shared_ptr<serpar_composition_node_t<shared_ptr<edge_with_cond<pc, tfg_node> const>>>, predicate>> iconds_apply = t.graph_apply_trans_funs_for_edges(ewc_ec, icond_pred, false);
      ASSERT(ipreds_apply.size() == 1);
      ASSERT(iconds_apply.size() == 1);
      predicate const& ipred_apply = ipreds_apply.front().second;
      predicate const& icond_apply = iconds_apply.front().second;
      ASSERT(icond_apply.get_precond().precond_is_trivial());
      ASSERT(icond_apply.get_rhs_expr() == expr_true(ctx));
      expr_ref new_icond = expr_and(icond_apply.get_lhs_expr(), e->get_cond());

      //cout << __func__ << " " << __LINE__ << ": " << this->to_string() << ": after applying " << ewc->to_string() << ": ipred_apply = " << ipred_apply.to_string(true) << endl;

      ret.push_back(make_pair(new_icond, ipred_apply));
    }
    return ret;
  };
  std::function<itfg_pathcond_pred_ls_t (itfg_pathcond_pred_ls_t const&, itfg_pathcond_pred_ls_t const&)> f_parallel =
    [](itfg_pathcond_pred_ls_t const& a, itfg_pathcond_pred_ls_t const& b)
  {
    itfg_pathcond_pred_ls_t ret = a;
    list_append(ret, b);
    return ret;
  };
  itfg_pathcond_pred_ls_t ret = m_ec->visit_nodes_postorder_series(f_atom, f_parallel, itfg_pathcond_pred_ls_t());

  sprel_map_pair_t const &sprel_map_pair = t.get_sprel_map_pair(this->get_from_pc());
  auto const& memlabel_map = t.get_memlabel_map();
  std::function<expr_ref (string const&, expr_ref const&)> f =
    [ctx, &sprel_map_pair, &memlabel_map](string const& name, expr_ref const& e)
  {
    return ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(e, sprel_map_pair, memlabel_map);
  };

  itfg_pathcond_pred_ls_t ret_simplified;

  for (auto const& ipp : ret) {
    expr_ref icond = ipp.first;
    predicate ipred = ipp.second;
    //cout << __func__ << " " << __LINE__ << ": e = " << this->to_string() << ", icond = " << expr_string(icond) << endl;
    icond = f("icond", icond);
    //cout << __func__ << " " << __LINE__ << ": e = " << this->to_string() << ", after simplify, icond = " << expr_string(icond) << endl;
    //cout << __func__ << " " << __LINE__ << ": e = " << this->to_string() << ", ipred = " << ipred.to_string(true) << endl;
    ipred = ipred.visit_exprs(f);
    //cout << __func__ << " " << __LINE__ << ": e = " << this->to_string() << ", after simplify, ipred = " << ipred.to_string(true) << endl;
    ret_simplified.push_back(make_pair(icond, ipred));
  }
  return ret_simplified;
}*/

predicate_set_t
tfg_edge::tfg_edge_get_pathcond_pred_ls_for_unsafe_cond(tfg const& t, std::function<predicate_set_t (expr_ref const&)> unsafe_cond) const
{
  expr_ref const& edgecond = this->get_cond();
  pc const& from_pc = this->get_from_pc();
  context *ctx = edgecond->get_context();
  predicate_set_t ret;
  std::function<expr_ref (string const&, expr_ref const&)> f_simpl = [&t, &from_pc](string const&, expr_ref const& e)
  {
    return t.expr_simplify_at_pc(e, from_pc);
  };
  std::function<void (string const&, expr_ref)> f_apply = [&ret, f_simpl, unsafe_cond](string const& k, expr_ref e)
  {
    predicate_set_t eret = unsafe_cond(e);
    for (auto const& pathcond_pred : eret) {
      //expr_ref cond_simpl = f_simpl("edgecond", pathcond_pred.first);
      predicate_ref pred_simpl = pathcond_pred->visit_exprs(f_simpl);
      //ret.push_back(make_pair(cond_simpl, pred_simpl));
      ret.insert(pred_simpl);
    }
  };
  f_apply("edgecond", edgecond);
  this->get_to_state().state_visit_exprs(f_apply);
  return ret;
}

predicate_set_t
tfg_edge::tfg_edge_get_pathcond_pred_ls_for_div_by_zero(tfg const& t) const
{
  std::function<predicate_set_t (expr_ref const&)> f = [](expr_ref const& e)
  {
    return expr_get_div_by_zero_cond(e);
  };
  return this->tfg_edge_get_pathcond_pred_ls_for_unsafe_cond(t, f);
}

predicate_set_t
tfg_edge::tfg_edge_get_pathcond_pred_ls_for_ismemlabel_heap(tfg const& t) const
{
  graph_memlabel_map_t const& memlabel_map = t.get_memlabel_map();
  std::function<predicate_set_t (expr_ref const&)> f = [&memlabel_map](expr_ref const& e)
  {
    return expr_get_memlabel_heap_access_cond(e, memlabel_map);
  };
  return this->tfg_edge_get_pathcond_pred_ls_for_unsafe_cond(t, f);
}

predicate_set_t
tfg_edge::tfg_edge_get_pathcond_pred_ls_for_is_not_memlabel(tfg const& t, graph_memlabel_map_t const& memlabel_map) const
{
  std::function<predicate_set_t (expr_ref const&)> f = [&memlabel_map](expr_ref const& e)
  {
    return expr_is_not_memlabel_access_cond(e, memlabel_map);
  };
  return this->tfg_edge_get_pathcond_pred_ls_for_unsafe_cond(t, f);
}

shared_ptr<tfg_edge const>
tfg_edge::tfg_edge_update_state(std::function<void (pc const&, state&)> update_state_fn) const
{
  state st = this->get_to_state();
  update_state_fn(this->get_from_pc(), st);
  //auto new_ec = this->get_itfg_ec()->itfg_ec_update_state(update_state_fn);
  shared_ptr<tfg_edge const> ret = mk_tfg_edge(this->get_from_pc(), this->get_to_pc(), this->get_cond(), st, this->get_assumes(), this->m_te_comment/*, new_ec*/);
  return ret;
}

shared_ptr<tfg_edge const>
tfg_edge::tfg_edge_set_state(state const& new_st) const
{
  shared_ptr<tfg_edge const> ret = mk_tfg_edge(this->get_from_pc(), this->get_to_pc(), this->get_cond(), new_st, this->get_assumes(), this->m_te_comment);
  return ret;
}

shared_ptr<tfg_edge const>
tfg_edge::visit_exprs(std::function<expr_ref (string const&, expr_ref const&)> update_expr_fn, bool opt/*, state const& start_state*/) const
{
  state new_st = this->get_to_state();
  if (opt) {
    new_st.state_visit_exprs(update_expr_fn);
  } else {
    new_st.state_visit_exprs_with_start_state(update_expr_fn/*, start_state*/);
  }
  expr_ref new_cond = update_expr_fn(G_EDGE_COND_IDENTIFIER, this->get_cond());
  unordered_set<expr_ref> new_assumes = unordered_set_visit_exprs(this->get_assumes(), update_expr_fn);
  shared_ptr<tfg_edge const> ret = mk_tfg_edge(this->get_from_pc(), this->get_to_pc(), new_cond, new_st, new_assumes, this->m_te_comment);
  return ret;
}

shared_ptr<tfg_edge const>
tfg_edge::visit_exprs(std::function<expr_ref (expr_ref const&)> update_expr_fn) const
{
  std::function<expr_ref (string const&, expr_ref)> f = [&update_expr_fn](string const& k, expr_ref e)
  {
    return update_expr_fn(e);
  };
  return this->visit_exprs(f);
}


shared_ptr<tfg_edge const>
tfg_edge::tfg_edge_update_edgecond(std::function<expr_ref (pc const&, expr_ref const&)> update_expr_fn) const
{
  expr_ref cond = this->get_cond();
  cond = update_expr_fn(this->get_from_pc(), cond);
  //auto new_ec = this->get_itfg_ec()->itfg_ec_update_edgecond(update_expr_fn);
  shared_ptr<tfg_edge const> ret = mk_tfg_edge(this->get_from_pc(), this->get_to_pc(), cond, this->get_to_state(), this->get_assumes(), this->m_te_comment/*, new_ec*/);
  return ret;
}

shared_ptr<tfg_edge const>
tfg_edge::tfg_edge_update_assumes(unordered_set<expr_ref> const& new_assumes) const
{
  shared_ptr<tfg_edge const> ret = mk_tfg_edge(this->get_from_pc(), this->get_to_pc(), this->get_cond(), this->get_to_state(), new_assumes, this->m_te_comment);
  return ret;
}

shared_ptr<tfg_edge const>
tfg_edge::substitute_variables_at_input(state const &start_state, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, context *ctx) const
{
  std::function<expr_ref (pc const&, expr_ref const&)> f = [ctx, &submap](pc const&p, expr_ref const& e)
  {
    return ctx->expr_substitute(e, submap);
  };
  auto new_e = this->tfg_edge_update_edgecond(f);
  std::function<void (pc const&, state&)> fs = [/*&start_state, */&submap](pc const&p, state &s)
  {
    //std::function<expr_ref (string const&, expr_ref)> f = [&submap](string const& s, expr_ref e)
    //{
    //  context* ctx = e->get_context();
    //  return ctx->expr_substitute(e, submap);
    //};
    s.substitute_variables_using_state_submap(/*start_state, */state_submap_t(submap));
    //s.state_visit_exprs(f);
  };
  //ASSERT(this->tfg_edge_is_atomic());
  //ASSERT(m_ec->get_type() == itfg_ec_node_t::atom);
  new_e = new_e->tfg_edge_update_state(fs);
  return new_e;
}

//void
//tfg_edge::edge_visit_exprs_const(std::function<void (pc const&, string const&, expr_ref)> f) const
//{
//  this->edge_with_cond<pc>::edge_visit_exprs_const(f);
//}

tfg_edge_ref
tfg_edge::empty(pc const& p)
{
  return mk_tfg_edge(p, p, nullptr, state(), {}, te_comment_t::te_comment_epsilon());
}

tfg_edge_ref
tfg_edge::empty()
{
  return mk_tfg_edge(pc::start(), pc::start(), nullptr, state(), {}, te_comment_t::te_comment_epsilon());
}

string
tfg_edge::graph_edge_from_stream(istream &in, string const& first_line, string const &prefix/*, state const &start_state*/, context* ctx, shared_ptr<tfg_edge const>& te)
{
  pc from_pc, to_pc;
  //string comment;
  state state_to;
  unordered_set<expr_ref> assumes;
  expr_ref cond;
  //context *ctx = this->get_context();
  //cout << __func__ << " " << __LINE__ << ": first_line = " << first_line << ".\n";
  string line = edge_with_cond<pc>::read_edge_with_cond_using_start_state(in, first_line, prefix/*, start_state*/, ctx, from_pc, to_pc, cond, state_to, assumes);
  bool end;
  //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
  ASSERTCHECK(is_line(line, prefix + ".te_comment"), cout << "line = " << line << endl; fflush(stdout));
  end = !getline(in, line);
  ASSERT(!end);
  vector<string> te_comment_fields = splitOnChar(line, ':');
  ASSERT(te_comment_fields.size() == 5);
  ASSERT(stringIsInteger(te_comment_fields.at(0)));
  bbl_order_descriptor_t bo(string_to_int(te_comment_fields.at(0)), te_comment_fields.at(1));
  ASSERT(stringIsInteger(te_comment_fields.at(2)));
  bool is_phi = string_to_int(te_comment_fields.at(2));
  ASSERT(stringIsInteger(te_comment_fields.at(3)));
  int insn_idx = string_to_int(te_comment_fields.at(3));
  string comment_str = te_comment_fields.at(4);
  //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
  end = !getline(in, line);
  ASSERT(!end);
  while (line != "" && line.at(0) != '=') {
    comment_str += "\n" + line;
    end = !getline(in, line);
    ASSERT(!end);
  }
  te_comment_t te_comment(bo, is_phi, insn_idx, comment_str);
  //cout << __func__ << " " << __LINE__ << ": parsed te_comment\n" << te_comment.get_string() << endl;
  //ASSERT(comment == "");
  //ASSERT(is_line(line, "=constituent_itfg_edge"));
  //list<itfg_edge_ref> itfg_edges;
  //while (is_line(line, "=constituent_itfg_edge")) {
  //  pc ifrom_pc, ito_pc;
  //  string icomment;
  //  state istate_to;
  //  expr_ref icond;

  //  bool end = !getline(in, line);
  //  ASSERT(!end);
  //  line = read_edge_with_cond_using_start_state(in, line, ctx, "=itfg_edge", start_state, ifrom_pc, ito_pc, icond, istate_to);
  //  ASSERT(is_line(line, "=itfg_edge.Comment"));
  //  end = !getline(in, line);
  //  ASSERT(!end);
  //  ASSERT(line.size() >= 1);
  //  if (line.at(0) != '=') {
  //    icomment = line;
  //    end = !getline(in, line);
  //    ASSERT(!end);
  //  }
  //  itfg_edges.push_back(mk_itfg_edge(ifrom_pc, ito_pc, istate_to, icond, start_state, icomment));
  //}
  //if (!is_line(line, "=itfg_ec")) {
  //  cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
  //}
  //ASSERT(is_line(line, "=itfg_ec"));
  //end = !getline(in, line);
  //ASSERT(!end);

  //std::function<itfg_edge_ref (string const&)> f =
  //  [&itfg_edges](string const& str)
  //{
  //  pc p1, p2;
  //  string comment;
  //  ASSERT(str.at(0) == '(');
  //  ASSERT(str.at(str.size() - 1) == ')');
  //  string s = str.substr(1, str.size() - 2);
  //  edge_read_pcs_and_comment(s, p1, p2, comment);
  //  ASSERT(comment == "");
  //  for (auto const& ie : itfg_edges) {
  //    if (   ie->get_from_pc() == p1
  //        && ie->get_to_pc() == p2) {
  //      //cout << __func__ << " " << __LINE__ << ": ie = " << ie.to_string_for_eq() << ".\n";
  //      return ie;
  //    }
  //  }
  //  NOT_REACHED();
  //};
  //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
  //ec = serpar_composition_node_t<itfg_edge>::serpar_composition_node_create_from_string(line, f);
  //itfg_ec_ref ec = itfg_ec_node_t::itfg_ec_node_create_from_string(line, f);
  te = mk_tfg_edge(from_pc, to_pc, cond, state_to, assumes, te_comment);
  //end = !getline(in, line);
  //ASSERT(!end);
  while (line == "") {
    bool end = !getline(in, line);
    ASSERT(!end);
  }
  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  return line;
}

tfg_edge_ref
mk_tfg_edge(itfg_edge_ref const& ie)
{
  tfg_edge te(ie);
  return tfg_edge::get_tfg_edge_manager()->register_object(te);
}

tfg_edge_ref
mk_tfg_edge(shared_ptr<tfg_edge_composition_t> const& e, tfg const& t)
{
  tfg_edge te(e, t);
  return tfg_edge::get_tfg_edge_manager()->register_object(te);
}

tfg_edge_ref
mk_tfg_edge(pc const& from_pc, pc const& to_pc, expr_ref const& cond, state const& to_state, unordered_set<expr_ref> const& assumes, te_comment_t const& te_comment)
{
  tfg_edge te(from_pc, to_pc, cond, to_state, assumes, te_comment);
  return tfg_edge::get_tfg_edge_manager()->register_object(te);
}

}
