#pragma once
#include "tfg/predicate_set.h"

namespace eqspace {

//template<predicate_set_t GET_UNSAFE_CONDS_FROM_EDGECOND_AND_TO_STATE(shared_ptr<tfg_edge_composition_t> const&, tfg const&, corr_graph const&, pcpair const&)>
class unsafe_condition_t
{
private:
  std::function<predicate_set_t (shared_ptr<tfg_edge_composition_t> const&, tfg const&, corr_graph const&, pcpair const&)> m_get_unsafe_conds_from_tfg_ec;
public:
  unsafe_condition_t(std::function<predicate_set_t (shared_ptr<tfg_edge_composition_t> const&, tfg const&, corr_graph const&, pcpair const&)> get_unsafe_conds_from_tfg_ec) : m_get_unsafe_conds_from_tfg_ec(get_unsafe_conds_from_tfg_ec)
  { }

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
  expr_contained_in_set(predicate_ref const& needle, predicate_set_t const& haystack, corr_graph const& cg, pcpair const& pp, int dstnum)
  {
    tfg const& src_tfg = cg.get_src_tfg();
    tfg const& dst_tfg = cg.get_dst_tfg();
    context *ctx = cg.get_context();

    predicate_ref dst_pred = needle->flatten_edge_guard_using_conjunction(dst_tfg);
    size_t srcnum = 0;
    for (auto const &helem : haystack) {
      bool guess_made_weaker;
      predicate_ref src_pred = helem->flatten_edge_guard_using_conjunction(src_tfg); //taking a copy ensures that decide_hoare_triple() does not override the proof status of helem's unsafe_cond
      if (cg.graph_with_proofs<pcpair, corr_graph_node, corr_graph_edge, predicate>::decide_hoare_triple({dst_pred}, pp, mk_epsilon_ec<pcpair,corr_graph_edge>(), src_pred, guess_made_weaker) == proof_status_proven) {
        DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": dst pred num " << dstnum << " cancelled by src pred num " << srcnum << endl);
        return true;
      }
      srcnum++;
    }
    if (haystack.size() == 0) {
      //predicate const& unsafe_cond = needle.get_unsafe_cond();
      bool guess_made_weaker;
      if (cg.graph_with_proofs<pcpair, corr_graph_node, corr_graph_edge, predicate>::decide_hoare_triple({dst_pred}, pp, mk_epsilon_ec<pcpair,corr_graph_edge>(), predicate::false_predicate(ctx), guess_made_weaker) == proof_status_proven) {
        return true;
      }
    }
    return false;
  }

  static predicate_set_t
  wp_pred(tfg const& t, shared_ptr<tfg_edge_composition_t> const& ec, predicate_ref const& in_pred)
  {
    predicate_set_t ret;
    //shared_ptr<tfg_edge_composition_t> const& in_path = in_expr.get_path();
    //predicate const& in_pred = in_expr.get_unsafe_cond();
    list<pair<edge_guard_t, predicate_ref>> preds_apply = t.apply_trans_funs(ec, in_pred, true);
    for (auto epred_apply : preds_apply) {
      edge_guard_t eg = epred_apply.first;
      predicate_ref pred_apply = epred_apply.second;
      edge_guard_t const& psg = pred_apply->get_edge_guard();
      eg.add_guard_in_series(psg);
      pred_apply = pred_apply->set_edge_guard(eg);
      ret.insert(pred_apply);
      //shared_ptr<tfg_edge_composition_t> const& pred_ec = pred_eg.get_edge_composition();
      //shared_ptr<tfg_edge_composition_t> full_ec = mk_series(pred_ec, in_path);
      //ret.push_back(guarded_expr_t(full_ec, pred_apply.second));
    }
    return ret;
  }

