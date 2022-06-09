#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/bv_const.h"
#include "support/bv_const_expr.h"
#include "support/bv_const_ref.h"
#include "support/scatter_gather.h"
#include "support/bv_solve.h"
#include "support/matrix.h"

#include "gsupport/predicate_set.h"

#include "graph/point_set_helper.h"
#include "graph/point.h"
#include "graph/expr_group.h"
#include "graph/bvsolve_cache.h"
#include "graph/nodece.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_points;
template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_guessing;

/*
static
vector<bool> to_bool(vector<bv_const_ref> const& coeff_row)
{
  vector<bool> ret;
  ret.reserve(coeff_row.size());
  for (auto const& v : coeff_row) {
    ret.push_back(v->get_bv() != 0);
  }
  return ret;
}
*/

class point_set_t
{
public:
  using global_exprs_map_t = map<expr_group_t::expr_idx_t, point_exprs_t>;
private:
  using expr_idx_t = expr_group_t::expr_idx_t;
//  expr_group_ref m_exprs;
//  mutable set<expr_idx_t> m_unreachable_exprs;
  mutable map<string_ref, point_ref> m_points; //from point-name to point
  context *m_ctx;
  global_exprs_map_t m_global_exprs_map;

  mutable bvsolve_cache_t m_bvsolve_cache;
public:
  point_set_t(/*expr_group_ref const &exprs,*/ context *ctx, global_exprs_map_t const& global_eg) : /*m_exprs(exprs),*/ m_ctx(ctx), m_global_exprs_map(global_eg)
  {
    stats_num_point_set_constructions++;
  }

  point_set_t(istream& is, string const& prefix, context* ctx);

  point_set_t(point_set_t const& other) : /*m_exprs(other.m_exprs)*//*, m_unreachable_exprs(other.m_unreachable_exprs)*/ m_points(other.m_points), m_ctx(other.m_ctx), m_global_exprs_map(other.m_global_exprs_map)//, m_out_of_bound_exprs(other.m_out_of_bound_exprs)
  {
    stats_num_point_set_constructions++;
  }

  ~point_set_t()
  {
    stats_num_point_set_destructions++;
  }

  global_exprs_map_t const& get_global_exprs_map() const
  {
    return m_global_exprs_map;
  }

  //set<expr_idx_t> const& get_out_of_bound_exprs() const
  //{
  //  return m_out_of_bound_exprs;
  //}

  void update_global_exprs_map(global_exprs_map_t const& global_eg)
  {
    // ASSERT check that the new expr_group is superset of orig expr_group and the index for the exprs in the orig expr_group are maintained in the new expr_group
    m_global_exprs_map = global_eg;
  }

  bool point_exists(string const& point_name) const {
    for (auto const& [ptname, pt] : m_points) {
      ASSERT(ptname == pt->get_point_name());
      if (pt->get_point_name()->get_str() == point_name)
        return true;
    }
    return false;
  }

//  bool point_sets_are_identical(point_set_t const& other) const;

  //set<expr_idx_t> const& get_unreachable_exprs() const
  //{
  //  return m_unreachable_exprs;
  //}

  void point_set_to_stream(ostream& os, string const& prefix) const;

//  bool point_set_union(point_set_t const &other)
//  {
////    ASSERT(this->m_exprs == other.m_exprs);
//    //ASSERT(this->m_arr_exprs == other.m_arr_exprs);
//    //ASSERT(this->m_bv_exprs == other.m_bv_exprs);
//    //ASSERT(this->m_max_bvlen == other.m_max_bvlen);
//    ASSERT(this->m_ctx == other.m_ctx);
//    size_t old_size = this->m_points.size();
//    vector_union(this->m_points, other.m_points);
//    ASSERT(this->m_points.size() >= old_size);
//    return this->m_points.size() != old_size;
//  }

  template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
  bool point_set_add_point_using_CE(nodece_ref<T_PC, T_N, T_E> const &nce, graph_with_points<T_PC, T_N, T_E, T_PRED> const &g, expr_group_ref const& exprs, set<expr_group_t::expr_idx_t>& out_of_bound_exprs);

  void pop_back()
  {
// In SSA framework, it may not be a good idea to pop_back a point as it may cause computation duplication because the set of exprs is global 
// and the values are gathered from the same point for mutiple regions. While the point may not refine the invariants further for a particular region
// but it be useful for other regions and in that case if we pop the point, we will have to re-evaluate the entire point again for the next region.
// We can dedup the gather values for a region for same row or one row is scalar mutiple or constant offset from other row  
//    m_points.pop_back();
  }

  //unordered_set<predicate>
  //compute_preds_for_points() const
  //{
  //  unordered_set<predicate> ret;
  //  this->compute_preds_for_bv_points(ret);
  //  this->compute_preds_for_arr_points(ret);
  //  return ret;
  //}

