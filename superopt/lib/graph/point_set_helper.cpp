#include "expr/context.h"
#include "graph/point_set_helper.h"
#include "support/bv_const_expr.h"

namespace eqspace {

void canonicalize_coeffs(vector<bv_const>& rhs_coeffs, vector<bv_const>& lhs_coeffs, bv_const const& mod)
{
  // move lhs coefficient to other side of '='
  for (auto& lhs_coeff : lhs_coeffs) {
    lhs_coeff = bv_mod(mod - lhs_coeff, mod);
  }

  // if rhs coeff is odd (has multiplicative inverse) then simplify rhs by multiplifying both sides by it
  bv_const mul_inv = 1_mpz;
  for (auto const& rhs_coeff : rhs_coeffs) {
    if (bv_isodd(rhs_coeff)) {
      mul_inv = bv_mulinv(rhs_coeff, mod);
      break;
    }
  }
  if (mul_inv != 1_mpz) {
    for (auto& rhs_coeff : rhs_coeffs) {
      rhs_coeff = bv_mod(rhs_coeff*mul_inv, mod);
    }
    for (auto& lhs_coeff : lhs_coeffs) {
      lhs_coeff = bv_mod(lhs_coeff*mul_inv, mod);
    }
  }
}

expr_ref
expr_mk_compatible_with_sort(expr_ref const& e, sort_ref const& s)
{
  if (s->is_bv_kind()) {
    ASSERT(e->is_bv_sort() || e->is_bool_sort());

    context* ctx = e->get_context();
    unsigned bvlen = s->get_size();
    ASSERT(bvlen >= 1);

    // use ite if bool, zero extend if bv
    if (e->is_bool_sort()) {
      //return ctx->mk_ite(e, ctx->mk_onebv(bvlen), ctx->mk_zerobv(bvlen));
      expr_ref ret = expr_bool_to_bv1(e);
      if (bvlen > 1) {
        return ctx->mk_bvzero_ext(ret, bvlen - 1);
      } else {
        return ret;
      }
    } else {
      unsigned e_bvlen = e->get_sort()->get_size();
      if (e_bvlen < bvlen) {
        return ctx->mk_bvzero_ext(e, bvlen - e_bvlen);
      } else if (e_bvlen == bvlen) {
        return e;
      } else if (e_bvlen > bvlen) {
        return ctx->mk_bvextract(e, bvlen - 1, 0);
      } else NOT_REACHED();
    }
  }
  else NOT_IMPLEMENTED();
}

void
construct_linear_combination_exprs(vector<expr_ref> const& vars, vector<expr_ref> const& coeffs, expr_ref& ret)
{
  ASSERT(vars.size() == coeffs.size());

  if (vars.empty())
    return;

  context* ctx = ret->get_context();
  unsigned bvlen = ret->is_bool_sort() ? 1 : ret->get_sort()->get_size();
  expr_ref expr_zero = ctx->mk_zerobv(bvlen);

  auto var_itr = vars.cbegin();
  auto coeffs_itr = coeffs.cbegin();
  for (; var_itr != vars.cend() && coeffs_itr != coeffs.cend(); ++var_itr, ++coeffs_itr) {
    if (*coeffs_itr != expr_zero) {
      expr_ref var_expr = *var_itr;
      var_expr = expr_mk_compatible_with_sort(var_expr, ctx->mk_bv_sort(bvlen));
      ret = expr_bvadd(ret, expr_bvmul(var_expr, *coeffs_itr)); // r = r + v_i*c_i
    }
  }
}

unsigned
coeff_row_arity(vector<bv_const> const& coeff_row)
{
  unsigned ret = 0;
  for (size_t i = 0; i < coeff_row.size()-1; ++i) { // last coeff is constant term
    if (coeff_row[i]) {
      ret++;
    }
  }
  return ret;
}

}
