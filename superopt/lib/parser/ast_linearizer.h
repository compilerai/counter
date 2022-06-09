#pragma once

#include <variant>
#include "expr/context.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/range.h"
#include "parser/parsing_common.h"

namespace eqspace
{

class ExprLinearLazy
{
public:
  ExprLinearLazy(vector<range_t> const& ranges, expr_ref const& val)
  {
    // assert ranges are overlapping?
    ASSERT(ranges.size());
    for (auto const& r : ranges) {
      ASSERT(!r.range_is_empty());
      m_vals.emplace(r, val);
    }
  }

  bool is_empty() const { return m_vals.empty(); }

  ExprLinearLazy union_with(ExprLinearLazy const& other) const;
  ExprLinearLazy masked(vector<range_t> const& mask_ranges) const;

  expr_ref eval_to_eager() const;

  sort_ref get_sort() const
  {
    context* ctx = this->get_context();
    ASSERT(this->m_vals.size());
    auto itr = this->m_vals.cbegin();
    return ctx->mk_array_sort({ ctx->mk_bv_sort(itr->first.bvsize()) }, itr->second->get_sort());
  }

  string to_string() const;

private:
  context* get_context() const
  {
    ASSERT(this->m_vals.size());
    return this->m_vals.cbegin()->second->get_context();
  }

  ExprLinearLazy(map<range_t,expr_ref> const& vals) : m_vals(vals) { }

  mutable expr_ref m_cached_eager_expr;
  map<range_t,expr_ref> m_vals;
};


class ExprDualLazy
{
public:
  static ExprDualLazy dual(expr_ref const& e, expr_ref const& linear) { ASSERT(e); return ExprDualLazy(e, linear); }
  static ExprDualLazy normal(expr_ref const& e)                       { ASSERT(e); return ExprDualLazy(e, nullptr); }
  static ExprDualLazy dual_if_const(expr_ref const& e)
  {
    ASSERT(e);
    if (e->is_const())
      return dual(e, e);
    else return normal(e);
  }
  static ExprDualLazy dual_lazy(expr_ref const& e, ExprLinearLazy const& linear_lazy) { ASSERT(e); return ExprDualLazy(e, linear_lazy); }

  // for implicit conversion from ExprDual
  ExprDualLazy(ExprDual const& ed)
  {
    if (ed.has_linear_form())
      *this = dual(ed.normal_form(), ed.linear_form());
    else
      *this = normal(ed.normal_form());
  }

  ExprDual get_ExprDual() const
  {
    if (has_linear_form())
      return ExprDual::dual(normal_form(), linear_form());
    else
      return ExprDual::normal(normal_form());
  }

  bool has_linear_form() const
  {
    return has_linear_eager_form() || has_linear_lazy_form();
  }

  expr_ref normal_form() const { ASSERT(m_normal); return m_normal; }
  expr_ref linear_form() const
  {
    if (has_linear_eager_form())
      return get_linear_eager();
    if (has_linear_lazy_form())
      return get_linear_lazy().eval_to_eager();
    NOT_IMPLEMENTED();
  }
  ExprLinearLazy linear_lazy_form() const
  {
    ASSERT(has_linear_lazy_form());
    return get_linear_lazy();
  }

  sort_ref linear_form_sort() const
  {
    if (has_linear_eager_form())
      return get_linear_eager()->get_sort();
    if (has_linear_lazy_form())
      return get_linear_lazy().get_sort();
    NOT_IMPLEMENTED();
  }

  bool linear_form_is_lazy() const { return has_linear_lazy_form(); }

  string to_string() const;

private:
  ExprDualLazy(expr_ref const& normal, expr_ref const& linear)
    : m_normal(normal),
      m_linear(linear)
  { }
  ExprDualLazy(expr_ref const& normal, ExprLinearLazy const& linear_lazy)
    : m_normal(normal),
      m_linear(linear_lazy)
  { }

  bool has_linear_eager_form() const { return holds_alternative<expr_ref>(m_linear) && get<expr_ref>(m_linear) != nullptr; }
  bool has_linear_lazy_form() const  { return holds_alternative<ExprLinearLazy>(m_linear); }

  // no checking -- may throw
  expr_ref get_linear_eager() const      { return get<expr_ref>(m_linear); }
  ExprLinearLazy get_linear_lazy() const { return get<ExprLinearLazy>(m_linear); }

  expr_ref m_normal;
  variant<expr_ref,ExprLinearLazy> m_linear;
};

class AstLinearizer
{
public:
  AstLinearizer(AstNodeRef const& ast, context* ctx, ParsingEnv const& env) : m_ast(ast), m_ctx(ctx), m_env(env) { }

  ExprDual get_result()
  {
    return linearize(m_ast).get_ExprDual();
  }

private:

  ExprDualLazy linearize(AstNodeRef const& ast);
  optional<ExprDualLazy> linearize_with_refined_ranges(vector<range_t> const& refined_ranges, AstNodeRef const& ast);

  ExprDualLazy process_op(expr::operation_kind const& opk, vector<ExprDualLazy> const& args);

