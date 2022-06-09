#pragma once

#include "expr/expr.h"
#include "support/bv_const.h"

namespace eqspace {

class range_t
{
private:
  bv_const m_begin;
  bv_const m_end; // [begin, end)
  unsigned m_bvsz;

  range_t(bv_const const& begin, bv_const const& end, unsigned bvsz)
    : m_begin(begin),
      m_end(end),
      m_bvsz(bvsz)
  { ASSERT(begin <= end); ASSERT(bvsz); }

public:
  static range_t empty_range(range_t const& ref)
  {
    return range_t(ref.m_begin, ref.m_begin, ref.m_bvsz);
  }
  static range_t empty_range(size_t bvsize)
  {
    return range_t(0,0,bvsize);
  }
  bool range_is_empty() const { return m_begin == m_end; }

  static range_t full_range(size_t bvsize)
  {
    return range_t(0, bv_exp2(bvsize), bvsize);
  }
  
  static range_t range_from_mybitset(mybitset const& begin, mybitset const& end)
  {
    return range_t(begin.get_unsigned_mpz(), bv_inc(end.get_unsigned_mpz()), begin.get_numbits());
  }

  static range_t range_from_expr_pair(pair<expr_ref,expr_ref> const& re)
  {
    return range_t::range_from_mybitset(re.first->get_mybitset_value(), re.second->get_mybitset_value());
  }

  bool operator==(range_t const& other) const
  {
    return    this->m_begin == other.m_begin
           && this->m_end == other.m_end
           && this->m_bvsz == other.m_bvsz;
  }

  // restore the original mybitset
  mybitset front() const { return mybitset(m_begin,m_bvsz); }
  mybitset back() const  { return mybitset(bv_dec(m_end),m_bvsz); }

  bv_const begin() const  { return m_begin; }
  bv_const end() const    { return m_end; }
  unsigned bvsize() const { return m_bvsz; }

  bv_const size() const     { return (m_end - m_begin); }

  bool is_unit_size() const { return (m_end - m_begin) == 1; }

  bool lies_within(bv_const const& a) const { return a >= m_begin && a < m_end; }
  bool lies_within(mybitset const& a) const { return lies_within(a.get_unsigned_mpz()); }
  bool is_subrange_of(range_t const& r) const  { return r.m_begin <= this->m_begin && this->m_end <= r.m_end; }
  bool has_intersection(range_t const& other) const
  {
    return     !range_is_empty() && !other.range_is_empty()
           && (   other.lies_within(m_begin)
               || lies_within(other.m_begin));
  }

  bool is_in_immediate_neighbourhood_of(range_t const& other) const
  {
    return     !this->range_is_empty() && !other.range_is_empty()
           && (   this->m_begin == other.m_end
               || other.m_begin == this->m_end);
  }

  range_t range_intersect(range_t const& other) const
  {
    if (!has_intersection(other))
      return range_t::empty_range(other); // empty range
    else
      return range_t(max(m_begin,other.m_begin), min(m_end,other.m_end), m_bvsz);
  }

  vector<range_t> range_difference(range_t const& other) const
  {
    return this->range_intersect_and_difference(other).second;
  }

  vector<range_t> range_difference(vector<range_t> ranges) const;
  pair<range_t,vector<range_t>> range_intersect_and_difference(range_t const& other) const;

  bool comes_before(range_t const& other) const { return this->m_end <= other.m_begin; }
  bool ends_before(range_t const& other) const  { return this->m_end < other.m_end; }
  bool operator<(range_t const&rhs) const       { return m_begin < rhs.m_begin; }
  bool operator<=(range_t const&rhs) const      { return m_begin <= rhs.m_begin; }

  string to_string() const;

  pair<expr_ref,expr_ref> expr_pair_from_range(context* ctx) const
  {
    return make_pair(ctx->mk_bv_const(m_bvsz, this->front()),
                     ctx->mk_bv_const(m_bvsz, this->back()));
  }

  static bool range_is_well_formed(pair<expr_ref,expr_ref> const& re);
  static void subtract_points_from_sorted_ranges(list<range_t>& ret, vector<mybitset> point_mapping);
  static bool ranges_give_full_coverage(vector<range_t>& ranges);
  static void union_with_sorted_range(list<range_t>& ranges, range_t const& ir);

  //static range_t find_first_fit_range_of_size(uint32_t size, list<range_t>& avail_ranges);
};
}
