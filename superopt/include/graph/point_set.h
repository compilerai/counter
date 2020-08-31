#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/bv_const.h"
#include "support/bv_const_expr.h"
#include "support/bv_const_ref.h"
#include "support/scatter_gather.h"
#include "support/bv_solve.h"
#include "graph/point_set_helper.h"
#include "graph/point.h"
#include "graph/expr_group.h"
#include "support/matrix.h"

namespace eqspace {

using namespace std;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_guessing;

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

class point_set_t
{
private:
  using expr_idx_t = expr_group_t::expr_idx_t;
  expr_group_ref m_exprs;
  set<expr_ref> m_unreachable_exprs;
  vector<point_ref> m_points;
  context *m_ctx;
public:
  point_set_t(expr_group_ref const &exprs, context *ctx) : m_exprs(exprs), m_ctx(ctx)
  {
    stats_num_point_set_constructions++;
  }

  point_set_t(istream& is, string const& prefix, context* ctx) : m_exprs(mk_expr_group(is, ctx, prefix)), m_ctx(ctx)
  {
    stats_num_point_set_constructions++;
  }

  point_set_t(point_set_t const& other) : m_exprs(other.m_exprs), m_unreachable_exprs(other.m_unreachable_exprs), m_points(other.m_points), m_ctx(other.m_ctx)
  {
    stats_num_point_set_constructions++;
  }

  ~point_set_t()
  {
    stats_num_point_set_destructions++;
  }

  expr_group_ref const& get_exprs() const
  {
    return m_exprs;
  }

  void point_set_to_stream(ostream& os, string const& prefix) const
  {
    m_exprs->expr_group_to_stream(os, prefix);
  }

  bool point_set_union(point_set_t const &other)
  {
    ASSERT(this->m_exprs == other.m_exprs);
    //ASSERT(this->m_arr_exprs == other.m_arr_exprs);
    //ASSERT(this->m_bv_exprs == other.m_bv_exprs);
    //ASSERT(this->m_max_bvlen == other.m_max_bvlen);
    ASSERT(this->m_ctx == other.m_ctx);
    size_t old_size = this->m_points.size();
    vector_union(this->m_points, other.m_points);
    ASSERT(this->m_points.size() >= old_size);
    return this->m_points.size() != old_size;
  }

  template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
  bool add_point_using_CE(nodece_ref<T_PC, T_N, T_E> const &nce, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g)
  {
    context *ctx = g.get_context();
    counter_example_t ce = nce->nodece_get_counter_example(&g);
    counter_example_t rand_vals(ctx);
    vector<point_val_t> point_vals;
    bool success;
    set<memlabel_ref> const& relevant_memlabels = g.get_relevant_memlabels();
    set<size_t> masked_expr_idx;

    for (size_t i = 0; i < m_exprs->size(); ++i) {
      expr_ref const& e = m_exprs->at(i);
      expr_ref c;
      bool is_outof_bound;
      success = ce.evaluate_expr_assigning_random_consts_and_check_bounds(e, c, rand_vals, relevant_memlabels, is_outof_bound);
      if (!success) { //this can happen during propagation but should not happen for the to_pc of the edge across which the CE was generated (during invariant inference). We do not assert this condition currently, but it would be good to find a way to assert this in future
        DYN_DEBUG(ce_add, cout << __func__ << " " << __LINE__ << ": CE evaluation failed over expression " << expr_string(e) << endl);
        DYN_DEBUG(ce_add, cout << "ce =\n" << ce.counter_example_to_string() << endl);
        DYN_DEBUG(ce_add, cout << "e =\n" << ctx->expr_to_string_table(e) << endl);
        if(is_outof_bound){
          masked_expr_idx.insert(i);
          continue;
        }
        return false;
      }
      ASSERT(c->is_const());
      point_val_t pval(c);
      point_vals.push_back(pval);
    }
    if(masked_expr_idx.size())
      mask_unreachable_expr_at_index(masked_expr_idx);
    m_points.push_back(mk_point(point_vals));

    /*if (point_vals.front().is_arr()) {
      list<expr_idx_t> const &arr_indices = m_exprs->get_arr_indices();
      expr_idx_t idx1 = arr_indices.front();
      expr_idx_t idx2 = arr_indices.back();
      if (!point_vals.at(idx1).get_arr_const().equals(point_vals.at(idx2).get_arr_const())) {
        CPP_DBG_EXEC(POINT_SET, cout << __func__ << " " << __LINE__ << ": arrays mismatch.\n"
            << expr_string(m_exprs->at(idx1)) << " = " << point_vals.at(idx1).get_arr_const().to_string() << '\n'
            << expr_string(m_exprs->at(idx2)) << " = " << point_vals.at(idx2).get_arr_const().to_string() << endl);
        CPP_DBG_EXEC(POINT_SET, cout << __func__ << " " << __LINE__ << ": ce:\n" << ce.counter_example_to_string() << endl);
      }
    }*/
    return true;
  }

