#pragma once

#include "support/string_ref.h"

#include "expr/expr.h"

#include "gsupport/src_dst_cg_path_tuple.h"

namespace eqspace {

class failcond_t {
private:
  expr_ref m_expr;
  src_dst_cg_path_tuple_t m_src_dst_cg_path_tuple;
  string_ref m_description;
public:
  failcond_t() = delete;
  //failcond_t(istream& is, context* ctx);
  static failcond_t failcond_create(expr_ref const& e, src_dst_cg_path_tuple_t const& src_dst_cg_path_tuple, string const& description)
  { return failcond_t(e, src_dst_cg_path_tuple, description); }
  expr_ref const& get_expr() const { ASSERT(m_expr); return m_expr; }
  src_dst_cg_path_tuple_t const& get_src_dst_cg_path_tuple() { return m_src_dst_cg_path_tuple; }
  string_ref const& get_description() const { ASSERT(m_description); return m_description; }
  bool failcond_is_null() const { return m_expr == nullptr; }
  void failcond_to_stream(ostream& os, string const& prefix) const;
  string failcond_to_string() const;

  static failcond_t failcond_null() { return failcond_t(nullptr, src_dst_cg_path_tuple_t(), ""); }
private:
  failcond_t(expr_ref const& e, src_dst_cg_path_tuple_t const& src_dst_cg_path_tuple, string const& description) : m_expr(e), m_src_dst_cg_path_tuple(src_dst_cg_path_tuple), m_description(mk_string_ref(description))
  { }
};

}