  size_t size() const
  {
    return m_points.size();
  }

  map<string_ref, point_ref> const& get_points_vec() const { return m_points; }

  string point_set_to_string(string const &prefix) const
  {
    stringstream ss;
    //size_t n = 1;

    for (const auto &[ptname, point] : m_points) {
      ASSERT(ptname == point->get_point_name());
      ss << prefix << "point " << ptname->get_str()/*n++*/ << ":\n";
      ss << point->point_to_string(prefix, m_global_exprs_map);
    }

    ss << "matrix:\n";
    ss << m_points.size() << " " << m_global_exprs_map.size() << endl; //" " << exprs->get_max_bvlen() << endl;
    for (const auto &[ptname, point] : m_points) {
      ASSERT(ptname == point->get_point_name());
      for (auto const& [i, pval] : point->get_vals()) {
        ss << " " << pval.point_val_to_string();
      }
      ss << endl;
    }
    ss << endl;
//    ss << "Unreachable exprs:\n";
//    for (auto const &eid: m_unreachable_exprs) {
//      point_exprs_t const& pe = m_exprs->expr_group_get_point_exprs_at_index(eid);
//      expr_ref e = pe.get_expr();
//      ss << expr_string(e) << ", ";
//    }
//    ss << endl;
    return ss.str();
  }

  map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const_ref>> solve_for_bv_points(expr_group_ref const& exprs, set<string_ref> const& visted_ces, set<expr_group_t::expr_idx_t> const& out_of_bound_exprs) const;

  predicate_set_t compute_preds_for_bv_points(map<bv_solve_var_idx_t, map<bv_solve_var_idx_t, bv_const_ref>> const& coeff, expr_group_ref const& exprs_to_be_correlated) const;

  set<bv_const> get_bv_const_points_for_single_expr_eg(expr_group_ref const& eg, set<string_ref> const& visited_ces) const
  {
    set<bv_const> ret;
    map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs = eg->get_expr_vec();
    ASSERT(exprs.size() == 1);
    expr_ref const& e = exprs.begin()->second.get_expr();
    expr_group_t::expr_idx_t const& e_idx = exprs.begin()->first;
    ASSERT(e->is_bv_sort());
    size_t bvlen = e->get_sort()->get_size();
    //ASSERT(!m_out_of_bound_exprs.count(e_idx));
    for (auto const& [ptname, pt] : m_points) {
      ASSERT(ptname == pt->get_point_name());
      if (!visited_ces.count(pt->get_point_name()))
        continue;
      ASSERT(pt->get_vals().count(e_idx));
      ret.insert(pt->get_vals().at(e_idx).get_bv_const());
    }
    return ret;
  }


//  void remove_exprs_at_index(set<expr_group_t::expr_idx_t> const& idx_set) const
//  {
//    //auto idx_set = map_get_values(idx_map);
//    //if (m_exprs->size() < idx_set.size()) {
//    //  cout << _FNLN_ << ": m_exprs->size() = " << m_exprs->size() << ", idx_set.size() = " << idx_set.size() << endl;
//    //}
//    //ASSERT(m_exprs->size() >= idx_set.size());
//    if (!idx_set.size()) {
//      return;
//    }
//    ASSERT(idx_set.size());
//    //erase in decreasing order only otherwise the idx for later elems will change
//    map<expr_group_t::expr_idx_t, point_exprs_t> const& correlation_exprs = m_exprs->get_expr_vec();
//    //m_exprs->get_expr_vec(correlation_exprs);
//    for(auto const& idx : idx_set) {
//      //size_t idx = *it;
//      ASSERT(correlation_exprs.count(idx));
//      DYN_DEBUG2(ce_add, cout << __func__ << "  " << __LINE__ << " masking " << expr_string(correlation_exprs.at(idx).get_expr()) << endl);
//      //m_unreachable_exprs.insert(correlation_exprs.at(idx));
//      m_unreachable_exprs.insert(idx);
//    }
//    //map<expr_group_t::expr_idx_t, expr_ref> new_corr_exprs;
//    //for (auto iter = correlation_exprs.begin(); iter != correlation_exprs.end(); iter++) {
//    //  if (!set_belongs(idx_set, iter->first)) {
//    //    new_corr_exprs.insert(*iter);
//    //  }
//    //}
//    //correlation_exprs = new_corr_exprs;
//    //expr_group_ref new_exprs;
//    //if(m_exprs->get_type() == expr_group_t::EXPR_GROUP_TYPE_BV_EQ)
//    //  new_exprs = mk_expr_group(m_exprs->get_name(), m_exprs->get_type(), correlation_exprs/*, m_exprs->get_exprs_ranking_map()*//*, expr_guards*/);
//    //else
//    //  new_exprs = mk_expr_group(m_exprs->get_name(), m_exprs->get_type(), correlation_exprs/*, expr_guards*/);
//    //m_exprs = new_exprs;
//    vector<point_ref> new_points;
//    for(auto const &point : m_points)
//    {
//      map<expr_group_t::expr_idx_t, point_val_t> point_vals = point->get_vals();
//      //point_vals.insert(point_vals.end(), point->get_vals().begin(), point->get_vals().end());
//      for(auto it=idx_set.crbegin(); it != idx_set.crend(); ++it)
//      {
//        size_t idx = *it;
//        if (point_vals.count(idx)) {
//          point_vals.erase(idx);
//        }
//        //point_vals.erase(point_vals.begin()+idx);
//      }
//      new_points.push_back(mk_point(point->get_point_name()->get_str(), point_vals));
//    }
//    m_points = new_points;
//  }
  //void point_set_clear();
  void point_set_weaken_expr_idx_till_coeff_rank(expr_group_t::expr_idx_t expr_idx, unsigned int coeff_rank, unsigned int bvlen, set<string_ref> &visited_ces) const;
  static unsigned
  get_min_bvlen_for_generating_pred_from_coeff_row(map<expr_group_t::expr_idx_t, bv_const_ref> const& coeff_row, unsigned max_bvlen)
  {
    unsigned min_divisor_log2 = max_bvlen;
    //ASSERT(set_contains(map_get_keys(coeff_row), map_get_keys(corresponding_exprs)));
    //ASSERT(map_get_keys(coeff_row).size() == map_get_keys(corresponding_exprs).size() + 1);
    //ASSERT(coeff_row.size() == corresponding_exprs.size() + 1);
    for (auto const& [i,coeff] : coeff_row) {
      if (coeff->get_bv() == 0) {
        continue;
      }
      min_divisor_log2 = min(get_max_divisor_log2(coeff->get_bv()), min_divisor_log2);
    }
    return max_bvlen - min_divisor_log2;
  }

