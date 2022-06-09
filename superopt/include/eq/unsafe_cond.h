#pragma once
#include "gsupport/predicate_set.h"
#include "gsupport/graph_ec.h"

namespace eqspace {

//template<predicate_set_t GET_UNSAFE_CONDS_FROM_EDGECOND_AND_TO_STATE(shared_ptr<tfg_edge_composition_t> const&, tfg const&, corr_graph const&/*, pcpair const&*/)>
class unsafe_condition_t
{
private:
  //std::function<predicate_set_t (shared_ptr<tfg_edge_composition_t> const&, tfg const&, corr_graph const&, pcpair const&)> m_get_unsafe_conds_from_tfg_ec;
  std::function<list<guarded_pred_t> (shared_ptr<tfg_edge_composition_t> const&, tfg const&/*, corr_graph const&, pcpair const&*/)> m_get_unsafe_conds_from_tfg_ec;
public:
  unsafe_condition_t(std::function<list<guarded_pred_t> (shared_ptr<tfg_edge_composition_t> const&, tfg const&/*, corr_graph const&, pcpair const&*/)> get_unsafe_conds_from_tfg_ec) : m_get_unsafe_conds_from_tfg_ec(get_unsafe_conds_from_tfg_ec)
  { }

  list<guarded_pred_t> xfer_src(list<guarded_pred_t> const& in, dshared_ptr<corr_graph_edge const> const& e, corr_graph const& cg) const;

  list<guarded_pred_t> xfer_dst(list<guarded_pred_t> const& dst_exprs_in, dshared_ptr<corr_graph_edge const> const& e, corr_graph const& cg, list<guarded_pred_t> const& src_exprs_in, bool& some_pred_has_traveled_across_a_cycle_without_getting_cancelled) const;


private:
  //static list<guarded_expr_t> retain_true_guard_exprs(list<guarded_expr_t> const& guarded_exprs)
  //{
  //  list<guarded_expr_t> ret;
  //  for (auto const& guarded_expr : guarded_exprs) {
  //    if (guarded_expr.has_true_cond()) {
  //      ret.push_back(guarded_expr);
  //    }
  //  }
  //  return ret;
  //}

  static bool
  expr_semantically_contained_in_set(guarded_pred_t const& needle, list<guarded_pred_t> const& haystack, corr_graph const& cg, pcpair const& pp, int dstnum)
  {
    tfg const& src_tfg = cg.get_src_tfg();
    tfg const& dst_tfg = cg.get_dst_tfg();
    context *ctx = cg.get_context();
    //auto epsilon_ec = graph_edge_composition_t<pcpair,corr_graph_edge>::mk_epsilon_ec();

    //predicate_ref dst_pred = needle->flatten_edge_guard_using_conjunction(dst_tfg);
    guarded_pred_t dst_pred = needle;
    //predicate_ref dst_pred_ref = get_predicate_from_guarded_pred(dst_pred);
    size_t srcnum = 0;
    for (auto const &helem : haystack) {
      bool guess_made_weaker;
      //predicate_ref src_pred = helem->flatten_edge_guard_using_conjunction(src_tfg); //taking a copy ensures that decide_hoare_triple() does not override the proof status of helem's unsafe_cond
      guarded_pred_t src_pred = helem; //taking a copy ensures that decide_hoare_triple() does not override the proof status of helem's unsafe_cond
      predicate_ref src_pred_ref = get_predicate_from_guarded_pred(src_pred);
      if (cg.graph_with_proofs<pcpair, corr_graph_node, corr_graph_edge, predicate>::decide_hoare_triple({dst_pred}, pp, graph_edge_composition_t<pcpair,corr_graph_edge>::mk_epsilon_ec(), src_pred_ref, guess_made_weaker, reason_for_counterexamples_t::inductive_invariants()) == proof_status_proven) {
        DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": dst pred num " << dstnum << " cancelled by src pred num " << srcnum << endl);
        return true;
      }
      srcnum++;
    }
    if (haystack.size() == 0) {
      //predicate const& unsafe_cond = needle.get_unsafe_cond();
      bool guess_made_weaker;
      if (cg.graph_with_proofs<pcpair, corr_graph_node, corr_graph_edge, predicate>::decide_hoare_triple({dst_pred}, pp, graph_edge_composition_t<pcpair,corr_graph_edge>::mk_epsilon_ec(), predicate::false_predicate(ctx), guess_made_weaker, reason_for_counterexamples_t::inductive_invariants()) == proof_status_proven) {
        return true;
      }
    }
    return false;
  }

