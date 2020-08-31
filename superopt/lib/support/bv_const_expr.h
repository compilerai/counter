#pragma once

#include <vector>
#include <expr/defs.h>
#include "support/bv_const.h"
#include "support/bv_const_ref.h"

namespace eqspace {

bv_const expr2bv_const(expr_ref e);
expr_ref bv_const2expr(bv_const const& a, context* ctx, unsigned sz);

vector<bv_const> expr_vector2bv_const_vector(vector<expr_ref> const& exprs);
vector<vector<bv_const>> vec_expr_vec2vec_bv_const_vec(vector<vector<expr_ref>> const& exprs);
vector<expr_ref> bv_const_vector2expr_vector(vector<bv_const> const& bv_consts, context* ctx, int sz);
vector<expr_ref> bv_const_ref_vector2expr_vector(vector<bv_const_ref> const& bv_consts, context* ctx, int sz);

}
