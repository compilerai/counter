#include "parser/parse_tree_yices.h"
#include "expr/context.h"

using namespace std;

expr_ref
fn_expr_pair_list_to_ite(vector<pair<expr_ref,expr_ref>> const& exprlist)
{
  ASSERT(exprlist.size());
  ASSERT(exprlist.back().first->is_const_bool_true());
  expr_ref ret = exprlist.back().second;
  context* ctx = ret->get_context();

  auto ri = exprlist.crbegin();
  for (++ri; ri != exprlist.crend(); ++ri) {
    expr_ref const& cond = ri->first;
    expr_ref const& then = ri->second;
    ret = ctx->mk_ite(cond, then, ret);
  }
  return ret;
}
