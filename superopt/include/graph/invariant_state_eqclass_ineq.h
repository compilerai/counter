#pragma once
#include <map>
#include <string>

#include "graph/invariant_state_eqclass.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_ineq_t : public invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>
{
private:
  //all inclusive
  bv_const m_lower_bound_signed_cur, m_upper_bound_signed_cur;
  bv_const m_lower_bound_unsigned_cur, m_upper_bound_unsigned_cur;

  bv_const m_lower_bound_signed_known, m_upper_bound_signed_known;
  bv_const m_lower_bound_unsigned_known, m_upper_bound_unsigned_known;

  set<bv_const> m_distinct_stable_values_for_lower_bound_signed_known;
  set<bv_const> m_distinct_stable_values_for_upper_bound_signed_known;
  set<bv_const> m_distinct_stable_values_for_lower_bound_unsigned_known;
  set<bv_const> m_distinct_stable_values_for_upper_bound_unsigned_known;

  //consts
  size_t m_bvlen;
  bv_const m_min_lower_bound_signed, m_max_upper_bound_signed, m_min_lower_bound_unsigned, m_max_upper_bound_unsigned;
  static const int MAX_STABLE_VALUES_SEEN_BEFORE_WIDENING = 4;
public:
  invariant_state_eqclass_ineq_t(expr_group_ref const &exprs_to_be_correlated, context *ctx) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(exprs_to_be_correlated, ctx)
  {
    this->compute_params();
  }

  invariant_state_eqclass_ineq_t(istream& is, string const& inprefix/*, state const& start_state*/, context* ctx) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(is, inprefix/*, start_state*/, ctx)
  {
    this->compute_params();
  }

  virtual invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> *clone() const override
  {
    return new invariant_state_eqclass_ineq_t<T_PC, T_N, T_E, T_PRED>(*this);
  }

  virtual void invariant_state_eqclass_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type ineq";
    os << "=" + prefix + "\n";
    this->invariant_state_eqclass_to_stream_helper(os, prefix + " ");
  }

  /*static string invariant_state_eqclass_ineq_from_stream(istream& is, context* ctx, list<nodece_ref<T_PC, T_N, T_E>> const &counterexamples, invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>*& eqclass)
  {
    return invariant_state_eqclass_from_stream_helper(is, ctx, counterexamples, eqclass);
  }*/

private:
  //returns TRUE if we are done
  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>> &preds) override
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
      NOT_IMPLEMENTED();
    }
    this->propose_cur_bounds_and_gen_preds(preds);
    return true;
  }

  void compute_params()
  {
    expr_group_ref const& exprs_to_be_correlated = this->get_exprs_to_be_correlated();
    m_bvlen = exprs_to_be_correlated->get_max_bvlen();
    m_min_lower_bound_signed = bv_neg(bv_exp2(m_bvlen - 1));
    m_max_upper_bound_signed = bv_exp2(m_bvlen - 1) - 1;
    m_min_lower_bound_unsigned = 0;
    m_max_upper_bound_unsigned = bv_exp2(m_bvlen) - 1;

    ASSERT(exprs_to_be_correlated->size() == 1);
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
  propose_cur_bounds_and_gen_preds(unordered_set<shared_ptr<T_PRED const>> &preds)
  {
    bool ret = this->propose_cur_bounds();
    preds = gen_preds_from_cur_bounds();
    return ret;
  }

  unordered_set<shared_ptr<T_PRED const>>
  gen_preds_from_cur_bounds() const
  {
    unordered_set<shared_ptr<T_PRED const>> ret;
    ASSERT(this->get_exprs_to_be_correlated()->size() == 1);
    expr_ref expr = this->get_exprs_to_be_correlated()->at(0);
    ASSERT(expr->is_bv_sort());
    unsigned bvlen = expr->get_sort()->get_size();
    context *ctx = this->m_ctx;
    if (m_lower_bound_signed_cur != m_min_lower_bound_signed) {
      bv_const lb = m_lower_bound_signed_cur;
      expr_ref lb_expr = ctx->mk_bv_const(bvlen, lb);
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": lb signed = " << lb << endl);
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), ctx->mk_bvsge(expr, lb_expr), expr_true(ctx), "lb-signed");
      ret.insert(pred);
    }
    if (m_upper_bound_signed_cur != m_max_upper_bound_signed) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": ub signed = " << m_upper_bound_signed_cur << endl);
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), ctx->mk_bvsle(expr, ctx->mk_bv_const(bvlen, m_upper_bound_signed_cur)), expr_true(ctx), "ub-signed");
      ret.insert(pred);
    }

    if (m_lower_bound_unsigned_cur != m_min_lower_bound_unsigned) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": lb unsigned = " << m_lower_bound_unsigned_cur << endl);
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), ctx->mk_bvuge(expr, ctx->mk_bv_const(bvlen, m_lower_bound_unsigned_cur)), expr_true(ctx), "lb-unsigned");
      ret.insert(pred);
    }
    if (m_upper_bound_unsigned_cur != m_max_upper_bound_unsigned) {
      CPP_DBG_EXEC(EQCLASS_INEQ, cout << __func__ << " " << __LINE__ << ": ub unsigned = " << m_upper_bound_unsigned_cur << endl);
      predicate_ref pred = predicate::mk_predicate_ref(precond_t(), ctx->mk_bvule(expr, ctx->mk_bv_const(bvlen, m_upper_bound_unsigned_cur)), expr_true(ctx), "ub-unsigned");
      ret.insert(pred);
    }
    return ret;
  }

  bool propose_cur_bounds()
  {
    bv_const point_smallest_signed = this->m_point_set.get_smallest_value_signed();
    bv_const point_largest_signed = this->m_point_set.get_largest_value_signed();
    bv_const point_smallest_unsigned = this->m_point_set.get_smallest_value_unsigned();
    bv_const point_largest_unsigned = this->m_point_set.get_largest_value_unsigned();

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

    m_lower_bound_signed_cur = bv_avg_roundup(m_lower_bound_signed_known, point_smallest_signed);
    m_upper_bound_signed_cur = bv_avg_rounddown(m_upper_bound_signed_known, point_largest_signed);

    m_lower_bound_unsigned_cur = bv_avg_roundup(m_lower_bound_unsigned_known, point_smallest_unsigned);
    m_upper_bound_unsigned_cur = bv_avg_rounddown(m_upper_bound_unsigned_known, point_largest_unsigned);

    CPP_DBG_EXEC2(EQCLASS_INEQ,
        cout << __func__ << " " << __LINE__ << ": m_points.size() = " << this->m_point_set.size() << endl;
        cout << __func__ << " " << __LINE__ << ": m_points:\n" << this->m_point_set.point_set_to_string("  ") << endl;
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
        cout << __func__ << " " << __LINE__ << ": m_points:\n" << this->m_point_set.point_set_to_string("  ") << endl;
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
