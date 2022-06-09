#pragma once
#include <map>
#include <string>

#include "graph/smallest_point_cover.h"

namespace eqspace {

using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class smallest_point_cover_ineq_const_t : public smallest_point_cover_ineq_loose_t<T_PC, T_N, T_E, T_PRED>
{
private:
  set<expr_ref> m_const_bound_exprs;
  //all inclusive
  set<bv_const> m_const_bounds;

public:
  smallest_point_cover_ineq_const_t(point_set_t const &point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs, expr_group_ref const& global_eg_at_p, set<expr_ref> const& const_bound_exprs, context *ctx) : smallest_point_cover_ineq_loose_t<T_PC, T_N, T_E, T_PRED>(point_set, out_of_bound_exprs, global_eg_at_p, ctx), m_const_bound_exprs(const_bound_exprs)
  {
    this->compute_const_bounds(const_bound_exprs);
  }

  smallest_point_cover_ineq_const_t(istream& is, string const& inprefix/*, state const& start_state*/, context* ctx, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_ineq_loose_t<T_PC, T_N, T_E, T_PRED>(is, inprefix/*, start_state*/, ctx, point_set, out_of_bound_exprs)
  {
    m_const_bound_exprs = const_bound_exprs_from_stream(is, inprefix, ctx);
    this->compute_const_bounds(m_const_bound_exprs);
  }

  smallest_point_cover_ineq_const_t(smallest_point_cover_ineq_const_t const& other, point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) : smallest_point_cover_ineq_loose_t<T_PC, T_N, T_E, T_PRED>(other, point_set, out_of_bound_exprs), m_const_bound_exprs(other.m_const_bound_exprs),
   m_const_bounds(other.m_const_bounds)
  { }


  virtual dshared_ptr<smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>> clone(point_set_t const& point_set, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const override
  {
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return make_dshared<smallest_point_cover_ineq_const_t<T_PC, T_N, T_E, T_PRED>>(*this, point_set, out_of_bound_exprs);
  }

  virtual void smallest_point_cover_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type ineq_const";
    os << "=" + prefix + "\n";
    this->smallest_point_cover_ineq_loose_t<T_PC, T_N, T_E, T_PRED>::smallest_point_cover_ineq_loose_to_stream_helper(os, prefix);
    this->const_bound_exprs_to_stream(os, prefix + " ");
  }

  /*static string smallest_point_cover_ineq_loose_from_stream(istream& is, context* ctx, list<nodece_ref<T_PC, T_N, T_E>> const &counterexamples, smallest_point_cover_t<T_PC, T_N, T_E, T_PRED>*& eqclass)
  {
    return smallest_point_cover_from_stream_helper(is, ctx, counterexamples, eqclass);
  }*/

private:

  set<expr_ref> const_bound_exprs_from_stream(istream& is, string const& prefix, context* ctx) const
  {
    set<expr_ref> ret;
    string line;
    bool end;

    end = !getline(is, line);
    ASSERT(!end);
    string const done_prefix = string("=") + prefix + "const_bound_exprs done";

    while (line != done_prefix) {
      string const expr_prefix = string("=") + prefix + "const_bound_expr ";
      if (!string_has_prefix(line, expr_prefix)) {
        cout << _FNLN_ << ": line =\n" << line << "." << endl;
        cout << "done_prefix =\n" << (string("=") + prefix + "const_bound_exprs done") << "." << endl;
      }
      ASSERT(string_has_prefix(line, expr_prefix));
      expr_ref e;
      line = read_expr(is, e, ctx);

      ret.insert(e);
    }
    return ret;
  }

  void const_bound_exprs_to_stream(ostream& os, string const& prefix) const
  {
    int i = 0;
    for (auto const& e : m_const_bound_exprs) {
      context* ctx = e->get_context();
      os << "=" << prefix << "const_bound_expr " << i << endl;
      ctx->expr_to_stream(os, e);
      os << endl;
      i++;
    }
    os << "=" << prefix << "const_bound_exprs done\n";
  }

  void compute_const_bounds(set<expr_ref> const& const_bound_exprs) {
    m_const_bounds.clear();
    for (auto const& e : const_bound_exprs) {
      if (e->get_sort()->get_size() == this->m_bvlen) {
        bv_const val = expr2bv_const(e);
        m_const_bounds.insert(val);
        m_const_bounds.insert(bv_dec(val));
      }
    }
  }

  virtual void refine_bounds(bv_const const& point_smallest_signed, bv_const const& point_largest_signed, bv_const const& point_smallest_unsigned, bv_const const& point_largest_unsigned) const override
  {
    this->m_lower_bound_unsigned_cur = get_largest_value_unsigned(m_const_bounds, this->m_bvlen, point_smallest_unsigned); 
    this->m_lower_bound_signed_cur   = get_largest_value_signed(m_const_bounds, this->m_bvlen, point_smallest_signed); 
    this->m_upper_bound_unsigned_cur = get_smallest_value_unsigned(m_const_bounds, this->m_bvlen, point_largest_unsigned); 
    this->m_upper_bound_signed_cur   = get_smallest_value_signed(m_const_bounds, this->m_bvlen, point_largest_signed); 
  }

};

}
