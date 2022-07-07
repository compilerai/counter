#pragma once

#include "graph/smallest_point_cover.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_houdini_axiom_based_t : public smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>
{
public:
  smallest_point_cover_houdini_axiom_based_t(point_set_t const &point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, expr_group_ref const& global_eg_at_p, context *ctx) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(point_set, out_of_bound_exprs, global_eg_at_p, ctx)
  { }

  smallest_point_cover_houdini_axiom_based_t(istream& is, string const& inprefix, context* ctx, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(is, inprefix, ctx, point_set, out_of_bound_exprs), m_first(false)
  { }

  smallest_point_cover_houdini_axiom_based_t(smallest_point_cover_houdini_axiom_based_t const& other, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>(other, point_set, out_of_bound_exprs), m_first(other.m_first)
  { }

  virtual dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> clone(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const override
  {
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_dshared<smallest_point_cover_houdini_axiom_based_t<T_PC, T_N, T_E, T_PRED>>(*this, point_set, out_of_bound_exprs);
  }

  virtual void smallest_point_cover_to_stream(ostream& os, string const& inprefix) const override
  {
    ASSERT(!this->m_first);
    string prefix = inprefix + "type houdini-axiom-based";
    os << "=" + prefix + "\n";
    this->smallest_point_cover_to_stream_helper(os, prefix + " ");
  }

private:

  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>>& preds) const override
  {
    if (m_first) {
      //size_t num_exprs = this->get_exprs_to_be_correlated()->size();
      for (auto const& [i,pe] : this->get_exprs_to_be_correlated()->get_expr_vec()) {
        predicate_ref pred = predicate::mk_predicate_ref(precond_t(), pe.get_expr(), expr_true(this->m_ctx), "houdini-axiom-based-guess");
        preds.insert(pred);
      }
      m_first = false;
    }
    return true; //axiom-based preds do not depend on CEs
  }

private:
  mutable bool m_first = true;
};

}
