#pragma once
#include "gsupport/predicate.h"

namespace eqspace {

using predicate_set_t = unordered_set<predicate_ref>;

void predicate_set_union(predicate_set_t &dst, predicate_set_t const& src);
//void predicate_set_intersect(predicate_set_t &dst, predicate_set_t const& src);
void guarded_predicate_set_intersect(list<guarded_pred_t> &dst, list<guarded_pred_t> const& src);
void guarded_predicate_set_union(list<guarded_pred_t> &dst, list<guarded_pred_t> const& src);
//bool predicate_set_meet(predicate_set_t &preds);
bool guarded_predicate_set_meet(list<pair<graph_edge_composition_ref<pc,tfg_edge>, predicate_ref>> &preds);
//string predicate_set_to_string(predicate_set_t const& pred_set);
string guarded_predicate_set_to_string(list<guarded_pred_t> const& pred_set);
//predicate_ref predicate_set_or(predicate_set_t const& pset, tfg const& t);
predicate_ref guarded_predicate_set_or(list<guarded_pred_t> const& pset, tfg const& t);
bool predicate_set_contains(predicate_set_t const &haystack, predicate_set_t const &needle);
predicate_set_t predicate_set_difference(predicate_set_t const& a, predicate_set_t const& b);
predicate_set_t predicate_set_union(predicate_set_t const& a, predicate_set_t const& b);
void predicate_set_difference(predicate_set_t& a, predicate_set_t const& b);
bool predicate_set_contains_false_predicate(predicate_set_t const& preds);
void guarded_pred_to_stream(ostream& os, guarded_pred_t const& l, string const& prefix);
//guarded_pred_t guarded_pred_from_stream(istream& is, context* ctx);

list<guarded_pred_t> guarded_pred_set_difference(list<guarded_pred_t> const& a, list<guarded_pred_t> const& b);
}
