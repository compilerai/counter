#include "graph/query_decomposition.h"
#include <fstream>
#include "eq/corr_graph.h"
#include "expr/context_cache.h"
//#include "expr/expr_query_cache.h"
#include "tfg/predicate.h"
#include "tfg/parse_input_eq_file.h"

namespace eqspace
{

class expr_subexpr_size_visitor : public expr_visitor
{
public:
  expr_subexpr_size_visitor(expr_ref const& e)
  {
    visit_recursive(e);
  }

  map<expr_ref,unsigned long> const& get_result() const { return m_map; }

private:
  void visit(expr_ref const &e) override;

  map<expr_ref,unsigned long> m_map;
};

void
expr_subexpr_size_visitor::visit(expr_ref const& e)
{
  if (e->is_var() || e->is_const()) {
    m_map[e] = 1;
    return;
  }
  ASSERT(e->get_args().size() > 0);
  size_t ret = 0;
  for (size_t i = 0; i < e->get_args().size(); i++) {
    ret += m_map.at(e->get_args()[i]);
  }
  m_map[e] = ++ret;
}

set<pair<expr_ref,unsigned long>> expr_get_subexpr_size(expr_ref const& e)
{
  expr_subexpr_size_visitor vtor(e);
  auto const& res = vtor.get_result();
  return set<pair<expr_ref,unsigned long>>(res.begin(), res.end());
}

class replace_expr_visitor : public expr_visitor {
public:
  replace_expr_visitor(expr_ref const &e, map<expr_ref,expr_ref> const& dst_to_src_map)
  : m_in(e),
    m_dst_to_src_map(dst_to_src_map)
  {
    visit_recursive(m_in);
  }

  expr_ref get_result() const { return m_map.at(m_in->get_id()); }

private:
  virtual void visit(expr_ref const &);

