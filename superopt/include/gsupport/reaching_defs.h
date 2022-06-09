#pragma once

#include "expr/context.h"

#include "gsupport/tfg_edge.h"
#include "gsupport/tfg_edge_set.h"

template <typename T_VAL, typename T_E>
class reaching_defs_t {
private:
  //map<expr_ref, set<tfg_edge_ref>> m_map;
  map<T_VAL, set<T_E> > m_map;
public:
  reaching_defs_t() : m_map()
  { }
  reaching_defs_t(map<T_VAL, set<T_E> > const& m) : m_map(m)
  { }
  reaching_defs_t(reaching_defs_t const& other) : m_map(other.m_map)
  { }
  static reaching_defs_t top()
  {
    reaching_defs_t ret;
    return ret;
  }
  void kill_and_add_reaching_definition(T_VAL const& e, T_E const& te)
  {
    // replace old set with this new value
//    if (!m_map.count(e)) {
      m_map[e] = {te};
//    } else {
//      auto s = m_map.at(e)->get_set();
//      s.insert(te);
//      m_map.at(e) = tfg_edge_set_t::mk_tfg_edge_set(s);
//    }
  }
  void add_reaching_definition(T_VAL const& e, T_E const& te)
  {
    if (!m_map.count(e)) {
      m_map[e] = {te};
    } else {
      auto s = m_map.at(e);
      s.insert(te);
      m_map.at(e) = s;
    }
  }
  map<T_VAL, set<T_E> > const& get_map() const { return m_map; }
  bool reaching_definitions_union(reaching_defs_t const& other) //returns true if something changed
  {
    bool changed = false;
    for (auto const& om : other.m_map) {
      if (!m_map.count(om.first)) {
        m_map[om.first] = om.second;
        changed = true;
      } else {
        auto old = m_map[om.first];
        auto s = old;
//        auto s = old->get_set();
        set_union(s, om.second);
        m_map[om.first] = s;
        changed = changed || (old != s);
      }
    }
    return changed;
  }
};
