#ifndef __REG_IDENTIFIER_H
#define __REG_IDENTIFIER_H
#include <map>
#include <string>
#include <sstream>
#include <assert.h>
#include <iostream>
#include <map>
#include <string>
#include <cstdlib>
#include <memory>
#include "boost/lexical_cast.hpp"
#include <bits/stdc++.h>
#include "support/types.h"
#include "cutils/chash.h"
#include "valtag/valtag.h"
#include "support/debug.h"
#include "support/utils.h"



class reg_identifier_object_t
{
public:
  virtual std::string reg_identifier_obj_to_string() const = 0;
  virtual unsigned reg_identifier_obj_hash() const = 0;
  virtual ~reg_identifier_object_t()  {}
};

class reg_identifier_int_t : public reg_identifier_object_t
{
public:
  reg_identifier_int_t(exreg_id_t id) : m_id(id) {}
  exreg_id_t reg_identifier_int_get_id() const { return m_id; }
  virtual ~reg_identifier_int_t() {}
  bool reg_identifier_int_is_unassigned() const
  {
    bool ret = (m_id == TRANSMAP_REG_ID_UNASSIGNED);
    //std::cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << this->reg_identifier_obj_to_string() << std::endl;
    return ret;
  }
  bool operator==(reg_identifier_int_t const &other) const
  {
    return m_id == other.m_id;
  }
  bool operator!=(reg_identifier_int_t const &other) const
  {
    return m_id != other.m_id;
  }
  bool operator<(reg_identifier_int_t const &other) const
  {
    return m_id < other.m_id;
  }
  virtual std::string reg_identifier_obj_to_string() const override
  {
    return boost::lexical_cast<std::string>(m_id);
  }
  virtual unsigned reg_identifier_obj_hash() const override
  {
    return (unsigned)m_id;
  }
private:
  exreg_id_t m_id;
};

class reg_identifier_string_t : public reg_identifier_object_t
{
public:
  reg_identifier_string_t(std::string const &id) : m_id(id)
  {
    ASSERT(!stringIsInteger(id));
  }
  virtual ~reg_identifier_string_t() {}
  /*virtual bool reg_identifier_is_valid() const override
  {
    return true;
    //bool ret = m_id != "-1";
    //return ret;
  }*/
  /*virtual std::string reg_identifier_get_str_id() const override
  {
    //assert(reg_identifier_is_valid());
    return m_id;
  }*/
  /*virtual exreg_id_t reg_identifier_get_id() const override
  {
    NOT_REACHED();
  }*/
  bool reg_identifier_string_denotes_constant() const;
  bool reg_identifier_string_represents_arg() const;
  valtag_t reg_identifier_string_get_imm_valtag() const;
  bool operator==(reg_identifier_string_t const &other) const
  {
    return m_id == other.m_id;
  }
  bool operator!=(reg_identifier_string_t const &other) const
  {
    return m_id != other.m_id;
  }
  bool operator<(reg_identifier_string_t const &other) const
  {
    return m_id < other.m_id;
  }
  virtual std::string reg_identifier_obj_to_string() const override
  {
    return m_id;
  }
  virtual unsigned reg_identifier_obj_hash() const override
  {
    return myhash_string(m_id.c_str());
  }
private:
  std::string m_id;
};

namespace std
{

template<>
struct hash<reg_identifier_int_t>
{
  std::size_t operator()(reg_identifier_int_t const &s) const
  {
    return s.reg_identifier_int_get_id();
  }
};

template<>
struct hash<reg_identifier_string_t>
{
  std::size_t operator()(reg_identifier_string_t const &s) const
  {
    return myhash_string(s.reg_identifier_obj_to_string().c_str());
  }
};

}



#include "support/manager_raw.h"



extern manager_raw<reg_identifier_int_t> reg_identifier_int_map;
extern manager_raw<reg_identifier_string_t> reg_identifier_string_map;

class reg_identifier_t
{
public:
  reg_identifier_t(exreg_id_t id)
  {
    ASSERT(id >= 0);
    //m_obj = std::make_shared<reg_identifier_int_t>(id);
    m_obj = reg_identifier_int_map.register_object(reg_identifier_int_t(id));
    //reg_identifier_int_t ri_int(id);
    //m_obj = reg_identifier_int_map.register_object(ri_int);
  }
  reg_identifier_t(std::string const &id)
  {
    ASSERT(id != "-1");
    if (stringIsInteger(id)) {
      m_obj = reg_identifier_int_map.register_object(reg_identifier_int_t(stoi(id)));
    } else {
      m_obj = reg_identifier_string_map.register_object(reg_identifier_string_t(id));
    }
    //m_obj = std::make_shared<reg_identifier_string_t>(id);
    //reg_identifier_string_t ri_string(id);
    //m_obj = reg_identifier_string_map.register_object(ri_string);
  }
  virtual ~reg_identifier_t()
  {
  }

