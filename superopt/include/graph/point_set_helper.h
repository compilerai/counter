#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/bv_const.h"

#include "expr/expr.h"

#include "graph/point_exprs.h"

namespace eqspace {

void canonicalize_coeffs(vector<bv_const>& rhs_coeffs, vector<bv_const>& lhs_coeffs, bv_const const& mod);
void construct_linear_combination_exprs(std::map<bv_solve_var_idx_t, point_exprs_t> const& vars, map<bv_solve_var_idx_t, expr_ref> const& coeffs, expr_ref& ret);
//unsigned coeff_row_arity(vector<bv_const> const& coeff_row);

}
