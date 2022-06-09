#pragma once

#include <memory>

#include "expr/memlabel.h"
#include "expr/memlabel_obj.h"

namespace eqspace {

class reachable_memlabels_map_t {
private:
  map<memlabel_ref, set<memlabel_ref>> m_map;
public:
  reachable_memlabels_map_t(map<memlabel_ref, set<memlabel_ref>> const& m) : m_map(m)
  { }
  memlabel_t reachable_memlabels_get(memlabel_t const& ml) const;
  void reachable_memlabels_map_to_stream(ostream& os) const;
  //memlabel_t at(memlabel_t const& ml) const;
  //bool count(memlabel_t const& ml) const;
  //map<memlabel_ref, set<memlabel_ref>> const& get_map() const
  //{
  //  return m_map;
  //}
};

}
