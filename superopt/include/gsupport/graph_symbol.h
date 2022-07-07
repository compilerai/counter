#pragma once

#include "support/types.h"
#include "support/str_utils.h"
#include "expr/expr.h"

struct input_exec_t;

namespace eqspace {

using namespace std;

class rodata_map_t;

class graph_symbol_t
{
private:
  string_ref m_name;
  size_t m_size;
  align_t m_alignment;
  bool m_is_const;
public:
  graph_symbol_t(string_ref const& s, size_t size, align_t alignment, bool is_const) : m_name(s), m_size(size), m_alignment(alignment), m_is_const(is_const)
  { }
  string_ref const& get_name() const { return m_name; }
  size_t get_size() const { return m_size; }
  align_t get_alignment() const { return m_alignment; }
  bool graph_symbol_is_const() const { return m_is_const; }
  bool is_dst_const_symbol() const { return m_is_const && string_has_prefix(m_name->get_str(), G_DST_SYMBOL_PREFIX); }
  bool is_marker_fcall_symbol() const { return string_has_prefix(m_name->get_str(), QCC_MARKER_PREFIX); }

  void set_name(string_ref const&s) { m_name = s; }
};

class graph_symbol_map_t
{
private:
  std::map<symbol_id_t, graph_symbol_t> m_map;
public:
  graph_symbol_map_t() {}
  graph_symbol_map_t(istream& is);
  void graph_symbol_map_to_stream(ostream& os) const;
  void insert(pair<symbol_id_t, graph_symbol_t> const &e) { m_map.insert(e); }
  graph_symbol_t const& at(symbol_id_t id) const { return m_map.at(id); }
  size_t count(symbol_id_t id) const { return m_map.count(id); }
  void symbol_map_union(graph_symbol_map_t const& other) { ::map_union(m_map, other.m_map); }
  map<symbol_id_t, graph_symbol_t> const& get_map() const { return m_map; }
  size_t size() const { return m_map.size(); }
  void symbol_map_remove_function_name_from_symbols(string const& function_name)
  {
    string function_name_dot(function_name + ".");
    for (auto iter = m_map.begin();
        iter != m_map.end();
        iter++) {
      if (iter->second.get_name()->get_str().substr(0, function_name_dot.length()) == function_name_dot) {
        //get<0>(iter->second)->get_str() = get<0>(iter->second)->get_str().substr(function_name_dot.length());
        iter->second.set_name(mk_string_ref(iter->second.get_name()->get_str().substr(function_name_dot.length())));
      }
    }
  }
  void clear() { m_map.clear(); }
  void add_dst_symbols_referenced_in_rodata_map(rodata_map_t const& rodata_map, struct input_exec_t const* in);
  symbol_id_t find_max_symbol_id() const;
  bool symbol_is_rodata(symbol_id_t symbol_id) const;
  size_t symbol_map_get_size_for_symbol(symbol_id_t symbol_id) const;
  symbol_id_t symbol_map_get_symbol_id_for_name(string const& symbol_name) const;
};

}
