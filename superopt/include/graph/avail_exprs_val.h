#pragma once
#include <map>
#include <string>

#include "support/types.h"

#include "expr/expr.h"

using namespace std;

namespace eqspace {

class avail_exprs_val_t {
private:
  expr_ref m_expr;
  bool m_is_top;
public:
  avail_exprs_val_t(expr_ref const& e) : m_expr(e), m_is_top(false)
  { }
  avail_exprs_val_t(istream& is, context* ctx);
  static avail_exprs_val_t avail_exprs_val_top()
  {
    avail_exprs_val_t ret(nullptr);
    ret.m_is_top = true;
    return ret;
  }
  void avail_exprs_val_to_stream(ostream& os) const;
  expr_ref avail_exprs_val_get_expr() const { ASSERT(!m_is_top); ASSERT(m_expr); return m_expr; }
  bool avail_exprs_val_is_top() const { return m_is_top; }
  bool operator==(avail_exprs_val_t const& other) const
  {
    return    true
           && m_expr == other.m_expr
           && m_is_top == other.m_is_top
    ;
  }
  string avail_exprs_val_to_string() const
  {
    if (m_is_top) return "avail-exprs-val-top";
    return expr_string(m_expr);
  }
  bool operator!=(avail_exprs_val_t const& other) const
  {
    return !(*this == other);
  }
};

}
