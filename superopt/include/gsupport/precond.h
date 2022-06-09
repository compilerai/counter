#pragma once

#include <vector>
#include <list>

#include "expr/expr.h"
#include "support/consts.h"
#include "support/types.h"
#include "support/debug.h"
#include "support/utils.h"
#include "expr/consts_struct.h"
//#include "gsupport/edge_guard.h"
#include "graph/indicator_varset.h"
//#include "expr/local_sprel_expr_guesses.h"

namespace eqspace {

class precond_t {
private:
  //edge_guard_t m_guard;
  indicator_varset_t m_indicator_vars;
  //local_sprel_expr_guesses_t m_local_sprel_expr_assumes;

  //bool edge_guard_evaluates_to_true(tfg const &t) const;
public:
  precond_t(/*edge_guard_t const &guard, *//*local_sprel_expr_guesses_t const &lsprel_assumes*/) /*:*/ /*m_guard(guard), *//*m_local_sprel_expr_assumes(lsprel_assumes)*/ {}
  precond_t(indicator_varset_t const& indicator_vars, context *ctx) : m_indicator_vars(indicator_vars) {}
  precond_t(/*edge_guard_t const &guard, */context *ctx) /*: m_guard(guard)*/ {}
  //precond_t() {}

  bool precond_implies(precond_t const &other) const;
  bool preconds_are_incompatible(precond_t const &other) const;
  bool operator==(precond_t const &other) const;
  bool operator<(precond_t const &other) const;
  //bool equals_mod_lsprels(precond_t const &other) const;
  bool equals(precond_t const &other) const;
  expr_ref precond_get_expr(context* ctx/*tfg const &t*//*, bool simplified*/) const;
  expr_ref precond_get_expr_for_lsprels(context *ctx) const;
  //void subtract_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &guesses);
  //void add_local_sprel_expr_assumes_as_guards(local_sprel_expr_guesses_t const &local_sprel_expr_assumes);
  string precond_to_string() const;
  string precond_to_string_for_eq(bool use_orig_ids = false) const;
  //local_sprel_expr_guesses_t const &get_local_sprel_expr_assumes() const;
  //void set_local_sprel_expr_assumes(local_sprel_expr_guesses_t const &lsprel_guesses);
  void precond_clear();
  bool precond_is_trivial() const;
  //edge_guard_t const &precond_get_guard() const;
  indicator_varset_t const &precond_get_indicator_vars() const { return m_indicator_vars; }
  //void precond_set_guard(edge_guard_t const &eg);
  //map<allocsite_t, expr_ref> get_local_sprel_expr_map() const;
  void visit_exprs(std::function<expr_ref (const string &, expr_ref const &)> f);
  void visit_exprs(std::function<void (const string &, expr_ref const &)> f) const;
  //map<expr_id_t, pair<expr_ref, expr_ref>> get_local_sprel_expr_submap() const;

  bool precond_eval_on_counter_example(set<memlabel_ref> const& relevant_memlabels, bool &eval_result, counter_example_t &counter_example) const;
  //bool precond_try_unioning_edge_guards(precond_t const& other);

  static bool precond_compare_ids(precond_t const &a, precond_t const &b)
  {
    //if (!(a.m_guard == b.m_guard)) {
    //  return edge_guard_t::edge_guard_compare_ids(a.m_guard, b.m_guard);
    //}
    if (!a.m_indicator_vars.equals(b.m_indicator_vars)) {
      return indicator_varset_t::indicator_varset_compare_ids(a.m_indicator_vars, b.m_indicator_vars);
    }
    return false;
    //return local_sprel_expr_guesses_t::local_sprel_expr_guesses_compare_ids(a.m_local_sprel_expr_assumes, b.m_local_sprel_expr_assumes);
  }
  static string read_precond(istream& in/*, state const& start_state*/, context* ctx, precond_t& precond);
};

}

namespace std{
using namespace eqspace;
template<>
struct hash<precond_t>
{
  std::size_t operator()(precond_t const &p) const
  {
    size_t seed = 0;
    //myhash_combine<size_t>(seed, hash<edge_guard_t>()(p.precond_get_guard()));
    myhash_combine<size_t>(seed, hash<indicator_varset_t>()(p.precond_get_indicator_vars()));
    //do not combine hash of mutable m_local_sprel_expr_assumes, as it may be changed in an unordered_set
    return seed;
  }
};

}


