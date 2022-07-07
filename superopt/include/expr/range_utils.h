#pragma once

#include "expr/range.h"

namespace eqspace {

using range_vector = vector<range_t>;

inline bool expr_constant_value_in_range(expr_ref const& a, range_t const &range)
{
  return range.lies_within(a->get_mybitset_value());
}
inline bool expr_constant_value_in_range(expr_ref const& a, expr_pair const &range)
{
  mybitset const& av = a->get_mybitset_value();
  return    av.bvuge(range.first->get_mybitset_value())
         && av.bvule(range.second->get_mybitset_value());
}

bool ranges_are_refined_than(range_vector const& refined, range_vector const& coarse);

range_vector ranges_union(vector<range_vector> const& ranges);
range_vector range_intersection(range_vector const& ranges, range_t const& target);
range_vector ranges_intersection(vector<range_vector> const& ranges);
range_vector ranges_difference(range_vector const& from, range_vector const& ranges);

expr_ref encode_ranges_as_contraints_over_var(vector<expr_pair> const& ranges, expr_ref const& var);

string range_vector_to_string(range_vector const& rv, string sep = ", ");

}
