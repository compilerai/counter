#pragma once

class counterexample_translate_retval_t {
private:
  bool m_eval_success; //we did not encounter an evaluation failure
  bool m_edgecond_satisfied; //we traversed the edge (we satisfied the edge condition)
public:
  bool is_eval_success() const { return m_eval_success; }
  bool is_edgecond_satisfied() const { /*ASSERT(m_eval_success);*/ return m_edgecond_satisfied; }
  bool is_true() const { return m_eval_success && m_edgecond_satisfied; }

  static counterexample_translate_retval_t eval_failure() { return counterexample_translate_retval_t(false); }
  static counterexample_translate_retval_t eval_success(bool edgecond_satisfied) { return counterexample_translate_retval_t(true, edgecond_satisfied); }
  static counterexample_translate_retval_t eval_result(bool eval_result, bool edgecond_satisfied) { return counterexample_translate_retval_t(eval_result, edgecond_satisfied); }
private:
  counterexample_translate_retval_t(bool eval_success) : m_eval_success(eval_success), m_edgecond_satisfied(false)
  { ASSERT(!eval_success); }
  counterexample_translate_retval_t(bool eval_success, bool edgecond_satisfied) : m_eval_success(eval_success), m_edgecond_satisfied(edgecond_satisfied)
  { }
};
