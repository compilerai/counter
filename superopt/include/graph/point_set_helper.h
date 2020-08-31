#pragma once
#include <map>
#include <string>

#include "expr/expr.h"
#include "support/types.h"
#include "support/bv_const.h"

namespace eqspace {

void canonicalize_coeffs(vector<bv_const>& rhs_coeffs, vector<bv_const>& lhs_coeffs, bv_const const& mod);
void construct_linear_combination_exprs(vector<expr_ref> const& vars, vector<expr_ref> const& coeffs, expr_ref& ret);
unsigned coeff_row_arity(vector<bv_const> const& coeff_row);

}
