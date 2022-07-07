#pragma once

#include "graph/smallest_point_cover.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_houdini_expects_stability_t : public smallest_point_cover_houdini_t<T_PC, T_N, T_E, T_PRED>
{
public:
  smallest_point_cover_houdini_expects_stability_t(point_set_t const &point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, expr_group_ref const& global_eg_at_p, context *ctx) : smallest_point_cover_houdini_t<T_PC, T_N, T_E, T_PRED>(point_set, out_of_bound_exprs, global_eg_at_p, ctx)
  { }

  smallest_point_cover_houdini_expects_stability_t(istream& is, string const& inprefix, context* ctx, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_houdini_t<T_PC, T_N, T_E, T_PRED>(is, inprefix, ctx, point_set, out_of_bound_exprs)
  { }

  smallest_point_cover_houdini_expects_stability_t(smallest_point_cover_houdini_expects_stability_t const& other, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_houdini_t<T_PC, T_N, T_E, T_PRED>(other, point_set, out_of_bound_exprs)
  { }

  virtual dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> clone(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const override
  {
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_dshared<smallest_point_cover_houdini_expects_stability_t<T_PC, T_N, T_E, T_PRED>>(*this, point_set, out_of_bound_exprs);
  }

  virtual void smallest_point_cover_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type houdini_expects_stability";
    os << "=" + prefix + "\n";
    this->smallest_point_cover_to_stream_helper(os, prefix + " ");
  }

private:
  virtual failcond_t eqclass_check_stability() const override
  {
    bool ret = !this->smallest_point_cover_get_preds().empty();
    if (ret) {
      return failcond_t::failcond_null();
    }
    return failcond_t::failcond_create(this->get_weakest_possible_pred(), src_dst_cg_path_tuple_t(), this->get_exprs_to_be_correlated()->get_name() + ".stability-failure");
  }

  expr_ref get_weakest_possible_pred() const
  {
    expr_ref ret;
    map<string_ref, point_ref> const& points = this->m_point_set.get_points_vec();
    vector<point_ref> defined_points;
    for (auto const& [ptname, pt] : points) {
      ASSERT(ptname == pt->get_point_name());
      if (!this->get_visited_ces().count(pt->get_point_name())) 
        continue;
      // All exprs should be defined in a visited point
      ASSERT(this->point_has_mapping_for_all_exprs(pt));
      defined_points.push_back(pt);
    }
    //size_t num_exprs = this->get_exprs_to_be_correlated()->size();
    for (auto const& [i,pe] : this->get_exprs_to_be_correlated()->get_expr_vec()) {
      if (this->houdini_expr_idx_is_falsified(defined_points, i)) {
        if (ret) {
          ret = expr_and(ret, pe.get_expr());
        } else {
          ret = pe.get_expr();
        }
      }
    }
    return ret;
  }
};

}
