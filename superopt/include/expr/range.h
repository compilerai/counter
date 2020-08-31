#pragma once

#include "expr/expr.h"

namespace eqspace {

class range_t
{
private:
  mybitset m_begin;
  mybitset m_end; // [begin, end)

  range_t(mybitset const& begin, mybitset const& end)
    : m_begin(begin),
      m_end(end)
  { ASSERT(begin.bvule(end)); }

public:
  static range_t empty_range(size_t bvsize)
  {
    return range_t(mybitset(0,bvsize+1),mybitset(0,bvsize+1));
  }

  static range_t range_from_mybitset(mybitset const& begin, mybitset const& end)
  {
    // zero extend by 1 to prevent overflow
    return range_t(begin.bvzero_ext(1), end.bvzero_ext(1).inc());
  }

  static range_t range_from_expr_pair(pair<expr_ref,expr_ref> const& re)
  {
    return range_t::range_from_mybitset(re.first->get_mybitset_value(), re.second->get_mybitset_value());
  }


  // restore the original mybitset
  mybitset front() const { return m_begin.bvextract(m_begin.get_numbits()-2,0); }
  mybitset back() const { return (m_end-1).bvextract(m_end.get_numbits()-2,0); }

  unsigned long size() const { return (m_end - m_begin).toUnsignedLong(); }
  bool range_is_empty() const { return m_begin == m_end; }

  bool lies_within(mybitset const& a) const { return a.bvuge(m_begin) && a.bvult(m_end); }
  bool has_intersection(range_t const& other) const;
  range_t range_intersect(range_t const& other) const;

  list<range_t> range_difference(range_t const& other) const;
  list<range_t> range_difference(vector<range_t> ranges) const;
  bool operator<(range_t const&rhs) const { return m_begin.bvult(rhs.m_begin); }

  string to_string() const;

  static bool range_is_well_formed(pair<expr_ref,expr_ref> const& re);
  static void subtract_points_from_sorted_ranges(list<range_t>& ret, vector<mybitset> point_mapping);
  static range_t find_first_fit_range_of_size(uint32_t size, list<range_t>& avail_ranges);
};

bool expr_constant_value_in_range(expr_ref a, range_t const &range);
bool expr_constant_value_in_range(expr_ref a, pair<expr_ref,expr_ref> const &range);
}