  static list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>>
  wp_pred(tfg const& t, shared_ptr<tfg_edge_composition_t> const& ec, guarded_pred_t const& in_pred)
  {
    list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> ret;
    //predicate_set_t ret;
    context* ctx = t.get_context();
    //shared_ptr<tfg_edge_composition_t> const& in_path = in_expr.get_path();
    //predicate const& in_pred = in_expr.get_unsafe_cond();
    list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> preds_apply = t.apply_trans_funs_simplified(ec, in_pred.second);
    for (auto epred_apply : preds_apply) {
      graph_edge_composition_ref<pc,tfg_edge> eg = epred_apply.first;
      predicate_ref pred_apply = epred_apply.second;
      eg = graph_edge_composition_t<pc,tfg_edge>::mk_series(eg, in_pred.first);
      ret.push_back(make_pair(eg, pred_apply));
      //edge_guard_t const& psg = pred_apply->get_edge_guard();
      //eg.add_guard_in_series(psg);
      //pred_apply = pred_apply->set_edge_guard(eg);
      //expr_ref eg_expr = t.graph_edge_composition_get_simplified_edge_cond(eg, ctx);
      //pred_apply = pred_apply->predicate_add_guard_expr(eg_expr);
      //ret.insert(pred_apply);
      //shared_ptr<tfg_edge_composition_t> const& pred_ec = pred_eg.get_edge_composition();
      //shared_ptr<tfg_edge_composition_t> full_ec = mk_series(pred_ec, in_path);
      //ret.push_back(guarded_expr_t(full_ec, pred_apply.second));
    }
    return ret;
    //return preds_apply;
  }

  static list<list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>>>
  wp_preds(tfg const& t, shared_ptr<tfg_edge_composition_t> const& ec, list<guarded_pred_t> const& in_exprs)
  {
    list<list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>>> ret;
    for (auto const& in_expr : in_exprs) {
      ret.push_back(wp_pred(t, ec, in_expr));
    }
    return ret;
    /*itfg_ec_ref itfg_ec = tfg_edge::create_itfg_edge_composition_from_tfg_edge_composition(ec, t);
    list<guarded_expr_t> out_exprs;
    for (auto const& in_expr : in_exprs) {
      itfg_ec_ref const& cond = in_expr.get_cond();
      expr_ref const& e = in_expr.get_expr();
      itfg_ec_ref out_cond = mk_itfg_ec_series(itfg_ec, cond);
      expr_ref out_expr = t.apply_trans_funs
    }
    return out_exprs;*/
  }

  //src_xferred_exprs_meet_and_merge : because we check dst preds for containment in one of the src preds (overapproximately),
  //it is important that we merge each group into a single guarded_expr (to get better results); this should have already happened before this function is called. This function performs meet which is necessary to ensure termination.
  static list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> src_xferred_preds_meet(list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> const& src_xferred_exprs)
  {
    list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> /*predicate_set_t */ret = src_xferred_exprs;
    guarded_predicate_set_meet(ret);
    return ret;
  }

  static predicate_ref unsafe_cond_guarded_predicate_set_or(list<guarded_pred_t> const& pset, tfg const& t);
  static expr_ref unsafe_cond_get_expr_from_guarded_pred_by_conjuncting_guard(guarded_pred_t const& guarded_expr, tfg const& t);

  static list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> src_wp_preds(tfg const& src_tfg, shared_ptr<tfg_edge_composition_t> const& src_ec, list<guarded_pred_t> const& in);
  static list<guarded_pred_t> dst_wp_preds(tfg const& dst_tfg, shared_ptr<tfg_edge_composition_t> const& dst_ec, list<guarded_pred_t> const& in, bool &some_pred_has_traveled_across_a_cycle_without_getting_cancelled);
};

}
