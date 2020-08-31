#include "eq/corr_graph.h"
#include "tfg/tfg.h"
#include "eq/unsafe_cond.h"

namespace eqspace {

static predicate_set_t
get_unsafe_expressions(shared_ptr<tfg_edge_composition_t> const& tfg_ec, std::function<predicate_set_t (tfg_edge_ref const&, predicate_set_t const&)> f_atom)
{
  std::function<predicate_set_t (predicate_set_t const&, predicate_set_t const&)> f_fold_parallel =
    [](predicate_set_t const& a, predicate_set_t const& b)
  {
    predicate_set_t ret = a;
    predicate::predicate_set_union(ret, b);
    //cout << __func__ << " " << __LINE__ << ": a.size() = " << a.size() << ", b.size() = " << b.size() << ", ret.size() = " << ret.size() << endl;
    return ret;
    //return guarded_expr_t::guarded_expr_ls_meet(a, b);
    //itfg_pathcond_pred_ls_t ret = a;
    //list_append(ret, b);
    //return ret;
  };
  return tfg_ec->visit_nodes_postorder_series(f_atom, f_fold_parallel, predicate_set_t());
}



predicate_set_t
get_unsafe_expressions_using_generator_func(shared_ptr<tfg_edge_composition_t> const& tfg_ec, tfg const& t, corr_graph const& cg, pcpair const& pp, std::function<predicate_set_t (tfg_edge_ref const&)> generator_func)
{
  context *ctx = t.get_context();
  std::function<predicate_set_t (tfg_edge_ref const&, predicate_set_t const&)> f_atom =
    [&t, generator_func](tfg_edge_ref const& te, predicate_set_t const& in)
  {
    if (te->is_empty()) {
      return in;
    }
    //expr_ref edgecond = t.graph_get_simplified_edgecond(te);
    predicate_set_t te_ret = generator_func(te);
    for (auto const& pred : in) {
      list<pair<edge_guard_t, predicate_ref>> preds_apply = t.apply_trans_funs(mk_edge_composition<pc,tfg_edge>(te), pred, true);
      ASSERT(preds_apply.size() == 1);
      auto const& epred_apply = preds_apply.front();
      edge_guard_t eg = epred_apply.first;
      predicate_ref pred_apply = epred_apply.second;
      edge_guard_t psg = pred_apply->get_edge_guard();
      eg.add_guard_in_series(psg);
      pred_apply = pred_apply->set_edge_guard(eg);
      te_ret.insert(pred_apply);
    }
    //if (te_ret.size()) {
    //  cout << __func__ << " " << __LINE__ << ": returning list of size " << te_ret.size() << " at edge " << te->to_string() << endl;
    //  print_preds(te_ret);
    //}
    return te_ret;
  };
  predicate_set_t preds = get_unsafe_expressions(tfg_ec, f_atom);
  return preds;
  //list<guarded_expr_t> ret;
  //for (auto const& ipp : ls) {
  //  predicate new_potential_pred = ipp.second;
  //  if (pathcond != expr_true(ctx)) {
  //    new_potential_pred = predicate(precond_t(), ctx->mk_and(pathcond, ctx->mk_eq(new_potential_pred.get_lhs_expr(), new_potential_pred.get_rhs_expr())), expr_true(ctx), new_potential_pred.get_comment(), predicate::provable);
  //  }
  //  ret.push_back(guarded_expr_t(mk_epsilon_ec<pc, tfg_node, tfg_edge>(), new_potential_pred));
  //}
  //return ret;
}

}
