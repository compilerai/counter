#pragma once
#include <string>
#include <map>
#include <deque>
#include "support/consts.h"
#include "expr/expr.h"
#include "tfg/tfg.h"
#include "support/types.h"
#include "gsupport/sprel_map.h"
#include "expr/sprel_map_pair.h"
//#include "graph/predicate_set.h"

using namespace std;
namespace eqspace {

constexpr size_t EXPR_QUERY_CACHE_SIZE = 4;

template<typename T>
class expr_query_cache_t {
private:
  unsigned long m_misses = 0;
  unsigned long m_total = 0;
  unsigned long m_both_found = 0;
  unsigned long m_either_found = 0;
  map<vector<expr_id_t>, pair<vector<expr_ref>, deque<tuple<T, set<predicate_ref>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, avail_exprs_t>>>>m_cache;
  /*bool (&m_lessT)(T const &a, T const &b);
  T minT(T const &a, T const &b) {
    if (m_lessT(a, b)) {
      return a;
    } else {
      ASSERT(m_lessT(b, a));
      return b;
    }
  }
  T maxT(T const &a, T const &b) {
    if (m_lessT(a, b)) {
      return b;
    } else {
      ASSERT(m_lessT(b, a));
      return a;
    }
  }*/
  void add_vec(set<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, vector<expr_ref> const &rhs, T val)
  {
    if (g_ctx->expr_query_caches_are_disabled()) { //HACK
      return;
    }
    vector<expr_id_t> rhs_ids;
    for (auto r : rhs) {
      rhs_ids.push_back(r->get_id());
    }
    if (m_cache.count(rhs_ids) != 0) {
      auto& deck =  m_cache[rhs_ids].second;
      deck.push_back(make_tuple(val, lhs_set, precond, sprel_map_pair, src_suffixpath, src_avail_exprs));
      if (deck.size() > EXPR_QUERY_CACHE_SIZE)
        deck.pop_front();
    } else {
      deque<tuple<T, set<predicate_ref>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, avail_exprs_t>> ls;
      ls.push_back(make_tuple(val, lhs_set, precond, sprel_map_pair, src_suffixpath, src_avail_exprs));
      m_cache[rhs_ids] = make_pair(rhs, ls);
    }
  }

  /* assumes subset x bool lattice with top = empty set x true, and bottom = full set x false; lower-bound (lb) corresponds to full set x true; upper-bound (ub) corresponds to empty set x false */
  void find_bounds_vec(set<predicate_ref> const &lhs_predicates, precond_t const &lhs_precond, sprel_map_pair_t const &lhs_sprel_map_pair, tfg_suffixpath_t const &lhs_suffixpath, avail_exprs_t const &lhs_avail_exprs, vector<expr_ref> const &rhs_vec, bool &lb_found, T &lb, bool &ub_found, T &ub)
  {
    if (g_ctx->expr_query_caches_are_disabled()) { //HACK
      return;
    }
    ++m_total;
    //print when elapsed is > average
    vector<expr_id_t> rhs_ids;
    for (auto r : rhs_vec) {
      rhs_ids.push_back(r->get_id());
    }
    lb_found = false;
    ub_found = false;
    if (m_cache.count(rhs_ids) == 0) {
      ++m_misses;
      return;
    }
    deque<tuple<T, set<predicate_ref>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, avail_exprs_t>> const &ls = m_cache.at(rhs_ids).second;
    set<predicate_ref> const *cur_top_predicates = nullptr, *cur_bottom_predicates = nullptr;
    precond_t const *cur_top_precond = nullptr, *cur_bottom_precond = nullptr;
    sprel_map_pair_t const *cur_top_sprel_map_pair = nullptr, *cur_bottom_sprel_map_pair = nullptr;
    tfg_suffixpath_t const *cur_top_suffixpath = nullptr, *cur_bottom_suffixpath = nullptr;
    avail_exprs_t const *cur_top_avail_exprs = nullptr, *cur_bottom_avail_exprs = nullptr;
    for (auto const& p : ls) {
      set<predicate_ref> const &predicates = get<1>(p);
      precond_t const &precond = get<2>(p);
      sprel_map_pair_t const &sprel_map_pair = get<3>(p);
      tfg_suffixpath_t const &suffixpath = get<4>(p);
      avail_exprs_t const &avail_exprs = get<5>(p);
      if (   lhs_precond.precond_implies(precond)
          && tfg::tfg_suffixpath_implies(lhs_suffixpath, suffixpath)
          && lhs_sprel_map_pair.sprel_map_pair_implies(sprel_map_pair)
          && set_contains(lhs_predicates, predicates)
          && lhs_avail_exprs.avail_exprs_contains(avail_exprs)
          && (   !ub_found
              || (   precond.precond_implies(*cur_top_precond)
                  && tfg::tfg_suffixpath_implies(suffixpath, *cur_top_suffixpath)
                  && sprel_map_pair.sprel_map_pair_implies(*cur_top_sprel_map_pair)
                  && set_contains(predicates, *cur_top_predicates)
                  && avail_exprs.avail_exprs_contains(*cur_top_avail_exprs)))) {
        ub = get<0>(p);
        cur_top_predicates = &predicates;
        cur_top_precond = &precond;
        cur_top_sprel_map_pair = &sprel_map_pair;
        cur_top_suffixpath = &suffixpath;
        cur_top_avail_exprs = &avail_exprs;
        ub_found = true;
      }
      if (   precond.precond_implies(lhs_precond)
          && tfg::tfg_suffixpath_implies(suffixpath, lhs_suffixpath)
          && sprel_map_pair.sprel_map_pair_implies(lhs_sprel_map_pair)
          && set_contains(predicates, lhs_predicates)
          && avail_exprs.avail_exprs_contains(lhs_avail_exprs)
          && (   !lb_found
              || (   cur_bottom_precond->precond_implies(precond)
                  && tfg::tfg_suffixpath_implies(*cur_bottom_suffixpath, suffixpath)
                  && cur_bottom_sprel_map_pair->sprel_map_pair_implies(sprel_map_pair)
                  && set_contains(*cur_bottom_predicates, predicates)
                  && cur_bottom_avail_exprs->avail_exprs_contains(avail_exprs)))) {
        lb = get<0>(p);
        cur_bottom_predicates = &predicates;
        cur_bottom_precond = &precond;
        cur_bottom_sprel_map_pair = &sprel_map_pair;
        cur_bottom_suffixpath = &suffixpath;
        cur_bottom_avail_exprs = &avail_exprs;
        lb_found = true;
      }
    }
    if (ub_found && lb_found) m_both_found++;
    if (ub_found || lb_found) m_either_found++;
  }

public:
  expr_query_cache_t()
  {
  }