  static bool point_name_is_weakening_point(string const& str);
  static vector<map<bv_solve_var_idx_t, bv_const>> named_points_to_points_vec(map<string_ref, map<bv_solve_var_idx_t, bv_const>> const& named_points);

private:
  static void add_constant_term_to_point(map<bv_solve_var_idx_t, bv_const>& pt, string const& ptname);

  size_t get_bvlen(expr_group_ref const& eg) const
  {
    map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs = eg->get_expr_vec();
    ASSERT(exprs.size() == 1);
    expr_ref const& e = exprs.begin()->second.get_expr();
    ASSERT(e->is_bv_sort());
    //ASSERT(!m_out_of_bound_exprs.count(exprs.begin()->first));
    return e->get_sort()->get_size();
  }

  //bv_const
  //fold_over_points_with_single_expr(std::function<bool (bv_const const&, bv_const const&)> f, std::function<bv_const ()> init_val, bool is_signed, expr_group_ref const& eg) const
  //{
  //  map<expr_group_t::expr_idx_t, point_exprs_t> const& exprs = eg->get_expr_vec();
  //  ASSERT(exprs.size() == 1);
  //  expr_ref const& e = exprs.begin()->second.get_expr();
  //  expr_group_t::expr_idx_t const& e_idx = exprs.begin()->first;
  //  ASSERT(e->is_bv_sort());
  //  size_t bvlen = e->get_sort()->get_size();
  //  bv_const ret = init_val();
  //  //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
  //  for (auto const& pt : m_points) {
  //    if(!pt->count(e_idx))
  //      continue;
  //    bv_const cur = pt->at(e_idx).get_bv_const();
  //    if (is_signed) {
  //      cur = bv_convert_to_signed(cur, bvlen);
  //    }
  //    //cout << __func__ << " " << __LINE__ << ": cur = " << cur << endl;
  //    if (f(ret, cur)) {
  //      ret = cur;
  //    }
  //    //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
  //  }
  //  return ret;
  //}

  static unsigned
  get_max_divisor_log2(bv_const b)
  {
    unsigned ret = 0;
    while (1) {
      if (bv_isodd(b)) {
        return ret;
      }
      ret++;
      b = bv_ashr(b, 1);
    }
    NOT_REACHED();
  }

  predicate_ref gen_pred_from_coeff_row(map<expr_group_t::expr_idx_t, bv_const_ref> const &coeff_row, map<expr_group_t::expr_idx_t, point_exprs_t> const& corresponding_exprs, expr_idx_t free_var_idx, expr_group_ref const& exprs_to_be_correlated) const;
};

using point_set_ref = shared_ptr<point_set_t>;

}
