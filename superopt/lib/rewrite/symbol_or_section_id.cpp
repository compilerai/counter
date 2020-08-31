#include "rewrite/symbol_or_section_id.h"
#include "support/debug.h"
#include "expr/context.h"
#include "rewrite/translation_instance.h"

namespace eqspace {

map<expr_id_t, pair<expr_ref, expr_ref>>
symbol_or_section_id_t::get_expr_rename_submap(map<symbol_or_section_id_t, symbol_id_t> const& m, input_exec_t const* in, context* ctx)
{
  consts_struct_t const& cs = ctx->get_consts_struct();
  map<expr_id_t, pair<expr_ref, expr_ref>> ret;
  for (auto const& e : m) {
    if (e.first.is_symbol()) {
      expr_ref from = cs.get_expr_value(reg_type_symbol, e.first.get_index());
      expr_ref to = cs.get_expr_value(reg_type_dst_symbol_addr, e.second);
      ret.insert(make_pair(from->get_id(), make_pair(from, to)));
    } else if (e.first.is_section()) {
      expr_ref from = cs.get_expr_value(reg_type_section, e.first.get_index());
      expr_ref to = cs.get_expr_value(reg_type_dst_symbol_addr, e.second);
      ret.insert(make_pair(from->get_id(), make_pair(from, to)));
    } else NOT_REACHED();
  }
  return ret;
}

string
symbol_or_section_id_t::to_string() const
{
  ostringstream ss;
  if (this->is_symbol()) {
    ss << "symbol" << m_idx;
  } else {
    ss << "section" << m_idx;
  }
  return ss.str();
}

}