  static list<predicate_set_t>
  wp_preds(tfg const& t, shared_ptr<tfg_edge_composition_t> const& ec, predicate_set_t const& in_exprs)
  {
    list<predicate_set_t> ret;
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
  static predicate_set_t src_xferred_preds_meet(predicate_set_t const& src_xferred_exprs)
  {
    predicate_set_t ret = src_xferred_exprs;
    predicate_set_meet(ret);
    return ret;
  }

  static predicate_set_t src_wp_preds(tfg const& src_tfg, shared_ptr<tfg_edge_composition_t> const& src_ec, predicate_set_t const& in)
  {
    list<predicate_set_t> edge_xferred_exprs = wp_preds(src_tfg, src_ec, in);
    predicate_set_t edge_xferred_exprs_flattened;
    for (auto const& ge : edge_xferred_exprs) {
      predicate::predicate_set_union(edge_xferred_exprs_flattened, ge);
    }
    predicate_set_t merged_exprs = src_xferred_preds_meet(edge_xferred_exprs_flattened);
    return merged_exprs;
  }

  static predicate_set_t dst_wp_preds(tfg const& dst_tfg, shared_ptr<tfg_edge_composition_t> const& dst_ec, predicate_set_t const& in, bool &some_pred_has_traveled_across_a_cycle_without_getting_cancelled)
  {
    list<predicate_set_t> edge_xferred_exprs = wp_preds(dst_tfg, dst_ec, in);
    predicate_set_t ret;
    for (auto const& ps : edge_xferred_exprs) {
      unordered_set_union(ret, ps);
      // READ THIS BEFORE UNCOMMENTING FOLLOWING LINE:
      /* predicate_set_union performs edgeguard based dedup'ing as well which
       * is undesirable because our check for
       * some_pred_has_traveled_across_a_cycle_without_getting_cancelled relies
       * on implying edgeguard check, i.e., if two predicates have implying
       * edgeguards then we consider both to be same prediate, one of which
       * travelled an iteration (or more) more than the other.
       */
      //predicate_set_union(ret, ps);
    }
    if (predicate_set_meet(ret)) {
      some_pred_has_traveled_across_a_cycle_without_getting_cancelled = true;
    }
    return ret;
  }
public: 
  predicate_set_t
  xfer_src(predicate_set_t const& in, shared_ptr<corr_graph_edge const> const& e, corr_graph const& cg) const
  {
    //look at to_state and econd to identify any divisions on src expression; put the divisor into the returned set
    //expr_ref src_cond = e->get_src_cond(cg);
    //state const& src_to_state = e->get_src_to_state(cg);
    //return retain_true_guard_exprs(GET_UNSAFE_CONDS_FROM_EDGECOND_AND_TO_STATE(src_cond, src_to_state, cg, e));
    //return retain_true_guard_exprs(GET_UNSAFE_CONDS_FROM_EDGECOND_AND_TO_STATE(e->get_src_edge(), cg, e));
    tfg const& src_tfg = cg.get_src_tfg();
    pcpair const& pp = e->get_from_pc();
    //cout << __func__ << " " << __LINE__ << ": src_tfg =\n" << src_tfg.graph_to_string() << endl;
    predicate_set_t edge_src_exprs = m_get_unsafe_conds_from_tfg_ec(e->get_src_edge(), src_tfg, cg, pp);
    predicate_set_t in_xferred = src_wp_preds(src_tfg, e->get_src_edge(), in);
    predicate::predicate_set_union(edge_src_exprs, in_xferred);
    return edge_src_exprs;
  }
  predicate_set_t
  xfer_dst(predicate_set_t const& dst_exprs_in, shared_ptr<corr_graph_edge const> const& e, corr_graph const& cg, predicate_set_t const& src_exprs_in, bool& some_pred_has_traveled_across_a_cycle_without_getting_cancelled) const
  {
    //look at to_state and econd to identify any divisions on dst expression; put the divisor into the returned set
    //among the expressions in the returned set, remove those that are equal to one of the src_exprs or one of the src-divisions on this edge (will require proof queries)
    //expr_ref dst_cond = e->get_dst_cond(cg);
    //state const& dst_to_state = e->get_dst_to_state(cg);
    tfg const& src_tfg = cg.get_src_tfg();
    tfg const& dst_tfg = cg.get_dst_tfg();
    pcpair const& pp = e->get_from_pc();
    predicate_set_t dst_exprs_on_edge = m_get_unsafe_conds_from_tfg_ec(e->get_dst_edge(), dst_tfg, cg, pp);

    //expr_ref src_cond = e->get_src_cond(cg);
    //state const& src_to_state = e->get_src_to_state(cg);
    DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": getting unsafe conditions for src edge : " << e->get_src_edge()->graph_edge_composition_to_string() << endl);
    predicate_set_t src_exprs_on_edge = m_get_unsafe_conds_from_tfg_ec(e->get_src_edge(), src_tfg, cg, pp);
    DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": src_exprs_on_edge.size() = " << src_exprs_on_edge.size() << endl);

