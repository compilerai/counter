#pragma once

#include "support/types.h"
#include "valtag/reg_identifier.h"

class alloc_regname_t {
public:
  typedef enum { SRC_REG, TEMP_REG } alloc_regname_type_t;

  //alloc_regname_t() = delete;
  alloc_regname_t() : m_alloc_regname_type(TEMP_REG), m_group(-1), m_reg_id(reg_identifier_t::reg_identifier_invalid()) {} //needed for alloc_elem_t()
  alloc_regname_t(alloc_regname_type_t alloc_regname_type, exreg_group_id_t group, reg_identifier_t const &reg_id) : m_alloc_regname_type(alloc_regname_type), m_group(group), m_reg_id(reg_id)
  {
  }
  bool equals_mod_phi_modifier(alloc_regname_t const &other) const
  {
    return    true
           && m_alloc_regname_type == other.m_alloc_regname_type
           && m_group == other.m_group
           && m_reg_id.equals_mod_phi_modifier(other.m_reg_id)
    ;
  }
  bool operator==(alloc_regname_t const &other) const
  {
    return    true
           && m_alloc_regname_type == other.m_alloc_regname_type
           && m_group == other.m_group
           && m_reg_id == other.m_reg_id
    ;
  }
  bool operator<(alloc_regname_t const &other) const
  {
    return    false
           || (m_alloc_regname_type < other.m_alloc_regname_type)
           || (m_alloc_regname_type == other.m_alloc_regname_type && m_group < other.m_group)
           || (m_alloc_regname_type == other.m_alloc_regname_type && m_group == other.m_group && m_reg_id < other.m_reg_id);
  }
  std::string alloc_regname_to_string() const
  {
    std::stringstream ss;
    ss << "(" << alloc_regname_type_to_string(m_alloc_regname_type) << ", " << m_group << ", " << m_reg_id.reg_identifier_to_string() << ")";
    return ss.str();
  }
  static alloc_regname_t alloc_regname_from_string(string const &s)
  {
    istringstream iss(s);
    string word;
    char ch;
    iss >> ch;
    ASSERT(ch == '(');
    string typ_str;
    while ((iss >> ch) && (ch != ',')) {
      typ_str.append(1, ch);
    }
    alloc_regname_type_t typ = alloc_regname_type_from_string(typ_str);
    exreg_group_id_t group;
    iss >> group;
    iss >> ch;
    ASSERT(ch == ',');
    iss >> word;
    ASSERT(word.at(word.size() - 1) == ')');
    reg_identifier_t reg_id(word.substr(0, word.size() - 1));
    return alloc_regname_t(typ, group, reg_id); 
  }
  static std::string alloc_regname_type_to_string(alloc_regname_type_t typ)
  {
    if (typ == SRC_REG) return "src";
    else if (typ == TEMP_REG) return "temp";
    else NOT_REACHED();
  }
  static alloc_regname_type_t alloc_regname_type_from_string(string const &s)
  {
    if (s == "src") {
      return SRC_REG;
    } else if (s == "temp") {
      return TEMP_REG;
    } else NOT_REACHED();
  }
  bool alloc_regname_is_dst_temporary() const { return m_alloc_regname_type == TEMP_REG; }
  exreg_group_id_t alloc_regname_get_group() const { return m_group; }
  reg_identifier_t const &alloc_regname_get_reg_id() const { return m_reg_id; }
private:
  alloc_regname_type_t m_alloc_regname_type;
  exreg_group_id_t m_group;
  reg_identifier_t m_reg_id;
};


