#pragma once
#include "support/debug.h"

class expr_dummy_class
{
public:
  map<expr_id_t, pair<expr_ref, expr_ref>> get_local_sprel_expr_submap() const { return {}; }
  expr_ref get_lhs_expr() const { NOT_REACHED(); }
  expr_ref get_rhs_expr() const { NOT_REACHED(); }
  bool precond_implies(expr_dummy_class const& other) const { NOT_REACHED(); }
  expr_dummy_class const& get_precond() const { NOT_REACHED(); }
};
