#pragma once

#include "support/debug.h"
#include "support/lru.h"

#include "expr/expr.h"
#include "expr/expr_eval_status.h"
#include "expr/solver_interface.h"
#include "expr/proof_status.h"

//namespace std {
//template<>
//struct hash<tuple<unordered_set<predicate>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, avail_exprs_t>>
//{
//  std::size_t operator()(tuple<unordered_set<predicate>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, avail_exprs_t> const &t) const
//  {
//    size_t seed = 0;
//    for (auto const& p : get<0>(t)) myhash_combine<size_t>(seed, hash<predicate>()(p));
//    myhash_combine<size_t>(seed, hash<precond_t>()(get<1>(t)));
//    myhash_combine<size_t>(seed, hash<sprel_map_pair_t>()(get<2>(t)));
//    myhash_combine<size_t>(seed, hash<tfg_suffixpath_t>()(get<3>(t)));
//    myhash_combine<size_t>(seed, hash<avail_exprs_t>()(get<4>(t)));
//    return seed;
//  }
//};
//}

//namespace std {
//template<>
//struct hash<tuple<unordered_set<predicate>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, pred_avail_exprs_t>>
//{
//  std::size_t operator()(tuple<unordered_set<predicate>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, pred_avail_exprs_t> const &t) const
//  {
//    size_t seed = 0;
//    for (auto const& p : get<0>(t)) myhash_combine<size_t>(seed, hash<predicate>()(p));
//    myhash_combine<size_t>(seed, hash<precond_t>()(get<1>(t)));
//    myhash_combine<size_t>(seed, hash<sprel_map_pair_t>()(get<2>(t)));
//    myhash_combine<size_t>(seed, hash<tfg_suffixpath_t>()(get<3>(t)));
//    myhash_combine<size_t>(seed, hash<pred_avail_exprs_t>()(get<4>(t)));
//    return seed;
//  }
//};
//}

namespace eqspace {
using namespace std;
class expr;
class expr_sort;
struct consts_struct_t;
typedef shared_ptr<expr> expr_ref;
typedef shared_ptr<expr_sort> sort_ref;
class context;

template<typename T_KEY, typename T_VAL>
class lookup_cache_t
{
  class cache_stats_t
  {
  public:
    void inc_hits() { m_hits++; }
    void inc_lookups() { m_total++; }
    void reset() { m_hits = m_total = 0; }
    string stats(string const& pfx) const
    {
      if (m_total == 0)
        return "";

      stringstream ss;
      ss << pfx << ":\n";
      ss << "\t----------- total:\t" << m_total << '\n'
        << "\t        hit ratio:\t" << (float) (m_hits) / m_total << '\n';
      return ss.str();
    }

  private:
    unsigned long m_hits = 0;
    unsigned long m_total = 0;
  };

private:
  mutable cache_stats_t m_stats;
  lru_t<T_KEY,T_VAL> m_cache;

public:
  lookup_cache_t(size_t max_size = LOOKUP_CACHE_DEFAULT_SIZE)
    : m_cache(max_size)
  { }

  bool lookup(T_KEY const& key) const
  {
    m_stats.inc_lookups();
    bool ret = m_cache.lookup(key);
    if (ret)
      m_stats.inc_hits();
    return ret;
  }
  T_VAL const& get(T_KEY const& key) const { return m_cache.get(key); }
  void add(T_KEY const& key, T_VAL const& val) { m_cache.put(key, val); }
  string stats(string const& pfx) const { return m_stats.stats(pfx); }
  void clear() { m_cache.clear(); m_stats.reset(); }
};

class context_cache_t
{
public:
  //map<expr_id_t, pair<expr_ref, list<pair<expr_ref, unordered_set<predicate>>>>> m_simplify_using_proof_queries;
  //expr_query_cache_t<expr_ref> m_simplify_syntactic;
  //expr_query_cache_t<expr_ref> m_simplify_using_lhs_set;
  //expr_query_cache_t<expr_ref> m_simplify_using_lhs_set_without_const_specific_simplifications;
  //expr_query_cache_t<expr_ref> m_simplify_visitor;
  //expr_query_cache_t<expr_ref> m_simplify_visitor_without_const_specific_simplifications;

