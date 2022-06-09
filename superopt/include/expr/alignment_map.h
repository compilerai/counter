#pragma once

#include <map>
#include <string>

#include "expr/memlabel_obj.h"
#include "support/types.h"
#include "gsupport/graph_symbol.h"

namespace eqspace {

class predicate;
class consts_struct_t;

class alignment_map_t
{
private:
  map<memlabel_ref,align_t> m_map;

  alignment_map_t(map<memlabel_ref,align_t> const& map) : m_map(map) { }
public:

  align_t get(memlabel_ref const& m) const
  {
    if (m_map.count(m))
      return m_map.at(m);
    return 1; // default alignment
  }

  string to_string() const;

  //friend alignment_map_t create_alignment_map_from_predicate_set(consts_struct_t const& cs, unordered_set<predicate> const& pred_set, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map);
  //friend alignment_map_t create_alignment_map_from_symbol_map(consts_struct_t const& cs, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map);
};

//alignment_map_t create_alignment_map_from_predicate_set(consts_struct_t const& cs, unordered_set<predicate> const& pred_set, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map);
//alignment_map_t create_alignment_map_from_symbol_map(consts_struct_t const& cs, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map);

}
