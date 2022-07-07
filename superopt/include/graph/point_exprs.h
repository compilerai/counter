#pragma once

#include "support/types.h"
#include "support/bv_const.h"
#include "support/bv_const_expr.h"
#include "support/dyn_debug.h"
#include "support/dshared_ptr.h"

#include "expr/expr.h"
#include "expr/context.h"

#include "graph/addr_that_may_differ.h"

namespace eqspace {

using namespace std;

class point_exprs_t {
private:
  expr_ref m_expr;
  dshared_ptr<addr_that_may_differ_t const> m_addr_that_may_differ;
public:
  point_exprs_t(expr_ref const& e) : m_expr(e)
  { }
  point_exprs_t(expr_ref const& e, addr_that_may_differ_t const& addr_that_may_differ) : m_expr(e), m_addr_that_may_differ(make_dshared<addr_that_may_differ_t const>(addr_that_may_differ))
  { }

  expr_ref get_expr() const
  {
    return m_expr;
  }
  addr_that_may_differ_t const& get_addr_that_may_differ() const { ASSERT(m_addr_that_may_differ); return *m_addr_that_may_differ; }
  bool operator==(point_exprs_t const& other) const
  {
    if (m_expr != other.m_expr)
      return false;
    if (!m_addr_that_may_differ && !other.m_addr_that_may_differ)
      return true;
    if (m_addr_that_may_differ && !other.m_addr_that_may_differ || !m_addr_that_may_differ && other.m_addr_that_may_differ)
      return false;
    if (m_addr_that_may_differ == other.m_addr_that_may_differ)
      return true;
    else
      return false;
  }
  void point_exprs_to_stream(ostream& os) const
  {
    expr_ref e = this->get_expr();
    context* ctx = e->get_context();
    ctx->expr_to_stream(os, e);
    os << "\n";
    if (m_addr_that_may_differ) {
      os << "=addr_that_may_differ" << endl;
      m_addr_that_may_differ->addr_that_may_differ_to_stream(os);
    }
  }
  string point_exprs_to_string() const
  {
    expr_ref e = this->get_expr();
    context* ctx = e->get_context();
    string ret = expr_string(e);
    if (m_addr_that_may_differ) {
      ret += " [+ addr_that_may_differ]\n";
    }
    return ret;
  }


  point_exprs_t(istream& is, context* ctx, string &line)
  {
    bool end;
    line = read_expr(is, m_expr, ctx);
    string prefix = "=addr_that_may_differ";
    if (string_has_prefix(line, prefix)) {
      addr_that_may_differ_t addr(is, ctx);
      m_addr_that_may_differ = make_dshared<addr_that_may_differ_t const>(addr);
      end = !getline(is, line);
      ASSERT(!end);
    }
  }
};

}