  void clear()
  {
    m_cache.clear();
  }

  void add(predicate_set_t const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, expr_ref rhs, T val)
  {
    vector<expr_ref> rhs_vec;
    rhs_vec.push_back(rhs);
    add_vec(set<predicate_ref>(lhs_set.begin(),lhs_set.end()), precond, sprel_map_pair, src_suffixpath, src_avail_exprs, rhs_vec, val);
  }

  void add(predicate_set_t const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, expr_ref rhs1, expr_ref rhs2, T val)
  {
    vector<expr_ref> rhs_vec;
    rhs_vec.push_back(rhs1);
    rhs_vec.push_back(rhs2);
    add_vec(set<predicate_ref>(lhs_set.begin(),lhs_set.end()), precond, sprel_map_pair, src_suffixpath, src_avail_exprs, rhs_vec, val);
  }

  void find_bounds(predicate_set_t const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, expr_ref rhs, bool &lb_found, T &lb, bool &ub_found, T &ub)
  {
    vector<expr_ref> rhs_vec;
    rhs_vec.push_back(rhs);
    find_bounds_vec(set<predicate_ref>(lhs_set.begin(),lhs_set.end()), precond, sprel_map_pair, src_suffixpath, src_avail_exprs, rhs_vec, lb_found, lb, ub_found, ub);
  }

  void find_bounds(predicate_set_t const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, expr_ref rhs1, expr_ref rhs2, bool &lb_found, T &lb, bool &ub_found, T &ub)
  {
    vector<expr_ref> rhs_vec;
    rhs_vec.push_back(rhs1);
    rhs_vec.push_back(rhs2);
    find_bounds_vec(set<predicate_ref>(lhs_set.begin(),lhs_set.end()), precond, sprel_map_pair, src_suffixpath, src_avail_exprs, rhs_vec, lb_found, lb, ub_found, ub);
  }

  string stats(string const& pfx) const
  {
    size_t num_buckets = m_cache.size();
    if (num_buckets == 0)
      return "";

    stringstream ss;
    ss << pfx << ":\n";
    ss << "\tnum_buckets:\t\t" << num_buckets << endl;

    auto itr = m_cache.begin();
    size_t max_bucket_size = itr->second.second.size();
    size_t min_bucket_size = itr->second.second.size();
    size_t cumulative_bucket_size = itr->second.second.size();
    ++itr;

    for ( ; itr != m_cache.end(); ++itr) {
      auto sz = itr->second.second.size();
      if (sz > max_bucket_size) {
        max_bucket_size = sz;
      }
      else if (sz < min_bucket_size) {
        min_bucket_size = sz;
      }
      cumulative_bucket_size += sz;
    }

    ss << "\tmin_bucket_size:\t" << min_bucket_size << endl
       << "\tavg_bucket_size:\t" << (float)cumulative_bucket_size / num_buckets << endl
       << "\tmax_bucket_size:\t" << max_bucket_size << endl
       << "\t----lookup misses:\t" << m_misses << endl
       << "\t------ both-found:\t" << m_both_found << endl
       << "\t---- either-found:\t" << m_either_found << endl
       << "\t----------- total:\t" << m_total << endl
       << "\t lookup hit ratio:\t" << (float) (m_total - m_misses) / m_total << endl
       << "\t-- both hit ratio:\t" << (float) (m_both_found) / m_total << endl
       << "\t either hit ratio:\t" << (float) (m_either_found) / m_total << endl;


    return ss.str();
  }

  unsigned long get_total_lookups() const { return m_total; }
};

}