  class cmp_regnum_t
  {
    public:
    bool operator()(reg_identifier_t const &a, reg_identifier_t const &b) const
    {
      if (a.m_obj == b.m_obj) {
        return false;
      }
      if (!a.m_obj && b.m_obj) {
        return true;
      }
      if (a.m_obj && !b.m_obj) {
        return false;
      }
      ASSERT(a.m_obj && b.m_obj);
      reg_identifier_int_t const *a_int_obj = dynamic_cast<reg_identifier_int_t const *>(a.m_obj);
      reg_identifier_int_t const *b_int_obj = dynamic_cast<reg_identifier_int_t const *>(b.m_obj);
      reg_identifier_string_t const *a_str_obj = dynamic_cast<reg_identifier_string_t const *>(a.m_obj);
      reg_identifier_string_t const *b_str_obj = dynamic_cast<reg_identifier_string_t const *>(b.m_obj);
      ASSERT(a_int_obj || a_str_obj);
      ASSERT(b_int_obj || b_str_obj);
      if (a_int_obj && !b_int_obj) {
        return true;
      }
      if (!a_int_obj && b_int_obj) {
        return false;
      }
      if (a_int_obj) {
        ASSERT(b_int_obj);
        return *a_int_obj < *b_int_obj;
      }
      if (a_str_obj) {
        ASSERT(b_str_obj);
        return *a_str_obj < *b_str_obj;
      }
      NOT_REACHED();
    }
  };

