#include <fstream>
#include "tfg/predicate_set.h"
#include "eq/corr_graph.h"
#include "expr/context_cache.h"
//#include "expr/expr_query_cache.h"
#include "expr/aliasing_constraints.h"
#include "expr/alignment_map.h"
#include "tfg/parse_input_eq_file.h"
#include "graph/query_decomposition.h"
#include "support/globals_cpp.h"
#include "support/c_utils.h"
#include "support/bv_const_expr.h"
#include "expr/eval.h"

namespace eqspace
{

void
predicate_set_intersect(predicate_set_t &dst, predicate_set_t const& src)
{
  predicate_set_t new_dst;
  for (auto const& r : dst) {
    bool found_in_src = false;
    for (auto const& s : src) {
      if (r == s) {
        found_in_src = true;
        break;
      }
      //XXX: can have a more nuanced check that considers edge-guards
    }
    if (found_in_src) {
      new_dst.insert(r);
    }
  }
  dst = new_dst;
}

//returns TRUE if something was changed during the meet operation
bool
predicate_set_meet(predicate_set_t &preds)
{
  bool something_changed = true;
  predicate_set_t new_preds;
  bool ret = false;
  while (something_changed) {
    something_changed = false;
    predicate_set_t new_ls;
    for (auto iter1 = preds.begin(); iter1 != preds.end(); iter1++) {
      bool found_meet_partner = false;
      for (auto iter2 = preds.begin(); iter2 != preds.end(); iter2++) {
        if (iter1 == iter2) {
          continue;
        }
        edge_guard_t eg1 = (*iter1)->get_edge_guard();
        edge_guard_t eg2 = (*iter2)->get_edge_guard();
        auto ec1 = eg1.get_edge_composition();
        auto ec2 = eg2.get_edge_composition();
   
        if (   ec1
            && ec2
            && ec1 != ec2
            && ec1->graph_edge_composition_get_to_pc() == ec2->graph_edge_composition_get_to_pc()
            && (   eg1.edge_guard_implies(eg2)
                || eg2.edge_guard_implies(eg1))) {
          found_meet_partner = true; //ignore both in this case (because the preds are different but paths are contained in one another indicating that the generation is happening in a loop; ignoring ensures finiteness and termination but also over-approximates
          break;
        }
      }
      if (!found_meet_partner) {
        new_preds.insert(*iter1);
      } else {
        something_changed = true;
        ret = true;
      }
    }
    preds = new_preds;
  }
  return ret;
}

string
predicate_set_to_string(predicate_set_t const& pred_set)
{
  stringstream ss;
  size_t i = 0;
  for (auto const& pred : pred_set) {
    ss << i++ << "." << pred->to_string(true) << endl;
  }
  return ss.str();
}

predicate_ref
predicate_set_or(predicate_set_t const& pset, tfg const& t)
{
  context *ctx = t.get_context();
  expr_ref e = expr_false(ctx);
  for (auto const& pred : pset) {
    expr_ref pred_expr = pred->predicate_get_expr(t, true);
    e = expr_or(e, pred_expr);
  }
  return predicate::mk_predicate_ref(precond_t(), e, expr_true(ctx), "or-combination");
}

bool
predicate_set_contains(predicate_set_t const &haystack, predicate_set_t const &needle)
{
  for (predicate_set_t::const_iterator n = needle.begin();
       n != needle.end();
       n++) {
    predicate_set_t::const_iterator found = haystack.find(*n);
    if (found == haystack.end()) {
      return false;
    }
  }
  return true;
}

void
predicate_set_difference(predicate_set_t const& a, predicate_set_t const& b, predicate_set_t& res)
{
  unordered_set_difference(a, b, res);
}

}
