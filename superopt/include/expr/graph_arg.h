#pragma once

#include <map>
#include <string>

#include "support/types.h"

#include "expr/context.h"
#include "expr/expr.h"

namespace eqspace {

using namespace std;

class graph_arg_t {
private:
  expr_ref m_addr;
  expr_ref m_val;

public:
  graph_arg_t(expr_ref const& addr, expr_ref const& val) : m_addr(addr), m_val(val)
  { }

  graph_arg_t(istream& is, context* ctx, string const& prefix);

  void graph_arg_to_stream(ostream& os, string const& prefix) const;

  expr_ref const& get_addr() const { return m_addr; }
  expr_ref const& get_val() const { return m_val; }
  bool is_vararg() const { return string_contains(m_val->get_name()->get_str(), VARARG_LOCAL_VARNAME); }
};

}
