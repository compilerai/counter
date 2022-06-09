#pragma once

#include "expr/allocsite.h"
#include "expr/expr.h"

namespace eqspace {

enum class alloc_dealloc_pc_t { START, END };

ostream& operator<<(ostream& os, alloc_dealloc_pc_t const& adp);

class alloc_dealloc_t
{
private:
  allocsite_t m_allocsite;
  bool m_is_dealloc;
  expr_ref m_expr;
public:
  alloc_dealloc_t(allocsite_t const& allocsite, bool is_dealloc, expr_ref const& expr)
    : m_allocsite(allocsite), m_is_dealloc(is_dealloc), m_expr(expr)
  { }
  allocsite_t const& get_allocsite() const { return m_allocsite; }
  bool is_alloc() const { return !m_is_dealloc; }
  bool is_dealloc() const { return m_is_dealloc; }
  expr_ref get_expr() const { return m_expr; }
  bool operator==(alloc_dealloc_t const& other) const
  {
    return    m_allocsite == other.m_allocsite
           && m_is_dealloc == other.m_is_dealloc
    ;
  }
  bool operator<(alloc_dealloc_t const& other) const
  {
    return make_pair(m_allocsite, m_is_dealloc) < make_pair(other.m_allocsite, other.m_is_dealloc);
  }
  string alloc_dealloc_to_string() const;
  static bool ls_contains_dealloc(list<alloc_dealloc_t> const& ls);
};

ostream& operator<<(ostream& os, alloc_dealloc_t const& ad);

}
