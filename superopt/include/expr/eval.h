#pragma once

#include "expr/expr.h"
#include "expr/context.h"

namespace eqspace {

class expr_evaluates_to_constant_visitor : public expr_visitor
{
public:
  expr_evaluates_to_constant_visitor(expr_ref e, bool enable_cache = true)
    : m_ctx(e->get_context()),
      m_cs(e->get_context()->get_consts_struct()),
      m_in(e),
      m_cache_enabled(enable_cache && !m_ctx->disable_caches())
  { }

  expr_ref evaluate();

  virtual bool evaluate_var(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_select(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_store(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_memmask(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_memmasks_are_equal(expr_ref const& e, expr_ref& const_val) const;
  //virtual bool evaluate_memsplice(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_function_call(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_ismemlabel(expr_ref const& e, expr_ref& const_val) const;

protected:
  virtual bool evaluate_const(expr_ref const& e, expr_ref& const_val) const;
  virtual bool evaluate_eq(expr_ref const& e, expr_ref& const_val) const;

  context *m_ctx;
  consts_struct_t const &m_cs;
  map<unsigned, pair<bool, expr_ref>> m_map;

private:
  void previsit(expr_ref const &e, int interesting_args[], int &num_interesting_args);
  void visit(expr_ref const &e);

  expr_ref m_in;
  bool m_cache_enabled;
};

expr_ref expr_evaluates_to_constant(expr_ref const &e/*, expr_ref &const_val*/);

}
