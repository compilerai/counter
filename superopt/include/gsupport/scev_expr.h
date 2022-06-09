#pragma once

#include "support/mybitset.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/free_id_list.h"
#include "support/utils.h"

#include "expr/context.h"
#include "expr/expr.h"
#include "expr/pc.h"

#include "gsupport/scev_toplevel.h"

class scev_expr_t {
  expr_ref m_expr;
  expr_ref m_var;
  scev_toplevel_t<pc> m_var_scev;
public:
  scev_expr_t(expr_ref const& e, expr_ref const& v, scev_toplevel_t<pc> const& var_scev) : m_expr(e), m_var(v), m_var_scev(var_scev)
  { }
  expr_ref const& get_expr() const { return m_expr; }
  expr_ref const& get_var() const { return m_var; }
  scev_toplevel_t<pc> const& get_var_scev() const { return m_var_scev; }
};
