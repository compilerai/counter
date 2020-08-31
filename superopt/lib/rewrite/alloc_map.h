#ifndef STACK_SLOT_MAP_H
#define STACK_SLOT_MAP_H
#include <map>
#include <algorithm>
#include <set>
#include "support/debug.h"
#include "support/types.h"
#include "valtag/reg_identifier.h"
#include "rewrite/stack_offset.h"
#include "rewrite/alloc_regname.h"

template<typename VALTYPE>
class alloc_map_t {
private:
  std::map<alloc_regname_t, VALTYPE> m_map;
public:
  std::map<alloc_regname_t, VALTYPE> const &get_map() const
  {
    return m_map;
  }
  bool ssm_entry_exists(alloc_regname_t const &alloc_regname/*exreg_id_t group, reg_identifier_t const &reg_id*/) const
  {
    return m_map.count(alloc_regname);
    //return    m_map.count(group)
    //       && m_map.at(group).count(reg_id);
  }
  /*dst_long ssm_get_offset(exreg_id_t group, reg_identifier_t const &reg_id) const
  {
    ASSERT(m_map.count(group));
    ASSERT(m_map.at(group).count(reg_id));
    return m_map.at(group).at(reg_id).get_offset();
  }
  bool ssm_get_is_arg(exreg_id_t group, reg_identifier_t const &reg_id) const
  {
    ASSERT(m_map.count(group));
    ASSERT(m_map.at(group).count(reg_id));
    return m_map.at(group).at(reg_id).get_is_arg();
  }*/

  VALTYPE const &ssm_get_value(alloc_regname_t const &alloc_regname/*exreg_id_t group, reg_identifier_t const &reg_id*/) const
  {
    //ASSERT(m_map.count(group));
    //ASSERT(m_map.at(group).count(reg_id));
    //return m_map.at(group).at(reg_id);
    ASSERT(m_map.count(alloc_regname));
    return m_map.at(alloc_regname);
  }

  void ssm_add_entry(alloc_regname_t const &alloc_regname/*exreg_id_t group, reg_identifier_t const &reg_id*/, VALTYPE const &off)
  {
    m_map.insert(make_pair(alloc_regname, off));
    //m_map[group].insert(std::make_pair(reg_id, off));
  }
  std::set<VALTYPE> get_used_stack_slots() const
  {
    std::set<VALTYPE> ret;
    //for (const auto &g : m_map) {
    //  for (const auto &r : g.second) {
    //    ret.insert(r.second);
    //  }
    //}
    for (const auto &r : m_map) {
      ret.insert(r.second);
    }
    return ret;
  }
  void alloc_map_union(alloc_map_t<VALTYPE> const &other)
  {
    for (const auto &mm : other.m_map) {
      if (!m_map.count(mm.first)) {
        m_map.insert(mm);
      }
    }
  }
  std::string to_string() const
  {
    std::ostringstream ss;
    for (const auto &g : m_map) {
      /*for (const auto &r : g.second) */{
        //ss << g.first << "." << r.first.reg_identifier_to_string() << ": ";
        ss << g.first.alloc_regname_to_string() << ": ";
        ss << g.second << "\n";
      }
    }
    return ss.str();
  }
};

#endif
