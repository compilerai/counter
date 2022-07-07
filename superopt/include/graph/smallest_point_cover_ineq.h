#pragma once

#include "graph/smallest_point_cover.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_ineq_t : public smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>
{
private:
  //all inclusive
  mutable bv_const m_lower_bound_signed_cur, m_upper_bound_signed_cur;
  mutable bv_const m_lower_bound_unsigned_cur, m_upper_bound_unsigned_cur;

  mutable bv_const m_lower_bound_signed_known, m_upper_bound_signed_known;
  mutable bv_const m_lower_bound_unsigned_known, m_upper_bound_unsigned_known;

  mutable set<bv_const> m_distinct_stable_values_for_lower_bound_signed_known;
  mutable set<bv_const> m_distinct_stable_values_for_upper_bound_signed_known;
  mutable set<bv_const> m_distinct_stable_values_for_lower_bound_unsigned_known;
  mutable set<bv_const> m_distinct_stable_values_for_upper_bound_unsigned_known;

  //consts
  size_t m_bvlen;
  bv_const m_min_lower_bound_signed, m_max_upper_bound_signed, m_min_lower_bound_unsigned, m_max_upper_bound_unsigned;
  static const int MAX_STABLE_VALUES_SEEN_BEFORE_WIDENING = 4;
public:
  smallest_point_cover_ineq_t(point_set_t const &point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, expr_group_ref const& global_eg_at_p, context *ctx) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(point_set, out_of_bound_exprs, global_eg_at_p, ctx)
  {
    this->compute_params();
  }

  smallest_point_cover_ineq_t(istream& is, string const& inprefix, context* ctx, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(is, inprefix, ctx, point_set, out_of_bound_exprs)
  {
    this->compute_params();
  }

  smallest_point_cover_ineq_t(smallest_point_cover_ineq_t const& other, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(other, point_set, out_of_bound_exprs),
   m_lower_bound_signed_cur(other.m_lower_bound_signed_cur), m_upper_bound_signed_cur(other.m_upper_bound_signed_cur),
   m_lower_bound_unsigned_cur(other.m_lower_bound_unsigned_cur), m_upper_bound_unsigned_cur(other.m_upper_bound_unsigned_cur),
   m_lower_bound_signed_known(other.m_lower_bound_signed_known), m_upper_bound_signed_known(other.m_upper_bound_signed_known),
   m_lower_bound_unsigned_known(other.m_lower_bound_unsigned_known), m_upper_bound_unsigned_known(other.m_upper_bound_unsigned_known),
   m_distinct_stable_values_for_lower_bound_signed_known(other.m_distinct_stable_values_for_lower_bound_signed_known),
   m_distinct_stable_values_for_upper_bound_signed_known(other.m_distinct_stable_values_for_upper_bound_signed_known),
   m_distinct_stable_values_for_lower_bound_unsigned_known(other.m_distinct_stable_values_for_lower_bound_unsigned_known),
   m_distinct_stable_values_for_upper_bound_unsigned_known(other.m_distinct_stable_values_for_upper_bound_unsigned_known)
  { }

  virtual dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> clone(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const override
  {
    return make_dshared<smallest_point_cover_ineq_t<T_PC, T_N, T_E, T_PRED>>(*this, point_set, out_of_bound_exprs);
  }

  virtual void smallest_point_cover_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type ineq";
    os << "=" + prefix + "\n";
    this->smallest_point_cover_to_stream_helper(os, prefix + " ");
  }

  /*static string smallest_point_cover_ineq_from_stream(istream& is, context* ctx, list<nodece_ref<T_PC, T_N, T_E>> const &counterexamples, smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>*& eqclass)
  {
    return smallest_point_cover_from_stream_helper(is, ctx, counterexamples, eqclass);
  }*/

private:
  //returns TRUE if we are done
  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>> &preds) const override
  {
    if (last_proof_success) {
      ASSERT(bv_cmp(m_lower_bound_signed_known, m_lower_bound_signed_cur) <= 0 || bv_cmp(m_lower_bound_signed_cur, m_min_lower_bound_signed) == 0);
      ASSERT(bv_cmp(m_upper_bound_signed_cur, m_upper_bound_signed_known) <= 0 || bv_cmp(m_upper_bound_signed_cur, m_max_upper_bound_signed) == 0);
      ASSERT(bv_cmp(m_lower_bound_unsigned_known, m_lower_bound_unsigned_cur) <= 0 || bv_cmp(m_lower_bound_unsigned_cur, m_min_lower_bound_unsigned) == 0);
      ASSERT(bv_cmp(m_upper_bound_unsigned_cur, m_upper_bound_unsigned_known) <= 0 || bv_cmp(m_upper_bound_unsigned_cur, m_max_upper_bound_unsigned) == 0);

      m_lower_bound_signed_known = m_lower_bound_signed_cur;
      m_upper_bound_signed_known = m_upper_bound_signed_cur;
      m_lower_bound_unsigned_known = m_lower_bound_unsigned_cur;
      m_upper_bound_unsigned_known = m_upper_bound_unsigned_cur;

      if (!this->propose_cur_bounds_and_gen_preds(preds)) {
        m_distinct_stable_values_for_lower_bound_signed_known.insert(m_lower_bound_signed_known);
        m_distinct_stable_values_for_upper_bound_signed_known.insert(m_upper_bound_signed_known);
        m_distinct_stable_values_for_lower_bound_unsigned_known.insert(m_lower_bound_unsigned_known);
        m_distinct_stable_values_for_upper_bound_unsigned_known.insert(m_upper_bound_unsigned_known);
        return true;
      }
      return false;
    }

    if (this->m_timeout_info.timeout_occurred()) {
      cout << _FNLN_ << ": WRN: ineq pred query timed out!\n";
      preds = {};
      return true;
    }
    this->propose_cur_bounds_and_gen_preds(preds);
    return false;
  }

  void compute_params()
  {
    expr_group_ref const& exprs_to_be_correlated = this->get_exprs_to_be_correlated();
    m_bvlen = exprs_to_be_correlated->get_max_bvlen();
    m_min_lower_bound_signed = bv_neg(bv_exp2(m_bvlen - 1));
    m_max_upper_bound_signed = bv_exp2(m_bvlen - 1) - 1;
    m_min_lower_bound_unsigned = 0;
    m_max_upper_bound_unsigned = bv_exp2(m_bvlen) - 1;

    ASSERT(exprs_to_be_correlated->expr_group_get_num_exprs() == 1);
    m_lower_bound_signed_known = m_min_lower_bound_signed;
    m_upper_bound_signed_known = m_max_upper_bound_signed;
    m_lower_bound_unsigned_known = m_min_lower_bound_unsigned;
    m_upper_bound_unsigned_known = m_max_upper_bound_unsigned;

    m_distinct_stable_values_for_lower_bound_signed_known.insert(m_lower_bound_signed_known);
    m_distinct_stable_values_for_upper_bound_signed_known.insert(m_upper_bound_signed_known);
    m_distinct_stable_values_for_lower_bound_unsigned_known.insert(m_lower_bound_unsigned_known);
    m_distinct_stable_values_for_upper_bound_unsigned_known.insert(m_upper_bound_unsigned_known);
  }

  bool
  propose_cur_bounds_and_gen_preds(unordered_set<shared_ptr<T_PRED const>> &preds) const
  {
    bool ret = this->propose_cur_bounds();
    preds = gen_preds_from_cur_bounds();
    return ret;
  }

  unordered_set<shared_ptr<T_PRED const>>
  gen_preds_from_cur_bounds() const
  {
    unordered_set<shared_ptr<T_PRED const>> ret;
    ASSERT(this->get_exprs_to_be_correlated()->expr_group_get_num_exprs() == 1);
    expr_ref expr = this->get_exprs_to_be_correlated()->get_expr_vec().begin()->second.get_expr();
    ASSERT(expr->is_bv_sort());
    unsigned bvlen = expr->get_sort()->get_size();
    context *ctx = this->m_ctx;
    if (m_lower_bound_signed_cur != m_min_lower_bound_signed) {
      bv_const lb = m_lower_bound_signed_cur;
      expr_ref lb_expr = ctx->mk_bv_const(bvlen, lb);
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": lb signed = " << lb << endl);
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), ctx->mk_bvsge(expr, lb_expr), expr_true(ctx), PRED_COMMENT_INEQ_LOWER_BOUND_SIGNED);
      ret.insert(pred);
    }
    if (m_upper_bound_signed_cur != m_max_upper_bound_signed) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": ub signed = " << m_upper_bound_signed_cur << endl);
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), ctx->mk_bvsle(expr, ctx->mk_bv_const(bvlen, m_upper_bound_signed_cur)), expr_true(ctx), PRED_COMMENT_INEQ_UPPER_BOUND_SIGNED);
      ret.insert(pred);
    }

    if (m_lower_bound_unsigned_cur != m_min_lower_bound_unsigned) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": lb unsigned = " << m_lower_bound_unsigned_cur << endl);
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), ctx->mk_bvuge(expr, ctx->mk_bv_const(bvlen, m_lower_bound_unsigned_cur)), expr_true(ctx), PRED_COMMENT_INEQ_LOWER_BOUND_UNSIGNED);
      ret.insert(pred);
    }
    if (m_upper_bound_unsigned_cur != m_max_upper_bound_unsigned) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": ub unsigned = " << m_upper_bound_unsigned_cur << endl);
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), ctx->mk_bvule(expr, ctx->mk_bv_const(bvlen, m_upper_bound_unsigned_cur)), expr_true(ctx), PRED_COMMENT_INEQ_UPPER_BOUND_UNSIGNED);
      ret.insert(pred);
    }
    return ret;
  }
  virtual void refine_bounds(bv_const const& point_smallest_signed, bv_const const& point_largest_signed, bv_const const& point_smallest_unsigned, bv_const const& point_largest_unsigned) const
  {
    m_lower_bound_signed_cur = bv_avg_roundup(m_lower_bound_signed_known, point_smallest_signed);
    m_upper_bound_signed_cur = bv_avg_rounddown(m_upper_bound_signed_known, point_largest_signed);

    m_lower_bound_unsigned_cur = bv_avg_roundup(m_lower_bound_unsigned_known, point_smallest_unsigned);
    m_upper_bound_unsigned_cur = bv_avg_rounddown(m_upper_bound_unsigned_known, point_largest_unsigned);
  }


  bool propose_cur_bounds() const
  {
    set<bv_const> expr_pts = this->m_point_set.get_bv_const_points_for_single_expr_eg(this->get_exprs_to_be_correlated(), this->get_visited_ces());
    bv_const point_smallest_signed   = get_smallest_value_signed(expr_pts, m_bvlen, m_min_lower_bound_signed);
    bv_const point_largest_signed    = get_largest_value_signed(expr_pts, m_bvlen, m_max_upper_bound_signed);
    bv_const point_smallest_unsigned = get_smallest_value_unsigned(expr_pts, m_bvlen, m_min_lower_bound_unsigned);
    bv_const point_largest_unsigned  = get_largest_value_unsigned(expr_pts, m_bvlen, m_max_upper_bound_unsigned);

    if (bv_cmp(point_smallest_signed, m_lower_bound_signed_known) < 0) {
      m_lower_bound_signed_known = m_min_lower_bound_signed;
    }
    if (bv_cmp(point_largest_signed, m_upper_bound_signed_known) > 0) {
      m_upper_bound_signed_known = m_max_upper_bound_signed;
    }
    if (bv_cmp(point_smallest_unsigned, m_lower_bound_unsigned_known) < 0) {
      m_lower_bound_unsigned_known = m_min_lower_bound_unsigned;
    }
    if (bv_cmp(point_largest_unsigned, m_upper_bound_unsigned_known) > 0) {
      m_upper_bound_unsigned_known = m_max_upper_bound_unsigned;
    }
    refine_bounds(point_smallest_signed, point_largest_signed, point_smallest_unsigned, point_largest_unsigned);


    CPP_DBG_EXEC2(EQCLASS_INEQ,
        cout << __func__ << " " << __LINE__ << ": m_points.size() = " << this->m_point_set.size() << endl;
//        cout << __func__ << " " << __LINE__ << ": m_points:\n" << this->m_point_set.point_set_to_string("  ") << endl;
        cout << __func__ << " " << __LINE__ << ": point_smallest_signed = " << point_smallest_signed << endl;
        cout << __func__ << " " << __LINE__ << ": point_largest_signed = " << point_largest_signed << endl;
        cout << __func__ << " " << __LINE__ << ": point_smallest_unsigned = " << point_smallest_unsigned << endl;
        cout << __func__ << " " << __LINE__ << ": point_largest_unsigned = " << point_largest_unsigned << endl;
        cout << __func__ << " " << __LINE__ << ": m_lower_bound_signed_known = " << m_lower_bound_signed_known << endl;
        cout << __func__ << " " << __LINE__ << ": m_upper_bound_signed_known = " << m_upper_bound_signed_known << endl;
        cout << __func__ << " " << __LINE__ << ": m_lower_bound_signed_cur = " << m_lower_bound_signed_cur << endl;
        cout << __func__ << " " << __LINE__ << ": m_upper_bound_signed_cur = " << m_upper_bound_signed_cur << endl;
        cout << __func__ << " " << __LINE__ << ": m_lower_bound_unsigned_known = " << m_lower_bound_unsigned_known << endl;
        cout << __func__ << " " << __LINE__ << ": m_upper_bound_unsigned_known = " << m_upper_bound_unsigned_known << endl;
        cout << __func__ << " " << __LINE__ << ": m_lower_bound_unsigned_cur = " << m_lower_bound_unsigned_cur << endl;
        cout << __func__ << " " << __LINE__ << ": m_upper_bound_unsigned_cur = " << m_upper_bound_unsigned_cur << endl;
    );


    ASSERT(bv_cmp(m_lower_bound_signed_known, m_lower_bound_signed_cur) <= 0);
    //ASSERT(bv_cmp_signed(m_lower_bound_signed_cur, m_upper_bound_signed_cur, m_bvlen) <= 0);
    ASSERT(bv_cmp(m_upper_bound_signed_cur, m_upper_bound_signed_known) <= 0);
    ASSERT(bv_cmp(m_lower_bound_unsigned_known, m_lower_bound_unsigned_cur) <= 0);
    //ASSERT(bv_cmp(m_lower_bound_unsigned_cur, m_upper_bound_unsigned_cur) <= 0);
    ASSERT(bv_cmp(m_upper_bound_unsigned_cur, m_upper_bound_unsigned_known) <= 0);

    if (m_distinct_stable_values_for_lower_bound_signed_known.size() >= MAX_STABLE_VALUES_SEEN_BEFORE_WIDENING) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": widening lower_bound_signed to " << m_min_lower_bound_signed << endl);
      m_lower_bound_signed_cur = m_min_lower_bound_signed;
    }
    if (m_distinct_stable_values_for_upper_bound_signed_known.size() >= MAX_STABLE_VALUES_SEEN_BEFORE_WIDENING) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": widening upper_bound_signed to " << m_max_upper_bound_signed << endl);
      m_upper_bound_signed_cur = m_max_upper_bound_signed;
    }
    if (m_distinct_stable_values_for_lower_bound_unsigned_known.size() >= MAX_STABLE_VALUES_SEEN_BEFORE_WIDENING) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": widening lower_bound_unsigned to " << m_min_lower_bound_unsigned << endl);
      m_lower_bound_unsigned_cur = m_min_lower_bound_unsigned;
    }
    if (m_distinct_stable_values_for_upper_bound_unsigned_known.size() >= MAX_STABLE_VALUES_SEEN_BEFORE_WIDENING) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": widening upper_bound_unsigned to " << m_max_upper_bound_unsigned << endl);
      m_upper_bound_unsigned_cur = m_max_upper_bound_unsigned;
    }

    CPP_DBG_EXEC(EQCLASS_INEQ,
        cout << __func__ << " " << __LINE__ << ": m_points.size() = " << this->m_point_set.size() << endl;
//        cout << __func__ << " " << __LINE__ << ": m_points:\n" << this->m_point_set.point_set_to_string("  ") << endl;
        cout << __func__ << " " << __LINE__ << ": point_smallest_signed = " << point_smallest_signed << endl;
        cout << __func__ << " " << __LINE__ << ": point_largest_signed = " << point_largest_signed << endl;
        cout << __func__ << " " << __LINE__ << ": point_smallest_unsigned = " << point_smallest_unsigned << endl;
        cout << __func__ << " " << __LINE__ << ": point_largest_unsigned = " << point_largest_unsigned << endl;
        cout << __func__ << " " << __LINE__ << ": m_lower_bound_signed_known = " << m_lower_bound_signed_known << endl;
        cout << __func__ << " " << __LINE__ << ": m_upper_bound_signed_known = " << m_upper_bound_signed_known << endl;
        cout << __func__ << " " << __LINE__ << ": m_lower_bound_signed_cur = " << m_lower_bound_signed_cur << endl;
        cout << __func__ << " " << __LINE__ << ": m_upper_bound_signed_cur = " << m_upper_bound_signed_cur << endl;
        cout << __func__ << " " << __LINE__ << ": m_lower_bound_unsigned_known = " << m_lower_bound_unsigned_known << endl;
        cout << __func__ << " " << __LINE__ << ": m_upper_bound_unsigned_known = " << m_upper_bound_unsigned_known << endl;
        cout << __func__ << " " << __LINE__ << ": m_lower_bound_unsigned_cur = " << m_lower_bound_unsigned_cur << endl;
        cout << __func__ << " " << __LINE__ << ": m_upper_bound_unsigned_cur = " << m_upper_bound_unsigned_cur << endl;
    );

    return    m_lower_bound_signed_cur != m_lower_bound_signed_known
           || m_upper_bound_signed_cur != m_upper_bound_signed_known
           || m_lower_bound_unsigned_cur != m_lower_bound_unsigned_known
           || m_upper_bound_unsigned_cur != m_upper_bound_unsigned_known
    ;
  }


};

}
