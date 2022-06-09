#pragma once

namespace eqspace {

class expr_eval_status_t {
public:
  enum val_t { constant, notconstant, timeout };
private:
  val_t m_val;
public:
  expr_eval_status_t() : expr_eval_status_t(notconstant)
  { }
  expr_eval_status_t(val_t val) : m_val(val)
  { }
  bool operator==(expr_eval_status_t const& other) const
  {
    return m_val == other.m_val;
  }
  bool operator!=(expr_eval_status_t const& other) const
  {
    return !(*this == other);
  }
};

extern expr_eval_status_t expr_eval_yields_constant;
extern expr_eval_status_t expr_eval_yields_notconstant;
extern expr_eval_status_t expr_eval_status_timeout;

}
