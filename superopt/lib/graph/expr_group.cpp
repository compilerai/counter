#include "graph/expr_group.h"

namespace eqspace {
manager<expr_group_t> *
expr_group_t::get_expr_group_manager()
{
  static manager<expr_group_t> *ret = NULL;
  if (!ret) {
    ret = new manager<expr_group_t>;
  }
  return ret;
}

expr_group_ref
mk_expr_group(expr_group_t::expr_group_type_t t, vector<expr_ref> const &exprs)
{
  expr_group_t g(t, exprs);
  return expr_group_t::get_expr_group_manager()->register_object(g);
}

expr_group_ref mk_expr_group(istream& is, context* ctx, string const& prefix)
{
  expr_group_t g(is, ctx, prefix);
  return expr_group_t::get_expr_group_manager()->register_object(g);
}

expr_group_t::~expr_group_t()
{
  if (m_is_managed) {
    this->get_expr_group_manager()->deregister_object(*this);
  }
}

}
