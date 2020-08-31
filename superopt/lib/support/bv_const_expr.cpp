#include "support/bv_const_expr.h"
#include "expr/context.h"
#include "support/debug.h"
#include "support/mybitset.h"

namespace eqspace {

bv_const expr2bv_const(expr_ref e)
{
  if (!e->is_const()) {
    cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
  }
  ASSERT(e->is_const());
  return e->get_mybitset_value().get_unsigned_mpz();
}

expr_ref bv_const2expr(bv_const const& a, context* ctx, unsigned sz)
{
  return ctx->mk_bv_const(sz, mybitset(a, sz));
}

vector<expr_ref> bv_const_ref_vector2expr_vector(vector<bv_const_ref> const& bv_consts, context* ctx, int sz)
{
  vector<expr_ref> ret;
  ret.reserve(bv_consts.size());
  for (const auto& c : bv_consts)
    ret.push_back(bv_const2expr(c->get_bv(), ctx, sz));
  return ret;
}

vector<expr_ref> bv_const_vector2expr_vector(vector<bv_const> const& bv_consts, context* ctx, int sz)
{
  vector<expr_ref> ret;
  ret.reserve(bv_consts.size());
  for (const auto& c : bv_consts)
    ret.push_back(bv_const2expr(c, ctx, sz));
  return ret;
}

vector<bv_const> expr_vector2bv_const_vector(vector<expr_ref> const& exprs)
{
  vector<bv_const> ret;
  ret.reserve(exprs.size());
  for (const auto& e : exprs) {
    ret.push_back(expr2bv_const(e));
  }
  return ret;
}

vector<vector<bv_const>> vec_expr_vec2vec_bv_const_vec(vector<vector<expr_ref>> const& exprs)
{
  vector<vector<bv_const>> ret;
  ret.reserve(exprs.size());
  for (const auto& e : exprs) {
    ret.push_back(expr_vector2bv_const_vector(e));
  }
  return ret;
}

}