  expr_ref const &m_in;
  map<expr_ref,expr_ref> const& m_dst_to_src_map;
  map<expr_id_t,expr_ref> m_map;
};

query_decomposer::query_decomposer(expr_ref const &precond_expr, expr_ref const &src, expr_ref const &dst)
: m_src(src),
  m_dst(dst),
  m_precond_expr(precond_expr)
{
  context* ctx = src->get_context();
  set<pair<expr_ref,unsigned long>> src_expr_to_size, dst_expr_to_size;
  if (   m_src == expr_true(ctx)
      || m_dst == expr_true(ctx)) {
    // probably has memmasks_are_equal;
    // if so, populate m_src_expr_to_size and m_dst_expr_to_size based on subexprs
    expr_ref target = m_src == expr_true(ctx) ? m_dst : m_src;
    expr_find_op find_memmasks_are_equal(target, expr::OP_MEMMASKS_ARE_EQUAL);
    expr_vector op_memmasks_are_equals_subexprs = find_memmasks_are_equal.get_matched_expr();
    for (auto const& e : op_memmasks_are_equals_subexprs) {
      auto const& this_src_expr_to_size = expr_get_subexpr_size(e->get_args().at(0));
      auto const& this_dst_expr_to_size = expr_get_subexpr_size(e->get_args().at(1));
      src_expr_to_size.insert(this_src_expr_to_size.begin(), this_src_expr_to_size.end());
      dst_expr_to_size.insert(this_dst_expr_to_size.begin(), this_dst_expr_to_size.end());
    }
  } else {
    src_expr_to_size = expr_get_subexpr_size(m_src);
    dst_expr_to_size = expr_get_subexpr_size(m_dst);
  }
  m_src_expr_to_size = vector<pair<expr_ref,unsigned long>>(src_expr_to_size.begin(), src_expr_to_size.end());
  m_dst_expr_to_size = vector<pair<expr_ref,unsigned long>>(dst_expr_to_size.begin(), dst_expr_to_size.end());
  DYN_DEBUG(query_decompose, cout << _FNLN_ << ": src subexprs = " << m_src_expr_to_size.size() << endl);
  DYN_DEBUG(query_decompose, cout << _FNLN_ << ": dst subexprs = " << m_dst_expr_to_size.size() << endl);
  sort(m_src_expr_to_size.begin(), m_src_expr_to_size.end(), sortbysec);
  sort(m_dst_expr_to_size.begin(), m_dst_expr_to_size.end(), sortbysec);
}

void replace_expr_visitor::visit(expr_ref const &e)
{
  context *ctx = e->get_context();
  expr_vector new_args;
  expr_ref ret;
  if (m_dst_to_src_map.count(e)) {
    m_map[e->get_id()] = m_dst_to_src_map.at(e);
    return;
  }
  if (e->get_args().size()==0) {
    m_map[e->get_id()] = e;
    return;
  }
  for (size_t i = 0; i < e->get_args().size(); i++) {
    expr_ref arg = e->get_args().at(i);
    expr_ref new_arg = m_map.at(arg->get_id());
    new_args.push_back(new_arg);
  }
  ret = ctx->mk_app(e->get_operation_kind(), new_args);
  m_map[e->get_id()] = ret;
  return;
}

expr_ref query_decomposer::replace_expr_helper(expr_ref e)
{
  replace_expr_visitor visitor(e, m_dst_to_src_map);
  expr_ref ret = visitor.get_result();
  return ret;
}

bool query_decomposer::find_mappings(list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, graph_memlabel_map_t const &memlabel_map, const query_comment& qc, bool timeout_res, list<counter_example_t> &counter_examples, memlabel_assertions_t const& mlasserts, aliasing_constraints_t const& rhs_alias_cons, tfg const &src_tfg, consts_struct_t const &cs)
{
  vector<pair<expr_ref,expr_ref>> comp_with_src_expr;
  context *ctx = m_dst->get_context();
  expr_ref true_expr = expr_true(ctx);
  expr_ref bit1_expr = ctx->mk_bv_const(1, 1);
  expr_ref bit0_expr = ctx->mk_bv_const(1, 0);

  comp_with_src_expr.reserve(m_dst_expr_to_size.size()*m_src_expr_to_size.size()/2); // m*n/2
  for (auto it2 = m_dst_expr_to_size.begin(); it2 != m_dst_expr_to_size.end(); ++it2) {
    expr_ref dst_part = it2->first;
    if (!dst_part->is_const()) {
      if (dst_part->is_bool_sort()) {
        comp_with_src_expr.push_back(make_pair(dst_part, true_expr));
      } else if (   dst_part->is_bv_sort()
                 && dst_part->get_sort()->get_size() == 1) {
        comp_with_src_expr.push_back(make_pair(dst_part, bit1_expr));
        comp_with_src_expr.push_back(make_pair(dst_part, bit0_expr));
      }
    }
  }
  for(auto it = m_src_expr_to_size.begin(); it != m_src_expr_to_size.end(); ++it) {
    expr_ref src_part = it->first;
    for(auto it2 = m_dst_expr_to_size.begin(); it2 != m_dst_expr_to_size.end(); ++it2) {
      expr_ref dst_part = it2->first;
      if (   !src_part->is_int_sort()
          && !dst_part->is_const()
          && src_part->get_sort() == dst_part->get_sort()) {
        comp_with_src_expr.push_back(make_pair(dst_part, src_part));
      }
    }
  }
  DYN_DEBUG(query_decompose, cout << __func__ << " " << __LINE__ <<" Total Comparisons to be done: " << comp_with_src_expr.size() << endl);
  unsigned query_cnt = 0;
  unsigned timed_out_queries = 0;
  unsigned mappings_found = 0;
  for (size_t i = 0; i < comp_with_src_expr.size(); i++) {
    if (!m_dst_to_src_map.count(comp_with_src_expr[i].first)) {
      query_cnt++;
      expr_ref src_part = comp_with_src_expr[i].second;
      expr_ref dst_part = replace_expr_helper(comp_with_src_expr[i].first);
      DYN_DEBUG2(query_decompose, cout << __func__ << " " << __LINE__ <<" query #" << query_cnt << ", mappings found = " << mappings_found << ", timeouts = " << timed_out_queries << endl);
      solver::solver_res rslt = is_expr_equal_using_lhs_set_and_precond_helper(ctx, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, src_part, dst_part, qc, timeout_res, counter_examples, mlasserts, rhs_alias_cons, src_tfg, cs);
      if (   rslt == solver::solver_res_false
          && dst_part->is_bool_sort()) {
        // try again with negation
        src_part = ctx->mk_not(src_part);
        rslt = is_expr_equal_using_lhs_set_and_precond_helper(ctx, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, src_part, dst_part, qc, timeout_res, counter_examples, mlasserts, rhs_alias_cons, src_tfg, cs);
      }
      counter_examples.clear();

      if (rslt == solver::solver_res_true) {
        m_dst_to_src_map[comp_with_src_expr[i].first] = src_part;
        ++mappings_found;
      } else if (rslt == solver::solver_res_timeout) {
        ++timed_out_queries;
      }

      if (timed_out_queries > QUERY_DECOMPOSE_TIMED_OUT_QUERIES_THRESHOLD) {
        break; // return with whatever we have
      }
      if (query_cnt > QUERY_DECOMPOSE_QUERIES_THRESHOLD) {
        break;
      }
    }
  }
  DYN_DEBUG(query_decompose, cout << __func__ << " " << __LINE__ <<" Total Comparisons done: " << query_cnt << endl);
  if (query_cnt) {
    // This additional check because many times m_dst is equal to some sub expression of m_src and without this check it will return false instead of true; testcase: ht_hashcode
    DYN_DEBUG3(query_decompose, cout << __func__ << " " << __LINE__ << " final expr is\n" << ctx->expr_to_string_table(ctx->mk_eq(replace_expr_helper(m_src), replace_expr_helper(m_dst)), false) << endl);
    solver::solver_res rslt = is_expr_equal_using_lhs_set_and_precond_helper(ctx, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, replace_expr_helper(m_src), replace_expr_helper(m_dst), qc, timeout_res, counter_examples, mlasserts, rhs_alias_cons, src_tfg, cs);
    return (rslt == solver::solver_res_true);
  } else {
    // no simplification was made; return with failure
    return false;
  }
}

}
