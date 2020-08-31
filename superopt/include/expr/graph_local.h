#pragma once

#include <map>
#include <string>

#include "support/types.h"
#include "expr/expr.h"

namespace eqspace {

using namespace std;

class graph_local_t
{
private:
  string_ref m_name;
  size_t     m_size;
  align_t    m_align;
  bool       m_varsize;
public:
  graph_local_t(string const& name, size_t size, align_t align, bool varsize = false) : graph_local_t(mk_string_ref(name), size, align, varsize)
  { }
  graph_local_t(string_ref const& name, size_t size, align_t align, bool varsize = false) : m_name(name), m_size(size), m_align(align), m_varsize(varsize)
  { }
  string_ref const& get_name() const { return m_name; }
  align_t get_alignment() const { return m_align; }
  size_t get_size() const { return m_size; }
  bool is_varsize() const { return m_varsize; }
};


class graph_locals_map_t
{
private:
  static local_id_t const m_vararg_local_id;
  map<local_id_t, graph_local_t> m_map;
public:
  graph_locals_map_t()
  { }
  graph_locals_map_t(map<local_id_t, graph_local_t> const& m) : m_map(m)
  { }
  graph_locals_map_t(graph_locals_map_t const& other)
  {
    m_map = other.m_map;
  }
  graph_locals_map_t(istream& is);
  graph_local_t const& at(local_id_t l) const
  {
    return m_map.at(l);
  }
  map<local_id_t, graph_local_t> const& get_map() const { return m_map; }
  size_t count(local_id_t l) const
  {
    //cout << __func__ << " " << __LINE__ << ": l = " << l << endl;
    size_t ret = m_map.count(l);
    //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
    return ret;
  }
  bool has_vararg() const
  {
    return m_map.count(m_vararg_local_id) != 0;
  }
  size_t size() const { return m_map.size(); }
  size_t graph_locals_map_get_stack_size() const;
  void graph_locals_map_to_stream(ostream& os) const;

  static local_id_t first_non_arg_local() { return m_vararg_local_id + 1; }
};

//void locals_map_to_stream(graph_locals_map_t const& locals_map, ostream& os);
//graph_locals_map_t locals_map_from_stream(istream& is);

}
