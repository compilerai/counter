#pragma once

namespace eqspace {

enum dfa_dir_t { forward, backward };
enum dfa_state_type_t { global, per_pc };

class dfa_xfer_retval_t {
public:
  enum val_t { unchanged, changed/*, restart*/ };
private:
  val_t m_val;
public:
  dfa_xfer_retval_t(val_t val) : m_val(val)
  { }
  bool operator==(dfa_xfer_retval_t const& other) const
  {
    return m_val == other.m_val;
  }
  bool operator!=(dfa_xfer_retval_t const& other) const
  {
    return !(*this == other);
  }
  string to_string() const {
    return    m_val == unchanged ? "unchanged"
            : m_val == changed   ? "changed"
            : "restart"
    ;
  }
};

extern dfa_xfer_retval_t DFA_XFER_RETVAL_UNCHANGED;
extern dfa_xfer_retval_t DFA_XFER_RETVAL_CHANGED;
//extern dfa_xfer_retval_t DFA_XFER_RETVAL_RESTART;

}
