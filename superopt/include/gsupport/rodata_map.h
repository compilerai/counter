#pragma once
#include "expr/expr.h"
#include "expr/context.h"
#include "support/types.h"

namespace eqspace {

class rodata_offset_t {
private:
  symbol_id_t m_symbol_id;
  src_ulong m_offset;
public:
  rodata_offset_t(symbol_id_t const& symbol_id, src_ulong offset) : m_symbol_id(symbol_id), m_offset(offset)
  { }
  symbol_id_t get_symbol_id() const { return m_symbol_id; }
  src_ulong get_offset() const { return m_offset; }
  string rodata_offset_to_string() const
  {
    stringstream ss;
    ss << m_symbol_id << ":" << m_offset;
    return ss.str();
  }
  bool operator==(rodata_offset_t const& other) const
  {
    return    m_symbol_id == other.m_symbol_id
           && m_offset == other.m_offset
    ;
  }
  bool operator<(rodata_offset_t const& other) const
  {
    return    m_symbol_id < other.m_symbol_id
           || (   m_symbol_id == other.m_symbol_id
               && m_offset < other.m_offset)
    ;
  }
  expr_ref get_expr(context* ctx) const;
};

class rodata_map_t {
private:
  map<symbol_id_t, set<rodata_offset_t>> m_map;
public:
  rodata_map_t() {}
  rodata_map_t(istream& is);
  //map<expr_id_t, pair<expr_ref, expr_ref>> rodata_map_get_submap(context *ctx, tfg const& src_tfg) const;
  bool operator==(rodata_map_t const &other) const
  {
    return m_map == other.m_map;
  }
  map<symbol_id_t, set<rodata_offset_t>> const& get_map() const { return m_map; }
  string to_string_for_eq() const;
  void rodata_map_to_stream(ostream& os) const;
  bool prune_using_expr_pair(expr_ref const &src, expr_ref const &dst);
  void prune_non_const_symbols(graph_symbol_map_t const& symbol_map);
  void rodata_map_add_mapping(symbol_id_t symbol_id, set<rodata_offset_t> const &symaddrs);
  void rodata_map_add_mapping(symbol_id_t symbol_id, rodata_offset_t const& symaddr);
  expr_ref get_assertions(context* ctx) const;
private:
};

}