  //expr_query_cache_t<memlabel_t> m_memlabel_intersect_using_lhs_set_and_addr;
  //expr_query_cache_t<map<expr_id_t,expr_ref>> m_gen_equivalence_class_using_lhs_set_and_precond;
  //unordered_map<tuple<unordered_set<predicate>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, pred_avail_exprs_t>,unordered_set<predicate>> m_lhs_simplify_until_fixpoint;
  //expr_query_cache_t<pair<bool, counter_example_t>> m_is_expr_equal_using_lhs_set;
  //map<unsigned, pair<expr_ref, expr_ref>> m_zero_out_comments;
  //map<pair<expr_id_t, pair<expr_id_t, string> >, pair<expr_ref, pair<bool, pair<expr_ref, expr_ref>>>> m_replace_needle_with_keyword;
  //map<expr_id_t, pair<bool, expr_ref>> m_contains_only_consts_struct_constants_or_arguments;
  //expr_query_cache_t<bool> m_is_expr_equal_syntactic_using_lhs_set;
  lookup_cache_t<expr_id_t , pair<expr_ref, expr_ref>> m_simplify_solver;
  lookup_cache_t<pair<expr_id_t, bool>, pair<expr_ref, expr_ref>> m_simplify;
  lookup_cache_t<expr_id_t, pair<expr_ref, expr_ref>> m_z3_solver_simplify;
  lookup_cache_t<expr_id_t, pair<expr_ref, solver_res>> m_z3_solver_prove;
  lookup_cache_t<expr_id_t, tuple<expr_ref,expr_ref,map<expr_id_t,expr_pair>>> m_z3_solver_substitution;
  lookup_cache_t<pair<unsigned, unsigned>, pair<pair<expr_ref, expr_ref>, expr_ref>> m_prune_obviously_false_branches_using_assume_clause;
  lookup_cache_t<expr_id_t, tuple<expr_ref, map<expr_id_t,pair<expr_ref, expr_ref>>, expr_ref>> m_replace_donotsimplify_using_solver_expressions_by_free_vars;
  lookup_cache_t<pair<expr_id_t, expr_id_t>, pair<pair<expr_ref, expr_ref>, bool>> m_is_expr_not_equal_syntactic;

  lookup_cache_t<expr_id_t, pair<expr_ref, size_t>> m_expr_size;
  lookup_cache_t<expr_ref, bool> m_contains_only_constants_or_sp_versions;
  lookup_cache_t<expr_id_t, pair<expr_ref,pair<expr_eval_status_t,expr_ref>>> m_expr_evaluates_to_constant_visitor;

  void clear() {
    //m_simplify_using_proof_queries.clear();
    //m_simplify_syntactic.clear();
    //m_simplify_using_lhs_set.clear();
    //m_simplify_using_lhs_set_without_const_specific_simplifications.clear();
    //m_simplify_visitor.clear();
    //m_simplify_visitor_without_const_specific_simplifications.clear();
    //m_memlabel_intersect_using_lhs_set_and_addr.clear();
    //m_gen_equivalence_class_using_lhs_set_and_precond.clear();
    //m_lhs_simplify_until_fixpoint.clear();
    //m_is_expr_equal_using_lhs_set.clear();
    //m_zero_out_comments.clear();
    //m_replace_needle_with_keyword.clear();
    //m_contains_only_consts_struct_constants_or_arguments.clear();
    //m_is_expr_equal_syntactic_using_lhs_set.clear();
    m_simplify_solver.clear();
    m_simplify.clear();
    m_z3_solver_simplify.clear();
    m_z3_solver_prove.clear();
    m_z3_solver_substitution.clear();
    m_prune_obviously_false_branches_using_assume_clause.clear();
    m_replace_donotsimplify_using_solver_expressions_by_free_vars.clear();
    m_is_expr_not_equal_syntactic.clear();

    m_expr_size.clear();
    m_contains_only_constants_or_sp_versions.clear();
    m_expr_evaluates_to_constant_visitor.clear();
  }

  string stats() const;
};

}
