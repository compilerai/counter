#pragma once
#include <map>
#include <string>

#include "graph/smallest_point_cover.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_houdini_t : public smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>
{
private:
  mutable set<expr_ref> m_masked_exprs;
public:
  smallest_point_cover_houdini_t(point_set_t const &point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, expr_group_ref global_eg_at_p, context *ctx) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(point_set, out_of_bound_exprs, global_eg_at_p, ctx)
  { }

  smallest_point_cover_houdini_t(istream& is, string const& inprefix/*, state const& start_state*/, context* ctx, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(is, inprefix/*, start_state*/, ctx, point_set, out_of_bound_exprs)
  { }

  smallest_point_cover_houdini_t(smallest_point_cover_houdini_t const& other, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(other, point_set, out_of_bound_exprs)
  { }

  virtual dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> clone(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const override
  {
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_dshared<smallest_point_cover_houdini_t<T_PC, T_N, T_E, T_PRED>>(*this, point_set, out_of_bound_exprs);
  }

  virtual void smallest_point_cover_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type houdini";
    os << "=" + prefix + "\n";
    this->smallest_point_cover_to_stream_helper(os, prefix + " ");
  }

protected:

  bool houdini_expr_idx_is_falsified(vector<point_ref> const& points, size_t idx) const
  {
    expr_ref e = this->get_exprs_to_be_correlated()->get_expr_vec().at(idx).get_expr();
    if (m_masked_exprs.count(e)) {
      return true;
    }
    if (this->m_out_of_bound_exprs/*this->m_point_set.get_out_of_bound_exprs()*/.count(idx)) {
      return true;
    }
    for (const auto &pt : points) {
      if(pt->at(idx).get_bv_const() == bv_zero()){
        return true;
      }
    }
    return false;
  }

private:

  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>>& preds) const override
  {
    autostop_timer func_timer(demangled_type_name(typeid(*this))+"."+__func__);
    if (last_proof_success) {
      return true;
    }

    auto const& exprs_to_be_correlated = this->get_exprs_to_be_correlated();

    if (this->m_timeout_info.timeout_occurred()) {
      unordered_set<shared_ptr<T_PRED const>> const& timeout_preds = this->m_timeout_info.get_preds_that_timed_out();
      for (auto const& timeout_pred : timeout_preds) {
        expr_ref timeout_expr = timeout_pred->get_lhs_expr();
        ASSERT(!timeout_expr->is_const_bool_true());
        DYN_DEBUG(eqclass_bv, cout << "masking out expr " << expr_string(timeout_expr) << " because its query timed out in previous runs" << endl);
        auto const& expr_vec = exprs_to_be_correlated->get_expr_vec();
        ASSERT(any_of(expr_vec.begin(), expr_vec.end(), [timeout_expr](pair<expr_group_t::expr_idx_t, point_exprs_t> const& e) -> bool { return e.second.get_expr() == timeout_expr; }));
        m_masked_exprs.insert(timeout_expr);
      }
    }

    //size_t num_exprs = exprs_to_be_correlated->size();
    string pred_comment = exprs_to_be_correlated->get_name()+string(PRED_COMMENT_HOUDINI_GUESS_SUFFIX);

    preds.clear();
    vector<point_ref> defined_points;
    map<string_ref, point_ref> const& points = this->m_point_set.get_points_vec();
    for (auto const& [ptname, pt] : points) {
      ASSERT(ptname == pt->get_point_name());
      if (!this->get_visited_ces().count(pt->get_point_name())) {
        continue;
      }
      // All exprs should be defined in a visited point
      ASSERT(this->point_has_mapping_for_all_exprs(pt));
      defined_points.push_back(pt);
    }
    for (auto const& [i,pe] : this->get_exprs_to_be_correlated()->get_expr_vec()) {
      if (!this->houdini_expr_idx_is_falsified(defined_points, i)) {
        predicate_ref pred = predicate::mk_predicate_ref(pe.get_expr(), pred_comment);
        preds.insert(pred);
      }
    }
    CPP_DBG_EXEC(EQCLASS_HOUDINI,
      cout << __func__ << " " << __LINE__ << ": returning preds =\n";
      size_t n = 0;
      for (auto const& pred : preds) {
        cout << "Pred " << n << ":\n" << pred->pred_to_string("  ");
        n++;
      }
    );
    return false;
  }
};

}