  virtual bool reg_identifier_is_valid() const
  {
    return m_obj != nullptr;
  }
  virtual bool reg_identifier_is_unassigned() const
  {
    ASSERT(m_obj != nullptr);
    reg_identifier_int_t const *m_int_obj = dynamic_cast<reg_identifier_int_t const *>(m_obj);
    if (m_int_obj == nullptr) {
      //std::cout << __func__ << " " << __LINE__ << ": returning false for " << this->reg_identifier_to_string() << std::endl;
      return false;
    }
    return m_int_obj->reg_identifier_int_is_unassigned();
  }
  bool reg_identifier_has_id() const
  {
    ASSERT(this->reg_identifier_is_valid());
    ASSERT(m_obj != nullptr);
    reg_identifier_int_t const *m_int_obj = dynamic_cast<reg_identifier_int_t const *>(m_obj);
    if (m_int_obj == nullptr) {
      return false;
    } else {
      return true;
    }
  }
  virtual exreg_id_t reg_identifier_get_id() const
  {
    //std::shared_ptr<reg_identifier_int_t const> m_int_obj = std::dynamic_pointer_cast<reg_identifier_int_t const>(m_obj);
    ASSERT(this->reg_identifier_is_valid());
    ASSERT(m_obj != nullptr);
    reg_identifier_int_t const *m_int_obj = dynamic_cast<reg_identifier_int_t const *>(m_obj);
    if (m_int_obj == nullptr) {
      std::cout << __func__ << " " << __LINE__ << ": this = " << this->reg_identifier_to_string() << std::endl;
    }
    ASSERT(m_int_obj != nullptr);
    return m_int_obj->reg_identifier_int_get_id();
  }
  bool equals_mod_phi_modifier(reg_identifier_t const &other) const
  {
    reg_identifier_string_t const *this_str_obj = dynamic_cast<reg_identifier_string_t const *>(m_obj);
    reg_identifier_string_t const *other_str_obj = dynamic_cast<reg_identifier_string_t const *>(other.m_obj);
    if (!this_str_obj || !other_str_obj) {
      return *this == other;
    } else {
      std::string this_str = this_str_obj->reg_identifier_obj_to_string();
      std::string other_str = other_str_obj->reg_identifier_obj_to_string();
      if (   (this_str.find(other_str) == 0 && string_has_prefix(this_str.substr(other_str.size()), PHI_NODE_TMPVAR_SUFFIX "."))
          || (other_str.find(this_str) == 0 && string_has_prefix(other_str.substr(this_str.size()), PHI_NODE_TMPVAR_SUFFIX "."))) {
        return true;
      } else {
        return this_str == other_str;
      }
    }
  }
  virtual bool operator==(reg_identifier_t const &other) const
  {
    return m_obj == other.m_obj;
    /*if (m_obj == other.m_obj) {
      return true;
    }
    if (!m_obj && other.m_obj) {
      return false;
    }
    if (m_obj && !other.m_obj) {
      return false;
    }
    ASSERT(m_obj && other.m_obj);
    std::shared_ptr<reg_identifier_int_t const> m_int_obj = std::dynamic_pointer_cast<reg_identifier_int_t const>(m_obj);
    std::shared_ptr<reg_identifier_int_t const> other_int_obj = std::dynamic_pointer_cast<reg_identifier_int_t const>(other.m_obj);
    std::shared_ptr<reg_identifier_string_t const> m_str_obj = std::dynamic_pointer_cast<reg_identifier_string_t const>(m_obj);
    std::shared_ptr<reg_identifier_string_t const> other_str_obj = std::dynamic_pointer_cast<reg_identifier_string_t const>(other.m_obj);
    ASSERT(m_int_obj || m_str_obj);
    ASSERT(other_int_obj || other_str_obj);
    if (m_int_obj && !other_int_obj) {
      return false;
    }
    if (!m_int_obj && other_int_obj) {
      return false;
    }
    if (m_int_obj) {
      return *m_int_obj == *other_int_obj;
    }
    if (m_str_obj) {
      return *m_str_obj == *other_str_obj;
    }
    NOT_REACHED();*/
  }
  virtual bool operator!=(reg_identifier_t const &other) const
  {
    return m_obj != other.m_obj;
    //return !(*this == other);
  }
  virtual bool operator<(reg_identifier_t const &other) const
  {
    //return m_obj < other.m_obj; //this is okay and fast but introduces non-determinism
    //assume int_obj < str_obj
    if (m_obj == other.m_obj) {
      return false;
    }
    if (!m_obj && other.m_obj) {
      return true;
    }
    if (m_obj && !other.m_obj) {
      return false;
    }
    ASSERT(m_obj && other.m_obj);
    reg_identifier_int_t const *m_int_obj = dynamic_cast<reg_identifier_int_t const *>(m_obj);
    reg_identifier_int_t const *other_int_obj = dynamic_cast<reg_identifier_int_t const *>(other.m_obj);
    reg_identifier_string_t const *m_str_obj = dynamic_cast<reg_identifier_string_t const *>(m_obj);
    reg_identifier_string_t const *other_str_obj = dynamic_cast<reg_identifier_string_t const *>(other.m_obj);
    ASSERT(m_int_obj || m_str_obj);
    ASSERT(other_int_obj || other_str_obj);
    if (m_int_obj && !other_int_obj) {
      return true;
    }
    if (!m_int_obj && other_int_obj) {
      return false;
    }
    if (m_int_obj) {
      ASSERT(other_int_obj);
      return *m_int_obj < *other_int_obj;
    }
    if (m_str_obj) {
      ASSERT(other_str_obj);
      return *m_str_obj < *other_str_obj;
    }
    NOT_REACHED();
  }
  bool reg_identifier_denotes_constant() const
  {
    ASSERT(m_obj);
    //std::shared_ptr<reg_identifier_string_t const> m_str_obj = std::dynamic_pointer_cast<reg_identifier_string_t const>(m_obj);
    reg_identifier_string_t const *m_str_obj = dynamic_cast<reg_identifier_string_t const *>(m_obj);
    if (!m_str_obj) {
      return false;
    }
    return m_str_obj->reg_identifier_string_denotes_constant();
  }
  bool reg_identifier_represents_arg() const
  {
    ASSERT(m_obj);
    //std::shared_ptr<reg_identifier_string_t const> m_str_obj = std::dynamic_pointer_cast<reg_identifier_string_t const>(m_obj);
    reg_identifier_string_t const *m_str_obj = dynamic_cast<reg_identifier_string_t const *>(m_obj);
    if (!m_str_obj) {
      return false;
    }

    return m_str_obj->reg_identifier_string_represents_arg();
  }
  valtag_t reg_identifier_get_imm_valtag() const
  {
    ASSERT(m_obj);
    //std::shared_ptr<reg_identifier_string_t const> m_str_obj = std::dynamic_pointer_cast<reg_identifier_string_t const>(m_obj);
    reg_identifier_string_t const *m_str_obj = dynamic_cast<reg_identifier_string_t const *>(m_obj);
    ASSERT(m_str_obj);
    return m_str_obj->reg_identifier_string_get_imm_valtag();
  }
  std::string reg_identifier_to_string() const
  {
    ASSERT(m_obj);
    return m_obj->reg_identifier_obj_to_string();
  }
  unsigned reg_identifier_hash() const
  {
    if (!m_obj) {
      return (unsigned)-1;
    }
    return m_obj->reg_identifier_obj_hash();
  }
  static bool is_potential_regid_char(char c)
  {
    return isalpha(c) || isdigit(c) || c == '_' || c == '-' || c == '%';
  }
  static reg_identifier_t const &reg_identifier_invalid()
  {
    static reg_identifier_t ret((reg_identifier_object_t const*)nullptr);
    return ret;
  }
  static reg_identifier_t const &reg_identifier_unassigned()
  {
    static reg_identifier_t ret(TRANSMAP_REG_ID_UNASSIGNED);
    return ret;
  }
  static reg_identifier_t const &get_reg_identifier_for_int(int i)
  {
    static std::vector<reg_identifier_t> vec;
    if (vec.size() == 0) {
      for (int v = 0; v < MAX_NUM_REGS_EXREGS; v++) {
        vec.push_back(reg_identifier_t(v));
      }
    }
    return vec.at(i);
  }
private:
  reg_identifier_t(reg_identifier_object_t const *obj) : m_obj(obj) {}
  reg_identifier_object_t const *m_obj;
};

//reg_identifier_t const &reg_identifier_invalid();
reg_identifier_t get_reg_identifier_for_regname(std::string const &regname);
void reg_identifier_map_insert(std::map<reg_identifier_t, reg_identifier_t> &haystack, std::pair<reg_identifier_t, reg_identifier_t> const &needle);

#endif