  ExprDualLazy handle_ite(AstNodeRef const& condA, AstNodeRef const& thenA, AstNodeRef const& elseA);
  pair<ExprDualLazy,optional<vector<range_t>>> linearize_ite_cond_and_recover_valid_ranges_for_var(AstNodeRef const& cond, vector<range_t> const& var_val_ranges, expr_ref const& var);
  pair<ExprDualLazy,optional<vector<range_t>>> linearize_boolean_ast_and_recover_valid_ranges_for_var(AstNodeRef const& cond, vector<range_t> const& input_val_ranges, expr_ref const& var);

  ExprDualLazy handle_apply(AstNodeRef const& ast);
  ExprDualLazy handle_lambda(AstNodeRef const& ast);
  ExprDualLazy handle_forall(AstNodeRef const& ast);

  ExprDualLazy lookup_name(string_ref const& name) const;

  void push_letdecl(string_ref const& name, ExprDualLazy const& e) { m_letdecls.push_back(make_pair(name, e)); }
  void pop_letdecl()                                           { ASSERT(m_letdecls.size()); m_letdecls.pop_back(); }
  optional<ExprDualLazy> lookup_letdecl(string_ref const& name) const
  {
    /* search in reverse order -- most recent to least */
    for (auto ritr = m_letdecls.crbegin(); ritr != m_letdecls.crend(); ++ritr) {
      if (ritr->first == name)
        return ritr->second;
    }
    return nullopt;
  }

  bool is_unit_sized_bv_sorted(vector<expr_ref> const& ev) const { return ev.size() == 1 && ev.front()->is_bv_sort(); }

  void push_lambda_params(vector<expr_ref> const& params)
  {
    if (is_unit_sized_bv_sorted(params)) {
      auto param_bvsize = params.front()->get_sort()->get_size();
      auto dom_low  = mybitset(0, param_bvsize);
      auto dom_high = mybitset(-1, param_bvsize);
      m_lambda_param_ranges_stk.push({ range_t::range_from_mybitset(dom_low, dom_high) });
    }
    m_lambda_params_stk.push(params);
  }
  void pop_lambda_params()
  {
    ASSERT(m_lambda_params_stk.size());
    if (is_unit_sized_bv_sorted(m_lambda_params_stk.top())) {
      m_lambda_param_ranges_stk.pop();
    }
    m_lambda_params_stk.pop();
  }
  expr_ref lookup_lambda_param(string_ref const& name) const
  {
    if (m_lambda_params_stk.empty())
      return nullptr;
    for (auto const& param : m_lambda_params_stk.top()) {
      if (param->get_name() == name)
        return param;
    }
    return nullptr;
  }
  optional<expr_ref> enclosing_lambda_is_unit_arity_bv_sorted() const
  {
    if (m_lambda_params_stk.empty())
      return nullopt;
    if (is_unit_sized_bv_sorted(m_lambda_params_stk.top()))
      return m_lambda_params_stk.top().front();
    return nullopt;
  }
  optional<vector<expr_ref>> enclosing_lambda_has_arity_more_than_one() const
  {
    if (m_lambda_params_stk.empty())
      return nullopt;
    if (m_lambda_params_stk.top().size() > 1)
      return m_lambda_params_stk.top();
    return nullopt;
  }
  vector<sort_ref> get_array_constant_domain_sort_for_enclosing_lambda() const
  {
    if (m_lambda_params_stk.empty())
      return {};
    return expr_vector_to_sort_vector(m_lambda_params_stk.top());
  }
  vector<range_t> get_enclosing_lambda_param_ranges() const
  {
    ASSERT(m_lambda_param_ranges_stk.size());
    return m_lambda_param_ranges_stk.top();
  }

  void set_refined_lambda_param_ranges(vector<range_t> const& param_ranges)
  {
    //CPP_DBG_EXEC(ASSERTCHECKS, ASSERT(ranges_are_refined_than(param_ranges, this->get_enclosing_lambda_param_ranges())));
    ASSERT(m_lambda_param_ranges_stk.size());
    m_lambda_param_ranges_stk.pop();
    m_lambda_param_ranges_stk.push(param_ranges);
  }
  void restore_lambda_param_ranges(vector<range_t> const& param_ranges)
  {
    //CPP_DBG_EXEC(ASSERTCHECKS, ASSERT(ranges_are_refined_than(this->get_enclosing_lambda_param_ranges(), param_ranges)));
    ASSERT(m_lambda_param_ranges_stk.size());
    m_lambda_param_ranges_stk.pop();
    m_lambda_param_ranges_stk.push(param_ranges);
  }

  optional<ExprDualLazy> lookup_func(string_ref const& name) const
  {
    if (!m_env.is_func(name))
      return nullopt;
    return m_env.linearize_func_to_expr(name);
  }

  string fresh_varname() const;

  vector<pair<string_ref,ExprDualLazy>> m_letdecls;       /* stack of "let" declarations; we push/pop these as we linearize */
  stack<vector<expr_ref>> m_lambda_params_stk;        /* stack of params of lambda functions */
  stack<vector<range_t>> m_lambda_param_ranges_stk; /* ONLY FOR 1-arity BV-sorted lambdas: stack of param ranges */

  AstNodeRef const& m_ast;
  context* m_ctx;
  ParsingEnv const& m_env;
};

}
