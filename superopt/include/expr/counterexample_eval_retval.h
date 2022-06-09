#pragma once

#include "expr/expr.h"

class counterexample_eval_retval_t {
private:
  bool m_eval_success; //we did not encounter an evaluation failure
  expr_ref m_result;
public:
  bool is_eval_success() const { return m_eval_success; }
  expr_ref const& get_result() const { ASSERT(m_eval_success); ASSERT(m_result->is_const()); return m_result; }

  static counterexample_eval_retval_t eval_failure() { return counterexample_eval_retval_t(false); }
  static counterexample_eval_retval_t eval_success(expr_ref const& e) { return counterexample_eval_retval_t(true, e); }
private:
  counterexample_eval_retval_t(bool eval_success) : m_eval_success(eval_success), m_result(nullptr)
  { ASSERT(!eval_success); }
  counterexample_eval_retval_t(bool eval_success, expr_ref const& res) : m_eval_success(eval_success), m_result(res)
  { }
};
