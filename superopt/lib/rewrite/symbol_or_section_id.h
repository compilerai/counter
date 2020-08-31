#pragma once
#include "expr/context.h"
#include "expr/expr.h"
#include "rewrite/translation_instance.h"
#include "support/types.h"

namespace eqspace {

class symbol_or_section_id_t {
public:
  using type_t = enum { symbol, section };
private:
  type_t m_type;
  int m_idx;
public:
  symbol_or_section_id_t(type_t t, int idx) : m_type(t), m_idx(idx)
  { }
  static map<expr_id_t, pair<expr_ref, expr_ref>> get_expr_rename_submap(map<symbol_or_section_id_t, symbol_id_t> const& m, input_exec_t const* in, context* ctx);
  bool operator==(symbol_or_section_id_t const& other) const
  {
    return m_type == other.m_type && m_idx == other.m_idx;
  }
  bool operator<(symbol_or_section_id_t const& other) const
  {
    return    m_type < other.m_type
          || (m_type == other.m_type && m_idx < other.m_idx)
    ;
  }
  bool is_symbol() const { return m_type == symbol; }
  bool is_section() const { return m_type == section; }
  int get_index() const { return m_idx; }
  string to_string() const;
};

}
