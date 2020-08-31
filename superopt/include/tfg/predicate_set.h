#pragma once
#include "tfg/predicate.h"
#include <set>

namespace eqspace {

using predicate_set_t = unordered_set<predicate_ref>;

//void predicate_set_union(predicate_set_t &dst, predicate_set_t const& src);
void predicate_set_intersect(predicate_set_t &dst, predicate_set_t const& src);
bool predicate_set_meet(predicate_set_t &preds);
string predicate_set_to_string(predicate_set_t const& pred_set);
predicate_ref predicate_set_or(predicate_set_t const& pset, tfg const& t);
bool predicate_set_contains(predicate_set_t const &haystack, predicate_set_t const &needle);
void predicate_set_difference(predicate_set_t const& a, predicate_set_t const& b, predicate_set_t& res);

}
