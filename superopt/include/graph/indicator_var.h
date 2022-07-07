#pragma once

#include "support/consts.h"
#include "support/types.h"
#include "support/debug.h"
#include "support/utils.h"

#include "expr/consts_struct.h"
#include "expr/context.h"
#include "expr/expr.h"
#include "expr/counter_example.h"
#include "expr/relevant_memlabels.h"

namespace eqspace {

class indicator_var_t {
private:
  expr_ref m_var;
  map<expr_id_t, expr_ref> m_possible_values;
public:
  indicator_var_t() {}
  indicator_var_t(expr_ref var, map<expr_id_t, expr_ref> const& possible_values) : m_var(var), m_possible_values(possible_values)
  { }
  expr_ref const& get_var() const { return m_var; }
  map<expr_id_t, expr_ref> const& get_possible_values() const { return m_possible_values; }
  expr_ref indicator_var_get_expr(context *ctx) const
  {
    expr_ref ret = expr_false(ctx);
    for (auto const& val : m_possible_values) {
      ret = expr_or(ret, expr_eq(m_var, val.second));
    }
    return ret;
  }
  bool operator==(indicator_var_t const& other) const
  {
    return m_var == other.m_var && m_possible_values == other.m_possible_values;
  }
  static bool indicator_var_compare_ids(indicator_var_t const& a, indicator_var_t const& b)
  {
    if (a.m_var->get_id() < b.m_var->get_id()) {
      return true;
    }
    if (a.m_var->get_id() > b.m_var->get_id()) {
      return false;
    }
    return a.m_possible_values < b.m_possible_values;
  }
  bool indicator_var_implies(indicator_var_t const& other) const
  {
    if (m_var != other.m_var) {
      return false;
    }
    for (auto const& val : this->m_possible_values) {
      if (!other.m_possible_values.count(val.first)) {
        return false;
      }
    }
    return true;
  }
  string to_string() const
  {
    std::stringstream ss;
    ss << expr_string(m_var) << ": {";
    for (auto const& v : m_possible_values) {
      ss << expr_string(v.second) << "; ";
    }
    ss << "}";
    return ss.str();
  }
  string to_string_for_eq(bool use_orig_ids) const
  {
    std::stringstream ss;
    context *ctx = m_var->get_context();
    ss << "=indicator_var\n";
    ss << ctx->expr_to_string_table(m_var, use_orig_ids) << '\n';
    size_t n = 0;
    for (auto const& v : m_possible_values) {
      ss << "==possible_val." << n << endl;
      ss << ctx->expr_to_string_table(v.second, use_orig_ids) << '\n';
      n++;
    }
    return ss.str();
  }
  indicator_var_t visit_exprs(std::function<expr_ref (const string &, expr_ref const &)> f) const
  {
    map<expr_id_t, expr_ref> new_map;
    expr_ref new_var;

    new_var = f("indicator_var", m_var);
    for (auto const& m : this->m_possible_values) {
      expr_ref new_val = f("indicator_val", m.second);
      new_map.insert(make_pair(new_val->get_id(), new_val));
    }
    return indicator_var_t(new_var, new_map);
  }
  void visit_exprs(std::function<void (const string &, expr_ref const &)> f) const
  {
    f("indicator_var", m_var);
    for (auto const& m : this->m_possible_values) {
      f("indicator_val", m.second);
    }
  }
  bool indicator_var_eval_on_counter_example(set<memlabel_ref> const& relevant_memlabels, bool& eval_result, counter_example_t& counter_example) const
  {
    context *ctx = m_var->get_context();
    expr_ref e = this->indicator_var_get_expr(ctx);
    counter_example_t rand_vals(ctx, ctx->get_next_ce_name(RAND_VALS_CE_NAME_PREFIX));
    expr_ref ret_expr;
    bool ret = counter_example.evaluate_expr_assigning_random_consts_as_needed(e, ret_expr, rand_vals, relevant_memlabels);
    if (!ret) {
      return false;
    }
    ASSERT(ret_expr->is_const());
    counter_example.counter_example_union(rand_vals);
    ASSERT(ret_expr->is_bool_sort());
    return ret_expr->get_bool_value();
  }
  static string read_indicator_var(istream& in, context* ctx, indicator_var_t& indvar)
  {
    string line;
    ASSERT(!!getline(in, line));
    ASSERT(is_line(line, "=indicator_var"));
    line = read_expr(in, indvar.m_var, ctx);
    while (is_line(line, "=possible.val")) {
      expr_ref possible_val;
      line = read_expr(in, possible_val, ctx);
      indvar.m_possible_values.insert(make_pair(possible_val->get_id(), possible_val));
    }
    return line;
  }
};

}