    predicate_set_t dst_exprs_out = dst_wp_preds(dst_tfg, e->get_dst_edge(), dst_exprs_in, some_pred_has_traveled_across_a_cycle_without_getting_cancelled);
    predicate_set_t src_exprs_out = src_wp_preds(src_tfg, e->get_src_edge(), src_exprs_in);

    predicate_set_t dst_exprs_all = dst_exprs_out;
    unordered_set_union(dst_exprs_all, dst_exprs_on_edge);

    if (dst_exprs_all.size() == 0) {
      return {};
    }

    predicate_ref src_exprs_on_edge_combined = predicate_set_or(src_exprs_on_edge, src_tfg);

    DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": e = " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << ": dst_exprs_all.size() = " << dst_exprs_all.size() << ", dst_exprs_in.size() = " << dst_exprs_in.size() << ", dst_exprs_out.size() = " << dst_exprs_out.size() << ", dst_exprs_on_edge.size() = " << dst_exprs_on_edge.size() << endl);
    DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": e = " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << ": src_exprs_in.size() = " << src_exprs_in.size() << ", src_exprs_out.size() = " << src_exprs_out.size() << ", src_exprs_on_edge.size() = " << src_exprs_on_edge.size() << endl);
    DYN_DEBUG2(safety, cout << __func__ << " " << __LINE__ << ": dst_exprs_all =\n" << predicate_set_to_string(dst_exprs_all) << endl);
    DYN_DEBUG2(safety, cout << __func__ << " " << __LINE__ << ": src_exprs_on_edge =\n" << predicate_set_to_string(src_exprs_on_edge) << endl);
    DYN_DEBUG2(safety, cout << __func__ << " " << __LINE__ << ": src_exprs_on_edge_combined =\n" << src_exprs_on_edge_combined->to_string(true) << endl);
    DYN_DEBUG2(safety, cout << __func__ << " " << __LINE__ << ": src_exprs_out =\n" << predicate_set_to_string(src_exprs_out) << endl);
    size_t dstnum = 0;
    predicate_set_t ret;
    for (auto const& dst_guarded_expr : dst_exprs_all) {
      DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": checking dst_pred\n" << dst_guarded_expr->to_string_for_eq() << endl);
      //for the src exprs appearing on this edge, use their combined disjunction (for more robustness); for others, check each one individually (to avoid huge expression sizes at the cost of robustness)
      if (   !expr_contained_in_set(dst_guarded_expr, {src_exprs_on_edge_combined}, cg, e->get_from_pc(), dstnum)
          && !expr_contained_in_set(dst_guarded_expr, src_exprs_out, cg, e->get_from_pc(), dstnum)) {
        DYN_DEBUG(safety, cout << __func__ << " " << __LINE__ << ": adding dst_guarded_expr: " << dst_guarded_expr->to_string(true) << endl);
        ret.insert(dst_guarded_expr);
      }
      dstnum++;
    }
    DYN_DEBUG(safety, cout << _FNLN_ << ": e = " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << ": returning ret.size() = " << ret.size() << endl);
    return ret;
  }
};

predicate_set_t
get_unsafe_expressions_using_generator_func(shared_ptr<tfg_edge_composition_t> const& tfg_ec, tfg const& t, corr_graph const& cg, pcpair const& pp, std::function<predicate_set_t (tfg_edge_ref const&)> generator_func);

}
