#pragma once

namespace eqspace {

class proof_timeout_info_t
{
private:
  bool m_timeout_occurred;
  predicate_set_t m_preds_that_timed_out;
public:
  proof_timeout_info_t(context *ctx) : m_timeout_occurred(false), m_preds_that_timed_out()
  { }
  void clear() { m_timeout_occurred = false; m_preds_that_timed_out.clear(); }
  void register_timeout(predicate_ref const& pred)
  {
    m_timeout_occurred = true;
    m_preds_that_timed_out.insert(pred);
  }
  bool timeout_occurred() const { return m_timeout_occurred; }
  predicate_set_t const& get_preds_that_timed_out() const { return m_preds_that_timed_out; }
};

}