  void pop_back()
  {
    m_points.pop_back();
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

  vector<point_ref> const& get_points_vec() const { return m_points; }

  string point_set_to_string(string const &prefix) const
  {
    stringstream ss;
    size_t n = 1;

    for (const auto &point : m_points) {
      ASSERT(point->size() == m_exprs->size());
      ss << prefix << "p" << n++ << ":\n";
      for (size_t i = 0; i < m_exprs->size(); i++) {
        string new_prefix = prefix + "  ";
        ss << new_prefix << expr_string(m_exprs->at(i)) << " -> " << point->at(i).point_val_to_string() << endl;
      }
    }

    ss << "matrix:\n";
    ss << m_points.size() << " " << m_exprs->size() << " " << m_exprs->get_max_bvlen() << endl;
    for (const auto &point : m_points) {
      for (size_t i = 0; i < m_exprs->size(); i++) {
        ss << " " << point->at(i).point_val_to_string();
      }
      ss << endl;
    }
    ss << endl;
    ss << "Unreachable exprs:\n";
    for (auto const &e: m_unreachable_exprs)
      ss << expr_string(e) << ", ";
    ss << endl;
    return ss.str();
  }

  map<bv_solve_var_idx_t, vector<bv_const_ref>> solve_for_bv_points() const
  {
    autostop_timer func_timer(__func__);
    //static char tbuf[256];
    ASSERT(m_points.size() != 0);
    ASSERT(m_exprs->get_max_bvlen() != -1);
    vector<vector<bv_const>> input_points;
    input_points.reserve(m_points.size());
    //list<expr_idx_t> const &bv_indices = m_exprs->get_bv_indices();
    size_t num_exprs = m_exprs->size();
    for (point_ref const& pt : m_points) {
      vector<bv_const> input_pt;
      input_pt.reserve(num_exprs);
      for (size_t idx = 0; idx < num_exprs; idx++) {
        ASSERT(idx >= 0 && idx < pt->size());
        ASSERT(m_exprs->size() == pt->size());
        input_pt.push_back(pt->at(idx).get_bv_const());
      }
      input_points.push_back(input_pt);
    }

    CPP_DBG_EXEC(POINT_SET, cout << __func__ << ':' << __LINE__ << ": calling bv_solve, input_points.size() = " << input_points.size() << endl);
    CPP_DBG_EXEC2(POINT_SET, cout << __func__ << ':' << __LINE__ << ": Input points:\n"; for (auto const& pt : input_points) cout << '[' << bv_const_vector2str(pt) << "]\n");
    map<bv_solve_var_idx_t, vector<bv_const>> coeff = bv_solve(input_points, m_exprs->get_max_bvlen());
    CPP_DBG_EXEC(POINT_SET, cout << __func__ << ':' << __LINE__ << ": number of soln rows: " << coeff.size() << endl);

    map<bv_solve_var_idx_t, vector<bv_const_ref>> ret;
    for (auto const& coeff_row : coeff) {
      vector<bv_const_ref> bvr_vec;
      bvr_vec.reserve(coeff_row.second.size());
      for (auto const c : coeff_row.second) {
        bvr_vec.push_back(mk_bv_const_ref(c));
      }
      ret.insert(make_pair(coeff_row.first, bvr_vec));
    }
    return ret;
  }

  predicate_set_t compute_preds_for_bv_points(matrix_t<bool> &coeff_matrix, map<bv_solve_var_idx_t, vector<bv_const_ref>> const& coeff) const
  {
    autostop_timer func_timer(__func__);
    predicate_set_t ret;
    if (m_points.size() == 0) {
      ret.insert(predicate::false_predicate(m_ctx));
      return ret;
    }
    if (m_exprs->get_max_bvlen() == -1) {
      return ret;
    }

    // gather exprs indexed by m_bv_exprs
    //vector<expr_ref> corresponding_exprs = scatter_gather_t(bv_indices).gather_vector(m_exprs->get_expr_vec());
    vector<expr_ref> const& corresponding_exprs = m_exprs->get_expr_vec();
    for (auto const &var_coeff_row : coeff) {
      bv_solve_var_idx_t var_idx = var_coeff_row.first;
      auto const &coeff_row = var_coeff_row.second;
      //CPP_DBG_EXEC2(POINT_SET, cout << __func__ << ':' << __LINE__ << ": soln row: [ " << bv_const_vector2str(coeff_row) << "]\n");
      predicate_ref new_pred = gen_pred_from_coeff_row(coeff_row, corresponding_exprs, var_idx);
      if (!new_pred->predicate_is_trivial()) {
        ret.insert(new_pred);
        coeff_matrix.push_back(to_bool(coeff_row));
      }
    }
    CPP_DBG_EXEC(POINT_SET, cout << __func__ << ':' << __LINE__ << ": returning ret.size() = " << ret.size() << "\n");
    return ret;
  }

  bv_const get_smallest_value_signed() const
  {
    //cout << __func__ << " " << __LINE__ << ": calling fold_over_points_with_single_expr\n";
    auto n = this->get_bvlen();
    bv_const ret = fold_over_points_with_single_expr(
        [](bv_const const& a, bv_const const& b) { return bv_cmp(a, b) > 0; },
        [n]() { return bv_sub(bv_exp2(n - 1), bv_one()); },
        true
    );
    //cout << __func__ << " " << __LINE__ << ": returned fold_over_points_with_single_expr\n";
    return bv_convert_to_signed(ret, n);
  }
  bv_const get_largest_value_signed() const
  {
    auto n = this->get_bvlen();
    bv_const ret = fold_over_points_with_single_expr(
        [](bv_const const& a, bv_const const& b) { return bv_cmp(a, b) < 0; },
        [n]() { return bv_neg(bv_exp2(n - 1)); },
        true
    );
    return bv_convert_to_signed(ret, n);
  }
  bv_const get_smallest_value_unsigned() const
  {
    auto n = this->get_bvlen();
    return fold_over_points_with_single_expr(
        [](bv_const const& a, bv_const const& b) { return bv_cmp(a, b) > 0; },
        [n]() { return bv_sub(bv_exp2(n), bv_one()); },
        false
    );
  }
  bv_const get_largest_value_unsigned() const
  {
    //auto n = this->get_bvlen();
    return fold_over_points_with_single_expr(
        [](bv_const const& a, bv_const const& b) { return bv_cmp(a, b) < 0; },
        []() { return bv_zero(); },
        false
    );
  }

private:
  void mask_unreachable_expr_at_index(set<size_t> idx_set)
  {
    ASSERT(m_exprs->size() != 1);
    //erase in decreasing order only otherwise the idx for later elems will change
    vector<expr_ref> correlation_exprs;
    //vector<edge_guard_t> expr_guards;
    m_exprs->get_expr_vec(correlation_exprs);
    //m_exprs->get_expr_guards(expr_guards);
    for(auto it=idx_set.crbegin(); it != idx_set.crend(); ++it)
    {
      size_t idx = *it;
      CPP_DBG_EXEC2(CE_ADD, cout << __func__ << "  " << __LINE__ << " masking " << expr_string(correlation_exprs.at(idx)) << endl);
      m_unreachable_exprs.insert(correlation_exprs.at(idx));
      correlation_exprs.erase(correlation_exprs.begin() + idx);
      //if(expr_guards.size()) {
      //  ASSERT(expr_guards.size() == (correlation_exprs.size() + 1));
      //  expr_guards.erase(expr_guards.begin() + idx);
      //}
    }
    expr_group_ref new_exprs = mk_expr_group(m_exprs->get_type(), correlation_exprs/*, expr_guards*/);
    m_exprs = new_exprs;
    vector<point_ref> new_points;
    for(auto const &point : m_points)
    {
      vector<point_val_t> point_vals;
      point_vals.insert(point_vals.end(), point->get_vals().begin(), point->get_vals().end());
      for(auto it=idx_set.crbegin(); it != idx_set.crend(); ++it)
      {
        size_t idx = *it;
        point_vals.erase(point_vals.begin()+idx);
      }
      new_points.push_back(mk_point(point_vals));
    }
    m_points = new_points;
  }

  size_t get_bvlen() const
  {
    ASSERT(m_exprs->size() == 1);
    expr_ref const& e = m_exprs->at(0);
    ASSERT(e->is_bv_sort());
    return e->get_sort()->get_size();
  }

  bv_const
  fold_over_points_with_single_expr(std::function<bool (bv_const const&, bv_const const&)> f, std::function<bv_const ()> init_val, bool is_signed) const
  {
    ASSERT(m_exprs->size() == 1);
    expr_ref const& e = m_exprs->at(0);
    ASSERT(e->is_bv_sort());
    size_t bvlen = e->get_sort()->get_size();
    bv_const ret = init_val();
    //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
    for (auto const& pt : m_points) {
      bv_const cur = pt->at(0).get_bv_const();
      if (is_signed) {
        cur = bv_convert_to_signed(cur, bvlen);
      }
      //cout << __func__ << " " << __LINE__ << ": cur = " << cur << endl;
      if (f(ret, cur)) {
        ret = cur;
      }
      //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
    }
    return ret;
  }

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

  static unsigned
  get_min_bvlen_for_generating_pred_from_coeff_row(vector<bv_const_ref> const& coeff_row, vector<expr_ref> const& corresponding_exprs, unsigned max_bvlen)
  {
    unsigned min_divisor_log2 = max_bvlen;
    ASSERT(coeff_row.size() == corresponding_exprs.size() + 1);
    for (size_t i = 0; i < coeff_row.size(); i++) {
      if (coeff_row.at(i)->get_bv() == 0) {
        continue;
      }
      min_divisor_log2 = min(get_max_divisor_log2(coeff_row.at(i)->get_bv()), min_divisor_log2);
    }
    return max_bvlen - min_divisor_log2;
  }

  predicate_ref gen_pred_from_coeff_row(vector<bv_const_ref> const &coeff_row, vector<expr_ref> const& corresponding_exprs, expr_idx_t free_var_idx) const
  {
    autostop_timer ft(__func__);

    unsigned max_bvlen = m_exprs->get_max_bvlen();
    unsigned bvlen = get_min_bvlen_for_generating_pred_from_coeff_row(coeff_row, corresponding_exprs, max_bvlen);
    vector<bv_const_ref> shr_coeff_row;
    shr_coeff_row.reserve(coeff_row.size());
    for (auto const& c : coeff_row) {
      ASSERT(bvlen <= max_bvlen);
      if (bvlen < max_bvlen) {
        shr_coeff_row.push_back(mk_bv_const_ref(bv_ashr(c->get_bv(), max_bvlen - bvlen)));
      } else {
        shr_coeff_row.push_back(c);
      }
    }

		// get all term coeffs excluding the constant term which is the last element
		vector<bv_const_ref> term_coeffs = scatter_gather_t(shr_coeff_row.size()-1).gather_vector(shr_coeff_row);
		bv_const_ref const_term = shr_coeff_row.back();

		expr_vector term_coeffs_exprs = bv_const_ref_vector2expr_vector(term_coeffs, m_ctx, bvlen);
		expr_ref const_term_expr = bv_const2expr(const_term->get_bv(), m_ctx, bvlen);

		expr_ref lhs = m_ctx->mk_zerobv(bvlen);
		expr_ref rhs = const_term_expr;
		construct_linear_combination_exprs(corresponding_exprs, term_coeffs_exprs, rhs);
    {
    //autostop_timer simplify_timer(string(__func__) + ".simplify");
		rhs = m_ctx->expr_do_simplify(rhs);
    }

		stringstream ss2;
		ss2 << PRED_LINEAR_PREFIX << count_if(shr_coeff_row.begin(), prev(shr_coeff_row.end()), [](bv_const_ref const& a) { return a->get_bv() != 0; }) << '-' << bvlen << "-" FREE_VAR_IDX_PREFIX << free_var_idx;
		predicate_ref ret = predicate::mk_predicate_ref(precond_t(), lhs, rhs, ss2.str());
		ret = ret->predicate_canonicalized();
		return ret;
  }
};

using point_set_ref = shared_ptr<point_set_t>;

}
