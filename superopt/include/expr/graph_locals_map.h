#pragma once

#include "support/types.h"

#include "expr/expr.h"
#include "expr/graph_local.h"

namespace eqspace {

using namespace std;

class graph_locals_map_t
{
private:
  static allocsite_t const m_vararg_local_id;
  map<allocsite_t, graph_local_t> m_map;
public:
  graph_locals_map_t()
  { }
  graph_locals_map_t(map<allocsite_t, graph_local_t> const& m) : m_map(m)
  { }
  graph_locals_map_t(istream& is);
  set<allocsite_t> graph_locals_map_get_allocsites() const;
  graph_local_t const& at(allocsite_t const& l) const
  {
    return m_map.at(l);
  }
  map<allocsite_t, graph_local_t> const& get_map() const { return m_map; }
  pair<bool,allocsite_t> get_id_for_name(string const& name) const;
  size_t count(allocsite_t l) const
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

  static allocsite_t arg_local_id(argnum_t argnum);
  static allocsite_t vararg_local_id();
};

}
